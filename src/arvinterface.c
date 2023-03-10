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
 * SECTION: arvinterface
 * @short_description: Abstract base class for camera discovery
 *
 * #ArvInterface is an abstract base class for camera discovery. It maintains a
 * list of the available devices and helps to instantiate the corresponding
 * #ArvDevice objects. If the user already knows the device id of the device, he should
 * not worry about this class and just use arv_camera_new() or
 * arv_open_device().
 */

#include <arvinterfaceprivate.h>

typedef struct {
	GArray *device_ids;
        int flags;
} ArvInterfacePrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ArvInterface, arv_interface, G_TYPE_OBJECT, G_ADD_PRIVATE (ArvInterface))

static void
arv_interface_clear_device_ids (ArvInterface *iface)
{
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);
	unsigned int i;

	for (i = 0; i < priv->device_ids->len; i++) {
		g_free (g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, i)->device);
		g_free (g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, i)->physical);
		g_free (g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, i)->address);
		g_free (g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, i)->vendor);
		g_free (g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, i)->manufacturer_info);
		g_free (g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, i)->model);
		g_free (g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, i)->serial_nbr);
		g_free (g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, i));
	}
	g_array_set_size (priv->device_ids, 0);
}

static gint
_compare_device_ids (ArvInterfaceDeviceIds **a, ArvInterfaceDeviceIds **b)
{
	if (*a == NULL || (*a)->device == NULL)
		return -1;
	if (*b == NULL || (*b)->device == NULL)
		return 1;

	return g_ascii_strcasecmp ((*a)->device, (*b)->device);
}

/**
 * arv_interface_update_device_list:
 * @iface: a #ArvInterface
 *
 * Updates the internal list of available devices. This may change the
 * connection between a list index and a device ID.
 *
 * Since: 0.2.0
 */

void
arv_interface_update_device_list (ArvInterface *iface)
{
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);
	g_return_if_fail (ARV_IS_INTERFACE (iface));

	arv_interface_clear_device_ids (iface);

	ARV_INTERFACE_GET_CLASS (iface)->update_device_list (iface, priv->device_ids);

	g_array_sort (priv->device_ids, (GCompareFunc) _compare_device_ids);
}

void
arv_interface_set_flags (ArvInterface *iface, int flags)
{
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);
	g_return_if_fail (ARV_IS_INTERFACE (iface));

        priv->flags = flags;
}

int
arv_interface_get_flags (ArvInterface *iface)
{
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);
	g_return_val_if_fail (ARV_IS_INTERFACE (iface), 0);

        return priv->flags;
}

/**
 * arv_interface_get_n_devices:
 * @iface: a #ArvInterface
 *
 * Queries the number of available devices on this interface. Prior to this
 * call the @arv_interface_update_device_list function must be called. The list content will not
 * change until the next call of the update function.
 *
 * Returns: the number of available devices
 *
 * Since: 0.2.0
 */

unsigned int
arv_interface_get_n_devices (ArvInterface *iface)
{
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);

	g_return_val_if_fail (ARV_IS_INTERFACE (iface), 0);
	g_return_val_if_fail (priv->device_ids != NULL, 0);

	return priv->device_ids->len;
}

/**
 * arv_interface_get_device_id:
 * @iface: a #ArvInterface
 * @index: device index
 *
 * Queries the unique device id corresponding to index.  Prior to this
 * call the arv_interface_update_device_list() function must be called.
 *
 * Returns: a unique device id
 *
 * Since: 0.2.0
 */

const char *
arv_interface_get_device_id (ArvInterface *iface, unsigned int index)
{
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);

	g_return_val_if_fail (ARV_IS_INTERFACE (iface), 0);
	g_return_val_if_fail (priv->device_ids != NULL, 0);

	if (index >= priv->device_ids->len)
		return NULL;

	return g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, index)->device;
}

/**
 * arv_interface_get_device_physical_id:
 * @iface: a #ArvInterface
 * @index: device index
 *
 * Queries the physical device id corresponding to index such
 * as the MAC address for Ethernet based devices, bus id for PCI,
 * USB or Firewire based devices.
 *
 * Prior to this call the arv_interface_update_device_list()
 * function must be called.
 *
 * Returns: a physical device id
 *
 * Since: 0.2.0
 */

const char *
arv_interface_get_device_physical_id (ArvInterface *iface, unsigned int index)
{
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);

	g_return_val_if_fail (ARV_IS_INTERFACE (iface), 0);
	g_return_val_if_fail (priv->device_ids != NULL, 0);

	if (index >= priv->device_ids->len)
		return NULL;

	return g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, index)->physical;
}

/**
 * arv_interface_get_device_address:
 * @iface: a #ArvInterface
 * @index: device index
 *
 * queries the device address (IP address in the case of an ethernet camera). Useful
 * for constructing manual connections to devices using @arv_gv_device_new
 *
 * Prior to this call the arv_interface_update_device_list()
 * function must be called.
 *
 * Returns: (transfer none): the device address
 *
 * Since: 0.2.0
 */

