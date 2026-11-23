/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

/**
 * SECTION: arvgvstream
 * @short_description: GigEVision stream
 */

#include <arvdebugprivate.h>
#include <arvgvstreamprivate.h>
#include <arvgvdeviceprivate.h>
#include <arvstreamprivate.h>
#include <arvbufferprivate.h>
#include <arvfeatures.h>
#include <arvparamsprivate.h>
#include <arvgvspprivate.h>
#include <arvgvcpprivate.h>
#include <arvdebug.h>
#include <arvmisc.h>
#include <arvmiscprivate.h>
#include <arvnetworkprivate.h>
#include <arvrealtime.h>
#include <arvstr.h>
#include <arvenumtypes.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#if ARAVIS_HAS_PACKET_SOCKET
#include <ifaddrs.h>
#include <netinet/udp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/filter.h>
#include <sys/types.h>
#include <sys/mman.h>
#endif

#ifdef __APPLE__
#include <mach/mach.h>
#include <pthread.h>
#include <sys/sysctl.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#endif

#define ARV_GV_STREAM_DISCARD_LATE_FRAME_THRESHOLD	100
#define ARV_GV_STREAM_THREAD_STATS_PERIOD_US		100000
#define ARV_GV_STREAM_BUFFER_SIZE_PROTOCOL_OVERHEAD     1024 /* Some room for protocol overhead (IP + UDP + GV) */
#define ARV_GV_STREAM_MIN_BUFFER_SIZE                   20 * 1024
#define ARV_GV_STREAM_SOCKET_BUFFER_FLOOR               (1024 * 1024)
#define ARV_GV_STREAM_SOCKET_BUFFER_MAX_REQUEST         (64 * 1024 * 1024) /* "max" where the limit can't be read */
#define ARV_GV_STREAM_RX_BATCH_DEFAULT                  64
#define ARV_GV_STREAM_RX_BATCH_MAX                      256  /* kern.ipc.maxrecvmsgx */
#define ARV_GV_STREAM_RX_LOWAT_PACKETS_DEFAULT          16
#define ARV_GV_STREAM_RX_ACTIVE_WAIT_US                 1000 /* longest wait while a stream is active */
#define ARV_GV_STREAM_RX_ACTIVE_US                      100000 /* active: frames open or a packet this recently */
#define ARV_GV_STREAM_RELEARN_IDLE_US                   1000000 /* connected socket silent this long: learn again */
#define ARV_GV_STREAM_MAX_FOREIGN_SOURCES               64     /* unqualified senders before connecting is given up */
#define ARV_GV_STREAM_RX_MAX_ERRORS                     100    /* consecutive recvmsg_x failures before _loop */
#define ARV_GV_STREAM_FAST_FAIL_REORDER_PACKETS         16     /* first packet ids that may still see their leader */

enum {
	ARV_GV_STREAM_PROPERTY_0,
	ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER,
	ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER_SIZE,
	ARV_GV_STREAM_PROPERTY_PACKET_RESEND,
	ARV_GV_STREAM_PROPERTY_PACKET_REQUEST_RATIO,
	ARV_GV_STREAM_PROPERTY_INITIAL_PACKET_TIMEOUT,
	ARV_GV_STREAM_PROPERTY_PACKET_TIMEOUT,
	ARV_GV_STREAM_PROPERTY_FRAME_RETENTION
} ArvGvStreamProperties;

typedef struct _ArvGvStreamThreadData ArvGvStreamThreadData;

typedef struct {
        ArvGvDevice *gv_device;

        guint stream_channel;

	GThread *thread;
	ArvGvStreamThreadData *thread_data;
} ArvGvStreamPrivate;

struct _ArvGvStream {
        ArvStream stream;
};

struct _ArvGvStreamClass {
	ArvStreamClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvGvStream, arv_gv_stream, ARV_TYPE_STREAM, G_ADD_PRIVATE (ArvGvStream))

/* Acquisition thread */

typedef struct {
	gboolean received;
        gboolean resend_requested;
	guint64 abs_timeout_us;
} ArvGvStreamPacketData;

typedef struct {
	ArvBuffer *buffer;
	guint64 frame_id;

        gboolean leader_received;

        gsize received_size;

	gint32 last_valid_packet;
	guint64 first_packet_time_us;
	guint64 last_packet_time_us;

	gboolean disable_resend_request;

	guint n_packets;
	ArvGvStreamPacketData *packet_data;

	guint n_packet_resend_requests;
	gboolean resend_ratio_reached;

	/* The trailer was processed: n_packets is the device's count, not the buffer size estimate */
	gboolean trailer_received;
	/* When the device first answered a resend request with a status packet (disable_resend_request) */
	guint64 disable_resend_us;

	/* Fast-fail bookkeeping of _missing_packet_check: every missing packet up to max_checked_packet has its
	 * deadline, and next_check_us is the earliest of them (G_MAXUINT64 when none is pending). */
	gint32 max_checked_packet;
	guint64 next_check_us;

	gboolean extended_ids;
} ArvGvStreamFrameData;

struct _ArvGvStreamThreadData {
	GCancellable *cancellable;

	ArvStream *stream;

        gboolean thread_started;
        GMutex thread_started_mutex;
        GCond thread_started_cond;

	ArvStreamCallback callback;
	void *callback_data;

	GSocket *socket;
	GInetAddress *interface_address;
	GSocketAddress *interface_socket_address;
	GInetAddress *device_address;
	GSocketAddress *device_socket_address;
	guint16 source_stream_port;
	guint16 stream_port;

	ArvGvStreamPacketResend packet_resend;
	double packet_request_ratio;
	guint initial_packet_timeout_us;
	guint packet_timeout_us;
	guint frame_retention_us;

	/* ARV_GV_STREAM_FAST_FAIL, read once at construction: once packets are lost, do not start frames that cannot
	 * complete, release hopeless ones promptly, bound the missing packet scans and ask for a lost tail as soon as
	 * the next frame shows the sender is done. Exposed as the "fast_fail" info (0 or 1). */
	guint64 fast_fail;

	guint64 timestamp_tick_frequency;
	guint scps_packet_size;

	guint16 packet_id;

	GSList *frames;
	gboolean first_packet;
	guint64 last_frame_id;

	/* Fast fail: the frame dropped without a buffer (skipped, or no input buffer at its first packet), whose
	 * remaining packets are ignored without looking for a buffer again, and the packet count of the latest frame,
	 * the estimate of the next one's. The record lasts until the next frame starts (or the thread restarts):
	 * from then on the dropped frame's stragglers are late packets, and the same frame id coming round again
	 * (a 16 bit id wraps every 65535 frames, and a device may restart its ids) is a new frame. */
	gboolean has_dropped_frame;
	guint64 dropped_frame_id;
	gboolean dropped_frame_underrun;
	guint last_n_packets;

	gboolean use_packet_socket;

	/* Statistics */

	guint64 n_completed_buffers;
	guint64 n_failures;
	guint64 n_underruns;
	guint64 n_timeouts;
	guint64 n_aborted;
	guint64 n_missing_frames;

	guint64 n_size_mismatch_errors;

	guint64 n_received_packets;
	guint64 n_missing_packets;
	guint64 n_error_packets;
	guint64 n_ignored_packets;
	guint64 n_resend_requests;
	guint64 n_resent_packets;
	guint64 n_resend_ratio_reached;
        guint64 n_resend_disabled;
	guint64 n_duplicated_packets;

        guint64 n_transferred_bytes;
        guint64 n_ignored_bytes;

	/* Receive path diagnostics */

	guint64 socket_buffer_size_granted;	/* "socket_buffer_size": SO_RCVBUF read back, 0 if never set */
	guint64 n_underrun_frames;		/* distinct frames that found no input buffer */
	guint64 n_skipped_frames;		/* fast-fail: frames dropped without taking a buffer */
	guint64 last_underrun_frame_id;
	gboolean has_underrun_frame;
	guint64 n_rx_wakeups;			/* poll returned with the socket readable */
	guint64 n_rx_batches;			/* receive calls returning at least one packet */
	guint64 n_rx_max_batch;			/* most packets returned by one receive call */
	guint64 stream_thread_cpu_us;		/* user + system CPU time of the stream thread (Apple only) */
	guint64 thread_priority_mode;		/* 0 default, 1 high (QoS), 2 realtime: what was achieved */
	guint64 stream_thread_min_priority;	/* lowest scheduler priority observed (Apple only) */
	guint64 last_thread_stats_us;
	guint64 rx_mode;			/* 0 legacy _loop, 1 recvmsg_x (Apple only), 2 packet socket (Linux) */
	guint64 n_rx_truncated;			/* recvmsg_x slots that came back full, not processed */
	guint64 n_short_packets;		/* datagrams too short for their GVSP headers, not processed */

	/* Connected stream socket (ARV_GV_STREAM_CONNECT, see _connect_to_source) */
	gboolean connect_socket;		/* connect once the sender is known from traffic */
	guint64 socket_connected;		/* 1 while the stream socket is connected to the sender */
	guint64 n_rx_foreign_source;		/* datagrams seen unconnected from another address, or the GVCP port */
	guint n_learn_foreign_sources;		/* the same, since learning last (re)started */
	guint64 n_socket_reconnects;		/* connections dissolved to learn the sender again */
	guint64 last_datagram_us;		/* time of the last datagram received */
	guint32 device_address_be;		/* device IPv4 address, network order, for the raw source check */
	guint64 n_resend_send_errors;		/* PACKETRESEND commands the socket refused to send */
	GSocket *resend_socket;			/* sends PACKETRESEND while the stream socket is connected */

	ArvHistogram *histogram;
	guint32 statistic_count;

	ArvGvStreamSocketBuffer socket_buffer_option;
	int socket_buffer_size;
	int current_socket_buffer_size;
	int last_attempted_socket_buffer_size;	/* never retried: a refused size is refused every time */
	int configured_socket_buffer_size;	/* obtained by _configure_socket_buffer, the floor for _update_socket */
};

/* Tells arv_gv_stream_start_thread that the receive loop is running. Once per thread start: a receive loop that
 * gives up and hands over to another (_loop_darwin to _loop) does not signal again. */

static void
_signal_thread_started (ArvGvStreamThreadData *thread_data)
{
	g_mutex_lock (&thread_data->thread_started_mutex);
	if (!thread_data->thread_started) {
		thread_data->thread_started = TRUE;
		g_cond_signal (&thread_data->thread_started_cond);
	}
	g_mutex_unlock (&thread_data->thread_started_mutex);
}

static void
_send_packet_request (ArvGvStreamThreadData *thread_data,
		      guint64 frame_id,
		      guint32 first_block,
		      guint32 last_block,
		      gboolean extended_ids)
{
	ArvGvcpPacket *packet;
	size_t packet_size;
	GSocket *socket;
	GError *error = NULL;

	thread_data->packet_id = arv_gvcp_next_packet_id (thread_data->packet_id);

	packet = arv_gvcp_packet_new_packet_resend_cmd (frame_id, first_block, last_block, extended_ids,
							thread_data->packet_id, &packet_size);

	arv_debug_stream_thread ("[GvStream::send_packet_request] frame_id = %" G_GUINT64_FORMAT
			       " (from packet %" G_GUINT32_FORMAT " to %" G_GUINT32_FORMAT ")",
			       frame_id, first_block, last_block);

	arv_gvcp_packet_debug (packet, ARV_DEBUG_LEVEL_DEBUG);

	/* A connected stream socket can't send to the GVCP port (EISCONN): the request goes out on its own
	 * socket, bound to the same interface address */
	socket = thread_data->resend_socket != NULL ? thread_data->resend_socket : thread_data->socket;

	if (g_socket_send_to (socket, thread_data->device_socket_address, (const char *) packet, packet_size,
			      NULL, &error) < 0) {
		thread_data->n_resend_send_errors++;
		if (thread_data->n_resend_send_errors == 1)
			arv_warning_stream_thread ("[GvStream::send_packet_request] Can't send a packet resend"
						   " request (%s); further failures are counted in"
						   " n_resend_send_errors", error != NULL ? error->message : "unknown");
		g_clear_error (&error);
	}

	arv_gvcp_packet_free (packet);
}

/* ARV_GV_STREAM_CONNECT (1 or 0; unset: on for Apple, off elsewhere). On macOS the kernel charges far more
 * per datagram for an unconnected UDP socket receiving from a non-loopback address than for a connected one
 * (standalone benchmark, 8968-byte datagrams, recvmsg_x batches of 64: 8.4 to 11.8 us against 1.4 to 2.0 us
 * per datagram, and the unconnected receiver lost packets). The stream socket is therefore connected to the
 * sender, which is learnt from the first valid datagram received from the device address, not from the
 * ArvGevSCSP register: a device that reports a wrong source port there would otherwise see every packet
 * filtered out. Datagrams queued before connect() stay readable (checked on macOS 26, loopback and
 * bridge101). After connect(), the kernel drops datagrams from any other address or port.
 *
 * The sender is learnt again (_disconnect_from_source) at every stream thread start, and when the connected
 * socket has received nothing for ARV_GV_STREAM_RELEARN_IDLE_US (1 s): a device that comes back from a reboot,
 * or picks a new source port for an acquisition, would otherwise be filtered out by the kernel for the life of
 * the stream, showing only as "no packets". A normal stream is never silent for 1 s; a slow triggered one can
 * be, and then pays one learning step (a few datagrams received unconnected) per gap, losing nothing.
 * n_socket_reconnects counts these.
 *
 * While unconnected, a valid datagram from another address, or from the device's GVCP port, is counted in
 * n_rx_foreign_source and processed as before. After ARV_GV_STREAM_MAX_FOREIGN_SOURCES (64) of them in one
 * learning phase, connecting is given up for this stream (one warning), so that a sender that never qualifies
 * does not make every datagram pay for its source address. The check compares the raw sockaddr_in, without
 * allocating.
 *
 * Returns TRUE when no further source address is needed: the socket is connected, or connecting failed or was
 * given up and is not retried. */

static gboolean
_connect_to_source (ArvGvStreamThreadData *thread_data, const struct sockaddr *native, gsize native_size)
{
	const struct sockaddr_in *source;
	GSocketAddress *source_address;
	GError *error = NULL;

	if (native == NULL || native_size < sizeof (struct sockaddr_in) || native->sa_family != AF_INET)
		return FALSE;

	source = (const struct sockaddr_in *) native;

	/* GVSP never comes from the GVCP port: a reply from there (a device answering a resend request sent
	 * from this socket before the connection) must not become the only accepted source */
	if (source->sin_addr.s_addr != thread_data->device_address_be ||
	    source->sin_port == g_htons (ARV_GVCP_PORT)) {
		thread_data->n_rx_foreign_source++;
		thread_data->n_learn_foreign_sources++;
		if (thread_data->n_learn_foreign_sources >= ARV_GV_STREAM_MAX_FOREIGN_SOURCES) {
			arv_warning_stream_thread ("[GvStream::connect_to_source] %u datagrams from a source other"
						   " than the device's stream port, the stream socket stays"
						   " unconnected", thread_data->n_learn_foreign_sources);
			thread_data->connect_socket = FALSE;
			return TRUE;
		}
		return FALSE;
	}

	/* The resend socket first: without it, a connected stream socket could not send resend requests. It is
	 * kept when the socket is disconnected, and reused when it connects again. */
	if (thread_data->resend_socket == NULL) {
		GSocket *resend_socket;

		resend_socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP,
					      &error);
		if (resend_socket != NULL) {
			GSocketAddress *resend_address;

			g_socket_set_blocking (resend_socket, FALSE);
			/* Same interface address as the stream socket, any port of the configured GV port range */
			resend_address = arv_socket_bind_with_range (resend_socket, thread_data->interface_address, 0,
								     FALSE, &error);
			if (resend_address == NULL)
				g_clear_object (&resend_socket);
			g_clear_object (&resend_address);
		}
		if (resend_socket == NULL) {
			arv_warning_stream_thread ("[GvStream::connect_to_source] Can't create the resend socket"
						   " (%s), the stream socket stays unconnected",
						   error != NULL ? error->message : "unknown");
			g_clear_error (&error);
			thread_data->connect_socket = FALSE;
			return TRUE;
		}
		thread_data->resend_socket = resend_socket;
	}

	source_address = g_socket_address_new_from_native ((gpointer) native, native_size);
	if (source_address == NULL || !g_socket_connect (thread_data->socket, source_address, NULL, &error)) {
		arv_warning_stream_thread ("[GvStream::connect_to_source] Can't connect the stream socket (%s),"
					   " it stays unconnected", error != NULL ? error->message : "unknown");
		g_clear_error (&error);
		g_clear_object (&source_address);
		thread_data->connect_socket = FALSE;
		return TRUE;
	}
	g_object_unref (source_address);

	thread_data->socket_connected = 1;

	arv_info_stream_thread ("[GvStream::connect_to_source] Stream socket connected to the sender port %u"
				" (ArvGevSCSP reads %u)", g_ntohs (source->sin_port), thread_data->source_stream_port);

	return TRUE;
}

/* Dissolves the association made by _connect_to_source, so that the next valid datagram from the device teaches
 * the sender again. BSD: connect() to an AF_UNSPEC address disconnects a UDP socket; Darwin then fails the
 * reconnection to that address with EAFNOSUPPORT, which is the documented success, Linux returns 0. Winsock
 * disconnects on a connect() to the any address. The local port is kept (Darwin widens the local address to
 * INADDR_ANY until the next connect(), checked on macOS 26), so the device's datagrams still arrive. */

