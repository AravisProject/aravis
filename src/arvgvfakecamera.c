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

#include <arvgvfakecamera.h>
#include <arvfakecamera.h>
#include <arvbufferprivate.h>
#include <arvgvcpprivate.h>
#include <arvgvspprivate.h>
#include <arvmisc.h>
#include <arvmiscprivate.h>
#include <arvnetworkprivate.h>
#include <string.h>

/**
 * SECTION: arvgvfakecamera
 * @short_description: GigE Vision Simulator
 *
 * #ArvGvFakeCamera is a class that simulates a real GigEVision camera.
 */

#define ARV_GV_FAKE_CAMERA_BUFFER_SIZE	65536

#define ARV_GV_FAKE_CAMERA_FRAME_RETENTION_DEFAULT	4
#define ARV_GV_FAKE_CAMERA_GVCP_POLL_INTERVAL_DEFAULT	64
/* Maximum time between two GVCP polls while a frame is being sent */
#define ARV_GV_FAKE_CAMERA_GVCP_POLL_PERIOD_US		1000
/* Maximum number of control packets handled per poll */
#define ARV_GV_FAKE_CAMERA_MAX_CONTROL_PACKETS_PER_POLL	32

enum {
	ARV_GV_FAKE_CAMERA_INPUT_SOCKET_GVCP = 0,
	ARV_GV_FAKE_CAMERA_INPUT_SOCKET_GLOBAL_DISCOVERY,
	ARV_GV_FAKE_CAMERA_INPUT_SOCKET_SUBNET_DISCOVERY,
	ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS
};

enum
{
  PROP_0,
  PROP_INTERFACE_NAME,
  PROP_SERIAL_NUMBER,
  PROP_GENICAM_FILENAME,
  PROP_GVSP_LOST_PACKET_RATIO,
  PROP_CM_DOMAIN,
  PROP_PACKET_RESEND,
  PROP_FRAME_RETENTION,
  PROP_STATIC_IMAGE,
  PROP_PACKET_RATE,
  PROP_GVCP_POLL_INTERVAL,
  PROP_DROP_EVERY_NTH_PACKET,
  PROP_DROP_BURST_LENGTH,
  PROP_DROP_BURST_FRAME_INTERVAL,
  PROP_DROP_HEAD_LENGTH,
  PROP_DROP_HEAD_FRAME_INTERVAL,
  PROP_RESEND_LOST_RATIO,
  PROP_FRAME_ID_STEP,
  PROP_SENSOR_WIDTH,
  PROP_SENSOR_HEIGHT,
  PROP_N_SENT_FRAMES,
  PROP_N_SENT_PACKETS,
  PROP_N_SEND_ERRORS,
  PROP_N_SEND_NO_BUFFER_SPACE,
  PROP_N_DROPPED_PACKETS,
  PROP_N_RESEND_REQUESTS,
  PROP_N_RESEND_REQUESTED_PACKETS,
  PROP_N_RESENT_PACKETS,
  PROP_N_RESEND_DROPPED_PACKETS,
  PROP_N_RESEND_UNAVAILABLE,
  PROP_SENDING_TIME_US,
  PROP_STREAMING_TIME_US
};

/* Sender statistics. The sender thread updates its own copy and publishes it at every GVCP poll and at the end of
 * every frame, so that a reader never needs to synchronise with the per packet path. */

typedef struct {
	guint64 n_sent_frames;
	guint64 n_sent_packets;			/* every datagram accepted by the socket, resent and error packets included */
	guint64 n_send_errors;
	guint64 n_send_no_buffer_space;		/* subset of n_send_errors: ENOBUFS / ENOSPC */
	guint64 n_dropped_packets;		/* injected loss on first transmissions */
	guint64 n_resend_requests;		/* PACKET_RESEND commands received */
	guint64 n_resend_requested_packets;	/* blocks covered by those commands */
	guint64 n_resent_packets;
	guint64 n_resend_dropped_packets;	/* resent packets dropped by resend-lost-ratio */
	guint64 n_resend_unavailable;		/* PACKET_UNAVAILABLE error packets sent */
	guint64 sending_time_us;		/* time spent inside frames, from leader to trailer */
	guint64 streaming_time_us;		/* time spent with an open stream */
} ArvGvFakeCameraCounters;

/* One retained frame, for answering resend requests. */

typedef struct {
	ArvBuffer *buffer;	/* image data and geometry; shared by every slot in static image mode */
	guint16 frame_id;
	guint64 timestamp_ns;
	size_t payload;
	size_t data_size;	/* payload bytes per data packet */
	guint32 n_blocks;	/* leader + data packets + trailer */
	gboolean valid;
} ArvGvFakeCameraFrame;

typedef struct {
	char *interface_name;
	char *serial_number;
	char *genicam_filename;

	ArvFakeCamera *camera;

	gboolean is_running;

	GPollFD socket_fds[ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS];
	guint n_socket_fds;

	GSocketAddress *controller_address;
	gint64 controller_time;

	GSocket *input_sockets[ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS];

	GSocket *gvsp_socket;

	GThread *thread;
	gboolean cancel;

	double gvsp_lost_packet_ratio;

	/* Configuration */

	gboolean packet_resend;
	guint frame_retention;
	gboolean static_image;
	guint packet_rate;
	guint gvcp_poll_interval;
	guint drop_every_nth_packet;
	guint drop_burst_length;
	guint drop_burst_frame_interval;
	guint drop_head_length;
	guint drop_head_frame_interval;
	double resend_lost_ratio;
	guint frame_id_step;
	guint sensor_width;
	guint sensor_height;

	/* Sender state, owned by the sender thread */

	GInputVector input_vector;

	GSocketAddress *stream_address;
	gboolean is_gvsp_connected;
	gboolean send_error_reported;

	ArvGvFakeCameraFrame *frames;
	guint n_frames;
	guint current_frame;
	ArvBuffer *static_buffer;
	guint16 static_frame_id;
	gboolean has_static_frame_id;

	guint64 stream_packet_count;	/* first transmissions since stream start */
	guint64 stream_frame_count;	/* frames since stream start */
	gint64 stream_start_us;
	gint64 last_poll_us;

	ArvGvFakeCameraCounters counters;

	GMutex counters_mutex;
	ArvGvFakeCameraCounters published_counters;
} ArvGvFakeCameraPrivate;

struct _ArvGvFakeCamera {
	GObject	object;

	ArvGvFakeCameraPrivate *priv;
};

struct _ArvGvFakeCameraClass {
	GObjectClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvGvFakeCamera, arv_gv_fake_camera, G_TYPE_OBJECT, G_ADD_PRIVATE (ArvGvFakeCamera))

static gboolean
_g_inet_socket_address_is_equal (GInetSocketAddress *a, GInetSocketAddress *b)
{
	if (!G_IS_INET_SOCKET_ADDRESS (a) ||
	    !G_IS_INET_SOCKET_ADDRESS (b))
		return FALSE;

	if (g_inet_socket_address_get_port (a) != g_inet_socket_address_get_port (b))
		return FALSE;

	return g_inet_address_equal (g_inet_socket_address_get_address (a),
				     g_inet_socket_address_get_address (b));
}

static void
_publish_counters (ArvGvFakeCamera *gv_fake_camera)
{
	ArvGvFakeCameraPrivate *priv = gv_fake_camera->priv;

	g_mutex_lock (&priv->counters_mutex);
	priv->published_counters = priv->counters;
	if (priv->stream_address != NULL)
		priv->published_counters.streaming_time_us += g_get_monotonic_time () - priv->stream_start_us;
	g_mutex_unlock (&priv->counters_mutex);
}

/* Send one GVSP datagram made of a header and an optional data block, without copying the data. */

