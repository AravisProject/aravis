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
 * SECTION: arvuvinterface
 * @short_description: USB3 camera interface
 */

#include <arvuvinterface.h>
#include <arvdebug.h>
#include <arvmisc.h>
#include <libusb.h>
#include <stdio.h>

#define ARV_UV_INTERFACE_DEVICE_CLASS			0xef
#define ARV_UV_INTERFACE_DEVICE_SUBCLASS		0x02
#define ARV_UV_INTERFACE_DEVICE_PROTOCOL		0x01
#define ARV_UV_INTERFACE_INTERFACE_CLASS		0xef
#define ARV_UV_INTERFACE_INTERFACE_SUBCLASS		0x05
#define ARV_UV_INTERFACE_CONTROL_INTERFACE_PROTOCOL	0x00
#define ARV_UV_INTERFACE_EVENT_INTERFACE_PROTOCOL	0x01
#define ARV_UV_INTERFACE_STREAMING_INTERFACE_PROTOCOL	0x02

/* ArvUvInterface implementation */

static GObjectClass *parent_class = NULL;

struct _ArvUvInterfacePrivate {
	GHashTable *devices;
	libusb_context *usb;
};


#if 0
static void
printdev (libusb_device *device)
{
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;
	const struct libusb_interface *inter;
	const struct libusb_interface_descriptor *interdesc;
	int r, i, j;

	r = libusb_get_device_descriptor (device, &desc);

	if (r < 0) {
		printf ("Failed to get device descriptor\n");
		return;
	}

	printf ("VendorID:          0x%04x\n", desc.idVendor);
	printf ("ProductID:         0x%04x\n", desc.idProduct);
	printf ("Device Class:      0x%02x\n", (int) desc.bDeviceClass);
	printf ("Device SubClass:   0x%02x\n", (int) desc.bDeviceSubClass);
	printf ("Protocol:          0x%02x\n", (int) desc.bDeviceProtocol);

	libusb_get_config_descriptor (device, 0, &config);

	printf ("Nbr of Interfaces: %d\n", (int)config->bNumInterfaces);
	for (i = 0; i< (int) config->bNumInterfaces; i++) {
		inter = &config->interface[i];
		for (j = 0; j < inter->num_altsetting; j++) {
			interdesc = &inter->altsetting[j];
			printf ("  Interface Class:    0x%02x\n", (int) interdesc->bInterfaceClass);
			printf ("  Interface SubClass: 0x%02x\n", (int) interdesc->bInterfaceSubClass);
			printf ("  Interface Protocol: 0x%02x\n", (int) interdesc->bInterfaceProtocol);
		}
	}
	libusb_free_config_descriptor (config);
}
#endif

gboolean
_usb_device_is_usb3vision (libusb_device *device)
{
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;
	const struct libusb_interface *inter;
	const struct libusb_interface_descriptor *interdesc;
	gboolean success = TRUE;
	int r, i, j;

	r = libusb_get_device_descriptor (device, &desc);
	if (r < 0) {
		g_warning ("Failed to get device descriptor");
		return FALSE;
	}

	if (desc.bDeviceClass != ARV_UV_INTERFACE_DEVICE_CLASS ||
	    desc.bDeviceSubClass != ARV_UV_INTERFACE_DEVICE_SUBCLASS ||
	    desc.bDeviceProtocol != ARV_UV_INTERFACE_DEVICE_PROTOCOL)
		return FALSE;

	libusb_get_config_descriptor (device, 0, &config);
	for (i = 0; i< (int) config->bNumInterfaces; i++) {
		inter = &config->interface[i];
		for (j = 0; j < inter->num_altsetting; j++) {
			interdesc = &inter->altsetting[j];
			if (interdesc->bInterfaceClass != ARV_UV_INTERFACE_INTERFACE_CLASS ||
			    interdesc->bInterfaceSubClass != ARV_UV_INTERFACE_INTERFACE_SUBCLASS)
				success = FALSE;
		}
	}
	libusb_free_config_descriptor (config);

	return success;
}

static void
arv_uv_interface_update_device_list (ArvInterface *interface, GArray *device_ids)
{
	ArvUvInterface *uv_interface = ARV_UV_INTERFACE (interface);
	libusb_device **devices;
	unsigned uv_count = 0;
	unsigned count;
	unsigned i;

	count = libusb_get_device_list(uv_interface->priv->usb, &devices);
	if (count < 0)
		return;

	for (i = 0; i < count; i++)
		if (_usb_device_is_usb3vision (devices[i]))
		    uv_count++;

	arv_debug_interface ("Found %d USB3Vision device%s (among %d USB device%ss)\n",
			     uv_count , uv_count > 1 ? "s" : "",
			     count, count > 1 ? "s" : "");

	libusb_free_device_list (devices, 1);
}

static ArvDevice *
arv_uv_interface_open_device (ArvInterface *interface, const char *device_id)
{
	return NULL;
}

static ArvInterface *uv_interface = NULL;
ARV_DEFINE_STATIC_MUTEX (uv_interface_mutex);

/**
 * arv_uv_interface_get_instance:
 *
 * Gets the unique instance of the GV interface.
 *
 * Returns: (transfer none): a #ArvInterface singleton.
 */

ArvInterface *
arv_uv_interface_get_instance (void)
{
	arv_g_mutex_lock (&uv_interface_mutex);

	if (uv_interface == NULL)
		uv_interface = g_object_new (ARV_TYPE_UV_INTERFACE, NULL);

	arv_g_mutex_unlock (&uv_interface_mutex);

	return ARV_INTERFACE (uv_interface);
}

void
arv_uv_interface_destroy_instance (void)
{
	arv_g_mutex_lock (&uv_interface_mutex);

	g_clear_object (&uv_interface);

	arv_g_mutex_unlock (&uv_interface_mutex);
}

static void
arv_uv_interface_init (ArvUvInterface *uv_interface)
{
	uv_interface->priv = G_TYPE_INSTANCE_GET_PRIVATE (uv_interface, ARV_TYPE_UV_INTERFACE, ArvUvInterfacePrivate);
	libusb_init (&uv_interface->priv->usb);
}

static void
arv_uv_interface_finalize (GObject *object)
{
	ArvUvInterface *uv_interface = ARV_UV_INTERFACE (object);

	parent_class->finalize (object);
	libusb_exit (uv_interface->priv->usb);
}

static void
arv_uv_interface_class_init (ArvUvInterfaceClass *uv_interface_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (uv_interface_class);
	ArvInterfaceClass *interface_class = ARV_INTERFACE_CLASS (uv_interface_class);

	g_type_class_add_private (uv_interface_class, sizeof (ArvUvInterfacePrivate));

	parent_class = g_type_class_peek_parent (uv_interface_class);

	object_class->finalize = arv_uv_interface_finalize;

	interface_class->update_device_list = arv_uv_interface_update_device_list;
	interface_class->open_device = arv_uv_interface_open_device;
}

G_DEFINE_TYPE (ArvUvInterface, arv_uv_interface, ARV_TYPE_INTERFACE)
