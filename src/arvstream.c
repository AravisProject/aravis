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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <arvstream.h>
#include <arvbuffer.h>
#include <arvdebug.h>

static GObjectClass *parent_class = NULL;

/**
 * arv_stream_push_buffer:
 * @stream: a #ArvStream
 * @buffer: (transfer full): buffer to push
 *
 * Pushes a #ArvBuffer to the @stream thread. The @stream takes ownership of @buffer,
 * and will free all the buffers still in its queues when destroyed.
 */

void
arv_stream_push_buffer (ArvStream *stream, ArvBuffer *buffer)
{
	g_return_if_fail (ARV_IS_STREAM (stream));
	g_return_if_fail (ARV_IS_BUFFER (buffer));

	g_async_queue_push (stream->input_queue, buffer);
}

/**
 * arv_stream_pop_buffer:
 * @stream: a #ArvStream
 * Returns: a #ArvBuffer
 *
 * Pops a buffer from the output queue of @stream. The retrieved buffer
 * may contain an invalid image. Caller should check the buffer status before using it.
 */

ArvBuffer *
arv_stream_pop_buffer (ArvStream *stream)
{
	g_return_val_if_fail (ARV_IS_STREAM (stream), NULL);

	return g_async_queue_try_pop (stream->output_queue);
}

int
arv_stream_get_n_available_buffers (ArvStream *stream)
{
	g_return_val_if_fail (ARV_IS_STREAM (stream), 0);

	return g_async_queue_length (stream->output_queue);
}

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

static void
arv_stream_init (ArvStream *stream)
{
	stream->input_queue = g_async_queue_new ();
	stream->output_queue = g_async_queue_new ();
}

static void
arv_stream_finalize (GObject *object)
{
	ArvStream *stream = ARV_STREAM (object);
	ArvBuffer *buffer;

	arv_debug ("stream",
		   "[Stream::finalize] Flush %d buffer[s] in input queue",
		   g_async_queue_length (stream->input_queue));
	arv_debug ("stream",
		   "[Stream::finalize] Flush %d buffer[s] in output queue",
		   g_async_queue_length (stream->output_queue));

	do {
		buffer = g_async_queue_try_pop (stream->output_queue);
		if (buffer != NULL)
			g_object_unref (buffer);
	} while (buffer != NULL);

	do {
		buffer = g_async_queue_try_pop (stream->input_queue);
		if (buffer != NULL)
			g_object_unref (buffer);
	} while (buffer != NULL);

	g_async_queue_unref (stream->input_queue);
	g_async_queue_unref (stream->output_queue);

	parent_class->finalize (object);
}

static void
arv_stream_class_init (ArvStreamClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_stream_finalize;
}

G_DEFINE_ABSTRACT_TYPE (ArvStream, arv_stream, G_TYPE_OBJECT)

