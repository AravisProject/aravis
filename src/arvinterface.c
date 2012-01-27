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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvinterface
 * @short_description: Base abstract class for camera discovery
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

/**
 * arv_interface_update_device_list
 * @interface: a #ArvInterface
 *
 * Updates the internal list of available devices. This may change the
 * connection between a list index and a device ID.
 **/

void
arv_interface_update_device_list (ArvInterface *interface)
{
	g_return_if_fail (ARV_IS_INTERFACE (interface));

	ARV_INTERFACE_GET_CLASS (interface)->update_device_list (interface, interface->priv->device_ids);
}

/**
 * arv_interface_get_n_devices
 * @interface: a #ArvInterface
 * Return value: the number of available devices 
 *
 * Queries the number of available devices on this interface. Prior to this
 * call the @arv_interface_update_device_list function must be called. The list content will not
 * change until the next call of the update function.
 **/

unsigned int
arv_interface_get_n_devices (ArvInterface *interface)
{
	g_return_val_if_fail (ARV_IS_INTERFACE (interface), 0);
	g_return_val_if_fail (interface->priv->device_ids != NULL, 0);

	return interface->priv->device_ids->len;
}

/**
 * arv_interface_get_device_id
 * @interface: a #ArvInterface
 * @index: device index
 * Return value: a unique device id
 *
 * Queries the unique device id corresponding to index.  Prior to this
 * call the @arv_interface_update_device_list function must be called.
 **/

const char *
arv_interface_get_device_id (ArvInterface *interface, unsigned int index)
{
	g_return_val_if_fail (ARV_IS_INTERFACE (interface), 0);
	g_return_val_if_fail (interface->priv->device_ids != NULL, 0);

	if (index >= interface->priv->device_ids->len)
		return NULL;

	return g_array_index (interface->priv->device_ids, char **, index)[0];
}

/**
 * arv_interface_get_device_physical_id
 * @interface: a #ArvInterface
 * @index: device index
 * Return value: a physical device id
 *
 * Queries the physical device id corresponding to index such
 * as the MAC address for Ethernet based devices, bus id for PCI,
 * USB or Firewire based devices.
 *
 * Prior to this call the @arv_interface_update_device_list
 * function must be called.
 **/

const char *
arv_interface_get_device_physical_id (ArvInterface *interface, unsigned int index)
{
	g_return_val_if_fail (ARV_IS_INTERFACE (interface), 0);
	g_return_val_if_fail (interface->priv->device_ids != NULL, 0);

	if (index >= interface->priv->device_ids->len)
		return NULL;

	return g_array_index (interface->priv->device_ids, char **, index)[1];
}
/**
 * arv_interface_open_device
 * @interface: a #ArvInterface
 * @device_id: (allow-none): device unique id
 * Return value: (transfer full): a new #ArvDevice
 *
 * Creates a new #ArvDevice object corresponding to the given device id string. The first available device is returned if @device_id is null.
 **/

ArvDevice *
arv_interface_open_device (ArvInterface *interface, const char *device_id)
{
	ArvDevice *device;

	g_return_val_if_fail (ARV_IS_INTERFACE (interface), NULL);

	device = ARV_INTERFACE_GET_CLASS (interface)->open_device (interface, device_id);

	if (device != NULL)
		return device;

	arv_interface_update_device_list (interface);

	return ARV_INTERFACE_GET_CLASS (interface)->open_device (interface, device_id);
}

static void
arv_interface_init (ArvInterface *interface)
{
	interface->priv = G_TYPE_INSTANCE_GET_PRIVATE (interface, ARV_TYPE_INTERFACE, ArvInterfacePrivate);

	interface->priv->device_ids = g_array_new (FALSE, TRUE, sizeof (char **));
}

static void
arv_interface_finalize (GObject *object)
{
	ArvInterface *interface = ARV_INTERFACE (object);
	unsigned int i;

	parent_class->finalize (object);

	for (i = 0; i < interface->priv->device_ids->len; i++)
		g_strfreev (g_array_index (interface->priv->device_ids, char **, i));
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
