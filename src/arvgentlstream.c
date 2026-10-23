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
 * Author: Xiaoqiang Wang <xiaoqiang.wang@psi.ch>
 */

/**
 * SECTION: arvgentlstream
 * @short_description: GenTL stream
 */

#include <arvdebugprivate.h>
#include <arvgentlsystemprivate.h>
#include <arvgentlstreamprivate.h>
#include <arvgentldeviceprivate.h>
#include <arvstreamprivate.h>
#include <arvbufferprivate.h>
#include <arvmiscprivate.h>
#include <arvdebug.h>
#include <stdio.h>

/* This structure allows to Revoke a GenTL buffer even after ArvStream object is finalized, which may happen if a buffer
 * is poped and never returned to the stream queues. We keep a DataStream handle and a reference to the parent device,
 * and attach a referenced pointer of this data to each buffer as "gentl-buffer-data", to be used in the buffer data
 * destroy callback. Once the stream and every buffer have been destroyed, the reference to the parent device will be
 * also released. */

typedef struct {
        gatomicrefcount ref_count;
	DS_HANDLE data_stream;
        ArvGenTLDevice *device;
} ArvGenTLDataStream;

static ArvGenTLDataStream *
arv_gentl_data_stream_new (ArvGenTLDevice *device)
{
        ArvGenTLDataStream *gentl_data_stream;

        g_return_val_if_fail(ARV_IS_GENTL_DEVICE(device), NULL);

        gentl_data_stream = g_new0 (ArvGenTLDataStream, 1);
        gentl_data_stream->data_stream = arv_gentl_device_open_stream_handle(device);
        gentl_data_stream->device = g_object_ref (device);
        g_atomic_ref_count_init (&gentl_data_stream->ref_count);

        return gentl_data_stream;
}

static ArvGenTLDataStream *
arv_gentl_data_stream_ref (ArvGenTLDataStream *gentl_data_stream)
{
        g_return_val_if_fail (gentl_data_stream != NULL, NULL);

        g_atomic_ref_count_inc (&gentl_data_stream->ref_count);

        return gentl_data_stream;
}

static void
arv_gentl_data_stream_unref (ArvGenTLDataStream *gentl_data_stream)
{
        g_return_if_fail(gentl_data_stream != NULL);

        if (g_atomic_ref_count_dec (&gentl_data_stream->ref_count)) {
                ArvGenTLSystem *gentl_system = arv_gentl_device_get_system(gentl_data_stream->device);
                ArvGenTLModule *gentl = arv_gentl_system_get_gentl(gentl_system);
                GC_ERROR error;

                arv_info_stream ("[GenTLDataStream::unref] close data stream");
                error = gentl->DSClose (gentl_data_stream->data_stream);
                if (error != GC_ERR_SUCCESS) {
                        arv_warning_stream ("[GenTLDataStream::unref] DSClose error (%s)",
                                            arv_gentl_gc_error_to_string(error));
                }

                g_clear_object (&gentl_data_stream->device);
                g_free (gentl_data_stream);
        }
}

enum
{
	PROP_0,
	PROP_STREAM_N_BUFFERS
};

typedef struct {
        ArvGenTLDataStream *gentl_data_stream;
        BUFFER_HANDLE gentl_buffer;
} ArvGenTLStreamBufferData;

typedef struct {
	ArvStream *stream;

	gboolean thread_started;
	GMutex thread_started_mutex;
	GCond thread_started_cond;

	ArvGenTLDevice *gentl_device;
	ArvStreamCallback callback;
	void *callback_data;

	GCancellable *cancellable;

	/* Statistics */
	guint64 n_completed_buffers;
	guint64 n_failures;
	guint64 n_underruns;

	guint64 n_transferred_bytes;

	/* Notification for completed transfers and cancellation */
	GMutex stream_mtx;
	GCond stream_event;
} ArvGenTLStreamThreadData;

typedef struct {
	ArvGenTLDevice *gentl_device;
        ArvGenTLDataStream *gentl_data_stream;
	EVENT_HANDLE event_handle;
	uint64_t timestamp_tick_frequency;

	guint n_buffers;

	GThread *thread;
	ArvGenTLStreamThreadData *thread_data;
} ArvGenTLStreamPrivate;

struct _ArvGenTLStream {
	ArvStream stream;
};

struct _ArvGenTLStreamClass {
	ArvStreamClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvGenTLStream, arv_gentl_stream, ARV_TYPE_STREAM, G_ADD_PRIVATE (ArvGenTLStream))

/* Acquisition thread */

