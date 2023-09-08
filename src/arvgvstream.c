/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
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

#define ARV_GV_STREAM_DISCARD_LATE_FRAME_THRESHOLD	100

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

	guint64 timestamp_tick_frequency;
	guint scps_packet_size;

	guint16 packet_id;

	GSList *frames;
	gboolean first_packet;
	guint64 last_frame_id;

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

	ArvHistogram *histogram;
	guint32 statistic_count;

	ArvGvStreamSocketBuffer socket_buffer_option;
	int socket_buffer_size;
	int current_socket_buffer_size;
};

static void
_send_packet_request (ArvGvStreamThreadData *thread_data,
		      guint64 frame_id,
		      guint32 first_block,
		      guint32 last_block,
		      gboolean extended_ids)
{
	ArvGvcpPacket *packet;
	size_t packet_size;

	thread_data->packet_id = arv_gvcp_next_packet_id (thread_data->packet_id);

	packet = arv_gvcp_packet_new_packet_resend_cmd (frame_id, first_block, last_block, extended_ids,
							thread_data->packet_id, &packet_size);

	arv_debug_stream_thread ("[GvStream::send_packet_request] frame_id = %" G_GUINT64_FORMAT
			       " (from packet %" G_GUINT32_FORMAT " to %" G_GUINT32_FORMAT ")",
			       frame_id, first_block, last_block);

	arv_gvcp_packet_debug (packet, ARV_DEBUG_LEVEL_DEBUG);

	g_socket_send_to (thread_data->socket, thread_data->device_socket_address, (const char *) packet, packet_size,
			  NULL, NULL);

	arv_gvcp_packet_free (packet);
}

static void
_update_socket (ArvGvStreamThreadData *thread_data, ArvBuffer *buffer)
{
	int buffer_size = thread_data->current_socket_buffer_size;
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
				buffer_size = buffer->priv->allocated_size;
			else
				buffer_size = MIN (buffer->priv->allocated_size, thread_data->socket_buffer_size);
			break;
	}

	if (buffer_size != thread_data->current_socket_buffer_size) {
		gboolean result;

		result = arv_socket_set_recv_buffer_size (fd, buffer_size);
		if (result) {
			thread_data->current_socket_buffer_size = buffer_size;
			arv_info_stream_thread ("[GvStream::update_socket] Socket buffer size set to %d", buffer_size);
		} else {
			arv_warning_stream_thread ("[GvStream::update_socket] Failed to set socket buffer size to %d (%d)",
						   buffer_size, errno);
		}
	}
}

