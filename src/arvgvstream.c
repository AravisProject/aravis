/* Aravis - Digital camera library
 *
 * Copyright © 2009-2016 Emmanuel Pacaud
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvgvstream
 * @short_description: GigEVision stream
 */

#include <arvgvstreamprivate.h>
#include <arvgvdeviceprivate.h>
#include <arvstreamprivate.h>
#include <arvbufferprivate.h>
#include <arvconfig.h>
#include <arvgvsp.h>
#include <arvgvcp.h>
#include <arvdebug.h>
#include <arvmisc.h>
#include <arvstr.h>
#include <arvenumtypes.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>

#ifdef ARAVIS_BUILD_PACKET_SOCKET
#include <ifaddrs.h>
#include <netinet/udp.h>
#include <cap-ng.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/filter.h>
#include <sys/types.h>
#include <sys/mman.h>
#endif

#define ARV_GV_STREAM_INCOMING_BUFFER_SIZE	65536

#define ARV_GV_STREAM_POLL_TIMEOUT_US			1000000
#define ARV_GV_STREAM_PACKET_TIMEOUT_US_DEFAULT		40000
#define ARV_GV_STREAM_FRAME_RETENTION_US_DEFAULT	200000

#define ARV_GV_STREAM_DISCARD_LATE_FRAME_THRESHOLD	100

enum {
	ARV_GV_STREAM_PROPERTY_0,
	ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER,
	ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER_SIZE,
	ARV_GV_STREAM_PROPERTY_PACKET_RESEND,
	ARV_GV_STREAM_PROPERTY_PACKET_TIMEOUT,
	ARV_GV_STREAM_PROPERTY_FRAME_RETENTION
} ArvGvStreamProperties;

static GObjectClass *parent_class = NULL;

typedef struct _ArvGvStreamThreadData ArvGvStreamThreadData;

struct _ArvGvStreamPrivate {
	GThread *thread;
	ArvGvStreamThreadData *thread_data;
};

/* Acquisition thread */

typedef struct {
	gboolean received;
	guint64 time_us;
} ArvGvStreamPacketData;

typedef struct {
	ArvBuffer *buffer;
	guint32 frame_id;

	gint32 last_valid_packet;
	guint64 first_packet_time_us;
	guint64 last_packet_time_us;

	gboolean error_packet_received;

	guint n_packets;
	ArvGvStreamPacketData *packet_data;
} ArvGvStreamFrameData;

struct _ArvGvStreamThreadData {
	ArvGvDevice *gv_device;
	ArvStream *stream;

	ArvStreamCallback callback;
	void *user_data;

	GSocket *socket;
	GInetAddress *interface_address;
	GSocketAddress *interface_socket_address;
	GInetAddress *device_address;
	GSocketAddress *device_socket_address;
	guint16 source_stream_port;
	guint16 stream_port;

	ArvGvStreamPacketResend packet_resend;
	guint packet_timeout_us;
	guint frame_retention_us;

	guint64 timestamp_tick_frequency;
	guint data_size;

	gboolean cancel;

	guint16 packet_id;

	GSList *frames;
	gboolean first_packet;
	guint32 last_frame_id;

	gboolean use_packet_socket;

	/* Statistics */

	guint n_completed_buffers;
	guint n_failures;
	guint n_timeouts;
	guint n_underruns;
	guint n_aborteds;
	guint n_missing_frames;

	guint n_size_mismatch_errors;

	guint n_received_packets;
	guint n_missing_packets;
	guint n_error_packets;
	guint n_ignored_packets;
	guint n_resend_requests;
	guint n_resent_packets;
	guint n_duplicated_packets;

	ArvStatistic *statistic;
	guint32 statistic_count;

	ArvGvStreamSocketBuffer socket_buffer_option;
	int socket_buffer_size;
	int current_socket_buffer_size;
};

static void
_send_packet_request (ArvGvStreamThreadData *thread_data,
		      guint32 frame_id,
		      guint32 first_block,
		      guint32 last_block)
{
	ArvGvcpPacket *packet;
	size_t packet_size;

	thread_data->packet_id = arv_gvcp_next_packet_id (thread_data->packet_id);

	packet = arv_gvcp_packet_new_packet_resend_cmd (frame_id, first_block, last_block,
							thread_data->packet_id, &packet_size);

	arv_log_stream_thread ("[GvStream::send_packet_request] frame_id = %u (%d - %d)",
			       frame_id, first_block, last_block);

	arv_gvcp_packet_debug (packet, ARV_DEBUG_LEVEL_LOG);

	g_socket_send_to (thread_data->socket, thread_data->device_socket_address, (const char *) packet, packet_size,
			  NULL, NULL);

	arv_gvcp_packet_free (packet);
}

static void
_update_socket (ArvGvStreamThreadData *thread_data, ArvBuffer *buffer)
{
	int buffer_size, fd;

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
				buffer_size = buffer->priv->size;
			else
				buffer_size = MIN (buffer->priv->size, thread_data->socket_buffer_size);
			break;
	}

	if (buffer_size != thread_data->current_socket_buffer_size) {
		int result;

		result = setsockopt (fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof (buffer_size));
		if (result == 0) {
			thread_data->current_socket_buffer_size = buffer_size;
			arv_debug_stream_thread ("[GvStream::update_socket] Socket buffer size set to %d", buffer_size);
		} else {
			arv_warning_stream_thread ("[GvStream::update_socket] Failed to set socket buffer size to %d (%d)",
						   buffer_size, errno);
		}
	}
}