static gboolean
_send_gvsp (ArvGvFakeCamera *gv_fake_camera, const void *header, size_t header_size, const void *data, size_t data_size)
{
	ArvGvFakeCameraPrivate *priv = gv_fake_camera->priv;
	GOutputVector vectors[2];
	GError *error = NULL;
	gssize result;

	vectors[0].buffer = header;
	vectors[0].size = header_size;
	vectors[1].buffer = data;
	vectors[1].size = data_size;

	result = g_socket_send_message (priv->gvsp_socket,
					priv->is_gvsp_connected ? NULL : priv->stream_address,
					vectors, data_size > 0 ? 2 : 1,
					NULL, 0, G_SOCKET_MSG_NONE, NULL, &error);

	if (G_LIKELY (result >= 0)) {
		priv->counters.n_sent_packets++;
		return TRUE;
	}

	priv->counters.n_send_errors++;
	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NO_SPACE))
		priv->counters.n_send_no_buffer_space++;

	if (!priv->send_error_reported) {
		arv_warning_stream_thread ("[GvFakeCamera::send_gvsp] Failed to send packet: %s"
					   " (further send errors of this stream are only counted)",
					   error != NULL ? error->message : "unknown error");
		priv->send_error_reported = TRUE;
	} else
		arv_debug_stream_thread ("[GvFakeCamera::send_gvsp] Failed to send packet: %s",
					 error != NULL ? error->message : "unknown error");

	g_clear_error (&error);

	return FALSE;
}

/* Send block @block_id of @frame: 0 is the leader, n_blocks - 1 the trailer, anything in between a data packet. */

static gboolean
_send_block (ArvGvFakeCamera *gv_fake_camera, ArvGvFakeCameraFrame *frame, guint32 block_id)
{
	ArvBuffer *buffer = frame->buffer;
	guint8 header[64];
	size_t header_size = sizeof (header);

	if (block_id == 0) {
		arv_gvsp_packet_new_image_leader (frame->frame_id, block_id,
						  frame->timestamp_ns,
						  arv_buffer_get_image_pixel_format (buffer),
						  arv_buffer_get_image_width (buffer),
						  arv_buffer_get_image_height (buffer),
						  arv_buffer_get_image_x (buffer),
						  arv_buffer_get_image_y (buffer),
						  0, 0,
						  header, sizeof (header), &header_size);
		return _send_gvsp (gv_fake_camera, header, header_size, NULL, 0);
	} else if (block_id == frame->n_blocks - 1) {
		arv_gvsp_packet_new_data_trailer (frame->frame_id, block_id,
						  arv_buffer_get_image_height (buffer),
						  header, sizeof (header), &header_size);
		return _send_gvsp (gv_fake_camera, header, header_size, NULL, 0);
	} else {
		size_t offset = (size_t) (block_id - 1) * frame->data_size;
		size_t data_size = MIN (frame->data_size, frame->payload - offset);
		const char *data = ((const char *) buffer->priv->data) + offset;

		/* Header only: the data goes in a second vector */
		arv_gvsp_packet_new_payload (frame->frame_id, block_id, 0, (void *) data,
					     header, sizeof (header), &header_size);
		return _send_gvsp (gv_fake_camera, header, header_size, data, data_size);
	}
}

/* What a device sends when a resend request names a block it no longer has: a GVSP packet with the status field set
 * to an error code, for the requested frame and block. ArvGvStream recognises it by the 0x8000 bit of the status
 * and, for PACKET_UNAVAILABLE, stops requesting resends for that frame. */

static void
_send_packet_unavailable (ArvGvFakeCamera *gv_fake_camera, guint16 frame_id, guint32 block_id)
{
	guint8 header[64];
	size_t header_size = sizeof (header);
	ArvGvspPacket *packet;
	char dummy = 0;

	packet = arv_gvsp_packet_new_payload (frame_id, block_id, 0, &dummy, header, sizeof (header), &header_size);
	packet->status = g_htons (ARV_GVSP_PACKET_STATUS_PACKET_UNAVAILABLE);

	if (_send_gvsp (gv_fake_camera, header, header_size, NULL, 0))
		gv_fake_camera->priv->counters.n_resend_unavailable++;
}

static void
_handle_packet_resend (ArvGvFakeCamera *gv_fake_camera, ArvGvcpPacket *packet, size_t size)
{
	ArvGvFakeCameraPrivate *priv = gv_fake_camera->priv;
	ArvGvFakeCameraFrame *frame = NULL;
	gboolean extended_ids;
	guint32 data[5];
	guint64 frame_id;
	guint32 first_block;
	guint32 last_block;
	guint32 block_id;
	guint i;

	extended_ids = (packet->header.packet_flags & ARV_GVCP_CMD_PACKET_FLAGS_EXTENDED_IDS) != 0;

	if (size < sizeof (ArvGvcpHeader) + sizeof (guint32) * (extended_ids ? 5 : 3)) {
		arv_warning_device ("[GvFakeCamera::handle_packet_resend] Truncated packet resend command");
		return;
	}

	memcpy (data, &packet->data, sizeof (guint32) * (extended_ids ? 5 : 3));

	if (extended_ids) {
		guint64 frame_id_be;

		first_block = g_ntohl (data[1]);
		last_block = g_ntohl (data[2]);
		memcpy (&frame_id_be, &data[3], sizeof (frame_id_be));
		frame_id = GUINT64_FROM_BE (frame_id_be);
	} else {
		frame_id = g_ntohl (data[0]) & 0xffff;
		first_block = g_ntohl (data[1]) & ARV_GVSP_PACKET_ID_MASK;
		last_block = g_ntohl (data[2]) & ARV_GVSP_PACKET_ID_MASK;
	}

	priv->counters.n_resend_requests++;
	if (last_block >= first_block)
		priv->counters.n_resend_requested_packets += last_block - first_block + 1;

	arv_debug_stream_thread ("[GvFakeCamera::handle_packet_resend] Resend request for frame %" G_GUINT64_FORMAT
				 " blocks %u to %u", frame_id, first_block, last_block);

	if (priv->stream_address == NULL)
		return;

	for (i = 0; i < priv->n_frames; i++) {
		if (priv->frames[i].valid && priv->frames[i].frame_id == frame_id) {
			frame = &priv->frames[i];
			break;
		}
	}

	if (frame == NULL || first_block > last_block || first_block >= frame->n_blocks) {
		arv_info_stream_thread ("[GvFakeCamera::handle_packet_resend] Frame %" G_GUINT64_FORMAT
					" block %u unavailable", frame_id, first_block);
		_send_packet_unavailable (gv_fake_camera, frame_id, first_block);
		return;
	}

	last_block = MIN (last_block, frame->n_blocks - 1);

	for (block_id = first_block; block_id <= last_block; block_id++) {
		if (priv->resend_lost_ratio > 0.0 && g_random_double () < priv->resend_lost_ratio) {
			priv->counters.n_resend_dropped_packets++;
			arv_debug_stream_thread ("Drop resent GVSP packet frame: %u, block: %u", frame->frame_id, block_id);
			continue;
		}

		if (_send_block (gv_fake_camera, frame, block_id))
			priv->counters.n_resent_packets++;
	}
}