static unsigned int
_compute_n_expected_packets (const ArvGvspPacket *packet, size_t allocated_size, size_t packet_size)
{
        ArvGvspContentType content_type;
        ArvBufferPayloadType payload_type;
        gboolean extended_ids;
	guint32 block_size;

	extended_ids = arv_gvsp_packet_has_extended_ids (packet);
        content_type = arv_gvsp_packet_get_content_type (packet);

        switch (content_type) {
                case ARV_GVSP_CONTENT_TYPE_LEADER:
                        payload_type = arv_gvsp_leader_packet_get_buffer_payload_type(packet, NULL);
                        if (payload_type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE ||
                            payload_type == ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA ||
                            payload_type == ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA) {
                                block_size = packet_size - ARV_GVSP_PAYLOAD_PACKET_PROTOCOL_OVERHEAD (extended_ids);
                                return (allocated_size + block_size - 1) / block_size + (2 /* leader + trailer */);
                        } else if (payload_type == ARV_BUFFER_PAYLOAD_TYPE_MULTIPART) {
                                unsigned int n_parts;
                                unsigned int n_packets = 0;
                                unsigned int i;

                                n_parts = arv_gvsp_leader_packet_get_multipart_n_parts(packet);
                                block_size = packet_size - ARV_GVSP_MULTIPART_PACKET_PROTOCOL_OVERHEAD (extended_ids);

                                for (i = 0; i < n_parts; i++) {
                                        n_packets += (arv_gvsp_leader_packet_get_multipart_size (packet, i) +
                                                      block_size - 1) / block_size;
                                }

                                return n_packets + (2 /* leader + trailer */);
                        } else {
                                return 0;
                        }
                        break;
                case ARV_GVSP_CONTENT_TYPE_PAYLOAD:
                        block_size = packet_size - ARV_GVSP_PAYLOAD_PACKET_PROTOCOL_OVERHEAD (extended_ids);
                        return (allocated_size + block_size - 1) / block_size + (2 /* leader + trailer */);
                case ARV_GVSP_CONTENT_TYPE_MULTIPART:
                        block_size = packet_size - ARV_GVSP_MULTIPART_PACKET_PROTOCOL_OVERHEAD (extended_ids);
                        return (allocated_size + block_size - 1) / block_size +
                                (2 /* leader + trailer */) + (255 /* n_parts_max) */);
                        break;
                case ARV_GVSP_CONTENT_TYPE_TRAILER:
                        return arv_gvsp_packet_get_packet_id (packet) + 1;
                        break;
                case ARV_GVSP_CONTENT_TYPE_ALL_IN:
                        return 1;
                case ARV_GVSP_CONTENT_TYPE_H264:
                case ARV_GVSP_CONTENT_TYPE_GENDC:
                case ARV_GVSP_CONTENT_TYPE_MULTIZONE:
                        break;
        }

        return 0;
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

	extended_ids = arv_gvsp_packet_has_extended_ids (packet);

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

	if (frame_id_inc < 1  && frame_id_inc > -ARV_GV_STREAM_DISCARD_LATE_FRAME_THRESHOLD) {
		arv_info_stream_thread ("[GvStream::find_frame_data] Discard late frame %" G_GUINT64_FORMAT
					 " (last: %" G_GUINT64_FORMAT ")",
					 frame_id, thread_data->last_frame_id);
		arv_gvsp_packet_debug (packet, packet_size, ARV_DEBUG_LEVEL_INFO);
		return NULL;
	}

	buffer = arv_stream_pop_input_buffer (thread_data->stream);
	if (buffer == NULL) {
		thread_data->n_underruns++;

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
                                               frame->buffer);
                return NULL;
        }

	frame = g_new0 (ArvGvStreamFrameData, 1);

	frame->disable_resend_request = FALSE;

	frame->frame_id = frame_id;
	frame->last_valid_packet = -1;

	frame->buffer = buffer;
	_update_socket (thread_data, frame->buffer);
	frame->buffer->priv->status = ARV_BUFFER_STATUS_FILLING;

	frame->first_packet_time_us = time_us;
	frame->last_packet_time_us = time_us;

	frame->packet_data = g_new0 (ArvGvStreamPacketData, n_packets);
	frame->n_packets = n_packets;

	if (thread_data->callback != NULL &&
	    frame->buffer != NULL)
		thread_data->callback (thread_data->callback_data,
				       ARV_STREAM_CALLBACK_TYPE_START_BUFFER,
				       NULL);

	thread_data->last_frame_id = frame_id;

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
                (packet, &frame->buffer->priv->has_chunks);
	frame->buffer->priv->frame_id = frame->frame_id;
	frame->buffer->priv->chunk_endianness = G_BIG_ENDIAN;

	frame->buffer->priv->system_timestamp_ns = g_get_real_time() * 1000LL;

        if (frame->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE ||
            frame->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA) {
                guint64 timestamp;

                arv_buffer_set_n_parts (frame->buffer, 1);

                timestamp = arv_gvsp_leader_packet_get_timestamp(packet);

                frame->buffer->priv->parts[0].data_offset = 0;
                frame->buffer->priv->parts[0].component_id = 0;
                frame->buffer->priv->parts[0].data_type = ARV_BUFFER_PART_DATA_TYPE_2D_IMAGE;
                arv_gvsp_leader_packet_get_image_infos (packet,
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
                timestamp = arv_gvsp_leader_packet_get_timestamp(packet);
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

                n_parts = arv_gvsp_leader_packet_get_multipart_n_parts(packet);

                timestamp = arv_gvsp_leader_packet_get_timestamp(packet);

                arv_buffer_set_n_parts (frame->buffer, n_parts);

                for (i = 0; i < n_parts; i++) {
                        frame->buffer->priv->parts[i].data_offset = offset;
                        arv_gvsp_leader_packet_get_multipart_infos (packet, i,
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
		     guint32 packet_id,
		     size_t read_count)
{
	size_t block_size;
	ptrdiff_t block_offset;
	ptrdiff_t block_end;
	gboolean extended_ids;

	if (frame->buffer->priv->status != ARV_BUFFER_STATUS_FILLING)
		return;

	if (packet_id > frame->n_packets - 2 || packet_id < 1) {
		arv_gvsp_packet_debug (packet, read_count, ARV_DEBUG_LEVEL_INFO);
		frame->buffer->priv->status = ARV_BUFFER_STATUS_WRONG_PACKET_ID;
		return;
	}

	extended_ids = arv_gvsp_packet_has_extended_ids (packet);

	block_size = arv_gvsp_payload_packet_get_data_size (packet, read_count);
	block_offset = (packet_id - 1) * (thread_data->scps_packet_size -
                                         ARV_GVSP_PAYLOAD_PACKET_PROTOCOL_OVERHEAD (extended_ids));
	block_end = block_size + block_offset;

	if (block_end > frame->buffer->priv->allocated_size) {
		arv_info_stream_thread ("[GvStream::process_data_block] %" G_GINTPTR_FORMAT " unexpected bytes in packet %u "
					 " for frame %" G_GUINT64_FORMAT,
					 block_end - frame->buffer->priv->allocated_size,
					 packet_id, frame->frame_id);
		thread_data->n_size_mismatch_errors++;

		block_end = frame->buffer->priv->allocated_size;
		block_size = block_end - block_offset;
	}

	memcpy (((char *) frame->buffer->priv->data) + block_offset, arv_gvsp_packet_get_data (packet), block_size);

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
                          guint32 packet_id,
                          size_t read_count)
{
        guint part_id;
        ptrdiff_t block_offset;

	if (frame->buffer->priv->status != ARV_BUFFER_STATUS_FILLING)
		return;

        if (arv_gvsp_multipart_packet_get_infos (packet, &part_id, &block_offset)) {
                size_t block_size;
                ptrdiff_t block_end;
                void *data;

                block_size = arv_gvsp_multipart_packet_get_data_size (packet, read_count);

                block_end = block_offset + block_size;

                if (block_end > frame->buffer->priv->allocated_size) {
                        arv_info_stream_thread ("[GvStream::process_multipart_block] %" G_GINTPTR_FORMAT
                                                " unexpected bytes in packet %u "
                                                " for frame %" G_GUINT64_FORMAT,
                                                block_end - frame->buffer->priv->allocated_size,
                                                packet_id, frame->frame_id);
                        return;
                }

                data = arv_gvsp_multipart_packet_get_data (packet);
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

					thread_data->n_resend_requests += n_missing_packets;

					first_missing = -1;
				}
			}
		}
	}
}

