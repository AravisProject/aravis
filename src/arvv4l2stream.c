/* Aravis - Digital camera library
 *
 * Copyright © 2009-2021 Emmanuel Pacaud
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
 * SECTION: arvv4l2stream
 * @short_description: V4l2 stream
 */

#include <arvv4l2streamprivate.h>
#include <arvv4l2deviceprivate.h>
#include <arvstreamprivate.h>
#include <arvbufferprivate.h>
#include <arvdebug.h>
#include <arvmisc.h>
#include <arvstr.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <sys/mman.h>

#define ARV_V4L2_STREAM_N_BUFFERS       3

typedef enum {
        ARV_V4L2_STREAM_IO_METHOD_UNKNOWN = -1,
        ARV_V4L2_STREAM_IO_METHOD_READ,
        ARV_V4L2_STREAM_IO_METHOD_MMAP,
        ARV_V4L2_STREAM_IO_METHOD_USER_POINTER
} ArvV4l2StreamIOMethod;

typedef struct {
        ArvV4l2Device *v4l2_device;
        void *data;
        size_t size;
        int index;
} ArvV4l2StreamBufferData;

typedef struct {
	ArvStream *stream;

        gboolean thread_started;
        GMutex thread_started_mutex;
        GCond thread_started_cond;

	ArvV4l2Device *v4l2_device;
	ArvStreamCallback callback;
	void *callback_data;

	gboolean cancel;

        int device_fd;

        ArvV4l2StreamIOMethod io_method;

        ArvPixelFormat pixel_format;
        guint32 image_width;
        guint32 image_height;

        guint32 frame_id;

	/* Statistics */

	guint n_completed_buffers;
	guint n_failures;
	guint n_underruns;
        guint n_transferred_bytes;
} ArvV4l2StreamThreadData;

typedef struct {
	GThread *thread;

	ArvV4l2StreamThreadData *thread_data;
} ArvV4l2StreamPrivate;

struct _ArvV4l2Stream {
	ArvStream	stream;
};

struct _ArvV4l2StreamClass {
	ArvStreamClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvV4l2Stream, arv_v4l2_stream, ARV_TYPE_STREAM, G_ADD_PRIVATE (ArvV4l2Stream))

/* Acquisition thread */

