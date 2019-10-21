/* Aravis - Digital camera library
 *
 * Copyright © 2009-2019 Emmanuel Pacaud
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
#include <arvfakecamera.h>
#include <arvgc.h>
#include <arvdebug.h>

typedef struct {
	ArvFakeCamera *camera;
	ArvGc *genicam;
} ArvFakeDevicePrivate;

struct _ArvFakeDevice {
	ArvDevice device;
};

struct _ArvFakeDeviceClass {
	ArvDeviceClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvFakeDevice, arv_fake_device, ARV_TYPE_DEVICE, G_ADD_PRIVATE (ArvFakeDevice))

/* ArvFakeDevice implemenation */

/* ArvDevice implemenation */

static ArvStream *
arv_fake_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data)
{
	ArvFakeDevicePrivate *priv = arv_fake_device_get_instance_private (ARV_FAKE_DEVICE (device));

	return arv_fake_stream_new (priv->camera, callback, user_data);
}

static const char *
arv_fake_device_get_genicam_xml (ArvDevice *device, size_t *size)
{
	ArvFakeDevicePrivate *priv = arv_fake_device_get_instance_private (ARV_FAKE_DEVICE (device));

	return arv_fake_camera_get_genicam_xml (priv->camera, size);
}

static ArvGc *
arv_fake_device_get_genicam (ArvDevice *device)
{
	ArvFakeDevicePrivate *priv = arv_fake_device_get_instance_private (ARV_FAKE_DEVICE (device));

	return priv->genicam;
}

static gboolean
arv_fake_device_read_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	ArvFakeDevicePrivate *priv = arv_fake_device_get_instance_private (ARV_FAKE_DEVICE (device));

	return arv_fake_camera_read_memory (priv->camera, address, size, buffer);
}

static gboolean
arv_fake_device_write_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	ArvFakeDevicePrivate *priv = arv_fake_device_get_instance_private (ARV_FAKE_DEVICE (device));

	return arv_fake_camera_write_memory (priv->camera, address, size, buffer);
}

static gboolean
arv_fake_device_read_register (ArvDevice *device, guint64 address, guint32 *value, GError **error)
{
	ArvFakeDevicePrivate *priv = arv_fake_device_get_instance_private (ARV_FAKE_DEVICE (device));

	return arv_fake_camera_read_register (priv->camera, address, value);
}

static gboolean
arv_fake_device_write_register (ArvDevice *device, guint64 address, guint32 value, GError **error)
{
	ArvFakeDevicePrivate *priv = arv_fake_device_get_instance_private (ARV_FAKE_DEVICE (device));

	return arv_fake_camera_write_register (priv->camera, address, value);
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
	ArvFakeDevicePrivate *priv = arv_fake_device_get_instance_private (ARV_FAKE_DEVICE (device));

	g_return_val_if_fail (ARV_IS_FAKE_DEVICE (device), NULL);

	return priv->camera;
}

ArvDevice *
arv_fake_device_new (const char *serial_number)
{
	ArvFakeDevicePrivate *priv;
	ArvFakeDevice *fake_device;
	ArvFakeCamera *fake_camera;
	const char *genicam_xml;
	gsize genicam_xml_size;

	g_return_val_if_fail (serial_number != NULL, NULL);

	fake_camera = arv_fake_camera_new_full (serial_number, NULL);
	genicam_xml = arv_fake_camera_get_genicam_xml (fake_camera, &genicam_xml_size);

	fake_device = g_object_new (ARV_TYPE_FAKE_DEVICE, NULL);

	priv = arv_fake_device_get_instance_private (fake_device);
	priv->camera = fake_camera;
	priv->genicam = arv_gc_new (ARV_DEVICE (fake_device), genicam_xml, genicam_xml_size);

	return ARV_DEVICE (fake_device);
}

static void
arv_fake_device_init (ArvFakeDevice *fake_device)
{
}

static void
arv_fake_device_finalize (GObject *object)
{
	ArvFakeDevicePrivate *priv = arv_fake_device_get_instance_private (ARV_FAKE_DEVICE (object));

	g_object_unref (priv->genicam);
	g_object_unref (priv->camera);

	G_OBJECT_CLASS (arv_fake_device_parent_class)->finalize (object);
}

static void
arv_fake_device_class_init (ArvFakeDeviceClass *fake_device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (fake_device_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (fake_device_class);

	object_class->finalize = arv_fake_device_finalize;

	device_class->create_stream = arv_fake_device_create_stream;
	device_class->get_genicam_xml = arv_fake_device_get_genicam_xml;
	device_class->get_genicam = arv_fake_device_get_genicam;
	device_class->read_memory = arv_fake_device_read_memory;
	device_class->write_memory = arv_fake_device_write_memory;
	device_class->read_register = arv_fake_device_read_register;
	device_class->write_register = arv_fake_device_write_register;
}