static void
_process_data_leader (ArvGvStreamThreadData *thread_data,
		      ArvGvStreamFrameData *frame,
		      const ArvGvspPacket *packet,
		      guint32 packet_id)
{
	if (frame->buffer->priv->status != ARV_BUFFER_STATUS_FILLING)
		return;

	if (packet_id != 0) {
		frame->buffer->priv->status = ARV_BUFFER_STATUS_WRONG_PACKET_ID;
		return;
	}

	frame->buffer->priv->gvsp_payload_type = arv_gvsp_packet_get_payload_type (packet);
	frame->buffer->priv->frame_id = arv_gvsp_packet_get_frame_id (packet);

	if (frame->buffer->priv->gvsp_payload_type != ARV_GVSP_PAYLOAD_TYPE_H264) {
		if (G_LIKELY (thread_data->timestamp_tick_frequency != 0))
			frame->buffer->priv->timestamp_ns = arv_gvsp_packet_get_timestamp (packet,
											   thread_data->timestamp_tick_frequency);
		else {
			GTimeVal time;

			g_get_current_time (&time);
			frame->buffer->priv->timestamp_ns = ((guint64) time.tv_sec * 1000000000LL) + ((guint64) time.tv_usec * 1000) ;
		}
	} else
		frame->buffer->priv->timestamp_ns = g_get_real_time () * 1000LL;

	if (frame->buffer->priv->gvsp_payload_type == ARV_GVSP_PAYLOAD_TYPE_IMAGE) {
		frame->buffer->priv->x_offset = arv_gvsp_packet_get_x_offset (packet);
		frame->buffer->priv->y_offset = arv_gvsp_packet_get_y_offset (packet);
		frame->buffer->priv->width = arv_gvsp_packet_get_width (packet);
		frame->buffer->priv->height = arv_gvsp_packet_get_height (packet);
		frame->buffer->priv->pixel_format = arv_gvsp_packet_get_pixel_format (packet);
	}

	if (frame->packet_data[packet_id].time_us > 0) {
		thread_data->n_resent_packets++;
		arv_log_stream_thread ("[GvStream::process_data_leader] Received resent packet %u for frame %u",
				       packet_id, frame->frame_id);
	}
}

static void
_process_data_block (ArvGvStreamThreadData *thread_data,
		     ArvGvStreamFrameData *frame,
		     const ArvGvspPacket *packet,
		     guint32 packet_id,
		     size_t read_count)
{
	size_t block_size;
	ptrdiff_t block_offset;
	ptrdiff_t block_end;

	if (frame->buffer->priv->status != ARV_BUFFER_STATUS_FILLING)
		return;

	if (packet_id > frame->n_packets - 2 || packet_id < 1) {
		arv_gvsp_packet_debug (packet, read_count, ARV_DEBUG_LEVEL_DEBUG);
		frame->buffer->priv->status = ARV_BUFFER_STATUS_WRONG_PACKET_ID;
		return;
	}

	block_size = arv_gvsp_packet_get_data_size (read_count);
	block_offset = (packet_id - 1) * thread_data->data_size;
	block_end = block_size + block_offset;

	if (block_end > frame->buffer->priv->size) {
		arv_debug_stream_thread ("[GvStream::process_data_block] %d unexpected bytes in packet %u "
					 " for frame %u",
					 block_end - frame->buffer->priv->size,
					 packet_id, frame->frame_id);
		thread_data->n_size_mismatch_errors++;

		block_end = frame->buffer->priv->size;
		block_size = block_end - block_offset;
	}

	memcpy (((char *) frame->buffer->priv->data) + block_offset, &packet->data, block_size);

	if (frame->packet_data[packet_id].time_us > 0) {
		thread_data->n_resent_packets++;
		arv_log_stream_thread ("[GvStream::process_data_block] Received resent packet %u for frame %u",
				       packet_id, frame->frame_id);
	}
}

static void
_process_data_trailer (ArvGvStreamThreadData *thread_data,
		       ArvGvStreamFrameData *frame,
		       const ArvGvspPacket *packet,
		       guint32 packet_id)
{
	if (frame->buffer->priv->status != ARV_BUFFER_STATUS_FILLING)
		return;

	if (packet_id != frame->n_packets - 1) {
		frame->buffer->priv->status = ARV_BUFFER_STATUS_WRONG_PACKET_ID;
		return;
	}

	if (frame->packet_data[packet_id].time_us > 0) {
		thread_data->n_resent_packets++;
		arv_log_stream_thread ("[GvStream::process_data_trailer] Received resent packet %u for frame %u",
				       packet_id, frame->frame_id);
	}
}