static void
_disconnect_from_source (ArvGvStreamThreadData *thread_data, const char *reason)
{
	int fd = g_socket_get_fd (thread_data->socket);
	int result;
	int errsv;
#ifdef G_OS_WIN32
	struct sockaddr_in any;

	memset (&any, 0, sizeof (any));
	any.sin_family = AF_INET;
	result = connect (fd, (struct sockaddr *) &any, sizeof (any));
	errsv = result == 0 ? 0 : WSAGetLastError ();
#else
	struct sockaddr unspecified;

	memset (&unspecified, 0, sizeof (unspecified));
	unspecified.sa_family = AF_UNSPEC;
#ifdef __APPLE__
	unspecified.sa_len = sizeof (unspecified);
#endif
	result = connect (fd, &unspecified, sizeof (unspecified));
	errsv = result == 0 ? 0 : errno;
	if (result != 0 && errsv == EAFNOSUPPORT)
		result = 0;
#endif

	if (result != 0) {
		/* Still connected to the old sender: never retried, and nothing more is learnt */
		arv_warning_stream_thread ("[GvStream::disconnect_from_source] Can't disconnect the stream socket"
					   " (%s): it stays connected to the sender it learnt", g_strerror (errsv));
		thread_data->connect_socket = FALSE;
		return;
	}

	thread_data->socket_connected = 0;
	thread_data->n_learn_foreign_sources = 0;
	thread_data->n_socket_reconnects++;

	arv_info_stream_thread ("[GvStream::disconnect_from_source] Stream socket disconnected (%s), learning the"
				" sender again", reason);
}

/* Called when the receive loop wakes without data */

static inline void
_check_source_idle (ArvGvStreamThreadData *thread_data, guint64 time_us)
{
	if (G_UNLIKELY (thread_data->socket_connected && thread_data->connect_socket &&
			time_us - thread_data->last_datagram_us >= ARV_GV_STREAM_RELEARN_IDLE_US))
		_disconnect_from_source (thread_data, "no datagram for 1 s");
}

static guint64
_read_socket_buffer_size (int fd)
{
#ifdef G_OS_WIN32
	DWORD size = 0;
#else
	gint size = 0;
#endif
	socklen_t optlen = sizeof (size);

	if (getsockopt (fd, SOL_SOCKET, SO_RCVBUF, (char *) &size, &optlen) != 0)
		return 0;
#ifndef G_OS_WIN32
	/* DWORD is unsigned */
	if (size < 0)
		return 0;
#endif

	return size;
}

/* SO_RCVBUF as read back, in the units of a request: Linux stores and reports twice the size it was asked for (the
 * other half is its bookkeeping overhead), and clamps the request to net.core.rmem_max before doubling it. Compared
 * with requested sizes, the raw value would make a clamped request look granted. */

static int
_granted_in_request_units (int granted_size)
{
#ifdef __linux__
	return granted_size / 2;
#else
	return granted_size;
#endif
}

/* Samples the stream thread's CPU time and scheduler priority, at most every
 * ARV_GV_STREAM_THREAD_STATS_PERIOD_US unless forced. Only implemented on Apple,
 * where a real-time thread demoted by the kernel's fail-safe shows up as a drop
 * of the observed priority. Must be called from the stream thread. */

static void
_sample_thread_stats (ArvGvStreamThreadData *thread_data, guint64 time_us, gboolean force)
{
#ifdef __APPLE__
	thread_basic_info_data_t basic_info;
	thread_extended_info_data_t extended_info;
	mach_msg_type_number_t count;
	thread_act_t thread;

	if (!force && time_us - thread_data->last_thread_stats_us < ARV_GV_STREAM_THREAD_STATS_PERIOD_US)
		return;

	thread_data->last_thread_stats_us = time_us;

	/* pthread_mach_thread_np does not add a port reference, unlike mach_thread_self */
	thread = pthread_mach_thread_np (pthread_self ());

	count = THREAD_BASIC_INFO_COUNT;
	if (thread_info (thread, THREAD_BASIC_INFO, (thread_info_t) &basic_info, &count) == KERN_SUCCESS)
		thread_data->stream_thread_cpu_us =
			(guint64) basic_info.user_time.seconds * 1000000 + basic_info.user_time.microseconds +
			(guint64) basic_info.system_time.seconds * 1000000 + basic_info.system_time.microseconds;

	count = THREAD_EXTENDED_INFO_COUNT;
	if (thread_info (thread, THREAD_EXTENDED_INFO, (thread_info_t) &extended_info, &count) == KERN_SUCCESS &&
	    extended_info.pth_curpri >= 0) {
		if (thread_data->stream_thread_min_priority == 0 ||
		    (guint64) extended_info.pth_curpri < thread_data->stream_thread_min_priority)
			thread_data->stream_thread_min_priority = extended_info.pth_curpri;
	}
#endif
}

static void
_update_socket (ArvGvStreamThreadData *thread_data, ArvBuffer *buffer)
{
	int buffer_size = thread_data->current_socket_buffer_size;
	int granted_size = 0;
	gboolean result;
	int fd;

	if (thread_data->socket_buffer_option == ARV_GV_STREAM_SOCKET_BUFFER_FIXED &&
	    thread_data->socket_buffer_size <= 0)
		return;

	fd = g_socket_get_fd (thread_data->socket);

	switch (thread_data->socket_buffer_option) {
		case ARV_GV_STREAM_SOCKET_BUFFER_FIXED:
			buffer_size = thread_data->socket_buffer_size;
			break;
		case ARV_GV_STREAM_SOCKET_BUFFER_AUTO:
			if (thread_data->socket_buffer_size <= 0)
				buffer_size = buffer->priv->allocated_size + ARV_GV_STREAM_BUFFER_SIZE_PROTOCOL_OVERHEAD;
			else
				buffer_size = MIN (buffer->priv->allocated_size +
                                                   ARV_GV_STREAM_BUFFER_SIZE_PROTOCOL_OVERHEAD,
                                                   thread_data->socket_buffer_size);
			break;
	}

        buffer_size = MAX (buffer_size, ARV_GV_STREAM_MIN_BUFFER_SIZE);

	/* Never shrink the buffer obtained at thread start */
	if (buffer_size <= thread_data->configured_socket_buffer_size)
		return;

	/* This runs for every new frame: a size that was already tried, whether it was granted or not,
	 * is not tried again. */
	if (buffer_size == thread_data->current_socket_buffer_size ||
	    buffer_size == thread_data->last_attempted_socket_buffer_size)
		return;

	thread_data->last_attempted_socket_buffer_size = buffer_size;

	result = arv_socket_set_recv_buffer_size_full (fd, buffer_size, &granted_size);
	if (granted_size > 0)
		thread_data->socket_buffer_size_granted = granted_size;

	if (result) {
		thread_data->current_socket_buffer_size = buffer_size;
		if (_granted_in_request_units (granted_size) < buffer_size)
			arv_warning_stream_thread ("[GvStream::update_socket] Socket buffer size %d requested,"
						   " the kernel granted %d; you might see missing packets",
						   buffer_size, granted_size);
		else
			arv_info_stream_thread ("[GvStream::update_socket] Socket buffer size set to %d (granted %d)",
						buffer_size, granted_size);
	} else {
		arv_warning_stream_thread ("[GvStream::update_socket] Failed to set socket buffer size to %d (%s),"
					   " current size %d",
					   buffer_size, g_strerror (errno), granted_size);
	}
}

static int
_get_max_socket_buffer_size (void)
{
#ifdef __APPLE__
	int max_size = 0;
	size_t length = sizeof (max_size);

	if (sysctlbyname ("kern.ipc.maxsockbuf", &max_size, &length, NULL, 0) == 0 && max_size > 0)
		return max_size;

	arv_warning_stream_thread ("[GvStream::configure_socket_buffer] Can't read kern.ipc.maxsockbuf (%s)",
				   g_strerror (errno));
#endif
	/* Linux clamps silently to net.core.rmem_max; the granted size is read back */
	return ARV_GV_STREAM_SOCKET_BUFFER_MAX_REQUEST;
}

/* Sizes the socket receive buffer once, before any receive loop runs.
 *
 * ARV_GV_STREAM_SOCKET_BUFFER_SIZE: a byte count, "max" (as large as the kernel grants), or "0" /
 * "default" to leave the OS default. Unset: "max" on Apple, where the default (786,896 bytes,
 * under a millisecond of a 10GigE stream) loses most packets of a fast camera; elsewhere the
 * socket-buffer-size property if socket-buffer is FIXED, otherwise nothing.
 *
 * On macOS a request above kern.ipc.maxsockbuf is clamped silently the first time and refused with
 * ENOBUFS when the buffer is already at the maximum, so the granted size is always read back. */

static void
_configure_socket_buffer (ArvGvStreamThreadData *thread_data)
{
	const char *env;
	const char *source = "ARV_GV_STREAM_SOCKET_BUFFER_SIZE";
	gboolean use_max = FALSE;
	gint64 request = 0;
	int granted_size = 0;
	gboolean result = FALSE;
	int fd;

	fd = g_socket_get_fd (thread_data->socket);
	env = g_getenv ("ARV_GV_STREAM_SOCKET_BUFFER_SIZE");

	if (env == NULL) {
#ifdef __APPLE__
		use_max = TRUE;
#else
		/* A fixed socket-buffer-size is known now. Left to _update_socket, it would only be applied when the
		 * first frame starts, after that frame's first burst has met the OS default buffer. */
		if (thread_data->socket_buffer_option != ARV_GV_STREAM_SOCKET_BUFFER_FIXED ||
		    thread_data->socket_buffer_size <= 0)
			return;
		request = thread_data->socket_buffer_size;
		source = "socket-buffer-size";
#endif
	} else if (g_ascii_strcasecmp (env, "max") == 0) {
		use_max = TRUE;
	} else if (g_ascii_strcasecmp (env, "default") == 0 || g_strcmp0 (env, "0") == 0) {
		arv_info_stream_thread ("[GvStream::configure_socket_buffer] Socket buffer left at the OS default"
					" (%" G_GUINT64_FORMAT " bytes)", _read_socket_buffer_size (fd));
		return;
	} else {
		char *end = NULL;

		request = g_ascii_strtoll (env, &end, 10);
		if (end == env || *end != '\0' || request <= 0 || request > G_MAXINT) {
			arv_warning_stream_thread ("[GvStream::configure_socket_buffer] Invalid"
						   " ARV_GV_STREAM_SOCKET_BUFFER_SIZE '%s' (expected a byte count,"
						   " 'max', '0' or 'default'); socket buffer left at the OS default",
						   env);
			return;
		}
	}

	if (use_max) {
		request = _get_max_socket_buffer_size ();

		for (;;) {
			result = arv_socket_set_recv_buffer_size_full (fd, request, &granted_size);
			if (result && _granted_in_request_units (granted_size) >= ARV_GV_STREAM_SOCKET_BUFFER_FLOOR)
				break;
			if (request <= ARV_GV_STREAM_SOCKET_BUFFER_FLOOR)
				break;
			request = MAX (request * 7 / 8, ARV_GV_STREAM_SOCKET_BUFFER_FLOOR);
		}
	} else {
		result = arv_socket_set_recv_buffer_size_full (fd, request, &granted_size);
	}

	if (granted_size > 0)
		thread_data->socket_buffer_size_granted = granted_size;

	if (result) {
		/* The floor is what was asked and granted, in request units (Linux reports twice the size it stores) */
		thread_data->configured_socket_buffer_size = MIN (request, _granted_in_request_units (granted_size));
		thread_data->current_socket_buffer_size = thread_data->configured_socket_buffer_size;
		arv_info_stream_thread ("[GvStream::configure_socket_buffer] Socket buffer: requested %" G_GINT64_FORMAT
					" bytes (%s), granted %d bytes",
					request, use_max ? "max" : source, granted_size);
	} else {
		arv_warning_stream_thread ("[GvStream::configure_socket_buffer] Failed to set socket buffer to %"
					   G_GINT64_FORMAT " bytes (%s), current size %d bytes",
					   request, g_strerror (errno), granted_size);
	}
}

static unsigned int
_compute_n_expected_packets (const ArvGvspPacket *packet, size_t allocated_size, size_t packet_size)
{
        ArvGvspContentType content_type;
        ArvBufferPayloadType payload_type;
        gboolean extended_ids;
	guint32 block_size;

	extended_ids = arv_gvsp_packet_has_extended_ids (packet, packet_size);
        content_type = arv_gvsp_packet_get_content_type (packet, packet_size);

        switch (content_type) {
                case ARV_GVSP_CONTENT_TYPE_LEADER:
                        payload_type = arv_gvsp_leader_packet_get_buffer_payload_type(packet, packet_size, NULL);
                        if (payload_type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE ||
                            payload_type == ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA ||
                            payload_type == ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA) {
                                if (packet_size <= ARV_GVSP_PAYLOAD_PACKET_PROTOCOL_OVERHEAD (extended_ids))
                                        return 0;
                                block_size = packet_size - ARV_GVSP_PAYLOAD_PACKET_PROTOCOL_OVERHEAD (extended_ids);
                                return (allocated_size + block_size - 1) / block_size + (2 /* leader + trailer */);
                        } else if (payload_type == ARV_BUFFER_PAYLOAD_TYPE_MULTIPART) {
                                unsigned int n_parts;
                                unsigned int n_packets = 0;
                                unsigned int i;

                                n_parts = arv_gvsp_leader_packet_get_multipart_n_parts(packet, packet_size);
                                if (packet_size <= ARV_GVSP_MULTIPART_PACKET_PROTOCOL_OVERHEAD (extended_ids))
                                        return 0;
                                block_size = packet_size - ARV_GVSP_MULTIPART_PACKET_PROTOCOL_OVERHEAD (extended_ids);

                                for (i = 0; i < n_parts; i++) {
                                        n_packets += (arv_gvsp_leader_packet_get_multipart_size (packet, packet_size, i) +
                                                      block_size - 1) / block_size;
                                }

                                return n_packets + (2 /* leader + trailer */);
                        } else {
                                return 0;
                        }
                        break;
                case ARV_GVSP_CONTENT_TYPE_PAYLOAD:
                        /* Only the standard header's overhead is checked at construction: an extended id stream with
                         * a packet size at or below the extended overhead would underflow, or divide by zero */
                        if (packet_size <= ARV_GVSP_PAYLOAD_PACKET_PROTOCOL_OVERHEAD (extended_ids))
                                return 0;
                        block_size = packet_size - ARV_GVSP_PAYLOAD_PACKET_PROTOCOL_OVERHEAD (extended_ids);
                        return (allocated_size + block_size - 1) / block_size + (2 /* leader + trailer */);
                case ARV_GVSP_CONTENT_TYPE_MULTIPART:
                        if (packet_size <= ARV_GVSP_MULTIPART_PACKET_PROTOCOL_OVERHEAD (extended_ids))
                                return 0;
                        block_size = packet_size - ARV_GVSP_MULTIPART_PACKET_PROTOCOL_OVERHEAD (extended_ids);
                        return (allocated_size + block_size - 1) / block_size +
                                (2 /* leader + trailer */) + (255 /* n_parts_max) */);
                        break;
                case ARV_GVSP_CONTENT_TYPE_TRAILER:
                        return arv_gvsp_packet_get_packet_id (packet, packet_size) + 1;
                        break;
                case ARV_GVSP_CONTENT_TYPE_ALL_IN:
                        return 1;
                case ARV_GVSP_CONTENT_TYPE_H264:
                case ARV_GVSP_CONTENT_TYPE_GENDC:
                case ARV_GVSP_CONTENT_TYPE_MULTIZONE:
                case ARV_GVSP_CONTENT_TYPE_UNKNOWN:
                        break;
        }

        return 0;
}

/* Fast fail: a new frame is dropped without a buffer. It counts as seen (last_frame_id), so that its stragglers are
 * late packets and the next frame is not reported as following missing ones. From the client's side it is a
 * missing frame: it is counted in n_missing_frames, besides the gap before it if there is one, and in
 * n_skipped_frames. It never reaches n_failures or n_timeouts, which count buffers. */

static void
_drop_frame (ArvGvStreamThreadData *thread_data, guint64 frame_id, gint64 frame_id_inc, gboolean underrun)
{
	if (frame_id_inc > 1)
		thread_data->n_missing_frames++;
	thread_data->n_missing_frames++;

	thread_data->last_frame_id = frame_id;
	thread_data->dropped_frame_id = frame_id;
	thread_data->dropped_frame_underrun = underrun;
	thread_data->has_dropped_frame = TRUE;
	thread_data->n_skipped_frames++;
}

