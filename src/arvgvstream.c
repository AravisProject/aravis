/* Aravis - Digital camera library
 *
 * Copyright © 2009-2011 Emmanuel Pacaud
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvgvstream
 * @short_description: Gigabit ethernet camera stream
 */

#include <arvgvstream.h>
#include <arvbuffer.h>
#include <arvgvsp.h>
#include <arvgvcp.h>
#include <arvdebug.h>
#include <arvtools.h>
#include <arvenumtypes.h>
#include <string.h>
#include <sys/socket.h>

#define ARV_GV_STREAM_INCOMING_BUFFER_SIZE	65536
#define ARV_GV_STREAM_THREAD_N_FRAMES		8
#define ARV_GV_STREAM_THREAD_DISCARD_WINDOW	65536

#define ARV_GV_STREAM_PACKET_TIMEOUT_MS		40
#define ARV_GV_STREAM_FRAME_RETENTION_MS	200

enum {
	ARV_GV_STREAM_PROPERTY_0,
	ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER,
	ARV_GV_STREAM_PROPERTY_SOCKET_BUFFER_SIZE,
	ARV_GV_STREAM_PROPERTY_PACKET_RESEND
} ArvGvStreamProperties;

static GObjectClass *parent_class = NULL;

/* Acquisition thread */

typedef struct {
	gboolean received;
	guint	n_requests;
} ArvGvStreamPacketData;

typedef struct {
	ArvBuffer *buffer;
	guint frame_id;

	gint last_valid_packet;
	guint64 first_packet_timestamp_us;
	guint64 last_packet_timestamp_us;

	guint n_packets;
	ArvGvStreamPacketData *packet_data;
} ArvGvStreamFrameData;

typedef struct {
	ArvStream *stream;

	ArvStreamCallback callback;
	void *user_data;

	GSocket *socket;
	GSocketAddress *device_address;

	ArvGvStreamPacketResend packet_resend;

	guint64 timestamp_tick_frequency;
	guint data_size;

	gboolean cancel;

	guint32 packet_count;

	guint32 last_frame_id;

	/* Statistics */

	guint n_completed_buffers;
	guint n_failures;
	guint n_timeouts;
	guint n_underruns;
	guint n_aborteds;
	guint n_size_mismatch_errors;

	guint n_late_packets;
	guint n_resend_requests;
	guint n_resent_packets;
	guint n_missing_packets;
	guint n_duplicated_packets;

	ArvStatistic *statistic;
	guint32 statistic_count;

	ArvGvStreamSocketBuffer socket_buffer_option;
	int socket_buffer_size;
	int current_socket_buffer_size;
} ArvGvStreamThreadData;

static void
_send_packet_request (ArvGvStreamThreadData *thread_data,
		      guint32 frame_id,
		      guint32 first_block,
		      guint32 last_block)
{
	ArvGvcpPacket *packet;
	size_t packet_size;

	packet = arv_gvcp_packet_new_packet_resend_cmd (frame_id, first_block, last_block,
							thread_data->packet_count++, &packet_size);

	arv_debug ("stream-thread", "[GvStream::send_packet_request] frame_id = %u (%d - %d)",
		   frame_id, first_block, last_block);

	arv_gvcp_packet_debug (packet);

	g_socket_send_to (thread_data->socket, thread_data->device_address, (const char *) packet, packet_size,
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
				buffer_size = buffer->size;
			else
				buffer_size = MIN (buffer->size, thread_data->socket_buffer_size);
			break;
	}

	if (buffer_size != thread_data->current_socket_buffer_size) {
		setsockopt (fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof (buffer_size));
		thread_data->current_socket_buffer_size = buffer_size;
		arv_debug ("stream-thread", "[GvStream::update_socket] Socket buffer size set to %d",
			 buffer_size);
	}
}