static ArvGvStreamFrameData *
_find_frame_data (ArvGvStreamThreadData *thread_data,
		  guint32 frame_id,
		  const ArvGvspPacket *packet,
		  guint32 packet_id,
		  size_t read_count,
		  guint64 time_us)
{
	ArvGvStreamFrameData *frame = NULL;
	ArvBuffer *buffer;
	GSList *iter;
	guint n_packets = 0;
	gint16 frame_id_inc;

	for (iter = thread_data->frames; iter != NULL; iter = iter->next) {
		frame = iter->data;
		if (frame->frame_id == frame_id) {
			frame->last_packet_time_us = time_us;
			return frame;
		}
	}

	frame_id_inc = (gint16) frame_id - (gint16) thread_data->last_frame_id;
	/* Frame id 0 is not a valid value */
	if ((gint16) frame_id > 0 && (gint16) thread_data->last_frame_id < 0)
		frame_id_inc--;
	if (frame_id_inc < 1  && frame_id_inc > -ARV_GV_STREAM_DISCARD_LATE_FRAME_THRESHOLD) {
		arv_debug_stream_thread ("[GvStream::find_frame_data] Discard late frame %u (last: %u)",
					 frame_id, thread_data->last_frame_id);
		return NULL;
	}

	buffer = arv_stream_pop_input_buffer (thread_data->stream);
	if (buffer == NULL) {
		thread_data->n_underruns++;

		return NULL;
	}

	frame = g_new0 (ArvGvStreamFrameData, 1);

	frame->error_packet_received = FALSE;

	frame->frame_id = frame_id;
	frame->last_valid_packet = -1;

	frame->buffer = buffer;
	_update_socket (thread_data, frame->buffer);
	frame->buffer->priv->status = ARV_BUFFER_STATUS_FILLING;
	n_packets = (frame->buffer->priv->size + thread_data->data_size - 1) / thread_data->data_size + 2;

	frame->first_packet_time_us = time_us;
	frame->last_packet_time_us = time_us;

	frame->packet_data = g_new0 (ArvGvStreamPacketData, n_packets);
	frame->n_packets = n_packets;

	if (thread_data->callback != NULL &&
	    frame->buffer != NULL)
		thread_data->callback (thread_data->user_data,
				       ARV_STREAM_CALLBACK_TYPE_START_BUFFER,
				       NULL);

	thread_data->last_frame_id = frame_id;

	if (frame_id_inc > 1) {
		thread_data->n_missing_frames++;
		arv_log_stream_thread ("[GvStream::find_frame_data] Missed %d frame(s) before %u",
				       frame_id_inc - 1, frame_id);
	}

	thread_data->frames = g_slist_append (thread_data->frames, frame);

	arv_log_stream_thread ("[GvStream::find_frame_data] Start frame %u", frame_id);

	return frame;
}

static void
_missing_packet_check (ArvGvStreamThreadData *thread_data,
		       ArvGvStreamFrameData *frame,
		       guint32 packet_id,
		       guint64 time_us)
{
	int i;

	if (thread_data->packet_resend == ARV_GV_STREAM_PACKET_RESEND_NEVER ||
	    frame->error_packet_received)
		return;

	if (packet_id < frame->n_packets) {
		int first_missing = -1;

		for (i = frame->last_valid_packet + 1; i <= packet_id; i++) {
			if (!frame->packet_data[i].received &&
			    (frame->packet_data[i].time_us == 0 ||
			     (time_us - frame->packet_data[i].time_us > thread_data->packet_timeout_us))) {
				if (first_missing < 0)
					first_missing = i;
			} else
				if (first_missing >= 0) {
					int j;

					arv_log_stream_thread ("[GvStream::missing_packet_check]"
							       " Resend request at dt = %" G_GINT64_FORMAT ", packet id = %u/%u",
							       time_us - frame->first_packet_time_us,
							       packet_id, frame->n_packets);

					_send_packet_request (thread_data, frame->frame_id,
							      first_missing, i - 1);
					for (j = first_missing; j < i; j++)
						frame->packet_data[j].time_us = time_us;
					thread_data->n_resend_requests += (i - first_missing);

					first_missing = -1;
				}
		}

		if (first_missing >= 0) {
			int j;

			arv_log_stream_thread ("[GvStream::missing_packet_check]"
					       " Resend request at dt = %" G_GINT64_FORMAT", packet id = %u/%u",
					       time_us - frame->first_packet_time_us,
					       packet_id, frame->n_packets);

			_send_packet_request (thread_data, frame->frame_id,
					      first_missing, i - 1);
			for (j = first_missing; j < i; j++)
				frame->packet_data[j].time_us = time_us;
			thread_data->n_resend_requests += (i - first_missing);
		}
	}
}

