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

#define ARV_UV_STREAM_POP_INPUT_BUFFER_TIMEOUT_MS       10
#define ARV_UV_STREAM_TRANSFER_WAIT_TIMEOUT_MS          10

enum {
       ARV_UV_STREAM_PROPERTY_0,
       ARV_UV_STREAM_PROPERTY_USB_MODE
} ArvUvStreamProperties;

/* Acquisition thread */

typedef struct {
        guint64 n_completed_buffers;
        guint64 n_failures;
        guint64 n_underruns;
        guint64 n_aborted;

        guint64 n_transferred_bytes;
        guint64 n_ignored_bytes;
} ArvStreamStatistics;

typedef struct {
	ArvStream *stream;

        gboolean thread_started;
        GMutex thread_started_mutex;
        GCond thread_started_cond;

	ArvUvDevice *uv_device;
	ArvStreamCallback callback;
	void *callback_data;

        size_t expected_size;
	size_t leader_size;
	size_t payload_size;
        guint32 payload_count;
        size_t transfer1_size;
	size_t trailer_size;

	gboolean cancel;

	/* Notification for completed transfers and cancellation */
	GMutex stream_mtx;
	GCond stream_event;

	/* Statistics */
	ArvStreamStatistics statistics;

        gint n_buffer_in_use;
} ArvUvStreamThreadData;

typedef struct {
	GThread *thread;
	ArvUvStreamThreadData *thread_data;
	ArvUvUsbMode usb_mode;

        guint64 sirm_address;
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

        ArvStreamCallback callback;
        gpointer callback_data;

	GMutex* transfer_completed_mtx;
	GCond* transfer_completed_event;

	size_t total_payload_transferred;
        size_t expected_size;

	guint8 *leader_buffer, *trailer_buffer;

	int num_payload_transfers;
	struct libusb_transfer *leader_transfer, *trailer_transfer, **payload_transfers;

	guint num_submitted;

	gint *total_submitted_bytes;

        gboolean is_aborting;

	ArvStreamStatistics *statistics;

        gint *n_buffer_in_use;
} ArvUvStreamBufferContext;

G_DEFINE_TYPE_WITH_CODE (ArvUvStream, arv_uv_stream, ARV_TYPE_STREAM, G_ADD_PRIVATE (ArvUvStream))

