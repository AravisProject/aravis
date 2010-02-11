#include <arvinterface.h>

static GObjectClass *parent_class = NULL;

void
arv_interface_get_devices (ArvInterface *interface)
{
	ArvInterfaceClass *interface_class = ARV_INTERFACE_GET_CLASS (interface);

	interface_class->get_devices (interface);
}

static void
arv_interface_init (ArvInterface *interface)
{
}

static void
arv_interface_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_interface_class_init (ArvInterfaceClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_interface_finalize;
}

G_DEFINE_ABSTRACT_TYPE (ArvInterface, arv_interface, G_TYPE_OBJECT)
