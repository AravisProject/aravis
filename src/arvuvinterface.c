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
 * SECTION: arvuvinterface
 * @short_description: USB3Vision interface
 */

#include <arvuvinterfaceprivate.h>
#include <arvinterfaceprivate.h>
#include <arvuvdeviceprivate.h>
#include <arvdebugprivate.h>
#include <arvmiscprivate.h>
#include <arvstr.h>
#include <libusb.h>
#include <stdio.h>

/* ArvUvInterface implementation */

typedef struct {
	char *id;
	char *name;
	char *full_name;
	char *manufacturer;
	char *product;
	char *serial_nbr;
	char *guid;

	volatile gint ref_count;
} ArvUvInterfaceDeviceInfos;

static ArvUvInterfaceDeviceInfos *
arv_uv_interface_device_infos_new (const char *manufacturer,
				   const char *product,
				   const char *serial_nbr,
				   const char *guid)
{
	ArvUvInterfaceDeviceInfos *infos;

	g_return_val_if_fail (manufacturer != NULL, NULL);
	g_return_val_if_fail (product != NULL, NULL);
	g_return_val_if_fail (serial_nbr != NULL, NULL);
	g_return_val_if_fail (guid != NULL, NULL);

	infos = g_new (ArvUvInterfaceDeviceInfos, 1);
	infos->id = g_strdup_printf ("%s-%s-%s", manufacturer, guid, serial_nbr);
	infos->manufacturer = g_strdup (manufacturer);
	infos->name = g_strdup_printf ("%s-%s", arv_vendor_alias_lookup (manufacturer), serial_nbr);
	infos->full_name = g_strdup_printf ("%s-%s", manufacturer, serial_nbr);
	infos->product = g_strdup (product);
	infos->serial_nbr = g_strdup (serial_nbr);
	infos->guid = g_strdup (guid);
	infos->ref_count = 1;

	arv_str_strip (infos->id, ARV_DEVICE_NAME_ILLEGAL_CHARACTERS, ARV_DEVICE_NAME_REPLACEMENT_CHARACTER);
	arv_str_strip (infos->name, ARV_DEVICE_NAME_ILLEGAL_CHARACTERS, ARV_DEVICE_NAME_REPLACEMENT_CHARACTER);
	arv_str_strip (infos->full_name, ARV_DEVICE_NAME_ILLEGAL_CHARACTERS, ARV_DEVICE_NAME_REPLACEMENT_CHARACTER);

	return infos;
}

static ArvUvInterfaceDeviceInfos *
arv_uv_interface_device_infos_ref (ArvUvInterfaceDeviceInfos *infos)
{
	g_return_val_if_fail (infos != NULL, NULL);
	g_return_val_if_fail (g_atomic_int_get (&infos->ref_count) > 0, NULL);

	g_atomic_int_inc (&infos->ref_count);

	return infos;
}

static void
arv_uv_interface_device_infos_unref (ArvUvInterfaceDeviceInfos *infos)
{
	g_return_if_fail (infos != NULL);
	g_return_if_fail (g_atomic_int_get (&infos->ref_count) > 0);

	if (g_atomic_int_dec_and_test (&infos->ref_count)) {
		g_clear_pointer (&infos->id, g_free);
		g_clear_pointer (&infos->name, g_free);
		g_clear_pointer (&infos->full_name, g_free);
		g_clear_pointer (&infos->manufacturer, g_free);
		g_clear_pointer (&infos->product, g_free);
		g_clear_pointer (&infos->serial_nbr, g_free);
		g_clear_pointer (&infos->guid, g_free);
		g_clear_pointer (&infos, g_free);
	}
}

typedef struct {
	GHashTable *devices;
	libusb_context *usb;
} ArvUvInterfacePrivate;

struct _ArvUvInterface {
	ArvInterface	interface;

	ArvUvInterfacePrivate *priv;
};

