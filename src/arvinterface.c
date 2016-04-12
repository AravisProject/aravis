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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvinterface
 * @short_description: Abstract base class for camera discovery
 *
 * #ArvCamera is an abstract base class for camera discovery. It maintains a
 * list of the available devices and help to instantiate the corresponding
 * #ArvDevice object. If user already knows the device id of the device, he should
 * not worry about this class and just use arv_camera_new() or
 * arv_open_device().
 */

#include <arvinterface.h>

static GObjectClass *parent_class = NULL;

struct _ArvInterfacePrivate {
	GArray *device_ids;
};

static void
arv_interface_clear_device_ids (ArvInterface *interface)
{
	unsigned int i;

	for (i = 0; i < interface->priv->device_ids->len; i++) {
		g_free (g_array_index (interface->priv->device_ids, ArvInterfaceDeviceIds *, i)->device);
		g_free (g_array_index (interface->priv->device_ids, ArvInterfaceDeviceIds *, i)->physical);
		g_free (g_array_index (interface->priv->device_ids, ArvInterfaceDeviceIds *, i)->address);
	}
	g_array_set_size (interface->priv->device_ids, 0);
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
 * @interface: a #ArvInterface
 *
 * Updates the internal list of available devices. This may change the
 * connection between a list index and a device ID.
 *
 * Since: 0.2.0
 */

void
arv_interface_update_device_list (ArvInterface *interface)
{
	g_return_if_fail (ARV_IS_INTERFACE (interface));

	arv_interface_clear_device_ids (interface);

	ARV_INTERFACE_GET_CLASS (interface)->update_device_list (interface, interface->priv->device_ids);

	g_array_sort (interface->priv->device_ids, (GCompareFunc) _compare_device_ids);
}

/**
 * arv_interface_get_n_devices:
 * @interface: a #ArvInterface
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
arv_interface_get_n_devices (ArvInterface *interface)
{
	g_return_val_if_fail (ARV_IS_INTERFACE (interface), 0);
	g_return_val_if_fail (interface->priv->device_ids != NULL, 0);

	return interface->priv->device_ids->len;
}

/**
 * arv_interface_get_device_id:
 * @interface: a #ArvInterface
 * @index: device index
 *
 * Queries the unique device id corresponding to index.  Prior to this
 * call the @arv_interface_update_device_list function must be called.
 *
 * Returns: a unique device id
 *
 * Since: 0.2.0
 */

const char *
arv_interface_get_device_id (ArvInterface *interface, unsigned int index)
{
	g_return_val_if_fail (ARV_IS_INTERFACE (interface), 0);
	g_return_val_if_fail (interface->priv->device_ids != NULL, 0);

	if (index >= interface->priv->device_ids->len)
		return NULL;

	return g_array_index (interface->priv->device_ids, ArvInterfaceDeviceIds *, index)->device;
}

/**
 * arv_interface_get_device_physical_id:
 * @interface: a #ArvInterface
 * @index: device index
 *
 * Queries the physical device id corresponding to index such
 * as the MAC address for Ethernet based devices, bus id for PCI,
 * USB or Firewire based devices.
 *
 * Prior to this call the @arv_interface_update_device_list
 * function must be called.
 *
 * Returns: a physical device id
 *
 * Since: 0.2.0
 */

const char *
arv_interface_get_device_physical_id (ArvInterface *interface, unsigned int index)
{
	g_return_val_if_fail (ARV_IS_INTERFACE (interface), 0);
	g_return_val_if_fail (interface->priv->device_ids != NULL, 0);

	if (index >= interface->priv->device_ids->len)
		return NULL;

	return g_array_index (interface->priv->device_ids, ArvInterfaceDeviceIds *, index)->physical;
}

/**
 * arv_interface_get_device_address:
 * @interface: a #ArvInterface
 * @index: device index
 *
 * queries the device address (IP address in the case of an ethernet camera). Useful
 * for constructing manual connections to devices using @arv_gv_device_new
 *
 * Prior to this call the @arv_interface_update_device_list
 * function must be called.
 *
 * Returns: (transfer none): the device address
 *
 * Since: 0.2.0
 */

const char *
arv_interface_get_device_address (ArvInterface *interface, unsigned int index)
{
	g_return_val_if_fail (ARV_IS_INTERFACE (interface), 0);
	g_return_val_if_fail (interface->priv->device_ids != NULL, 0);

	if (index >= interface->priv->device_ids->len)
		return NULL;

	return g_array_index (interface->priv->device_ids, ArvInterfaceDeviceIds *, index)->address;
}

/**
 * arv_interface_open_device:
 * @interface: a #ArvInterface
 * @device_id: (allow-none): device unique id
 *
 * Creates a new #ArvDevice object corresponding to the given device id string. The first available device is returned if @device_id is null.
 *
 * Returns: (transfer full): a new #ArvDevice
 *
 * Since: 0.2.0
 */

ArvDevice *
arv_interface_open_device (ArvInterface *interface, const char *device_id)
{
	g_return_val_if_fail (ARV_IS_INTERFACE (interface), NULL);

	return ARV_INTERFACE_GET_CLASS (interface)->open_device (interface, device_id);
}

static void
arv_interface_init (ArvInterface *interface)
{
	interface->priv = G_TYPE_INSTANCE_GET_PRIVATE (interface, ARV_TYPE_INTERFACE, ArvInterfacePrivate);

	interface->priv->device_ids = g_array_new (FALSE, TRUE, sizeof (ArvInterfaceDeviceIds *));
}

static void
arv_interface_finalize (GObject *object)
{
	ArvInterface *interface = ARV_INTERFACE (object);

	parent_class->finalize (object);

	arv_interface_clear_device_ids (interface);
	g_array_free (interface->priv->device_ids, TRUE);
	interface->priv->device_ids = NULL;
}

static void
arv_interface_class_init (ArvInterfaceClass *interface_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (interface_class);

	g_type_class_add_private (interface_class, sizeof (ArvInterfacePrivate));

	parent_class = g_type_class_peek_parent (interface_class);

	object_class->finalize = arv_interface_finalize;
}

G_DEFINE_ABSTRACT_TYPE (ArvInterface, arv_interface, G_TYPE_OBJECT)
