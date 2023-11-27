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
 * SECTION: arvstream
 * @short_description: Abstract base class for video stream reception
 *
 * #ArvStream provides an abstract base class for the implementation of video
 * stream reception threads. The interface between the reception thread and the
 * main thread is done using asynchronous queues, containing #ArvBuffer
 * objects.
 */

#include <arvstreamprivate.h>
#include <arvbuffer.h>
#include <arvdevice.h>
#include <arvdebugprivate.h>
#include <gio/gio.h>

typedef struct {
        char *name;
        char *description;
        GType type;
        gpointer data;
} ArvStreamInfo;

enum {
	ARV_STREAM_SIGNAL_NEW_BUFFER,
	ARV_STREAM_SIGNAL_LAST
} ArvStreamSignals;

static guint arv_stream_signals[ARV_STREAM_SIGNAL_LAST] = {0};

enum {
	ARV_STREAM_PROPERTY_0,
	ARV_STREAM_PROPERTY_EMIT_SIGNALS,
	ARV_STREAM_PROPERTY_DEVICE,
	ARV_STREAM_PROPERTY_CALLBACK,
	ARV_STREAM_PROPERTY_CALLBACK_DATA,
	ARV_STREAM_PROPERTY_DESTROY_NOTIFY
} ArvStreamProperties;

typedef struct {
	GAsyncQueue *input_queue;
	GAsyncQueue *output_queue;
        gint n_buffer_filling;
	GRecMutex mutex;
	gboolean emit_signals;

	ArvDevice *device;
	ArvStreamCallback callback;
	void *callback_data;
	GDestroyNotify destroy_notify;

	GError *init_error;

        GPtrArray *infos;
} ArvStreamPrivate;

static void arv_stream_initable_iface_init (GInitableIface *iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ArvStream, arv_stream, G_TYPE_OBJECT,
				  G_ADD_PRIVATE (ArvStream)
				  G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, arv_stream_initable_iface_init))

/**
 * arv_stream_push_buffer:
 * @stream: a #ArvStream
 * @buffer: (transfer full): buffer to push
 *
 * Pushes a #ArvBuffer to the @stream thread. The @stream takes ownership of @buffer,
 * and will free all the buffers still in its queues when destroyed.
 *
 * This method is thread safe.
 *
 * Since: 0.2.0
 */

void
arv_stream_push_buffer (ArvStream *stream, ArvBuffer *buffer)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	g_return_if_fail (ARV_IS_STREAM (stream));
	g_return_if_fail (ARV_IS_BUFFER (buffer));

	g_async_queue_push (priv->input_queue, buffer);
}

/**
 * arv_stream_pop_buffer:
 * @stream: a #ArvStream
 *
 * Pops a buffer from the output queue of @stream. The retrieved buffer
 * may contain an invalid image. Caller should check the buffer status before using it.
 * This function blocks until a buffer is available.
 *
 * This method is thread safe.
 *
 * Returns: (transfer full): a #ArvBuffer
 *
 * Since: 0.2.0
 */

ArvBuffer *
arv_stream_pop_buffer (ArvStream *stream)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	g_return_val_if_fail (ARV_IS_STREAM (stream), NULL);

	return g_async_queue_pop (priv->output_queue);
}

/**
 * arv_stream_try_pop_buffer:
 * @stream: a #ArvStream
 *
 * Pops a buffer from the output queue of @stream. The retrieved buffer
 * may contain an invalid image. Caller should check the buffer status before using it.
 * This is the non blocking version of pop_buffer.
 *
 * This method is thread safe.
 *
 * Returns: (transfer full): a #ArvBuffer, NULL if no buffer is available.
 *
 * Since: 0.2.0
 */

ArvBuffer *
arv_stream_try_pop_buffer (ArvStream *stream)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	g_return_val_if_fail (ARV_IS_STREAM (stream), NULL);

	return g_async_queue_try_pop (priv->output_queue);
}

/**
 * arv_stream_timeout_pop_buffer:
 * @stream: a #ArvStream
 * @timeout: timeout, in µs
 *
 * Pops a buffer from the output queue of @stream, waiting no more than @timeout. The retrieved buffer
 * may contain an invalid image. Caller should check the buffer status before using it.
 *
 * This method is thread safe.
 *
 * Returns: (transfer full): a #ArvBuffer, NULL if no buffer is available until the timeout occurs.
 *
 * Since: 0.2.0
 */

