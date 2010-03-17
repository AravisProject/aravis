/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
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

#include <arvgvstream.h>
#include <arvbuffer.h>
#include <arvgvsp.h>
#include <arvgvcp.h>
#include <arvdebug.h>
#include <arvtools.h>
#include <string.h>
#include <sys/socket.h>

static GObjectClass *parent_class = NULL;

/* Acquisition thread */

typedef struct {
	GSocket *socket;
	GSocketAddress *device_address;

	gboolean cancel;
	GAsyncQueue *input_queue;
	GAsyncQueue *output_queue;

	guint32 packet_count;

	/* Statistics */

	guint n_completed_frames;
	guint n_missed_frames;
	guint n_size_mismatch_errors;
	guint n_missing_blocks;

	ArvStatistic *statistic;
} ArvGvStreamThreadData;

#if 0
static void
arv_gv_stream_send_packet_request (ArvGvStreamThreadData *thread_data,
				   guint32 frame_id,
				   guint32 first_block,
				   guint32 last_block)
{
	ArvGvcpPacket *packet;
	guint32 packet_size;

	packet = arv_gvcp_packet_new_packet_resend_cmd (frame_id, first_block, last_block,
							thread_data->packet_count++, &packet_size);

	arv_debug (ARV_DEBUG_LEVEL_STANDARD, "[GvStream::send_packet_request] frame_id = %d (%d - %d)",
		   frame_id, first_block, last_block);

	arv_gvcp_packet_debug (packet);

	g_socket_send_to (thread_data->socket, thread_data->device_address, (const char *) packet, packet_size,
			  NULL, NULL);

	arv_gvcp_packet_free (packet);
}
#endif

