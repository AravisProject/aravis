/* Aravis - Digital camera library
 *
 * Copyright © 2009-2019 Emmanuel Pacaud
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
#include <arvuvspprivate.h>
#include <arvuvcpprivate.h>
#include <arvdebug.h>
#include <arvmisc.h>
#include <libusb.h>
#include <string.h>

#define ARV_UV_STREAM_MAXIMUM_TRANSFER_SIZE	1048576

/* Acquisition thread */

typedef struct {
	ArvStream *stream;

	ArvUvDevice *uv_device;
	ArvStreamCallback callback;
	void *callback_data;

	size_t leader_size;
	size_t payload_size;
	size_t trailer_size;

	gboolean cancel;

	/* Statistics */

	guint n_completed_buffers;
	guint n_failures;
	guint n_underruns;
} ArvUvStreamThreadData;

typedef struct {
	GThread *thread;

	ArvUvStreamThreadData *thread_data;
} ArvUvStreamPrivate;

struct _ArvUvStream {
	ArvStream	stream;
};

struct _ArvUvStreamClass {
	ArvStreamClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvUvStream, arv_uv_stream, ARV_TYPE_STREAM, G_ADD_PRIVATE (ArvUvStream))

static void *
arv_uv_stream_thread (void *data)
{
	ArvUvStreamThreadData *thread_data = data;
	ArvUvspPacket *packet;
	ArvBuffer *buffer = NULL;
	void *incoming_buffer;
	guint64 offset;
	size_t transferred;

	arv_log_stream_thread ("Start USB3Vision stream thread");

	incoming_buffer = g_malloc (ARV_UV_STREAM_MAXIMUM_TRANSFER_SIZE);

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

	offset = 0;

	while (!g_atomic_int_get (&thread_data->cancel)) {
		GError *error = NULL;
		size_t size;
		transferred = 0;

		if (buffer == NULL)
			size = ARV_UV_STREAM_MAXIMUM_TRANSFER_SIZE;
		else {
			if (offset < buffer->priv->size)
				size = MIN (thread_data->payload_size, buffer->priv->size - offset);
			else
				size = thread_data->trailer_size;
		}

		/* Avoid unnecessary memory copy by transferring data directly to the image buffer */
		if (buffer != NULL &&
		    buffer->priv->status == ARV_BUFFER_STATUS_FILLING &&
		    offset + size <= buffer->priv->size)
			packet = (ArvUvspPacket *) (buffer->priv->data + offset);
		else
			packet = incoming_buffer;

		arv_log_sp ("Asking for %" G_GSIZE_FORMAT " bytes", size);
		arv_uv_device_bulk_transfer (thread_data->uv_device,  ARV_UV_ENDPOINT_DATA, LIBUSB_ENDPOINT_IN,
					     packet, size, &transferred, 0, &error);

		if (error != NULL) {
			arv_warning_sp ("USB transfer error: %s", error->message);
			g_clear_error (&error);
		} else {
			ArvUvspPacketType packet_type;

			arv_log_sp ("Received %" G_GSIZE_FORMAT " bytes", transferred);
			arv_uvsp_packet_debug (packet, ARV_DEBUG_LEVEL_LOG);

			packet_type = arv_uvsp_packet_get_packet_type (packet);
			switch (packet_type) {
				case ARV_UVSP_PACKET_TYPE_LEADER:
					if (buffer != NULL) {
						arv_debug_stream_thread ("New leader received while a buffer is still open");
						buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
						arv_stream_push_output_buffer (thread_data->stream, buffer);
						if (thread_data->callback != NULL)
							thread_data->callback (thread_data->callback_data,
									       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
									       buffer);
						thread_data->n_failures++;
						buffer = NULL;
					}
					buffer = arv_stream_pop_input_buffer (thread_data->stream);
					if (buffer != NULL) {
						buffer->priv->system_timestamp_ns = g_get_real_time () * 1000LL;
						buffer->priv->status = ARV_BUFFER_STATUS_FILLING;
						buffer->priv->payload_type = arv_uvsp_packet_get_buffer_payload_type (packet);
						buffer->priv->chunk_endianness = G_LITTLE_ENDIAN;
						if (buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE ||
						    buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA) {
							arv_uvsp_packet_get_region (packet,
										    &buffer->priv->width,
										    &buffer->priv->height,
										    &buffer->priv->x_offset,
										    &buffer->priv->y_offset);
							buffer->priv->pixel_format = arv_uvsp_packet_get_pixel_format (packet);
						}
						buffer->priv->frame_id = arv_uvsp_packet_get_frame_id (packet);
						buffer->priv->timestamp_ns = arv_uvsp_packet_get_timestamp (packet);
						offset = 0;
						if (thread_data->callback != NULL)
							thread_data->callback (thread_data->callback_data,
									       ARV_STREAM_CALLBACK_TYPE_START_BUFFER,
									       NULL);
					} else
						thread_data->n_underruns++;
					break;
				case ARV_UVSP_PACKET_TYPE_TRAILER:
					if (buffer != NULL) {
						arv_log_stream_thread ("Received %" G_GUINT64_FORMAT
								       " bytes - expected %zu",
								       offset, buffer->priv->size);

						/* If the image was incomplete, drop the frame and try again. */
						if (offset != buffer->priv->size) {
							arv_debug_stream_thread ("Incomplete image received, dropping "
										 "(received %" G_GUINT64_FORMAT
										 " / expected %" G_GSIZE_FORMAT ")",
										 offset, buffer->priv->size);

							buffer->priv->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
							arv_stream_push_output_buffer (thread_data->stream, buffer);
							if (thread_data->callback != NULL)
								thread_data->callback (thread_data->callback_data,
										       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
										       buffer);
							thread_data->n_failures++;
							buffer = NULL;
						} else {
							buffer->priv->status = ARV_BUFFER_STATUS_SUCCESS;
							arv_stream_push_output_buffer (thread_data->stream, buffer);
							if (thread_data->callback != NULL)
								thread_data->callback (thread_data->callback_data,
										       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
										       buffer);
							thread_data->n_completed_buffers++;
							buffer = NULL;
						}
					}
					break;
				case ARV_UVSP_PACKET_TYPE_DATA:
					if (buffer != NULL && buffer->priv->status == ARV_BUFFER_STATUS_FILLING) {
						if (offset + transferred <= buffer->priv->size) {
							if (packet == incoming_buffer)
								memcpy (((char *) buffer->priv->data) + offset, packet, transferred);
							offset += transferred;
						} else
							buffer->priv->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
					}
					break;
				default:
					arv_debug_stream_thread ("Unknown packet type");
					break;
			}
		}
	}

        if (buffer != NULL) {
		buffer->priv->status = ARV_BUFFER_STATUS_ABORTED;
		arv_stream_push_output_buffer (thread_data->stream, buffer);
		if (thread_data->callback != NULL)
			thread_data->callback (thread_data->callback_data,
					       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
					       buffer);
	}

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

	g_free (incoming_buffer);

        /* The thread was cancelled with unprocessed frame. Release it to prevent memory leak */
	arv_log_stream_thread ("Stop USB3Vision stream thread");

	return NULL;
}

