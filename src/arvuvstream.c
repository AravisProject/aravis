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

#include <arvenumtypes.h>
#include <arvuvstreamprivate.h>
#include <arvstreamprivate.h>
#include <arvbufferprivate.h>
#include <arvuvspprivate.h>
#include <arvuvcpprivate.h>
#include <arvdebug.h>
#include <arvmisc.h>
#include <libusb.h>
#include <string.h>

#define ARV_UV_STREAM_MAXIMUM_TRANSFER_SIZE	(1024*1024*1)
#define ARV_UV_STREAM_MAXIMUM_SUBMIT_TOTAL	(8*1024*1024)

enum {
       ARV_UV_STREAM_PROPERTY_0,
       ARV_UV_STREAM_PROPERTY_USB_MODE
} ArvUvStreamProperties;

/* Acquisition thread */

typedef struct {
        guint64 n_completed_buffers;
        guint64 n_failures;
        guint64 n_underruns;

        guint64 n_transferred_bytes;
        guint64 n_ignored_bytes;
} ArvStreamStatistics;

typedef struct {
	ArvStream *stream;

	ArvUvDevice *uv_device;
	ArvStreamCallback callback;
	void *callback_data;

	size_t leader_size;
	size_t payload_size;
	size_t trailer_size;

	gboolean cancel;

	/* Notification for completed transfers and cancellation */
	GMutex stream_mtx;
	GCond stream_event;

	/* Statistics */
	ArvStreamStatistics statistics;

} ArvUvStreamThreadData;

typedef struct {
	GThread *thread;
	ArvUvStreamThreadData *thread_data;
	ArvUvUsbMode usb_mode;
} ArvUvStreamPrivate;

struct _ArvUvStream {
	ArvStream	stream;
};

struct _ArvUvStreamClass {
	ArvStreamClass parent_class;
};

typedef struct {
	ArvBuffer *buffer;
	ArvStream *stream;

	GMutex* transfer_completed_mtx;
	GCond* transfer_completed_event;

	size_t total_payload_transferred;

	guint8 *leader_buffer, *trailer_buffer;

	int num_payload_transfers;
	struct libusb_transfer *leader_transfer, *trailer_transfer, **payload_transfers;

	guint num_submitted;

	gint *total_submitted_bytes;

	ArvStreamStatistics *statistics;
} ArvUvStreamBufferContext;

G_DEFINE_TYPE_WITH_CODE (ArvUvStream, arv_uv_stream, ARV_TYPE_STREAM, G_ADD_PRIVATE (ArvUvStream))

static void
arv_uv_stream_buffer_context_wait_transfer_completed (ArvUvStreamBufferContext* ctx)
{
	g_mutex_lock( ctx->transfer_completed_mtx );
	g_cond_wait( ctx->transfer_completed_event, ctx->transfer_completed_mtx );
	g_mutex_unlock( ctx->transfer_completed_mtx );
}

static void
arv_uv_stream_buffer_context_notify_transfer_completed (ArvUvStreamBufferContext* ctx)
{
	g_mutex_lock( ctx->transfer_completed_mtx );
	g_cond_broadcast( ctx->transfer_completed_event );
	g_mutex_unlock( ctx->transfer_completed_mtx );
}