static void *
arv_gv_stream_thread (void *data)
{
	ArvGvStreamThreadData *thread_data = data;
	GPollFD poll_fd;
	int n_events;
	char packet_buffer[65536];
	ArvGvspPacket *packet;
	ArvBuffer *buffer = NULL;
	guint16 block_id = 0;
	size_t read_count;
	size_t block_size;
	ptrdiff_t offset = 0;
	GTimeVal current_time;
	gint64 current_time_us;
	gint64 last_time_us;

	packet = (ArvGvspPacket *) packet_buffer;

	poll_fd.fd = g_socket_get_fd (thread_data->socket);
	poll_fd.events =  G_IO_IN;
	poll_fd.revents = 0;

	g_get_current_time (&current_time);
	last_time_us = current_time.tv_sec * 1000000 + current_time.tv_usec;

	do {
		n_events = g_poll (&poll_fd, 1, 1000);

		if (n_events > 0) {
			read_count = g_socket_receive (thread_data->socket, packet_buffer, 65536, NULL, NULL);

			switch (arv_gvsp_packet_get_packet_type (packet)) {
				case ARV_GVSP_PACKET_TYPE_DATA_LEADER:
					if (buffer != NULL)
						g_async_queue_push (thread_data->output_queue, buffer);
					buffer = g_async_queue_try_pop (thread_data->input_queue);
					if (buffer == NULL) {
						thread_data->n_missed_frames++;
						break;
					}
					buffer->width = arv_gvsp_packet_get_width (packet);
					buffer->height = arv_gvsp_packet_get_height (packet);
					buffer->frame_id = arv_gvsp_packet_get_frame_id (packet);
					buffer->status = ARV_BUFFER_STATUS_FILLING;
					block_id = 0;
					offset = 0;
					break;

				case ARV_GVSP_PACKET_TYPE_DATA_BLOCK:
					if (buffer == NULL ||
					    buffer->status != ARV_BUFFER_STATUS_FILLING)
						break;
					block_id++;
					if (arv_gvsp_packet_get_block_id (packet) != block_id) {
						arv_gvsp_packet_debug (packet, read_count);
						arv_debug (ARV_DEBUG_LEVEL_STANDARD,
							   "[GvStream::thread] Missing block (expected %d - %d)"
							   " frame %d",
							   block_id,
							   arv_gvsp_packet_get_block_id (packet),
							   arv_gvsp_packet_get_frame_id (packet));
						buffer->status = ARV_BUFFER_STATUS_MISSING_BLOCK;
						block_id =  arv_gvsp_packet_get_block_id (packet);
						thread_data->n_missing_blocks++;
						break;
					}
					block_size = arv_gvsp_packet_get_data_size (read_count);
					if (block_size + offset > buffer->size) {
						arv_gvsp_packet_debug (packet, read_count);
						buffer->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
						thread_data->n_size_mismatch_errors++;
						break;
					}
					memcpy (buffer->data + offset, &packet->data, block_size);
					offset += block_size;

					if (offset == buffer->size)
						buffer->status = ARV_BUFFER_STATUS_SUCCESS;

					break;

				case ARV_GVSP_PACKET_TYPE_DATA_TRAILER:
					if (buffer != NULL) {
						if (buffer->status == ARV_BUFFER_STATUS_FILLING)
							buffer->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
						if (buffer->status == ARV_BUFFER_STATUS_SUCCESS)
							thread_data->n_completed_frames++;

						arv_buffer_run_callback (buffer);

						g_get_current_time (&current_time);
						current_time_us = current_time.tv_sec * 1000000 + current_time.tv_usec;
						arv_statistic_fill (thread_data->statistic,
								    0, (current_time_us - last_time_us) / 1000,
								    buffer->frame_id);
						last_time_us = current_time_us;
						g_async_queue_push (thread_data->output_queue, buffer);
						buffer = NULL;
					}
					break;
			}
		}
	} while (!thread_data->cancel);

	if (buffer != NULL) {
		buffer->status = ARV_BUFFER_STATUS_ABORTED;
		g_async_queue_push (thread_data->output_queue, buffer);
	}

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
arv_gv_stream_new (GInetAddress *device_address, guint16 port)
{
	ArvGvStream *gv_stream;
	ArvStream *stream;
	GInetAddress *incoming_inet_address;
	ArvGvStreamThreadData *thread_data;

	g_return_val_if_fail (G_IS_INET_ADDRESS (device_address), NULL);

	gv_stream = g_object_new (ARV_TYPE_GV_STREAM, NULL);

	stream = ARV_STREAM (gv_stream);

	gv_stream->socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
					  G_SOCKET_TYPE_DATAGRAM,
					  G_SOCKET_PROTOCOL_UDP, NULL);

	incoming_inet_address = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
	gv_stream->incoming_address = g_inet_socket_address_new (incoming_inet_address, port);
	g_object_unref (incoming_inet_address);

	g_socket_bind (gv_stream->socket, gv_stream->incoming_address, TRUE, NULL);

#if 0
	{
		int rcvbuf, fd;
		size_t rcvbuf_size = sizeof (rcvbuf);

		fd = g_socket_get_fd (gv_stream->socket);
		rcvbuf = 10000000;
		setsockopt (fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof (rcvbuf));
		getsockopt (fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &rcvbuf_size);
		g_message ("RCVBUF = %d", rcvbuf);
	}
#endif

	thread_data = g_new (ArvGvStreamThreadData, 1);
	thread_data->socket = gv_stream->socket;
	thread_data->device_address = g_inet_socket_address_new (device_address, ARV_GVCP_PORT);
	thread_data->cancel = FALSE;
	thread_data->input_queue = stream->input_queue;
	thread_data->output_queue = stream->output_queue;

	thread_data->packet_count = 1;

	thread_data->n_completed_frames = 0;
	thread_data->n_missed_frames = 0;
	thread_data->n_size_mismatch_errors = 0;
	thread_data->n_missing_blocks = 0;

	thread_data->statistic = arv_statistic_new (1, 100, 1, 10);

	gv_stream->thread_data = thread_data;

	gv_stream->thread = g_thread_create (arv_gv_stream_thread, gv_stream->thread_data, TRUE, NULL);

	return ARV_STREAM (gv_stream);
}

/* ArvStream implementation */

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

		arv_debug (ARV_DEBUG_LEVEL_STANDARD,
			   "[GvStream::finalize] n_completed_frames     = %d", thread_data->n_completed_frames);
		arv_debug (ARV_DEBUG_LEVEL_STANDARD,
			   "[GvStream::finalize] n_missed_frames        = %d", thread_data->n_missed_frames);
		arv_debug (ARV_DEBUG_LEVEL_STANDARD,
			   "[GvStream::finalize] n_size_mismatch_errors = %d", thread_data->n_size_mismatch_errors);
		arv_debug (ARV_DEBUG_LEVEL_STANDARD,
			   "[GvStream::finalize] n_missing_blocks       = %d", thread_data->n_missing_blocks);

		thread_data->cancel = TRUE;
		g_thread_join (gv_stream->thread);

		g_object_unref (thread_data->device_address);

		statistic_string = arv_statistic_to_string (thread_data->statistic);
		arv_debug (ARV_DEBUG_LEVEL_STANDARD, statistic_string);
		g_free (statistic_string);

		arv_statistic_free (thread_data->statistic);

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

	parent_class = g_type_class_peek_parent (gv_stream_class);

	object_class->finalize = arv_gv_stream_finalize;
}

G_DEFINE_TYPE (ArvGvStream, arv_gv_stream, ARV_TYPE_STREAM)
