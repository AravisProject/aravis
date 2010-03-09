#include <arvgcportnode.h>

static GObjectClass *parent_class = NULL;

ArvGcNode *
arv_gc_port_node_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_PORT_NODE, NULL);

	return node;
}

static void
arv_gc_port_node_init (ArvGcPortNode *gc_port_node)
{
}

static void
arv_gc_port_node_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_gc_port_node_class_init (ArvGcPortNodeClass *port_node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (port_node_class);

	parent_class = g_type_class_peek_parent (port_node_class);

	object_class->finalize = arv_gc_port_node_finalize;
}

G_DEFINE_TYPE (ArvGcPortNode, arv_gc_port_node, ARV_TYPE_GC_NODE)