static void
_close_buffer (ArvGvStreamThreadData *thread_data, ArvGvStreamFrameData *frame)
{
	GTimeVal current_time;
	gint64 current_time_us;
	int i;
	guint n_missing_packets = 0;

	if (frame->buffer == NULL)
		return;

	if (frame->buffer->status == ARV_BUFFER_STATUS_FILLING) {
		for (i = 0; i < frame->n_packets; i++)
			if (!frame->packet_data[i].received)
				n_missing_packets++;

		thread_data->n_missing_packets += n_missing_packets;

		if (n_missing_packets == 0)
			frame->buffer->status = ARV_BUFFER_STATUS_SUCCESS;
		else
			frame->buffer->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
	}

	if (frame->buffer->status == ARV_BUFFER_STATUS_SUCCESS)
		thread_data->n_completed_buffers++;
	else
		thread_data->n_failures++;

	if (frame->buffer->status == ARV_BUFFER_STATUS_ABORTED)
		thread_data->n_aborteds++;

	if (frame->buffer->status == ARV_BUFFER_STATUS_TIMEOUT) {
		guint32 i;

		thread_data->n_timeouts++;
		for (i = 0; i < frame->n_packets; i++) {
			if (!frame->packet_data[i].received)
				arv_debug ("stream-thread", "[GvStream::_close_buffer] Missing packet %u for frame %u",
					   i, frame->frame_id);
		}
	}

	if (frame->buffer->status == ARV_BUFFER_STATUS_SIZE_MISMATCH)
		thread_data->n_size_mismatch_errors++;

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->user_data,
				       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
				       frame->buffer);

	g_get_current_time (&current_time);
	current_time_us = current_time.tv_sec * 1000000 + current_time.tv_usec;
	if (thread_data->statistic_count > 5) {
		arv_statistic_fill (thread_data->statistic, 1,
				    current_time_us - frame->first_packet_timestamp_us,
				    frame->buffer->frame_id);
	} else
		thread_data->statistic_count++;

	arv_stream_push_output_buffer (thread_data->stream, frame->buffer);

	frame->buffer = NULL;
	frame->frame_id = 0;
}

static void
_process_data_leader (ArvGvStreamThreadData *thread_data,
		      ArvGvStreamFrameData *frame,
		      ArvGvspPacket *packet)
{
	guint32 packet_id;

	if (frame->buffer == NULL)
		return;

	packet_id = arv_gvsp_packet_get_packet_id (packet);
	if (packet_id != 0) {
		frame->buffer->status = ARV_BUFFER_STATUS_WRONG_PACKET_ID;
		_close_buffer (thread_data, frame);
		return;
	}

	frame->buffer->x_offset = arv_gvsp_packet_get_x_offset (packet);
	frame->buffer->y_offset = arv_gvsp_packet_get_y_offset (packet);
	frame->buffer->width = arv_gvsp_packet_get_width (packet);
	frame->buffer->height = arv_gvsp_packet_get_height (packet);
	frame->buffer->pixel_format = arv_gvsp_packet_get_pixel_format (packet);
	frame->buffer->frame_id = arv_gvsp_packet_get_frame_id (packet);

	frame->buffer->timestamp_ns = arv_gvsp_packet_get_timestamp (packet,
								     thread_data->timestamp_tick_frequency);

	if (frame->packet_data[packet_id].n_requests > 0) {
		thread_data->n_resent_packets++;
		arv_debug ("stream-thread", "[GvStream::_process_data_leader] Received resent packet %u for frame %u",
			   packet_id, frame->frame_id);
	}

	if (frame->last_valid_packet == frame->n_packets - 1)
		_close_buffer (thread_data, frame);
}

static void
_process_data_block (ArvGvStreamThreadData *thread_data,
		     ArvGvStreamFrameData *frame,
		     ArvGvspPacket *packet,
		     size_t read_count)
{
	size_t block_size;
	ptrdiff_t block_offset;
	ptrdiff_t block_end;
	guint32 packet_id;

	if (frame->buffer == NULL ||
	    frame->buffer->status != ARV_BUFFER_STATUS_FILLING)
		return;

	packet_id = arv_gvsp_packet_get_packet_id (packet);
	if (packet_id > frame->n_packets - 2) {
		arv_gvsp_packet_debug (packet, read_count);
		frame->buffer->status = ARV_BUFFER_STATUS_WRONG_PACKET_ID;
		_close_buffer (thread_data, frame);
		return;
	}

	block_size = arv_gvsp_packet_get_data_size (read_count);
	block_offset = (packet_id - 1) * thread_data->data_size;
	block_end = block_size + block_offset;

	if (block_end > frame->buffer->size) {
		arv_gvsp_packet_debug (packet, read_count);
		frame->buffer->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
		_close_buffer (thread_data, frame);
		return;
	}

	memcpy (frame->buffer->data + block_offset, &packet->data, block_size);

	if (frame->packet_data[packet_id].n_requests > 0) {
		thread_data->n_resent_packets++;
		arv_debug ("stream-thread", "[GvStream::_process_data_leader] Received resent packet %u for frame %u",
			   packet_id, frame->frame_id);
	}

	if (frame->last_valid_packet == frame->n_packets - 1)
		_close_buffer (thread_data, frame);
}