ArvBuffer *
arv_stream_timeout_pop_buffer (ArvStream *stream, guint64 timeout)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	g_return_val_if_fail (ARV_IS_STREAM (stream), NULL);

	return g_async_queue_timeout_pop (priv->output_queue, timeout);
}

/**
 * arv_stream_pop_input_buffer: (skip)
 * @stream: (transfer full): a #ArvStream
 *
 * Pops a buffer from the input queue of @stream.
 *
 * Since: 0.2.0
 */

ArvBuffer *
arv_stream_pop_input_buffer (ArvStream *stream)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);
        void *data;

	g_return_val_if_fail (ARV_IS_STREAM (stream), NULL);

        g_async_queue_lock(priv->input_queue);
	data = g_async_queue_try_pop_unlocked (priv->input_queue);
        if (data != NULL)
                priv->n_buffer_filling++;
        g_async_queue_unlock(priv->input_queue);

        return data;
}

ArvBuffer *
arv_stream_timeout_pop_input_buffer (ArvStream *stream, guint64 timeout)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);
        void *data;

	g_return_val_if_fail (ARV_IS_STREAM (stream), NULL);

        g_async_queue_lock(priv->input_queue);
	data = g_async_queue_timeout_pop_unlocked (priv->input_queue, timeout);
        if (data != NULL)
                priv->n_buffer_filling++;
        g_async_queue_unlock(priv->input_queue);

        return data;
}

void
arv_stream_push_output_buffer (ArvStream *stream, ArvBuffer *buffer)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	g_return_if_fail (ARV_IS_STREAM (stream));
	g_return_if_fail (ARV_IS_BUFFER (buffer));

        g_async_queue_lock (priv->output_queue);
	g_async_queue_push_unlocked (priv->output_queue, buffer);
        priv->n_buffer_filling--;
        g_async_queue_unlock(priv->output_queue);

	g_rec_mutex_lock (&priv->mutex);

	if (priv->emit_signals)
		g_signal_emit (stream, arv_stream_signals[ARV_STREAM_SIGNAL_NEW_BUFFER], 0);

	g_rec_mutex_unlock (&priv->mutex);
}

/**
 * arv_stream_get_n_owned_buffers:
 * @stream: a #ArvStream
 * @n_input_buffers: (out) (allow-none): input queue length
 * @n_output_buffers: (out) (allow-none): output queue length
 * @n_buffer_filling: (out) (allow-none): number of buffer owned by the stream receiving thread
 *
 * An accessor to the number of buffer owned by the stream instance.
 *
 * Since: 0.10.0
 */

void
arv_stream_get_n_owned_buffers (ArvStream *stream, gint *n_input_buffers, gint *n_output_buffers, gint *n_buffer_filling)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	if (!ARV_IS_STREAM (stream)) {
		if (n_input_buffers != NULL)
			*n_input_buffers = 0;
		if (n_output_buffers != NULL)
			*n_output_buffers = 0;
                if (n_buffer_filling != NULL)
                        *n_buffer_filling = 0;
		return;
	}

        g_async_queue_lock (priv->input_queue);
        g_async_queue_lock (priv->output_queue);
	if (n_input_buffers != NULL)
		*n_input_buffers = g_async_queue_length_unlocked (priv->input_queue);
	if (n_output_buffers != NULL)
		*n_output_buffers = g_async_queue_length_unlocked (priv->output_queue);
        if (n_buffer_filling != NULL)
                *n_buffer_filling = priv->n_buffer_filling;
        g_async_queue_unlock (priv->output_queue);
        g_async_queue_unlock (priv->input_queue);
}

/**
 * arv_stream_start_acquisition:
 * @stream: a #ArvStream
 *
 * Start the stream acquisition. This has to be done before the `AcquisitionStart` feature is executed.
 * [method@Aravis.Camera.start_acquisition] does this for you.
 *
 * Since: 0.10.0
 *
 * Returns: %TRUE on success
 */

gboolean
arv_stream_start_acquisition (ArvStream *stream, GError **error)
{
	ArvStreamClass *stream_class;
        gboolean success;

	g_return_val_if_fail (ARV_IS_STREAM (stream), FALSE);

	stream_class = ARV_STREAM_GET_CLASS (stream);
	g_return_val_if_fail (stream_class->start_acquisition != NULL, FALSE);

	success = stream_class->start_acquisition (stream, error);
        if (!success)
                arv_warning_stream ("Failed to start stream acquisition ");

        return success;
}