static void *
arv_v4l2_stream_thread (void *data)
{
	ArvV4l2StreamThreadData *thread_data = data;
        GHashTable *buffers;
        GHashTableIter iter;
        gpointer key, value;
	ArvBuffer *arv_buffer;

	arv_info_stream_thread ("[V4l2Stream::thread] Start");

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

        buffers = g_hash_table_new (g_direct_hash, g_direct_equal);

        g_mutex_lock (&thread_data->thread_started_mutex);
        thread_data->thread_started = TRUE;
        g_cond_signal (&thread_data->thread_started_cond);
        g_mutex_unlock (&thread_data->thread_started_mutex);

        g_usleep (100000);

	while (!g_atomic_int_get (&thread_data->cancel)) {
                struct v4l2_buffer bufd = {0};
                fd_set fds;
                struct timeval tv;
                int result;

                do {
                        arv_buffer = arv_stream_pop_input_buffer (thread_data->stream);
                        if (ARV_IS_BUFFER (arv_buffer)) {
                                bufd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                                bufd.memory = V4L2_MEMORY_MMAP;
                                bufd.index = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (arv_buffer), "v4l2-index"));

                                if (v4l2_ioctl (thread_data->device_fd, VIDIOC_QBUF, &bufd) == -1) {
                                        arv_warning_stream_thread ("Failed to queue v4l2 buffer");
                                        arv_stream_push_output_buffer(thread_data->stream, arv_buffer);
                                } else {
                                        arv_trace_stream_thread ("Queue buffer %d\n", bufd.index);
                                        g_hash_table_replace (buffers, GINT_TO_POINTER (bufd.index), arv_buffer);
                                }
                        }
                } while (arv_buffer != NULL);

                FD_ZERO(&fds);
                FD_SET(thread_data->device_fd, &fds);

                tv.tv_sec = 1;
                tv.tv_usec = 0;
                result = select(thread_data->device_fd + 1, &fds, NULL, NULL, &tv);
                if(result == -1){
                        if (errno != EINTR)
                                arv_warning_stream_thread ("Error while waiting for frame (%s)", strerror(errno));
                        continue;
                }

                if (result == 0)
                        continue;

                bufd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                bufd.memory = V4L2_MEMORY_MMAP;
                bufd.index = 0;

                if(v4l2_ioctl(thread_data->device_fd, VIDIOC_DQBUF, &bufd) == -1)
                        arv_warning_stream_thread("DeQueue buffer error (%s)", strerror(errno));
                else
                        arv_trace_stream_thread ("Dequeued buffer %d\n", bufd.index);

                arv_buffer = g_hash_table_lookup (buffers, GINT_TO_POINTER (bufd.index));
                if (ARV_IS_BUFFER (arv_buffer)) {

                        g_hash_table_remove (buffers, GINT_TO_POINTER(bufd.index));
                        arv_buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_IMAGE;
                        arv_buffer->priv->chunk_endianness = G_BIG_ENDIAN;
                        arv_buffer->priv->status = ARV_BUFFER_STATUS_SUCCESS;
                        arv_buffer->priv->timestamp_ns = 1000000000000L * bufd.timestamp.tv_sec +
                                1000L * bufd.timestamp.tv_usec;
                        arv_buffer->priv->system_timestamp_ns = g_get_real_time () * 1000;
                        arv_buffer->priv->frame_id = thread_data->frame_id++;
                        arv_buffer->priv->received_size = bufd.bytesused;

                        arv_buffer_set_n_parts(arv_buffer, 1);
                        arv_buffer->priv->parts[0].data_offset = 0;
                        arv_buffer->priv->parts[0].component_id = 0;
                        arv_buffer->priv->parts[0].data_type = ARV_BUFFER_PART_DATA_TYPE_2D_IMAGE;
                        arv_buffer->priv->parts[0].pixel_format = thread_data->pixel_format;
                        arv_buffer->priv->parts[0].width = thread_data->image_width;
                        arv_buffer->priv->parts[0].height = thread_data->image_height;
                        arv_buffer->priv->parts[0].x_offset = 0;
                        arv_buffer->priv->parts[0].y_offset = 0;
                        arv_buffer->priv->parts[0].x_padding = 0;
                        arv_buffer->priv->parts[0].y_padding = 0;
                        arv_buffer->priv->parts[0].size = arv_buffer->priv->received_size;

                        arv_trace_stream_thread("size = %zu", arv_buffer->priv->received_size);

                        thread_data->n_completed_buffers++;
                        thread_data->n_transferred_bytes += bufd.length;
                        arv_stream_push_output_buffer (thread_data->stream, arv_buffer);
                        if (thread_data->callback != NULL)
                                thread_data->callback (thread_data->callback_data,
                                                       ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
                                                       arv_buffer);
                } else
                        arv_warning_stream_thread("buffer for index %d not found", bufd.index);
        }

        g_hash_table_iter_init (&iter, buffers);
        while (g_hash_table_iter_next (&iter, &key, &value))
        {
                arv_stream_push_output_buffer (thread_data->stream, value);
        }
        g_hash_table_unref (buffers);

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

	arv_info_stream_thread ("[V4l2Stream::thread] Stop");

	return NULL;
}

/* ArvV4l2Stream implemenation */

static gboolean
arv_v4l2_stream_start_acquisition (ArvStream *stream, GError **error)
{
	ArvV4l2Stream *v4l2_stream = ARV_V4L2_STREAM (stream);
	ArvV4l2StreamPrivate *priv = arv_v4l2_stream_get_instance_private (v4l2_stream);
	ArvV4l2StreamThreadData *thread_data;
        ArvBuffer *buffer;
        gboolean mixed_io_method = FALSE;

	g_return_val_if_fail (priv->thread == NULL, FALSE);
	g_return_val_if_fail (priv->thread_data != NULL, FALSE);

	thread_data = priv->thread_data;
	thread_data->cancel = FALSE;
        thread_data->thread_started = FALSE;

	do {
		buffer = arv_stream_pop_input_buffer(stream);
		if (ARV_IS_BUFFER(buffer))
                        arv_stream_push_output_buffer(stream, buffer);
	} while (buffer != NULL);

	do {
		buffer = arv_stream_pop_buffer (stream);
		if (ARV_IS_BUFFER(buffer)) {
                        ArvV4l2StreamBufferData *buffer_data;

                        buffer_data = g_object_get_data (G_OBJECT(buffer), "v4l2-buffer-data");
                        if (buffer_data != NULL) {
                                if (thread_data->io_method != ARV_V4L2_STREAM_IO_METHOD_UNKNOWN &&
                                    thread_data->io_method != ARV_V4L2_STREAM_IO_METHOD_MMAP)
                                        mixed_io_method = TRUE;
                                thread_data->io_method = ARV_V4L2_STREAM_IO_METHOD_MMAP;
                        } else {
                                if (thread_data->io_method != ARV_V4L2_STREAM_IO_METHOD_UNKNOWN &&
                                    thread_data->io_method != ARV_V4L2_STREAM_IO_METHOD_READ)
                                        mixed_io_method = TRUE;
                                thread_data->io_method = ARV_V4L2_STREAM_IO_METHOD_READ;
                        }

                        arv_stream_push_buffer(stream, buffer);
		}
	} while (buffer != NULL);

        if (mixed_io_method) {
                g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                             "V4l2 mixed IO method not allowed (mmap and read)");
                return FALSE;
        }

        if (!arv_v4l2_device_get_image_format (priv->thread_data->v4l2_device, NULL,
                                               &thread_data->pixel_format,
                                               &thread_data->image_width,
                                               &thread_data->image_height)) {
                g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                             "Failed to query v4l2 image format");
                return FALSE;
        }

        thread_data->frame_id = 0;

	priv->thread = g_thread_new ("arv_v4l2_stream", arv_v4l2_stream_thread, priv->thread_data);

        g_mutex_lock (&thread_data->thread_started_mutex);
        while (!thread_data->thread_started)
                g_cond_wait (&thread_data->thread_started_cond,
                             &thread_data->thread_started_mutex);
        g_mutex_unlock (&thread_data->thread_started_mutex);

        return TRUE;
}

