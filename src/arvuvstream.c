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
 * SECTION: arvuvstream
 * @short_description: USB3Vision video stream
 */

#include <arvuvstreamprivate.h>
#include <arvstreamprivate.h>
#include <arvbufferprivate.h>
#include <arvuvsp.h>
#include <arvuvcp.h>
#include <arvdebug.h>
#include <arvmisc.h>
#include <libusb.h>
#include <string.h>

#define MAXIMUM_TRANSFER_SIZE	1048576

static GObjectClass *parent_class = NULL;

struct _ArvUvStreamPrivate {
	GThread *thread;
	void *thread_data;
};

/* Acquisition thread */

typedef struct {
	ArvUvDevice *uv_device;
	ArvStream *stream;

	ArvStreamCallback callback;
	void *user_data;

	size_t leader_size;
	size_t payload_size;
	size_t trailer_size;

	guint64 offset;
	ArvBuffer *buffer;
	void *incoming_buffer;

	gboolean cancel;
	gboolean thread_enabled;
    gboolean blocking_schedule;

	ArvUvspPacket *current_packet;

	/* Statistics */

	guint n_completed_buffers;
	guint n_failures;
	guint n_underruns;
} ArvUvStreamThreadData;

static void *
arv_uv_stream_thread (void *data)
{
	ArvUvStreamThreadData *thread_data = data;
	size_t transferred;

	while (!g_atomic_int_get (&thread_data->cancel)) {
		transferred = 0;

		if(!arv_uv_is_transfer_active(thread_data->uv_device)) {
			size_t size;

			if (thread_data->buffer == NULL)
				size = thread_data->leader_size;
			else {
				if (thread_data->offset < thread_data->buffer->priv->size)
					size = MIN (thread_data->payload_size, thread_data->buffer->priv->size - thread_data->offset);
				else
					size = thread_data->trailer_size;
			}

			/* Avoid unnecessary memory copy by transferring data directly to the image buffer */
			if (thread_data->buffer != NULL &&
				thread_data->buffer->priv->status == ARV_BUFFER_STATUS_FILLING &&
				thread_data->offset + size <= thread_data->buffer->priv->size)
				thread_data->current_packet = thread_data->buffer->priv->data + thread_data->offset;
			else
				thread_data->current_packet = thread_data->incoming_buffer;

			arv_log_sp ("Asking for %u bytes", size);

			arv_uv_device_submit_bulk_transfer (thread_data->uv_device, ARV_UV_ENDPOINT_DATA, LIBUSB_ENDPOINT_IN,
				thread_data->current_packet, size, 0, NULL);
		}

		arv_uv_devce_collect_transfer(thread_data->uv_device, &transferred, thread_data->blocking_schedule, NULL);

		if (transferred > 0) {
			ArvUvspPacketType packet_type;

			arv_log_sp ("Received %d bytes", transferred);
			arv_uvsp_packet_debug (thread_data->current_packet, ARV_DEBUG_LEVEL_LOG);

			packet_type = arv_uvsp_packet_get_packet_type (thread_data->current_packet);
			switch (packet_type) {
				case ARV_UVSP_PACKET_TYPE_LEADER:
					if (thread_data->buffer != NULL) {
						arv_debug_stream_thread ("New leader received while a buffer is still open");
						thread_data->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
						arv_stream_push_output_buffer (thread_data->stream, thread_data->buffer);
						thread_data->n_failures++;
						thread_data->buffer = NULL;
					}
					thread_data->buffer = arv_stream_pop_input_buffer (thread_data->stream);
					if (thread_data->buffer != NULL) {
                        thread_data->buffer->priv->system_timestamp_ns = g_get_real_time () * 1000LL;
						thread_data->buffer->priv->status = ARV_BUFFER_STATUS_FILLING;
						thread_data->buffer->priv->gvsp_payload_type = ARV_GVSP_PAYLOAD_TYPE_IMAGE;
						arv_uvsp_packet_get_region (thread_data->current_packet,
									    &thread_data->buffer->priv->width,
									    &thread_data->buffer->priv->height,
									    &thread_data->buffer->priv->x_offset,
									    &thread_data->buffer->priv->y_offset);
						thread_data->buffer->priv->pixel_format = arv_uvsp_packet_get_pixel_format (thread_data->current_packet);
						thread_data->buffer->priv->frame_id = arv_uvsp_packet_get_frame_id (thread_data->current_packet);
						thread_data->buffer->priv->timestamp_ns = arv_uvsp_packet_get_timestamp (thread_data->current_packet);
						thread_data->offset = 0;
					} else
						thread_data->n_underruns++;
					break;
				case ARV_UVSP_PACKET_TYPE_TRAILER:
					if (thread_data->buffer != NULL) {
						arv_log_stream_thread ("Received %" G_GUINT64_FORMAT
								       " bytes - expected %" G_GUINT64_FORMAT,
								       thread_data->offset, thread_data->buffer->priv->size);

						/* If the image was incomplete, drop the frame and try again. */
						if (thread_data->offset != thread_data->buffer->priv->size) {
							arv_debug_stream_thread ("Incomplete image received, dropping");

							thread_data->buffer->priv->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
							arv_stream_push_output_buffer (thread_data->stream, thread_data->buffer);
							thread_data->n_underruns++;
							thread_data->buffer = NULL;
						} else {
							thread_data->buffer->priv->status = ARV_BUFFER_STATUS_SUCCESS;
							arv_stream_push_output_buffer (thread_data->stream, thread_data->buffer);
							thread_data->n_completed_buffers++;
							thread_data->buffer = NULL;
						}
					}
					break;
				case ARV_UVSP_PACKET_TYPE_DATA:
					if (thread_data->buffer != NULL && thread_data->buffer->priv->status == ARV_BUFFER_STATUS_FILLING) {
						if (thread_data->offset + transferred <= thread_data->buffer->priv->size) {
							if (thread_data->current_packet == thread_data->incoming_buffer)
								memcpy (((char *) thread_data->buffer->priv->data) + thread_data->offset, thread_data->current_packet, transferred);
							thread_data->offset += transferred;
						} else
							thread_data->buffer->priv->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
					}
					break;
				default:
					arv_debug_stream_thread ("Unkown packet type");
					break;
			}
		}

		if((thread_data->blocking_schedule || transferred <= 0) && !thread_data->thread_enabled) {
		    // Scheduling stops when threading is disabled and there are
            // no more packets to be processed
            break;
        }
	}

	return NULL;
}