/**
 * arv_stream_stop_acquisition:
 * @stream: a #ArvStream
 *
 * Stop the stream acquisition. This is optional, as the acquisition will always be stopped on stream object
 * destruction. [method@Aravis.Camera.stop_acquisition] calls this function.
 *
 * Returns: %TRUE on success
 *
 * Since: 0.10.0
 */

gboolean
arv_stream_stop_acquisition (ArvStream *stream, GError **error)
{
	ArvStreamClass *stream_class;
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);
        gboolean success;

	g_return_val_if_fail (ARV_IS_STREAM (stream), FALSE);

	stream_class = ARV_STREAM_GET_CLASS (stream);
	g_return_val_if_fail (stream_class->stop_acquisition != NULL, FALSE);

	success = stream_class->stop_acquisition (stream, error);

        if (success && priv->n_buffer_filling != 0) {
                g_critical ("Buffer filling count must be 0 after acquisition stop (was %d)", priv->n_buffer_filling);
        }
        if (!success)
                arv_warning_stream ("Failed to stop stream acquisition ");

        return success;
}

/**
 * arv_stream_delete_buffers:
 * @stream: a #ArvStream
 *
 * Remove all the buffers still in the input and output queues. If the stream is still in acquisition mode, some buffers
 * may be in use by the receiving thread, and not removed. Main use of the buffer deletion is to be able to quickly
 * change an acquisition parameter that changes the payload size, then restart the acquisition without
 * deleting/recreating the stream object.
 *
 * Returns: the number of deleted buffers.
 *
 * Since: 0.10.0
 */

guint
arv_stream_delete_buffers (ArvStream *stream)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);
	ArvBuffer *buffer;
	unsigned int n_deleted = 0;

        g_return_val_if_fail (ARV_IS_STREAM(stream), 0);

	g_async_queue_lock (priv->input_queue);
	g_async_queue_lock (priv->output_queue);

	arv_info_stream ("[Stream::delete_buffers] Delete %d buffer[s] in input queue",
                         g_async_queue_length_unlocked(priv->input_queue));
	arv_info_stream ("[Stream::delete_buffers] Delete %d buffer[s] in output queue",
                         g_async_queue_length_unlocked(priv->output_queue));

	do {
		buffer = g_async_queue_try_pop_unlocked (priv->input_queue);
		if (buffer != NULL) {
			g_object_unref (buffer);
			n_deleted++;
		}
	} while (buffer != NULL);

	do {
		buffer = g_async_queue_try_pop_unlocked (priv->output_queue);
		if (buffer != NULL) {
			g_object_unref (buffer);
			n_deleted++;
		}
	} while (buffer != NULL);

	g_async_queue_unlock (priv->output_queue);
	g_async_queue_unlock (priv->input_queue);

	return n_deleted;
}

/**
 * arv_stream_get_statistics:
 * @stream: a #ArvStream
 * @n_completed_buffers: (out) (allow-none): number of complete received buffers
 * @n_failures: (out) (allow-none): number of reception failures
 * @n_underruns: (out) (allow-none): number of input buffer underruns
 *
 * An accessor to the stream statistics.
 *
 * Since: 0.2.0
 */

void
arv_stream_get_statistics (ArvStream *stream,
			   guint64 *n_completed_buffers,
			   guint64 *n_failures,
			   guint64 *n_underruns)
{
	guint64 dummy;

	if (n_completed_buffers == NULL)
		n_completed_buffers = &dummy;
	if (n_failures == NULL)
		n_failures = &dummy;
	if (n_underruns == NULL)
		n_underruns = &dummy;

	*n_completed_buffers = arv_stream_get_info_uint64_by_name (stream, "n_completed_buffers");
	*n_failures = arv_stream_get_info_uint64_by_name (stream, "n_failures");
	*n_underruns = arv_stream_get_info_uint64_by_name (stream, "n_underruns");
}

/**
 * arv_stream_set_emit_signals:
 * @stream: a #ArvStream
 * @emit_signals: the new state
 *
 * Make @stream emit signals. This option is
 * by default disabled because signal emission is expensive and unneeded when
 * the application prefers to operate in pull mode.
 *
 * Since: 0.2.0
 */

void
arv_stream_set_emit_signals (ArvStream *stream, gboolean emit_signals)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	g_return_if_fail (ARV_IS_STREAM (stream));

	g_rec_mutex_lock (&priv->mutex);

	priv->emit_signals = emit_signals;

	g_rec_mutex_unlock (&priv->mutex);
}

