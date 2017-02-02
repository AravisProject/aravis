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
 * @short_description: USB3Vision interface
 */

#include <arvuvinterfaceprivate.h>
#include <arvinterfaceprivate.h>
#include <arvuvdeviceprivate.h>
#include <arvdebug.h>
#include <arvmisc.h>
#include <arvstr.h>
#include <libusb.h>
#include <stdio.h>

#define ARV_UV_INTERFACE_DEVICE_CLASS			0xef	/* Miscellaneous device */
#define ARV_UV_INTERFACE_DEVICE_SUBCLASS		0x02
#define ARV_UV_INTERFACE_DEVICE_PROTOCOL		0x01
#define ARV_UV_INTERFACE_INTERFACE_CLASS		0xef
#define ARV_UV_INTERFACE_INTERFACE_SUBCLASS		0x05
#define ARV_UV_INTERFACE_CONTROL_INTERFACE_PROTOCOL	0x00
#define ARV_UV_INTERFACE_EVENT_INTERFACE_PROTOCOL	0x01
#define ARV_UV_INTERFACE_STREAMING_INTERFACE_PROTOCOL	0x02

/* ArvUvInterface implementation */

static GObjectClass *parent_class = NULL;

typedef struct {
	char *name;
	char *manufacturer;
	char *product;
	char *serial_nbr;

	volatile gint ref_count;
} ArvUvInterfaceDeviceInfos;

static ArvUvInterfaceDeviceInfos *
arv_uv_interface_device_infos_new (const char *manufacturer,
				   const char *product,
				   const char *serial_nbr)
{
	ArvUvInterfaceDeviceInfos *infos;

	g_return_val_if_fail (manufacturer != NULL, NULL);
	g_return_val_if_fail (product != NULL, NULL);
	g_return_val_if_fail (serial_nbr != NULL, NULL);

	infos = g_new (ArvUvInterfaceDeviceInfos, 1);
	infos->manufacturer = g_strdup (manufacturer);
	infos->name = g_strdup_printf ("%s-%s", manufacturer, serial_nbr);
	infos->product = g_strdup (product);
	infos->serial_nbr = g_strdup (serial_nbr);
	infos->ref_count = 1;

	arv_str_strip (infos->name, ARV_DEVICE_NAME_ILLEGAL_CHARACTERS, ARV_DEVICE_NAME_REPLACEMENT_CHARACTER);

	return infos;
}

/*
static void
arv_uv_interface_device_infos_ref (ArvUvInterfaceDeviceInfos *infos)
{
	g_return_if_fail (infos != NULL);
	g_return_if_fail (g_atomic_int_get (&infos->ref_count) > 0);
	g_atomic_int_inc (&infos->ref_count);
}
*/

static void
arv_uv_interface_device_infos_unref (ArvUvInterfaceDeviceInfos *infos)
{
	g_return_if_fail (infos != NULL);
	g_return_if_fail (g_atomic_int_get (&infos->ref_count) > 0);

	if (g_atomic_int_dec_and_test (&infos->ref_count)) {
		g_clear_pointer (&infos->name, g_free);
		g_clear_pointer (&infos->manufacturer, g_free);
		g_clear_pointer (&infos->product, g_free);
		g_clear_pointer (&infos->serial_nbr, g_free);
		g_clear_pointer (&infos, g_free);
	}
}

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