static void
arv_uv_stream_buffer_context_wait_transfer_completed (ArvUvStreamBufferContext* ctx, gint64 timeout_ms)
{
	g_mutex_lock( ctx->transfer_completed_mtx );

        if (timeout_ms > 0) {
                gint64 end_time;

                end_time = g_get_monotonic_time() + timeout_ms * G_TIME_SPAN_MILLISECOND;
                g_cond_wait_until (ctx->transfer_completed_event, ctx->transfer_completed_mtx, end_time);
        } else {
                g_cond_wait( ctx->transfer_completed_event, ctx->transfer_completed_mtx );
        }

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
void LIBUSB_CALL arv_uv_stream_leader_cb (struct libusb_transfer *transfer)
{
	ArvUvStreamBufferContext *ctx = transfer->user_data;
	ArvUvspPacket *packet = (ArvUvspPacket*)transfer->buffer;

        if (ctx->buffer != NULL) {
                if (ctx->is_aborting) {
                        ctx->buffer->priv->status = ARV_BUFFER_STATUS_ABORTED;
                } else {
                        switch (transfer->status) {
                                case LIBUSB_TRANSFER_COMPLETED:
                                        arv_uvsp_packet_debug (packet, ARV_DEBUG_LEVEL_DEBUG);

                                if (arv_uvsp_packet_get_packet_type (packet) != ARV_UVSP_PACKET_TYPE_LEADER) {
                                        arv_warning_stream_thread ("Unexpected packet type (was expecting leader packet)");
                                        ctx->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
                                        break;
                                }

                                ctx->buffer->priv->system_timestamp_ns = g_get_real_time () * 1000LL;
                                ctx->buffer->priv->payload_type = arv_uvsp_packet_get_buffer_payload_type
                                        (packet, &ctx->buffer->priv->has_chunks);
                                ctx->buffer->priv->chunk_endianness = G_LITTLE_ENDIAN;
                                if (ctx->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE ||
                                    ctx->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA ||
									ctx->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_GENDC_CONTAINER ) {
                                        arv_buffer_set_n_parts(ctx->buffer, 1);
                                        ctx->buffer->priv->parts[0].data_offset = 0;
                                        ctx->buffer->priv->parts[0].component_id = 0;
                                        ctx->buffer->priv->parts[0].data_type = ARV_BUFFER_PART_DATA_TYPE_2D_IMAGE;
                                        ctx->buffer->priv->parts[0].pixel_format = arv_uvsp_packet_get_pixel_format (packet);
                                        arv_uvsp_packet_get_region (packet,
                                                                    &ctx->buffer->priv->parts[0].width,
                                                                    &ctx->buffer->priv->parts[0].height,
                                                                    &ctx->buffer->priv->parts[0].x_offset,
                                                                    &ctx->buffer->priv->parts[0].y_offset,
                                                                    &ctx->buffer->priv->parts[0].x_padding,
                                                                    &ctx->buffer->priv->parts[0].y_padding);
                                }
                                ctx->buffer->priv->frame_id = arv_uvsp_packet_get_frame_id (packet);
                                ctx->buffer->priv->timestamp_ns = arv_uvsp_packet_get_timestamp (packet);
                                break;
                        default:
                                arv_warning_stream_thread ("Leader transfer failed (%s)",
                                                           libusb_error_name (transfer->status));
                                ctx->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
                                break;
                        }
                }
        }

	g_atomic_int_dec_and_test (&ctx->num_submitted);
	g_atomic_int_add (ctx->total_submitted_bytes, -transfer->length);
	ctx->statistics->n_transferred_bytes += transfer->length;
	arv_uv_stream_buffer_context_notify_transfer_completed (ctx);
}

static
void LIBUSB_CALL arv_uv_stream_payload_cb (struct libusb_transfer *transfer)
{
	ArvUvStreamBufferContext *ctx = transfer->user_data;
	int component_count;

        if (ctx->buffer != NULL) {
                if (ctx->is_aborting) {
                        ctx->buffer->priv->status = ARV_BUFFER_STATUS_ABORTED;
                } else {
                        switch (transfer->status) {
                                case LIBUSB_TRANSFER_COMPLETED:
                                        ctx->total_payload_transferred += transfer->actual_length;

										if (ctx->buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_GENDC_CONTAINER){
											if(!arv_uvsp_packet_is_gendc (ctx->buffer->priv->data)){
												arv_warning_sp ("Invalid GenDC Container: Signature shows %.4s which is supposed to be GNDC", ctx->buffer->priv->data);
											}else{
												ctx->buffer->priv->has_gendc = TRUE;
												ctx->buffer->priv->gendc_data_offset = arv_uvsp_packet_get_gendc_dataoffset(ctx->buffer->priv->data);
												ctx->buffer->priv->gendc_descriptor_size = arv_uvsp_packet_get_gendc_descriptorsize(ctx->buffer->priv->data);
												ctx->buffer->priv->gendc_data_size = arv_uvsp_packet_get_gendc_datasize(ctx->buffer->priv->data);
												component_count = (int) arv_uvsp_packet_get_gendc_componentcount(ctx->buffer->priv->data);
												
												for(int ith_component = 0; ith_component < component_count; ++ith_component){
													int64_t ith_component_offset = arv_uvsp_packet_get_gendc_componentoffset(ctx->buffer->priv->data, ith_component);

													if (arv_uvsp_packet_get_gendc_iscomponentvalid(ctx->buffer->priv->data + ith_component_offset)
														&& arv_uvsp_packet_get_gendc_componenttypeid(ctx->buffer->priv->data + ith_component_offset) == 0x1 ){

														guint64 partoffset = arv_uvsp_packet_get_gendc_partoffset(ctx->buffer->priv->data + ith_component_offset, 0);

														ctx->buffer->priv->parts[0].data_offset = arv_uvsp_packet_get_gendc_partdatapffset(ctx->buffer->priv->data + partoffset);
														ctx->buffer->priv->parts[0].component_id = ith_component;
														ctx->buffer->priv->parts[0].data_type = ARV_BUFFER_PART_DATA_TYPE_2D_IMAGE;
														ctx->buffer->priv->parts[0].pixel_format = arv_uvsp_packet_get_gendc_componentpixelformat(ctx->buffer->priv->data + ith_component_offset);
														ctx->buffer->priv->parts[0].width = arv_uvsp_packet_get_gendc_partdimension_x(ctx->buffer->priv->data + partoffset);
														ctx->buffer->priv->parts[0].width = arv_uvsp_packet_get_gendc_partdimension_y(ctx->buffer->priv->data + partoffset);
														ctx->buffer->priv->parts[0].x_offset = 0;
														ctx->buffer->priv->parts[0].y_offset = 0;
														ctx->buffer->priv->parts[0].x_padding = arv_uvsp_packet_get_gendc_partpadding_x(ctx->buffer->priv->data + partoffset);
														ctx->buffer->priv->parts[0].y_padding = arv_uvsp_packet_get_gendc_partpadding_y(ctx->buffer->priv->data + partoffset);

														break;
													}
												}
											}
										}
                                        break;
                                default:
                                        arv_warning_stream_thread ("Payload transfer failed (%s)",
                                                                   libusb_error_name (transfer->status));
                                        ctx->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
                                        break;
                        }
                }
        }

	g_atomic_int_dec_and_test( &ctx->num_submitted );
	g_atomic_int_add (ctx->total_submitted_bytes, -transfer->length);
	ctx->statistics->n_transferred_bytes += transfer->length;
	arv_uv_stream_buffer_context_notify_transfer_completed (ctx);
}

static
void LIBUSB_CALL arv_uv_stream_trailer_cb (struct libusb_transfer *transfer)
{
	ArvUvStreamBufferContext *ctx = transfer->user_data;
	ArvUvspPacket *packet = (ArvUvspPacket*)transfer->buffer;

        if (ctx->buffer != NULL) {
                if (ctx->is_aborting) {
                        ctx->buffer->priv->status = ARV_BUFFER_STATUS_ABORTED;
                        ctx->statistics->n_aborted += 1;
                } else {
                        switch (transfer->status) {
                                case LIBUSB_TRANSFER_COMPLETED:
                                        arv_uvsp_packet_debug (packet, ARV_DEBUG_LEVEL_DEBUG);

                                        if (arv_uvsp_packet_get_packet_type (packet) != ARV_UVSP_PACKET_TYPE_TRAILER) {
                                                arv_warning_stream_thread ("Unexpected packet type (was expecting trailer packet)");
                                                ctx->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
                                                break;
                                        }

                                        arv_debug_stream_thread ("Total payload: %zu bytes", ctx->total_payload_transferred);
                                        if (ctx->total_payload_transferred != ctx->expected_size) {
                                                arv_warning_stream_thread ("Unexpected total payload size (received %"
                                                                           G_GSIZE_FORMAT " - expected %" G_GSIZE_FORMAT")",
                                                                           ctx->total_payload_transferred, ctx->expected_size);
                                                ctx->buffer->priv->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
                                                break;
                                        }

                                        break;
                                default:
                                        arv_warning_stream_thread ("Trailer transfer failed (%s)",
                                                                   libusb_error_name(transfer->status));
                                        ctx->buffer->priv->status = ARV_BUFFER_STATUS_MISSING_PACKETS;
                                        break;
                        }

                        switch (ctx->buffer->priv->status) {
                                case ARV_BUFFER_STATUS_FILLING:
                                        ctx->buffer->priv->status = ARV_BUFFER_STATUS_SUCCESS;
                                        ctx->buffer->priv->received_size = ctx->total_payload_transferred;
                                        ctx->buffer->priv->parts[0].size = ctx->total_payload_transferred;
                                        ctx->statistics->n_completed_buffers += 1;
                                        break;
                                default:
                                        ctx->statistics->n_failures += 1;
                                        break;
                        }
                }

                arv_stream_push_output_buffer (ctx->stream, ctx->buffer);
                if (ctx->callback != NULL)
                        ctx->callback (ctx->callback_data,
                                       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
                                       ctx->buffer);
                g_atomic_int_dec_and_test(ctx->n_buffer_in_use);
                ctx->buffer = NULL;
        }

	g_atomic_int_dec_and_test( &ctx->num_submitted );
	g_atomic_int_add (ctx->total_submitted_bytes, -transfer->length);
	ctx->statistics->n_transferred_bytes += transfer->length;
	arv_uv_stream_buffer_context_notify_transfer_completed (ctx);
}

static ArvUvStreamBufferContext*
arv_uv_stream_buffer_context_new (ArvBuffer *buffer, ArvUvStreamThreadData *thread_data, gint *total_submitted_bytes)
{
	ArvUvStreamBufferContext* ctx = g_malloc0 (sizeof(ArvUvStreamBufferContext));
	int i;
	size_t offset = 0;

	ctx->buffer = NULL;
	ctx->stream = thread_data->stream;
        ctx->callback = thread_data->callback;
        ctx->callback_data = thread_data->callback_data;
	ctx->transfer_completed_mtx = &thread_data->stream_mtx;
	ctx->transfer_completed_event = &thread_data->stream_event;
        ctx->n_buffer_in_use = &thread_data->n_buffer_in_use;

	ctx->leader_buffer = g_malloc (thread_data->leader_size);
	ctx->leader_transfer = libusb_alloc_transfer (0);
	arv_uv_device_fill_bulk_transfer (ctx->leader_transfer, thread_data->uv_device,
		ARV_UV_ENDPOINT_DATA, LIBUSB_ENDPOINT_IN,
		ctx->leader_buffer, thread_data->leader_size,
		arv_uv_stream_leader_cb, ctx,
		0);

	ctx->num_payload_transfers = (buffer->priv->allocated_size - 1) / thread_data->payload_size + 1;
	ctx->payload_transfers = g_malloc (ctx->num_payload_transfers * sizeof(struct libusb_transfer*));

	for (i = 0; i < ctx->num_payload_transfers; ++i) {
		size_t size = MIN (thread_data->payload_size, buffer->priv->allocated_size - offset);

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
        g_free (ctx->payload_transfers);
	g_free (ctx->trailer_buffer);

        if (ctx->buffer != NULL) {
                ctx->buffer->priv->status = ARV_BUFFER_STATUS_ABORTED;
                arv_stream_push_output_buffer (ctx->stream, ctx->buffer);
                if (ctx->callback != NULL)
                        ctx->callback (ctx->callback_data,
                                       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
                                       ctx->buffer);
                g_atomic_int_dec_and_test(ctx->n_buffer_in_use);
                ctx->buffer = NULL;
        }

	g_free (ctx);
}

static void
_submit_transfer (ArvUvStreamBufferContext* ctx, struct libusb_transfer* transfer, gboolean* cancel)
{
	while (!g_atomic_int_get (cancel) &&
               ((g_atomic_int_get(ctx->total_submitted_bytes) + transfer->length) > ARV_UV_STREAM_MAXIMUM_SUBMIT_TOTAL)) {
		arv_uv_stream_buffer_context_wait_transfer_completed (ctx, ARV_UV_STREAM_TRANSFER_WAIT_TIMEOUT_MS);
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
			arv_uv_stream_buffer_context_wait_transfer_completed (ctx, ARV_UV_STREAM_TRANSFER_WAIT_TIMEOUT_MS);
			break;

		default:
			arv_warning_stream_thread ("libusb_submit_transfer failed (%d)", status);
			return;
		}
	}
}

static void
arv_uv_stream_buffer_context_submit (ArvUvStreamBufferContext* ctx, ArvBuffer *buffer, ArvUvStreamThreadData *thread_data)
{
	int i;

        if (ctx->callback != NULL)
                ctx->callback (ctx->callback_data,
                               ARV_STREAM_CALLBACK_TYPE_START_BUFFER,
                               buffer);

        ctx->buffer = buffer;
        ctx->total_payload_transferred = 0;
        buffer->priv->status = ARV_BUFFER_STATUS_FILLING;

        ctx->expected_size = thread_data->expected_size;

        _submit_transfer (ctx, ctx->leader_transfer, &thread_data->cancel);

        for (i = 0; i < ctx->num_payload_transfers; ++i) {
                _submit_transfer (ctx, ctx->payload_transfers[i], &thread_data->cancel);
        }

        _submit_transfer (ctx, ctx->trailer_transfer, &thread_data->cancel);
}

static void
arv_uv_stream_buffer_context_cancel (gpointer key, gpointer value, gpointer user_data)
{
	ArvUvStreamBufferContext* ctx = value;
	int i;

        ctx->is_aborting = TRUE;

	libusb_cancel_transfer (ctx->leader_transfer );

	for (i = 0; i < ctx->num_payload_transfers; ++i) {
		libusb_cancel_transfer (ctx->payload_transfers[i]);
	}

	libusb_cancel_transfer (ctx->trailer_transfer);

	while (ctx->num_submitted > 0)
	{
		arv_uv_stream_buffer_context_wait_transfer_completed (ctx, ARV_UV_STREAM_TRANSFER_WAIT_TIMEOUT_MS);
	}
}

static void *
arv_uv_stream_thread_async (void *data)
{
	ArvUvStreamThreadData *thread_data = data;
	ArvBuffer *buffer = NULL;
	GHashTable *ctx_lookup;
	gint total_submitted_bytes = 0;

	arv_info_stream_thread ("Start async USB3Vision stream thread");

	arv_debug_stream_thread ("leader_size = %zu", thread_data->leader_size );
	arv_debug_stream_thread ("payload_size = %zu", thread_data->payload_size );
	arv_debug_stream_thread ("trailer_size = %zu", thread_data->trailer_size );

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

	ctx_lookup = g_hash_table_new_full( g_direct_hash, g_direct_equal, NULL, arv_uv_stream_buffer_context_free );

        g_mutex_lock (&thread_data->thread_started_mutex);
        thread_data->thread_started = TRUE;
        g_cond_signal (&thread_data->thread_started_cond);
        g_mutex_unlock (&thread_data->thread_started_mutex);

	while (!g_atomic_int_get (&thread_data->cancel) &&
               arv_uv_device_is_connected (thread_data->uv_device)) {
		ArvUvStreamBufferContext* ctx;

                buffer = arv_stream_timeout_pop_input_buffer (thread_data->stream,
                                                              ARV_UV_STREAM_POP_INPUT_BUFFER_TIMEOUT_MS * 1000);

		if( buffer == NULL ) {
                        if (thread_data->n_buffer_in_use == 0)
                                thread_data->statistics.n_underruns += 1;
                        /* NOTE: n_ignored_bytes is not accumulated because it doesn't submit next USB transfer if
                         * buffer is shortage. It means back pressure might be hanlded by USB slave side. */
			continue;
		} else {
                        g_atomic_int_inc(&thread_data->n_buffer_in_use);
                }

		ctx = g_hash_table_lookup( ctx_lookup, buffer );
		if (!ctx) {
			arv_debug_stream_thread ("Stream buffer context not found for buffer %p, creating...", buffer);

			ctx = arv_uv_stream_buffer_context_new (buffer, thread_data, &total_submitted_bytes);

			g_hash_table_insert (ctx_lookup, buffer, ctx);
		}

                arv_uv_stream_buffer_context_submit (ctx, buffer, thread_data);
	}

	g_hash_table_foreach (ctx_lookup, arv_uv_stream_buffer_context_cancel, NULL);

	g_hash_table_destroy (ctx_lookup);

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

	arv_info_stream_thread ("Stop USB3Vision async stream thread");

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
	int component_count;

	arv_info_stream_thread ("Start sync USB3Vision stream thread");

	incoming_buffer = g_malloc (ARV_UV_STREAM_MAXIMUM_TRANSFER_SIZE);

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

	offset = 0;

        g_mutex_lock (&thread_data->thread_started_mutex);
        thread_data->thread_started = TRUE;
        g_cond_signal (&thread_data->thread_started_cond);
        g_mutex_unlock (&thread_data->thread_started_mutex);

	while (!g_atomic_int_get (&thread_data->cancel)) {
		GError *error = NULL;
		size_t size;
		transferred = 0;

		if (buffer == NULL)
			size = ARV_UV_STREAM_MAXIMUM_TRANSFER_SIZE;
		else {
			if (offset < buffer->priv->allocated_size)
				size = MIN (thread_data->payload_size, buffer->priv->allocated_size - offset);
			else
				size = thread_data->trailer_size;
		}

		/* Avoid unnecessary memory copy by transferring data directly to the image buffer */
		if (buffer != NULL &&
		    buffer->priv->status == ARV_BUFFER_STATUS_FILLING &&
		    offset + size <= buffer->priv->allocated_size)
			packet = (ArvUvspPacket *) (buffer->priv->data + offset);
		else
			packet = incoming_buffer;

		arv_debug_sp ("Asking for %" G_GSIZE_FORMAT " bytes", size);
		arv_uv_device_bulk_transfer (thread_data->uv_device,  ARV_UV_ENDPOINT_DATA, LIBUSB_ENDPOINT_IN,
					     packet, size, &transferred, 0, &error);

		if (error != NULL) {
			arv_warning_sp ("USB transfer error: %s", error->message);
			g_clear_error (&error);
                } else if (transferred < 1) {
			arv_warning_sp ("No data transferred");
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
                                                g_atomic_int_dec_and_test(&thread_data->n_buffer_in_use);
						buffer = NULL;
					}
					buffer = arv_stream_pop_input_buffer (thread_data->stream);
					if (buffer != NULL) {
                                                g_atomic_int_inc(&thread_data->n_buffer_in_use);
						buffer->priv->system_timestamp_ns = g_get_real_time () * 1000LL;
						buffer->priv->status = ARV_BUFFER_STATUS_FILLING;
                                                buffer->priv->received_size = 0;
						buffer->priv->payload_type = arv_uvsp_packet_get_buffer_payload_type
                                                        (packet, &buffer->priv->has_chunks);
						buffer->priv->chunk_endianness = G_LITTLE_ENDIAN;
						if (buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE ||
						    buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA ||
							buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_GENDC_CONTAINER) {
                                                        arv_buffer_set_n_parts(buffer, 1);
                                                        buffer->priv->parts[0].data_offset = 0;
                                                        buffer->priv->parts[0].component_id = 0;
                                                        buffer->priv->parts[0].data_type =
                                                                ARV_BUFFER_PART_DATA_TYPE_2D_IMAGE;
							buffer->priv->parts[0].pixel_format =
                                                                arv_uvsp_packet_get_pixel_format (packet);
							arv_uvsp_packet_get_region (packet,
										    &buffer->priv->parts[0].width,
										    &buffer->priv->parts[0].height,
										    &buffer->priv->parts[0].x_offset,
										    &buffer->priv->parts[0].y_offset,
                                                                                    &buffer->priv->parts[0].x_padding,
                                                                                    &buffer->priv->parts[0].y_padding);
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
						arv_debug_stream_thread ("Received %" G_GUINT64_FORMAT " bytes",
								       offset);

                                                if (offset != thread_data->expected_size) {
                                                        arv_info_stream_thread ("Incomplete image received, dropping "
                                                                                "(received %" G_GUINT64_FORMAT
                                                                                " / expected %" G_GSIZE_FORMAT ")",
                                                                                offset, thread_data->expected_size);

                                                       buffer->priv->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
                                                       arv_stream_push_output_buffer (thread_data->stream, buffer);
                                                       if (thread_data->callback != NULL)
                                                               thread_data->callback (thread_data->callback_data,
                                                                                      ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
                                                                                      buffer);
                                                       thread_data->statistics.n_failures++;
                                                       thread_data->statistics.n_ignored_bytes += transferred;
                                                        g_atomic_int_dec_and_test(&thread_data->n_buffer_in_use);
                                                       buffer = NULL;
                                                } else {
                                                        buffer->priv->status = ARV_BUFFER_STATUS_SUCCESS;
                                                        buffer->priv->received_size = offset;
                                                        buffer->priv->parts[0].size = offset;
                                                        arv_stream_push_output_buffer (thread_data->stream, buffer);
                                                        if (thread_data->callback != NULL)
                                                                thread_data->callback (thread_data->callback_data,
                                                                                       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
                                                                                       buffer);
                                                        thread_data->statistics.n_completed_buffers++;
                                                        thread_data->statistics.n_transferred_bytes += transferred;
                                                        g_atomic_int_dec_and_test(&thread_data->n_buffer_in_use);
                                                        buffer = NULL;
                                                }
                                        }
                                        break;
                                case ARV_UVSP_PACKET_TYPE_DATA:
                                        if (buffer != NULL && buffer->priv->status == ARV_BUFFER_STATUS_FILLING) {
                                                if (offset + transferred <= buffer->priv->allocated_size) {
                                                        if (packet == incoming_buffer)
                                                                memcpy (((char *) buffer->priv->data) + offset,
                                                                        packet, transferred);
                                                        offset += transferred;
                                                        thread_data->statistics.n_transferred_bytes += transferred;

														if (buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_GENDC_CONTAINER){
															if(!arv_uvsp_packet_is_gendc (buffer->priv->data)){
																arv_warning_sp ("Invalid GenDC Container: Signature shows %.4s which is supposed to be GNDC", buffer->priv->data);      
															}else{
																buffer->priv->has_gendc = TRUE;
																buffer->priv->gendc_data_offset = arv_uvsp_packet_get_gendc_dataoffset(buffer->priv->data);
																buffer->priv->gendc_descriptor_size = arv_uvsp_packet_get_gendc_descriptorsize(buffer->priv->data);
																buffer->priv->gendc_data_size = arv_uvsp_packet_get_gendc_datasize(buffer->priv->data);

																component_count = (int) arv_uvsp_packet_get_gendc_componentcount(buffer->priv->data);
																for(int ith_component = 0; ith_component < component_count; ++ith_component){
																	int64_t ith_component_offset = arv_uvsp_packet_get_gendc_componentoffset(buffer->priv->data, ith_component);    

																	// only if the component is valid and have an image data (GDC_INTENSITY from SFNC)
																	if (arv_uvsp_packet_get_gendc_iscomponentvalid(buffer->priv->data + ith_component_offset)
																	&& arv_uvsp_packet_get_gendc_componenttypeid(buffer->priv->data + ith_component_offset) == 0x1 ){       
																		guint64 partoffset = arv_uvsp_packet_get_gendc_partoffset(buffer->priv->data + ith_component_offset, 0);
																		buffer->priv->parts[0].data_offset = arv_uvsp_packet_get_gendc_partdatapffset(buffer->priv->data + partoffset);
																		buffer->priv->parts[0].component_id = ith_component;
																		buffer->priv->parts[0].data_type = ARV_BUFFER_PART_DATA_TYPE_2D_IMAGE;
																		buffer->priv->parts[0].pixel_format = arv_uvsp_packet_get_gendc_componentpixelformat(buffer->priv->data + ith_component_offset);
																		buffer->priv->parts[0].width = arv_uvsp_packet_get_gendc_partdimension_x(buffer->priv->data + partoffset);
																		buffer->priv->parts[0].width = arv_uvsp_packet_get_gendc_partdimension_y(buffer->priv->data + partoffset);
																		buffer->priv->parts[0].x_offset = 0;
																		buffer->priv->parts[0].y_offset = 0;
																		buffer->priv->parts[0].x_padding = arv_uvsp_packet_get_gendc_partpadding_x(buffer->priv->data + partoffset);
																		buffer->priv->parts[0].y_padding = arv_uvsp_packet_get_gendc_partpadding_y(buffer->priv->data + partoffset);

																		break;
																	}
																}
															}
														}

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
                thread_data->statistics.n_aborted++;
		arv_stream_push_output_buffer (thread_data->stream, buffer);
		if (thread_data->callback != NULL)
			thread_data->callback (thread_data->callback_data,
					       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
					       buffer);
                g_atomic_int_dec_and_test(&thread_data->n_buffer_in_use);
	}

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

	g_free (incoming_buffer);

	arv_info_stream_thread ("Stop USB3Vision sync stream thread");

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
        GError *error = NULL;
        guint64 sbrm_address;
	guint32 si_info;
	guint64 si_req_payload_size;
	guint32 si_req_leader_size;
	guint32 si_req_trailer_size;
	guint32 si_payload_size;
	guint32 si_payload_count;
	guint32 si_transfer1_size;
	guint32 si_transfer2_size;
	guint32 si_leader_size;
	guint32 si_trailer_size;
	guint32 si_control;
	guint32 alignment;
	guint32 aligned_maximum_transfer_size;

	g_return_if_fail (priv->thread == NULL);
	g_return_if_fail (priv->thread_data != NULL);

	thread_data = priv->thread_data;

	device = ARV_DEVICE (thread_data->uv_device);

	arv_device_read_memory (device, ARV_ABRM_SBRM_ADDRESS,
                                sizeof (guint64), &sbrm_address, NULL);
	arv_device_read_memory (device, sbrm_address + ARV_SBRM_SIRM_ADDRESS,
                                sizeof (guint64), &priv->sirm_address, NULL);

	arv_device_read_memory (device, priv->sirm_address + ARV_SIRM_INFO,
                                sizeof (si_info), &si_info, NULL);
	arv_device_read_memory (device, priv->sirm_address + ARV_SIRM_REQ_PAYLOAD_SIZE,
                                sizeof (si_req_payload_size), &si_req_payload_size, NULL);
	arv_device_read_memory (device, priv->sirm_address + ARV_SIRM_REQ_LEADER_SIZE,
                                sizeof (si_req_leader_size), &si_req_leader_size, NULL);
	arv_device_read_memory (device, priv->sirm_address + ARV_SIRM_REQ_TRAILER_SIZE,
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
		si_leader_size = aligned_maximum_transfer_size;
	} else {
		si_leader_size = align (si_req_leader_size, alignment);
	}

	if (si_req_trailer_size < 1) {
		arv_warning_stream ("Wrong SI_REQ_TRAILER_SIZE value, using %d instead", aligned_maximum_transfer_size);
		si_trailer_size = aligned_maximum_transfer_size;
	} else {
		si_trailer_size = align (si_req_trailer_size, alignment);
	}

	si_payload_size = MIN(si_req_payload_size , aligned_maximum_transfer_size);
	si_payload_count=  si_req_payload_size / si_payload_size;
	si_transfer1_size = align(si_req_payload_size % si_payload_size, alignment);
	si_transfer2_size = 0;

	arv_device_write_memory (device, priv->sirm_address + ARV_SIRM_MAX_LEADER_SIZE,
                                 sizeof (si_leader_size), &si_leader_size, NULL);
	arv_device_write_memory (device, priv->sirm_address + ARV_SIRM_MAX_TRAILER_SIZE,
                                 sizeof (si_trailer_size), &si_trailer_size, NULL);
	arv_device_write_memory (device, priv->sirm_address + ARV_SIRM_PAYLOAD_SIZE,
                                 sizeof (si_payload_size), &si_payload_size, NULL);
	arv_device_write_memory (device, priv->sirm_address + ARV_SIRM_PAYLOAD_COUNT,
                                 sizeof (si_payload_count), &si_payload_count, NULL);
	arv_device_write_memory (device, priv->sirm_address + ARV_SIRM_TRANSFER1_SIZE,
                                 sizeof (si_transfer1_size), &si_transfer1_size, NULL);
	arv_device_write_memory (device, priv->sirm_address + ARV_SIRM_TRANSFER2_SIZE,
                                 sizeof (si_transfer2_size), &si_transfer2_size, NULL);

	arv_info_stream ("SIRM_PAYLOAD_SIZE     = 0x%08x", si_payload_size);
	arv_info_stream ("SIRM_PAYLOAD_COUNT    = 0x%08x", si_payload_count);
	arv_info_stream ("SIRM_TRANSFER1_SIZE   = 0x%08x", si_transfer1_size);
	arv_info_stream ("SIRM_TRANSFER2_SIZE   = 0x%08x", si_transfer2_size);
	arv_info_stream ("SIRM_MAX_LEADER_SIZE  = 0x%08x", si_leader_size);
	arv_info_stream ("SIRM_MAX_TRAILER_SIZE = 0x%08x", si_trailer_size);

        thread_data->expected_size = si_req_payload_size;
	thread_data->leader_size = si_leader_size;
	thread_data->payload_size = si_payload_size;
        thread_data->payload_count = si_payload_count;
        thread_data->transfer1_size = si_transfer1_size;
	thread_data->trailer_size = si_trailer_size;
        thread_data->n_buffer_in_use = 0;
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

        g_mutex_lock (&thread_data->thread_started_mutex);
        while (!thread_data->thread_started)
                g_cond_wait (&thread_data->thread_started_cond,
                             &thread_data->thread_started_mutex);
        g_mutex_unlock (&thread_data->thread_started_mutex);

        arv_uv_device_reset_stream_endpoint (thread_data->uv_device);

        si_control = ARV_SIRM_CONTROL_STREAM_ENABLE;
        arv_device_write_memory (device, priv->sirm_address + ARV_SIRM_CONTROL, sizeof (si_control), &si_control, &error);
        if (error != NULL) {
                arv_warning_stream ("Failed to enable stream (%s)",
                                    error->message);
                g_clear_error(&error);
        }
}

static void
arv_uv_stream_stop_thread (ArvStream *stream)
{
	ArvUvStream *uv_stream = ARV_UV_STREAM (stream);
	ArvUvStreamPrivate *priv = arv_uv_stream_get_instance_private (uv_stream);
	ArvUvStreamThreadData *thread_data;
	guint32 si_control;
        GError *error = NULL;

	g_return_if_fail (priv->thread != NULL);
	g_return_if_fail (priv->thread_data != NULL);

	thread_data = priv->thread_data;

	g_atomic_int_set (&priv->thread_data->cancel, TRUE);
	g_cond_broadcast (&priv->thread_data->stream_event);
	g_thread_join (priv->thread);

	priv->thread = NULL;

	si_control = 0x0;
	arv_device_write_memory (ARV_DEVICE (thread_data->uv_device),
				 priv->sirm_address + ARV_SIRM_CONTROL, sizeof (si_control), &si_control, &error);
        if (error != NULL) {
                arv_warning_stream ("Failed to disable stream (%s)",
                                    error->message);
                g_clear_error(&error);
        }
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
arv_uv_stream_new (ArvUvDevice *uv_device, ArvStreamCallback callback, void *callback_data, GDestroyNotify destroy,
                   ArvUvUsbMode usb_mode, GError **error)
{
	return g_initable_new (ARV_TYPE_UV_STREAM, NULL, error,
			       "device", uv_device,
			       "callback", callback,
			       "callback-data", callback_data,
						 "destroy-notify", destroy,
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
        thread_data->statistics.n_aborted = 0;
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
        arv_stream_declare_info (ARV_STREAM (uv_stream), "n_aborted",
                                 G_TYPE_UINT64, &thread_data->statistics.n_aborted);
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
		arv_info_stream ("[UvStream::finalize] n_aborted              = %" G_GUINT64_FORMAT,
				  thread_data->statistics.n_aborted);

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