/* ArvUvStream implemenation */


/**
 * arv_uv_stream_new: (skip)
 * @uv_device: a #ArvUvDevice
 * @callback: (scope call): image processing callback
 * @user_data: (closure): user data for @callback
 *
 * Return Value: (transfer full): a new #ArvStream.
 */

ArvStream *
arv_uv_stream_new (ArvUvDevice *uv_device, ArvStreamCallback callback, void *user_data)
{
	ArvDevice *device;
	ArvUvStream *uv_stream;
	ArvUvStreamThreadData *thread_data;
	ArvStream *stream;
	guint64 offset;
	guint64 sirm_offset;
	guint64 si_req_payload_size;
	guint32 si_req_leader_size;
	guint32 si_req_trailer_size;
	guint32 si_payload_size;
	guint32 si_payload_count;
	guint32 si_transfer1_size;
	guint32 si_transfer2_size;
	guint32 si_control;

	g_return_val_if_fail (ARV_IS_UV_DEVICE (uv_device), NULL);

	device = ARV_DEVICE (uv_device);

	arv_device_read_memory (device, ARV_ABRM_SBRM_ADDRESS, sizeof (guint64), &offset, NULL);
	arv_device_read_memory (device, offset + ARV_SBRM_SIRM_ADDRESS, sizeof (guint64), &sirm_offset, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SI_REQ_PAYLOAD_SIZE, sizeof (si_req_payload_size), &si_req_payload_size, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SI_REQ_LEADER_SIZE, sizeof (si_req_leader_size), &si_req_leader_size, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SI_REQ_TRAILER_SIZE, sizeof (si_req_trailer_size), &si_req_trailer_size, NULL);

	arv_debug_stream ("SI_REQ_PAYLOAD_SIZE =      0x%016lx", si_req_payload_size);
	arv_debug_stream ("SI_REQ_LEADER_SIZE =       0x%08x", si_req_leader_size);
	arv_debug_stream ("SI_REQ_TRAILER_SIZE =      0x%08x", si_req_trailer_size);

	si_payload_size = MAXIMUM_TRANSFER_SIZE;
	si_payload_count=  si_req_payload_size / si_payload_size;
	si_transfer1_size = si_req_payload_size % si_payload_size;
	si_transfer2_size = 0;

	arv_device_write_memory (device, sirm_offset + ARV_SI_MAX_LEADER_SIZE, sizeof (si_req_leader_size), &si_req_leader_size, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SI_MAX_TRAILER_SIZE, sizeof (si_req_trailer_size), &si_req_trailer_size, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SI_PAYLOAD_SIZE, sizeof (si_payload_size), &si_payload_size, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SI_PAYLOAD_COUNT, sizeof (si_payload_count), &si_payload_count, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SI_TRANSFER1_SIZE, sizeof (si_transfer1_size), &si_transfer1_size, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SI_TRANSFER2_SIZE, sizeof (si_transfer2_size), &si_transfer2_size, NULL);

	arv_debug_stream ("SI_PAYLOAD_SIZE =          0x%08x", si_payload_size);
	arv_debug_stream ("SI_PAYLOAD_COUNT =         0x%08x", si_payload_count);
	arv_debug_stream ("SI_TRANSFER1_SIZE =        0x%08x", si_transfer1_size);
	arv_debug_stream ("SI_TRANSFER2_SIZE =        0x%08x", si_transfer2_size);
	arv_debug_stream ("SI_MAX_LEADER_SIZE =       0x%08x", si_req_leader_size);
	arv_debug_stream ("SI_MAX_TRAILER_SIZE =      0x%08x", si_req_trailer_size);

	si_control = 0x1;
	arv_device_write_memory (device, sirm_offset + ARV_SI_CONTROL, sizeof (si_control), &si_control, NULL);

	uv_stream = g_object_new (ARV_TYPE_UV_STREAM, NULL);

	stream = ARV_STREAM (uv_stream);

	thread_data = g_new (ArvUvStreamThreadData, 1);
	thread_data->uv_device = g_object_ref (uv_device);
	thread_data->stream = stream;
	thread_data->callback = callback;
	thread_data->user_data = user_data;
	thread_data->cancel = FALSE;

	thread_data->leader_size = si_req_leader_size;
	thread_data->payload_size = si_payload_size;
	thread_data->trailer_size = si_req_trailer_size;

    if(si_req_leader_size == 0 || si_payload_size == 0 || si_req_trailer_size == 0) {
        g_error("[UvStream::arv_uv_stream_new]: Transfer size is 0");
    }

	thread_data->n_completed_buffers = 0;
	thread_data->n_failures = 0;
	thread_data->n_underruns = 0;

	thread_data->thread_enabled = ((arv_uv_device_get_stream_options(uv_device) & ARV_UV_STREAM_OPTION_THREADING_DISABLED) == 0);

	uv_stream->priv->thread_data = thread_data;

	thread_data->incoming_buffer = g_malloc (MAXIMUM_TRANSFER_SIZE);
	thread_data->offset = 0;
	thread_data->buffer = NULL;
	thread_data->current_packet = NULL;

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->user_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

	if(thread_data->thread_enabled) {
        thread_data->blocking_schedule = TRUE;
		uv_stream->priv->thread = arv_g_thread_new ("arv_uv_stream", arv_uv_stream_thread, uv_stream->priv->thread_data);
	} else {
		uv_stream->priv->thread = NULL;
        thread_data->blocking_schedule = FALSE;
		arv_uv_stream_thread(uv_stream->priv->thread_data);
	}

	return ARV_STREAM (uv_stream);
}