static
void arv_uv_stream_leader_cb (struct libusb_transfer *transfer)
{
	ArvUvStreamBufferContext *ctx = transfer->user_data;
	ArvUvspPacket *packet = (ArvUvspPacket*)transfer->buffer;

	switch (transfer->status) {
		case LIBUSB_TRANSFER_COMPLETED:
			arv_uvsp_packet_debug (packet, ARV_DEBUG_LEVEL_DEBUG);

			if (arv_uvsp_packet_get_packet_type (packet) != ARV_UVSP_PACKET_TYPE_LEADER) {
				arv_warning_stream_thread ("Unexpected packet type (was expecting leader packet)");
				ctx->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
				break;
			}

			ctx->buffer->priv->system_timestamp_ns = g_get_real_time () * 1000LL;
                        ctx->buffer->priv->payload_type = arv_uvsp_packet_get_buffer_payload_type (packet);
                        ctx->buffer->priv->chunk_endianness = G_LITTLE_ENDIAN;
                        if (ctx->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE ||
                            ctx->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA) {
                                arv_uvsp_packet_get_region (packet,
                                                            &ctx->buffer->priv->width, &ctx->buffer->priv->height,
                                                            &ctx->buffer->priv->x_offset, &ctx->buffer->priv->y_offset);
                                ctx->buffer->priv->pixel_format = arv_uvsp_packet_get_pixel_format (packet);
                        }
                        ctx->buffer->priv->frame_id = arv_uvsp_packet_get_frame_id (packet);
                        ctx->buffer->priv->timestamp_ns = arv_uvsp_packet_get_timestamp (packet);
                        break;
                default:
                        arv_warning_stream_thread ("Leader transfer failed: transfer->status = %d", transfer->status);
                        ctx->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
                        break;
        }

	g_atomic_int_dec_and_test (&ctx->num_submitted);
	g_atomic_int_add (ctx->total_submitted_bytes, -transfer->length);
	ctx->statistics->n_transferred_bytes += transfer->length;
	arv_uv_stream_buffer_context_notify_transfer_completed (ctx);
}

static
void arv_uv_stream_trailer_cb (struct libusb_transfer *transfer)
{
	ArvUvStreamBufferContext *ctx = transfer->user_data;
	ArvUvspPacket *packet = (ArvUvspPacket*)transfer->buffer;

	switch (transfer->status) {
		case LIBUSB_TRANSFER_COMPLETED:
			arv_uvsp_packet_debug (packet, ARV_DEBUG_LEVEL_DEBUG);

			if (arv_uvsp_packet_get_packet_type (packet) != ARV_UVSP_PACKET_TYPE_TRAILER) {
				arv_warning_stream_thread ("Unexpected packet type (was expecting trailer packet)");
				ctx->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
				break;
			}

			arv_debug_stream_thread ("Total payload: %zu bytes", ctx->total_payload_transferred);
			if (ctx->total_payload_transferred < ctx->buffer->priv->size) {
				arv_warning_stream_thread ("Total payload smaller than expected");
				ctx->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
				break;
			}

			break;
		default:
			arv_warning_stream_thread ("Trailer transfer failed: transfer->status = %d", transfer->status);
			ctx->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
			break;
	}

	switch (ctx->buffer->priv->status) {
		case ARV_BUFFER_STATUS_FILLING:
			ctx->buffer->priv->status = ARV_BUFFER_STATUS_SUCCESS;
			ctx->statistics->n_completed_buffers += 1;
			break;
		default:
			ctx->statistics->n_failures += 1;
			break;
	}

	arv_stream_push_output_buffer (ctx->stream, ctx->buffer);

	g_atomic_int_dec_and_test( &ctx->num_submitted );
	g_atomic_int_add (ctx->total_submitted_bytes, -transfer->length);
	ctx->statistics->n_transferred_bytes += transfer->length;
	arv_uv_stream_buffer_context_notify_transfer_completed (ctx);
}

static
void arv_uv_stream_payload_cb (struct libusb_transfer *transfer)
{
	ArvUvStreamBufferContext *ctx = transfer->user_data;

	switch (transfer->status) {
		case LIBUSB_TRANSFER_COMPLETED:
			ctx->total_payload_transferred += transfer->actual_length;
			break;
		default:
			arv_warning_stream_thread ("Payload transfer failed: transfer->status = %d", transfer->status);
			ctx->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
			break;
	}

	g_atomic_int_dec_and_test( &ctx->num_submitted );
	g_atomic_int_add (ctx->total_submitted_bytes, -transfer->length);
	ctx->statistics->n_transferred_bytes += transfer->length;
	arv_uv_stream_buffer_context_notify_transfer_completed (ctx);
}

