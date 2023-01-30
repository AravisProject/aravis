/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

/**
 * SECTION: arvfakedevice
 * @short_description: Fake device
 */

#include <arvdeviceprivate.h>
#include <arvgvdeviceprivate.h>
#include <arvfakedeviceprivate.h>
#include <arvfakestreamprivate.h>
#include <arvfakecamera.h>
#include <arvgc.h>
#include <arvdebug.h>

enum
{
	PROP_0,
	PROP_FAKE_DEVICE_SERIAL_NUMBER,
};

typedef struct {
	char *serial_number;
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
arv_fake_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data, GDestroyNotify destroy, GError **error)
{
	return arv_fake_stream_new (ARV_FAKE_DEVICE (device), callback, user_data, destroy, error);
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
 *
 * Since: 0.8.0
 */

ArvFakeCamera *
arv_fake_device_get_fake_camera	(ArvFakeDevice *device)
{
	ArvFakeDevicePrivate *priv = arv_fake_device_get_instance_private (ARV_FAKE_DEVICE (device));

	g_return_val_if_fail (ARV_IS_FAKE_DEVICE (device), NULL);

	return priv->camera;
}

/**
 * arv_fake_device_new:
 * @serial_number: fake device serial number
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: a newly created #ArvDevice simulating a real device
 *
 * Since: 0.8.0
 */

ArvDevice *
arv_fake_device_new (const char *serial_number, GError **error)
{
	return g_initable_new (ARV_TYPE_FAKE_DEVICE, NULL, error, "serial-number", serial_number, NULL);
}

static void
arv_fake_device_init (ArvFakeDevice *fake_device)
{
}

static void
arv_fake_device_finalize (GObject *object)
{
	ArvFakeDevicePrivate *priv = arv_fake_device_get_instance_private (ARV_FAKE_DEVICE (object));

	g_clear_pointer (&priv->serial_number, g_free);
	g_clear_object (&priv->genicam);
	g_clear_object (&priv->camera);

	G_OBJECT_CLASS (arv_fake_device_parent_class)->finalize (object);
}

static void
arv_fake_device_constructed (GObject *self)
{
	ArvFakeDevicePrivate *priv = arv_fake_device_get_instance_private (ARV_FAKE_DEVICE (self));
	const char *genicam_xml;
	gsize genicam_xml_size;

        G_OBJECT_CLASS (arv_fake_device_parent_class)->constructed (self);

	if (priv->serial_number == NULL) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
							 "Can't construct a fake device without a serial number"));
		return;
	}

	priv->camera = arv_fake_camera_new_full (priv->serial_number, NULL);
	genicam_xml = arv_fake_camera_get_genicam_xml (priv->camera, &genicam_xml_size);

	if (genicam_xml == NULL) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_GENICAM_NOT_FOUND,
							 "Genicam data not found"));
		return;
	}
	priv->genicam = arv_gc_new (ARV_DEVICE (self), genicam_xml, genicam_xml_size);
	if (!ARV_IS_GC (priv->genicam)) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_GENICAM_NOT_FOUND,
							 "Invalid Genicam data"));
		return;
	}

        arv_gc_set_default_gv_features(priv->genicam);
}

static void
arv_fake_device_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ArvFakeDevicePrivate *priv = arv_fake_device_get_instance_private (ARV_FAKE_DEVICE (self));

	switch (prop_id)
	{
		case PROP_FAKE_DEVICE_SERIAL_NUMBER:
			g_free (priv->serial_number);
			priv->serial_number = g_value_dup_string (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
			break;
	}
}

static void
arv_fake_device_class_init (ArvFakeDeviceClass *fake_device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (fake_device_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (fake_device_class);

	object_class->finalize = arv_fake_device_finalize;
	object_class->constructed = arv_fake_device_constructed;
	object_class->set_property = arv_fake_device_set_property;

	device_class->create_stream = arv_fake_device_create_stream;
	device_class->get_genicam_xml = arv_fake_device_get_genicam_xml;
	device_class->get_genicam = arv_fake_device_get_genicam;
	device_class->read_memory = arv_fake_device_read_memory;
	device_class->write_memory = arv_fake_device_write_memory;
	device_class->read_register = arv_fake_device_read_register;
	device_class->write_register = arv_fake_device_write_register;

	g_object_class_install_property
		(object_class,
		 PROP_FAKE_DEVICE_SERIAL_NUMBER,
		 g_param_spec_string ("serial-number",
				      "Serial number",
				      "Fake device serial number",
				      NULL,
				      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}