const char *
arv_interface_get_device_address (ArvInterface *iface, unsigned int index)
{
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);

	g_return_val_if_fail (ARV_IS_INTERFACE (iface), 0);
	g_return_val_if_fail (priv->device_ids != NULL, 0);

	if (index >= priv->device_ids->len)
		return NULL;

	return g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, index)->address;
}

/**
 * arv_interface_get_device_vendor:
 * @iface: a #ArvInterface
 * @index: device index
 *
 * Queries the device vendor.
 *
 * Prior to this call the arv_interface_update_device_list()
 * function must be called.
 *
 * Returns: (transfer none): the device vendor, NULL on error
 *
 * Since: 0.6.0
 */

const char *
arv_interface_get_device_vendor (ArvInterface *iface, unsigned int index)
{
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);

	g_return_val_if_fail (ARV_IS_INTERFACE (iface), 0);
	g_return_val_if_fail (priv->device_ids != NULL, 0);

	if (index >= priv->device_ids->len)
		return NULL;

	return g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, index)->vendor;
}

/**
 * arv_interface_get_device_manufacturer_info:
 * @iface: a #ArvInterface
 * @index: device index
 *
 * Queries the device manufacturer info.
 *
 * Prior to this call the arv_interface_update_device_list()
 * function must be called.
 *
 * Returns: (transfer none): the device manufacturer info, NULL on error
 *
 * Since: 0.8.20
 */

const char *
arv_interface_get_device_manufacturer_info (ArvInterface *iface, unsigned int index)
{
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);

	g_return_val_if_fail (ARV_IS_INTERFACE (iface), 0);
	g_return_val_if_fail (priv->device_ids != NULL, 0);

	if (index >= priv->device_ids->len)
		return NULL;

	return g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, index)->manufacturer_info;
}

/**
 * arv_interface_get_device_model:
 * @iface: a #ArvInterface
 * @index: device index
 *
 * Queries the device model.
 *
 * Prior to this call the arv_interface_update_device_list()
 * function must be called.
 *
 * Returns: (transfer none): the device model, NULL on error
 *
 * Since: 0.6.0
 */

const char *
arv_interface_get_device_model (ArvInterface *iface, unsigned int index)
{
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);

	g_return_val_if_fail (ARV_IS_INTERFACE (iface), 0);
	g_return_val_if_fail (priv->device_ids != NULL, 0);

	if (index >= priv->device_ids->len)
		return NULL;

	return g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, index)->model;
}

/**
 * arv_interface_get_device_serial_nbr:
 * @iface: a #ArvInterface
 * @index: device index
 *
 * Queries the device serial.
 *
 * Prior to this call the arv_interface_update_device_list()
 * function must be called.
 *
 * Returns: (transfer none): the device serial, NULL on error
 *
 * Since: 0.6.0
 */

const char *
arv_interface_get_device_serial_nbr (ArvInterface *iface, unsigned int index)
{
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);

	g_return_val_if_fail (ARV_IS_INTERFACE (iface), 0);
	g_return_val_if_fail (priv->device_ids != NULL, 0);

	if (index >= priv->device_ids->len)
		return NULL;

	return g_array_index (priv->device_ids, ArvInterfaceDeviceIds *, index)->serial_nbr;
}

/**
 * arv_interface_get_device_protocol:
 * @iface: a #ArvInterface
 * @index: device index
 *
 * Queries the device protocol. Possible values are 'USB3Vision', 'GigEVision'
 * and 'Fake'.
 *
 * Prior to this call the arv_interface_update_device_list()
 * function must be called.
 *
 * Returns: (transfer none): the device protocol as a string, NULL on error
 *
 * Since: 0.6.0
 */

const char *
arv_interface_get_device_protocol (ArvInterface *iface, unsigned int index)
{
	g_return_val_if_fail (ARV_IS_INTERFACE (iface), NULL);

	return ARV_INTERFACE_GET_CLASS (iface)->protocol;
}

/**
 * arv_interface_open_device:
 * @iface: a #ArvInterface
 * @device_id: (allow-none): device unique id
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Creates a new #ArvDevice object corresponding to the given device id string.
 * The first available device is returned if @device_id is %NULL.
 *
 * Returns: (transfer full): a new #ArvDevice
 *
 * Since: 0.2.0
 */

ArvDevice *
arv_interface_open_device (ArvInterface *iface, const char *device_id, GError **error)
{
	g_return_val_if_fail (ARV_IS_INTERFACE (iface), NULL);

	return ARV_INTERFACE_GET_CLASS (iface)->open_device (iface, device_id, error);
}

static void
arv_interface_init (ArvInterface *iface)
{
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);

	priv->device_ids = g_array_new (FALSE, TRUE, sizeof (ArvInterfaceDeviceIds *));
}

static void
arv_interface_finalize (GObject *object)
{
	ArvInterface *iface = ARV_INTERFACE (object);
	ArvInterfacePrivate *priv = arv_interface_get_instance_private (iface);

	G_OBJECT_CLASS (arv_interface_parent_class)->finalize (object);

	arv_interface_clear_device_ids (iface);
	g_array_free (priv->device_ids, TRUE);
	priv->device_ids = NULL;
}

static void
arv_interface_class_init (ArvInterfaceClass *interface_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (interface_class);

	object_class->finalize = arv_interface_finalize;
}