static ArvGvStreamFrameData *
_find_frame_data (ArvGvStreamThreadData *thread_data,
		  const ArvGvspPacket *packet,
		  size_t packet_size,
		  guint64 frame_id,
		  guint32 packet_id,
		  size_t read_count,
		  guint64 time_us)
{
	ArvGvStreamFrameData *frame = NULL;
	ArvBuffer *buffer;
	GSList *iter;
	guint n_packets = 0;
	gint64 frame_id_inc;
        gboolean extended_ids;

	extended_ids = arv_gvsp_packet_has_extended_ids (packet, packet_size);

	for (iter = thread_data->frames; iter != NULL; iter = iter->next) {
		frame = iter->data;
		if (frame->frame_id == frame_id) {
                        arv_histogram_fill (thread_data->histogram, 1, time_us - frame->first_packet_time_us);
                        arv_histogram_fill (thread_data->histogram, 2, time_us - frame->last_packet_time_us);

			frame->last_packet_time_us = time_us;
			return frame;
		}
	}

	if (extended_ids) {
		frame_id_inc = (gint64) frame_id - (gint64) thread_data->last_frame_id;
		/* Frame id 0 is not a valid value */
		if ((gint64) frame_id > 0 && (gint64) thread_data->last_frame_id < 0)
			frame_id_inc--;
	} else {
		frame_id_inc = (gint16) frame_id - (gint16) thread_data->last_frame_id;
		/* Frame id 0 is not a valid value */
		if ((gint16) frame_id > 0 && (gint16) thread_data->last_frame_id < 0)
			frame_id_inc--;
	}

	/* Fast fail: the rest of a frame already dropped returns here, without looking for a buffer */
	if (thread_data->fast_fail &&
	    thread_data->has_dropped_frame &&
	    thread_data->dropped_frame_id == frame_id) {
		if (thread_data->dropped_frame_underrun)
			thread_data->n_underruns++;
		return NULL;
	}

	if (frame_id_inc < 1  && frame_id_inc > -ARV_GV_STREAM_DISCARD_LATE_FRAME_THRESHOLD) {
		arv_info_stream_thread ("[GvStream::find_frame_data] Discard late frame %" G_GUINT64_FORMAT
					 " (last: %" G_GUINT64_FORMAT ")",
					 frame_id, thread_data->last_frame_id);
		arv_gvsp_packet_debug (packet, packet_size, ARV_DEBUG_LEVEL_INFO);
		return NULL;
	}

	/* Fast fail: the first packet of a new frame is not its leader, and the missing head cannot be requested
	 * (resend NEVER, or more packets than the resend budget, which is what _missing_packet_check would refuse).
	 * Such a frame can only fail, so it does not take a buffer. A first packet within
	 * ARV_GV_STREAM_FAST_FAIL_REORDER_PACKETS (16) of the leader may only have overtaken it, and the frame is
	 * started: under resend NEVER it completes if the head arrives late, as upstream. The budget is computed
	 * from last_n_packets, the n_packets the latest frame was started with (its buffer size estimate), which is
	 * the n_packets _missing_packet_check sees when it asks for a head: the trailer has not arrived then. */
	if (thread_data->fast_fail && packet_id > ARV_GV_STREAM_FAST_FAIL_REORDER_PACKETS &&
	    (thread_data->packet_resend == ARV_GV_STREAM_PACKET_RESEND_NEVER ||
	     (thread_data->last_n_packets > 0 &&
	      ((int) (thread_data->last_n_packets * thread_data->packet_request_ratio) <= 0 ||
	       packet_id > thread_data->last_n_packets * thread_data->packet_request_ratio)))) {
		arv_info_stream_thread ("[GvStream::find_frame_data] Skip frame %" G_GUINT64_FORMAT
					" (first packet %u, head not recoverable)", frame_id, packet_id);
		_drop_frame (thread_data, frame_id, frame_id_inc, FALSE);
		return NULL;
	}

	/* A status packet answers a resend request for a frame already open; one for a newer frame starts nothing,
	 * and its leader, if it claims to be one, is not read (_gvsp_packet_size_is_valid does not check it) */
	if (G_UNLIKELY (arv_gvsp_packet_status_is_error (arv_gvsp_packet_get_status (packet, packet_size))))
		return NULL;

	buffer = arv_stream_pop_input_buffer (thread_data->stream);
	if (buffer == NULL) {
		/* Counts every packet of a frame that found no input buffer */
		thread_data->n_underruns++;
		if (!thread_data->has_underrun_frame || thread_data->last_underrun_frame_id != frame_id) {
			thread_data->n_underrun_frames++;
			thread_data->last_underrun_frame_id = frame_id;
			thread_data->has_underrun_frame = TRUE;
		}

		/* Fast fail: no buffer at the first packet seen of this frame. Do not retry for every later packet,
		 * and do not start the frame mid-way if a buffer is returned while it is being received. */
		if (thread_data->fast_fail)
			_drop_frame (thread_data, frame_id, frame_id_inc, TRUE);

		return NULL;
	}

	n_packets = _compute_n_expected_packets (packet,
                                                 buffer->priv->allocated_size,
                                                 thread_data->scps_packet_size);
        if (n_packets < 1) {
	        buffer->priv->status = ARV_BUFFER_STATUS_PAYLOAD_NOT_SUPPORTED;
                arv_stream_push_output_buffer(thread_data->stream, buffer);
                if (thread_data->callback != NULL)
                        thread_data->callback (thread_data->callback_data,
                                               ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
                                               buffer);
                return NULL;
        }

	frame = g_new0 (ArvGvStreamFrameData, 1);

	frame->disable_resend_request = FALSE;

	frame->frame_id = frame_id;
	frame->last_valid_packet = -1;
	frame->max_checked_packet = -1;
	frame->next_check_us = G_MAXUINT64;

	frame->buffer = buffer;
	_update_socket (thread_data, frame->buffer);
	frame->buffer->priv->status = ARV_BUFFER_STATUS_FILLING;

	frame->first_packet_time_us = time_us;
	frame->last_packet_time_us = time_us;

	frame->packet_data = g_new0 (ArvGvStreamPacketData, n_packets);
	frame->n_packets = n_packets;
	thread_data->last_n_packets = n_packets;

	if (thread_data->callback != NULL &&
	    frame->buffer != NULL)
		thread_data->callback (thread_data->callback_data,
				       ARV_STREAM_CALLBACK_TYPE_START_BUFFER,
				       NULL);

	thread_data->last_frame_id = frame_id;
	thread_data->has_dropped_frame = FALSE;

	if (frame_id_inc > 1) {
		thread_data->n_missing_frames++;
		arv_debug_stream_thread ("[GvStream::find_frame_data] Missed %" G_GINT64_FORMAT
                                         " frame(s) before %" G_GUINT64_FORMAT,
                                         frame_id_inc - 1, frame_id);
	}

	thread_data->frames = g_slist_append (thread_data->frames, frame);

	arv_debug_stream_thread ("[GvStream::find_frame_data] Start frame %" G_GUINT64_FORMAT, frame_id);

	frame->extended_ids = extended_ids;

        arv_histogram_fill (thread_data->histogram, 1, 0);

	return frame;
}

static void
_process_data_leader (ArvGvStreamThreadData *thread_data,
		      ArvGvStreamFrameData *frame,
		      const ArvGvspPacket *packet,
                      size_t packet_size,
		      guint32 packet_id)
{
	if (frame->buffer->priv->status != ARV_BUFFER_STATUS_FILLING)
		return;

	if (packet_id != 0) {
		frame->buffer->priv->status = ARV_BUFFER_STATUS_WRONG_PACKET_ID;
		return;
	}

        frame->leader_received = TRUE;

	frame->buffer->priv->payload_type = arv_gvsp_leader_packet_get_buffer_payload_type
                (packet, packet_size, &frame->buffer->priv->has_chunks);
	frame->buffer->priv->frame_id = frame->frame_id;
	frame->buffer->priv->chunk_endianness = G_BIG_ENDIAN;

	frame->buffer->priv->system_timestamp_ns = g_get_monotonic_time() * 1000LL;

        if (frame->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE ||
            frame->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA) {
                guint64 timestamp;

                arv_buffer_set_n_parts (frame->buffer, 1);

                timestamp = arv_gvsp_leader_packet_get_timestamp(packet, packet_size);

                frame->buffer->priv->parts[0].data_offset = 0;
                frame->buffer->priv->parts[0].component_id = 0;
                frame->buffer->priv->parts[0].data_type = ARV_BUFFER_PART_DATA_TYPE_2D_IMAGE;
                arv_gvsp_leader_packet_get_image_infos (packet, packet_size,
                                                        &frame->buffer->priv->parts[0].pixel_format,
                                                        &frame->buffer->priv->parts[0].width,
                                                        &frame->buffer->priv->parts[0].height,
                                                        &frame->buffer->priv->parts[0].x_offset,
                                                        &frame->buffer->priv->parts[0].y_offset,
                                                        &frame->buffer->priv->parts[0].x_padding,
                                                        &frame->buffer->priv->parts[0].y_padding);

		if (G_LIKELY (thread_data->timestamp_tick_frequency != 0))
			frame->buffer->priv->timestamp_ns =
                                arv_gvsp_timestamp_to_ns (timestamp, thread_data->timestamp_tick_frequency);
		else
			frame->buffer->priv->timestamp_ns = frame->buffer->priv->system_timestamp_ns;
        } else if (frame->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA) {
                guint64 timestamp;
                arv_buffer_set_n_parts (frame->buffer, 0);
                timestamp = arv_gvsp_leader_packet_get_timestamp(packet, packet_size);
		if (G_LIKELY (thread_data->timestamp_tick_frequency != 0))
			frame->buffer->priv->timestamp_ns =
                                arv_gvsp_timestamp_to_ns (timestamp, thread_data->timestamp_tick_frequency);
		else
			frame->buffer->priv->timestamp_ns = frame->buffer->priv->system_timestamp_ns;
        } else if (frame->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_MULTIPART) {
                guint64 timestamp;
                unsigned int i;
                guint n_parts;
                ptrdiff_t offset = 0;

                n_parts = arv_gvsp_leader_packet_get_multipart_n_parts(packet, packet_size);

                timestamp = arv_gvsp_leader_packet_get_timestamp(packet, packet_size);

                arv_buffer_set_n_parts (frame->buffer, n_parts);

                for (i = 0; i < n_parts; i++) {
                        frame->buffer->priv->parts[i].data_offset = offset;
                        arv_gvsp_leader_packet_get_multipart_infos (packet, packet_size, i,
                                                                    &frame->buffer->priv->parts[i].component_id,
                                                                    &frame->buffer->priv->parts[i].data_type,
                                                                    &frame->buffer->priv->parts[i].size,
                                                                    &frame->buffer->priv->parts[i].pixel_format,
                                                                    &frame->buffer->priv->parts[i].width,
                                                                    &frame->buffer->priv->parts[i].height,
                                                                    &frame->buffer->priv->parts[i].x_offset,
                                                                    &frame->buffer->priv->parts[i].y_offset,
                                                                    &frame->buffer->priv->parts[i].x_padding,
                                                                    &frame->buffer->priv->parts[i].y_padding);
                        offset += frame->buffer->priv->parts[i].size;
                }

		if (G_LIKELY (thread_data->timestamp_tick_frequency != 0))
			frame->buffer->priv->timestamp_ns =
                                arv_gvsp_timestamp_to_ns (timestamp, thread_data->timestamp_tick_frequency);
		else
			frame->buffer->priv->timestamp_ns = frame->buffer->priv->system_timestamp_ns;
        } else {
                frame->buffer->priv->timestamp_ns = frame->buffer->priv->system_timestamp_ns;
        }

	if (frame->packet_data[packet_id].resend_requested) {
		thread_data->n_resent_packets++;
		arv_debug_stream_thread ("[GvStream::process_data_leader] Received resent packet %u for frame %" G_GUINT64_FORMAT,
				       packet_id, frame->frame_id);
	}
}

static void
_process_payload_block (ArvGvStreamThreadData *thread_data,
		     ArvGvStreamFrameData *frame,
		     const ArvGvspPacket *packet,
                     size_t packet_size,
		     guint32 packet_id)
{
	size_t block_size;
	ptrdiff_t block_offset;
	ptrdiff_t block_end;
	gboolean extended_ids;

	if (frame->buffer->priv->status != ARV_BUFFER_STATUS_FILLING)
		return;

	/* Data packets are 1 to n_packets - 2. n_packets can be 1 or 2 (an ALL_IN first packet, a trailer at packet
	 * id 0 or 1 starting or shrinking the frame), where n_packets - 2 would wrap and accept any packet id. */
	if (frame->n_packets < 3 || packet_id > frame->n_packets - 2 || packet_id < 1) {
		arv_gvsp_packet_debug (packet, packet_size, ARV_DEBUG_LEVEL_INFO);
		frame->buffer->priv->status = ARV_BUFFER_STATUS_WRONG_PACKET_ID;
		return;
	}

	extended_ids = arv_gvsp_packet_has_extended_ids (packet, packet_size);

	block_size = arv_gvsp_payload_packet_get_data_size (packet, packet_size);
	block_offset = (packet_id - 1) * (thread_data->scps_packet_size -
                                         ARV_GVSP_PAYLOAD_PACKET_PROTOCOL_OVERHEAD (extended_ids));
	block_end = block_size + block_offset;

	/* A frame started by its trailer takes n_packets from the trailer's packet id, not from the buffer size, so a
	 * valid packet id can still point past the buffer: clamping block_end below would make block_size negative */
	if (block_offset >= (ptrdiff_t) frame->buffer->priv->allocated_size) {
		arv_info_stream_thread ("[GvStream::process_data_block] Packet %u of frame %" G_GUINT64_FORMAT
					" starts beyond the buffer", packet_id, frame->frame_id);
		thread_data->n_size_mismatch_errors++;
		return;
	}

	if (block_end > frame->buffer->priv->allocated_size) {
		arv_info_stream_thread ("[GvStream::process_data_block] %" G_GINTPTR_FORMAT " unexpected bytes in packet %u "
					 " for frame %" G_GUINT64_FORMAT,
					 block_end - frame->buffer->priv->allocated_size,
					 packet_id, frame->frame_id);
		thread_data->n_size_mismatch_errors++;

		block_end = frame->buffer->priv->allocated_size;
		block_size = block_end - block_offset;
	}

	memcpy (((char *) frame->buffer->priv->data) + block_offset,
                arv_gvsp_packet_get_data (packet, packet_size),
                block_size);

        frame->received_size += block_size;

	if (frame->packet_data[packet_id].resend_requested) {
		thread_data->n_resent_packets++;
		arv_debug_stream_thread ("[GvStream::process_data_block] Received resent packet %u for frame %" G_GUINT64_FORMAT,
				       packet_id, frame->frame_id);
	}
}

static void
_process_multipart_block (ArvGvStreamThreadData *thread_data,
                          ArvGvStreamFrameData *frame,
                          const ArvGvspPacket *packet,
                          size_t packet_size,
                          guint32 packet_id)
{
        guint part_id;
        ptrdiff_t block_offset;

	if (frame->buffer->priv->status != ARV_BUFFER_STATUS_FILLING)
		return;

        if (arv_gvsp_multipart_packet_get_infos (packet, packet_size, &part_id, &block_offset)) {
                size_t block_size;
                ptrdiff_t block_end;
                void *data;

                block_size = arv_gvsp_multipart_packet_get_data_size (packet, packet_size);

                if (block_size == 0)
                        return;

                block_end = block_offset + block_size;

                if (block_end > frame->buffer->priv->allocated_size) {
                        arv_info_stream_thread ("[GvStream::process_multipart_block] %" G_GINTPTR_FORMAT
                                                " unexpected bytes in packet %u "
                                                " for frame %" G_GUINT64_FORMAT,
                                                block_end - frame->buffer->priv->allocated_size,
                                                packet_id, frame->frame_id);
                        return;
                }

                data = arv_gvsp_multipart_packet_get_data (packet, packet_size);
                memcpy ((char *) frame->buffer->priv->data + block_offset, data, block_size);

                frame->received_size += block_size;
        }
}

static void
_process_data_trailer (ArvGvStreamThreadData *thread_data,
		       ArvGvStreamFrameData *frame,
		       guint32 packet_id)
{
	if (frame->buffer->priv->status != ARV_BUFFER_STATUS_FILLING)
		return;

	if (packet_id > frame->n_packets - 1) {
		frame->buffer->priv->status = ARV_BUFFER_STATUS_WRONG_PACKET_ID;
                return;
        }

        /* Trailer packet received before expected, because the actual payload size is smaller than the buffer size */
        if (frame->n_packets != packet_id + 1) {
		arv_debug_stream_thread ("[GvStream::process_data_trailer] Update expected number of packets (%u → %u)",
                                         frame->n_packets, packet_id + 1);
                frame->n_packets = packet_id + 1;
        }

	frame->trailer_received = TRUE;

	if (frame->packet_data[packet_id].resend_requested) {
		thread_data->n_resent_packets++;
		arv_debug_stream_thread ("[GvStream::process_data_trailer] Received resent packet %u for frame %"
                                         G_GUINT64_FORMAT,
                                         packet_id, frame->frame_id);
        }
}

static void
_missing_packet_check (ArvGvStreamThreadData *thread_data,
		       ArvGvStreamFrameData *frame,
		       guint32 packet_id,
		       guint64 time_us)
{
	int i;

	if (thread_data->packet_resend == ARV_GV_STREAM_PACKET_RESEND_NEVER ||
	    frame->disable_resend_request ||
	    frame->resend_ratio_reached)
		return;

	if ((int) (frame->n_packets * thread_data->packet_request_ratio) <= 0)
		return;

	/* Fast fail: the scan below runs only once the earliest pending deadline has passed. Packets seen missing
	 * for the first time (above every packet id checked before) get their initial deadline here, at the same time
	 * and with the same value the scan would give them; below max_checked_packet every missing packet already has
	 * one. A scan that would request nothing is skipped, so the requests and their deadlines are unchanged. */
	if (thread_data->fast_fail && packet_id < frame->n_packets) {
		if ((gint32) packet_id > frame->max_checked_packet) {
			for (i = MAX (frame->max_checked_packet, frame->last_valid_packet) + 1; i <= (int) packet_id; i++) {
				if (!frame->packet_data[i].received && frame->packet_data[i].abs_timeout_us == 0) {
					frame->packet_data[i].abs_timeout_us = time_us +
						thread_data->initial_packet_timeout_us;
					frame->next_check_us = MIN (frame->next_check_us,
								    frame->packet_data[i].abs_timeout_us);
				}
			}
			frame->max_checked_packet = packet_id;
		}

		/* need_resend is time_us > abs_timeout_us */
		if (time_us <= frame->next_check_us)
			return;
	}

	if (packet_id < frame->n_packets) {
		int first_missing = -1;

		for (i = frame->last_valid_packet + 1; i <= packet_id + 1; i++) {
			gboolean need_resend;

			if (i <= packet_id && !frame->packet_data[i].received) {
                                if (frame->packet_data[i].abs_timeout_us == 0)
                                        frame->packet_data[i].abs_timeout_us = time_us +
                                                thread_data->initial_packet_timeout_us;
                                need_resend = time_us > frame->packet_data[i].abs_timeout_us;
                        } else
                                need_resend = FALSE;

			if (need_resend) {
				if (first_missing < 0)
					first_missing = i;
			}

			if (i > packet_id || !need_resend) {
				if (first_missing >= 0) {
					int last_missing;
					int n_missing_packets;
					int j;

					last_missing = i - 1;
					n_missing_packets = last_missing - first_missing + 1;

					if (frame->n_packet_resend_requests + n_missing_packets >
					    (frame->n_packets * thread_data->packet_request_ratio)) {
						frame->n_packet_resend_requests += n_missing_packets;

						arv_info_stream_thread ("[GvStream::missing_packet_check]"
									 " Maximum number of requests "
									 "reached at dt = %" G_GINT64_FORMAT
									 ", n_packet_requests = %u (%u packets/frame), frame_id = %"
									 G_GUINT64_FORMAT,
									 time_us - frame->first_packet_time_us,
									 frame->n_packet_resend_requests, frame->n_packets,
									 frame->frame_id);

						thread_data->n_resend_ratio_reached++;
						frame->resend_ratio_reached = TRUE;

						return;
					}

					arv_debug_stream_thread ("[GvStream::missing_packet_check]"
							       " Resend request at dt = %" G_GINT64_FORMAT
							       ", packet id = %u (%u packets/frame)",
							       time_us - frame->first_packet_time_us,
							       packet_id, frame->n_packets);

					_send_packet_request (thread_data,
							      frame->frame_id,
							      first_missing,
							      last_missing,
							      frame->extended_ids);

					for (j = first_missing; j <= last_missing; j++) {
						frame->packet_data[j].abs_timeout_us = time_us +
                                                        thread_data->packet_timeout_us;
                                                frame->packet_data[j].resend_requested = TRUE;
                                        }

					/* The budget is cumulative: every requested packet counts, a repeated request
					 * for the same packet included, so that packet-request-ratio bounds the resend
					 * traffic a frame can cause. */
					frame->n_packet_resend_requests += n_missing_packets;
					thread_data->n_resend_requests += n_missing_packets;

					first_missing = -1;
				}
			}
		}

		if (thread_data->fast_fail) {
			gint32 last = MIN (frame->max_checked_packet, (gint32) frame->n_packets - 1);

			frame->next_check_us = G_MAXUINT64;
			for (i = frame->last_valid_packet + 1; i <= last; i++)
				if (!frame->packet_data[i].received && frame->packet_data[i].abs_timeout_us != 0)
					frame->next_check_us = MIN (frame->next_check_us,
								    frame->packet_data[i].abs_timeout_us);
		}
	}
}