struct _ArvUvInterfaceClass {
	ArvInterfaceClass parent_class;
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

static ArvInterfaceDeviceIds *
_usb_device_to_device_ids (ArvUvInterface *uv_interface, libusb_device *device)
{
	ArvInterfaceDeviceIds *device_ids = NULL;
	libusb_device_handle *device_handle;
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;
	const struct libusb_interface *inter;
	const struct libusb_interface_descriptor *interdesc;
	gboolean control_protocol_found;
	gboolean data_protocol_found;
	int guid_index = -1;
	int result, i, j;

	result = libusb_get_device_descriptor (device, &desc);
	if (result < 0) {
		arv_warning_interface ("Failed to get device descriptor: %s",
				       libusb_error_name (result));
		return NULL;
	}

	if (desc.bDeviceClass != ARV_UV_INTERFACE_DEVICE_CLASS ||
	    desc.bDeviceSubClass != ARV_UV_INTERFACE_DEVICE_SUBCLASS ||
	    desc.bDeviceProtocol != ARV_UV_INTERFACE_DEVICE_PROTOCOL)
		return NULL;

	control_protocol_found = FALSE;
	data_protocol_found = FALSE;
	libusb_get_config_descriptor (device, 0, &config);
	for (i = 0; i< (int) config->bNumInterfaces; i++) {
		inter = &config->interface[i];
		for (j = 0; j < inter->num_altsetting; j++) {
			interdesc = &inter->altsetting[j];
			if (interdesc->bInterfaceClass == ARV_UV_INTERFACE_INTERFACE_CLASS &&
			    interdesc->bInterfaceSubClass == ARV_UV_INTERFACE_INTERFACE_SUBCLASS) {
				if (interdesc->bInterfaceProtocol == ARV_UV_INTERFACE_CONTROL_PROTOCOL) {
					control_protocol_found = TRUE;
					if (interdesc->extra &&
					    interdesc->extra_length >= ARV_UV_INTERFACE_GUID_INDEX_OFFSET + sizeof(unsigned char)) {
						guid_index = (int) (*(interdesc->extra + ARV_UV_INTERFACE_GUID_INDEX_OFFSET));
					}
				}
				if (interdesc->bInterfaceProtocol == ARV_UV_INTERFACE_DATA_PROTOCOL)
					data_protocol_found = TRUE;
			}
		}
	}
	libusb_free_config_descriptor (config);

	if (!control_protocol_found || !data_protocol_found)
		return NULL;

        result = libusb_open (device, &device_handle);
	if (result == LIBUSB_SUCCESS) {
		ArvUvInterfaceDeviceInfos *device_infos;
		unsigned char *manufacturer;
		unsigned char *product;
		unsigned char *serial_nbr;
		unsigned char *guid;
		int index;

		device_ids = g_new0 (ArvInterfaceDeviceIds, 1);

		manufacturer = g_malloc0 (256);
		product = g_malloc0 (256);
		serial_nbr = g_malloc0 (256);
		guid = g_malloc0 (256);

		index = desc.iManufacturer;
		if (index > 0)
			libusb_get_string_descriptor_ascii (device_handle, index, manufacturer, 256);
		index = desc.iProduct;
		if (index > 0)
			libusb_get_string_descriptor_ascii (device_handle, index, product, 256);
		index = desc.iSerialNumber;
		if (index > 0)
			libusb_get_string_descriptor_ascii (device_handle, index, serial_nbr, 256);
		index = guid_index;
		if (index > 0)
			libusb_get_string_descriptor_ascii (device_handle, index, guid, 256);

		device_infos = arv_uv_interface_device_infos_new ((char *) manufacturer, (char *) product,
                                                                  (char *) serial_nbr, (char *) guid);
		g_hash_table_replace (uv_interface->priv->devices, device_infos->id,
				      arv_uv_interface_device_infos_ref (device_infos));
		g_hash_table_replace (uv_interface->priv->devices, device_infos->name,
				      arv_uv_interface_device_infos_ref (device_infos));
		g_hash_table_replace (uv_interface->priv->devices, device_infos->full_name,
				      arv_uv_interface_device_infos_ref (device_infos));
		g_hash_table_replace (uv_interface->priv->devices, device_infos->guid,
				      arv_uv_interface_device_infos_ref (device_infos));
		arv_uv_interface_device_infos_unref (device_infos);

		device_ids->device = g_strdup (device_infos->id);
		device_ids->physical = g_strdup (device_infos->guid);
		device_ids->address = g_strdup ("USB3");	/* FIXME */
		device_ids->vendor = g_strdup (device_infos->manufacturer);
                device_ids->manufacturer_info = g_strdup ("none");
		device_ids->model = g_strdup (device_infos->product);
		device_ids->serial_nbr = g_strdup (device_infos->serial_nbr);

		g_free (manufacturer);
		g_free (product);
		g_free (serial_nbr);
		g_free (guid);

		libusb_close (device_handle);
	} else
		arv_warning_interface ("Failed to open USB device: %s",
				       libusb_error_name (result));

	return device_ids;
}

static void
_discover (ArvUvInterface *uv_interface,  GArray *device_ids)
{
	libusb_device **devices;
	unsigned uv_count = 0;
	ssize_t result;
	unsigned i;

        if (uv_interface->priv->usb == NULL)
                return;

	result = libusb_get_device_list(uv_interface->priv->usb, &devices);
	if (result < 0) {
		arv_warning_interface ("Failed to get USB device list: %s",
				       libusb_error_name (result));
		return;
	}

	g_hash_table_remove_all (uv_interface->priv->devices);

	for (i = 0; i < result; i++) {
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
			    g_free (ids->vendor);
                            g_free (ids->manufacturer_info);
			    g_free (ids->model);
			    g_free (ids->serial_nbr);
			    g_free (ids);
		    }
		}
	}

	arv_info_interface ("Found %d USB3Vision device%s (among %" G_GSSIZE_FORMAT " USB device%s)",
			     uv_count , uv_count > 1 ? "s" : "",
			     result, result > 1 ? "s" : "");

	libusb_free_device_list (devices, 1);
}

