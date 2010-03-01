#include <arvstream.h>
#include <arvdebug.h>

static GObjectClass *parent_class = NULL;

void
arv_stream_push_buffer (ArvStream *stream, ArvBuffer *buffer)
{
	g_return_if_fail (ARV_IS_STREAM (stream));
	g_return_if_fail (ARV_IS_BUFFER (buffer));

	g_async_queue_push (stream->input_queue, buffer);
}

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

	arv_debug (ARV_DEBUG_LEVEL_STANDARD,
		   "[Stream::finalize] Flush %d buffer[s] in input queue",
		   g_async_queue_length (stream->input_queue));
	arv_debug (ARV_DEBUG_LEVEL_STANDARD,
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