static ArvUvStreamBufferContext*
arv_uv_stream_buffer_context_new (ArvBuffer *buffer, ArvUvStreamThreadData *thread_data, gint *total_submitted_bytes)
{
	ArvUvStreamBufferContext* ctx = g_malloc (sizeof(ArvUvStreamBufferContext));
	int i;
	size_t offset = 0;

	ctx->buffer = buffer;
	ctx->stream = thread_data->stream;
	ctx->transfer_completed_mtx = &thread_data->stream_mtx;
	ctx->transfer_completed_event = &thread_data->stream_event;

	ctx->leader_buffer = g_malloc (thread_data->leader_size);
	ctx->leader_transfer = libusb_alloc_transfer (0);
	arv_uv_device_fill_bulk_transfer (ctx->leader_transfer, thread_data->uv_device,
		ARV_UV_ENDPOINT_DATA, LIBUSB_ENDPOINT_IN,
		ctx->leader_buffer, thread_data->leader_size,
		arv_uv_stream_leader_cb, ctx,
		0);

	ctx->num_payload_transfers = (buffer->priv->size - 1) / thread_data->payload_size + 1;
	ctx->payload_transfers = g_malloc (ctx->num_payload_transfers * sizeof(struct libusb_transfer*));

	for (i = 0; i < ctx->num_payload_transfers; ++i) {
		size_t size = MIN (thread_data->payload_size, buffer->priv->size - offset);

		ctx->payload_transfers[i] = libusb_alloc_transfer(0);

		arv_uv_device_fill_bulk_transfer (ctx->payload_transfers[i], thread_data->uv_device,
			ARV_UV_ENDPOINT_DATA, LIBUSB_ENDPOINT_IN,
			buffer->priv->data + offset, size,
			arv_uv_stream_payload_cb, ctx,
			0);

		offset += size;
	}

	ctx->trailer_buffer = g_malloc (thread_data->trailer_size);
	ctx->trailer_transfer = libusb_alloc_transfer (0);
	arv_uv_device_fill_bulk_transfer (ctx->trailer_transfer, thread_data->uv_device,
		ARV_UV_ENDPOINT_DATA, LIBUSB_ENDPOINT_IN,
		ctx->trailer_buffer, thread_data->trailer_size,
		arv_uv_stream_trailer_cb, ctx,
		0);

	ctx->num_submitted = 0;
	ctx->total_submitted_bytes = total_submitted_bytes;
	ctx->statistics = &thread_data->statistics;

	return ctx;
}

static void
arv_uv_stream_buffer_context_cancel (gpointer key, gpointer value, gpointer user_data)
{
	ArvUvStreamBufferContext* ctx = value;
	int i;

	libusb_cancel_transfer (ctx->leader_transfer );

	for (i = 0; i < ctx->num_payload_transfers; ++i) {
		libusb_cancel_transfer (ctx->payload_transfers[i]);
	}

	libusb_cancel_transfer (ctx->trailer_transfer);

	while (ctx->num_submitted > 0)
	{
		arv_uv_stream_buffer_context_wait_transfer_completed (ctx);
	}
}

static void
arv_uv_stream_buffer_context_free (gpointer data)
{
	ArvUvStreamBufferContext* ctx = data;
	int i;

	g_return_if_fail (ctx->num_submitted == 0);

	libusb_free_transfer (ctx->leader_transfer);
	for (i = 0; i < ctx->num_payload_transfers; ++i) {
		libusb_free_transfer (ctx->payload_transfers[i]);
	}
	libusb_free_transfer (ctx->trailer_transfer );

	g_free (ctx->leader_buffer);
	g_free (ctx->trailer_buffer);

	g_free (ctx);
}

