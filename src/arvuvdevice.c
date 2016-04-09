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
 * SECTION: arvuvdevice
 * @short_description: Uv camera device
 */

#include <arvuvdevice.h>
#include <arvuvstream.h>
#include <arvgc.h>
#include <arvdebug.h>

static GObjectClass *parent_class = NULL;

struct _ArvUvDevicePrivate {
	ArvGc *genicam;

	const char *genicam_xml;
	size_t genicam_xml_size;
};

/* ArvUvDevice implemenation */

/* ArvDevice implemenation */

static ArvStream *
arv_uv_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data)
{
	ArvStream *stream;

	stream = arv_uv_stream_new (callback, user_data);

	return stream;
}

static const char *
arv_uv_device_get_genicam_xml (ArvDevice *device, size_t *size)
{
	ArvUvDevice *uv_device = ARV_UV_DEVICE (device);

	*size = uv_device->priv->genicam_xml_size;

	return uv_device->priv->genicam_xml;
}

static ArvGc *
arv_uv_device_get_genicam (ArvDevice *device)
{
	ArvUvDevice *uv_device = ARV_UV_DEVICE (device);

	return uv_device->priv->genicam;
}

static gboolean
arv_uv_device_read_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer, GError **error)
{
	g_assert_not_reached ();
	return FALSE;
}

static gboolean
arv_uv_device_write_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer, GError **error)
{
	g_assert_not_reached ();
	return FALSE;
}

static gboolean
arv_uv_device_read_register (ArvDevice *device, guint32 address, guint32 *value, GError **error)
{
	g_assert_not_reached ();
	return FALSE;
}

static gboolean
arv_uv_device_write_register (ArvDevice *device, guint32 address, guint32 value, GError **error)
{
	g_assert_not_reached ();
	return FALSE;
}

ArvDevice *
arv_uv_device_new (const char *serial_number)
{
	ArvUvDevice *uv_device;

	g_return_val_if_fail (serial_number != NULL, NULL);

	uv_device = g_object_new (ARV_TYPE_UV_DEVICE, NULL);

	return ARV_DEVICE (uv_device);
}

static void
arv_uv_device_init (ArvUvDevice *uv_device)
{
	uv_device->priv = G_TYPE_INSTANCE_GET_PRIVATE (uv_device, ARV_TYPE_UV_DEVICE, ArvUvDevicePrivate);
}

static void
arv_uv_device_finalize (GObject *object)
{
	ArvUvDevice *uv_device = ARV_UV_DEVICE (object);

	g_object_unref (uv_device->priv->genicam);

	parent_class->finalize (object);
}

static void
arv_uv_device_class_init (ArvUvDeviceClass *uv_device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (uv_device_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (uv_device_class);

	g_type_class_add_private (uv_device_class, sizeof (ArvUvDevicePrivate));

	parent_class = g_type_class_peek_parent (uv_device_class);

	object_class->finalize = arv_uv_device_finalize;

	device_class->create_stream = arv_uv_device_create_stream;
	device_class->get_genicam_xml = arv_uv_device_get_genicam_xml;
	device_class->get_genicam = arv_uv_device_get_genicam;
	device_class->read_memory = arv_uv_device_read_memory;
	device_class->write_memory = arv_uv_device_write_memory;
	device_class->read_register = arv_uv_device_read_register;
	device_class->write_register = arv_uv_device_write_register;
}

G_DEFINE_TYPE (ArvUvDevice, arv_uv_device, ARV_TYPE_DEVICE)