static gboolean
_gentl_buffer_info_ptr(ArvGenTLModule *gentl, DS_HANDLE datastream, BUFFER_HANDLE buffer,
                       BUFFER_INFO_CMD info_cmd, void** value)
{
	GC_ERROR error;
	INFO_DATATYPE type;
	size_t size = sizeof(void*);
	*value = NULL;
	error = gentl->DSGetBufferInfo(datastream, buffer, info_cmd, &type, value, &size);
	return error == GC_ERR_SUCCESS;
}

static gboolean
_gentl_buffer_info_bool8(ArvGenTLModule *gentl, DS_HANDLE datastream, BUFFER_HANDLE buffer,
                         BUFFER_INFO_CMD info_cmd, bool8_t *value)
{
	GC_ERROR error;
	INFO_DATATYPE type;
	size_t size = sizeof(bool8_t);
	*value = 0;
	error = gentl->DSGetBufferInfo(datastream, buffer, info_cmd, &type, value, &size);
	return error == GC_ERR_SUCCESS;
}

static gboolean
_gentl_buffer_info_sizet(ArvGenTLModule *gentl, DS_HANDLE datastream, BUFFER_HANDLE buffer,
                         BUFFER_INFO_CMD info_cmd, size_t *value)
{
	GC_ERROR error;
	INFO_DATATYPE type;
	size_t size = sizeof(size_t);
	*value = 0;
	error = gentl->DSGetBufferInfo(datastream, buffer, info_cmd, &type, value, &size);
	return error == GC_ERR_SUCCESS;
}

static gboolean
_gentl_buffer_info_uint64(ArvGenTLModule *gentl, DS_HANDLE datastream, BUFFER_HANDLE buffer,
                          BUFFER_INFO_CMD info_cmd, uint64_t *value)
{
	GC_ERROR error;
	INFO_DATATYPE type;
	size_t size = sizeof(uint64_t);
	*value = 0;
	error = gentl->DSGetBufferInfo(datastream, buffer, info_cmd, &type, value, &size);
	return error == GC_ERR_SUCCESS;
}

static gboolean
_gentl_buffer_part_info_ptr(ArvGenTLModule *gentl, DS_HANDLE datastream, BUFFER_HANDLE buffer, uint32_t index,
                            BUFFER_INFO_CMD info_cmd, void** value)
{
	GC_ERROR error;
	INFO_DATATYPE type;
	size_t size = sizeof(void*);
	*value = NULL;
	error = gentl->DSGetBufferPartInfo(datastream, buffer, index, info_cmd, &type, value, &size);
	return error == GC_ERR_SUCCESS;
}

static gboolean
_gentl_buffer_part_info_sizet(ArvGenTLModule *gentl, DS_HANDLE datastream, BUFFER_HANDLE buffer, uint32_t index,
                              BUFFER_INFO_CMD info_cmd, size_t *value)
{
	GC_ERROR error;
	INFO_DATATYPE type;
	size_t size = sizeof(size_t);
	*value = 0;
	error = gentl->DSGetBufferPartInfo(datastream, buffer, index, info_cmd, &type, value, &size);
	return error == GC_ERR_SUCCESS;
}

static gboolean
_gentl_buffer_part_info_uint64(ArvGenTLModule *gentl, DS_HANDLE datastream, BUFFER_HANDLE buffer, uint32_t index,
                               BUFFER_INFO_CMD info_cmd, uint64_t *value)
{
	GC_ERROR error;
	INFO_DATATYPE type;
	size_t size = sizeof(uint64_t);
	*value = 0;
	error = gentl->DSGetBufferPartInfo(datastream, buffer, index, info_cmd, &type, value, &size);
	return error == GC_ERR_SUCCESS;
}