static void
_close_frame (ArvGvStreamThreadData *thread_data, ArvGvStreamFrameData *frame)
{
	GTimeVal current_time;
	gint64 current_time_us;

	if (frame->buffer->priv->status == ARV_BUFFER_STATUS_SUCCESS)
		thread_data->n_completed_buffers++;
	else
		if (frame->buffer->priv->status != ARV_BUFFER_STATUS_ABORTED)
			thread_data->n_failures++;

	if (frame->buffer->priv->status == ARV_BUFFER_STATUS_TIMEOUT)
		thread_data->n_timeouts++;

	if (frame->buffer->priv->status == ARV_BUFFER_STATUS_ABORTED)
		thread_data->n_aborteds++;

	if (frame->buffer->priv->status != ARV_BUFFER_STATUS_SUCCESS &&
	    frame->buffer->priv->status != ARV_BUFFER_STATUS_ABORTED)
		thread_data->n_missing_packets += (int) frame->n_packets - (frame->last_valid_packet + 1);

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->user_data,
				       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
				       frame->buffer);

	g_get_current_time (&current_time);
	current_time_us = current_time.tv_sec * 1000000 + current_time.tv_usec;
	if (thread_data->statistic_count > 5) {
		arv_statistic_fill (thread_data->statistic, 0,
				    current_time_us - frame->first_packet_time_us,
				    frame->buffer->priv->frame_id);
	} else
		thread_data->statistic_count++;

	arv_stream_push_output_buffer (thread_data->stream, frame->buffer);

	arv_log_stream_thread ("[GvStream::close_frame] Close frame %u", frame->frame_id);

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

		if (can_close_frame &&
		    thread_data->packet_resend == ARV_GV_STREAM_PACKET_RESEND_NEVER &&
		    iter->next != NULL) {
			frame->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
			arv_debug_stream_thread ("[GvStream::check_frame_completion] Incomplete frame %u",
						 frame->frame_id);
			_close_frame (thread_data, frame);
			thread_data->frames = iter->next;
			g_slist_free_1 (iter);
			iter = thread_data->frames;
			continue;
		}

		if (can_close_frame &&
		    frame->last_valid_packet == frame->n_packets - 1) {
			frame->buffer->priv->status = ARV_BUFFER_STATUS_SUCCESS;
			arv_log_stream_thread ("[GvStream::check_frame_completion] Completed frame %u",
					       frame->frame_id);
			_close_frame (thread_data, frame);
			thread_data->frames = iter->next;
			g_slist_free_1 (iter);
			iter = thread_data->frames;
			continue;
		}

		if (can_close_frame &&
		    time_us - frame->last_packet_time_us >= thread_data->frame_retention_us) {
			frame->buffer->priv->status = ARV_BUFFER_STATUS_TIMEOUT;
			arv_debug_stream_thread ("[GvStream::check_frame_completion] Timeout for frame %u "
						 "at dt = %Lu",
						 frame->frame_id,
						 time_us - frame->first_packet_time_us);
#if 0
			if (arv_debug_check (&arv_debug_category_stream_thread, ARV_DEBUG_LEVEL_LOG)) {
				int i;
				arv_log_stream_thread ("frame_id          = %Lu", frame->frame_id);
				arv_log_stream_thread ("last_valid_packet = %d", frame->last_valid_packet);
				for (i = 0; i < frame->n_packets; i++) {
					arv_log_stream_thread ("%d - time = %Lu%s", i,
							       frame->packet_data[i].time_us,
							       frame->packet_data[i].received ? " - OK" : "");
				}
			}
#endif
			_close_frame (thread_data, frame);
			thread_data->frames = iter->next;
			g_slist_free_1 (iter);
			iter = thread_data->frames;
			continue;
		}

		can_close_frame = FALSE;

		if (frame != current_frame &&
		    time_us - frame->last_packet_time_us >= thread_data->packet_timeout_us) {
			_missing_packet_check (thread_data, frame, frame->n_packets - 1, time_us);
			iter = iter->next;
			continue;
		}

		iter = iter->next;
	}
}

static void
_flush_frames (ArvGvStreamThreadData *thread_data)
{
	GSList *iter;
	ArvGvStreamFrameData *frame;

	for (iter = thread_data->frames; iter != NULL; iter = iter->next) {
		frame = iter->data;
		frame->buffer->priv->status = ARV_BUFFER_STATUS_ABORTED;
		_close_frame (thread_data, frame);
	}

	g_slist_free (thread_data->frames);
	thread_data->frames = NULL;
}

static ArvGvStreamFrameData *
_process_packet (ArvGvStreamThreadData *thread_data, const ArvGvspPacket *packet, size_t packet_size, guint64 time_us)

{
	ArvGvStreamFrameData *frame;
	guint32 packet_id;
	guint32 frame_id;
	int i;

	thread_data->n_received_packets++;

	frame_id = arv_gvsp_packet_get_frame_id (packet);
	packet_id = arv_gvsp_packet_get_packet_id (packet);

	if (thread_data->first_packet) {
		thread_data->last_frame_id = frame_id - 1;
		thread_data->first_packet = FALSE;
	}

	frame = _find_frame_data (thread_data, frame_id, packet, packet_id, packet_size, time_us);

	if (frame != NULL) {
		ArvGvspPacketType packet_type = arv_gvsp_packet_get_packet_type (packet);

		if (packet_type != ARV_GVSP_PACKET_TYPE_OK &&
		    packet_type != ARV_GVSP_PACKET_TYPE_RESEND) {
			arv_debug_stream_thread ("[GvStream::process_packet]"
						 " Error packet at dt = %" G_GINT64_FORMAT ", packet id = %u"
						 " frame id = %u",
						 time_us - frame->first_packet_time_us,
						 packet_id, frame->frame_id);
			arv_gvsp_packet_debug (packet, packet_size, ARV_DEBUG_LEVEL_DEBUG);
			frame->error_packet_received = TRUE;

			thread_data->n_error_packets++;
		} else {
			/* Check for duplicated packets */
			if (packet_id < frame->n_packets) {
				if (frame->packet_data[packet_id].received)
					thread_data->n_duplicated_packets++;
				else
					frame->packet_data[packet_id].received = TRUE;
			}

			/* Keep track of last packet of a continuous block starting from packet 0 */
			for (i = frame->last_valid_packet + 1; i < frame->n_packets; i++)
				if (!frame->packet_data[i].received)
					break;
			frame->last_valid_packet = i - 1;

			switch (arv_gvsp_packet_get_content_type (packet)) {
				case ARV_GVSP_CONTENT_TYPE_DATA_LEADER:
					_process_data_leader (thread_data, frame, packet, packet_id);
					break;
				case ARV_GVSP_CONTENT_TYPE_DATA_BLOCK:
					_process_data_block (thread_data, frame, packet, packet_id,
							     packet_size);
					break;
				case ARV_GVSP_CONTENT_TYPE_DATA_TRAILER:
					_process_data_trailer (thread_data, frame, packet, packet_id);
					break;
				default:
					thread_data->n_ignored_packets++;
					break;
			}

			_missing_packet_check (thread_data, frame, packet_id, time_us);
		}
	} else
		thread_data->n_ignored_packets++;

	return frame;
}