/* ArvStream implementation */

static void
arv_uv_stream_schedule_thread	(ArvStream *stream, gboolean block) {
    ArvUvStream *uv_stream = ARV_UV_STREAM (stream);
	ArvUvStreamThreadData *thread_data = uv_stream->priv->thread_data;

    if(thread_data != NULL && !thread_data->thread_enabled) {
        thread_data->blocking_schedule = block;
        arv_uv_stream_thread(uv_stream->priv->thread_data);
    }
}


static void
arv_uv_stream_get_statistics (ArvStream *stream,
				guint64 *n_completed_buffers,
				guint64 *n_failures,
				guint64 *n_underruns)
{
	ArvUvStream *uv_stream = ARV_UV_STREAM (stream);
	ArvUvStreamThreadData *thread_data;

	thread_data = uv_stream->priv->thread_data;

	*n_completed_buffers = thread_data->n_completed_buffers;
	*n_failures = thread_data->n_failures;
	*n_underruns = thread_data->n_underruns;
}

static void
arv_uv_stream_init (ArvUvStream *uv_stream)
{
	uv_stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (uv_stream, ARV_TYPE_UV_STREAM, ArvUvStreamPrivate);
}

static void
arv_uv_stream_finalize (GObject *object)
{
	ArvUvStream *uv_stream = ARV_UV_STREAM (object);

	if (uv_stream->priv->thread_data != NULL) {
		ArvUvStreamThreadData *thread_data;
		guint64 offset;
		guint64 sirm_offset;
		guint32 si_control;

		thread_data = uv_stream->priv->thread_data;

		g_atomic_int_set (&thread_data->cancel, TRUE);
		if(uv_stream->priv->thread != NULL) {
			g_thread_join (uv_stream->priv->thread);
		}

		if (thread_data->callback != NULL)
			thread_data->callback (thread_data->user_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

		g_free (thread_data->incoming_buffer);

		si_control = 0x0;
		arv_device_read_memory (ARV_DEVICE (thread_data->uv_device),
					ARV_ABRM_SBRM_ADDRESS, sizeof (guint64), &offset, NULL);
		arv_device_read_memory (ARV_DEVICE (thread_data->uv_device),
					offset + ARV_SBRM_SIRM_ADDRESS, sizeof (guint64), &sirm_offset, NULL);
		arv_device_write_memory (ARV_DEVICE (thread_data->uv_device),
					 sirm_offset + ARV_SI_CONTROL, sizeof (si_control), &si_control, NULL);

		g_clear_object (&thread_data->uv_device);
		g_free (thread_data);

		uv_stream->priv->thread_data = NULL;
		uv_stream->priv->thread = NULL;
	}

	parent_class->finalize (object);
}

static void
arv_uv_stream_class_init (ArvUvStreamClass *uv_stream_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (uv_stream_class);
	ArvStreamClass *stream_class = ARV_STREAM_CLASS (uv_stream_class);

	g_type_class_add_private (uv_stream_class, sizeof (ArvUvStreamPrivate));

	parent_class = g_type_class_peek_parent (uv_stream_class);

	object_class->finalize = arv_uv_stream_finalize;

	stream_class->get_statistics = arv_uv_stream_get_statistics;

	stream_class->schedule_thread = arv_uv_stream_schedule_thread;
}

G_DEFINE_TYPE (ArvUvStream, arv_uv_stream, ARV_TYPE_STREAM)