static void
_gentl_buffer_to_arv_buffer(ArvGenTLModule *gentl, DS_HANDLE datastream, BUFFER_HANDLE gentl_buffer,
                            ArvBuffer *arv_buffer, uint64_t timestamp_tick_frequency)
{
	size_t payload_type = PAYLOAD_TYPE_UNKNOWN;
	bool8_t has_chunks = FALSE;
	uint32_t num_parts;
	size_t data_size, data_type, actual_size, image_offset, width, height, x_offset, y_offset, x_padding, y_padding;
	uint64_t source_id, pixel_format, frame_id, timestamp = 0;
	void *data;

	_gentl_buffer_info_sizet(gentl, datastream, gentl_buffer, BUFFER_INFO_PAYLOADTYPE, &payload_type);
	switch(payload_type) {
		case PAYLOAD_TYPE_UNKNOWN:
			arv_buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_UNKNOWN;
			break;
		case PAYLOAD_TYPE_IMAGE:
			arv_buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_IMAGE;
			break;
		case PAYLOAD_TYPE_RAW_DATA:
			arv_buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_RAWDATA;
			break;
		case PAYLOAD_TYPE_FILE:
			arv_buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_FILE;
			break;
		case PAYLOAD_TYPE_CHUNK_DATA:
			arv_buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA;
			break;
		case PAYLOAD_TYPE_CHUNK_ONLY:
			arv_buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA;
			break;
		case PAYLOAD_TYPE_JPEG:
			arv_buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_JPEG;
			break;
		case PAYLOAD_TYPE_JPEG2000:
			arv_buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_JPEG2000;
			break;
		case PAYLOAD_TYPE_H264:
			arv_buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_H264;
			break;
		case PAYLOAD_TYPE_MULTI_PART:
			arv_buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_MULTIPART;
			break;
		default:
			arv_buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_UNKNOWN;
			arv_buffer->priv->status = ARV_BUFFER_STATUS_PAYLOAD_NOT_SUPPORTED;
			return;
	}

	_gentl_buffer_info_bool8(gentl, datastream, gentl_buffer, BUFFER_INFO_CONTAINS_CHUNKDATA, &has_chunks);
	arv_buffer->priv->has_chunks =
                has_chunks ||
                payload_type == PAYLOAD_TYPE_CHUNK_DATA ||
                payload_type == PAYLOAD_TYPE_CHUNK_ONLY;

	_gentl_buffer_info_uint64(gentl, datastream, gentl_buffer, BUFFER_INFO_FRAMEID, &frame_id);
	arv_buffer->priv->frame_id = frame_id;

	if (!_gentl_buffer_info_uint64(gentl, datastream, gentl_buffer, BUFFER_INFO_TIMESTAMP_NS, &timestamp)) {
		uint64_t timestamp_ticks = 0;
		if (timestamp_tick_frequency &&
		    _gentl_buffer_info_uint64(gentl, datastream, gentl_buffer, BUFFER_INFO_TIMESTAMP, &timestamp_ticks)) {
			timestamp = timestamp_ticks / timestamp_tick_frequency * 1000000000
				  + ((timestamp_ticks % timestamp_tick_frequency) * 1000000000) / timestamp_tick_frequency;
		} else {
			timestamp = g_get_real_time() * 1000LL;
		}
	}
	arv_buffer->priv->timestamp_ns = timestamp;

	_gentl_buffer_info_sizet(gentl, datastream, gentl_buffer, BUFFER_INFO_DATA_SIZE, &data_size);
	_gentl_buffer_info_sizet(gentl, datastream, gentl_buffer, BUFFER_INFO_SIZE_FILLED, &actual_size);

	arv_buffer->priv->received_size = actual_size;

	_gentl_buffer_info_ptr(gentl, datastream, gentl_buffer, BUFFER_INFO_BASE, &data);
        if (data != arv_buffer->priv->data)
                memcpy(arv_buffer->priv->data, data, data_size);

	if (payload_type == PAYLOAD_TYPE_CHUNK_ONLY) {
		arv_buffer_set_n_parts(arv_buffer, 0);
	} else if (gentl->DSGetNumBufferParts(datastream, gentl_buffer, &num_parts) == GC_ERR_SUCCESS) {
		arv_buffer_set_n_parts(arv_buffer, num_parts);
		for (uint32_t i=0; i<num_parts; i++) {
			void *part_data;
			size_t part_data_size;

			_gentl_buffer_part_info_uint64(gentl, datastream, gentl_buffer, i,
                                                       BUFFER_PART_INFO_SOURCE_ID, &source_id);
			arv_buffer->priv->parts[i].component_id = source_id;

			_gentl_buffer_part_info_sizet(gentl, datastream, gentl_buffer, i,
                                                      BUFFER_PART_INFO_DATA_TYPE, &data_type);
			arv_buffer->priv->parts[i].data_type = data_type;

			_gentl_buffer_part_info_sizet(gentl, datastream, gentl_buffer, i,
                                                      BUFFER_PART_INFO_DATA_SIZE, &part_data_size);
			arv_buffer->priv->parts[i].size = part_data_size;

			_gentl_buffer_part_info_ptr(gentl, datastream, gentl_buffer, i,
                                                    BUFFER_PART_INFO_BASE, &part_data);
			arv_buffer->priv->parts[i].data_offset = (char*)part_data - (char*)data;

			_gentl_buffer_part_info_uint64(gentl, datastream, gentl_buffer, i,
                                                       BUFFER_PART_INFO_DATA_FORMAT, &pixel_format);
			arv_buffer->priv->parts[i].pixel_format = pixel_format;

			_gentl_buffer_part_info_sizet(gentl, datastream, gentl_buffer, i,
                                                      BUFFER_PART_INFO_WIDTH, &width);
			arv_buffer->priv->parts[i].width = width;

			_gentl_buffer_part_info_sizet(gentl, datastream, gentl_buffer, i,
                                                      BUFFER_PART_INFO_HEIGHT, &height);
			arv_buffer->priv->parts[i].height = height;

			_gentl_buffer_part_info_sizet(gentl, datastream, gentl_buffer, i,
                                                      BUFFER_PART_INFO_XOFFSET, &x_offset);
			arv_buffer->priv->parts[i].x_offset = x_offset;

			_gentl_buffer_part_info_sizet(gentl, datastream, gentl_buffer, i,
                                                      BUFFER_PART_INFO_YOFFSET, &y_offset);
			arv_buffer->priv->parts[i].y_offset = y_offset;

			_gentl_buffer_part_info_sizet(gentl, datastream, gentl_buffer, i,
                                                      BUFFER_PART_INFO_XPADDING, &x_padding);
			arv_buffer->priv->parts[i].x_padding = x_padding;
		}
	} else {
		arv_buffer_set_n_parts(arv_buffer, 1);

		arv_buffer->priv->parts[0].component_id = 0;
		arv_buffer->priv->parts[0].data_type = ARV_BUFFER_PART_DATA_TYPE_2D_IMAGE;
		arv_buffer->priv->parts[0].size = data_size;

		_gentl_buffer_info_sizet(gentl, datastream, gentl_buffer, BUFFER_INFO_IMAGEOFFSET, &image_offset);
		arv_buffer->priv->parts[0].data_offset = image_offset;

		_gentl_buffer_info_uint64(gentl, datastream, gentl_buffer, BUFFER_INFO_PIXELFORMAT, &pixel_format);
		arv_buffer->priv->parts[0].pixel_format = pixel_format;

		_gentl_buffer_info_sizet(gentl, datastream, gentl_buffer, BUFFER_INFO_WIDTH, &width);
		arv_buffer->priv->parts[0].width = width;

		_gentl_buffer_info_sizet(gentl, datastream, gentl_buffer, BUFFER_INFO_HEIGHT, &height);
		arv_buffer->priv->parts[0].height = height;

		_gentl_buffer_info_sizet(gentl, datastream, gentl_buffer, BUFFER_INFO_XOFFSET, &x_offset);
		arv_buffer->priv->parts[0].x_offset = x_offset;

		_gentl_buffer_info_sizet(gentl, datastream, gentl_buffer, BUFFER_INFO_YOFFSET, &y_offset);
		arv_buffer->priv->parts[0].y_offset = y_offset;

		_gentl_buffer_info_sizet(gentl, datastream, gentl_buffer, BUFFER_INFO_XPADDING, &x_padding);
		arv_buffer->priv->parts[0].x_padding = x_padding;

		_gentl_buffer_info_sizet(gentl, datastream, gentl_buffer, BUFFER_INFO_YPADDING, &y_padding);
		arv_buffer->priv->parts[0].y_padding = y_padding;
	}

	arv_buffer->priv->status = ARV_BUFFER_STATUS_SUCCESS;
}

