#include <arvgcregisternode.h>

static GObjectClass *parent_class = NULL;

ArvGcNode *
arv_gc_register_node_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_REGISTER_NODE, NULL);

	return node;
}

static void
arv_gc_register_node_init (ArvGcRegisterNode *gc_register_node)
{
}

static void
arv_gc_register_node_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_gc_register_node_class_init (ArvGcRegisterNodeClass *register_node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (register_node_class);

	parent_class = g_type_class_peek_parent (register_node_class);

	object_class->finalize = arv_gc_register_node_finalize;
}

G_DEFINE_TYPE (ArvGcRegisterNode, arv_gc_register_node, ARV_TYPE_GC_NODE)