static gboolean
_handle_control_packet (ArvGvFakeCamera *gv_fake_camera, GSocket *socket,
			GSocketAddress *remote_address,
			ArvGvcpPacket *packet, size_t size)
{
	ArvGvcpPacket *ack_packet = NULL;
	size_t ack_packet_size;
	guint32 block_address;
	guint32 block_size;
	guint16 packet_id;
	guint16 packet_type;
	guint32 register_address;
	guint32 register_value;
	gboolean write_access;
	gboolean success = FALSE;

	if (gv_fake_camera->priv->controller_address != NULL) {
		gint64 time;
		guint64 elapsed_ms;

		time = g_get_monotonic_time ();

		elapsed_ms = (time - gv_fake_camera->priv->controller_time) / 1000;

		if (elapsed_ms > arv_fake_camera_get_heartbeat_timeout (gv_fake_camera->priv->camera)) {
			g_object_unref (gv_fake_camera->priv->controller_address);
			gv_fake_camera->priv->controller_address = NULL;
			write_access = TRUE;
			arv_warning_device ("[GvFakeCamera::handle_control_packet] Heartbeat timeout");
			arv_fake_camera_set_control_channel_privilege (gv_fake_camera->priv->camera, 0);
		} else
			write_access = _g_inet_socket_address_is_equal
				(G_INET_SOCKET_ADDRESS (remote_address),
				 G_INET_SOCKET_ADDRESS (gv_fake_camera->priv->controller_address));
	} else
		write_access = TRUE;


	arv_gvcp_packet_debug (packet, ARV_DEBUG_LEVEL_DEBUG);

	packet_id = arv_gvcp_packet_get_packet_id (packet, size);
	packet_type = arv_gvcp_packet_get_packet_type (packet, size);

	if (packet_type != ARV_GVCP_PACKET_TYPE_CMD) {
		arv_warning_device ("[GvFakeCamera::handle_control_packet] Unknown packet type");
		return FALSE;
	}

	switch (g_ntohs (packet->header.command)) {
		case ARV_GVCP_COMMAND_DISCOVERY_CMD:
			ack_packet = arv_gvcp_packet_new_discovery_ack (packet_id, &ack_packet_size);
			arv_info_device ("[GvFakeCamera::handle_control_packet] Discovery command");
			arv_fake_camera_read_memory (gv_fake_camera->priv->camera, 0, ARV_GVBS_DISCOVERY_DATA_SIZE,
						     &ack_packet->data);
			break;
		case ARV_GVCP_COMMAND_READ_MEMORY_CMD:
			arv_gvcp_packet_get_read_memory_cmd_infos (packet, size, &block_address, &block_size);
			arv_info_device ("[GvFakeCamera::handle_control_packet] Read memory command %d (%d)",
					  block_address, block_size);
			ack_packet = arv_gvcp_packet_new_read_memory_ack (block_address, block_size,
									  packet_id, &ack_packet_size);
			arv_fake_camera_read_memory (gv_fake_camera->priv->camera, block_address, block_size,
						     arv_gvcp_packet_get_read_memory_ack_data (ack_packet));
			break;
		case ARV_GVCP_COMMAND_WRITE_MEMORY_CMD:
			arv_gvcp_packet_get_write_memory_cmd_infos (packet, size, &block_address, &block_size);
			if (!write_access) {
				arv_warning_device("[GvFakeCamera::handle_control_packet]"
                                                   " Ignore Write memory command %d (%d) not controller",
					block_address, block_size);
				break;
			}

			arv_info_device ("[GvFakeCamera::handle_control_packet] Write memory command %d (%d)",
					  block_address, block_size);
			arv_fake_camera_write_memory (gv_fake_camera->priv->camera, block_address, block_size,
						      arv_gvcp_packet_get_write_memory_cmd_data (packet));
			ack_packet = arv_gvcp_packet_new_write_memory_ack (block_address, packet_id,
									   &ack_packet_size);
			break;
		case ARV_GVCP_COMMAND_READ_REGISTER_CMD:
			arv_gvcp_packet_get_read_register_cmd_infos (packet, size, &register_address);
			arv_fake_camera_read_register (gv_fake_camera->priv->camera, register_address, &register_value);
			arv_info_device ("[GvFakeCamera::handle_control_packet] Read register command %d -> %d",
					  register_address, register_value);
			ack_packet = arv_gvcp_packet_new_read_register_ack (register_value, packet_id,
									    &ack_packet_size);

			if (register_address == ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_OFFSET)
				gv_fake_camera->priv->controller_time = g_get_monotonic_time ();

			break;
		case ARV_GVCP_COMMAND_WRITE_REGISTER_CMD:
			arv_gvcp_packet_get_write_register_cmd_infos (packet, size, &register_address, &register_value);
			if (!write_access) {
				arv_warning_device("[GvFakeCamera::handle_control_packet]"
                                                   " Ignore Write register command %d (%d) not controller",
					register_address, register_value);
				break;
			}

			arv_fake_camera_write_register (gv_fake_camera->priv->camera, register_address, register_value);
			arv_info_device ("[GvFakeCamera::handle_control_packet] Write register command %d -> %d",
					  register_address, register_value);
			ack_packet = arv_gvcp_packet_new_write_register_ack (1, packet_id,
									     &ack_packet_size);
			break;
		case ARV_GVCP_COMMAND_PACKET_RESEND_CMD:
			if (gv_fake_camera->priv->packet_resend) {
				/* No acknowledge: the answer is the resent GVSP packets themselves */
				_handle_packet_resend (gv_fake_camera, packet, size);
				success = TRUE;
			} else
				arv_warning_device ("[GvFakeCamera::handle_control_packet] Unknown command");
			break;
		default:
			arv_warning_device ("[GvFakeCamera::handle_control_packet] Unknown command");
	}

	if (ack_packet != NULL) {
		g_socket_send_to (socket, remote_address, (char *) ack_packet, ack_packet_size, NULL, NULL);
		arv_gvcp_packet_debug (ack_packet, ARV_DEBUG_LEVEL_DEBUG);
		g_free (ack_packet);

		success = TRUE;
	}

	if (gv_fake_camera->priv->controller_address == NULL &&
	    arv_fake_camera_get_control_channel_privilege (gv_fake_camera->priv->camera) != 0) {
		g_object_ref (remote_address);
		arv_info_device("[GvFakeCamera::handle_control_packet] New controller");
		gv_fake_camera->priv->controller_address = remote_address;
		gv_fake_camera->priv->controller_time = g_get_monotonic_time ();
	}
	else if (gv_fake_camera->priv->controller_address != NULL &&
	    arv_fake_camera_get_control_channel_privilege (gv_fake_camera->priv->camera) == 0) {
		g_object_unref (gv_fake_camera->priv->controller_address);
		arv_info_device("[GvFakeCamera::handle_control_packet] Controller releases");
		gv_fake_camera->priv->controller_address = NULL;
		gv_fake_camera->priv->controller_time = g_get_monotonic_time ();
	}

	return success;
}

/* Wait at most @timeout_ms for control packets and handle them. Returns the number of poll events. */

static int
_poll_control (ArvGvFakeCamera *gv_fake_camera, gint timeout_ms)
{
	ArvGvFakeCameraPrivate *priv = gv_fake_camera->priv;
	int n_events;

	priv->last_poll_us = g_get_monotonic_time ();

	n_events = g_poll (priv->socket_fds, priv->n_socket_fds, timeout_ms);
	if (n_events > 0) {
		unsigned int i;

		for (i = 0; i < ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS; i++) {
			GSocket *socket = priv->input_sockets[i];
			int count;
			int n_packets = 0;

			if (!G_IS_SOCKET (socket))
				continue;

			arv_gpollfd_clear_one (&priv->socket_fds[i], socket);

			/* The input sockets are non blocking: drain what is queued, so that a burst of resend
			 * requests does not wait one poll each. */
			do {
				GSocketAddress *remote_address = NULL;

				count = g_socket_receive_message (socket, &remote_address, &priv->input_vector, 1,
								  NULL, NULL, NULL, NULL, NULL);
				if (count > 0) {
					if (_handle_control_packet (gv_fake_camera, socket,
								    remote_address, priv->input_vector.buffer, count))
						arv_info_device ("[GvFakeCamera::thread] Control packet received");
				}
				g_clear_object (&remote_address);
				n_packets++;
			} while (count > 0 && n_packets < ARV_GV_FAKE_CAMERA_MAX_CONTROL_PACKETS_PER_POLL);
		}
	}

	_publish_counters (gv_fake_camera);

	return n_events;
}