ArvInterfaceDeviceIds *
_usb_device_to_device_ids (ArvUvInterface *uv_interface, libusb_device *device)
{
	ArvInterfaceDeviceIds *device_ids = NULL;
	libusb_device_handle *device_handle;
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;
	const struct libusb_interface *inter;
	const struct libusb_interface_descriptor *interdesc;
	gboolean success = TRUE;
	int r, i, j;

	r = libusb_get_device_descriptor (device, &desc);
	if (r < 0) {
		g_warning ("Failed to get device descriptor");
		return NULL;
	}

	if (desc.bDeviceClass != ARV_UV_INTERFACE_DEVICE_CLASS ||
	    desc.bDeviceSubClass != ARV_UV_INTERFACE_DEVICE_SUBCLASS ||
	    desc.bDeviceProtocol != ARV_UV_INTERFACE_DEVICE_PROTOCOL)
		return NULL;

	success = FALSE;
	libusb_get_config_descriptor (device, 0, &config);
	for (i = 0; i< (int) config->bNumInterfaces; i++) {
		inter = &config->interface[i];
		for (j = 0; j < inter->num_altsetting; j++) {
			interdesc = &inter->altsetting[j];
			if (interdesc->bInterfaceClass == ARV_UV_INTERFACE_INTERFACE_CLASS &&
			    interdesc->bInterfaceSubClass == ARV_UV_INTERFACE_INTERFACE_SUBCLASS) {
				success = TRUE;
			}
		}
	}
	libusb_free_config_descriptor (config);

	if (!success)
		return NULL;

	if (libusb_open (device, &device_handle) == LIBUSB_SUCCESS) {
		ArvUvInterfaceDeviceInfos *device_infos;
		unsigned char *manufacturer;
		unsigned char *product;
		unsigned char *serial_nbr;
		int index;

		device_ids = g_new0 (ArvInterfaceDeviceIds, 1);

		manufacturer = g_malloc0 (256);
		product = g_malloc0 (256);
		serial_nbr = g_malloc0 (256);

		index = desc.iManufacturer;
		if (index > 0)
			libusb_get_string_descriptor_ascii (device_handle, index, manufacturer, 256);
		index = desc.iProduct;
		if (index > 0)
			libusb_get_string_descriptor_ascii (device_handle, index, product, 256);
		index = desc.iSerialNumber;
		if (index > 0)
			libusb_get_string_descriptor_ascii (device_handle, index, serial_nbr, 256);

		device_infos = arv_uv_interface_device_infos_new ((char *) manufacturer, (char *) product, (char *) serial_nbr);
		g_hash_table_replace (uv_interface->priv->devices, device_infos->name, device_infos);

		device_ids->device = g_strdup (device_infos->name);
		device_ids->physical = g_strdup ("USB3");	/* FIXME */
		device_ids->address = g_strdup ("USB3");	/* FIXME */
		device_ids->vendor = g_strdup (device_infos->manufacturer);
		device_ids->model = g_strdup (device_infos->product);
		device_ids->serial_nbr = g_strdup (device_infos->serial_nbr);

		g_free (manufacturer);
		g_free (product);
		g_free (serial_nbr);

		libusb_close (device_handle);
	} else
		arv_warning_interface ("Failed to open USB device");

	return device_ids;
}

static void
_discover (ArvUvInterface *uv_interface,  GArray *device_ids)
{
	libusb_device **devices;
	unsigned uv_count = 0;
	ssize_t count;
	unsigned i;

	count = libusb_get_device_list(uv_interface->priv->usb, &devices);
	if (count < 0) {
		arv_warning_interface ("[[UvInterface:_discover] Failed to get USB device list: %s",
				       libusb_error_name (count));
		return;
	}

	for (i = 0; i < count; i++) {
		ArvInterfaceDeviceIds *ids;

		ids = _usb_device_to_device_ids (uv_interface, devices[i]);
		if (ids != NULL) {
		    uv_count++;
		    if (device_ids != NULL)
			    g_array_append_val (device_ids, ids);
		    else {
			    g_free (ids->device);
			    g_free (ids->physical);
			    g_free (ids->address);
			    g_free (ids);
		    }
		}
	}

	arv_debug_interface ("Found %d USB3Vision device%s (among %d USB device%s)",
			     uv_count , uv_count > 1 ? "s" : "",
			     count, count > 1 ? "s" : "");

	libusb_free_device_list (devices, 1);
}

static void
arv_uv_interface_update_device_list (ArvInterface *interface, GArray *device_ids)
{
	ArvUvInterface *uv_interface = ARV_UV_INTERFACE (interface);

	_discover (uv_interface, device_ids);
}

static ArvDevice *
_open_device (ArvInterface *interface, const char *device_id)
{
	ArvUvInterface *uv_interface;
	ArvDevice *device = NULL;
	ArvUvInterfaceDeviceInfos *device_infos;

	uv_interface = ARV_UV_INTERFACE (interface);

	if (device_id == NULL) {
		GList *device_list;

		device_list = g_hash_table_get_values (uv_interface->priv->devices);
		device_infos = device_list != NULL ? device_list->data : NULL;
		g_list_free (device_list);
	} else
		device_infos = g_hash_table_lookup (uv_interface->priv->devices, device_id);

	if (device_infos == NULL)
		return NULL;

	device = arv_uv_device_new (device_infos->manufacturer, device_infos->product, device_infos->serial_nbr);

	return device;
}

static ArvDevice *
arv_uv_interface_open_device (ArvInterface *interface, const char *device_id)
{
	ArvDevice *device;

	device = _open_device (interface, device_id);
	if (ARV_IS_DEVICE (device))
		return device;

	_discover (ARV_UV_INTERFACE (interface), NULL);

	return _open_device (interface, device_id);
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

	uv_interface->priv->devices = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
							     (GDestroyNotify) arv_uv_interface_device_infos_unref);
}

static void
arv_uv_interface_finalize (GObject *object)
{
	ArvUvInterface *uv_interface = ARV_UV_INTERFACE (object);

	g_hash_table_unref (uv_interface->priv->devices);

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
