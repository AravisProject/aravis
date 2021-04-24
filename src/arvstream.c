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
	ARV_STREAM_PROPERTY_CALLBACK_DATA
} ArvStreamProperties;

typedef struct {
	GAsyncQueue *input_queue;
	GAsyncQueue *output_queue;
	GRecMutex mutex;
	gboolean emit_signals;

	ArvDevice *device;
	ArvStreamCallback callback;
	void *callback_data;

	GError *init_error;
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

	g_return_val_if_fail (ARV_IS_STREAM (stream), NULL);

	return g_async_queue_try_pop (priv->input_queue);
}

void
arv_stream_push_output_buffer (ArvStream *stream, ArvBuffer *buffer)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	g_return_if_fail (ARV_IS_STREAM (stream));
	g_return_if_fail (ARV_IS_BUFFER (buffer));

	g_async_queue_push (priv->output_queue, buffer);

	g_rec_mutex_lock (&priv->mutex);

	if (priv->emit_signals)
		g_signal_emit (stream, arv_stream_signals[ARV_STREAM_SIGNAL_NEW_BUFFER], 0);

	g_rec_mutex_unlock (&priv->mutex);
}

/**
 * arv_stream_get_n_buffers:
 * @stream: a #ArvStream
 * @n_input_buffers: (out) (allow-none): input queue length
 * @n_output_buffers: (out) (allow-none): output queue length
 *
 * An accessor to the stream buffer queue lengths.
 *
 * Since: 0.2.0
 */

void
arv_stream_get_n_buffers (ArvStream *stream, gint *n_input_buffers, gint *n_output_buffers)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);

	if (!ARV_IS_STREAM (stream)) {
		if (n_input_buffers != NULL)
			*n_input_buffers = 0;
		if (n_output_buffers != NULL)
			*n_output_buffers = 0;
		return;
	}

	if (n_input_buffers != NULL)
		*n_input_buffers = g_async_queue_length (priv->input_queue);
	if (n_output_buffers != NULL)
		*n_output_buffers = g_async_queue_length (priv->output_queue);
}

/**
 * arv_stream_start_thread:
 * @stream: a #ArvStream
 *
 * Start the stream receiving thread. The thread is automatically started when
 * the #ArvStream object is instantiated, so this function is only useful if
 * the thread was stopped using @arv_stream_stop_thread.
 *
 * Since: 0.6.2
 */

void
arv_stream_start_thread (ArvStream *stream)
{
	ArvStreamClass *stream_class;

	g_return_if_fail (ARV_IS_STREAM (stream));

	stream_class = ARV_STREAM_GET_CLASS (stream);
	g_return_if_fail (stream_class->start_thread != NULL);

	stream_class->start_thread (stream);
}

/**
 * arv_stream_stop_thread:
 * @stream: a #ArvStream
 * @delete_buffers: enable buffer deletion
 *
 * Stop the stream receiving thread, and optionally delete all the #ArvBuffer
 * stored in the stream object queues. Main use of this function is to be able
 * to quickly change an acquisition parameter that changes the payload size,
 * without deleting/recreating the stream object.
 *
 * Returns: the number of deleted buffers if @delete_buffers == %TRUE, 0 otherwise.
 *
 * Since: 0.6.2
 */

unsigned int
arv_stream_stop_thread (ArvStream *stream, gboolean delete_buffers)
{
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);
	ArvStreamClass *stream_class;
	ArvBuffer *buffer;
	unsigned int n_deleted = 0;

	g_return_val_if_fail (ARV_IS_STREAM (stream), 0);

	stream_class = ARV_STREAM_GET_CLASS (stream);
	g_return_val_if_fail (stream_class->stop_thread != NULL, 0);

	stream_class->stop_thread (stream);

	if (!delete_buffers)
		return 0;

	g_async_queue_lock (priv->input_queue);
	do {
		buffer = g_async_queue_try_pop_unlocked (priv->input_queue);
		if (buffer != NULL) {
			g_object_unref (buffer);
			n_deleted++;
		}
	} while (buffer != NULL);
	g_async_queue_unlock (priv->input_queue);

	g_async_queue_lock (priv->output_queue);
	do {
		buffer = g_async_queue_try_pop_unlocked (priv->output_queue);
		if (buffer != NULL) {
			g_object_unref (buffer);
			n_deleted++;
		}
	} while (buffer != NULL);
	g_async_queue_unlock (priv->output_queue);

	arv_info_stream ("[Stream::reset] Deleted %u buffers\n", n_deleted);

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
	ArvStreamClass *stream_class;
	guint64 dummy;

	if (n_completed_buffers == NULL)
		n_completed_buffers = &dummy;
	if (n_failures == NULL)
		n_failures = &dummy;
	if (n_underruns == NULL)
		n_underruns = &dummy;

	*n_completed_buffers = 0;
	*n_failures = 0;
	*n_underruns = 0;

	g_return_if_fail (ARV_IS_STREAM (stream));

	stream_class = ARV_STREAM_GET_CLASS (stream);
	if (stream_class->get_statistics != NULL)
		stream_class->get_statistics (stream, n_completed_buffers, n_failures, n_underruns);
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

	g_rec_mutex_init (&priv->mutex);
}

static void
arv_stream_finalize (GObject *object)
{
	ArvStream *stream = ARV_STREAM (object);
	ArvStreamPrivate *priv = arv_stream_get_instance_private (stream);
	ArvBuffer *buffer;

	arv_info_stream ("[Stream::finalize] Flush %d buffer[s] in input queue",
			  g_async_queue_length (priv->input_queue));
	arv_info_stream ("[Stream::finalize] Flush %d buffer[s] in output queue",
			  g_async_queue_length (priv->output_queue));

	if (priv->emit_signals) {
		g_warning ("Stream finalized with 'new-buffer' signal enabled");
		g_warning ("Please call arv_stream_set_emit_signals (stream, FALSE) before ArvStream object finalization");
	}

	do {
		buffer = g_async_queue_try_pop (priv->output_queue);
		if (buffer != NULL)
			g_object_unref (buffer);
	} while (buffer != NULL);

	do {
		buffer = g_async_queue_try_pop (priv->input_queue);
		if (buffer != NULL)
			g_object_unref (buffer);
	} while (buffer != NULL);

	g_async_queue_unref (priv->input_queue);
	g_async_queue_unref (priv->output_queue);

	g_rec_mutex_clear (&priv->mutex);

	g_clear_object (&priv->device);

	g_clear_error (&priv->init_error);

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