static void
_close_frame (ArvGvStreamThreadData *thread_data,
              guint64 time_us,
              ArvGvStreamFrameData *frame)
{
	if (frame->buffer->priv->status == ARV_BUFFER_STATUS_SUCCESS) {
		thread_data->n_completed_buffers++;
	} else
		if (frame->buffer->priv->status != ARV_BUFFER_STATUS_ABORTED)
			thread_data->n_failures++;

	if (frame->buffer->priv->status == ARV_BUFFER_STATUS_TIMEOUT)
		thread_data->n_timeouts++;

	if (frame->buffer->priv->status == ARV_BUFFER_STATUS_ABORTED)
		thread_data->n_aborted++;

	/* last_valid_packet can be beyond the end when a trailer arrived after packets above its id: n_packets
	 * shrank under it. Adding the negative difference would wrap the counter. */
	if (frame->buffer->priv->status != ARV_BUFFER_STATUS_SUCCESS &&
	    frame->buffer->priv->status != ARV_BUFFER_STATUS_ABORTED &&
	    (gint64) frame->n_packets > (gint64) frame->last_valid_packet + 1)
		thread_data->n_missing_packets += (gint64) frame->n_packets - ((gint64) frame->last_valid_packet + 1);

	arv_stream_push_output_buffer (thread_data->stream, frame->buffer);
	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data,
				       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
				       frame->buffer);

        arv_histogram_fill (thread_data->histogram, 0,
                            time_us - frame->first_packet_time_us);

	arv_debug_stream_thread ("[GvStream::close_frame] Close frame %" G_GUINT64_FORMAT, frame->frame_id);

	frame->buffer = NULL;
	frame->frame_id = 0;

	g_free (frame->packet_data);
	g_free (frame);
}

static void
_check_frame_completion (ArvGvStreamThreadData *thread_data,
			 guint64 time_us,
			 ArvGvStreamFrameData *current_frame)
{
	GSList *iter;
	ArvGvStreamFrameData *frame;
	gboolean can_close_frame = TRUE;

	for (iter = thread_data->frames; iter != NULL;) {
		frame = iter->data;

		/* An incomplete frame that nothing will be requested for any more cannot complete once a newer
		 * frame exists. Upstream closes it only under resend NEVER; fast fail also when its resend budget is
		 * spent or the device said a packet is gone, instead of holding its buffer, and every frame behind
		 * it, for frame-retention.
		 *
		 * "Gone" is only final once the trailer has been seen. Without it, the tail request runs to the end
		 * of the buffer size estimate, and a device may answer PACKET_UNAVAILABLE for the ids beyond its real
		 * trailer before it resends the trailer that completes the frame. So, until the trailer is seen, the
		 * frame waits packet-timeout after the first status packet: every packet requested before it has a
		 * deadline earlier than that. */
		if (can_close_frame &&
		    iter->next != NULL &&
		    (thread_data->packet_resend == ARV_GV_STREAM_PACKET_RESEND_NEVER ||
		     (thread_data->fast_fail &&
		      frame->last_valid_packet != frame->n_packets - 1 &&
		      (frame->resend_ratio_reached ||
		       (frame->disable_resend_request &&
			(frame->trailer_received ||
			 time_us - frame->disable_resend_us >= thread_data->packet_timeout_us)))))) {
			frame->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
			arv_info_stream_thread ("[GvStream::check_frame_completion] Incomplete frame %" G_GUINT64_FORMAT,
						 frame->frame_id);
			_close_frame (thread_data, time_us, frame);
			thread_data->frames = iter->next;
			g_slist_free_1 (iter);
			iter = thread_data->frames;
			continue;
		}

		if (can_close_frame &&
		    frame->last_valid_packet == frame->n_packets - 1) {
			frame->buffer->priv->status = ARV_BUFFER_STATUS_SUCCESS;
                        frame->buffer->priv->received_size = frame->received_size;

                        if (frame->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE ||
                            frame->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA) {
                                frame->buffer->priv->parts[0].size = frame->received_size;
                        }

			arv_debug_stream_thread ("[GvStream::check_frame_completion] Completed frame %" G_GUINT64_FORMAT,
					       frame->frame_id);
			_close_frame (thread_data, time_us, frame);
			thread_data->frames = iter->next;
			g_slist_free_1 (iter);
			iter = thread_data->frames;
			continue;
		}

		if (can_close_frame &&
                    /* Do not timeout on the most recent frame if the LEADER packet is so far the ONLY
                     * valid packet received. This is needed by some devices sending the leader packet early, at
                     * acquisition start. */
                    (frame->frame_id != thread_data->last_frame_id || frame->last_valid_packet != 0) &&
		    time_us - frame->last_packet_time_us >= thread_data->frame_retention_us) {
			frame->buffer->priv->status = ARV_BUFFER_STATUS_TIMEOUT;
			arv_warning_stream_thread ("[GvStream::check_frame_completion] Timeout for frame %"
						   G_GUINT64_FORMAT " at dt = %" G_GUINT64_FORMAT,
						   frame->frame_id, time_us - frame->first_packet_time_us);
#if 0
			if (arv_debug_check (&arv_debug_category_stream_thread, ARV_DEBUG_LEVEL_LOG)) {
				int i;
				arv_debug_stream_thread ("frame_id          = %Lu", frame->frame_id);
				arv_debug_stream_thread ("last_valid_packet = %d", frame->last_valid_packet);
				for (i = 0; i < frame->n_packets; i++) {
					arv_debug_stream_thread ("%d - time = %Lu%s", i,
							       frame->packet_data[i].time_us,
							       frame->packet_data[i].received ? " - OK" : "");
				}
			}
#endif
			_close_frame (thread_data, time_us, frame);
			thread_data->frames = iter->next;
			g_slist_free_1 (iter);
			iter = thread_data->frames;
			continue;
		}

		can_close_frame = FALSE;

		/* Upstream checks a frame's missing tail (trailer included) only after packet-timeout of silence for
		 * it, so a lost trailer is requested ~21 ms late. With fast fail, a packet of a newer frame (or a
		 * newer frame dropped without a buffer) means the sender is done with this one: its missing tail gets
		 * the initial-packet-timeout deadline now. The newest frame is never checked this way, so a frame that
		 * is still being sent is not; within a frame, reordering is covered by the same initial timeout. The
		 * call is cheap while nothing is due (next_check_us). */
		if (frame != current_frame &&
		    (time_us - frame->last_packet_time_us >= thread_data->packet_timeout_us ||
		     (thread_data->fast_fail && frame->frame_id != thread_data->last_frame_id))) {
			_missing_packet_check (thread_data, frame, frame->n_packets - 1, time_us);
			iter = iter->next;
			continue;
		}

		iter = iter->next;
	}
}

static void
_flush_frames (ArvGvStreamThreadData *thread_data,
               guint64 time_us)
{
	GSList *iter;
	ArvGvStreamFrameData *frame;

	for (iter = thread_data->frames; iter != NULL; iter = iter->next) {
		frame = iter->data;
		frame->buffer->priv->status = ARV_BUFFER_STATUS_ABORTED;
		_close_frame (thread_data, time_us, frame);
	}

	g_slist_free (thread_data->frames);
	thread_data->frames = NULL;
}

/* Whether a datagram is long enough for every field the receiver reads from it: the GVSP header (the standard one,
 * then, once its extended-id flag can be read, the extended one) and, for a leader, the leader with its image or
 * multipart part information. The arv_gvsp accessors are bounds checked and read a field a short datagram does not
 * hold as 0, so without this check such a datagram would still be processed as a packet (frame 0, packet 0), counted
 * as received, and could start a frame or become the sender the stream socket connects to. Error status packets
 * are header only and are exempt from the leader check; they never start a frame (_find_frame_data), which is where
 * a leader would be read. */

static inline gboolean
_gvsp_packet_size_is_valid (const ArvGvspPacket *packet, size_t packet_size)
{
	size_t header_size;
	size_t leader_size;

	if (G_UNLIKELY (packet_size < sizeof (ArvGvspPacket) + sizeof (ArvGvspHeader)))
		return FALSE;

	if (G_UNLIKELY (arv_gvsp_packet_has_extended_ids (packet, packet_size))) {
		header_size = sizeof (ArvGvspPacket) + sizeof (ArvGvspExtendedHeader);
		if (packet_size < header_size)
			return FALSE;
	} else
		header_size = sizeof (ArvGvspPacket) + sizeof (ArvGvspHeader);

	if (G_LIKELY (arv_gvsp_packet_get_content_type (packet, packet_size) != ARV_GVSP_CONTENT_TYPE_LEADER) ||
	    arv_gvsp_packet_status_is_error (arv_gvsp_packet_get_status (packet, packet_size)))
		return TRUE;

	if (packet_size < header_size + sizeof (ArvGvspLeader))
		return FALSE;

	switch (arv_gvsp_leader_packet_get_buffer_payload_type (packet, packet_size, NULL)) {
		case ARV_BUFFER_PAYLOAD_TYPE_IMAGE:
		case ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA:
			leader_size = sizeof (ArvGvspImageLeader);
			break;
		case ARV_BUFFER_PAYLOAD_TYPE_MULTIPART: {
			guint32 packet_infos;

			/* The part count from the header itself: arv_gvsp_leader_packet_get_multipart_n_parts
			 * answers 0 for a leader too short for its parts, which would let it through */
			if (arv_gvsp_packet_has_extended_ids (packet, packet_size))
				packet_infos = ((const ArvGvspExtendedHeader *) &packet->header)->packet_infos;
			else
				packet_infos = ((const ArvGvspHeader *) &packet->header)->packet_infos;

			leader_size = sizeof (ArvGvspMultipartLeader) +
				(size_t) (g_ntohl (packet_infos) & ARV_GVSP_PACKET_INFOS_N_PARTS_MASK) *
				sizeof (ArvGvspPartInfos);
			break;
		}
		default:
			leader_size = sizeof (ArvGvspLeader);
			break;
	}

	return packet_size >= header_size + leader_size;
}

static ArvGvStreamFrameData *
_process_packet (ArvGvStreamThreadData *thread_data, const ArvGvspPacket *packet, size_t packet_size, guint64 time_us)

{
	ArvGvStreamFrameData *frame;
	guint32 packet_id;
	guint64 frame_id;
	int i;

	/* Every receive loop passes through here: a datagram too short to be read is counted and dropped, whatever
	 * sent it (the stream socket accepts any sender while it is unconnected) */
	if (G_UNLIKELY (!_gvsp_packet_size_is_valid (packet, packet_size))) {
		thread_data->n_short_packets++;
		return NULL;
	}

	thread_data->n_received_packets++;

	frame_id = arv_gvsp_packet_get_frame_id (packet, packet_size);
	packet_id = arv_gvsp_packet_get_packet_id (packet, packet_size);

	if (thread_data->first_packet) {
		thread_data->last_frame_id = frame_id - 1;
		thread_data->first_packet = FALSE;
	}

	frame = _find_frame_data (thread_data, packet, packet_size, frame_id, packet_id, packet_size, time_us);

	if (frame != NULL) {
		ArvGvspPacketStatus packet_status = arv_gvsp_packet_get_status (packet, packet_size);

		if (arv_gvsp_packet_status_is_error (packet_status)) {
			arv_info_stream_thread ("[GvStream::process_packet]"
						 " Error packet at dt = %" G_GINT64_FORMAT ", packet id = %u"
						 " frame id = %" G_GUINT64_FORMAT,
						 time_us - frame->first_packet_time_us,
						 packet_id, frame->frame_id);
			arv_gvsp_packet_debug (packet, packet_size, ARV_DEBUG_LEVEL_INFO);

                        if (packet_status == ARV_GVSP_PACKET_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY ||
                            packet_status == ARV_GVSP_PACKET_STATUS_PACKET_REMOVED_FROM_MEMORY ||
                            packet_status == ARV_GVSP_PACKET_STATUS_PACKET_UNAVAILABLE) {
                                if (!frame->disable_resend_request)
                                        frame->disable_resend_us = time_us;
                                frame->disable_resend_request = TRUE;
                                thread_data->n_resend_disabled++;
                        }

			thread_data->n_error_packets++;
                        thread_data->n_transferred_bytes += packet_size;
		} else if (packet_id < frame->n_packets &&
		           frame->packet_data[packet_id].received) {
			/* Ignore duplicate packet */
			thread_data->n_duplicated_packets++;
			arv_debug_stream_thread ("[GvStream::process_packet] Duplicated packet %d for frame %" G_GUINT64_FORMAT,
						 packet_id, frame->frame_id);
			arv_gvsp_packet_debug (packet, packet_size, ARV_DEBUG_LEVEL_DEBUG);

                        thread_data->n_transferred_bytes += packet_size;
		} else {
			ArvGvspContentType content_type;

                        if (packet_id < frame->n_packets) {
                                frame->packet_data[packet_id].received = TRUE;
                        }

                        /* Keep track of last packet of a continuous block starting from packet 0 */
                        for (i = frame->last_valid_packet + 1; i < frame->n_packets; i++)
                                if (!frame->packet_data[i].received)
                                        break;
                        frame->last_valid_packet = i - 1;

                        content_type = arv_gvsp_packet_get_content_type (packet, packet_size);

                        arv_gvsp_packet_debug (packet, packet_size,
                                               content_type == ARV_GVSP_CONTENT_TYPE_LEADER ||
                                               content_type == ARV_GVSP_CONTENT_TYPE_TRAILER ?
                                               ARV_DEBUG_LEVEL_DEBUG :
                                               ARV_DEBUG_LEVEL_TRACE);

                        switch (content_type) {
                                case ARV_GVSP_CONTENT_TYPE_LEADER:
                                        _process_data_leader (thread_data, frame, packet, packet_size, packet_id);
                                        thread_data->n_transferred_bytes += packet_size;
                                        break;
                                case ARV_GVSP_CONTENT_TYPE_PAYLOAD:
                                        _process_payload_block (thread_data, frame, packet, packet_size, packet_id);
                                        thread_data->n_transferred_bytes += packet_size;
                                        break;
                                case ARV_GVSP_CONTENT_TYPE_MULTIPART:
                                        _process_multipart_block (thread_data, frame, packet, packet_size, packet_id);
                                        thread_data->n_transferred_bytes += packet_size;
                                        break;
                                case ARV_GVSP_CONTENT_TYPE_TRAILER:
                                        _process_data_trailer (thread_data, frame, packet_id);
                                        thread_data->n_transferred_bytes += packet_size;
                                        break;
                                default:
                                        thread_data->n_ignored_packets++;
                                        thread_data->n_ignored_bytes += packet_size;
                                        break;
                        }

                        _missing_packet_check (thread_data, frame, packet_id, time_us);
		}
	} else {
                thread_data->n_ignored_packets++;
                thread_data->n_ignored_bytes += packet_size;
        }

	return frame;
}

