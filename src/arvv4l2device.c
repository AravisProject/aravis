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
 * SECTION: arvv4l2device
 * @short_description: V4l2 device
 */

#include <arvdeviceprivate.h>
#include <arvv4l2deviceprivate.h>
#include <arvv4l2streamprivate.h>
#include <arvdebug.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <fcntl.h>

#define ARV_V4L2_ADDRESS_DEVICE_VENDOR_NAME		0x48
#define ARV_V4L2_ADDRESS_DEVICE_MODEL_NAME		0x68
#define ARV_V4L2_ADDRESS_DEVICE_VERSION			0x88
#define ARV_V4L2_ADDRESS_DEVICE_MANUFACTURER_INFO	0xa8
#define ARV_V4L2_ADDRESS_DEVICE_ID			0xd8

enum
{
	PROP_0,
	PROP_V4L2_DEVICE_DEVICE_FILE
};

typedef struct {
	int device_fd;

	char *device_file;
	char *device_card;
	char *device_version;
	char *device_driver;

	char *genicam_xml;
	size_t genicam_xml_size;

	ArvGc *genicam;
} ArvV4l2DevicePrivate;

struct _ArvV4l2Device {
	ArvDevice device;
};

struct _ArvV4l2DeviceClass {
	ArvDeviceClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvV4l2Device, arv_v4l2_device, ARV_TYPE_DEVICE, G_ADD_PRIVATE (ArvV4l2Device))

/* ArvV4l2Device implemenation */

/* ArvDevice implemenation */

static ArvStream *
arv_v4l2_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data, GDestroyNotify destroy,
                               GError **error)
{
	return arv_v4l2_stream_new (ARV_V4L2_DEVICE (device), callback, user_data, destroy, error);
}

static const char *
arv_v4l2_device_get_genicam_xml (ArvDevice *device, size_t *size)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));

	return priv->genicam_xml;
}

static ArvGc *
arv_v4l2_device_get_genicam (ArvDevice *device)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));

	return priv->genicam;
}

static gboolean
arv_v4l2_device_read_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));

        if (size < 1 || buffer == NULL)
                return FALSE;

	if (address == ARV_V4L2_ADDRESS_DEVICE_VENDOR_NAME) {
		strncpy (buffer, priv->device_driver, size - 1);
                ((char *) buffer)[size - 1] = '\0';
	} else if (address == ARV_V4L2_ADDRESS_DEVICE_MODEL_NAME) {
		strncpy (buffer, priv->device_card, size - 1);
                ((char *) buffer)[size - 1] = '\0';
	} else if (address == ARV_V4L2_ADDRESS_DEVICE_VERSION) {
		strncpy (buffer, priv->device_version, size - 1);
                ((char *) buffer)[size - 1] = '\0';
	} else if (address == ARV_V4L2_ADDRESS_DEVICE_MANUFACTURER_INFO) {
		strncpy (buffer, "Aravis", size - 1);
                ((char *) buffer)[size - 1] = '\0';
	} else if (address == ARV_V4L2_ADDRESS_DEVICE_ID) {
		strncpy (buffer, priv->device_file, size - 1);
                ((char *) buffer)[size - 1] = '\0';
	} else {
		/* TODO set error */
		return FALSE;
	}

	return TRUE;
}

static gboolean
arv_v4l2_device_write_memory (ArvDevice *device, guint64 address, guint32 size, const void *buffer, GError **error)
{
	/* TODO set error */
	return FALSE;
}

static gboolean
arv_v4l2_device_read_register (ArvDevice *device, guint64 address, guint32 *value, GError **error)
{
	return arv_v4l2_device_read_memory (device, address, sizeof (guint32), value, error);
}

static gboolean
arv_v4l2_device_write_register (ArvDevice *device, guint64 address, guint32 value, GError **error)
{
	return arv_v4l2_device_write_memory (device, address, sizeof (guint32), &value, error);
}

/**
 * arv_v4l2_device_new:
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: a newly created #ArvDevice connected to a v4l2 device
 *
 * Since: 0.8.7
 */