static gboolean
arv_v4l2_stream_stop_acquisition (ArvStream *stream, GError **error)
{
	ArvV4l2Stream *v4l2_stream = ARV_V4L2_STREAM (stream);
	ArvV4l2StreamPrivate *priv = arv_v4l2_stream_get_instance_private (v4l2_stream);
	ArvV4l2StreamThreadData *thread_data;

	g_return_val_if_fail (priv->thread != NULL, FALSE);
	g_return_val_if_fail (priv->thread_data != NULL, FALSE);

	thread_data = priv->thread_data;

	g_atomic_int_set (&thread_data->cancel, TRUE);
	g_thread_join (priv->thread);

	priv->thread = NULL;

        return TRUE;
}

static void
_buffer_data_destroy_func (gpointer data)
{
        ArvV4l2StreamBufferData *buffer_data = data;

        arv_debug_stream ("free data %p size %zu\n", buffer_data->data, buffer_data->size);
        arv_debug_stream ("v4l2 device %p\n", buffer_data->v4l2_device);

        munmap (buffer_data->data, buffer_data->size);

        g_object_unref (buffer_data->v4l2_device);

        g_free (buffer_data);
}

static gboolean
arv_v4l2_stream_create_buffers (ArvStream *stream, guint n_buffers, size_t size,
                                void *user_data, GDestroyNotify user_data_destroy_func,
                                GError **error)
{
	ArvV4l2Stream *v4l2_stream = ARV_V4L2_STREAM (stream);
	ArvV4l2StreamPrivate *priv = arv_v4l2_stream_get_instance_private (v4l2_stream);
        struct v4l2_requestbuffers req = {0};
        guint i;

        if (!arv_v4l2_device_set_image_format (priv->thread_data->v4l2_device)) {
                g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                             "Failed to set image format (%s)",
                             strerror(errno));
                return FALSE;
        }

        req.count = n_buffers;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        if (v4l2_ioctl(priv->thread_data->device_fd, VIDIOC_REQBUFS, &req) == -1) {
                g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                             "Failed to request v4l2 buffer (%s)",
                             strerror(errno));
                return FALSE;
        }

        if (req.count < 2) {
                g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                             "Failed to request enough v4l2 buffer (%s)",
                             strerror(errno));
                return FALSE;
        }

        if (req.count != n_buffers)
               arv_warning_stream ("Could only create %d buffers, while %d were requested", req.count, n_buffers);

        for (i = 0; i < req.count; i++) {
                ArvBuffer *buffer;
                ArvV4l2StreamBufferData *buffer_data;
                struct v4l2_buffer buf = {0};
                unsigned char *v4l2_buffer = NULL;

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;

                if (v4l2_ioctl(priv->thread_data->device_fd, VIDIOC_QUERYBUF, &buf) == -1) {
                        g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                                     "Failed to request v4l2 buffer (%s)",
                                     strerror(errno));
                        return FALSE;
                }

                v4l2_buffer = (u_int8_t *) mmap (NULL, buf.length,
                                                 PROT_READ | PROT_WRITE,
                                                 MAP_SHARED,
                                                 priv->thread_data->device_fd, buf.m.offset);

                size = buf.length;

                buffer = arv_buffer_new_full (size, v4l2_buffer, user_data,user_data_destroy_func);

                buffer_data = g_new0 (ArvV4l2StreamBufferData, 1);
                buffer_data->v4l2_device = g_object_ref (priv->thread_data->v4l2_device);
                buffer_data->data = buffer->priv->data;
                buffer_data->size = size;

                arv_debug_stream ("buffer %d data %p size %zu\n", i, buffer->priv->data, size);

                g_object_set_data_full (G_OBJECT (buffer), "v4l2-buffer-data",
                                        buffer_data, _buffer_data_destroy_func);
                g_object_set_data (G_OBJECT(buffer), "v4l2-index", GINT_TO_POINTER(i));

                arv_stream_push_buffer (stream, buffer);
        }

        arv_info_stream ("Created %d v4l2 native buffers", i);

        return TRUE;
}