static void
_loop (ArvGvStreamThreadData *thread_data)
{
	ArvGvStreamFrameData *frame;
	ArvGvspPacket *packet_buffers;
	GPollFD poll_fd[2];
	guint64 time_us = 0;
	gboolean use_poll;
	int i;
	GInputVector packet_iv[ARV_GV_STREAM_NUM_BUFFERS] = { {NULL, 0}, };
	GInputMessage packet_im[ARV_GV_STREAM_NUM_BUFFERS] = { {NULL, NULL, 0, 0, 0, NULL, NULL}, };
	GSocketAddress *packet_addresses[ARV_GV_STREAM_NUM_BUFFERS] = { NULL, };
	guint n_rx_errors = 0;
	// we don't need to consider the IP and UDP header size
	guint packet_buffer_size = thread_data->scps_packet_size - 20 - 8;

	arv_info_stream ("[GvStream::loop] Standard socket method");

	poll_fd[0].fd = g_socket_get_fd (thread_data->socket);
	poll_fd[0].events =  G_IO_IN;
	poll_fd[0].revents = 0;

	arv_gpollfd_prepare_all(poll_fd,1);

	packet_buffers = g_malloc0 (packet_buffer_size * ARV_GV_STREAM_NUM_BUFFERS);

	for (i = 0; i < ARV_GV_STREAM_NUM_BUFFERS; i++) {
		packet_iv[i].buffer = (char *) packet_buffers + i * packet_buffer_size;
		packet_iv[i].size = packet_buffer_size;
		packet_im[i].vectors = &packet_iv[i];
		packet_im[i].num_vectors = 1;
	}

	use_poll = g_cancellable_make_pollfd (thread_data->cancellable, &poll_fd[1]);

        _signal_thread_started (thread_data);

	do {
                int timeout_ms;
		int n_events;
		int errsv;

		/* At least 1 ms: a packet timeout under 1 ms (ARV_GV_STREAM_PACKET_TIMEOUT_US) would make g_poll
		 * return at once while frames are open, a busy loop */
		if (thread_data->frames != NULL)
			timeout_ms = MAX (1, thread_data->packet_timeout_us / 1000);
		else
			timeout_ms = ARV_GV_STREAM_POLL_TIMEOUT_US / 1000;

		do {
			poll_fd[0].revents = 0;
			n_events = g_poll (poll_fd, use_poll ?  2 : 1, timeout_ms);
			errsv = errno;

		} while (n_events < 0 && errsv == EINTR);

		if (poll_fd[0].revents != 0) {
                        GError *error = NULL;
                        gboolean want_source;
                        int n_msgs;

			thread_data->n_rx_wakeups++;

			/* Source addresses are asked for only until the socket is connected (_connect_to_source) */
			want_source = thread_data->connect_socket && !thread_data->socket_connected;
			for (i = 0; i < ARV_GV_STREAM_NUM_BUFFERS; i++) {
				packet_addresses[i] = NULL;
				packet_im[i].address = want_source ? &packet_addresses[i] : NULL;
			}

			arv_gpollfd_clear_one (&poll_fd[0], thread_data->socket);
			n_msgs = g_socket_receive_messages (thread_data->socket,
		 					    packet_im,
		 					    ARV_GV_STREAM_NUM_BUFFERS,
		 					    G_SOCKET_MSG_NONE,
		 					    NULL,
		 					    &error);

                        if (G_LIKELY(n_msgs > 0)) {
                                n_rx_errors = 0;
                                time_us = g_get_monotonic_time ();
                                thread_data->last_datagram_us = time_us;
                                thread_data->n_rx_batches++;
                                if ((guint64) n_msgs > thread_data->n_rx_max_batch)
                                        thread_data->n_rx_max_batch = n_msgs;
                                for (i = 0; i < n_msgs; i++) {
                                        /* GLib allocates the source addresses: _loop is the fallback path */
                                        if (G_UNLIKELY (want_source) && packet_addresses[i] != NULL &&
                                            _gvsp_packet_size_is_valid (packet_iv[i].buffer,
                                                                        packet_im[i].bytes_received)) {
                                                struct sockaddr_storage native;

                                                if (g_socket_address_to_native (packet_addresses[i], &native,
                                                                                sizeof (native), NULL) &&
                                                    _connect_to_source (thread_data,
                                                                        (struct sockaddr *) &native,
                                                                        g_socket_address_get_native_size
                                                                        (packet_addresses[i])))
                                                        want_source = FALSE;
                                        }
                                        frame = _process_packet (thread_data,
                                                                 packet_iv[i].buffer,
                                                                 packet_im[i].bytes_received,
                                                                 time_us);
                                        _check_frame_completion (thread_data, time_us, frame);
                                }
                        } else {
                                /* Readable but failing: never spin (the thread may be real-time), and warn
                                 * on the first failure of a run and every 1000th */
                                if (n_rx_errors % 1000 == 0)
                                        arv_warning_stream_thread ("[GvStream::loop] receive_messages failed: %s"
                                                                   " (%u in a row)",
                                                                   error != NULL ? error->message :
                                                                   "Unknown reason", n_rx_errors + 1);
                                n_rx_errors++;
                                g_clear_error (&error);
                                g_usleep (1000);
                        }

			for (i = 0; i < ARV_GV_STREAM_NUM_BUFFERS; i++)
				g_clear_object (&packet_addresses[i]);
                } else {
                        time_us = g_get_monotonic_time ();
                        _check_frame_completion (thread_data, time_us, NULL);
                        _check_source_idle (thread_data, time_us);
                }

		_sample_thread_stats (thread_data, time_us, FALSE);

	} while (!g_cancellable_is_cancelled (thread_data->cancellable));

	if (use_poll)
		g_cancellable_release_fd (thread_data->cancellable);

	arv_gpollfd_finish_all (poll_fd,1);
	g_free (packet_buffers);
}

#ifdef __APPLE__

/* recvmsg_x is a private Darwin system call wrapper exported by libsystem_kernel, not declared in the
 * SDK. It receives up to cnt datagrams in one call and returns how many it received, or -1. Only
 * MSG_DONTWAIT / MSG_NBIO are accepted as flags, and cnt is clamped to kern.ipc.maxrecvmsgx. On
 * return, msg_datalen is the number of bytes copied into the message, and a datagram larger than its
 * iovec is truncated silently. Layout from XNU bsd/sys/socket_private.h. */

typedef struct {
	void *msg_name;
	socklen_t msg_namelen;
	struct iovec *msg_iov;
	int msg_iovlen;
	void *msg_control;
	socklen_t msg_controllen;
	int msg_flags;
	size_t msg_datalen;
} ArvGvStreamMsghdrX;

typedef ssize_t (*ArvGvStreamRecvmsgX) (int s, const ArvGvStreamMsghdrX *msgp, u_int cnt, int flags);

static guint
_get_env_uint (const char *name, guint default_value, guint max_value)
{
	const char *env;
	char *end = NULL;
	guint64 value;

	env = g_getenv (name);
	if (env == NULL)
		return default_value;

	value = g_ascii_strtoull (env, &end, 10);
	if (end == env || *end != '\0') {
		arv_warning_stream_thread ("[GvStream::loop_darwin] Invalid %s '%s' (expected an integer), using %u",
					   name, env, default_value);
		return default_value;
	}

	return MIN (value, max_value);
}

/* Receive loop for Apple. There, GLib's g_socket_receive_messages is one recvmsg per datagram plus a
 * failing call that builds a GError at the end of each drain, and _loop's g_poll returned every 3 to 4
 * datagrams of a fast stream. This loop receives up to ARV_GV_STREAM_RX_BATCH datagrams per system call with
 * recvmsg_x, and waits in kevent with a NOTE_LOWAT low-water mark, so that a wakeup finds a batch.
 *
 * The thread must block when the socket is empty: it runs under the Mach time constraint policy, and
 * the kernel demotes a real-time thread that computes without blocking. The wait is in kevent and not
 * in a blocking receive, as a thread blocked in the receive call is woken for every datagram.
 *
 * While a stream is active (a frame is open, or a packet arrived in the last
 * ARV_GV_STREAM_RX_ACTIVE_US), the wait is at most ARV_GV_STREAM_RX_ACTIVE_WAIT_US (and never longer
 * than the packet timeouts), so that a tail of packets below the low-water mark, a lost trailer or a
 * resend deadline is not delayed by more than that. When idle, the low-water mark is one datagram and
 * the wait is ARV_GV_STREAM_POLL_TIMEOUT_US, as in _loop.
 *
 * A recvmsg_x that keeps failing (any error but EAGAIN / EINTR) would make kevent return at once, as the data is
 * still queued, and the loop would spin: after a failure the loop waits the active wait before trying again,
 * and after ARV_GV_STREAM_RX_MAX_ERRORS (100) consecutive failures it gives up, and the caller continues in
 * _loop, whose receive call is a different one.
 *
 * Returns FALSE when it can't be set up (before signalling the thread start) or gave up after errors (after
 * signalling it); the caller then runs _loop. */