static void
arv_uv_interface_update_device_list (ArvInterface *interface, GArray *device_ids)
{
	ArvUvInterface *uv_interface = ARV_UV_INTERFACE (interface);

	g_assert (device_ids->len == 0);

	_discover (uv_interface, device_ids);
}

static ArvDevice *
_open_device (ArvInterface *interface, const char *device_id, GError **error)
{
	ArvUvInterface *uv_interface;
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

	return arv_uv_device_new_from_guid (device_infos->guid, error);
}

static ArvDevice *
arv_uv_interface_open_device (ArvInterface *interface, const char *device_id, GError **error)
{
	ArvDevice *device;
	GError *local_error = NULL;

	device = _open_device (interface, device_id, error);
	if (ARV_IS_DEVICE (device) || local_error != NULL) {
		if (local_error != NULL)
			g_propagate_error (error, local_error);
		return device;
	}

	_discover (ARV_UV_INTERFACE (interface), NULL);

	return _open_device (interface, device_id, error);
}

static ArvInterface *arv_uv_interface = NULL;
static GMutex arv_uv_interface_mutex;

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
	g_mutex_lock (&arv_uv_interface_mutex);

	if (arv_uv_interface == NULL)
		arv_uv_interface = g_object_new (ARV_TYPE_UV_INTERFACE, NULL);

	g_mutex_unlock (&arv_uv_interface_mutex);

	return ARV_INTERFACE (arv_uv_interface);
}

void
arv_uv_interface_destroy_instance (void)
{
	g_mutex_lock (&arv_uv_interface_mutex);

	g_clear_object (&arv_uv_interface);

	g_mutex_unlock (&arv_uv_interface_mutex);
}

G_DEFINE_TYPE_WITH_CODE (ArvUvInterface, arv_uv_interface, ARV_TYPE_INTERFACE, G_ADD_PRIVATE (ArvUvInterface))

static void
arv_uv_interface_init (ArvUvInterface *uv_interface)
{
        int result;

	uv_interface->priv = arv_uv_interface_get_instance_private (uv_interface);

	result = libusb_init (&uv_interface->priv->usb);
        if (result != 0)
		arv_warning_interface ("Failed to initialize USB library: %s",
				       libusb_error_name (result));

	uv_interface->priv->devices = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
							     (GDestroyNotify) arv_uv_interface_device_infos_unref);
}

static void
arv_uv_interface_finalize (GObject *object)
{
	ArvUvInterface *uv_interface = ARV_UV_INTERFACE (object);

	g_hash_table_unref (uv_interface->priv->devices);

	G_OBJECT_CLASS (arv_uv_interface_parent_class)->finalize (object);

        if (uv_interface->priv->usb != NULL)
                libusb_exit (uv_interface->priv->usb);
}

static void
arv_uv_interface_class_init (ArvUvInterfaceClass *uv_interface_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (uv_interface_class);
	ArvInterfaceClass *interface_class = ARV_INTERFACE_CLASS (uv_interface_class);

	object_class->finalize = arv_uv_interface_finalize;

	interface_class->update_device_list = arv_uv_interface_update_device_list;
	interface_class->open_device = arv_uv_interface_open_device;

	interface_class->protocol = "USB3Vision";
}
