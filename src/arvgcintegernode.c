#include <arvgcintegernode.h>

static GObjectClass *parent_class = NULL;

ArvGcNode *
arv_gc_integer_node_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_INTEGER_NODE, NULL);

	return node;
}

static void
arv_gc_integer_node_init (ArvGcIntegerNode *gc_integer_node)
{
}

static void
arv_gc_integer_node_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_gc_integer_node_class_init (ArvGcIntegerNodeClass *integer_node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (integer_node_class);

	parent_class = g_type_class_peek_parent (integer_node_class);

	object_class->finalize = arv_gc_integer_node_finalize;
}

G_DEFINE_TYPE (ArvGcIntegerNode, arv_gc_integer_node, ARV_TYPE_GC_NODE)