static void
_loop (ArvGenTLStreamThreadData *thread_data)
{
	ArvGenTLStreamPrivate *priv = arv_gentl_stream_get_instance_private (ARV_GENTL_STREAM (thread_data->stream));
	ArvGenTLSystem *gentl_system = arv_gentl_device_get_system(priv->gentl_device);
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(gentl_system);
        GHashTable *buffers;
        GHashTableIter iter;
        gpointer key, value;
	ArvBuffer *arv_buffer;
	BUFFER_HANDLE gentl_buffer;
	GC_ERROR error;

        buffers = g_hash_table_new (g_direct_hash, g_direct_equal);

	g_mutex_lock (&thread_data->thread_started_mutex);
	thread_data->thread_started = TRUE;
	g_cond_signal (&thread_data->thread_started_cond);
	g_mutex_unlock (&thread_data->thread_started_mutex);

	error = gentl->GCRegisterEvent(priv->gentl_data_stream->data_stream, EVENT_NEW_BUFFER, &priv->event_handle);
	if (error != GC_ERR_SUCCESS) {
		arv_warning_stream("[GenTLStream::_loop] GCRegisterEvent[NEW_BUFFER] error (%s)",
                                   arv_gentl_gc_error_to_string(error));
		g_cancellable_cancel(thread_data->cancellable);
	}

	do {
		EVENT_NEW_BUFFER_DATA NewImageEventData = {NULL, NULL};
		size_t size = sizeof(EVENT_NEW_BUFFER_DATA);

                do {
                        arv_buffer = arv_stream_pop_input_buffer (thread_data->stream);
                        if (ARV_IS_BUFFER (arv_buffer)) {
                                ArvGenTLStreamBufferData *buffer_data;

                                buffer_data = g_object_get_data (G_OBJECT(arv_buffer), "gentl-buffer-data");
                                if (buffer_data != NULL) {
                                        error = gentl->DSQueueBuffer(priv->gentl_data_stream->data_stream,
                                                                     buffer_data->gentl_buffer);
                                        if (error != GC_ERR_SUCCESS) {
                                                arv_warning_stream("[GenTLStream::loop] failed to queue buffer (%s)",
                                                                   arv_gentl_gc_error_to_string(error));
                                                arv_stream_push_output_buffer(thread_data->stream, arv_buffer);
                                        } else {
                                                g_hash_table_replace (buffers, buffer_data->gentl_buffer, arv_buffer);
                                        }
                                } else {
                                        arv_warning_stream ("[GenTLStream::loop] buffer without gentl metadata, ignoring");
                                        arv_stream_push_buffer (thread_data->stream, arv_buffer);
                                }
                        }
                } while (arv_buffer != NULL);

		error = gentl->EventGetData(priv->event_handle, &NewImageEventData, &size, GENTL_INFINITE);
		if (error != GC_ERR_SUCCESS) {
			if (error != GC_ERR_ABORT &&
                            !g_cancellable_is_cancelled (thread_data->cancellable))
				arv_warning_stream("[GenTLStream::loop] new buffer error (%s)",
                                                   arv_gentl_gc_error_to_string(error));
			continue;
		}

		gentl_buffer = NewImageEventData.BufferHandle;

		arv_buffer = g_hash_table_lookup (buffers, gentl_buffer);
		if (arv_buffer) {
			if (thread_data->callback != NULL)
				thread_data->callback (thread_data->callback_data,
					ARV_STREAM_CALLBACK_TYPE_START_BUFFER,
					NULL);

                        g_hash_table_remove (buffers, gentl_buffer);
			_gentl_buffer_to_arv_buffer(gentl, priv->gentl_data_stream->data_stream, gentl_buffer,
                                                    arv_buffer, priv->timestamp_tick_frequency);

			if (arv_buffer->priv->status == ARV_BUFFER_STATUS_SUCCESS)
				thread_data->n_completed_buffers += 1;
			else
				thread_data->n_failures += 1;

			thread_data->n_transferred_bytes += arv_buffer->priv->allocated_size;

			arv_stream_push_output_buffer(thread_data->stream, arv_buffer);
			if (thread_data->callback != NULL)
				thread_data->callback (thread_data->callback_data,
					ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
					arv_buffer);
		} else {
                        g_critical ("[GenTL::loop] error retrieving buffer");
		}
	} while (!g_cancellable_is_cancelled (thread_data->cancellable));

	error = gentl->GCUnregisterEvent(priv->gentl_data_stream->data_stream, EVENT_NEW_BUFFER);
	if (error != GC_ERR_SUCCESS)
		arv_warning_stream("[GenTLStream::_loop] GCUnregisterEvent error (%s)",
                                   arv_gentl_gc_error_to_string(error));

        error = gentl->DSFlushQueue(priv->gentl_data_stream->data_stream, ACQ_QUEUE_ALL_DISCARD);
        if (error != GC_ERR_SUCCESS) {
		arv_warning_stream("[GenTLStream::_loop] DSFlushQueue error (%s)",
                                   arv_gentl_gc_error_to_string(error));
        }

        g_hash_table_iter_init (&iter, buffers);
        while (g_hash_table_iter_next (&iter, &key, &value))
        {
                arv_stream_push_output_buffer (thread_data->stream, value);
        }

        g_hash_table_unref (buffers);
}