/**
 * arv_stream_get_emit_signals:
 * @stream: a #ArvStream
 *
 * Check if stream will emit its signals.
 *
 * Returns: %TRUE if @stream is emiting its signals.
 *
 * Since: 0.2.0
 */

gboolean
arv_stream_get_emit_signals (ArvStream *stream)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);
	gboolean ret;

	g_return_val_if_fail (ARV_IS_STREAM (stream), FALSE);

	g_rec_mutex_lock (&priv->mutex);

	ret = priv->emit_signals;

	g_rec_mutex_unlock (&priv->mutex);

	return ret;
}

static void arv_stream_info_free (ArvStreamInfo *info)
{
        if (info == NULL)
                return;

        g_free (info->name);
        g_free (info);
}

void
arv_stream_declare_info (ArvStream *stream, const char *name, GType type, gpointer data)
{
        ArvStreamInfo *info;

        ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	g_return_if_fail (ARV_IS_STREAM (stream));
        g_return_if_fail (type == G_TYPE_DOUBLE || type == G_TYPE_UINT64);
        g_return_if_fail (data != NULL);

        info = g_new0 (ArvStreamInfo, 1);
        info->name = g_strdup (name);
        info->type = type;
        info->data = data;

        g_ptr_array_add (priv->infos, info);
}

/**
 * arv_stream_get_n_infos:
 * @stream: a #ArvStream
 *
 * Returns: the number of stream informations. These informations contain useful numbers about data transfer quality.
 *
 * Since: 0.8.11
 */

guint
arv_stream_get_n_infos (ArvStream *stream)
{
        ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

        g_return_val_if_fail (ARV_IS_STREAM (stream), 0);

        return priv->infos->len;
}

/**
 * arv_stream_get_info_name:
 * @stream: a #ArvStream
 * @id: info id
 *
 * Returns: the name of the corresponding stream information.
 *
 * Since: 0.8.11
 */

const char *
arv_stream_get_info_name (ArvStream *stream, guint id)
{
        ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);
        const ArvStreamInfo *info;

        g_return_val_if_fail (ARV_IS_STREAM (stream), NULL);
        g_return_val_if_fail (id < priv->infos->len, NULL);

        info = g_ptr_array_index (priv->infos, id);
        if (info != NULL)
                return info->name;

        return NULL;
}

/**
 * arv_stream_get_info_type:
 * @stream: a #ArvStream
 * @id: info id
 *
 * Returns: the #GType of the corresponding stream information, which indicates what API to use to retrieve the
 * information value (arv_stream_get_info_uint64() or arv_stream_get_info_double()).
 *
 * Since: 0.8.11
 */

GType
arv_stream_get_info_type (ArvStream *stream, guint id)
{
        ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);
        const ArvStreamInfo *info;

        g_return_val_if_fail (ARV_IS_STREAM (stream), 0);
        g_return_val_if_fail (id < priv->infos->len, 0);

        info = g_ptr_array_index (priv->infos, id);
        if (info != NULL)
                return info->type;

        return 0;
}

/**
 * arv_stream_get_info_uint64:
 * @stream: a #ArvStream
 * @id: info id
 *
 * Returns: the value of the corresponding stream information, as a 64 bit unsigned integer.
 *
 * Since: 0.8.11
 */

guint64
arv_stream_get_info_uint64 (ArvStream *stream, guint id)
{
        ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);
        const ArvStreamInfo *info;

        g_return_val_if_fail (ARV_IS_STREAM (stream), 0);
        g_return_val_if_fail (id < priv->infos->len, 0);

        info = g_ptr_array_index (priv->infos, id);

        g_return_val_if_fail (info->type == G_TYPE_UINT64, 0);

        if (info != NULL)
                return *((guint64 *) (info->data));

        return 0;
}

/**
 * arv_stream_get_info_double:
 * @stream: a #ArvStream
 * @id: info id
 *
 * Returns: the value of the corresponding stream information, as a double.
 *
 * Since: 0.8.11
 */

double
arv_stream_get_info_double (ArvStream *stream, guint id)
{
        ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);
        const ArvStreamInfo *info;

        g_return_val_if_fail (ARV_IS_STREAM (stream), 0);
        g_return_val_if_fail (id < priv->infos->len, 0);

        info = g_ptr_array_index (priv->infos, id);

        g_return_val_if_fail (info->type == G_TYPE_DOUBLE, 0);

        if (info != NULL)
                return *((double *) (info->data));

        return 0;
}

