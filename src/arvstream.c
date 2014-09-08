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
#include <arvdebug.h>

enum {
	ARV_STREAM_SIGNAL_NEW_BUFFER,
	ARV_STREAM_SIGNAL_LAST
} ArvStreamSignals;

static guint arv_stream_signals[ARV_STREAM_SIGNAL_LAST] = {0};

enum {
	ARV_STREAM_PROPERTY_0,
	ARV_STREAM_PROPERTY_EMIT_SIGNALS,
	ARV_STREAM_PROPERTY_LAST
} ArvStreamProperties;

static GObjectClass *parent_class = NULL;

struct _ArvStreamPrivate {
	GAsyncQueue *input_queue;
	GAsyncQueue *output_queue;

	gboolean emit_signals;
};

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
	g_return_if_fail (ARV_IS_STREAM (stream));
	g_return_if_fail (ARV_IS_BUFFER (buffer));

	g_async_queue_push (stream->priv->input_queue, buffer);
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
	g_return_val_if_fail (ARV_IS_STREAM (stream), NULL);

	return g_async_queue_pop (stream->priv->output_queue);
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
	g_return_val_if_fail (ARV_IS_STREAM (stream), NULL);

	return g_async_queue_try_pop (stream->priv->output_queue);
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
#if GLIB_CHECK_VERSION(2,32,0)
	g_return_val_if_fail (ARV_IS_STREAM (stream), NULL);

	return g_async_queue_timeout_pop (stream->priv->output_queue, timeout);
#else
	GTimeVal end_time;

	g_return_val_if_fail (ARV_IS_STREAM (stream), NULL);

	g_get_current_time (&end_time);
	g_time_val_add (&end_time, timeout);

	return g_async_queue_timed_pop (stream->priv->output_queue, &end_time);
#endif
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
	g_return_val_if_fail (ARV_IS_STREAM (stream), NULL);

	return g_async_queue_try_pop (stream->priv->input_queue);
}

void
arv_stream_push_output_buffer (ArvStream *stream, ArvBuffer *buffer)
{
	g_return_if_fail (ARV_IS_STREAM (stream));
	g_return_if_fail (ARV_IS_BUFFER (buffer));

	g_async_queue_push (stream->priv->output_queue, buffer);

	if (stream->priv->emit_signals)
		g_signal_emit (stream, arv_stream_signals[ARV_STREAM_SIGNAL_NEW_BUFFER], 0);
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
	if (!ARV_IS_STREAM (stream)) {
		if (n_input_buffers != NULL)
			*n_input_buffers = 0;
		if (n_output_buffers != NULL)
			*n_output_buffers = 0;
		return;
	}

	if (n_input_buffers != NULL)
		*n_input_buffers = g_async_queue_length (stream->priv->input_queue);
	if (n_output_buffers != NULL)
		*n_output_buffers = g_async_queue_length (stream->priv->output_queue);
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
	g_return_if_fail (ARV_IS_STREAM (stream));

	stream->priv->emit_signals = emit_signals;
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
	g_return_val_if_fail (ARV_IS_STREAM (stream), FALSE);

	return stream->priv->emit_signals;
}

static void
arv_stream_set_property (GObject * object, guint prop_id,
			 const GValue * value, GParamSpec * pspec)
{
	ArvStream *stream = ARV_STREAM (object);

	switch (prop_id) {
		case ARV_STREAM_PROPERTY_EMIT_SIGNALS:
			arv_stream_set_emit_signals (stream, g_value_get_boolean (value));
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

	switch (prop_id) {
		case ARV_STREAM_PROPERTY_EMIT_SIGNALS:
			g_value_set_boolean (value, arv_stream_get_emit_signals (stream));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_stream_init (ArvStream *stream)
{
	stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream, ARV_TYPE_STREAM, ArvStreamPrivate);

	stream->priv->input_queue = g_async_queue_new ();
	stream->priv->output_queue = g_async_queue_new ();

	stream->priv->emit_signals = FALSE;
}

static void
arv_stream_finalize (GObject *object)
{
	ArvStream *stream = ARV_STREAM (object);
	ArvBuffer *buffer;

	arv_debug_stream ("[Stream::finalize] Flush %d buffer[s] in input queue",
			  g_async_queue_length (stream->priv->input_queue));
	arv_debug_stream ("[Stream::finalize] Flush %d buffer[s] in output queue",
			  g_async_queue_length (stream->priv->output_queue));

	do {
		buffer = g_async_queue_try_pop (stream->priv->output_queue);
		if (buffer != NULL)
			g_object_unref (buffer);
	} while (buffer != NULL);

	do {
		buffer = g_async_queue_try_pop (stream->priv->input_queue);
		if (buffer != NULL)
			g_object_unref (buffer);
	} while (buffer != NULL);

	g_async_queue_unref (stream->priv->input_queue);
	g_async_queue_unref (stream->priv->output_queue);

	parent_class->finalize (object);
}

static void
arv_stream_class_init (ArvStreamClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	g_type_class_add_private (node_class, sizeof (ArvStreamPrivate));

	parent_class = g_type_class_peek_parent (node_class);

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
}

G_DEFINE_ABSTRACT_TYPE (ArvStream, arv_stream, G_TYPE_OBJECT)