static void
arv_uv_stream_submit_transfer (ArvUvStreamBufferContext* ctx, struct libusb_transfer* transfer, gboolean* cancel)
{
	while (!g_atomic_int_get (cancel) &&
               ((g_atomic_int_get(ctx->total_submitted_bytes) + transfer->length) > ARV_UV_STREAM_MAXIMUM_SUBMIT_TOTAL)) {
		arv_uv_stream_buffer_context_wait_transfer_completed (ctx);
	}

	while (!g_atomic_int_get (cancel)) {
		int status = libusb_submit_transfer (transfer);

		switch (status)
		{
		case LIBUSB_SUCCESS:
			g_atomic_int_inc (&ctx->num_submitted);
			g_atomic_int_add (ctx->total_submitted_bytes, transfer->length);
			return;

		case LIBUSB_ERROR_IO:
                        /*
                         * arv_debug_stream_thread ("libusb_submit_transfer failed (%d)", status);
                         *
                         * The kernel USB memory buffer limit has been reached (default 16MBytes)
                         *
                         * In order to allow more memory to be used for submitted buffers, increase usbfs_memory_mb:
                         * sudo modprobe usbcore usbfs_memory_mb=1000
                        */
			arv_uv_stream_buffer_context_wait_transfer_completed (ctx);
			break;

		default:
			arv_warning_stream_thread ("libusb_submit_transfer failed (%d)", status);
			return;
		}
	}
}

static void *
arv_uv_stream_thread_async (void *data)
{
	ArvUvStreamThreadData *thread_data = data;
	ArvBuffer *buffer = NULL;
	GHashTable *ctx_lookup;
	gint total_submitted_bytes = 0;
	int i;

	arv_debug_stream_thread ("Start async USB3Vision stream thread");
	arv_debug_stream_thread ("leader_size = %zu", thread_data->leader_size );
	arv_debug_stream_thread ("payload_size = %zu", thread_data->payload_size );
	arv_debug_stream_thread ("trailer_size = %zu", thread_data->trailer_size );

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

	ctx_lookup = g_hash_table_new_full( g_direct_hash, g_direct_equal, NULL, arv_uv_stream_buffer_context_free );

	while (!g_atomic_int_get (&thread_data->cancel)) {
		ArvUvStreamBufferContext* ctx;

                buffer = arv_stream_pop_input_buffer (thread_data->stream);

		if( buffer == NULL ) {
			thread_data->statistics.n_underruns += 1;
                        /* NOTE: n_ignored_bytes is not accumulated because it doesn't submit next USB transfer if
                         * buffer is shortage. It means back pressure might be hanlded by USB slave side. */
			continue;
		}

		ctx = g_hash_table_lookup( ctx_lookup, buffer );
		if (!ctx) {
			arv_debug_stream_thread ("Stream buffer context not found for buffer %p, creating...", buffer);

			ctx = arv_uv_stream_buffer_context_new (buffer, thread_data, &total_submitted_bytes);

			g_hash_table_insert (ctx_lookup, buffer, ctx);
		}


		ctx->total_payload_transferred = 0;
		buffer->priv->status = ARV_BUFFER_STATUS_FILLING;

		arv_uv_stream_submit_transfer (ctx, ctx->leader_transfer, &thread_data->cancel);

		for (i = 0; i < ctx->num_payload_transfers; ++i) {
			arv_uv_stream_submit_transfer (ctx, ctx->payload_transfers[i], &thread_data->cancel);
		}

		arv_uv_stream_submit_transfer (ctx, ctx->trailer_transfer, &thread_data->cancel);
	}

	g_hash_table_foreach (ctx_lookup, arv_uv_stream_buffer_context_cancel, NULL);

	g_hash_table_destroy (ctx_lookup);

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

	arv_debug_stream_thread ("Stop USB3Vision stream thread");

	return NULL;
}