/* ArvUvStream implementation */

static guint32
align (guint32 val, guint32 alignment)
{
	/* Alignment must be a power of two, otherwise the used alignment algorithm does not work. */
	g_assert (alignment > 0 && (alignment & (alignment - 1)) == 0);

	return (val + (alignment - 1)) & ~(alignment - 1);
}

static void
arv_uv_stream_start_thread (ArvStream *stream)
{
	ArvUvStream *uv_stream = ARV_UV_STREAM (stream);
	ArvUvStreamPrivate *priv = arv_uv_stream_get_instance_private (uv_stream);
	ArvUvStreamThreadData *thread_data;
	ArvDevice *device;
	guint64 offset;
	guint64 sirm_offset;
	guint32 si_info;
	guint64 si_req_payload_size;
	guint32 si_req_leader_size;
	guint32 si_req_trailer_size;
	guint32 si_payload_size;
	guint32 si_payload_count;
	guint32 si_transfer1_size;
	guint32 si_transfer2_size;
	guint32 si_control;
	guint32 alignment;
	guint32 aligned_maximum_transfer_size;

	g_return_if_fail (priv->thread == NULL);
	g_return_if_fail (priv->thread_data != NULL);

	thread_data = priv->thread_data;

	device = ARV_DEVICE (thread_data->uv_device);

	arv_device_read_memory (device, ARV_ABRM_SBRM_ADDRESS, sizeof (guint64), &offset, NULL);
	arv_device_read_memory (device, offset + ARV_SBRM_SIRM_ADDRESS, sizeof (guint64), &sirm_offset, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SIRM_INFO, sizeof (si_info), &si_info, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SIRM_REQ_PAYLOAD_SIZE, sizeof (si_req_payload_size), &si_req_payload_size, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SIRM_REQ_LEADER_SIZE, sizeof (si_req_leader_size), &si_req_leader_size, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SIRM_REQ_TRAILER_SIZE, sizeof (si_req_trailer_size), &si_req_trailer_size, NULL);

	alignment = 1 << ((si_info & ARV_SIRM_INFO_ALIGNMENT_MASK) >> ARV_SIRM_INFO_ALIGNMENT_SHIFT);

	arv_debug_stream ("SIRM_INFO             = 0x%08x", si_info);
	arv_debug_stream ("SIRM_REQ_PAYLOAD_SIZE = 0x%016" G_GINT64_MODIFIER "x", si_req_payload_size);
	arv_debug_stream ("SIRM_REQ_LEADER_SIZE  = 0x%08x", si_req_leader_size);
	arv_debug_stream ("SIRM_REQ_TRAILER_SIZE = 0x%08x", si_req_trailer_size);

	arv_debug_stream ("Required alignment    = %d", alignment);

	aligned_maximum_transfer_size = ARV_UV_STREAM_MAXIMUM_TRANSFER_SIZE / alignment * alignment;

	if (si_req_leader_size < 1) {
		arv_warning_stream ("Wrong SI_REQ_LEADER_SIZE value, using %d instead", aligned_maximum_transfer_size);
		si_req_leader_size = aligned_maximum_transfer_size;
	} else {
		si_req_leader_size = align (si_req_leader_size, alignment);
	}

	if (si_req_trailer_size < 1) {
		arv_warning_stream ("Wrong SI_REQ_TRAILER_SIZE value, using %d instead", aligned_maximum_transfer_size);
		si_req_trailer_size = aligned_maximum_transfer_size;
	} else {
		si_req_trailer_size = align (si_req_trailer_size, alignment);
	}

	si_payload_size = aligned_maximum_transfer_size;
	si_payload_count=  si_req_payload_size / si_payload_size;
	si_transfer1_size = align(si_req_payload_size % si_payload_size, alignment);
	si_transfer2_size = 0;

	arv_device_write_memory (device, sirm_offset + ARV_SIRM_MAX_LEADER_SIZE, sizeof (si_req_leader_size), &si_req_leader_size, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SIRM_MAX_TRAILER_SIZE, sizeof (si_req_trailer_size), &si_req_trailer_size, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SIRM_PAYLOAD_SIZE, sizeof (si_payload_size), &si_payload_size, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SIRM_PAYLOAD_COUNT, sizeof (si_payload_count), &si_payload_count, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SIRM_TRANSFER1_SIZE, sizeof (si_transfer1_size), &si_transfer1_size, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SIRM_TRANSFER2_SIZE, sizeof (si_transfer2_size), &si_transfer2_size, NULL);

	arv_debug_stream ("SIRM_PAYLOAD_SIZE     = 0x%08x", si_payload_size);
	arv_debug_stream ("SIRM_PAYLOAD_COUNT    = 0x%08x", si_payload_count);
	arv_debug_stream ("SIRM_TRANSFER1_SIZE   = 0x%08x", si_transfer1_size);
	arv_debug_stream ("SIRM_TRANSFER2_SIZE   = 0x%08x", si_transfer2_size);
	arv_debug_stream ("SIRM_MAX_LEADER_SIZE  = 0x%08x", si_req_leader_size);
	arv_debug_stream ("SIRM_MAX_TRAILER_SIZE = 0x%08x", si_req_trailer_size);

	si_control = ARV_SIRM_CONTROL_STREAM_ENABLE;
	arv_device_write_memory (device, sirm_offset + ARV_SIRM_CONTROL, sizeof (si_control), &si_control, NULL);

	thread_data->leader_size = si_req_leader_size;
	thread_data->payload_size = si_payload_size;
	thread_data->trailer_size = si_req_trailer_size;
	thread_data->cancel = FALSE;

	priv->thread = g_thread_new ("arv_uv_stream", arv_uv_stream_thread, priv->thread_data);
}