static gboolean
_loop_darwin (ArvGvStreamThreadData *thread_data, ArvGvStreamRecvmsgX recvmsg_x_func, guint batch_size)
{
	ArvGvStreamMsghdrX *hdrs;
	struct iovec *iovs;
	struct sockaddr_storage *names;
	gboolean want_source;
	struct kevent changes[2];
	struct kevent events[2];
	GPollFD cancel_fd = {-1, 0, 0};
	gboolean use_cancel_fd;
	char *slots;
	gsize slot_size;
	guint64 time_us;
	guint lowat_packets;
	int lowat_bytes;
	int current_lowat = -1;
	gboolean kevent_error_reported = FALSE;
	guint n_rx_errors = 0;
	gboolean give_up = FALSE;
	guint active_wait_us;
	int socket_fd;
	int kq;
	guint i;

	socket_fd = g_socket_get_fd (thread_data->socket);

	kq = kqueue ();
	if (kq < 0) {
		arv_warning_stream_thread ("[GvStream::loop_darwin] kqueue failed (%s), using the standard loop",
					   g_strerror (errno));
		return FALSE;
	}

	/* The UDP payload is at most scps_packet_size - 28 bytes; the margin makes a full slot unambiguous */
	slot_size = ((gsize) thread_data->scps_packet_size + 64 + 63) & ~((gsize) 63);
	/* posix_memalign rather than g_aligned_alloc, which needs GLib 2.72 */
	if (posix_memalign ((void **) &slots, 64, batch_size * slot_size) != 0) {
		arv_warning_stream_thread ("[GvStream::loop_darwin] Can't allocate %u receive slots, using the"
					   " standard loop", batch_size);
		close (kq);
		return FALSE;
	}
	/* Never processed before being written, but a defect elsewhere should read zeros, not old heap contents */
	memset (slots, 0, batch_size * slot_size);
	hdrs = g_new0 (ArvGvStreamMsghdrX, batch_size);
	iovs = g_new0 (struct iovec, batch_size);
	names = g_new0 (struct sockaddr_storage, batch_size);

	for (i = 0; i < batch_size; i++) {
		iovs[i].iov_base = slots + i * slot_size;
		iovs[i].iov_len = slot_size;
		hdrs[i].msg_iov = &iovs[i];
		hdrs[i].msg_iovlen = 1;
	}

	lowat_packets = _get_env_uint ("ARV_GV_STREAM_RX_LOWAT_PACKETS", ARV_GV_STREAM_RX_LOWAT_PACKETS_DEFAULT,
				       G_MAXINT / 65536);
	/* 0 or 1: wake on the first datagram, whatever its size */
	lowat_bytes = lowat_packets <= 1 ? 1 : (int) lowat_packets * (int) (thread_data->scps_packet_size - 20 - 8);

	active_wait_us = MIN (ARV_GV_STREAM_RX_ACTIVE_WAIT_US,
			      MIN (thread_data->packet_timeout_us, thread_data->initial_packet_timeout_us));
	active_wait_us = MAX (active_wait_us, 100);

	use_cancel_fd = g_cancellable_make_pollfd (thread_data->cancellable, &cancel_fd);
	if (use_cancel_fd) {
		EV_SET (&changes[0], cancel_fd.fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
		if (kevent (kq, changes, 1, NULL, 0, NULL) < 0) {
			arv_warning_stream_thread ("[GvStream::loop_darwin] Can't watch the cancellable (%s)",
						   g_strerror (errno));
			g_cancellable_release_fd (thread_data->cancellable);
			use_cancel_fd = FALSE;
		}
	}

	thread_data->rx_mode = 1;

	arv_info_stream ("[GvStream::loop_darwin] recvmsg_x method, batch %u, slot %" G_GSIZE_FORMAT
			 " bytes, low-water mark %u packets (%d bytes), active wait %u us",
			 batch_size, slot_size, lowat_packets, lowat_bytes, active_wait_us);

        _signal_thread_started (thread_data);

	time_us = g_get_monotonic_time ();

	do {
		struct timespec timeout;
		gboolean active;
		gboolean socket_ready = FALSE;
		guint64 n_drained = 0;
		guint64 wait_us;
		int n_changes = 0;
		int wanted_lowat;
		int n_events;
		int errsv;

		active = thread_data->frames != NULL ||
			time_us - thread_data->last_datagram_us < ARV_GV_STREAM_RX_ACTIVE_US;

		/* When idle, wake on the first datagram */
		wanted_lowat = active ? lowat_bytes : 1;
		if (wanted_lowat != current_lowat) {
			EV_SET (&changes[0], socket_fd, EVFILT_READ, EV_ADD, NOTE_LOWAT, wanted_lowat, NULL);
			n_changes = 1;
			current_lowat = wanted_lowat;
		}

		wait_us = active ? active_wait_us : ARV_GV_STREAM_POLL_TIMEOUT_US;
		timeout.tv_sec = wait_us / 1000000;
		timeout.tv_nsec = (wait_us % 1000000) * 1000;

		do {
			n_events = kevent (kq, changes, n_changes, events, G_N_ELEMENTS (events), &timeout);
			errsv = errno;
		} while (n_events < 0 && errsv == EINTR);

		if (n_events < 0) {
			arv_warning_stream_thread ("[GvStream::loop_darwin] kevent failed (%s)", g_strerror (errsv));
			current_lowat = -1;
			/* Never spin on a failing kevent */
			g_usleep (active_wait_us);
		}

		for (i = 0; n_events > 0 && i < (guint) n_events; i++) {
			if (G_UNLIKELY (events[i].flags & EV_ERROR)) {
				/* A refused registration is not retried at once, which would spin: the wait
				 * degrades to the timeouts until the low-water mark changes */
				if (!kevent_error_reported)
					arv_warning_stream_thread ("[GvStream::loop_darwin] kevent registration of fd %d"
								   " failed (%s)", (int) events[i].ident,
								   g_strerror ((int) events[i].data));
				kevent_error_reported = TRUE;
				continue;
			}
			if (events[i].ident == (uintptr_t) socket_fd && events[i].filter == EVFILT_READ)
				socket_ready = TRUE;
		}

		if (socket_ready)
			thread_data->n_rx_wakeups++;

		/* Drain the socket, whatever woke the thread: after a timeout there may be a tail below the
		 * low-water mark. A batch shorter than requested means the queue was empty. */
		for (;;) {
			ArvGvStreamFrameData *frame = NULL;
			ssize_t n_msgs;

			/* Source addresses are asked for only until the socket is connected (_connect_to_source):
			 * the per-datagram name copy is a cost */
			want_source = thread_data->connect_socket && !thread_data->socket_connected;
			for (i = 0; i < batch_size; i++) {
				hdrs[i].msg_datalen = slot_size;
				hdrs[i].msg_flags = 0;
				hdrs[i].msg_name = want_source ? &names[i] : NULL;
				hdrs[i].msg_namelen = want_source ? sizeof (names[i]) : 0;
			}

			n_msgs = recvmsg_x_func (socket_fd, hdrs, batch_size, MSG_DONTWAIT);
			if (n_msgs <= 0) {
				int errsv = errno;

				if (n_msgs < 0 && errsv != EWOULDBLOCK && errsv != EAGAIN && errsv != EINTR) {
					n_rx_errors++;
					if (n_rx_errors == 1)
						arv_warning_stream_thread ("[GvStream::loop_darwin] recvmsg_x failed: %s",
									   g_strerror (errsv));
					if (n_rx_errors >= ARV_GV_STREAM_RX_MAX_ERRORS) {
						arv_warning_stream_thread ("[GvStream::loop_darwin] recvmsg_x failed %u"
									   " times in a row (%s), using the standard"
									   " loop", n_rx_errors, g_strerror (errsv));
						give_up = TRUE;
					} else
						/* The data is still queued: kevent would return at once */
						g_usleep (active_wait_us);
				} else
					n_rx_errors = 0;
				break;
			}

			n_rx_errors = 0;

			time_us = g_get_monotonic_time ();
			thread_data->last_datagram_us = time_us;
			n_drained += n_msgs;

			thread_data->n_rx_batches++;
			if ((guint64) n_msgs > thread_data->n_rx_max_batch)
				thread_data->n_rx_max_batch = n_msgs;

			for (i = 0; i < (guint) n_msgs; i++) {
				if (G_UNLIKELY (hdrs[i].msg_datalen >= slot_size || (hdrs[i].msg_flags & MSG_TRUNC) != 0)) {
					/* Truncated, or possibly: never larger than scps_packet_size in a valid stream */
					thread_data->n_rx_truncated++;
					continue;
				}

				if (G_UNLIKELY (want_source) && hdrs[i].msg_namelen > 0 &&
				    _gvsp_packet_size_is_valid (iovs[i].iov_base, hdrs[i].msg_datalen) &&
				    _connect_to_source (thread_data, (struct sockaddr *) &names[i],
							hdrs[i].msg_namelen))
					want_source = FALSE;

				frame = _process_packet (thread_data, iovs[i].iov_base, hdrs[i].msg_datalen, time_us);

				/* Hand a finished frame over without waiting for the end of the batch */
				if (frame != NULL && frame->last_valid_packet == frame->n_packets - 1) {
					_check_frame_completion (thread_data, time_us, frame);
					frame = NULL;
				}
			}

			_check_frame_completion (thread_data, time_us, frame);
			_sample_thread_stats (thread_data, time_us, FALSE);

			if ((guint) n_msgs < batch_size ||
			    g_cancellable_is_cancelled (thread_data->cancellable))
				break;
		}

		if (n_drained == 0) {
			time_us = g_get_monotonic_time ();
			_check_frame_completion (thread_data, time_us, NULL);
			_check_source_idle (thread_data, time_us);
			_sample_thread_stats (thread_data, time_us, FALSE);
		}

	} while (!give_up && !g_cancellable_is_cancelled (thread_data->cancellable));

	if (use_cancel_fd)
		g_cancellable_release_fd (thread_data->cancellable);

	close (kq);
	g_free (names);
	g_free (iovs);
	g_free (hdrs);
	free (slots);

	if (give_up) {
		thread_data->rx_mode = 0;
		return FALSE;
	}

	return TRUE;
}

/* ARV_GV_STREAM_RX_BATCH: datagrams per recvmsg_x call, 1 to 256, default 64; 0 selects _loop. Also
 * _loop if recvmsg_x can't be resolved. */

static ArvGvStreamRecvmsgX
_get_recvmsg_x (guint *batch_size)
{
	ArvGvStreamRecvmsgX recvmsg_x_func;

	*batch_size = _get_env_uint ("ARV_GV_STREAM_RX_BATCH", ARV_GV_STREAM_RX_BATCH_DEFAULT,
				     ARV_GV_STREAM_RX_BATCH_MAX);
	if (*batch_size == 0) {
		arv_info_stream_thread ("[GvStream::loop_darwin] ARV_GV_STREAM_RX_BATCH=0, using the standard loop");
		return NULL;
	}

	recvmsg_x_func = (ArvGvStreamRecvmsgX) dlsym (RTLD_DEFAULT, "recvmsg_x");
	if (recvmsg_x_func == NULL)
		arv_warning_stream_thread ("[GvStream::loop_darwin] recvmsg_x not found, using the standard loop");

	return recvmsg_x_func;
}

#endif /* __APPLE__ */


#if ARAVIS_HAS_PACKET_SOCKET

static void
_set_socket_filter (int socket,
                    guint32 source_ip, guint32 source_port, guint32 destination_ip, guint32 destination_port,
                    gboolean skip_outgoing)
{

#if 0
/*
 * sudo tcpdump -i lo -d "inbound and udp and src host 192.168.0.1 and src port 10 and dst host 192.168.0.2 and dst port 20"
 *
 * (000) ldh      [type]
 * (001) jeq      #0x4             jt 18	jf 2
 * (002) ldh      [12]
 * (003) jeq      #0x800           jt 4	jf 18
 * (004) ldb      [23]
 * (005) jeq      #0x11            jt 6	jf 18
 * (006) ld       [26]
 * (007) jeq      #0xc0a80001      jt 8	jf 18
 * (008) ldh      [20]
 * (009) jset     #0x1fff          jt 18	jf 10
 * (010) ldxb     4*([14]&0xf)
 * (011) ldh      [x + 14]
 * (012) jeq      #0xa             jt 13	jf 18
 * (013) ld       [30]
 * (014) jeq      #0xc0a80002      jt 15	jf 18
 * (015) ldh      [x + 16]
 * (016) jeq      #0x14            jt 17	jf 18
 * (017) ret      #262144
 * (018) ret      #0
 */

	struct sock_filter bpf[] = {
		/*  0 */ BPF_STMT (BPF_LD  | BPF_H   | BPF_ABS, SKF_AD_OFF + SKF_AD_PKTTYPE),
		/*  1 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   PACKET_OUTGOING, 16, 0),
		/*  2 */ BPF_STMT (BPF_LD  | BPF_H   | BPF_ABS, 12),
		/*  3 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   ETH_P_IP, 0, 14),
		/*  4 */ BPF_STMT (BPF_LD  | BPF_B   | BPF_ABS, 23),
		/*  5 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   IPPROTO_UDP, 0, 12),
		/*  6 */ BPF_STMT (BPF_LD  | BPF_W   | BPF_ABS, 26),
		/*  7 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   source_ip, 0, 10),
		/*  8 */ BPF_STMT (BPF_LD  | BPF_H   | BPF_ABS, 20),
		/*  9 */ BPF_JUMP (BPF_JMP | BPF_JSET | BPF_K,  0x1fff, 8, 0),
		/* 10 */ BPF_STMT (BPF_LDX | BPF_B   | BPF_MSH, 14),
		/* 11 */ BPF_STMT (BPF_LD  | BPF_H   | BPF_IND, 14),
		/* 12 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   source_port, 0, 5),
		/* 13 */ BPF_STMT (BPF_LD  | BPF_W   | BPF_ABS, 30),
		/* 14 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   destination_ip, 0, 3),
		/* 15 */ BPF_STMT (BPF_LD  | BPF_H   | BPF_IND, 16),
		/* 16 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   destination_port, 0, 1),
		/* 17 */ BPF_STMT (BPF_RET | BPF_K, 262144),
		/* 18 */ BPF_STMT (BPF_RET | BPF_K, 0)
	};

#else /* Variant without source port check. */
/*
 * sudo tcpdump -i lo -d "inbound and udp and src host 192.168.0.1 and dst host 192.168.0.2 and dst port 20"
 *
 * (000) ldh      [type]
 * (001) jeq      #0x4             jt 16	jf 2
 * (002) ldh      [12]
 * (003) jeq      #0x800           jt 4	jf 16
 * (004) ldb      [23]
 * (005) jeq      #0x11            jt 6	jf 16
 * (006) ld       [26]
 * (007) jeq      #0xc0a80001      jt 8	jf 16
 * (008) ld       [30]
 * (009) jeq      #0xc0a80002      jt 10	jf 16
 * (010) ldh      [20]
 * (011) jset     #0x1fff          jt 16	jf 12
 * (012) ldxb     4*([14]&0xf)
 * (013) ldh      [x + 16]
 * (014) jeq      #0x14            jt 15	jf 16
 * (015) ret      #262144
 * (016) ret      #0
 */

        struct sock_filter bpf_skip_outgoing[] = {
                /*  0 */ BPF_STMT (BPF_LD  | BPF_H   | BPF_ABS, SKF_AD_OFF + SKF_AD_PKTTYPE),
                /*  1 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   PACKET_OUTGOING, 14, 0),
                /*  2 */ BPF_STMT (BPF_LD  | BPF_H   | BPF_ABS, 12),
                /*  3 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   ETH_P_IP, 0, 12),
                /*  4 */ BPF_STMT (BPF_LD  | BPF_B   | BPF_ABS, 23),
                /*  5 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   IPPROTO_UDP, 0, 10),
                /*  6 */ BPF_STMT (BPF_LD  | BPF_W   | BPF_ABS, 26),
                /*  7 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   source_ip, 0, 8),
                /*  8 */ BPF_STMT (BPF_LD  | BPF_W   | BPF_ABS, 30),
                /*  9 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   destination_ip, 0, 6),
                /* 10 */ BPF_STMT (BPF_LD  | BPF_H   | BPF_ABS, 20),
                /* 11 */ BPF_JUMP (BPF_JMP | BPF_JSET | BPF_K,  0x1fff, 4, 0),
                /* 12 */ BPF_STMT (BPF_LDX | BPF_B   | BPF_MSH, 14),
                /* 13 */ BPF_STMT (BPF_LD  | BPF_H   | BPF_IND, 16),
                /* 14 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   destination_port, 0, 1),
                /* 15 */ BPF_STMT (BPF_RET | BPF_K, 262144),
                /* 16 */ BPF_STMT (BPF_RET | BPF_K, 0)
        };

        struct sock_filter bpf[] = {
                /*  0 */ BPF_STMT (BPF_LD  | BPF_H   | BPF_ABS, 12),
                /*  1 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   ETH_P_IP, 0, 12),
                /*  2 */ BPF_STMT (BPF_LD  | BPF_B   | BPF_ABS, 23),
                /*  3 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   IPPROTO_UDP, 0, 10),
                /*  4 */ BPF_STMT (BPF_LD  | BPF_W   | BPF_ABS, 26),
                /*  5 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   source_ip, 0, 8),
                /*  6 */ BPF_STMT (BPF_LD  | BPF_W   | BPF_ABS, 30),
                /*  7 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   destination_ip, 0, 6),
                /*  8 */ BPF_STMT (BPF_LD  | BPF_H   | BPF_ABS, 20),
                /*  9 */ BPF_JUMP (BPF_JMP | BPF_JSET | BPF_K,  0x1fff, 4, 0),
                /* 10 */ BPF_STMT (BPF_LDX | BPF_B   | BPF_MSH, 14),
                /* 11 */ BPF_STMT (BPF_LD  | BPF_H   | BPF_IND, 16),
                /* 12 */ BPF_JUMP (BPF_JMP | BPF_JEQ | BPF_K,   destination_port, 0, 1),
                /* 13 */ BPF_STMT (BPF_RET | BPF_K, 262144),
                /* 14 */ BPF_STMT (BPF_RET | BPF_K, 0)
        };
#endif

	struct sock_fprog bpf_prog;

       if (skip_outgoing) {
              bpf_prog.len = G_N_ELEMENTS (bpf_skip_outgoing);
              bpf_prog.filter = bpf_skip_outgoing;
       } else {
              bpf_prog.len = G_N_ELEMENTS (bpf);
              bpf_prog.filter = bpf;
       }

	arv_info_stream_thread ("[GvStream::set_socket_filter] source ip = 0x%08x - port = %d -"
                                " dest ip = 0x%08x - port %d",
				 source_ip, source_port, destination_ip, destination_port);

	if (setsockopt(socket, SOL_SOCKET, SO_ATTACH_FILTER, &bpf_prog, sizeof(bpf_prog)) != 0)
		arv_warning_stream_thread ("[GvStream::set_socket_filter] Failed to attach Beckerley Packet Filter"
                                           " to stream socket");
}

static unsigned
_interface_index_from_address (guint32 ip)
{
    struct ifaddrs *ifaddr = NULL;
    struct ifaddrs *ifa;
    unsigned index = 0;

    if (getifaddrs(&ifaddr) == -1) {
        return index;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
	    if (ifa->ifa_addr != NULL &&
		ifa->ifa_addr->sa_family == AF_INET) {
		    struct sockaddr_in *sa;

		    sa = (struct sockaddr_in *) (ifa->ifa_addr);
		    if (ip == g_ntohl (sa->sin_addr.s_addr)) {
			    index = if_nametoindex (ifa->ifa_name);
			    break;
		    }
	    }
    }

    freeifaddrs (ifaddr);

    return index;
}

typedef struct {
	guint32 version;
	guint32 offset_to_priv;
	struct tpacket_hdr_v1 h1;
} ArvGvStreamBlockDescriptor;

static void
_ring_buffer_loop (ArvGvStreamThreadData *thread_data)
{
	GPollFD poll_fd[2];
	char *buffer;
	struct tpacket_req3 req;
	struct sockaddr_ll local_address = {0};
	enum tpacket_versions version;
	int fd;
        unsigned ifindex;
	unsigned block_id;
	const guint8 *bytes;
	guint32 interface_address;
	guint32 device_address;
	gboolean use_poll;
        gboolean skip_outgoing;

	arv_info_stream ("[GvStream::loop] Packet socket method");

	fd = socket (PF_PACKET, SOCK_RAW, g_htons (ETH_P_ALL));
	if (fd < 0) {
		arv_warning_stream_thread ("[GvStream::loop] Failed to create AF_PACKET socket");
		goto af_packet_error;
	}

	version = TPACKET_V3;
	if (setsockopt (fd, SOL_PACKET, PACKET_VERSION, &version, sizeof(version)) < 0) {
		arv_warning_stream_thread ("[GvStream::loop] Failed to set packet version");
		goto socket_option_error;
	}

	req.tp_block_size = 1 << 21;
	req.tp_frame_size = 1024;
	req.tp_block_nr = 16;
	req.tp_frame_nr = (req.tp_block_size * req.tp_block_nr) / req.tp_frame_size;
	req.tp_sizeof_priv = 0;
	req.tp_retire_blk_tov = 5;
	req.tp_feature_req_word = TP_FT_REQ_FILL_RXHASH;
	if (setsockopt (fd, SOL_PACKET, PACKET_RX_RING, &req, sizeof(req)) < 0) {
		arv_warning_stream_thread ("[GvStream::loop] Failed to set packet rx ring");
		goto socket_option_error;
	}

	buffer = mmap (NULL, req.tp_block_size * req.tp_block_nr, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
	if (buffer == MAP_FAILED) {
		arv_warning_stream_thread ("[GvStream::loop] Failed to map ring buffer");
		goto map_error;
	}

	bytes = g_inet_address_to_bytes (thread_data->interface_address);
	interface_address = g_ntohl (*((guint32 *) bytes));
	bytes = g_inet_address_to_bytes (thread_data->device_address);
	device_address = g_ntohl (*((guint32 *) bytes));

        /* If the camera address is one of our own addresses, traffic goes through lo */
        if (_interface_index_from_address (device_address) != 0 ||
            (device_address >> 24) == 127) {
                ifindex = if_nametoindex ("lo");
                skip_outgoing = TRUE;
                arv_info_device ("[GvStream::_ring_buffer_loop] Local or loopback address detected, bind socket to lo");
        } else {
                ifindex = _interface_index_from_address (interface_address);
                skip_outgoing = FALSE;
        }

	local_address.sll_family   = AF_PACKET;
	local_address.sll_protocol = g_htons(ETH_P_IP);
	local_address.sll_ifindex  = ifindex;
	local_address.sll_hatype   = 0;
	local_address.sll_pkttype  = 0;
	local_address.sll_halen    = 0;
	if (bind (fd, (struct sockaddr *) &local_address, sizeof(local_address)) == -1) {
		arv_warning_stream_thread ("[GvStream::_ring_buffer_loop] Failed to bind packet socket");
		goto bind_error;
	}

	_set_socket_filter (fd,
                            device_address, thread_data->source_stream_port,
                            interface_address, thread_data->stream_port,
                            skip_outgoing);

	poll_fd[0].fd = fd;
	poll_fd[0].events =  G_IO_IN;
	poll_fd[0].revents = 0;

	use_poll = g_cancellable_make_pollfd (thread_data->cancellable, &poll_fd[1]);

	thread_data->rx_mode = 2;

        _signal_thread_started (thread_data);

	block_id = 0;
	do {
		ArvGvStreamBlockDescriptor *descriptor;
		guint64 time_us;

		time_us = g_get_monotonic_time ();

		descriptor = (void *) (buffer + block_id * req.tp_block_size);
		if ((descriptor->h1.block_status & TP_STATUS_USER) == 0) {
                        int timeout_ms;
			int n_events;
			int errsv;

			_check_frame_completion (thread_data, time_us, NULL);

                        if (thread_data->frames != NULL)
                                timeout_ms = MAX (1, thread_data->packet_timeout_us / 1000);
                        else
                                timeout_ms = ARV_GV_STREAM_POLL_TIMEOUT_US / 1000;

			do {
				n_events = g_poll (poll_fd, use_poll ? 2 : 1,  timeout_ms);
				errsv = errno;
			} while (n_events < 0 && errsv == EINTR);
		} else {
			ArvGvStreamFrameData *frame;
			const struct tpacket3_hdr *header;
			unsigned i;

			header = (void *) (((char *) descriptor) + descriptor->h1.offset_to_first_pkt);

			for (i = 0; i < descriptor->h1.num_pkts; i++) {
				const struct iphdr *ip;
				const ArvGvspPacket *packet;
				size_t size;

				gint64 ip_size;
				gint64 captured_size;

				ip = (void *) (((char *) header) + header->tp_mac + ETH_HLEN);
				packet = (void *) (((char *) ip) + sizeof (struct iphdr) + sizeof (struct udphdr));
				/* Signed: a datagram shorter than its IP and UDP headers must not underflow into a huge
				 * size, and the size is never more than what the ring captured */
				ip_size = (gint64) g_ntohs (ip->tot_len) -
					(gint64) (sizeof (struct iphdr) + sizeof (struct udphdr));
				captured_size = (gint64) header->tp_snaplen -
					(gint64) (ETH_HLEN + sizeof (struct iphdr) + sizeof (struct udphdr));
				size = MAX (0, MIN (ip_size, captured_size));

				frame = _process_packet (thread_data, packet, size, time_us);

				_check_frame_completion (thread_data, time_us, frame);

				header = (void *) (((char *) header) + header->tp_next_offset);
			}

			descriptor->h1.block_status = TP_STATUS_KERNEL;
			block_id = (block_id + 1) % req.tp_block_nr;
		}
	} while (!g_cancellable_is_cancelled (thread_data->cancellable));

	if (use_poll)
		g_cancellable_release_fd (thread_data->cancellable);

bind_error:
	munmap (buffer, req.tp_block_size * req.tp_block_nr);
socket_option_error:
map_error:
	close (fd);
af_packet_error:
        _signal_thread_started (thread_data);
}

#endif /* ARAVIS_HAS_PACKET_SOCKET */

typedef enum {
	ARV_GV_STREAM_THREAD_PRIORITY_DEFAULT = 0,
	ARV_GV_STREAM_THREAD_PRIORITY_HIGH = 1,
	ARV_GV_STREAM_THREAD_PRIORITY_REALTIME = 2
} ArvGvStreamThreadPriority;

static const char *
_thread_priority_to_string (ArvGvStreamThreadPriority mode)
{
	switch (mode) {
		case ARV_GV_STREAM_THREAD_PRIORITY_HIGH:
			return "high";
		case ARV_GV_STREAM_THREAD_PRIORITY_REALTIME:
			return "realtime";
		default:
			return "default";
	}
}

/* Raises the priority of the calling stream thread, from ARV_GV_STREAM_THREAD_PRIORITY = default |
 * high | realtime. Unset: realtime on Apple, default elsewhere. A refused realtime request falls
 * back to high, and a refused high request leaves the thread as it is. What was achieved is
 * exported as thread_priority_mode.
 *
 * On Apple, realtime is the Mach time constraint policy (see arv_make_thread_realtime). The kernel
 * demotes a real-time thread that computes for about 100 quanta without blocking; _loop blocks in
 * g_poll, and _loop_darwin in kevent, whenever the socket is empty, which resets that counter. A
 * receive loop that never blocks (busy polling) would be demoted, which stream_thread_min_priority
 * makes visible. */

static void
_configure_thread_priority (ArvGvStreamThreadData *thread_data)
{
	ArvGvStreamThreadPriority requested;
	ArvGvStreamThreadPriority achieved = ARV_GV_STREAM_THREAD_PRIORITY_DEFAULT;
	const char *env;

#ifdef __APPLE__
	{
		char name[64] = "";

		/* GLib names its threads on Apple, but not every build does */
		if (pthread_getname_np (pthread_self (), name, sizeof (name)) != 0 || name[0] == '\0') {
			pthread_setname_np ("arv_gv_stream");
			arv_debug_stream_thread ("[GvStream::configure_thread_priority] Thread named arv_gv_stream");
		} else {
			arv_debug_stream_thread ("[GvStream::configure_thread_priority] Thread already named '%s'",
						 name);
		}
	}
	requested = ARV_GV_STREAM_THREAD_PRIORITY_REALTIME;
#else
	requested = ARV_GV_STREAM_THREAD_PRIORITY_DEFAULT;
#endif

	env = g_getenv ("ARV_GV_STREAM_THREAD_PRIORITY");
	if (env != NULL) {
		if (g_ascii_strcasecmp (env, "default") == 0)
			requested = ARV_GV_STREAM_THREAD_PRIORITY_DEFAULT;
		else if (g_ascii_strcasecmp (env, "high") == 0)
			requested = ARV_GV_STREAM_THREAD_PRIORITY_HIGH;
		else if (g_ascii_strcasecmp (env, "realtime") == 0)
			requested = ARV_GV_STREAM_THREAD_PRIORITY_REALTIME;
		else
			arv_warning_stream_thread ("[GvStream::configure_thread_priority] Invalid"
						   " ARV_GV_STREAM_THREAD_PRIORITY '%s' (expected default, high or"
						   " realtime), using %s", env, _thread_priority_to_string (requested));
	}

	if (requested == ARV_GV_STREAM_THREAD_PRIORITY_REALTIME) {
		/* The priority argument is used by the Linux implementation; Mach has no equivalent */
		if (arv_make_thread_realtime (10))
			achieved = ARV_GV_STREAM_THREAD_PRIORITY_REALTIME;
		else
			requested = ARV_GV_STREAM_THREAD_PRIORITY_HIGH;
	}

	if (requested == ARV_GV_STREAM_THREAD_PRIORITY_HIGH) {
		if (arv_make_thread_high_priority (-10))
			achieved = ARV_GV_STREAM_THREAD_PRIORITY_HIGH;
	}

	thread_data->thread_priority_mode = achieved;

	if (env != NULL)
		arv_info_stream_thread ("[GvStream::configure_thread_priority] Stream thread priority: %s"
					" (ARV_GV_STREAM_THREAD_PRIORITY=%s)",
					_thread_priority_to_string (achieved), env);
	else
		arv_info_stream_thread ("[GvStream::configure_thread_priority] Stream thread priority: %s"
					" (platform default)", _thread_priority_to_string (achieved));
}

static void *
arv_gv_stream_thread (void *data)
{
	ArvGvStreamThreadData *thread_data = data;
#if ARAVIS_HAS_PACKET_SOCKET
	int fd;
#endif
#ifdef __APPLE__
	ArvGvStreamRecvmsgX recvmsg_x_func;
	guint rx_batch_size;
#endif

	thread_data->frames = NULL;
	thread_data->last_frame_id = 0;
	thread_data->first_packet = TRUE;
	/* Frame ids and sizes seen before a restart say nothing about the frames after it */
	thread_data->has_dropped_frame = FALSE;
	thread_data->has_underrun_frame = FALSE;
	thread_data->last_n_packets = 0;

	/* The sender may have changed while the thread was stopped (device reboot, new source port) */
	thread_data->last_datagram_us = g_get_monotonic_time ();
	if (thread_data->socket_connected && thread_data->connect_socket)
		_disconnect_from_source (thread_data, "stream thread start");

	_configure_socket_buffer (thread_data);
	/* Before the INIT callback, so that an application's own handler has the last word */
	_configure_thread_priority (thread_data);

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

#if ARAVIS_HAS_PACKET_SOCKET
	if (thread_data->use_packet_socket && (fd = socket (PF_PACKET, SOCK_RAW, g_htons (ETH_P_ALL))) >= 0) {
		close (fd);
		_ring_buffer_loop (thread_data);
	} else
#endif
#ifdef __APPLE__
	if ((recvmsg_x_func = _get_recvmsg_x (&rx_batch_size)) == NULL ||
	    !_loop_darwin (thread_data, recvmsg_x_func, rx_batch_size))
#endif
		_loop (thread_data);

	_flush_frames (thread_data, g_get_monotonic_time ());

	_sample_thread_stats (thread_data, g_get_monotonic_time (), TRUE);

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

	return NULL;
}

/* ArvGvStream implementation */

guint16
arv_gv_stream_get_port (ArvGvStream *gv_stream)
{
	ArvGvStreamPrivate *priv = arv_gv_stream_get_instance_private (gv_stream);

	g_return_val_if_fail (ARV_IS_GV_STREAM (gv_stream), 0);

	return priv->thread_data->stream_port;
}

static gboolean
arv_gv_stream_start_acquisition (ArvStream *stream, GError **error)
{
	ArvGvStreamPrivate *priv = arv_gv_stream_get_instance_private (ARV_GV_STREAM (stream));
	ArvGvStreamThreadData *thread_data;

	g_return_val_if_fail (priv->thread == NULL, FALSE);
	g_return_val_if_fail (priv->thread_data != NULL, FALSE);

	thread_data = priv->thread_data;

        thread_data->thread_started = FALSE;
	thread_data->cancellable = g_cancellable_new ();
	priv->thread = g_thread_new ("arv_gv_stream", arv_gv_stream_thread, priv->thread_data);

        g_mutex_lock (&thread_data->thread_started_mutex);
        while (!thread_data->thread_started)
                g_cond_wait (&thread_data->thread_started_cond,
                             &thread_data->thread_started_mutex);
        g_mutex_unlock (&thread_data->thread_started_mutex);

        return TRUE;
}

static gboolean
arv_gv_stream_stop_acquisition (ArvStream *stream, GError **error)
{
	ArvGvStreamPrivate *priv = arv_gv_stream_get_instance_private (ARV_GV_STREAM (stream));
	ArvGvStreamThreadData *thread_data;

	g_return_val_if_fail (priv->thread != NULL, FALSE);
	g_return_val_if_fail (priv->thread_data != NULL, FALSE);

	thread_data = priv->thread_data;

	g_cancellable_cancel (thread_data->cancellable);
	g_thread_join (priv->thread);
	g_clear_object (&thread_data->cancellable);

	priv->thread = NULL;

        return TRUE;
}

/**
 * arv_gv_stream_new: (skip)
 * @gv_device: a #ArvGvDevice
 * @callback: (scope call): processing callback
 * @callback_data: (closure): user data for @callback
 *
 * Return value: (transfer full): a new #ArvStream.
 */

ArvStream *
arv_gv_stream_new (ArvGvDevice *gv_device, ArvStreamCallback callback, void *callback_data, GDestroyNotify destroy, GError **error)
{
	return g_initable_new (ARV_TYPE_GV_STREAM, NULL, error,
			       "device", gv_device,
			       "callback", callback,
			       "callback-data", callback_data,
                               "destroy-notify", destroy,
			       NULL);
}

/* ArvStream implementation */

/**
 * arv_gv_stream_get_statistics:
 * @gv_stream: a #ArvGvStream
 * @n_resent_packets: (out)
 * @n_missing_packets: (out)
 */

void
arv_gv_stream_get_statistics (ArvGvStream *gv_stream,
			      guint64 *n_resent_packets,
			      guint64 *n_missing_packets)

{
	ArvGvStreamPrivate *priv = arv_gv_stream_get_instance_private (gv_stream);
	ArvGvStreamThreadData *thread_data;

	g_return_if_fail (ARV_IS_GV_STREAM (gv_stream));

	thread_data = priv->thread_data;

	if (n_resent_packets != NULL)
		*n_resent_packets = thread_data->n_resent_packets;
	if (n_missing_packets != NULL)
		*n_missing_packets = thread_data->n_missing_packets;
}

static void
arv_gv_stream_set_property (GObject * object, guint prop_id,
                            const GValue * value, GParamSpec * pspec)
{
	ArvGvStreamPrivate *priv = arv_gv_stream_get_instance_private (ARV_GV_STREAM (object));
	ArvGvStreamThreadData *thread_data;

	thread_data = priv->thread_data;

	switch (prop_id) {
		case ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER:
			thread_data->socket_buffer_option = g_value_get_enum (value);
			break;
		case ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER_SIZE:
			thread_data->socket_buffer_size = g_value_get_int (value);
			break;
		case ARV_GV_STREAM_PROPERTY_PACKET_RESEND:
			thread_data->packet_resend = g_value_get_enum (value);
			break;
		case ARV_GV_STREAM_PROPERTY_PACKET_REQUEST_RATIO:
			thread_data->packet_request_ratio = g_value_get_double (value);
			break;
		case ARV_GV_STREAM_PROPERTY_INITIAL_PACKET_TIMEOUT:
			thread_data->initial_packet_timeout_us = g_value_get_uint (value);
			break;
		case ARV_GV_STREAM_PROPERTY_PACKET_TIMEOUT:
			thread_data->packet_timeout_us = g_value_get_uint (value);
			break;
		case ARV_GV_STREAM_PROPERTY_FRAME_RETENTION:
			thread_data->frame_retention_us = g_value_get_uint (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_gv_stream_get_property (GObject * object, guint prop_id,
			    GValue * value, GParamSpec * pspec)
{
	ArvGvStreamPrivate *priv = arv_gv_stream_get_instance_private (ARV_GV_STREAM (object));
	ArvGvStreamThreadData *thread_data;

	thread_data = priv->thread_data;

	switch (prop_id) {
		case ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER:
			g_value_set_enum (value, thread_data->socket_buffer_option);
			break;
		case ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER_SIZE:
			g_value_set_int (value, thread_data->socket_buffer_size);
			break;
		case ARV_GV_STREAM_PROPERTY_PACKET_RESEND:
			g_value_set_enum (value, thread_data->packet_resend);
			break;
		case ARV_GV_STREAM_PROPERTY_PACKET_REQUEST_RATIO:
			g_value_set_double (value, thread_data->packet_request_ratio);
			break;
		case ARV_GV_STREAM_PROPERTY_INITIAL_PACKET_TIMEOUT:
			g_value_set_uint (value, thread_data->initial_packet_timeout_us);
			break;
		case ARV_GV_STREAM_PROPERTY_PACKET_TIMEOUT:
			g_value_set_uint (value, thread_data->packet_timeout_us);
			break;
		case ARV_GV_STREAM_PROPERTY_FRAME_RETENTION:
			g_value_set_uint (value, thread_data->frame_retention_us);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_gv_stream_init (ArvGvStream *gv_stream)
{
	ArvGvStreamPrivate *priv = arv_gv_stream_get_instance_private (gv_stream);

	priv->thread_data = g_new0 (ArvGvStreamThreadData, 1);
}

/* ARV_GV_STREAM_CONNECT: see _connect_to_source */

static gboolean
_get_connect_option (void)
{
#ifdef __APPLE__
	gboolean default_value = TRUE;
#else
	gboolean default_value = FALSE;
#endif
	const char *env;

	env = g_getenv ("ARV_GV_STREAM_CONNECT");
	if (env == NULL)
		return default_value;
	if (g_strcmp0 (env, "1") == 0)
		return TRUE;
	if (g_strcmp0 (env, "0") == 0)
		return FALSE;

	arv_warning_stream ("[GvStream::stream_new] Invalid ARV_GV_STREAM_CONNECT '%s' (expected 1 or 0), using %d",
			    env, default_value);
	return default_value;
}

static void
_read_env_uint (const char *name, guint *value)
{
	const char *env = g_getenv (name);
	guint64 parsed;
	char *end = NULL;

	if (env == NULL || env[0] == '\0')
		return;

	parsed = g_ascii_strtoull (env, &end, 10);
	if (end == env || *end != '\0' || parsed > G_MAXUINT) {
		arv_warning_stream ("[GvStream::constructed] Ignoring %s '%s' (expected a duration in µs)", name, env);
		return;
	}

	*value = parsed;
	arv_info_stream ("[GvStream::constructed] %s = %u", name, *value);
}

static void
_read_env_ratio (const char *name, double *value)
{
	const char *env = g_getenv (name);
	double parsed;
	char *end = NULL;

	if (env == NULL || env[0] == '\0')
		return;

	parsed = g_ascii_strtod (env, &end);
	if (end == env || *end != '\0' || !(parsed >= 0.0 && parsed <= 2.0)) {
		arv_warning_stream ("[GvStream::constructed] Ignoring %s '%s' (expected a ratio between 0 and 2)",
				    name, env);
		return;
	}

	*value = parsed;
	arv_info_stream ("[GvStream::constructed] %s = %g", name, *value);
}

/* ARV_GV_STREAM_FAST_FAIL = 1 | 0. When unset it is on for Apple and off elsewhere, so that other platforms keep the
 * upstream frame logic. */

static gboolean
_read_fast_fail (void)
{
	const char *env = g_getenv ("ARV_GV_STREAM_FAST_FAIL");
#ifdef __APPLE__
	gboolean fast_fail = TRUE;
#else
	gboolean fast_fail = FALSE;
#endif

	if (env != NULL && env[0] != '\0') {
		if (g_strcmp0 (env, "1") == 0)
			fast_fail = TRUE;
		else if (g_strcmp0 (env, "0") == 0)
			fast_fail = FALSE;
		else
			arv_warning_stream ("[GvStream::constructed] Ignoring ARV_GV_STREAM_FAST_FAIL '%s'"
					    " (expected 1 or 0)", env);
	}

	arv_info_stream ("[GvStream::constructed] Fast fail %s", fast_fail ? "on" : "off");

	return fast_fail;
}

static void
arv_gv_stream_constructed (GObject *object)
{
	ArvStream *stream = ARV_STREAM (object);
	ArvGvStream *gv_stream = ARV_GV_STREAM (object);
	ArvGvStreamPrivate *priv = arv_gv_stream_get_instance_private (ARV_GV_STREAM (stream));
	ArvGvStreamOption options;
        GError *error = NULL;
	GInetAddress *interface_address;
	GInetAddress *device_address;
	guint64 timestamp_tick_frequency;
	const guint8 *address_bytes;
	GInetSocketAddress *local_address;
	guint packet_size;

	G_OBJECT_CLASS (arv_gv_stream_parent_class)->constructed (object);

	g_object_get (object, "device", &priv->gv_device, NULL);

        priv->stream_channel = arv_device_get_integer_feature_value(ARV_DEVICE(priv->gv_device),
                                                                    "ArvGevStreamChannelSelector", &error);
        if (error != NULL) {
		arv_stream_take_init_error (stream, error);
		g_clear_object (&priv->gv_device);
		return;
        }

        arv_info_stream ("[GvStream::stream_new] Stream channel = %u", priv->stream_channel);

	timestamp_tick_frequency = arv_gv_device_get_timestamp_tick_frequency (priv->gv_device, NULL);
	options = arv_gv_device_get_stream_options (priv->gv_device);

	packet_size = arv_gv_device_get_packet_size (priv->gv_device, NULL);
	if (packet_size <= ARV_GVSP_PACKET_PROTOCOL_OVERHEAD(FALSE)) {
		arv_gv_device_set_packet_size (priv->gv_device, ARV_GV_DEVICE_GVSP_PACKET_SIZE_DEFAULT, NULL);
		arv_info_stream ("[GvStream::stream_new] Packet size set to default value (%d)",
				  ARV_GV_DEVICE_GVSP_PACKET_SIZE_DEFAULT);
	}

	packet_size = arv_gv_device_get_packet_size (priv->gv_device, NULL);
	arv_info_stream ("[GvStream::stream_new] Packet size = %d byte(s)", packet_size);

	if (packet_size <= ARV_GVSP_PACKET_PROTOCOL_OVERHEAD(FALSE)) {
		arv_stream_take_init_error (stream, g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
								 "Invalid packet size (%d byte(s))", packet_size));
		g_clear_object (&priv->gv_device);
		return;
	}

	priv->thread_data->stream = stream;

	g_object_get (object,
		      "callback", &priv->thread_data->callback,
		      "callback-data", &priv->thread_data->callback_data,
		      NULL);

	priv->thread_data->timestamp_tick_frequency = timestamp_tick_frequency;
	priv->thread_data->scps_packet_size = packet_size;
	priv->thread_data->use_packet_socket = (options & ARV_GV_STREAM_OPTION_PACKET_SOCKET_DISABLED) == 0;

	priv->thread_data->packet_id = 65300;

	priv->thread_data->connect_socket = _get_connect_option ();

	/* The construct properties already hold their defaults (or the caller's values); the environment overrides
	 * them only where it is set. A later g_object_set still wins. */
	priv->thread_data->fast_fail = _read_fast_fail ();
	_read_env_uint ("ARV_GV_STREAM_INITIAL_PACKET_TIMEOUT_US", &priv->thread_data->initial_packet_timeout_us);
	_read_env_uint ("ARV_GV_STREAM_PACKET_TIMEOUT_US", &priv->thread_data->packet_timeout_us);
	_read_env_uint ("ARV_GV_STREAM_FRAME_RETENTION_US", &priv->thread_data->frame_retention_us);
	_read_env_ratio ("ARV_GV_STREAM_PACKET_REQUEST_RATIO", &priv->thread_data->packet_request_ratio);

	priv->thread_data->histogram = arv_histogram_new (3, 100, 2000, 0);

	arv_histogram_set_variable_name (priv->thread_data->histogram, 0, "frame_retention");
	arv_histogram_set_variable_name (priv->thread_data->histogram, 1, "packet_time");
	arv_histogram_set_variable_name (priv->thread_data->histogram, 2, "inter_packet");

	interface_address = g_inet_socket_address_get_address
                (G_INET_SOCKET_ADDRESS (arv_gv_device_get_interface_address (priv->gv_device)));
	device_address = g_inet_socket_address_get_address
                (G_INET_SOCKET_ADDRESS (arv_gv_device_get_device_address (priv->gv_device)));

	priv->thread_data->socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, NULL);
	priv->thread_data->device_address = g_object_ref (device_address);
	if (g_inet_address_get_family (device_address) == G_SOCKET_FAMILY_IPV4)
		memcpy (&priv->thread_data->device_address_be, g_inet_address_to_bytes (device_address), 4);
	priv->thread_data->interface_address = g_object_ref (interface_address);
	priv->thread_data->device_socket_address = g_inet_socket_address_new (device_address, ARV_GVCP_PORT);
	g_socket_set_blocking (priv->thread_data->socket, FALSE);

	priv->thread_data->interface_socket_address = arv_socket_bind_with_range (priv->thread_data->socket,
                                                                                  interface_address, 0, FALSE, NULL);

	local_address = G_INET_SOCKET_ADDRESS (g_socket_get_local_address (priv->thread_data->socket, NULL));
	priv->thread_data->stream_port = g_inet_socket_address_get_port (local_address);
	g_object_unref (local_address);

	address_bytes = g_inet_address_to_bytes (interface_address);
	arv_device_set_integer_feature_value (ARV_DEVICE (priv->gv_device),
                                              "ArvGevSCDA", g_htonl (*((guint32 *) address_bytes)), NULL);
	arv_device_set_integer_feature_value (ARV_DEVICE (priv->gv_device),
                                              "ArvGevSCPHostPort", priv->thread_data->stream_port, NULL);
	priv->thread_data->source_stream_port = arv_device_get_integer_feature_value (ARV_DEVICE (priv->gv_device),
                                                                                      "ArvGevSCSP", NULL);

	arv_info_stream ("[GvStream::stream_new] Destination stream port = %d", priv->thread_data->stream_port);
	arv_info_stream ("[GvStream::stream_new] Source stream port = %d", priv->thread_data->source_stream_port);

        /* Stream infos. Every one is a guint64; "counter" means it only grows for the life of the stream object,
         * "state" that it is a current value. The upstream ones keep their meaning except where noted.
         *
         * n_completed_buffers   counter, buffers  buffers returned with status SUCCESS.
         * n_failures            counter, buffers  buffers returned with any other status but ABORTED; with fast fail
         *                                         this includes frames closed early (MISSING_PACKETS) because they
         *                                         could not complete once a newer frame existed.
         * n_underruns           counter, packets  packets that found no input buffer: with fast fail, every packet of
         *                                         a frame dropped for that reason (until the next frame starts).
         * n_timeouts            counter, buffers  buffers returned with status TIMEOUT.
         * n_aborted             counter, buffers  buffers returned ABORTED (stream thread stop).
         * n_missing_frames      counter, frames   gaps in the frame id sequence, one per gap whatever its length
         *                                         (upstream), plus, with fast fail, one per frame skipped without a
         *                                         buffer (n_skipped_frames): a skipped frame is missing for the
         *                                         client although no gap precedes it.
         * n_missing_packets     counter, packets  packets missing from failed buffers; never negative (a trailer
         *                                         below packets already received used to wrap it).
         * n_ignored_packets     counter, packets  valid packets not used: late frames, the rest of a skipped frame,
         *                                         unknown content types, status packets for frames not open.
         * n_resend_requests     counter, packets  packets asked for in PACKETRESEND commands.
         *
         * Added by this receiver:
         * socket_buffer_size    state, bytes      SO_RCVBUF as read back after the last change, 0 if never set.
         * n_underrun_frames     counter, frames   distinct frames that found no input buffer.
         * n_skipped_frames      counter, frames   fast fail: frames dropped without taking a buffer, because no
         *                                         buffer was free at their first packet or their head could not be
         *                                         recovered. Also counted in n_missing_frames.
         * fast_fail             state, 0 or 1     ARV_GV_STREAM_FAST_FAIL as read at construction.
         * n_rx_wakeups          counter, wakeups  waits that returned with the socket readable.
         * n_rx_batches          counter, calls    receive calls that returned at least one datagram.
         * n_rx_max_batch        state, datagrams  most datagrams returned by one receive call.
         * stream_thread_cpu_us  state, us         user + system CPU time of the stream thread (Apple only, 0
         *                                         elsewhere), sampled at most every 100 ms.
         * thread_priority_mode  state, 0 to 2     achieved stream thread priority: default, high, realtime.
         * stream_thread_min_priority state        lowest scheduler priority observed (Apple only): a drop shows a
         *                                         real-time thread demoted by the kernel.
         * rx_mode               state, 0 to 2     1 while the recvmsg_x loop runs; 2 while the packet socket loop
         *                                         runs; 0 for _loop, or after recvmsg_x failed 100 times in a row.
         * n_rx_truncated        counter, datagrams recvmsg_x datagrams that filled their slot or came back with
         *                                         MSG_TRUNC; not processed.
         * n_short_packets       counter, datagrams datagrams too short for their GVSP headers (or a leader for its
         *                                         leader); not processed, not in n_received_packets.
         * socket_connected      state, 0 or 1     the stream socket is connected to the sender learnt from traffic.
         * n_rx_foreign_source   counter, datagrams valid datagrams seen while learning, from an address other than
         *                                         the device's or from its GVCP port; 64 in one learning phase
         *                                         give up connecting.
         * n_socket_reconnects   counter, events   connections dissolved to learn the sender again: at stream thread
         *                                         start, or after 1 s without a datagram.
         * n_resend_send_errors  counter, commands PACKETRESEND commands the socket refused to send.
         */
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_completed_buffers",
                                 G_TYPE_UINT64, &priv->thread_data->n_completed_buffers);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_failures",
                                 G_TYPE_UINT64, &priv->thread_data->n_failures);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_underruns",
                                 G_TYPE_UINT64, &priv->thread_data->n_underruns);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_timeouts",
                                 G_TYPE_UINT64, &priv->thread_data->n_timeouts);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_aborted",
                                 G_TYPE_UINT64, &priv->thread_data->n_aborted);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_missing_frames",
                                 G_TYPE_UINT64, &priv->thread_data->n_missing_frames);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_size_mismatch_errors",
                                 G_TYPE_UINT64, &priv->thread_data->n_size_mismatch_errors);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_received_packets",
                                 G_TYPE_UINT64, &priv->thread_data->n_received_packets);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_missing_packets",
                                 G_TYPE_UINT64, &priv->thread_data->n_missing_packets);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_error_packets",
                                 G_TYPE_UINT64, &priv->thread_data->n_error_packets);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_ignored_packets",
                                 G_TYPE_UINT64, &priv->thread_data->n_ignored_packets);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_resend_requests",
                                 G_TYPE_UINT64, &priv->thread_data->n_resend_requests);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_resent_packets",
                                 G_TYPE_UINT64, &priv->thread_data->n_resent_packets);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_resend_ratio_reached",
                                 G_TYPE_UINT64, &priv->thread_data->n_resend_ratio_reached);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_resend_disabled",
                                 G_TYPE_UINT64, &priv->thread_data->n_resend_disabled);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_duplicated_packets",
                                 G_TYPE_UINT64, &priv->thread_data->n_duplicated_packets);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_transferred_bytes",
                                 G_TYPE_UINT64, &priv->thread_data->n_transferred_bytes);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_ignored_bytes",
                                 G_TYPE_UINT64, &priv->thread_data->n_ignored_bytes);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "socket_buffer_size",
                                 G_TYPE_UINT64, &priv->thread_data->socket_buffer_size_granted);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_underrun_frames",
                                 G_TYPE_UINT64, &priv->thread_data->n_underrun_frames);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_skipped_frames",
                                 G_TYPE_UINT64, &priv->thread_data->n_skipped_frames);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "fast_fail",
                                 G_TYPE_UINT64, &priv->thread_data->fast_fail);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_rx_wakeups",
                                 G_TYPE_UINT64, &priv->thread_data->n_rx_wakeups);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_rx_batches",
                                 G_TYPE_UINT64, &priv->thread_data->n_rx_batches);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_rx_max_batch",
                                 G_TYPE_UINT64, &priv->thread_data->n_rx_max_batch);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "stream_thread_cpu_us",
                                 G_TYPE_UINT64, &priv->thread_data->stream_thread_cpu_us);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "thread_priority_mode",
                                 G_TYPE_UINT64, &priv->thread_data->thread_priority_mode);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "stream_thread_min_priority",
                                 G_TYPE_UINT64, &priv->thread_data->stream_thread_min_priority);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "rx_mode",
                                 G_TYPE_UINT64, &priv->thread_data->rx_mode);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_rx_truncated",
                                 G_TYPE_UINT64, &priv->thread_data->n_rx_truncated);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_short_packets",
                                 G_TYPE_UINT64, &priv->thread_data->n_short_packets);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "socket_connected",
                                 G_TYPE_UINT64, &priv->thread_data->socket_connected);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_rx_foreign_source",
                                 G_TYPE_UINT64, &priv->thread_data->n_rx_foreign_source);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_socket_reconnects",
                                 G_TYPE_UINT64, &priv->thread_data->n_socket_reconnects);
        arv_stream_declare_info (ARV_STREAM (gv_stream), "n_resend_send_errors",
                                 G_TYPE_UINT64, &priv->thread_data->n_resend_send_errors);
}