static void
_loop (ArvGvStreamThreadData *thread_data)
{
	ArvGvStreamFrameData *frame;
	ArvGvspPacket *packet;
	GPollFD poll_fd;
	GTimeVal current_time;
	guint64 time_us;
	size_t read_count;
	int timeout_ms;
	int n_events;

	arv_debug_stream ("[GvStream::loop] Standard socket method");

	poll_fd.fd = g_socket_get_fd (thread_data->socket);
	poll_fd.events =  G_IO_IN;
	poll_fd.revents = 0;

	packet = g_malloc0 (ARV_GV_STREAM_INCOMING_BUFFER_SIZE);

	do {
		if (thread_data->frames != NULL)
			timeout_ms = thread_data->packet_timeout_us / 1000;
		else
			timeout_ms = ARV_GV_STREAM_POLL_TIMEOUT_US / 1000;

		n_events = g_poll (&poll_fd, 1, timeout_ms);

		g_get_current_time (&current_time);
		time_us = current_time.tv_sec * 1000000 + current_time.tv_usec;

		if (n_events > 0) {
			read_count = g_socket_receive (thread_data->socket, (char *) packet,
						       ARV_GV_STREAM_INCOMING_BUFFER_SIZE, NULL, NULL);

			frame = _process_packet (thread_data, packet, read_count, time_us);
		} else
			frame = NULL;

		_check_frame_completion (thread_data, time_us, frame);
	} while (!thread_data->cancel);

	g_free (packet);

}


#ifdef ARAVIS_BUILD_PACKET_SOCKET