static void
_process_data_trailer (ArvGvStreamThreadData *thread_data,
		       ArvGvStreamFrameData *frame,
		       ArvGvspPacket *packet)
{
	guint32 packet_id;

	if (frame->buffer == NULL)
		return;

	packet_id = arv_gvsp_packet_get_packet_id (packet);
	if (packet_id != frame->n_packets - 1) {
		frame->buffer->status = ARV_BUFFER_STATUS_WRONG_PACKET_ID;
		_close_buffer (thread_data, frame);
		return;
	}

	if (frame->packet_data[packet_id].n_requests > 0) {
		thread_data->n_resent_packets++;
		arv_debug ("stream-thread", "[GvStream::_process_data_leader] Received resent packet %u for frame %u",
			   packet_id, frame->frame_id);
	}

	if (frame->last_valid_packet == frame->n_packets - 1)
		_close_buffer (thread_data, frame);
}

static gboolean
_update_frame_data (ArvGvStreamThreadData *thread_data, ArvGvStreamFrameData *frame,
		    ArvGvspPacket *packet, size_t read_count, guint64 timestamp_us)
{
	guint frame_id;
	guint n_packets;
	int packet_id;
	int i;

	frame_id = arv_gvsp_packet_get_frame_id (packet);
	packet_id = arv_gvsp_packet_get_packet_id (packet);

	if (frame->buffer != NULL &&
	    frame->frame_id != frame_id) {
		/* discard old frames */
		if (frame_id - thread_data->last_frame_id > - ARV_GV_STREAM_THREAD_DISCARD_WINDOW) {
			arv_debug ("stream", "[GvStream::_update_frame_data] Discard late frame %u"
				   " - packet %u (last frame is %u)",
				   frame_id, packet_id, thread_data->last_frame_id);
			thread_data->n_late_packets++;
			return FALSE;
		}

		frame->buffer->status = ARV_BUFFER_STATUS_TIMEOUT;
		_close_buffer (thread_data, frame);
	}

	if (frame->buffer == NULL) {

		frame->buffer = arv_stream_pop_input_buffer (thread_data->stream);
		if (frame->buffer == NULL) {
			thread_data->n_underruns++;
			return FALSE;
		}

		_update_socket (thread_data, frame->buffer);

		frame->frame_id = frame_id;
		frame->last_valid_packet = -1;
		frame->buffer->status = ARV_BUFFER_STATUS_FILLING;

		thread_data->last_frame_id = frame_id;

		frame->first_packet_timestamp_us = timestamp_us;

		n_packets = (frame->buffer->size + thread_data->data_size - 1) / thread_data->data_size + 2;
		if (frame->n_packets != n_packets) {
			g_free (frame->packet_data);
			frame->packet_data = g_new (ArvGvStreamPacketData, n_packets);
			frame->n_packets = n_packets;

			arv_debug ("stream", "[GvStream::_update_frame_data] n_packets = %d", frame->n_packets);
		}

		memset (frame->packet_data, 0, sizeof (ArvGvStreamPacketData) * frame->n_packets);

		if (thread_data->callback != NULL)
			thread_data->callback (thread_data->user_data,
					       ARV_STREAM_CALLBACK_TYPE_START_BUFFER,
					       NULL);
	}

	frame->last_packet_timestamp_us = timestamp_us;

	if (packet_id < frame->n_packets) {
		if (frame->packet_data[packet_id].received)
			thread_data->n_duplicated_packets++;
		else
			frame->packet_data[packet_id].received = TRUE;
	}

	for (i = frame->last_valid_packet + 1; i < frame->n_packets; i++)
		if (!frame->packet_data[i].received)
			break;
	frame->last_valid_packet = i - 1;

	if (packet_id < frame->n_packets) {
		int first_missing = -1;

		for (i = frame->last_valid_packet + 1; i < packet_id; i++) {
			if (!frame->packet_data[i].received &&
			    frame->packet_data[i].n_requests == 0) {
				if (first_missing < 0)
					first_missing = i;
			} else
				if (first_missing >= 0) {
					int j;

					_send_packet_request (thread_data, frame->frame_id,
							      first_missing, i - 1);
					for (j = first_missing; j < i; j++)
						frame->packet_data[j].n_requests = 1;
					thread_data->n_resend_requests += (i - first_missing);

					first_missing = -1;
				}
		}

		if (first_missing >= 0) {
			int j;

			_send_packet_request (thread_data, frame->frame_id,
					      first_missing, i - 1);
			for (j = first_missing; j < i; j++)
				frame->packet_data[j].n_requests = 1;
			thread_data->n_resend_requests += (i - first_missing);
		}
	}

	return TRUE;
}