static void
arv_gv_stream_finalize (GObject *object)
{
	ArvGvStreamPrivate *priv = arv_gv_stream_get_instance_private (ARV_GV_STREAM (object));
        GError *error = NULL;

        if (priv->thread != NULL)
                arv_gv_stream_stop_acquisition (ARV_STREAM (object), NULL);

        /* Stop the stream channel. We use a raw register write here, as the Genicam based access rely on
         * ArvGevStreamSelector state, and we don't want to change it here. */
        arv_device_write_register(ARV_DEVICE(priv->gv_device), 0xd00 + 0x40 * priv->stream_channel, 0x0000, &error);

        if (error != NULL) {
                arv_warning_stream ("Failed to stop stream channel %d (%s)", priv->stream_channel, error->message);
                g_clear_error(&error);
        }

	if (priv->thread_data != NULL) {
		ArvGvStreamThreadData *thread_data;
		char *histogram_string;

		thread_data = priv->thread_data;

		histogram_string = arv_histogram_to_string (thread_data->histogram);
		arv_info_stream ("%s", histogram_string);
		g_free (histogram_string);
		arv_histogram_unref (thread_data->histogram);

		arv_info_stream ("[GvStream::finalize] n_completed_buffers    = %" G_GUINT64_FORMAT,
				  thread_data->n_completed_buffers);
		arv_info_stream ("[GvStream::finalize] n_failures             = %" G_GUINT64_FORMAT,
				  thread_data->n_failures);
		arv_info_stream ("[GvStream::finalize] n_underruns            = %" G_GUINT64_FORMAT,
				  thread_data->n_underruns);
		arv_info_stream ("[GvStream::finalize] n_timeouts             = %" G_GUINT64_FORMAT,
				  thread_data->n_timeouts);
		arv_info_stream ("[GvStream::finalize] n_aborted              = %" G_GUINT64_FORMAT,
				  thread_data->n_aborted);
		arv_info_stream ("[GvStream::finalize] n_missing_frames       = %" G_GUINT64_FORMAT,
				  thread_data->n_missing_frames);

		arv_info_stream ("[GvStream::finalize] n_size_mismatch_errors = %" G_GUINT64_FORMAT,
				  thread_data->n_size_mismatch_errors);

		arv_info_stream ("[GvStream::finalize] n_received_packets     = %" G_GUINT64_FORMAT,
				  thread_data->n_received_packets);
		arv_info_stream ("[GvStream::finalize] n_missing_packets      = %" G_GUINT64_FORMAT,
				  thread_data->n_missing_packets);
		arv_info_stream ("[GvStream::finalize] n_error_packets        = %" G_GUINT64_FORMAT,
				  thread_data->n_error_packets);
		arv_info_stream ("[GvStream::finalize] n_ignored_packets      = %" G_GUINT64_FORMAT,
				  thread_data->n_ignored_packets);

		arv_info_stream ("[GvStream::finalize] n_resend_requests      = %" G_GUINT64_FORMAT,
				  thread_data->n_resend_requests);
		arv_info_stream ("[GvStream::finalize] n_resent_packets       = %" G_GUINT64_FORMAT,
				  thread_data->n_resent_packets);
		arv_info_stream ("[GvStream::finalize] n_resend_ratio_reached = %" G_GUINT64_FORMAT,
				  thread_data->n_resend_ratio_reached);
		arv_info_stream ("[GvStream::finalize] n_resend_disabled      = %" G_GUINT64_FORMAT,
				  thread_data->n_resend_disabled);
		arv_info_stream ("[GvStream::finalize] n_duplicated_packets   = %" G_GUINT64_FORMAT,
				  thread_data->n_duplicated_packets);

		arv_info_stream ("[GvStream::finalize] n_transferred_bytes    = %" G_GUINT64_FORMAT,
				  thread_data->n_transferred_bytes);
		arv_info_stream ("[GvStream::finalize] n_ignored_bytes        = %" G_GUINT64_FORMAT,
				  thread_data->n_ignored_bytes);

		g_clear_object (&thread_data->device_address);
		g_clear_object (&thread_data->interface_address);
		g_clear_object (&thread_data->device_socket_address);
		g_clear_object (&thread_data->interface_socket_address);
		arv_info_stream ("[GvStream::finalize] socket_connected       = %" G_GUINT64_FORMAT,
				  thread_data->socket_connected);
		arv_info_stream ("[GvStream::finalize] n_rx_foreign_source    = %" G_GUINT64_FORMAT,
				  thread_data->n_rx_foreign_source);
		arv_info_stream ("[GvStream::finalize] n_socket_reconnects    = %" G_GUINT64_FORMAT,
				  thread_data->n_socket_reconnects);
		arv_info_stream ("[GvStream::finalize] n_resend_send_errors   = %" G_GUINT64_FORMAT,
				  thread_data->n_resend_send_errors);

		g_clear_object (&thread_data->socket);
		g_clear_object (&thread_data->resend_socket);

		g_clear_pointer (&thread_data, g_free);
	}

	g_clear_object (&priv->gv_device);

	G_OBJECT_CLASS (arv_gv_stream_parent_class)->finalize (object);
}

