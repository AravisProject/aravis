#include <arvgenicamnode.h>

static GObjectClass *parent_class = NULL;

static void
arv_genicam_node_init (ArvGenicamNode *genicam_node)
{
}

static void
arv_genicam_node_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_genicam_node_class_init (ArvGenicamNodeClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_genicam_node_finalize;
}

G_DEFINE_TYPE (ArvGenicamNode, arv_genicam_node, G_TYPE_OBJECT)