static void
_set_socket_filter (int socket, guint32 source_ip, guint32 source_port, guint32 destination_ip, guint32 destination_port)
{
	struct sock_filter bpf[18] = {
		{ 0x28, 0, 0, 0x0000000c },
		{ 0x15, 15, 0, 0x000086dd },
		{ 0x15, 0, 14, 0x00000800 },
		{ 0x30, 0, 0, 0x00000017 },
		{ 0x15, 0, 12, 0x00000011 },
		{ 0x20, 0, 0, 0x0000001a },
		{ 0x15, 0, 10, source_ip },
		{ 0x28, 0, 0, 0x00000014 },
		{ 0x45, 8, 0, 0x00001fff },
		{ 0xb1, 0, 0, 0x0000000e },
		{ 0x48, 0, 0, 0x0000000e },
		{ 0x15, 0, 5, source_port },
		{ 0x20, 0, 0, 0x0000001e },
		{ 0x15, 0, 3, destination_ip },
		{ 0x48, 0, 0, 0x00000010 },
		{ 0x15, 0, 1, destination_port },
		{ 0x6, 0, 0, 0x00040000 },
		{ 0x6, 0, 0, 0x00000000 }
	};
	struct sock_fprog bpf_prog = {sizeof(bpf) / sizeof(struct sock_filter), bpf};

	arv_debug_stream_thread ("[GvStream::set_socket_filter] source ip = 0x%08x - port = %d - dest ip = 0x%08x - port %d",
				 source_ip, source_port, destination_ip, destination_port);

	if (setsockopt(socket, SOL_SOCKET, SO_ATTACH_FILTER, &bpf_prog, sizeof(bpf_prog)) != 0)
		arv_warning_stream_thread ("[GvStream::set_socket_filter] Failed to attach Beckerley Packet Filter to stream socket");
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
	GPollFD poll_fd;
	char *buffer;
	struct tpacket_req3 req;
	struct sockaddr_ll local_address;
	enum tpacket_versions version;
	int fd;
	unsigned block_id;
	const guint8 *bytes;
	guint32 interface_address;
	guint32 device_address;

	arv_debug_stream ("[GvStream::loop] Packet socket method");

	fd = socket (PF_PACKET, SOCK_RAW, g_htons (ETH_P_ALL));
	if (fd < 0) {
		arv_warning_stream_thread ("[GvStream::loop] Failed to create AF_PACKET socket");
		return;
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

	local_address.sll_family   = AF_PACKET;
	local_address.sll_protocol = g_htons(ETH_P_IP);
	local_address.sll_ifindex  = _interface_index_from_address (interface_address);
	local_address.sll_hatype   = 0;
	local_address.sll_pkttype  = 0;
	local_address.sll_halen    = 0;
	if (bind (fd, (struct sockaddr *) &local_address, sizeof(local_address)) == -1) {
		arv_warning_stream_thread ("[GvStream::loop] Failed to bind packet socket");
		goto bind_error;
	}

	_set_socket_filter (fd, device_address, thread_data->source_stream_port, interface_address, thread_data->stream_port);

	poll_fd.fd = fd;
	poll_fd.events =  G_IO_IN;
	poll_fd.revents = 0;

	block_id = 0;
	do {
		ArvGvStreamBlockDescriptor *descriptor;
		GTimeVal current_time;
		guint64 time_us;

		g_get_current_time (&current_time);
		time_us = current_time.tv_sec * 1000000 + current_time.tv_usec;

		descriptor = (void *) (buffer + block_id * req.tp_block_size);
		if ((descriptor->h1.block_status & TP_STATUS_USER) == 0) {
			_check_frame_completion (thread_data, time_us, NULL);

			g_poll (&poll_fd, 1, 100);
		} else {
			ArvGvStreamFrameData *frame;
			const struct tpacket3_hdr *header;
			unsigned i;

			header = (void *) (((char *) descriptor) + descriptor->h1.offset_to_first_pkt);

			for (i = 0; i < descriptor->h1.num_pkts; i++) {
				const struct iphdr *ip;
				const ArvGvspPacket *packet;
				size_t size;

				ip = (void *) (((char *) header) + header->tp_mac + ETH_HLEN);
				packet = (void *) (((char *) ip) + sizeof (struct iphdr) + sizeof (struct udphdr));
				size = g_ntohs (ip->tot_len) -  sizeof (struct iphdr) - sizeof (struct udphdr);

				frame = _process_packet (thread_data, packet, size, time_us);

				_check_frame_completion (thread_data, time_us, frame);

				header = (void *) (((char *) header) + header->tp_next_offset);
			}

			descriptor->h1.block_status = TP_STATUS_KERNEL;
			block_id = (block_id + 1) % req.tp_block_nr;
		}
	} while (!thread_data->cancel);

bind_error:
	munmap (buffer, req.tp_block_size * req.tp_block_nr);
socket_option_error:
map_error:
	close (fd);
}

#endif /* ARAVIS_BUILD_PACKET_SOCKET */

static void *
arv_gv_stream_thread (void *data)
{
	ArvGvStreamThreadData *thread_data = data;

	thread_data->frames = NULL;
	thread_data->last_frame_id = 0;
	thread_data->first_packet = TRUE;

	arv_debug_stream_thread ("[GvStream::stream_thread] Packet timeout = %g ms",
				 thread_data->packet_timeout_us / 1000.0);
	arv_debug_stream_thread ("[GvStream::stream_thread] Frame retention = %g ms",
				 thread_data->frame_retention_us / 1000.0);

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->user_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

#ifdef ARAVIS_BUILD_PACKET_SOCKET
	if (capng_have_capability(CAPNG_EFFECTIVE, CAP_NET_RAW) && thread_data->use_packet_socket)
		_ring_buffer_loop (thread_data);
	else
#endif
		_loop (thread_data);

	_flush_frames (thread_data);

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->user_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

	return NULL;
}

/* ArvGvStream implementation */

guint16
arv_gv_stream_get_port (ArvGvStream *gv_stream)
{
	g_return_val_if_fail (ARV_IS_GV_STREAM (gv_stream), 0);

	return gv_stream->priv->thread_data->stream_port;
}

/**
 * arv_gv_stream_new: (skip)
 * @gv_device: a #ArvGvDevice
 * @interface_address: inet interface address for gvsp
 * @device_address: inet device address for gvsp
 * @callback: (scope call): processing callback
 * @user_data: (closure): user data for @callback
 *
 * Return value: (transfer full): a new #ArvStream.
 */

ArvStream *
arv_gv_stream_new (ArvGvDevice *gv_device,
		   GInetAddress *interface_address,
		   GInetAddress *device_address,
		   ArvStreamCallback callback, void *user_data)
{
	ArvGvStreamThreadData *thread_data;
	ArvGvStream *gv_stream;
	ArvStream *stream;
	ArvGvStreamOption options;
	guint64 timestamp_tick_frequency;
	const guint8 *address_bytes;
	GInetSocketAddress *local_address;
	guint packet_size;

	g_return_val_if_fail (ARV_IS_GV_DEVICE (gv_device), NULL);
	g_return_val_if_fail (G_IS_INET_ADDRESS (interface_address), NULL);
	g_return_val_if_fail (G_IS_INET_ADDRESS (device_address), NULL);

	timestamp_tick_frequency = arv_gv_device_get_timestamp_tick_frequency (gv_device);
	options = arv_gv_device_get_stream_options (gv_device);

	packet_size = arv_gv_device_get_packet_size (gv_device);
	if (packet_size <= ARV_GVSP_PACKET_PROTOCOL_OVERHEAD) {
		arv_gv_device_set_packet_size (gv_device, ARV_GV_DEVICE_GVSP_PACKET_SIZE_DEFAULT);
		arv_debug_device ("[GvStream::stream_new] Packet size set to default value (%d)",
				  ARV_GV_DEVICE_GVSP_PACKET_SIZE_DEFAULT);
	}

	packet_size = arv_gv_device_get_packet_size (gv_device);
	arv_debug_device ("[GvStream::stream_new] Packet size = %d byte(s)", packet_size);

	g_return_val_if_fail (packet_size > ARV_GVSP_PACKET_PROTOCOL_OVERHEAD, NULL);

	gv_stream = g_object_new (ARV_TYPE_GV_STREAM, NULL);

	stream = ARV_STREAM (gv_stream);

	thread_data = g_new (ArvGvStreamThreadData, 1);
	thread_data->gv_device = g_object_ref (gv_device);
	thread_data->stream = stream;
	thread_data->callback = callback;
	thread_data->user_data = user_data;
	thread_data->packet_resend = ARV_GV_STREAM_PACKET_RESEND_ALWAYS;
	thread_data->packet_timeout_us = ARV_GV_STREAM_PACKET_TIMEOUT_US_DEFAULT;
	thread_data->frame_retention_us = ARV_GV_STREAM_FRAME_RETENTION_US_DEFAULT;
	thread_data->timestamp_tick_frequency = timestamp_tick_frequency;
	thread_data->data_size = packet_size - ARV_GVSP_PACKET_PROTOCOL_OVERHEAD;
	thread_data->use_packet_socket = (options & ARV_GV_STREAM_OPTION_PACKET_SOCKET_DISABLED) == 0;
	thread_data->cancel = FALSE;

	thread_data->packet_id = 65300;
	thread_data->last_frame_id = 0;

	thread_data->n_completed_buffers = 0;
	thread_data->n_failures = 0;
	thread_data->n_underruns = 0;
	thread_data->n_aborteds = 0;
	thread_data->n_timeouts = 0;
	thread_data->n_missing_frames = 0;

	thread_data->n_size_mismatch_errors = 0;

	thread_data->n_received_packets = 0;
	thread_data->n_missing_packets = 0;
	thread_data->n_error_packets = 0;
	thread_data->n_ignored_packets = 0;
	thread_data->n_resent_packets = 0;
	thread_data->n_resend_requests = 0;
	thread_data->n_duplicated_packets = 0;

	thread_data->statistic = arv_statistic_new (1, 5000, 200, 0);
	thread_data->statistic_count = 0;

	arv_statistic_set_name (thread_data->statistic, 0, "Buffer reception time");

	thread_data->socket_buffer_option = ARV_GV_STREAM_SOCKET_BUFFER_FIXED;
	thread_data->socket_buffer_size = 0;
	thread_data->current_socket_buffer_size = 0;

	gv_stream->priv->thread_data = thread_data;

	thread_data->socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
					  G_SOCKET_TYPE_DATAGRAM,
					  G_SOCKET_PROTOCOL_UDP, NULL);
	thread_data->device_address = g_object_ref (device_address);
	thread_data->interface_address = g_object_ref (interface_address);
	thread_data->interface_socket_address = g_inet_socket_address_new (interface_address, 0);
	thread_data->device_socket_address = g_inet_socket_address_new (device_address, ARV_GVCP_PORT);
	g_socket_bind (thread_data->socket, thread_data->interface_socket_address, TRUE, NULL);

	local_address = G_INET_SOCKET_ADDRESS (g_socket_get_local_address (thread_data->socket, NULL));
	thread_data->stream_port = g_inet_socket_address_get_port (local_address);
	g_object_unref (local_address);

	address_bytes = g_inet_address_to_bytes (interface_address);
	arv_device_set_integer_feature_value (ARV_DEVICE (gv_device), "GevSCDA", g_htonl (*((guint32 *) address_bytes)));
	arv_device_set_integer_feature_value (ARV_DEVICE (gv_device), "GevSCPHostPort", thread_data->stream_port);
	thread_data->source_stream_port = arv_device_get_integer_feature_value (ARV_DEVICE (gv_device), "GevSCSP");

	arv_debug_stream ("[GvStream::stream_new] Destination stream port = %d", thread_data->stream_port);
	arv_debug_stream ("[GvStream::stream_new] Source stream port = %d", thread_data->source_stream_port);

	gv_stream->priv->thread = arv_g_thread_new ("arv_gv_stream", arv_gv_stream_thread, gv_stream->priv->thread_data);

	return ARV_STREAM (gv_stream);
}