/**
 * arv_v4l2_stream_new: (skip)
 * @camera: a #ArvV4l2Device
 * @callback: (scope call): image processing callback
 * @callback_data: (closure): user data for @callback
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Return Value: (transfer full): a new #ArvStream.
 *
 * Since: 0.10.0
 */

ArvStream *
arv_v4l2_stream_new (ArvV4l2Device *device,
                     ArvStreamCallback callback, void *callback_data, GDestroyNotify destroy,
                     GError **error)
{
	return g_initable_new (ARV_TYPE_V4L2_STREAM, NULL, error,
			       "device", device,
			       "callback", callback,
			       "callback-data", callback_data,
                               "destroy-notify", destroy,
			       NULL);
}

static void
arv_v4l2_stream_constructed (GObject *object)
{
	ArvStream *stream = ARV_STREAM (object);
	ArvV4l2Stream *v4l2_stream = ARV_V4L2_STREAM (object);
	ArvV4l2StreamPrivate *priv = arv_v4l2_stream_get_instance_private (v4l2_stream);
	ArvV4l2StreamThreadData *thread_data;
	g_autoptr (ArvV4l2Device) v4l2_device = NULL;

	thread_data = g_new0 (ArvV4l2StreamThreadData, 1);
	thread_data->stream = stream;

	g_object_get (object,
		      "device", &thread_data->v4l2_device,
		      "callback", &thread_data->callback,
		      "callback-data", &thread_data->callback_data,
		      NULL);

	thread_data->cancel = FALSE;

	thread_data->n_completed_buffers = 0;
	thread_data->n_failures = 0;
	thread_data->n_underruns = 0;

	priv->thread_data = thread_data;

        arv_stream_declare_info (ARV_STREAM (v4l2_stream), "n_completed_buffers",
                                 G_TYPE_UINT64, &priv->thread_data->n_completed_buffers);
        arv_stream_declare_info (ARV_STREAM (v4l2_stream), "n_failures",
                                 G_TYPE_UINT64, &priv->thread_data->n_failures);
        arv_stream_declare_info (ARV_STREAM (v4l2_stream), "n_underruns",
                                 G_TYPE_UINT64, &priv->thread_data->n_underruns);
        arv_stream_declare_info (ARV_STREAM (v4l2_stream), "n_transferred_bytes",
                                 G_TYPE_UINT64, &priv->thread_data->n_transferred_bytes);

        thread_data->device_fd = arv_v4l2_device_get_fd (ARV_V4L2_DEVICE(thread_data->v4l2_device));
}

/* ArvStream implementation */

static void
arv_v4l2_stream_init (ArvV4l2Stream *v4l2_stream)
{
}

static void
arv_v4l2_stream_finalize (GObject *object)
{
	ArvV4l2Stream *v4l2_stream = ARV_V4L2_STREAM (object);
	ArvV4l2StreamPrivate *priv = arv_v4l2_stream_get_instance_private (v4l2_stream);

        if (priv->thread != NULL)
                arv_v4l2_stream_stop_acquisition (ARV_STREAM (v4l2_stream), NULL);

	if (priv->thread_data != NULL) {
		g_clear_object (&priv->thread_data->v4l2_device);
		g_clear_pointer (&priv->thread_data, g_free);
	}

	G_OBJECT_CLASS (arv_v4l2_stream_parent_class)->finalize (object);
}

static void
arv_v4l2_stream_class_init (ArvV4l2StreamClass *v4l2_stream_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (v4l2_stream_class);
	ArvStreamClass *stream_class = ARV_STREAM_CLASS (v4l2_stream_class);

	object_class->constructed = arv_v4l2_stream_constructed;
	object_class->finalize = arv_v4l2_stream_finalize;

	stream_class->start_acquisition = arv_v4l2_stream_start_acquisition;
	stream_class->stop_acquisition = arv_v4l2_stream_stop_acquisition;
        stream_class->create_buffers =  arv_v4l2_stream_create_buffers;
}