static void *
arv_uv_stream_thread_sync (void *data)
{
	ArvUvStreamThreadData *thread_data = data;
	ArvUvspPacket *packet;
	ArvBuffer *buffer = NULL;
	void *incoming_buffer;
	guint64 offset;
	size_t transferred;

	arv_debug_stream_thread ("Start sync USB3Vision stream thread");

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

		arv_debug_sp ("Asking for %" G_GSIZE_FORMAT " bytes", size);
		arv_uv_device_bulk_transfer (thread_data->uv_device,  ARV_UV_ENDPOINT_DATA, LIBUSB_ENDPOINT_IN,
					     packet, size, &transferred, 0, &error);

		if (error != NULL) {
			arv_warning_sp ("USB transfer error: %s", error->message);
			g_clear_error (&error);
		} else {
			ArvUvspPacketType packet_type;

			arv_debug_sp ("Received %" G_GSIZE_FORMAT " bytes", transferred);
			arv_uvsp_packet_debug (packet, ARV_DEBUG_LEVEL_DEBUG);

			packet_type = arv_uvsp_packet_get_packet_type (packet);
			switch (packet_type) {
				case ARV_UVSP_PACKET_TYPE_LEADER:
					if (buffer != NULL) {
						arv_info_stream_thread ("New leader received while a buffer is still open");
						buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
						arv_stream_push_output_buffer (thread_data->stream, buffer);
						if (thread_data->callback != NULL)
							thread_data->callback (thread_data->callback_data,
									       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
									       buffer);
						thread_data->statistics.n_failures++;
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
                                                thread_data->statistics.n_transferred_bytes += transferred;
                                        } else {
                                                thread_data->statistics.n_underruns++;
                                                thread_data->statistics.n_ignored_bytes += transferred;
                                        }
                                        break;
				case ARV_UVSP_PACKET_TYPE_TRAILER:
					if (buffer != NULL) {
						arv_debug_stream_thread ("Received %" G_GUINT64_FORMAT
								       " bytes - expected %zu",
								       offset, buffer->priv->size);

						/* If the image was incomplete, drop the frame and try again. */
						if (offset != buffer->priv->size) {
							arv_info_stream_thread ("Incomplete image received, dropping "
										 "(received %" G_GUINT64_FORMAT
										 " / expected %" G_GSIZE_FORMAT ")",
										 offset, buffer->priv->size);

							buffer->priv->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
							arv_stream_push_output_buffer (thread_data->stream, buffer);
							if (thread_data->callback != NULL)
								thread_data->callback (thread_data->callback_data,
										       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
										       buffer);
							thread_data->statistics.n_failures++;
                                                        thread_data->statistics.n_ignored_bytes += transferred;
                                                        buffer = NULL;
						} else {
							buffer->priv->status = ARV_BUFFER_STATUS_SUCCESS;
							arv_stream_push_output_buffer (thread_data->stream, buffer);
							if (thread_data->callback != NULL)
								thread_data->callback (thread_data->callback_data,
										       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
										       buffer);
                                                        thread_data->statistics.n_completed_buffers++;
                                                        thread_data->statistics.n_transferred_bytes += transferred;
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
                                                        thread_data->statistics.n_transferred_bytes += transferred;
						} else {
							buffer->priv->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
                                                        thread_data->statistics.n_ignored_bytes += transferred;
                                                }
					} else {
                                                        thread_data->statistics.n_ignored_bytes += transferred;
                                        }
					break;
				default:
					arv_info_stream_thread ("Unknown packet type");
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

	arv_debug_stream_thread ("Stop USB3Vision stream thread");

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

	arv_device_read_memory (device, ARV_ABRM_SBRM_ADDRESS,
                                sizeof (guint64), &offset, NULL);
	arv_device_read_memory (device, offset + ARV_SBRM_SIRM_ADDRESS,
                                sizeof (guint64), &sirm_offset, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SIRM_INFO,
                                sizeof (si_info), &si_info, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SIRM_REQ_PAYLOAD_SIZE,
                                sizeof (si_req_payload_size), &si_req_payload_size, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SIRM_REQ_LEADER_SIZE,
                                sizeof (si_req_leader_size), &si_req_leader_size, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SIRM_REQ_TRAILER_SIZE,
                                sizeof (si_req_trailer_size), &si_req_trailer_size, NULL);

	alignment = 1 << ((si_info & ARV_SIRM_INFO_ALIGNMENT_MASK) >> ARV_SIRM_INFO_ALIGNMENT_SHIFT);

	arv_info_stream ("SIRM_INFO             = 0x%08x", si_info);
	arv_info_stream ("SIRM_REQ_PAYLOAD_SIZE = 0x%016" G_GINT64_MODIFIER "x", si_req_payload_size);
	arv_info_stream ("SIRM_REQ_LEADER_SIZE  = 0x%08x", si_req_leader_size);
	arv_info_stream ("SIRM_REQ_TRAILER_SIZE = 0x%08x", si_req_trailer_size);

	arv_info_stream ("Required alignment    = %d", alignment);

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

	arv_device_write_memory (device, sirm_offset + ARV_SIRM_MAX_LEADER_SIZE,
                                 sizeof (si_req_leader_size), &si_req_leader_size, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SIRM_MAX_TRAILER_SIZE,
                                 sizeof (si_req_trailer_size), &si_req_trailer_size, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SIRM_PAYLOAD_SIZE,
                                 sizeof (si_payload_size), &si_payload_size, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SIRM_PAYLOAD_COUNT,
                                 sizeof (si_payload_count), &si_payload_count, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SIRM_TRANSFER1_SIZE,
                                 sizeof (si_transfer1_size), &si_transfer1_size, NULL);
	arv_device_write_memory (device, sirm_offset + ARV_SIRM_TRANSFER2_SIZE,
                                 sizeof (si_transfer2_size), &si_transfer2_size, NULL);

	arv_info_stream ("SIRM_PAYLOAD_SIZE     = 0x%08x", si_payload_size);
	arv_info_stream ("SIRM_PAYLOAD_COUNT    = 0x%08x", si_payload_count);
	arv_info_stream ("SIRM_TRANSFER1_SIZE   = 0x%08x", si_transfer1_size);
	arv_info_stream ("SIRM_TRANSFER2_SIZE   = 0x%08x", si_transfer2_size);
	arv_info_stream ("SIRM_MAX_LEADER_SIZE  = 0x%08x", si_req_leader_size);
	arv_info_stream ("SIRM_MAX_TRAILER_SIZE = 0x%08x", si_req_trailer_size);

	si_control = ARV_SIRM_CONTROL_STREAM_ENABLE;
	arv_device_write_memory (device, sirm_offset + ARV_SIRM_CONTROL, sizeof (si_control), &si_control, NULL);

	thread_data->leader_size = si_req_leader_size;
	thread_data->payload_size = si_payload_size;
	thread_data->trailer_size = si_req_trailer_size;
	thread_data->cancel = FALSE;

        switch (priv->usb_mode) {
                case ARV_UV_USB_MODE_SYNC:
                        priv->thread = g_thread_new ("arv_uv_stream", arv_uv_stream_thread_sync, priv->thread_data);
                        break;
                case ARV_UV_USB_MODE_ASYNC:
                        priv->thread = g_thread_new ("arv_uv_stream", arv_uv_stream_thread_async, priv->thread_data);
                        break;
                default:
                        g_assert_not_reached ();
        }
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
	g_cond_broadcast (&priv->thread_data->stream_event);
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
arv_uv_stream_new (ArvUvDevice *uv_device, ArvStreamCallback callback, void *callback_data,
                   ArvUvUsbMode usb_mode, GError **error)
{
	return g_initable_new (ARV_TYPE_UV_STREAM, NULL, error,
			       "device", uv_device,
			       "callback", callback,
			       "callback-data", callback_data,
			       "usb-mode", usb_mode,
			       NULL);
}

static void
arv_uv_stream_constructed (GObject *object)
{
	ArvUvStream *uv_stream = ARV_UV_STREAM (object);
	ArvStream *stream = ARV_STREAM (uv_stream);
	ArvUvStreamPrivate *priv = arv_uv_stream_get_instance_private (uv_stream);
	ArvUvStreamThreadData *thread_data;

        G_OBJECT_CLASS (arv_uv_stream_parent_class)->constructed (object);

	thread_data = g_new0 (ArvUvStreamThreadData, 1);
	thread_data->stream = stream;

	g_cond_init( &thread_data->stream_event );
	g_mutex_init( &thread_data->stream_mtx );

	thread_data->statistics.n_completed_buffers = 0;
	thread_data->statistics.n_failures = 0;
	thread_data->statistics.n_underruns = 0;
	thread_data->statistics.n_transferred_bytes = 0;
	thread_data->statistics.n_ignored_bytes = 0;

	g_object_get (object,
		      "device", &thread_data->uv_device,
		      "callback", &thread_data->callback,
		      "callback-data", &thread_data->callback_data,
		      NULL);

	priv->thread_data = thread_data;

        arv_stream_declare_info (ARV_STREAM (uv_stream), "n_completed_buffers",
                                 G_TYPE_UINT64, &thread_data->statistics.n_completed_buffers);
        arv_stream_declare_info (ARV_STREAM (uv_stream), "n_failures",
                                 G_TYPE_UINT64, &thread_data->statistics.n_failures);
        arv_stream_declare_info (ARV_STREAM (uv_stream), "n_underruns",
                                 G_TYPE_UINT64, &thread_data->statistics.n_underruns);
        arv_stream_declare_info (ARV_STREAM (uv_stream), "n_transferred_bytes",
                                 G_TYPE_UINT64, &thread_data->statistics.n_transferred_bytes);
        arv_stream_declare_info (ARV_STREAM (uv_stream), "n_ignored_bytes",
                                 G_TYPE_UINT64, &thread_data->statistics.n_ignored_bytes);

        arv_uv_stream_start_thread (ARV_STREAM (uv_stream));
}

/* ArvStream implementation */

static void
arv_uv_stream_set_property (GObject * object, guint prop_id,
                            const GValue * value, GParamSpec * pspec)
{
       ArvUvStreamPrivate *priv = arv_uv_stream_get_instance_private (ARV_UV_STREAM (object));

       switch (prop_id) {
               case ARV_UV_STREAM_PROPERTY_USB_MODE:
                       priv->usb_mode = g_value_get_enum(value);
                       break;
               default:
                       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                       break;
       }
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

		arv_info_stream ("[UvStream::finalize] n_completed_buffers    = %" G_GUINT64_FORMAT,
				  thread_data->statistics.n_completed_buffers);
		arv_info_stream ("[UvStream::finalize] n_failures             = %" G_GUINT64_FORMAT,
				  thread_data->statistics.n_failures);
		arv_info_stream ("[UvStream::finalize] n_underruns            = %" G_GUINT64_FORMAT,
				  thread_data->statistics.n_underruns);

		arv_info_stream ("[UvStream::finalize] n_transferred_bytes    = %" G_GUINT64_FORMAT,
				  thread_data->statistics.n_transferred_bytes);
		arv_info_stream ("[UvStream::finalize] n_ignored_bytes        = %" G_GUINT64_FORMAT,
				  thread_data->statistics.n_ignored_bytes);

		g_mutex_clear (&thread_data->stream_mtx);
		g_cond_clear (&thread_data->stream_event);

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
	object_class->set_property = arv_uv_stream_set_property;

	stream_class->start_thread = arv_uv_stream_start_thread;
	stream_class->stop_thread = arv_uv_stream_stop_thread;

         /**
          * ArvUvStream:usb-mode:
          *
          * USB device I/O mode.
          */
        g_object_class_install_property (
                object_class, ARV_UV_STREAM_PROPERTY_USB_MODE,
                g_param_spec_enum ("usb-mode", "USB mode",
                                   "USB device I/O mode",
                                   ARV_TYPE_UV_USB_MODE,
                                   ARV_UV_USB_MODE_DEFAULT,
				   G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)
		);

}
