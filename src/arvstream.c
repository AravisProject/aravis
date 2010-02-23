#include <arvstream.h>

static GObjectClass *parent_class = NULL;

static void
arv_stream_init (ArvStream *stream)
{
}

static void
arv_stream_finalize (GObject *object)
{
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