static void
_close_frame (ArvGvStreamThreadData *thread_data,
              guint64 time_us,
              ArvGvStreamFrameData *frame)
{
	if (frame->buffer->priv->status == ARV_BUFFER_STATUS_SUCCESS)
		thread_data->n_completed_buffers++;
	else
		if (frame->buffer->priv->status != ARV_BUFFER_STATUS_ABORTED)
			thread_data->n_failures++;

	if (frame->buffer->priv->status == ARV_BUFFER_STATUS_TIMEOUT)
		thread_data->n_timeouts++;

	if (frame->buffer->priv->status == ARV_BUFFER_STATUS_ABORTED)
		thread_data->n_aborted++;

	if (frame->buffer->priv->status != ARV_BUFFER_STATUS_SUCCESS &&
	    frame->buffer->priv->status != ARV_BUFFER_STATUS_ABORTED)
		thread_data->n_missing_packets += (int) frame->n_packets - (frame->last_valid_packet + 1);

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

		if (can_close_frame &&
		    thread_data->packet_resend == ARV_GV_STREAM_PACKET_RESEND_NEVER &&
		    iter->next != NULL) {
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

static ArvGvStreamFrameData *
_process_packet (ArvGvStreamThreadData *thread_data, const ArvGvspPacket *packet, size_t packet_size, guint64 time_us)

{
	ArvGvStreamFrameData *frame;
	guint32 packet_id;
	guint64 frame_id;
	int i;

	thread_data->n_received_packets++;

	frame_id = arv_gvsp_packet_get_frame_id (packet);
	packet_id = arv_gvsp_packet_get_packet_id (packet);

	if (thread_data->first_packet) {
		thread_data->last_frame_id = frame_id - 1;
		thread_data->first_packet = FALSE;
	}

	frame = _find_frame_data (thread_data, packet, packet_size, frame_id, packet_id, packet_size, time_us);

	if (frame != NULL) {
		ArvGvspPacketType packet_type = arv_gvsp_packet_get_packet_type (packet);

		if (arv_gvsp_packet_type_is_error (packet_type)) {
                        ArvGvcpError error = packet_type & 0xff;

			arv_info_stream_thread ("[GvStream::process_packet]"
						 " Error packet at dt = %" G_GINT64_FORMAT ", packet id = %u"
						 " frame id = %" G_GUINT64_FORMAT,
						 time_us - frame->first_packet_time_us,
						 packet_id, frame->frame_id);
			arv_gvsp_packet_debug (packet, packet_size, ARV_DEBUG_LEVEL_INFO);

                        if (error == ARV_GVCP_ERROR_PACKET_AND_PREVIOUS_REMOVED_FROM_MEMORY ||
                            error == ARV_GVCP_ERROR_PACKET_REMOVED_FROM_MEMORY ||
                            error == ARV_GVCP_ERROR_PACKET_UNAVAILABLE) {
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

                        content_type = arv_gvsp_packet_get_content_type (packet);

                        arv_gvsp_packet_debug (packet, packet_size,
                                               content_type == ARV_GVSP_CONTENT_TYPE_LEADER ||
                                               content_type == ARV_GVSP_CONTENT_TYPE_TRAILER ?
                                               ARV_DEBUG_LEVEL_DEBUG :
                                               ARV_DEBUG_LEVEL_TRACE);

                        switch (content_type) {
                                case ARV_GVSP_CONTENT_TYPE_LEADER:
                                        _process_data_leader (thread_data, frame, packet, packet_id);
                                        thread_data->n_transferred_bytes += packet_size;
                                        break;
                                case ARV_GVSP_CONTENT_TYPE_PAYLOAD:
                                        _process_payload_block (thread_data, frame, packet, packet_id,
                                                                packet_size);
                                        thread_data->n_transferred_bytes += packet_size;
                                        break;
                                case ARV_GVSP_CONTENT_TYPE_MULTIPART:
                                        _process_multipart_block (thread_data, frame, packet, packet_id,
                                                                  packet_size);
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
	guint64 time_us;
	gboolean use_poll;
	int i;
	GInputVector packet_iv[ARV_GV_STREAM_NUM_BUFFERS] = { {NULL, 0}, };
	GInputMessage packet_im[ARV_GV_STREAM_NUM_BUFFERS] = { {NULL, NULL, 0, 0, 0, NULL, NULL}, };
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

        g_mutex_lock (&thread_data->thread_started_mutex);
        thread_data->thread_started = TRUE;
        g_cond_signal (&thread_data->thread_started_cond);
        g_mutex_unlock (&thread_data->thread_started_mutex);

	do {
                int timeout_ms;
		int n_events;
		int errsv;

		if (thread_data->frames != NULL)
			timeout_ms = thread_data->packet_timeout_us / 1000;
		else
			timeout_ms = ARV_GV_STREAM_POLL_TIMEOUT_US / 1000;

		do {
			poll_fd[0].revents = 0;
			n_events = g_poll (poll_fd, use_poll ?  2 : 1, timeout_ms);
			errsv = errno;

		} while (n_events < 0 && errsv == EINTR);

		if (poll_fd[0].revents != 0) {
                        GError *error = NULL;
                        int n_msgs;

			arv_gpollfd_clear_one (&poll_fd[0], thread_data->socket);
			n_msgs = g_socket_receive_messages (thread_data->socket,
		 					    packet_im,
		 					    ARV_GV_STREAM_NUM_BUFFERS,
		 					    G_SOCKET_MSG_NONE,
		 					    NULL,
		 					    &error);

                        if (G_LIKELY(n_msgs > 0)) {
                                time_us = g_get_monotonic_time ();
                                for (i = 0; i < n_msgs; i++) {
                                        frame = _process_packet (thread_data,
                                                                 packet_iv[i].buffer,
                                                                 packet_im[i].bytes_received,
                                                                 time_us);
                                        _check_frame_completion (thread_data, time_us, frame);
                                }
                        } else {
                                arv_warning_stream_thread ("[GvStream::loop] receive_messages failed: %s",
                                                           error != NULL ? error->message : "Unknown reason");
                                g_clear_error (&error);
                        }
                } else {
                        time_us = g_get_monotonic_time ();
                        _check_frame_completion (thread_data, time_us, NULL);
                }

	} while (!g_cancellable_is_cancelled (thread_data->cancellable));

	if (use_poll)
		g_cancellable_release_fd (thread_data->cancellable);

	arv_gpollfd_finish_all (poll_fd,1);
	g_free (packet_buffers);
}


#if ARAVIS_HAS_PACKET_SOCKET

static void
_set_socket_filter (int socket, guint32 source_ip, guint32 source_port, guint32 destination_ip, guint32 destination_port)
{

#if 0
/*
 * sudo tcpdump -i lo -e -nn  "udp and src host 192.168.0.1 and src port 10 and dst host 192.168.0.2 and dst port 20" -d
 *
 * (000) ldh      [12]
 * (001) jeq      #0x86dd          jt 17	jf 2
 * (002) jeq      #0x800           jt 3	        jf 17
 * (003) ldb      [23]
 * (004) jeq      #0x11            jt 5	        jf 17
 * (005) ld       [26]
 * (006) jeq      #0xc0a80001      jt 7	        jf 17           Source host
 * (007) ldh      [20]
 * (008) jset     #0x1fff          jt 17	jf 9
 * (009) ldxb     4*([14]&0xf)
 * (010) ldh      [x + 14]
 * (011) jeq      #0xa             jt 12	jf 17           Source port
 * (012) ld       [30]
 * (013) jeq      #0xc0a80002      jt 14	jf 17           Destination host
 * (014) ldh      [x + 16]
 * (015) jeq      #0x14            jt 16	jf 17           Destination port
 * (016) ret      #262144
 * (017) ret      #0
 */

	struct sock_filter bpf[] = {
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
#else /* Variant without source port check. */
/*
 * sudo tcpdump -i lo -e -nn  "udp and src host 192.168.0.1 and dst host 192.168.0.2 and dst port 20" -d
 *
 * (000) ldh      [12]
 * (001) jeq      #0x86dd          jt 15	jf 2
 * (002) jeq      #0x800           jt 3	jf 15
 * (003) ldb      [23]
 * (004) jeq      #0x11            jt 5	jf 15
 * (005) ld       [26]
 * (006) jeq      #0xc0a80001      jt 7	jf 15
 * (007) ld       [30]
 * (008) jeq      #0xc0a80002      jt 9	jf 15
 * (009) ldh      [20]
 * (010) jset     #0x1fff          jt 15	jf 11
 * (011) ldxb     4*([14]&0xf)
 * (012) ldh      [x + 16]
 * (013) jeq      #0x14            jt 14	jf 15
 * (014) ret      #262144
 * (015) ret      #0
 */

	struct sock_filter bpf[] = {
		{ 0x28, 0, 0, 0x0000000c },
		{ 0x15, 13, 0, 0x000086dd },
		{ 0x15, 0, 12, 0x00000800 },
		{ 0x30, 0, 0, 0x00000017 },
		{ 0x15, 0, 10, 0x00000011 },
		{ 0x20, 0, 0, 0x0000001a },
		{ 0x15, 0, 8, source_ip },
		{ 0x20, 0, 0, 0x0000001e },
		{ 0x15, 0, 6, destination_ip },
                { 0x28, 0, 0, 0x00000014},
		{ 0x45, 4, 0, 0x00001fff },
		{ 0xb1, 0, 0, 0x0000000e },
		{ 0x48, 0, 0, 0x00000010 },
		{ 0x15, 0, 1, destination_port },
		{ 0x6, 0, 0, 0x00040000 },
		{ 0x6, 0, 0, 0x00000000 }
	};
#endif

	struct sock_fprog bpf_prog = {G_N_ELEMENTS (bpf), bpf};

	arv_info_stream_thread ("[GvStream::set_socket_filter] source ip = 0x%08x - port = %d - dest ip = 0x%08x - port %d",
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
	GPollFD poll_fd[2];
	char *buffer;
	struct tpacket_req3 req;
	struct sockaddr_ll local_address = {0};
	enum tpacket_versions version;
	int fd;
	unsigned block_id;
	const guint8 *bytes;
	guint32 interface_address;
	guint32 device_address;
	gboolean use_poll;

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

	poll_fd[0].fd = fd;
	poll_fd[0].events =  G_IO_IN;
	poll_fd[0].revents = 0;

	use_poll = g_cancellable_make_pollfd (thread_data->cancellable, &poll_fd[1]);

        g_mutex_lock (&thread_data->thread_started_mutex);
        thread_data->thread_started = TRUE;
        g_cond_signal (&thread_data->thread_started_cond);
        g_mutex_unlock (&thread_data->thread_started_mutex);

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
                                timeout_ms = thread_data->packet_timeout_us / 1000;
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
	} while (!g_cancellable_is_cancelled (thread_data->cancellable));

	if (use_poll)
		g_cancellable_release_fd (thread_data->cancellable);

bind_error:
	munmap (buffer, req.tp_block_size * req.tp_block_nr);
socket_option_error:
map_error:
	close (fd);
af_packet_error:
        g_mutex_lock (&thread_data->thread_started_mutex);
        thread_data->thread_started = TRUE;
        g_cond_signal (&thread_data->thread_started_cond);
        g_mutex_unlock (&thread_data->thread_started_mutex);
}

#endif /* ARAVIS_HAS_PACKET_SOCKET */

static void *
arv_gv_stream_thread (void *data)
{
	ArvGvStreamThreadData *thread_data = data;
#if ARAVIS_HAS_PACKET_SOCKET
	int fd;
#endif

	thread_data->frames = NULL;
	thread_data->last_frame_id = 0;
	thread_data->first_packet = TRUE;

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

#if ARAVIS_HAS_PACKET_SOCKET
	if (thread_data->use_packet_socket && (fd = socket (PF_PACKET, SOCK_RAW, g_htons (ETH_P_ALL))) >= 0) {
		close (fd);
		_ring_buffer_loop (thread_data);
	} else
#endif
		_loop (thread_data);

	_flush_frames (thread_data, g_get_monotonic_time ());

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

static void
arv_gv_stream_start_thread (ArvStream *stream)
{
	ArvGvStreamPrivate *priv = arv_gv_stream_get_instance_private (ARV_GV_STREAM (stream));
	ArvGvStreamThreadData *thread_data;

	g_return_if_fail (priv->thread == NULL);
	g_return_if_fail (priv->thread_data != NULL);

	thread_data = priv->thread_data;

        thread_data->thread_started = FALSE;
	thread_data->cancellable = g_cancellable_new ();
	priv->thread = g_thread_new ("arv_gv_stream", arv_gv_stream_thread, priv->thread_data);

        g_mutex_lock (&thread_data->thread_started_mutex);
        while (!thread_data->thread_started)
                g_cond_wait (&thread_data->thread_started_cond,
                             &thread_data->thread_started_mutex);
        g_mutex_unlock (&thread_data->thread_started_mutex);
}

static void
arv_gv_stream_stop_thread (ArvStream *stream)
{
	ArvGvStreamPrivate *priv = arv_gv_stream_get_instance_private (ARV_GV_STREAM (stream));
	ArvGvStreamThreadData *thread_data;

	g_return_if_fail (priv->thread != NULL);
	g_return_if_fail (priv->thread_data != NULL);

	thread_data = priv->thread_data;

	g_cancellable_cancel (thread_data->cancellable);
	g_thread_join (priv->thread);
	g_clear_object (&thread_data->cancellable);

	priv->thread = NULL;
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
	priv->thread_data->interface_address = g_object_ref (interface_address);
	priv->thread_data->interface_socket_address = g_inet_socket_address_new (interface_address, 0);
	priv->thread_data->device_socket_address = g_inet_socket_address_new (device_address, ARV_GVCP_PORT);
	g_socket_set_blocking (priv->thread_data->socket, FALSE);
	g_socket_bind (priv->thread_data->socket, priv->thread_data->interface_socket_address, FALSE, NULL);

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

	arv_gv_stream_start_thread (ARV_STREAM (gv_stream));
}

static void
arv_gv_stream_finalize (GObject *object)
{
	ArvGvStreamPrivate *priv = arv_gv_stream_get_instance_private (ARV_GV_STREAM (object));
        GError *error = NULL;

	arv_gv_stream_stop_thread (ARV_STREAM (object));

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
		g_clear_object (&thread_data->socket);

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

	stream_class->start_thread = arv_gv_stream_start_thread;
	stream_class->stop_thread = arv_gv_stream_stop_thread;

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
         * Size in bytes of the incoming socket buffer. A greater value helps to lower the number of missings packets,
         * as the expense of an increased memory usage.
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