static void *
arv_gentl_stream_thread (void *data)
{
	ArvGenTLStreamThreadData *thread_data = data;

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

	_loop (thread_data);

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

	return NULL;
}

/* ArvGenTLStream implementation */

static void
_buffer_data_destroy_func (gpointer data)
{
        ArvGenTLStreamBufferData *buffer_data = data;
        ArvGenTLSystem *gentl_system = arv_gentl_device_get_system(buffer_data->gentl_data_stream->device);
        ArvGenTLModule *gentl = arv_gentl_system_get_gentl(gentl_system);

        gentl->DSRevokeBuffer(buffer_data->gentl_data_stream->data_stream, buffer_data->gentl_buffer, NULL, NULL);

        arv_gentl_data_stream_unref(buffer_data->gentl_data_stream);

        g_free (buffer_data);
}

static gboolean
arv_gentl_stream_start_acquisition (ArvStream *stream, GError **error)
{
	ArvGenTLStream *gentl_stream = ARV_GENTL_STREAM(stream);
	ArvGenTLStreamPrivate *priv = arv_gentl_stream_get_instance_private (gentl_stream);
	ArvGenTLStreamThreadData *thread_data;
	ArvGenTLSystem *gentl_system = arv_gentl_device_get_system(priv->gentl_device);
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(gentl_system);
        ArvBuffer *buffer;
	GC_ERROR gc_error;

	g_return_val_if_fail (priv->thread == NULL, FALSE);
	g_return_val_if_fail (priv->thread_data != NULL, FALSE);

        /* Ensure every buffer has a gentl_buffer attached, for those not pushed using arv_stream_create_buffers() */
	do {
		buffer = arv_stream_pop_input_buffer (stream);
		if (buffer != NULL) {
                        ArvGenTLStreamBufferData *buffer_data;

                        buffer_data = g_object_get_data (G_OBJECT(buffer), "gentl-buffer-data");
                        if (buffer_data == NULL) {
                                GC_ERROR gc_error;
                                BUFFER_HANDLE gentl_buffer;

                                gc_error = gentl->DSAnnounceBuffer(priv->gentl_data_stream->data_stream,
                                                                   buffer->priv->data, buffer->priv->allocated_size,
                                                                   NULL, &gentl_buffer);

                                if (gc_error == GC_ERR_SUCCESS) {
                                        buffer_data = g_new0 (ArvGenTLStreamBufferData, 1);
                                        buffer_data->gentl_buffer = gentl_buffer;
                                        buffer_data->gentl_data_stream = arv_gentl_data_stream_ref (priv->gentl_data_stream);

                                        g_object_set_data_full (G_OBJECT (buffer), "gentl-buffer-data",
                                                                buffer_data, _buffer_data_destroy_func);
                                } else {
                                        arv_warning_stream("[GenTLStream::start_acquisition] DSAnnounceBuffer error (%s)",
                                                           arv_gentl_gc_error_to_string(gc_error));
                                }
                        }
                        arv_stream_push_output_buffer (stream, buffer);
                }
        } while (buffer != NULL);

        do {
                buffer = arv_stream_try_pop_buffer (stream);
                if (buffer != NULL)
                        arv_stream_push_buffer (stream, buffer);
        } while (buffer != NULL);

	thread_data = priv->thread_data;

	thread_data->gentl_device = priv->gentl_device;
	thread_data->thread_started = FALSE;
	thread_data->cancellable = g_cancellable_new ();
	priv->thread = g_thread_new ("arv_gentl_stream", arv_gentl_stream_thread, priv->thread_data);

	g_mutex_lock (&thread_data->thread_started_mutex);
	while (!thread_data->thread_started)
		g_cond_wait (&thread_data->thread_started_cond,
				&thread_data->thread_started_mutex);
	g_mutex_unlock (&thread_data->thread_started_mutex);

	arv_info_stream("[GenTLStream::start_acquisition]");

	/* Start acquisition */
	gc_error = gentl->DSStartAcquisition(priv->gentl_data_stream->data_stream,
                                             ACQ_START_FLAGS_DEFAULT, GENTL_INFINITE);
	if (gc_error != GC_ERR_SUCCESS) {
		arv_warning_stream("[GenTLStream::start_acquisition] DSStartAcquisition error (%s)",
                                   arv_gentl_gc_error_to_string(gc_error));
	}

        return TRUE;
}