/* ArvStream implementation */

void
arv_gv_stream_get_statistics (ArvGvStream *gv_stream,
			      guint64 *n_resent_packets,
			      guint64 *n_missing_packets)

{
	ArvGvStreamThreadData *thread_data;

	g_return_if_fail (ARV_IS_GV_STREAM (gv_stream));

	thread_data = gv_stream->priv->thread_data;

	if (n_resent_packets != NULL)
		*n_resent_packets = thread_data->n_resent_packets;
	if (n_missing_packets != NULL)
		*n_missing_packets = thread_data->n_missing_packets;
}

static void
_get_statistics (ArvStream *stream,
		 guint64 *n_completed_buffers,
		 guint64 *n_failures,
		 guint64 *n_underruns)
{
	ArvGvStream *gv_stream = ARV_GV_STREAM (stream);
	ArvGvStreamThreadData *thread_data;

	thread_data = gv_stream->priv->thread_data;

	*n_completed_buffers = thread_data->n_completed_buffers;
	*n_failures = thread_data->n_failures;
	*n_underruns = thread_data->n_underruns;
}

static void
arv_gv_stream_set_property (GObject * object, guint prop_id,
			    const GValue * value, GParamSpec * pspec)
{
	ArvGvStream *gv_stream = ARV_GV_STREAM (object);
	ArvGvStreamThreadData *thread_data;

	thread_data = gv_stream->priv->thread_data;

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
	ArvGvStream *gv_stream = ARV_GV_STREAM (object);
	ArvGvStreamThreadData *thread_data;

	thread_data = gv_stream->priv->thread_data;

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
	gv_stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (gv_stream, ARV_TYPE_GV_STREAM, ArvGvStreamPrivate);
}