static void *
arv_gv_stream_thread (void *data)
{
	ArvGvStreamThreadData *thread_data = data;
	ArvGvStreamFrameData frames[ARV_GV_STREAM_THREAD_N_FRAMES];
	ArvGvStreamFrameData *frame;
	ArvGvspPacket *packet;
	GTimeVal current_time;
	guint64 timestamp_us;
	guint frame_id;
	GPollFD poll_fd;
	size_t read_count;
	int n_events;
	int i;

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->user_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

	poll_fd.fd = g_socket_get_fd (thread_data->socket);
	poll_fd.events =  G_IO_IN;
	poll_fd.revents = 0;

	packet = g_malloc0 (ARV_GV_STREAM_INCOMING_BUFFER_SIZE);

	memset (frames, 0, sizeof (ArvGvStreamFrameData) * ARV_GV_STREAM_THREAD_N_FRAMES);

	do {
		n_events = g_poll (&poll_fd, 1, ARV_GV_STREAM_PACKET_TIMEOUT_MS);

		if (n_events > 0) {
			g_get_current_time (&current_time);
			timestamp_us = current_time.tv_sec * 1000000 + current_time.tv_usec;

			read_count = g_socket_receive (thread_data->socket, (char *) packet,
						       ARV_GV_STREAM_INCOMING_BUFFER_SIZE, NULL, NULL);

			frame_id = arv_gvsp_packet_get_frame_id (packet);
			frame = &frames[frame_id % ARV_GV_STREAM_THREAD_N_FRAMES];

			if (_update_frame_data (thread_data, frame, packet, read_count, timestamp_us))
				switch (arv_gvsp_packet_get_packet_type (packet)) {
					case ARV_GVSP_PACKET_TYPE_DATA_LEADER:
						_process_data_leader (thread_data, frame, packet);
						break;
					case ARV_GVSP_PACKET_TYPE_DATA_BLOCK:
						_process_data_block (thread_data, frame, packet, read_count);
						break;
					case ARV_GVSP_PACKET_TYPE_DATA_TRAILER:
						_process_data_trailer (thread_data, frame, packet);
						break;
				}
		}
	} while (!thread_data->cancel);

	for (i = 0; i < ARV_GV_STREAM_THREAD_N_FRAMES; i++) {
		if (frames[i].buffer != NULL) {
			frames[i].buffer->status = ARV_BUFFER_STATUS_ABORTED;
			_close_buffer (thread_data, &frames[i]);
		}
		g_free (frames[i].packet_data);
	}

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->user_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

	g_free (packet);

	return NULL;
}

/* ArvGvStream implemenation */

guint16
arv_gv_stream_get_port (ArvGvStream *gv_stream)
{
	GInetSocketAddress *local_address;

	g_return_val_if_fail (ARV_IS_GV_STREAM (gv_stream), 0);

	local_address = G_INET_SOCKET_ADDRESS (g_socket_get_local_address (gv_stream->socket, NULL));

	return g_inet_socket_address_get_port (local_address);
}