static gboolean
arv_gentl_stream_stop_acquisition(ArvStream *stream, GError **error)
{
	ArvGenTLStream *gentl_stream = ARV_GENTL_STREAM(stream);
	ArvGenTLStreamPrivate *priv = arv_gentl_stream_get_instance_private (gentl_stream);
	ArvGenTLStreamThreadData *thread_data;
	ArvGenTLSystem *gentl_system = arv_gentl_device_get_system(priv->gentl_device);
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(gentl_system);
	GC_ERROR gc_error;

	g_return_val_if_fail (priv->thread != NULL, FALSE);
	g_return_val_if_fail (priv->thread_data != NULL, FALSE);

	arv_info_stream("[GenTLStream::stop_acquisition]");

	thread_data = priv->thread_data;

	gc_error = gentl->DSStopAcquisition(priv->gentl_data_stream->data_stream, ACQ_STOP_FLAGS_DEFAULT);
	if (gc_error != GC_ERR_SUCCESS && gc_error != GC_ERR_RESOURCE_IN_USE)
		arv_warning_stream("[GenTLStream::start_acquisition] DSStopAcquisition error (%s)",
                                   arv_gentl_gc_error_to_string(gc_error));

	g_cancellable_cancel (thread_data->cancellable);
	gentl->EventKill(priv->event_handle);
	g_thread_join (priv->thread);
	g_clear_object (&thread_data->cancellable);

	priv->thread = NULL;

        return TRUE;
}