static const ArvStreamInfo *
_find_info_by_name (ArvStream *stream, const char *name)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);
        guint i;

        for (i = 0; i < priv->infos->len; i++) {
                ArvStreamInfo *info = g_ptr_array_index (priv->infos, i);

                if (info != NULL && g_strcmp0 (name, info->name) == 0)
                        return info;
        }

        return NULL;
}

/**
 * arv_stream_get_info_uint64_by_name:
 * @stream: a #ArvStream
 * @name: info name
 *
 * Returns: the value of the corresponding stream information, as a 64 bit unsigned integer.
 *
 * Since: 0.8.11
 */

guint64
arv_stream_get_info_uint64_by_name (ArvStream *stream, const char *name)
{
        const ArvStreamInfo *info;

	g_return_val_if_fail (ARV_IS_STREAM (stream), 0);
        g_return_val_if_fail (name != NULL, 0);

        info = _find_info_by_name (stream, name);

        g_return_val_if_fail (info != NULL, 0);
        g_return_val_if_fail (info->type == G_TYPE_UINT64, 0);

        return *((guint64 *) (info->data));
}

/**
 * arv_stream_get_info_double_by_name:
 * @stream: a #ArvStream
 * @name: info name
 *
 * Returns: the value of the corresponding stream information, as a double.
 *
 * Since: 0.8.11
 */

double
arv_stream_get_info_double_by_name (ArvStream *stream, const char *name)
{
        const ArvStreamInfo *info;

	g_return_val_if_fail (ARV_IS_STREAM (stream), 0);
        g_return_val_if_fail (name != NULL, 0);

        info = _find_info_by_name (stream, name);

        g_return_val_if_fail (info != NULL, 0);
        g_return_val_if_fail (info->type == G_TYPE_DOUBLE, 0);

        return *((double *) (info->data));
}

gboolean
arv_stream_create_buffers (ArvStream *stream, unsigned int n_buffers,
                           void *user_data, GDestroyNotify user_data_destroy_func,
                           GError **error)
{
	ArvStreamClass *stream_class;
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);
        gboolean success;
        size_t payload_size;
        unsigned int i;

	g_return_val_if_fail (ARV_IS_STREAM (stream), FALSE);
	g_return_val_if_fail (n_buffers > 0, FALSE);
        g_return_val_if_fail (ARV_IS_DEVICE (priv->device), FALSE);

        payload_size = arv_device_get_integer_feature_value (priv->device, "PayloadSize", error);
        if (payload_size < 1)
                return FALSE;

	stream_class = ARV_STREAM_GET_CLASS (stream);
        if (stream_class->create_buffers != NULL) {
                success = stream_class->create_buffers (stream, n_buffers, payload_size,
                                                        user_data, user_data_destroy_func, error);
                if (!success)
                        arv_warning_stream ("Failed to create native buffers");
                return success;
        }

        for (i = 0; i < n_buffers; i++)
                arv_stream_push_buffer (stream, arv_buffer_new_full (payload_size, NULL,
                                                                     user_data, user_data_destroy_func));

        return TRUE;
}

