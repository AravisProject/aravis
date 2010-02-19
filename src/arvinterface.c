#include <arvinterface.h>

static GObjectClass *parent_class = NULL;

void
arv_interface_update_device_list (ArvInterface *interface)
{
	g_return_if_fail (ARV_IS_INTERFACE (interface));

	ARV_INTERFACE_GET_CLASS (interface)->update_device_list (interface);
}

ArvDevice *
arv_interface_get_device (ArvInterface *interface, int property, const char *value)
{
	ArvDevice *device;

	g_return_val_if_fail (ARV_IS_INTERFACE (interface), NULL);

	device = ARV_INTERFACE_GET_CLASS (interface)->get_device (interface, property, value);

	if (device != NULL)
		return device;

	arv_interface_update_device_list (interface);

	return ARV_INTERFACE_GET_CLASS (interface)->get_device (interface, property, value);
}

ArvDevice *
arv_interface_get_first_device (ArvInterface *interface)
{
	return arv_interface_get_device (interface, 0, NULL);
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