static gboolean
arv_gentl_stream_create_buffers (ArvStream *stream, guint n_buffers, size_t size,
                                 void *user_data, GDestroyNotify user_data_destroy_func,
                                 GError **error)
{
	ArvGenTLStream *gentl_stream = ARV_GENTL_STREAM(stream);
	ArvGenTLStreamPrivate *priv = arv_gentl_stream_get_instance_private (gentl_stream);
	ArvGenTLSystem *gentl_system = arv_gentl_device_get_system(priv->gentl_device);
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(gentl_system);
        GC_ERROR gc_error = GC_ERR_SUCCESS;
        guint i;

        for (i = 0; i < n_buffers; i++) {
                ArvBuffer *buffer;
                ArvGenTLStreamBufferData *buffer_data;
                BUFFER_HANDLE gentl_buffer;
                void *data;

		gc_error = gentl->DSAllocAndAnnounceBuffer(priv->gentl_data_stream->data_stream,
                                                           size, NULL, &gentl_buffer);
		if (gc_error != GC_ERR_SUCCESS) {
			arv_warning_stream("[GenTLStream::create_buffers] DSAllocAndAnnounceBuffer (%s)",
                                           arv_gentl_gc_error_to_string(gc_error));
			break;
		}

                /* FIXME handler error */
                _gentl_buffer_info_ptr(gentl, priv->gentl_data_stream->data_stream,
                                       gentl_buffer, BUFFER_INFO_BASE, &data);

                buffer_data = g_new0 (ArvGenTLStreamBufferData, 1);
                buffer_data->gentl_buffer = gentl_buffer;
                buffer_data->gentl_data_stream = arv_gentl_data_stream_ref (priv->gentl_data_stream);

                buffer = arv_buffer_new_full(size, data, user_data, user_data_destroy_func);
                g_object_set_data_full (G_OBJECT (buffer), "gentl-buffer-data", buffer_data, _buffer_data_destroy_func);
                arv_stream_push_buffer (stream, buffer);
        }

        return gc_error == GC_ERR_SUCCESS;
}

/**
 * arv_gentl_stream_new: (skip)
 * @gentl_device: a #ArvGenTLDevice
 * @callback: (scope call): processing callback
 * @callback_data: (closure): user data for @callback
 *
 * Return value: (transfer full): a new #ArvStream.
 */

ArvStream *
arv_gentl_stream_new (ArvGenTLDevice *gentl_device,
                      ArvStreamCallback callback, void *callback_data, GDestroyNotify destroy,
                      GError **error)
{
	return g_initable_new (ARV_TYPE_GENTL_STREAM, NULL, error,
			"device",  gentl_device,
			"callback", callback,
			"callback-data", callback_data,
			"destroy-notify", destroy,
			NULL);
}

/* ArvStream implementation */

static void
arv_gentl_stream_init (ArvGenTLStream *gentl_stream)
{
	ArvGenTLStreamPrivate *priv = arv_gentl_stream_get_instance_private (gentl_stream);

	priv->thread_data = g_new0 (ArvGenTLStreamThreadData, 1);
	priv->event_handle = NULL;
	priv->gentl_data_stream = NULL;
	priv->n_buffers = ARV_GENTL_STREAM_DEFAULT_N_BUFFERS;
}

static void
arv_gentl_stream_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ArvGenTLStreamPrivate *priv = arv_gentl_stream_get_instance_private (ARV_GENTL_STREAM (self));

	switch (prop_id)
	{
		case PROP_STREAM_N_BUFFERS:
			priv->n_buffers = g_value_get_uint(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
			break;
	}
}