static void
arv_gv_stream_finalize (GObject *object)
{
	ArvGvStream *gv_stream = ARV_GV_STREAM (object);

	if (gv_stream->priv->thread != NULL) {
		ArvGvStreamThreadData *thread_data;
		char *statistic_string;

		thread_data = gv_stream->priv->thread_data;

		thread_data->cancel = TRUE;
		g_thread_join (gv_stream->priv->thread);

		statistic_string = arv_statistic_to_string (thread_data->statistic);
		arv_debug_stream (statistic_string);
		g_free (statistic_string);
		arv_statistic_free (thread_data->statistic);

		arv_debug_stream ("[GvStream::finalize] n_completed_buffers    = %u",
				  thread_data->n_completed_buffers);
		arv_debug_stream ("[GvStream::finalize] n_failures             = %u",
				  thread_data->n_failures);
		arv_debug_stream ("[GvStream::finalize] n_timeouts             = %u",
				  thread_data->n_timeouts);
		arv_debug_stream ("[GvStream::finalize] n_aborteds             = %u",
				  thread_data->n_aborteds);
		arv_debug_stream ("[GvStream::finalize] n_underruns            = %u",
				  thread_data->n_underruns);
		arv_debug_stream ("[GvStream::finalize] n_missing_frames       = %u",
				  thread_data->n_missing_frames);

		arv_debug_stream ("[GvStream::finalize] n_size_mismatch_errors = %u",
				  thread_data->n_size_mismatch_errors);

		arv_debug_stream ("[GvStream::finalize] n_received_packets     = %u",
				  thread_data->n_received_packets);
		arv_debug_stream ("[GvStream::finalize] n_missing_packets      = %u",
				  thread_data->n_missing_packets);
		arv_debug_stream ("[GvStream::finalize] n_error_packets        = %u",
				  thread_data->n_error_packets);
		arv_debug_stream ("[GvStream::finalize] n_ignored_packets      = %u",
				  thread_data->n_ignored_packets);

		arv_debug_stream ("[GvStream::finalize] n_resend_requests      = %u",
				  thread_data->n_resend_requests);
		arv_debug_stream ("[GvStream::finalize] n_resent_packets       = %u",
				  thread_data->n_resent_packets);
		arv_debug_stream ("[GvStream::finalize] n_duplicated_packets   = %u",
				  thread_data->n_duplicated_packets);

		g_clear_object (&thread_data->device_address);
		g_clear_object (&thread_data->interface_address);
		g_clear_object (&thread_data->device_socket_address);
		g_clear_object (&thread_data->interface_socket_address);
		g_clear_object (&thread_data->socket);
		g_clear_object (&thread_data->gv_device);

		g_free (thread_data);

		gv_stream->priv->thread_data = NULL;
		gv_stream->priv->thread = NULL;
	}

	parent_class->finalize (object);
}

static void
arv_gv_stream_class_init (ArvGvStreamClass *gv_stream_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (gv_stream_class);
	ArvStreamClass *stream_class = ARV_STREAM_CLASS (gv_stream_class);

	g_type_class_add_private (gv_stream_class, sizeof (ArvGvStreamPrivate));

	parent_class = g_type_class_peek_parent (gv_stream_class);

	object_class->finalize = arv_gv_stream_finalize;
	object_class->set_property = arv_gv_stream_set_property;
	object_class->get_property = arv_gv_stream_get_property;

	stream_class->get_statistics = _get_statistics;

	g_object_class_install_property (
		object_class, ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER,
		g_param_spec_enum ("socket-buffer", "Socket buffer",
				   "Socket buffer behaviour",
				   ARV_TYPE_GV_STREAM_SOCKET_BUFFER,
				   ARV_GV_STREAM_SOCKET_BUFFER_AUTO,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
		);
	g_object_class_install_property (
		object_class, ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER_SIZE,
		g_param_spec_int ("socket-buffer-size", "Socket buffer size",
				  "Socket buffer size, in bytes",
				  -1, G_MAXINT, 0,
				  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
		);
	g_object_class_install_property (
		object_class, ARV_GV_STREAM_PROPERTY_PACKET_RESEND,
		g_param_spec_enum ("packet-resend", "Packet resend",
				   "Packet resend behaviour",
				   ARV_TYPE_GV_STREAM_PACKET_RESEND,
				   ARV_GV_STREAM_PACKET_RESEND_ALWAYS,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
		);
	g_object_class_install_property (
		object_class, ARV_GV_STREAM_PROPERTY_PACKET_TIMEOUT,
		g_param_spec_uint ("packet-timeout", "Packet timeout",
				   "Packet timeout, in µs",
				   1000,
				   10000000,
				   ARV_GV_STREAM_PACKET_TIMEOUT_US_DEFAULT,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
		);
	g_object_class_install_property (
		object_class, ARV_GV_STREAM_PROPERTY_FRAME_RETENTION,
		g_param_spec_uint ("frame-retention", "Frame retention",
				   "Packet retention, in µs",
				   1000,
				   10000000,
				   ARV_GV_STREAM_FRAME_RETENTION_US_DEFAULT,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
		);
}

G_DEFINE_TYPE (ArvGvStream, arv_gv_stream, ARV_TYPE_STREAM)