static void
_stream_stop (ArvGvFakeCamera *gv_fake_camera)
{
	ArvGvFakeCameraPrivate *priv = gv_fake_camera->priv;
	guint i;

	if (priv->stream_address == NULL)
		return;

	priv->counters.streaming_time_us += g_get_monotonic_time () - priv->stream_start_us;

	for (i = 0; i < priv->n_frames; i++)
		g_clear_object (&priv->frames[i].buffer);
	g_clear_pointer (&priv->frames, g_free);
	priv->n_frames = 0;
	g_clear_object (&priv->static_buffer);

	g_clear_object (&priv->stream_address);

	_publish_counters (gv_fake_camera);

	arv_info_stream_thread ("[GvFakeCamera::thread] Stop stream");
}

/* Static image mode: the frame id after @frame_id, frame-id-step further in the 16 bit id space, where 0 is not a valid
 * id. A step of 21845 (65535 / 3) makes every third frame reuse an id, which is how a test reaches, in a few frames,
 * what a real stream reaches when its ids wrap every 65535 frames. */

static guint16
_next_static_frame_id (ArvGvFakeCamera *gv_fake_camera, guint16 frame_id)
{
	guint step = MAX (1, (guint) g_atomic_int_get ((gint *) &gv_fake_camera->priv->frame_id_step));

	/* frame_id - 1 + step, modulo 65535, without going below 0 for frame_id 0 */
	return (guint16) (((guint) frame_id + 65534 + step) % 65535 + 1);
}

static void
_stream_start (ArvGvFakeCamera *gv_fake_camera)
{
	ArvGvFakeCameraPrivate *priv = gv_fake_camera->priv;
	GInetAddress *inet_address;
	char *inet_address_string;
	GError *error = NULL;
	size_t payload;
	guint i;

	priv->stream_address = arv_fake_camera_get_stream_address (priv->camera);
	inet_address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (priv->stream_address));
	inet_address_string = g_inet_address_to_string (inet_address);
	arv_info_stream_thread ("[GvFakeCamera::thread] Start stream to %s (%d)",
				inet_address_string,
				g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (priv->stream_address)));
	g_free (inet_address_string);

	/* A connected datagram socket skips the per packet address handling in the kernel, which roughly doubles the
	 * achievable packet rate on macOS loopback. Fall back to unconnected sends if it fails. */
	priv->is_gvsp_connected = g_socket_connect (priv->gvsp_socket, priv->stream_address, NULL, &error);
	if (!priv->is_gvsp_connected) {
		arv_info_stream_thread ("[GvFakeCamera::thread] Failed to connect GVSP socket: %s",
					error != NULL ? error->message : "unknown error");
		g_clear_error (&error);
	}
	priv->send_error_reported = FALSE;

	payload = arv_fake_camera_get_payload (priv->camera);

	priv->n_frames = MAX (1, priv->frame_retention);
	priv->frames = g_new0 (ArvGvFakeCameraFrame, priv->n_frames);
	priv->current_frame = priv->n_frames - 1;

	if (priv->static_image) {
		priv->static_buffer = arv_buffer_new (payload, NULL);
		/* Fill the image once per stream */
		arv_fake_camera_fill_buffer (priv->camera, priv->static_buffer, NULL);
		if (!priv->has_static_frame_id) {
			priv->static_frame_id = priv->static_buffer->priv->frame_id;
			priv->has_static_frame_id = TRUE;
		} else
			priv->static_frame_id = _next_static_frame_id (gv_fake_camera, priv->static_frame_id);
		for (i = 0; i < priv->n_frames; i++)
			priv->frames[i].buffer = g_object_ref (priv->static_buffer);
	} else {
		for (i = 0; i < priv->n_frames; i++)
			priv->frames[i].buffer = arv_buffer_new (payload, NULL);
	}

	priv->stream_packet_count = 0;
	priv->stream_frame_count = 0;
	priv->stream_start_us = g_get_monotonic_time ();
}

/* Prepare the next frame in the retention ring. Returns NULL if no frame can be sent. */

static ArvGvFakeCameraFrame *
_prepare_frame (ArvGvFakeCamera *gv_fake_camera)
{
	ArvGvFakeCameraPrivate *priv = gv_fake_camera->priv;
	ArvGvFakeCameraFrame *frame;
	guint32 gv_packet_size;
	size_t overhead = ARV_GVSP_PACKET_PROTOCOL_OVERHEAD (FALSE);

	priv->current_frame = (priv->current_frame + 1) % priv->n_frames;
	frame = &priv->frames[priv->current_frame];
	frame->valid = FALSE;

	if (priv->static_image) {
		guint32 value = 0;

		if (priv->stream_frame_count > 0)
			priv->static_frame_id = _next_static_frame_id (gv_fake_camera, priv->static_frame_id);

		arv_fake_camera_read_register (priv->camera, ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET, &value);
		gv_packet_size = (value >> ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_POS) &
			ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_MASK;

		frame->frame_id = priv->static_frame_id;
		frame->timestamp_ns = g_get_monotonic_time () * 1000;
	} else {
		arv_fake_camera_fill_buffer (priv->camera, frame->buffer, &gv_packet_size);
		if (arv_buffer_get_status (frame->buffer) != ARV_BUFFER_STATUS_SUCCESS) {
			arv_warning_stream_thread ("[GvFakeCamera::prepare_frame] Failed to fill frame");
			return NULL;
		}
		frame->frame_id = frame->buffer->priv->frame_id;
		frame->timestamp_ns = arv_buffer_get_timestamp (frame->buffer);
	}

	if (gv_packet_size <= overhead) {
		arv_warning_stream_thread ("[GvFakeCamera::prepare_frame] Invalid packet size (%u)", gv_packet_size);
		return NULL;
	}

	frame->payload = frame->buffer->priv->allocated_size;
	frame->data_size = gv_packet_size - overhead;
	frame->n_blocks = (frame->payload + frame->data_size - 1) / frame->data_size + 2;
	frame->valid = TRUE;

	return frame;
}

static gboolean
_should_drop (ArvGvFakeCamera *gv_fake_camera, ArvGvFakeCameraFrame *frame, guint32 block_id)
{
	ArvGvFakeCameraPrivate *priv = gv_fake_camera->priv;
	guint64 frame_number = priv->stream_frame_count + 1;	/* 1 based, within the stream */

	/* Random loss, as before: one draw per packet */
	if (g_random_double () < priv->gvsp_lost_packet_ratio)
		return TRUE;

	if (priv->drop_every_nth_packet > 0 &&
	    (priv->stream_packet_count + 1) % priv->drop_every_nth_packet == 0)
		return TRUE;

	if (priv->drop_burst_length > 0 && priv->drop_burst_frame_interval > 0 &&
	    frame_number % priv->drop_burst_frame_interval == 0) {
		guint32 n_data = frame->n_blocks - 2;
		guint32 length = MIN (priv->drop_burst_length, n_data);
		guint32 first = 1 + (n_data - length) / 2;	/* centred in the payload */

		if (block_id >= first && block_id < first + length)
			return TRUE;
	}

	if (priv->drop_head_frame_interval > 0 &&
	    frame_number % priv->drop_head_frame_interval == 0 &&
	    block_id <= priv->drop_head_length)
		return TRUE;

	return FALSE;
}