static void
arv_uv_stream_stop_thread (ArvStream *stream)
{
	ArvUvStream *uv_stream = ARV_UV_STREAM (stream);
	ArvUvStreamPrivate *priv = arv_uv_stream_get_instance_private (uv_stream);
	ArvUvStreamThreadData *thread_data;
	guint64 offset;
	guint64 sirm_offset;
	guint32 si_control;

	g_return_if_fail (priv->thread != NULL);
	g_return_if_fail (priv->thread_data != NULL);

	thread_data = priv->thread_data;

	g_atomic_int_set (&priv->thread_data->cancel, TRUE);
	g_thread_join (priv->thread);

	priv->thread = NULL;

	si_control = 0x0;
	arv_device_read_memory (ARV_DEVICE (thread_data->uv_device),
				ARV_ABRM_SBRM_ADDRESS, sizeof (guint64), &offset, NULL);
	arv_device_read_memory (ARV_DEVICE (thread_data->uv_device),
				offset + ARV_SBRM_SIRM_ADDRESS, sizeof (guint64), &sirm_offset, NULL);
	arv_device_write_memory (ARV_DEVICE (thread_data->uv_device),
				 sirm_offset + ARV_SIRM_CONTROL, sizeof (si_control), &si_control, NULL);

}

/**
 * arv_uv_stream_new: (skip)
 * @uv_device: a #ArvUvDevice
 * @callback: (scope call): image processing callback
 * @callback_data: (closure): user data for @callback
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Return Value: (transfer full): a new #ArvStream.
 */

