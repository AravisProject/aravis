/* Aravis - Digital camera library
 *
 * Copyright © 2009-2016 Emmanuel Pacaud
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvfakedevice
 * @short_description: Fake device
 */

#include <arvfakedeviceprivate.h>
#include <arvfakestreamprivate.h>
#include <arvgc.h>
#include <arvdebug.h>

static GObjectClass *parent_class = NULL;

struct _ArvFakeDevicePrivate {
	ArvFakeCamera *camera;
	ArvGc *genicam;

	const char *genicam_xml;
	size_t genicam_xml_size;
};

/* ArvFakeDevice implemenation */

/* ArvDevice implemenation */

static ArvStream *
arv_fake_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data)
{
	ArvFakeDevice *fake_device = ARV_FAKE_DEVICE (device);
	ArvStream *stream;

	stream = arv_fake_stream_new (fake_device->priv->camera, callback, user_data);

	return stream;
}

static const char *
arv_fake_device_get_genicam_xml (ArvDevice *device, size_t *size)
{
	ArvFakeDevice *fake_device = ARV_FAKE_DEVICE (device);

	*size = fake_device->priv->genicam_xml_size;

	return fake_device->priv->genicam_xml;
}

static ArvGc *
arv_fake_device_get_genicam (ArvDevice *device)
{
	ArvFakeDevice *fake_device = ARV_FAKE_DEVICE (device);

	return fake_device->priv->genicam;
}

static gboolean
arv_fake_device_read_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	return arv_fake_camera_read_memory (ARV_FAKE_DEVICE (device)->priv->camera, address, size, buffer);
}

static gboolean
arv_fake_device_write_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	return arv_fake_camera_write_memory (ARV_FAKE_DEVICE (device)->priv->camera, address, size, buffer);
}

static gboolean
arv_fake_device_read_register (ArvDevice *device, guint64 address, guint32 *value, GError **error)
{
	return arv_fake_camera_read_register (ARV_FAKE_DEVICE (device)->priv->camera, address, value);
}

static gboolean
arv_fake_device_write_register (ArvDevice *device, guint64 address, guint32 value, GError **error)
{
	return arv_fake_camera_write_register (ARV_FAKE_DEVICE (device)->priv->camera, address, value);
}

/**
 * arv_fake_device_get_fake_camera:
 * @device: a fake device
 *
 * Return value: (transfer none): the #ArvFakeCamera used by this device instance.
 */

ArvFakeCamera *
arv_fake_device_get_fake_camera	(ArvFakeDevice *device)
{
	g_return_val_if_fail (ARV_IS_FAKE_DEVICE (device), NULL);

	return device->priv->camera;
}

ArvDevice *
arv_fake_device_new (const char *serial_number)
{
	ArvFakeDevice *fake_device;

	g_return_val_if_fail (serial_number != NULL, NULL);

	fake_device = g_object_new (ARV_TYPE_FAKE_DEVICE, NULL);

	fake_device->priv->camera = arv_fake_camera_new (serial_number);

	fake_device->priv->genicam_xml = arv_get_fake_camera_genicam_xml (&fake_device->priv->genicam_xml_size);
	fake_device->priv->genicam = arv_gc_new (ARV_DEVICE (fake_device),
						 fake_device->priv->genicam_xml,
						 fake_device->priv->genicam_xml_size);

	return ARV_DEVICE (fake_device);
}

static void
arv_fake_device_init (ArvFakeDevice *fake_device)
{
	fake_device->priv = G_TYPE_INSTANCE_GET_PRIVATE (fake_device, ARV_TYPE_FAKE_DEVICE, ArvFakeDevicePrivate);
}

static void
arv_fake_device_finalize (GObject *object)
{
	ArvFakeDevice *fake_device = ARV_FAKE_DEVICE (object);

	g_object_unref (fake_device->priv->genicam);
	g_object_unref (fake_device->priv->camera);

	parent_class->finalize (object);
}

static void
arv_fake_device_class_init (ArvFakeDeviceClass *fake_device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (fake_device_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (fake_device_class);

	g_type_class_add_private (fake_device_class, sizeof (ArvFakeDevicePrivate));

	parent_class = g_type_class_peek_parent (fake_device_class);

	object_class->finalize = arv_fake_device_finalize;

	device_class->create_stream = arv_fake_device_create_stream;
	device_class->get_genicam_xml = arv_fake_device_get_genicam_xml;
	device_class->get_genicam = arv_fake_device_get_genicam;
	device_class->read_memory = arv_fake_device_read_memory;
	device_class->write_memory = arv_fake_device_write_memory;
	device_class->read_register = arv_fake_device_read_register;
	device_class->write_register = arv_fake_device_write_register;
}

G_DEFINE_TYPE (ArvFakeDevice, arv_fake_device, ARV_TYPE_DEVICE)