static void
_send_frame (ArvGvFakeCamera *gv_fake_camera, ArvGvFakeCameraFrame *frame)
{
	ArvGvFakeCameraPrivate *priv = gv_fake_camera->priv;
	gint64 start_us;
	double packet_period_us;
	guint poll_interval;
	guint n_since_poll = 0;
	guint32 block_id;

	arv_info_stream_thread ("[GvFakeCamera::thread] Send frame %u", frame->frame_id);

	start_us = g_get_monotonic_time ();
	packet_period_us = priv->packet_rate > 0 ? 1e6 / priv->packet_rate : 0.0;
	poll_interval = priv->gvcp_poll_interval;

	for (block_id = 0; block_id < frame->n_blocks; block_id++) {
		if (poll_interval > 0 && block_id > 0 &&
		    (n_since_poll >= poll_interval ||
		     g_get_monotonic_time () - priv->last_poll_us >= ARV_GV_FAKE_CAMERA_GVCP_POLL_PERIOD_US)) {
			/* Serve resend requests (and any other command) while the frame is in flight */
			_poll_control (gv_fake_camera, 0);
			n_since_poll = 0;
		}

		if (packet_period_us > 0.0) {
			gint64 deadline_us = start_us + (gint64) (block_id * packet_period_us);
			gint64 now_us;

			/* Spin rather than sleep: sleeping has a granularity far coarser than a packet period */
			while ((now_us = g_get_monotonic_time ()) < deadline_us) {
				if (poll_interval > 0 && now_us - priv->last_poll_us >= ARV_GV_FAKE_CAMERA_GVCP_POLL_PERIOD_US) {
					_poll_control (gv_fake_camera, 0);
					n_since_poll = 0;
				}
			}
		}

		if (_should_drop (gv_fake_camera, frame, block_id)) {
			priv->counters.n_dropped_packets++;
			arv_debug_stream_thread ("Drop GVSP packet frame: %u, block: %u", frame->frame_id, block_id);
		} else
			_send_block (gv_fake_camera, frame, block_id);

		priv->stream_packet_count++;
		n_since_poll++;
	}

	priv->stream_frame_count++;
	priv->counters.n_sent_frames++;
	priv->counters.sending_time_us += g_get_monotonic_time () - start_us;

	_publish_counters (gv_fake_camera);
}

static void *
_thread (void *user_data)
{
	ArvGvFakeCamera *gv_fake_camera = user_data;
	ArvGvFakeCameraPrivate *priv = gv_fake_camera->priv;
	int n_events;
	gboolean is_streaming = FALSE;

	priv->input_vector.buffer = g_malloc0 (ARV_GV_FAKE_CAMERA_BUFFER_SIZE);
	priv->input_vector.size = ARV_GV_FAKE_CAMERA_BUFFER_SIZE;

	do {
		guint64 next_timestamp_us;

		if (is_streaming) {
			arv_fake_camera_get_sleep_time_for_next_frame (priv->camera, &next_timestamp_us);
		} else {
			next_timestamp_us = g_get_monotonic_time () + 100000;
		}

		do {
			gint timeout_ms;

			/* Signed arithmetic: if next_timestamp_us is already in the past, an
			 * unsigned subtraction underflows and is clamped to a spurious 100 ms
			 * timeout, stalling the frame pacing. */
			timeout_ms = ((gint64) next_timestamp_us - g_get_monotonic_time ()) / 1000LL;
			if (timeout_ms < 0)
				timeout_ms = 0;
			else if (timeout_ms > 100)
				timeout_ms = 100;

			n_events = _poll_control (gv_fake_camera, timeout_ms);
			if (n_events > 0) {
				if (arv_fake_camera_get_control_channel_privilege (priv->camera) == 0 ||
				    arv_fake_camera_get_acquisition_status (priv->camera) == 0) {
					_stream_stop (gv_fake_camera);
					is_streaming = FALSE;
				}
			}
		} while (!g_atomic_int_get (&priv->cancel) && g_get_monotonic_time () < next_timestamp_us);

		if (arv_fake_camera_get_control_channel_privilege (priv->camera) != 0 &&
		    arv_fake_camera_get_acquisition_status (priv->camera) != 0) {
			if (priv->stream_address == NULL)
				_stream_start (gv_fake_camera);

			if (arv_fake_camera_is_in_free_running_mode (priv->camera) ||
			    (arv_fake_camera_is_in_software_trigger_mode (priv->camera) &&
			     arv_fake_camera_check_and_acknowledge_software_trigger (priv->camera))) {
				ArvGvFakeCameraFrame *frame;

				frame = _prepare_frame (gv_fake_camera);
				if (frame != NULL)
					_send_frame (gv_fake_camera, frame);

				is_streaming = TRUE;
			}
		}

	} while (!g_atomic_int_get (&priv->cancel));

	_stream_stop (gv_fake_camera);

	g_clear_pointer (&priv->input_vector.buffer, g_free);

	return NULL;
}

static gboolean
_create_and_bind_input_socket (GSocket **socket_out, const char *socket_name,
			       GInetAddress *inet_address, unsigned int port,
			       gboolean allow_reuse, gboolean blocking)
{
	GSocket *socket;
	GSocketAddress *socket_address;
	GError *error = NULL;
	gboolean success;
	char *address_string;

	address_string = g_inet_address_to_string (inet_address);
	if (port > 0)
		arv_info_device ("%s address = %s:%d",socket_name,  address_string, port);
	else
		arv_info_device ("%s address = %s",socket_name,  address_string);
	g_clear_pointer (&address_string, g_free);

	socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, NULL);
	if (!G_IS_SOCKET (socket)) {
		*socket_out = NULL;
		return FALSE;
	}

	socket_address = arv_socket_bind_with_range (socket, inet_address, port, allow_reuse, &error);
        success = G_IS_INET_SOCKET_ADDRESS (socket_address);

	if (error != NULL) {
		arv_warning_device ("Failed to bind %s socket: %s", socket_name, error->message);
		g_clear_error (&error);
	}

	g_clear_object (&socket_address);

	if (success)
		g_socket_set_blocking (socket, blocking);
	else
		g_clear_object (&socket);

	*socket_out = socket;

	return G_IS_SOCKET (socket);
}

