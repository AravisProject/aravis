#include <arvdevice.h>

static GObjectClass *parent_class = NULL;

size_t
arv_device_read_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), 0);
	g_return_val_if_fail (buffer != NULL, 0);
	g_return_val_if_fail (size > 0, 0);

	return ARV_DEVICE_GET_CLASS (device)->read_memory (device, address, size, buffer);
}

size_t
arv_device_write_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), 0);
	g_return_val_if_fail (buffer != NULL, 0);
	g_return_val_if_fail (size > 0, 0);

	return ARV_DEVICE_GET_CLASS (device)->write_memory (device, address, size, buffer);
}

guint32
arv_device_read_register (ArvDevice *device, guint32 address)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), 0);

	return ARV_DEVICE_GET_CLASS (device)->read_register (device, address);
}

void
arv_device_write_register (ArvDevice *device, guint32 address, guint32 value)
{
	g_return_if_fail (ARV_IS_DEVICE (device));

	return ARV_DEVICE_GET_CLASS (device)->write_register (device, address, value);
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