ArvStream *
arv_uv_stream_new (ArvUvDevice *uv_device, ArvStreamCallback callback, void *callback_data, GError **error)
{
	return g_initable_new (ARV_TYPE_UV_STREAM, NULL, error,
			       "device", uv_device,
			       "callback", callback,
			       "callback-data", callback_data,
			       NULL);
}

static void
arv_uv_stream_constructed (GObject *object)
{
	ArvUvStream *uv_stream = ARV_UV_STREAM (object);
	ArvStream *stream = ARV_STREAM (uv_stream);
	ArvUvStreamPrivate *priv = arv_uv_stream_get_instance_private (uv_stream);
	ArvUvStreamThreadData *thread_data;

	thread_data = g_new (ArvUvStreamThreadData, 1);
	thread_data->stream = stream;

	g_object_get (object,
		      "device", &thread_data->uv_device,
		      "callback", &thread_data->callback,
		      "callback-data", &thread_data->callback_data,
		      NULL);

	thread_data->n_completed_buffers = 0;
	thread_data->n_failures = 0;
	thread_data->n_underruns = 0;

	priv->thread_data = thread_data;

	arv_uv_stream_start_thread (ARV_STREAM (uv_stream));
}

/* ArvStream implementation */

static void
arv_uv_stream_get_statistics (ArvStream *stream,
				guint64 *n_completed_buffers,
				guint64 *n_failures,
				guint64 *n_underruns)
{
	ArvUvStream *uv_stream = ARV_UV_STREAM (stream);
	ArvUvStreamPrivate *priv = arv_uv_stream_get_instance_private (uv_stream);
	ArvUvStreamThreadData *thread_data;

	thread_data = priv->thread_data;

	*n_completed_buffers = thread_data->n_completed_buffers;
	*n_failures = thread_data->n_failures;
	*n_underruns = thread_data->n_underruns;
}

static void
arv_uv_stream_init (ArvUvStream *uv_stream)
{
}

static void
arv_uv_stream_finalize (GObject *object)
{
	ArvUvStream *uv_stream = ARV_UV_STREAM (object);
	ArvUvStreamPrivate *priv = arv_uv_stream_get_instance_private (uv_stream);

	arv_uv_stream_stop_thread (ARV_STREAM (uv_stream));

	if (priv->thread_data != NULL) {
		ArvUvStreamThreadData *thread_data;

		thread_data = priv->thread_data;

		arv_debug_stream ("[UvStream::finalize] n_completed_buffers    = %u",
				  thread_data->n_completed_buffers);
		arv_debug_stream ("[UvStream::finalize] n_failures             = %u",
				  thread_data->n_failures);
		arv_debug_stream ("[UvStream::finalize] n_underruns            = %u",
				  thread_data->n_underruns);

		g_clear_object (&thread_data->uv_device);
		g_clear_pointer (&priv->thread_data, g_free);
	}

	G_OBJECT_CLASS (arv_uv_stream_parent_class)->finalize (object);
}

static void
arv_uv_stream_class_init (ArvUvStreamClass *uv_stream_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (uv_stream_class);
	ArvStreamClass *stream_class = ARV_STREAM_CLASS (uv_stream_class);

	object_class->constructed = arv_uv_stream_constructed;
	object_class->finalize = arv_uv_stream_finalize;

	stream_class->start_thread = arv_uv_stream_start_thread;
	stream_class->stop_thread = arv_uv_stream_stop_thread;
	stream_class->get_statistics = arv_uv_stream_get_statistics;
}