ArvStream *
arv_gv_stream_new (GInetAddress *device_address, guint16 port,
		   ArvStreamCallback callback, void *user_data,
		   guint64 timestamp_tick_frequency,
		   guint packet_size)
{
	ArvGvStream *gv_stream;
	ArvStream *stream;
	GInetAddress *incoming_inet_address;
	ArvGvStreamThreadData *thread_data;

	g_return_val_if_fail (G_IS_INET_ADDRESS (device_address), NULL);
	g_return_val_if_fail (packet_size > ARV_GVSP_PACKET_PROTOCOL_OVERHEAD, NULL);

	gv_stream = g_object_new (ARV_TYPE_GV_STREAM, NULL);

	stream = ARV_STREAM (gv_stream);

	gv_stream->socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
					  G_SOCKET_TYPE_DATAGRAM,
					  G_SOCKET_PROTOCOL_UDP, NULL);

	incoming_inet_address = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
	gv_stream->incoming_address = g_inet_socket_address_new (incoming_inet_address, port);
	g_object_unref (incoming_inet_address);

	g_socket_bind (gv_stream->socket, gv_stream->incoming_address, TRUE, NULL);

	thread_data = g_new (ArvGvStreamThreadData, 1);
	thread_data->stream = stream;
	thread_data->callback = callback;
	thread_data->user_data = user_data;
	thread_data->socket = gv_stream->socket;
	thread_data->device_address = g_inet_socket_address_new (device_address, ARV_GVCP_PORT);
	thread_data->packet_resend = ARV_GV_STREAM_PACKET_RESEND_ALWAYS;
	thread_data->timestamp_tick_frequency = timestamp_tick_frequency;
	thread_data->data_size = packet_size - ARV_GVSP_PACKET_PROTOCOL_OVERHEAD;
	thread_data->cancel = FALSE;

	thread_data->packet_count = 1;
	thread_data->last_frame_id = 0;

	thread_data->n_completed_buffers = 0;
	thread_data->n_failures = 0;
	thread_data->n_underruns = 0;
	thread_data->n_size_mismatch_errors = 0;
	thread_data->n_missing_packets = 0;
	thread_data->n_late_packets = 0;
	thread_data->n_resent_packets = 0;
	thread_data->n_resend_requests = 0;
	thread_data->n_duplicated_packets = 0;
	thread_data->n_aborteds = 0;
	thread_data->n_timeouts = 0;

	thread_data->statistic = arv_statistic_new (2, 5000, 200, 0);
	thread_data->statistic_count = 0;

	arv_statistic_set_name (thread_data->statistic, 0, "Timestamp delta");
	arv_statistic_set_name (thread_data->statistic, 1, "Buffer reception time");

	thread_data->socket_buffer_option = ARV_GV_STREAM_SOCKET_BUFFER_FIXED;
	thread_data->socket_buffer_size = 0;
	thread_data->current_socket_buffer_size = 0;

	gv_stream->thread_data = thread_data;

	gv_stream->thread = g_thread_create (arv_gv_stream_thread, gv_stream->thread_data, TRUE, NULL);

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

	thread_data = gv_stream->thread_data;

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

	thread_data = gv_stream->thread_data;

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

	thread_data = gv_stream->thread_data;

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

	thread_data = gv_stream->thread_data;

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
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_gv_stream_init (ArvGvStream *gv_stream)
{
}

static void
arv_gv_stream_finalize (GObject *object)
{
	ArvGvStream *gv_stream = ARV_GV_STREAM (object);

	if (gv_stream->thread != NULL) {
		ArvGvStreamThreadData *thread_data;
		char *statistic_string;

		thread_data = gv_stream->thread_data;

		thread_data->cancel = TRUE;
		g_thread_join (gv_stream->thread);

		g_object_unref (thread_data->device_address);

		statistic_string = arv_statistic_to_string (thread_data->statistic);
		arv_debug ("stream", statistic_string);
		g_free (statistic_string);
		arv_statistic_free (thread_data->statistic);

		arv_debug ("stream",
			   "[GvStream::finalize] n_completed_buffers    = %d", thread_data->n_completed_buffers);
		arv_debug ("stream",
			   "[GvStream::finalize] n_failures             = %d", thread_data->n_failures);
		arv_debug ("stream",
			   "[GvStream::finalize] n_timeouts             = %d", thread_data->n_timeouts);
		arv_debug ("stream",
			   "[GvStream::finalize] n_aborteds             = %d", thread_data->n_aborteds);
		arv_debug ("stream",
			   "[GvStream::finalize] n_underruns            = %d", thread_data->n_underruns);
		arv_debug ("stream",
			   "[GvStream::finalize] n_size_mismatch_errors = %d", thread_data->n_size_mismatch_errors);
		arv_debug ("stream",
			   "[GvStream::finalize] n_missing_packets      = %d", thread_data->n_missing_packets);
		arv_debug ("stream",
			   "[GvStream::finalize] n_late_packets         = %d", thread_data->n_late_packets);
		arv_debug ("stream",
			   "[GvStream::finalize] n_resend_requests      = %d", thread_data->n_resend_requests);
		arv_debug ("stream",
			   "[GvStream::finalize] n_resent_packets       = %d", thread_data->n_resent_packets);
		arv_debug ("stream",
			   "[GvStream::finalize] n_duplicated_packets   = %d", thread_data->n_duplicated_packets);

		g_free (thread_data);

		gv_stream->thread_data = NULL;
		gv_stream->thread = NULL;
	}

	g_object_unref (gv_stream->incoming_address);
	g_object_unref (gv_stream->socket);

	parent_class->finalize (object);
}

static void
arv_gv_stream_class_init (ArvGvStreamClass *gv_stream_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (gv_stream_class);
	ArvStreamClass *stream_class = ARV_STREAM_CLASS (gv_stream_class);

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
}

G_DEFINE_TYPE (ArvGvStream, arv_gv_stream, ARV_TYPE_STREAM)
