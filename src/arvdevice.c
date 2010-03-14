/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <arvdevice.h>

static GObjectClass *parent_class = NULL;

struct _ArvDevicePrivate {
	ArvGc *genicam;
	char *genicam_data;
	size_t genicam_size;

	ArvStream *stream;
};

ArvStream *
arv_device_get_stream (ArvDevice *device)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	if (device->priv->stream != NULL) {
		g_object_ref (device->priv->stream);
		return device->priv->stream;
	}

	device->priv->stream = ARV_DEVICE_GET_CLASS (device)->create_stream (device);

	return device->priv->stream;
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
arv_device_set_genicam_data (ArvDevice *device, char *genicam, size_t size)
{
	g_return_if_fail (ARV_IS_DEVICE (device));

	g_free (device->priv->genicam_data);
	if (device->priv->genicam != NULL)
		g_object_unref (device->priv->genicam);
	device->priv->genicam_data = genicam;
	device->priv->genicam_size = size;

	device->priv->genicam = arv_gc_new (device,
					    device->priv->genicam_data,
					    device->priv->genicam_size);
}

const char *
arv_device_get_genicam_data (ArvDevice *device, size_t *size)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	if (size != NULL)
		*size = device->priv->genicam_size;

	return device->priv->genicam_data;
}

ArvGc *
arv_device_get_genicam (ArvDevice *device)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	return device->priv->genicam;
}

static void
arv_device_init (ArvDevice *device)
{
	device->priv = G_TYPE_INSTANCE_GET_PRIVATE (device, ARV_TYPE_DEVICE, ArvDevicePrivate);
}

static void
arv_device_finalize (GObject *object)
{
	ArvDevice *device = ARV_DEVICE (object);

	if (ARV_IS_STREAM (device->priv->stream))
		g_object_unref (device->priv->stream);

	g_free (device->priv->genicam_data);
	if (device->priv->genicam != NULL)
		g_object_unref (device->priv->genicam);

	parent_class->finalize (object);
}

static void
arv_device_class_init (ArvDeviceClass *device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (device_class);

	g_type_class_add_private (device_class, sizeof (ArvDevicePrivate));

	parent_class = g_type_class_peek_parent (device_class);

	object_class->finalize = arv_device_finalize;
}

G_DEFINE_ABSTRACT_TYPE (ArvDevice, arv_device, G_TYPE_OBJECT)