static void
arv_gentl_stream_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	ArvGenTLStreamPrivate *priv = arv_gentl_stream_get_instance_private (ARV_GENTL_STREAM (object));

	switch (prop_id)
	{
		case PROP_STREAM_N_BUFFERS:
			g_value_set_uint(value, priv->n_buffers);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_gentl_stream_constructed (GObject *object)
{
	ArvStream *stream = ARV_STREAM (object);
	ArvGenTLStream *gentl_stream = ARV_GENTL_STREAM (stream);
	ArvGenTLStreamPrivate *priv = arv_gentl_stream_get_instance_private (gentl_stream);

	G_OBJECT_CLASS (arv_gentl_stream_parent_class)->constructed (object);

        g_object_get (object,
                      "device",&priv->gentl_device,
                      "callback", &priv->thread_data->callback,
                      "callback-data", &priv->thread_data->callback_data,
                      NULL);

	priv->gentl_data_stream = arv_gentl_data_stream_new (priv->gentl_device);
	priv->timestamp_tick_frequency = arv_gentl_device_get_timestamp_tick_frequency(priv->gentl_device);
	priv->thread_data->stream = stream;

	arv_stream_declare_info (stream, "n_completed_buffers",
				 G_TYPE_UINT64, &priv->thread_data->n_completed_buffers);
	arv_stream_declare_info (stream, "n_failures",
				 G_TYPE_UINT64, &priv->thread_data->n_failures);
	arv_stream_declare_info (stream, "n_underruns",
				 G_TYPE_UINT64, &priv->thread_data->n_underruns);
	arv_stream_declare_info (stream, "n_transferred_bytes",
				 G_TYPE_UINT64, &priv->thread_data->n_transferred_bytes);
}

static void
arv_gentl_stream_finalize (GObject *object)
{
	ArvGenTLStream *gentl_stream = ARV_GENTL_STREAM (object);
	ArvGenTLStreamPrivate *priv = arv_gentl_stream_get_instance_private (gentl_stream);

        if (priv->thread != NULL)
                arv_gentl_stream_stop_acquisition (ARV_STREAM(gentl_stream), NULL);

	if (priv->thread_data != NULL) {
		ArvGenTLStreamThreadData *thread_data;

		thread_data = priv->thread_data;

		arv_info_stream ("[GenTLStream::finalize] n_completed_buffers    = %" G_GUINT64_FORMAT,
				  thread_data->n_completed_buffers);
		arv_info_stream ("[GenTLStream::finalize] n_failures             = %" G_GUINT64_FORMAT,
				  thread_data->n_failures);
		arv_info_stream ("[GenTLStream::finalize] n_underruns            = %" G_GUINT64_FORMAT,
				  thread_data->n_underruns);
		arv_info_stream ("[GenTLStream::finalize] n_transferred_bytes    = %" G_GUINT64_FORMAT,
				  thread_data->n_transferred_bytes);

		g_mutex_clear (&thread_data->stream_mtx);
		g_cond_clear (&thread_data->stream_event);
		g_clear_pointer (&priv->thread_data, g_free);
	}

	g_clear_object (&priv->gentl_device);

	G_OBJECT_CLASS (arv_gentl_stream_parent_class)->finalize (object);

        arv_gentl_data_stream_unref (priv->gentl_data_stream);
}

static void
arv_gentl_stream_class_init (ArvGenTLStreamClass *gentl_stream_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (gentl_stream_class);
	ArvStreamClass *stream_class = ARV_STREAM_CLASS (gentl_stream_class);

	object_class->constructed = arv_gentl_stream_constructed;
	object_class->finalize = arv_gentl_stream_finalize;
	object_class->get_property = arv_gentl_stream_get_property;
	object_class->set_property = arv_gentl_stream_set_property;

	stream_class->start_acquisition = arv_gentl_stream_start_acquisition;
	stream_class->stop_acquisition = arv_gentl_stream_stop_acquisition;
        stream_class->create_buffers = arv_gentl_stream_create_buffers;

	g_object_class_install_property
		(object_class,
		 PROP_STREAM_N_BUFFERS,
                 g_param_spec_uint ("n-buffers",
                                    "Number of buffers",
                                    "Number of buffers",
                                    ARV_GENTL_STREAM_MIN_N_BUFFERS,
                                    ARV_GENTL_STREAM_MAX_N_BUFFERS,
                                    ARV_GENTL_STREAM_DEFAULT_N_BUFFERS,
                                    G_PARAM_READWRITE));
}