static gboolean
arv_gv_fake_camera_start (ArvGvFakeCamera *gv_fake_camera)
{
	ArvNetworkInterface *iface;
	GSocketAddress *socket_address;
	GInetAddress *inet_address;
	GInetAddress *gvcp_inet_address;
	unsigned int i;
	unsigned int n_socket_fds;

	g_return_val_if_fail (ARV_IS_GV_FAKE_CAMERA (gv_fake_camera), FALSE);

	/* get the interface by name or address */
	iface = arv_network_get_interface_by_address(gv_fake_camera->priv->interface_name);
	if (iface == NULL) iface = arv_network_get_interface_by_name(gv_fake_camera->priv->interface_name);
	#ifdef G_OS_WIN32
		/* fake the default loopback interface in windows, in case not found */
		if(iface == NULL && g_strcmp0 (gv_fake_camera->priv->interface_name,ARV_GV_FAKE_CAMERA_DEFAULT_INTERFACE) == 0)
			iface = arv_network_get_fake_ipv4_loopback();
	#endif
	if (iface == NULL) {
		arv_warning_device ("[GvFakeCamera::start] No network interface with address or name '%s' found.",gv_fake_camera->priv->interface_name);
		return FALSE;
	}

	socket_address = g_socket_address_new_from_native (arv_network_interface_get_addr(iface),
								sizeof (struct sockaddr));
	gvcp_inet_address = g_object_ref (g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (socket_address)));
	arv_fake_camera_set_inet_address (gv_fake_camera->priv->camera, gvcp_inet_address);

	_create_and_bind_input_socket (&gv_fake_camera->priv->gvsp_socket,
								 "GVSP", gvcp_inet_address, 0, FALSE, TRUE);
	_create_and_bind_input_socket
		(&gv_fake_camera->priv->input_sockets[ARV_GV_FAKE_CAMERA_INPUT_SOCKET_GVCP],
		 "GVCP", gvcp_inet_address, ARV_GVCP_PORT, FALSE, FALSE);

	inet_address = g_inet_address_new_from_string ("255.255.255.255");
	if (!g_inet_address_equal (gvcp_inet_address, inet_address))
		_create_and_bind_input_socket
			(&gv_fake_camera->priv->input_sockets[ARV_GV_FAKE_CAMERA_INPUT_SOCKET_GLOBAL_DISCOVERY],
			 "Global discovery", inet_address, ARV_GVCP_PORT, TRUE, FALSE);
	g_clear_object (&inet_address);
	g_clear_object (&socket_address);

	socket_address = g_socket_address_new_from_native (arv_network_interface_get_broadaddr(iface),
								sizeof (struct sockaddr));
	inet_address = g_object_ref (g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (socket_address)));
	if (!g_inet_address_equal (gvcp_inet_address, inet_address))
		_create_and_bind_input_socket
			(&gv_fake_camera->priv->input_sockets[ARV_GV_FAKE_CAMERA_INPUT_SOCKET_SUBNET_DISCOVERY],
			 "Subnet discovery", inet_address, ARV_GVCP_PORT, FALSE, FALSE);
	g_clear_object (&inet_address);
	g_clear_object (&socket_address);

	g_clear_object (&gvcp_inet_address);

	arv_network_interface_free(iface);

	n_socket_fds = 0;
	for (i = 0; i < ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS; i++) {
		GSocket *socket = gv_fake_camera->priv->input_sockets[i];

		if (G_IS_SOCKET (socket)) {
			gv_fake_camera->priv->socket_fds[n_socket_fds].fd = g_socket_get_fd (socket);
			gv_fake_camera->priv->socket_fds[n_socket_fds].events = G_IO_IN;
			gv_fake_camera->priv->socket_fds[n_socket_fds].revents = 0;

			n_socket_fds++;
		}
	}
	arv_info_device ("Listening to %d sockets", n_socket_fds);

	gv_fake_camera->priv->n_socket_fds = n_socket_fds;

	arv_gpollfd_prepare_all (gv_fake_camera->priv->socket_fds, n_socket_fds);

	gv_fake_camera->priv->cancel = FALSE;
	gv_fake_camera->priv->thread = g_thread_new ("arv_fake_gv_fake_camera", _thread, gv_fake_camera);

	return TRUE;
}


static void
arv_gv_fake_camera_stop (ArvGvFakeCamera *gv_fake_camera)
{
	unsigned int i;

	g_return_if_fail (ARV_IS_GV_FAKE_CAMERA (gv_fake_camera));

	if (gv_fake_camera->priv->thread != NULL) {

		g_atomic_int_set (&gv_fake_camera->priv->cancel, TRUE);
		g_thread_join (gv_fake_camera->priv->thread);
		gv_fake_camera->priv->thread = NULL;
	}

	arv_gpollfd_finish_all (gv_fake_camera->priv->socket_fds, gv_fake_camera->priv->n_socket_fds);

	for (i = 0; i < ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS; i++) {
		g_clear_object (&gv_fake_camera->priv->input_sockets[i]);
	}
	g_clear_object (&gv_fake_camera->priv->gvsp_socket);

	g_clear_object (&gv_fake_camera->priv->controller_address);
}

/**
 * arv_gv_fake_camera_new_full:
 * @interface_name: (nullable): listening network interface, by name or IP address, default is 127.0.0.1
 * @serial_number: (nullable): fake device serial number, default is GV01
 * @genicam_filename: (nullable): path to alternative genicam data
 *
 * Returns: a new #ArvGvFakeCamera
 *
 * Since: 0.8.0
 */

ArvGvFakeCamera *
arv_gv_fake_camera_new_full (const char *interface_name, const char *serial_number, const char *genicam_filename)
{
	return g_object_new (ARV_TYPE_GV_FAKE_CAMERA,
			     "interface-name", interface_name != NULL ?
			     interface_name : ARV_GV_FAKE_CAMERA_DEFAULT_INTERFACE,
			     "serial-number", serial_number != NULL ?
			     serial_number : ARV_GV_FAKE_CAMERA_DEFAULT_SERIAL_NUMBER,
			     "genicam-filename", genicam_filename,
			     NULL);
}

/**
 * arv_gv_fake_camera_new:
 * @interface_name: (nullable): listening network interface ('lo' by default)
 * @serial_number: (nullable): fake device serial number ('GV01' by default)
 *
 * Returns: a new #ArvGvFakeCamera
 *
 * Since: 0.8.0
 */

ArvGvFakeCamera *
arv_gv_fake_camera_new (const char *interface_name, const char *serial_number)
{
	return arv_gv_fake_camera_new_full (interface_name, serial_number, NULL);
}

/* Settings that live in the device registers. Properties are set before the camera exists during construction, so
 * this runs again from _constructed. */

static void
_apply_device_settings (ArvGvFakeCamera *gv_fake_camera)
{
	ArvGvFakeCameraPrivate *priv = gv_fake_camera->priv;
	guint32 capabilities = 0;

	if (priv->camera == NULL)
		return;

	arv_fake_camera_read_register (priv->camera, ARV_GVBS_GVCP_CAPABILITY_OFFSET, &capabilities);
	if (priv->packet_resend)
		capabilities |= (guint32) (ARV_GVBS_GVCP_CAPABILITY_PACKET_RESEND);
	else
		capabilities &= ~((guint32) (ARV_GVBS_GVCP_CAPABILITY_PACKET_RESEND));
	arv_fake_camera_write_register (priv->camera, ARV_GVBS_GVCP_CAPABILITY_OFFSET, capabilities);

	if (priv->sensor_width > 0)
		arv_fake_camera_write_register (priv->camera, ARV_FAKE_CAMERA_REGISTER_SENSOR_WIDTH, priv->sensor_width);
	if (priv->sensor_height > 0)
		arv_fake_camera_write_register (priv->camera, ARV_FAKE_CAMERA_REGISTER_SENSOR_HEIGHT, priv->sensor_height);
}

