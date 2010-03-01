#include <arvdevice.h>

static GObjectClass *parent_class = NULL;

ArvStream *
arv_device_get_stream (ArvDevice *device)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	if (device->stream != NULL) {
		g_object_ref (device->stream);
		return device->stream;
	}

	device->stream = ARV_DEVICE_GET_CLASS (device)->create_stream (device);

	return device->stream;
}

gboolean
arv_device_read_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (buffer != NULL, FALSE);
	g_return_val_if_fail (size > 0, FALSE);

	return ARV_DEVICE_GET_CLASS (device)->read_memory (device, address, size, buffer);
}

gboolean
arv_device_write_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (buffer != NULL, FALSE);
	g_return_val_if_fail (size > 0, FALSE);

	return ARV_DEVICE_GET_CLASS (device)->write_memory (device, address, size, buffer);
}

gboolean
arv_device_read_register (ArvDevice *device, guint32 address, guint32 *value)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	return ARV_DEVICE_GET_CLASS (device)->read_register (device, address, value);
}

gboolean
arv_device_write_register (ArvDevice *device, guint32 address, guint32 value)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);

	return ARV_DEVICE_GET_CLASS (device)->write_register (device, address, value);
}

void
arv_device_set_genicam (ArvDevice *device, char *genicam)
{
	g_return_if_fail (ARV_IS_DEVICE (device));

	g_free (device->genicam);
	device->genicam = genicam;
}

static void
arv_device_init (ArvDevice *device)
{
}

static void
arv_device_finalize (GObject *object)
{
	ArvDevice *device = ARV_DEVICE (object);

	if (ARV_IS_STREAM (device->stream))
		g_object_unref (device->stream);

	g_free (device->genicam);

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