ArvDevice *
arv_v4l2_device_new (const char *device_file, GError **error)
{
	g_return_val_if_fail (device_file != NULL, NULL);

	return g_initable_new (ARV_TYPE_V4L2_DEVICE, NULL, error, "device-file", device_file, NULL);
}

static void
arv_v4l2_device_init (ArvV4l2Device *v4l2_device)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (v4l2_device);

	priv->device_fd = -1;
}

static void
arv_v4l2_device_finalize (GObject *object)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (object));

	if (priv->device_fd != -1)
		v4l2_close (priv->device_fd);

	g_clear_object (&priv->genicam);
	g_clear_pointer (&priv->genicam_xml, g_free);
	g_clear_pointer (&priv->device_file, g_free);
	g_clear_pointer (&priv->device_version, g_free);
	g_clear_pointer (&priv->device_driver, g_free);
	g_clear_pointer (&priv->device_card, g_free);

	G_OBJECT_CLASS (arv_v4l2_device_parent_class)->finalize (object);
}

static void
arv_v4l2_device_constructed (GObject *self)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (self));
	struct v4l2_capability cap;
	GBytes *bytes;
	GError *error = NULL;

	/* TODO errors */

	priv->device_fd = v4l2_open (priv->device_file, O_RDWR);
	if (priv->device_fd == -1) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
							 "Can't open '%s'", priv->device_file));
		return;
	}

	if (v4l2_ioctl (priv->device_fd, VIDIOC_QUERYCAP, &cap) == -1) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
							 "Device '%s' is not a V4L2 device", priv->device_file));
		return;
	}

	priv->device_card = g_strdup ((char *) cap.card);
	priv->device_driver = g_strdup ((char *) cap.driver);
	priv->device_version = g_strdup_printf ("%d.%d.%d",
						(cap.version >> 16) & 0xff,
						(cap.version >>  8) & 0xff,
						(cap.version >>  0) & 0xff);

	bytes = g_resources_lookup_data("/org/aravis/arv-v4l2.xml",
					G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
	if (error != NULL) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_GENICAM_NOT_FOUND,
							 "%s", error->message));
		g_clear_error (&error);
		return;
	}

	priv->genicam_xml = g_strndup (g_bytes_get_data(bytes,NULL), g_bytes_get_size(bytes));
	priv->genicam_xml_size = g_bytes_get_size(bytes);

	g_bytes_unref (bytes);

	priv->genicam = arv_gc_new (ARV_DEVICE (self), priv->genicam_xml, priv->genicam_xml_size);
	if (!ARV_IS_GC (priv->genicam)) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_GENICAM_NOT_FOUND,
							 "Invalid Genicam data"));
		return;
	}
}

static void
arv_v4l2_device_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (self));

	switch (prop_id)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
			break;
		case PROP_V4L2_DEVICE_DEVICE_FILE:
			g_free (priv->device_file);
			priv->device_file = g_value_dup_string (value);
			break;
	}
}

static void
arv_v4l2_device_class_init (ArvV4l2DeviceClass *v4l2_device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (v4l2_device_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (v4l2_device_class);

	object_class->finalize = arv_v4l2_device_finalize;
	object_class->constructed = arv_v4l2_device_constructed;
	object_class->set_property = arv_v4l2_device_set_property;

	device_class->create_stream = arv_v4l2_device_create_stream;
	device_class->get_genicam_xml = arv_v4l2_device_get_genicam_xml;
	device_class->get_genicam = arv_v4l2_device_get_genicam;
	device_class->read_memory = arv_v4l2_device_read_memory;
	device_class->write_memory = arv_v4l2_device_write_memory;
	device_class->read_register = arv_v4l2_device_read_register;
	device_class->write_register = arv_v4l2_device_write_register;

	g_object_class_install_property
		(object_class,
		 PROP_V4L2_DEVICE_DEVICE_FILE,
		 g_param_spec_string ("device-file",
				      "Device file",
				      "V4L2 device file",
				      NULL,
				      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}