static void
arv_gv_stream_class_init (ArvGvStreamClass *gv_stream_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (gv_stream_class);
	ArvStreamClass *stream_class = ARV_STREAM_CLASS (gv_stream_class);

	object_class->constructed = arv_gv_stream_constructed;
	object_class->finalize = arv_gv_stream_finalize;
	object_class->set_property = arv_gv_stream_set_property;
	object_class->get_property = arv_gv_stream_get_property;

	stream_class->start_acquisition = arv_gv_stream_start_acquisition;
	stream_class->stop_acquisition = arv_gv_stream_stop_acquisition;

        /**
         * ArvGvStream:socket-buffer:
         *
         * Incoming socket buffer policy.
         */
	g_object_class_install_property (
		object_class, ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER,
		g_param_spec_enum ("socket-buffer", "Socket buffer",
				   "Socket buffer behaviour",
				   ARV_TYPE_GV_STREAM_SOCKET_BUFFER,
				   ARV_GV_STREAM_SOCKET_BUFFER_FIXED,
				  G_PARAM_CONSTRUCT |  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
		);
        /**
         * ArvGvStream:socket-buffer-size:
         *
         * Size in bytes of the incoming socket buffer. A greater value helps to lower the number of missing packets,
         * as the expense of an increased memory usage. If the value is not strictly positive, this setting is ignored.
         */
	g_object_class_install_property (
		object_class, ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER_SIZE,
		g_param_spec_int ("socket-buffer-size", "Socket buffer size",
				  "Socket buffer size, in bytes",
				  -1, G_MAXINT, 0,
				  G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
		);
        /**
         * ArvGvStream:packet-resend:
         *
         * Packet resend policy. This only applies if the device supports packet resend.
         */
	g_object_class_install_property (
		object_class, ARV_GV_STREAM_PROPERTY_PACKET_RESEND,
		g_param_spec_enum ("packet-resend", "Packet resend",
				   "Packet resend behaviour",
				   ARV_TYPE_GV_STREAM_PACKET_RESEND,
				   ARV_GV_STREAM_PACKET_RESEND_ALWAYS,
				   G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
		);
        /**
         * ArvGvStream:packet-request-ratio:
         *
         * Maximum number of packet resend requests for a given frame, as a percentage of the number of packets per
         * frame.
         */
	g_object_class_install_property (
		object_class, ARV_GV_STREAM_PROPERTY_PACKET_REQUEST_RATIO,
		g_param_spec_double ("packet-request-ratio", "Packet request ratio",
				     "Packet resend request limit as a percentage of frame packet number",
				     0.0, 2.0, ARV_GV_STREAM_PACKET_REQUEST_RATIO_DEFAULT,
				     G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
		);
        /**
         * ArvGvStream:initial-packet-timeout:
         *
         * Delay before asking for a packet resend after the packet was detected missing for the first time. The reason
         * for this delay is, depending on the network topology, stream packets are not always received in increasing id
         * order. As the missing packet detection happens at each received packet, by verifying if each previous packet
         * has been received, we could emit useless packet resend requests if they are not ordered.
         *
         * Since: 0.8.15
         */
	g_object_class_install_property (
		object_class, ARV_GV_STREAM_PROPERTY_INITIAL_PACKET_TIMEOUT,
		g_param_spec_uint ("initial-packet-timeout", "Initial packet timeout",
				   "Initial packet timeout, in µs",
				   0,
				   G_MAXUINT,
				   ARV_GV_STREAM_INITIAL_PACKET_TIMEOUT_US_DEFAULT,
				   G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
		);
        /**
         * ArvGvStream:packet-timeout:
         *
         * Timeout while waiting for a packet after a resend request, before asking again.
         */
	g_object_class_install_property (
		object_class, ARV_GV_STREAM_PROPERTY_PACKET_TIMEOUT,
		g_param_spec_uint ("packet-timeout", "Packet timeout",
				   "Packet timeout, in µs",
				   0,
				   G_MAXUINT,
				   ARV_GV_STREAM_PACKET_TIMEOUT_US_DEFAULT,
				   G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
		);
        /**
         * ArvGvStream:frame-retention:
         *
         * Amount of time Aravis is wating for frame completion after the last packet is received. A greater value will
         * also increase the maximum frame latency in case of missing packets.
         */
	g_object_class_install_property (
		object_class, ARV_GV_STREAM_PROPERTY_FRAME_RETENTION,
		g_param_spec_uint ("frame-retention", "Frame retention",
				   "Packet retention, in µs",
				   0,
				   G_MAXUINT,
				   ARV_GV_STREAM_FRAME_RETENTION_US_DEFAULT,
				   G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
		);
}
