#include <arvgvdevice.h>

static GObjectClass *parent_class = NULL;

ArvDevice *
arv_gv_device_new_with_address (const char *address)
{
	ArvGvDevice *device;

	g_return_val_if_fail (address != NULL, NULL);


	device = g_object_new (ARV_TYPE_GV_DEVICE, NULL);
	device->inet_address = g_inet_address_new_from_string (address);

	return ARV_DEVICE (device);
}

static void
arv_gv_device_init (ArvGvDevice *gv_device)
{
}

static void
arv_gv_device_finalize (GObject *object)
{
	ArvGvDevice *device = ARV_GV_DEVICE (object);

	g_object_unref (device->inet_address);

	parent_class->finalize (object);
}

static void
arv_gv_device_class_init (ArvGvDeviceClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_gv_device_finalize;
}

G_DEFINE_TYPE (ArvGvDevice, arv_gv_device, ARV_TYPE_DEVICE)