static void
_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ArvGvFakeCamera *gv_fake_camera = ARV_GV_FAKE_CAMERA (object);

	switch (prop_id)
	{
		case PROP_INTERFACE_NAME:
			g_free (gv_fake_camera->priv->interface_name);
			gv_fake_camera->priv->interface_name = g_value_dup_string (value);
			break;
		case PROP_SERIAL_NUMBER:
			g_free (gv_fake_camera->priv->serial_number);
			gv_fake_camera->priv->serial_number = g_value_dup_string (value);
			break;
		case PROP_GENICAM_FILENAME:
			g_free (gv_fake_camera->priv->genicam_filename);
			gv_fake_camera->priv->genicam_filename = g_value_dup_string (value);
			break;
		case PROP_GVSP_LOST_PACKET_RATIO:
			gv_fake_camera->priv->gvsp_lost_packet_ratio = g_value_get_double (value);
			break;
		case PROP_PACKET_RESEND:
			gv_fake_camera->priv->packet_resend = g_value_get_boolean (value);
			_apply_device_settings (gv_fake_camera);
			break;
		case PROP_FRAME_RETENTION:
			gv_fake_camera->priv->frame_retention = g_value_get_uint (value);
			break;
		case PROP_STATIC_IMAGE:
			gv_fake_camera->priv->static_image = g_value_get_boolean (value);
			break;
		case PROP_PACKET_RATE:
			gv_fake_camera->priv->packet_rate = g_value_get_uint (value);
			break;
		case PROP_GVCP_POLL_INTERVAL:
			gv_fake_camera->priv->gvcp_poll_interval = g_value_get_uint (value);
			break;
		case PROP_DROP_EVERY_NTH_PACKET:
			gv_fake_camera->priv->drop_every_nth_packet = g_value_get_uint (value);
			break;
		case PROP_DROP_BURST_LENGTH:
			gv_fake_camera->priv->drop_burst_length = g_value_get_uint (value);
			break;
		case PROP_DROP_BURST_FRAME_INTERVAL:
			gv_fake_camera->priv->drop_burst_frame_interval = g_value_get_uint (value);
			break;
		case PROP_DROP_HEAD_LENGTH:
			gv_fake_camera->priv->drop_head_length = g_value_get_uint (value);
			break;
		case PROP_DROP_HEAD_FRAME_INTERVAL:
			gv_fake_camera->priv->drop_head_frame_interval = g_value_get_uint (value);
			break;
		case PROP_RESEND_LOST_RATIO:
			gv_fake_camera->priv->resend_lost_ratio = g_value_get_double (value);
			break;
		case PROP_FRAME_ID_STEP:
			g_atomic_int_set ((gint *) &gv_fake_camera->priv->frame_id_step, g_value_get_uint (value));
			break;
		case PROP_SENSOR_WIDTH:
			gv_fake_camera->priv->sensor_width = g_value_get_uint (value);
			_apply_device_settings (gv_fake_camera);
			break;
		case PROP_SENSOR_HEIGHT:
			gv_fake_camera->priv->sensor_height = g_value_get_uint (value);
			_apply_device_settings (gv_fake_camera);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	ArvGvFakeCamera *gv_fake_camera = ARV_GV_FAKE_CAMERA (object);
	ArvGvFakeCameraPrivate *priv = gv_fake_camera->priv;
	ArvGvFakeCameraCounters counters;

	g_mutex_lock (&priv->counters_mutex);
	counters = priv->published_counters;
	g_mutex_unlock (&priv->counters_mutex);

	switch (prop_id)
	{
		case PROP_GVSP_LOST_PACKET_RATIO:
			g_value_set_double (value, priv->gvsp_lost_packet_ratio);
			break;
		case PROP_PACKET_RESEND:
			g_value_set_boolean (value, priv->packet_resend);
			break;
		case PROP_FRAME_RETENTION:
			g_value_set_uint (value, priv->frame_retention);
			break;
		case PROP_STATIC_IMAGE:
			g_value_set_boolean (value, priv->static_image);
			break;
		case PROP_PACKET_RATE:
			g_value_set_uint (value, priv->packet_rate);
			break;
		case PROP_GVCP_POLL_INTERVAL:
			g_value_set_uint (value, priv->gvcp_poll_interval);
			break;
		case PROP_DROP_EVERY_NTH_PACKET:
			g_value_set_uint (value, priv->drop_every_nth_packet);
			break;
		case PROP_DROP_BURST_LENGTH:
			g_value_set_uint (value, priv->drop_burst_length);
			break;
		case PROP_DROP_BURST_FRAME_INTERVAL:
			g_value_set_uint (value, priv->drop_burst_frame_interval);
			break;
		case PROP_DROP_HEAD_LENGTH:
			g_value_set_uint (value, priv->drop_head_length);
			break;
		case PROP_DROP_HEAD_FRAME_INTERVAL:
			g_value_set_uint (value, priv->drop_head_frame_interval);
			break;
		case PROP_RESEND_LOST_RATIO:
			g_value_set_double (value, priv->resend_lost_ratio);
			break;
		case PROP_FRAME_ID_STEP:
			g_value_set_uint (value, g_atomic_int_get ((gint *) &priv->frame_id_step));
			break;
		case PROP_SENSOR_WIDTH:
			g_value_set_uint (value, priv->sensor_width);
			break;
		case PROP_SENSOR_HEIGHT:
			g_value_set_uint (value, priv->sensor_height);
			break;
		case PROP_N_SENT_FRAMES:
			g_value_set_uint64 (value, counters.n_sent_frames);
			break;
		case PROP_N_SENT_PACKETS:
			g_value_set_uint64 (value, counters.n_sent_packets);
			break;
		case PROP_N_SEND_ERRORS:
			g_value_set_uint64 (value, counters.n_send_errors);
			break;
		case PROP_N_SEND_NO_BUFFER_SPACE:
			g_value_set_uint64 (value, counters.n_send_no_buffer_space);
			break;
		case PROP_N_DROPPED_PACKETS:
			g_value_set_uint64 (value, counters.n_dropped_packets);
			break;
		case PROP_N_RESEND_REQUESTS:
			g_value_set_uint64 (value, counters.n_resend_requests);
			break;
		case PROP_N_RESEND_REQUESTED_PACKETS:
			g_value_set_uint64 (value, counters.n_resend_requested_packets);
			break;
		case PROP_N_RESENT_PACKETS:
			g_value_set_uint64 (value, counters.n_resent_packets);
			break;
		case PROP_N_RESEND_DROPPED_PACKETS:
			g_value_set_uint64 (value, counters.n_resend_dropped_packets);
			break;
		case PROP_N_RESEND_UNAVAILABLE:
			g_value_set_uint64 (value, counters.n_resend_unavailable);
			break;
		case PROP_SENDING_TIME_US:
			g_value_set_uint64 (value, counters.sending_time_us);
			break;
		case PROP_STREAMING_TIME_US:
			g_value_set_uint64 (value, counters.streaming_time_us);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

/**
 * arv_gv_fake_camera_get_fake_camera:
 * @gv_fake_camera: a #ArvGvFakeCamera
 *
 * Retrieves the underlying #ArvFakeCamera object owned by @gv_fake_camera.
 *
 * Returns: (transfer none): underlying fake camera object.
 *
 * Since: 0.8.0
 */

ArvFakeCamera *
arv_gv_fake_camera_get_fake_camera (ArvGvFakeCamera *gv_fake_camera)
{
	g_return_val_if_fail (ARV_IS_GV_FAKE_CAMERA (gv_fake_camera), NULL);

        return gv_fake_camera->priv->camera;
}

/**
 * arv_gv_fake_camera_is_running:
 * @gv_fake_camera: a #ArvGvFakeCamera
 *
 * Returns: %TRUE if the fake camera is correctly listening on the GVCP port
 *
 * Since: 0.8.0
 */

gboolean
arv_gv_fake_camera_is_running (ArvGvFakeCamera *gv_fake_camera)
{
	g_return_val_if_fail (ARV_IS_GV_FAKE_CAMERA (gv_fake_camera), FALSE);

	return gv_fake_camera->priv->is_running;
}

static void
arv_gv_fake_camera_init (ArvGvFakeCamera *gv_fake_camera)
{
	gv_fake_camera->priv = arv_gv_fake_camera_get_instance_private (gv_fake_camera);

	g_mutex_init (&gv_fake_camera->priv->counters_mutex);
}

static void
_constructed (GObject *gobject)
{
	ArvGvFakeCamera *gv_fake_camera = ARV_GV_FAKE_CAMERA (gobject);

	G_OBJECT_CLASS (arv_gv_fake_camera_parent_class)->constructed (gobject);

	gv_fake_camera->priv->camera = arv_fake_camera_new_full (gv_fake_camera->priv->serial_number, gv_fake_camera->priv->genicam_filename);
	_apply_device_settings (gv_fake_camera);
	gv_fake_camera->priv->is_running = arv_gv_fake_camera_start (gv_fake_camera);
}

static void
_finalize (GObject *object)
{
	ArvGvFakeCamera *gv_fake_camera = ARV_GV_FAKE_CAMERA (object);

	gv_fake_camera->priv->is_running = FALSE;

	arv_gv_fake_camera_stop (gv_fake_camera);

	g_object_unref (gv_fake_camera->priv->camera);

	g_clear_pointer (&gv_fake_camera->priv->interface_name, g_free);
	g_clear_pointer (&gv_fake_camera->priv->serial_number, g_free);
	g_clear_pointer (&gv_fake_camera->priv->genicam_filename, g_free);

	g_mutex_clear (&gv_fake_camera->priv->counters_mutex);

	G_OBJECT_CLASS (arv_gv_fake_camera_parent_class)->finalize (object);
}

static void
arv_gv_fake_camera_class_init (ArvGvFakeCameraClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);

	object_class->set_property = _set_property;
	object_class->get_property = _get_property;
	object_class->constructed = _constructed;
	object_class->finalize = _finalize;

	g_object_class_install_property (object_class,
					 PROP_INTERFACE_NAME,
					 g_param_spec_string ("interface-name",
							      "Interface name",
							      "Interface name",
							      ARV_GV_FAKE_CAMERA_DEFAULT_INTERFACE,
							      G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_SERIAL_NUMBER,
					 g_param_spec_string ("serial-number",
							      "Serial number",
							      "Serial number",
							      ARV_GV_FAKE_CAMERA_DEFAULT_SERIAL_NUMBER,
							      G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_GENICAM_FILENAME,
					 g_param_spec_string ("genicam-filename",
							      "Genicam filename",
							      "Genicam filename",
							      NULL,
							      G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_GVSP_LOST_PACKET_RATIO,
					 g_param_spec_double ("gvsp-lost-ratio",
							      "GVSP lost packet ratio",
							      "GVSP lost packet ratio",
							      0.0, 1.0, 0.0,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_PACKET_RESEND,
					 g_param_spec_boolean ("packet-resend",
							      "Packet resend",
							      "Advertise and serve GVCP PACKET_RESEND commands",
							      FALSE,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_FRAME_RETENTION,
					 g_param_spec_uint ("frame-retention",
							      "Frame retention",
							      "Number of frames kept for resends, the frame being sent included",
							      1, 1024, ARV_GV_FAKE_CAMERA_FRAME_RETENTION_DEFAULT,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_STATIC_IMAGE,
					 g_param_spec_boolean ("static-image",
							      "Static image",
							      "Fill the image once per stream, then only advance frame id and timestamp",
							      FALSE,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_PACKET_RATE,
					 g_param_spec_uint ("packet-rate",
							      "Packet rate",
							      "Packets per second within a frame, 0 for a burst as fast as possible",
							      0, G_MAXUINT, 0,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_GVCP_POLL_INTERVAL,
					 g_param_spec_uint ("gvcp-poll-interval",
							      "GVCP poll interval",
							      "Packets sent between two GVCP polls during a frame, 0 to only poll between frames",
							      0, G_MAXUINT, ARV_GV_FAKE_CAMERA_GVCP_POLL_INTERVAL_DEFAULT,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_DROP_EVERY_NTH_PACKET,
					 g_param_spec_uint ("drop-every-nth-packet",
							      "Drop every Nth packet",
							      "Drop every Nth first transmission of the stream, 0 to disable",
							      0, G_MAXUINT, 0,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_DROP_BURST_LENGTH,
					 g_param_spec_uint ("drop-burst-length",
							      "Drop burst length",
							      "Consecutive data packets dropped in the middle of every drop-burst-frame-interval-th frame",
							      0, G_MAXUINT, 0,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_DROP_BURST_FRAME_INTERVAL,
					 g_param_spec_uint ("drop-burst-frame-interval",
							      "Drop burst frame interval",
							      "Drop a burst every N frames, 0 to disable",
							      0, G_MAXUINT, 0,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_DROP_HEAD_LENGTH,
					 g_param_spec_uint ("drop-head-length",
							      "Drop head length",
							      "Data packets dropped after the leader of every drop-head-frame-interval-th frame",
							      0, G_MAXUINT, 0,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_DROP_HEAD_FRAME_INTERVAL,
					 g_param_spec_uint ("drop-head-frame-interval",
							      "Drop head frame interval",
							      "Drop the leader and drop-head-length packets every N frames, 0 to disable",
							      0, G_MAXUINT, 0,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_RESEND_LOST_RATIO,
					 g_param_spec_double ("resend-lost-ratio",
							      "Resend lost ratio",
							      "Probability of dropping a resent packet",
							      0.0, 1.0, 0.0,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_FRAME_ID_STEP,
					 g_param_spec_uint ("frame-id-step",
							      "Frame id step",
							      "Static image mode: increment between consecutive frame ids, in the"
							      " 16 bit id space without 0 (65535 values); 21845 repeats an id"
							      " every third frame, a fast stand-in for a frame id wrap",
							      1, 65534, 1,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_SENSOR_WIDTH,
					 g_param_spec_uint ("sensor-width",
							      "Sensor width",
							      "Sensor width in pixels, 0 for the default",
							      0, 65535, 0,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_SENSOR_HEIGHT,
					 g_param_spec_uint ("sensor-height",
							      "Sensor height",
							      "Sensor height in pixels, 0 for the default",
							      0, 65535, 0,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_N_SENT_FRAMES,
					 g_param_spec_uint64 ("n-sent-frames",
							      "n-sent-frames",
							      "Frames sent",
							      0, G_MAXUINT64, 0,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_N_SENT_PACKETS,
					 g_param_spec_uint64 ("n-sent-packets",
							      "n-sent-packets",
							      "Datagrams accepted by the socket, resent and error packets included",
							      0, G_MAXUINT64, 0,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_N_SEND_ERRORS,
					 g_param_spec_uint64 ("n-send-errors",
							      "n-send-errors",
							      "Failed sends",
							      0, G_MAXUINT64, 0,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_N_SEND_NO_BUFFER_SPACE,
					 g_param_spec_uint64 ("n-send-no-buffer-space",
							      "n-send-no-buffer-space",
							      "Failed sends for lack of buffer space (ENOBUFS)",
							      0, G_MAXUINT64, 0,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_N_DROPPED_PACKETS,
					 g_param_spec_uint64 ("n-dropped-packets",
							      "n-dropped-packets",
							      "First transmissions dropped by injected loss",
							      0, G_MAXUINT64, 0,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_N_RESEND_REQUESTS,
					 g_param_spec_uint64 ("n-resend-requests",
							      "n-resend-requests",
							      "PACKET_RESEND commands received",
							      0, G_MAXUINT64, 0,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_N_RESEND_REQUESTED_PACKETS,
					 g_param_spec_uint64 ("n-resend-requested-packets",
							      "n-resend-requested-packets",
							      "Blocks covered by PACKET_RESEND commands",
							      0, G_MAXUINT64, 0,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_N_RESENT_PACKETS,
					 g_param_spec_uint64 ("n-resent-packets",
							      "n-resent-packets",
							      "Packets resent",
							      0, G_MAXUINT64, 0,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_N_RESEND_DROPPED_PACKETS,
					 g_param_spec_uint64 ("n-resend-dropped-packets",
							      "n-resend-dropped-packets",
							      "Resent packets dropped by resend-lost-ratio",
							      0, G_MAXUINT64, 0,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_N_RESEND_UNAVAILABLE,
					 g_param_spec_uint64 ("n-resend-unavailable",
							      "n-resend-unavailable",
							      "PACKET_UNAVAILABLE error packets sent",
							      0, G_MAXUINT64, 0,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_SENDING_TIME_US,
					 g_param_spec_uint64 ("sending-time-us",
							      "sending-time-us",
							      "Time spent sending frames, leader to trailer, in microseconds",
							      0, G_MAXUINT64, 0,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_STREAMING_TIME_US,
					 g_param_spec_uint64 ("streaming-time-us",
							      "streaming-time-us",
							      "Time spent with an open stream, in microseconds",
							      0, G_MAXUINT64, 0,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
}