static void
arv_stream_set_property (GObject * object, guint prop_id,
			 const GValue * value, GParamSpec * pspec)
{
	ArvStream *stream = ARV_STREAM (object);
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	switch (prop_id) {
		case ARV_STREAM_PROPERTY_EMIT_SIGNALS:
			arv_stream_set_emit_signals (stream, g_value_get_boolean (value));
			break;
		case ARV_STREAM_PROPERTY_DEVICE:
			g_clear_object (&priv->device);
			priv->device = g_value_dup_object (value);
			break;
		case ARV_STREAM_PROPERTY_CALLBACK:
			priv->callback = g_value_get_pointer (value);
			break;
		case ARV_STREAM_PROPERTY_CALLBACK_DATA:
			priv->callback_data = g_value_get_pointer (value);
			break;
		case ARV_STREAM_PROPERTY_DESTROY_NOTIFY:
			priv->destroy_notify = g_value_get_pointer (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_stream_get_property (GObject * object, guint prop_id,
			 GValue * value, GParamSpec * pspec)
{
	ArvStream *stream = ARV_STREAM (object);
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	switch (prop_id) {
		case ARV_STREAM_PROPERTY_EMIT_SIGNALS:
			g_value_set_boolean (value, arv_stream_get_emit_signals (stream));
			break;
		case ARV_STREAM_PROPERTY_DEVICE:
			g_value_set_object (value, priv->device);
			break;
		case ARV_STREAM_PROPERTY_CALLBACK:
			g_value_set_pointer (value, priv->callback);
			break;
		case ARV_STREAM_PROPERTY_CALLBACK_DATA:
			g_value_set_pointer (value, priv->callback_data);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

void
arv_stream_take_init_error (ArvStream *stream, GError *error)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	g_return_if_fail (ARV_IS_STREAM (stream));

	g_clear_error (&priv->init_error);
	priv->init_error = error;
}

static void
arv_stream_init (ArvStream *stream)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	priv->input_queue = g_async_queue_new ();
	priv->output_queue = g_async_queue_new ();

	priv->emit_signals = FALSE;

        priv->infos = g_ptr_array_new ();

	g_rec_mutex_init (&priv->mutex);
}

static void
arv_stream_finalize (GObject *object)
{
	ArvStream *stream = ARV_STREAM (object);
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	if (priv->emit_signals) {
		g_warning ("Stream finalized with 'new-buffer' signal enabled");
		g_warning ("Please call arv_stream_set_emit_signals (stream, FALSE) before ArvStream object finalization");
	}

        arv_stream_delete_buffers (stream);

	g_async_queue_unref (priv->input_queue);
	g_async_queue_unref (priv->output_queue);

	g_rec_mutex_clear (&priv->mutex);

	g_clear_object (&priv->device);

	g_clear_error (&priv->init_error);

        g_ptr_array_foreach (priv->infos, (GFunc) arv_stream_info_free, NULL);
        g_clear_pointer (&priv->infos, g_ptr_array_unref);

	if (priv->destroy_notify != NULL) {
		priv->destroy_notify(priv->callback_data);
	}

	G_OBJECT_CLASS (arv_stream_parent_class)->finalize (object);
}

static void
arv_stream_class_init (ArvStreamClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	object_class->finalize = arv_stream_finalize;
	object_class->set_property = arv_stream_set_property;
	object_class->get_property = arv_stream_get_property;

	/**
	 * ArvStream::new-buffer:
	 * @stream: the stream that emited the signal
	 *
	 * Signal that a new buffer is available.
	 *
	 * This signal is emited from the stream receive thread and only when the
	 * "emit-signals" property is %TRUE.
	 *
	 * The new buffer can be retrieved with arv_stream_pop_buffer().
	 *
	 * Note that this signal is only emited when the "emit-signals" property is
	 * set to %TRUE, which it is not by default for performance reasons.
	 *
	 * Since: 0.2.0
	 */

	arv_stream_signals[ARV_STREAM_SIGNAL_NEW_BUFFER] =
		g_signal_new ("new-buffer",
			      G_TYPE_FROM_CLASS (node_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ArvStreamClass, new_buffer),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE);

	g_object_class_install_property (
		object_class, ARV_STREAM_PROPERTY_EMIT_SIGNALS,
		g_param_spec_boolean ("emit-signals", "Emit signals",
				      "Emit signals", FALSE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
		);
	g_object_class_install_property
		(object_class,
		 ARV_STREAM_PROPERTY_DEVICE,
		 g_param_spec_object ("device",
				      "Paret device",
				      "A ArvDevice parent object",
				      ARV_TYPE_DEVICE,
				      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property
		(object_class,
		 ARV_STREAM_PROPERTY_CALLBACK,
		 g_param_spec_pointer ("callback",
				       "Stream callback",
				       "Optional user callback",
				       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property
		(object_class,
		 ARV_STREAM_PROPERTY_CALLBACK_DATA,
		 g_param_spec_pointer ("callback-data",
				       "Stream callback data",
				       "Optional user callback data",
				       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property
		(object_class,
		 ARV_STREAM_PROPERTY_DESTROY_NOTIFY,
		 g_param_spec_pointer ("destroy-notify",
				       "Destroy notify",
				       "Optional destroy notify",
				       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static gboolean
arv_stream_initable_init (GInitable     *initable,
			  GCancellable  *cancellable,
			  GError       **error)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (ARV_STREAM (initable));

	g_return_val_if_fail (ARV_IS_STREAM (initable), FALSE);

	if (cancellable != NULL)
	{
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
				     "Cancellable initialization not supported");
		return FALSE;
	}

	if (priv->init_error) {
		if (error != NULL)
			*error = g_error_copy (priv->init_error);
		return FALSE;
	}

	return TRUE;
}

static void
arv_stream_initable_iface_init (GInitableIface *iface)
{
	iface->init = arv_stream_initable_init;
}

