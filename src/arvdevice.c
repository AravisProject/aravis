#include <arvdevice.h>

static GObjectClass *parent_class = NULL;

size_t
arv_device_read (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), 0);
	g_return_val_if_fail (buffer != NULL, 0);
	g_return_val_if_fail (size > 0, 0);

	return ARV_DEVICE_GET_CLASS (device)->read (device, address, size, buffer);
}

size_t
arv_device_write (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), 0);
	g_return_val_if_fail (buffer != NULL, 0);
	g_return_val_if_fail (size > 0, 0);

	return ARV_DEVICE_GET_CLASS (device)->write (device, address, size, buffer);
}

void
arv_device_load_genicam (ArvDevice *device)
{
}

static void
arv_device_init (ArvDevice *device)
{
}

static void
arv_device_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_device_class_init (ArvDeviceClass *device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (device_class);

	parent_class = g_type_class_peek_parent (device_class);

	object_class->finalize = arv_device_finalize;
}

G_DEFINE_ABSTRACT_TYPE (ArvDevice, arv_device, G_TYPE_OBJECT)
