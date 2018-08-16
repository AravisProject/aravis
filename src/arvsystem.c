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

#include <arvsystem.h>
#include <arvgvinterfaceprivate.h>
#include <arvconfig.h>
#ifdef ARAVIS_BUILD_USB
#include <arvuvinterfaceprivate.h>
#endif
#include <arvfakeinterfaceprivate.h>
#include <arvdevice.h>
#include <arvdebug.h>
#include <string.h>
#include <arvmisc.h>

static GMutex arv_system_mutex;

/**
 * SECTION: arv
 * @short_description: Device discovery and instantiation
 *
 * This module contans a set of APIs that allows to list and enable/disable the available interfaces,
 * and list and instantiate devices.
 */

typedef struct {
	const char *interface_id;
	gboolean is_available;
	ArvInterface * 	(*get_interface_instance) 	(void);
	void 		(*destroy_interface_instance) 	(void);
} ArvInterfaceInfos;

ArvInterfaceInfos interfaces[] = {
	{
		.interface_id = "Fake",
		.is_available = FALSE,
		.get_interface_instance = arv_fake_interface_get_instance,
		.destroy_interface_instance =  arv_fake_interface_destroy_instance
	},
#ifdef ARAVIS_BUILD_USB
	{	.interface_id = "USB3Vision",
		.is_available = TRUE,
		.get_interface_instance = arv_uv_interface_get_instance,
		.destroy_interface_instance = arv_uv_interface_destroy_instance
	},
#endif
	{	.interface_id = "GigEVision",
		.is_available = TRUE,
		.get_interface_instance = arv_gv_interface_get_instance,
		.destroy_interface_instance = arv_gv_interface_destroy_instance
	}
};

/**
 * arv_get_n_interfaces:
 *
 * Gets the number of available interfaces, including the disabled ones.
 *
 * Returns: The number of available interfaces.
 */

unsigned int
arv_get_n_interfaces (void)
{
	return G_N_ELEMENTS (interfaces);
}


/**
 * arv_get_interface_id:
 * @index: interface index
 *
 * Retrieves the interface identifier. Possible values are 'Fake', 'USB3Vision'
 * and 'GigEVision'.
 *
 * Returns: The interfae identifier string.
 */

const char *
arv_get_interface_id (unsigned int index)
{
	if (index >= G_N_ELEMENTS (interfaces))
		return NULL;

	return interfaces[index].interface_id;
}

/**
 * arv_enable_interface:
 * @interface_id: name of the interface
 *
 * Enable an interface by name. By default, all interfaces are enabled, except 'Fake'.
 */

void
arv_enable_interface (const char *interface_id)
{
	guint i;

	g_return_if_fail (interface_id != NULL);

	for (i = 0; i < G_N_ELEMENTS (interfaces) ; i++)
		if (strcmp (interface_id, interfaces[i].interface_id) == 0) {
			interfaces[i].is_available = TRUE;
			return;
		}

	g_warning ("[Arv::enable_interface] Unknown interface '%s'", interface_id);
}

/**
 * arv_disable_interface:
 * @interface_id: name of the interface
 *
 * Disable an interface by name. By default, all interfaces are enabled, except 'Fake'.
 */

void
arv_disable_interface (const char *interface_id)
{
	guint i;

	g_return_if_fail (interface_id != NULL);

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++)
		if (strcmp (interface_id, interfaces[i].interface_id) == 0) {
			interfaces[i].is_available = FALSE;
			return;
		}

	g_warning ("[Arv::enable_interface] Unknown interface '%s'", interface_id);
}

/**
 * arv_update_device_list:
 *
 * Updates the list of currently online devices.
 **/

void
arv_update_device_list (void)
{
	unsigned int i;

	g_mutex_lock (&arv_system_mutex);

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++) {
		ArvInterface *interface;

		if (interfaces[i].is_available) {
			interface = interfaces[i].get_interface_instance ();
			arv_interface_update_device_list (interface);
		}
	}

	g_mutex_unlock (&arv_system_mutex);
}

/**
 * arv_get_n_devices:
 *
 * Retrieves the number of currently online devices. This value is valid until
 * the next call to arv_update_device_list().
 *
 * Returns: The number of currently online devices.
 **/

unsigned int
arv_get_n_devices (void)
{
	unsigned int n_devices = 0;
	unsigned int i;

	g_mutex_lock (&arv_system_mutex);

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++) {
		ArvInterface *interface;

		if (interfaces[i].is_available) {
			interface = interfaces[i].get_interface_instance ();
			n_devices += arv_interface_get_n_devices (interface);
		}
	}

	g_mutex_unlock (&arv_system_mutex);

	return n_devices;
}

static const char *
arv_get_info (unsigned int index, const char *get_info (ArvInterface *, guint))
{
	unsigned int offset = 0;
	unsigned int i;
	const char *info;

	g_mutex_lock (&arv_system_mutex);

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++) {
		ArvInterface *interface;
		unsigned int n_devices;

		if (interfaces[i].is_available) {
			interface = interfaces[i].get_interface_instance ();
			n_devices = arv_interface_get_n_devices (interface);

			if (index - offset < n_devices) {
				info = get_info (interface, index - offset);

				g_mutex_unlock (&arv_system_mutex);

				return info;
			}

			offset += n_devices;
		}
	}

	g_mutex_unlock (&arv_system_mutex);

	return NULL;
}

/**
 * arv_get_device_id:
 * @index: device index
 *
 * Queries the unique device id corresponding to index.  Prior to this
 * call the arv_update_device_list() function must be called.
 *
 * Returns: a unique device id
 *
 * Since: 0.2.0
 */

const char *
arv_get_device_id (unsigned int index)
{
	return arv_get_info (index, arv_interface_get_device_id);
}

/**
 * arv_get_device_physical_id:
 * @index: device index
 *
 * Queries the physical device id corresponding to index such
 * as the MAC address for Ethernet based devices, bus id for PCI,
 * USB or Firewire based devices.
 *
 * Prior to this call the arv_update_device_list()
 * function must be called.
 *
 * Returns: a physical device id
 *
 * Since: 0.2.0
 */

const char *
arv_get_device_physical_id (unsigned int index)
{
	return arv_get_info (index, arv_interface_get_device_physical_id);
}

/**
 * arv_get_device_vendor:
 * @index: device index
 *
 * Queries the device vendor.
 *
 * Prior to this call the arv_update_device_list()
 * function must be called.
 *
 * Returns: (transfer none): the device vendor, NULL on error
 *
 * Since: 0.6.0
 */

const char *
arv_get_device_vendor	(unsigned int index)
{
	return arv_get_info (index, arv_interface_get_device_vendor);
}

/**
 * arv_get_device_model:
 * @index: device index
 *
 * Queries the device model.
 *
 * Prior to this call the arv_update_device_list()
 * function must be called.
 *
 * Returns: (transfer none): the device model, NULL on error
 *
 * Since: 0.6.0
 */

const char *
arv_get_device_model (unsigned int index)
{
	return arv_get_info (index, arv_interface_get_device_model);
}

/**
 * arv_get_device_serial_nbr:
 * @index: device index
 *
 * Queries the device serial.
 *
 * Prior to this call the arv_update_device_list()
 * function must be called.
 *
 * Returns: (transfer none): the device serial, NULL on error
 *
 * Since: 0.6.0
 */

const char *
arv_get_device_serial_nbr (unsigned int index)
{
	return arv_get_info (index, arv_interface_get_device_serial_nbr);
}

/**
 * arv_get_device_address:
 * @index: device index
 *
 * The index of a device may change after a call to arv_update_device_list().
 *
 * Returns: The address of the device corresponding to @index as a string.
 *
 * Since: 0.6.0
 */

const char *
arv_get_device_address (unsigned int index)
{
	return arv_get_info (index, arv_interface_get_device_address);
}

/**
 * arv_get_device_protocol:
 * @index: device index
 *
 * The index of a device may change after a call to arv_update_device_list().
 *
 * Returns: The device protocol as a string.
 *
 * Since: 0.6.0
 */

const char *
arv_get_device_protocol	(unsigned int index)
{
	return arv_get_info (index, arv_interface_get_device_protocol);
}

/**
 * arv_open_device:
 * @device_id: (allow-none): a device identifier string
 *
 * Open a device corresponding to the given identifier. A %NULL string makes
 * this function return the first available device.
 *
 * Return value: (transfer full): A new #ArvDevice instance.
 *
 * Since: 0.2.0
 */

ArvDevice *
arv_open_device (const char *device_id)
{
	unsigned int i;

	g_mutex_lock (&arv_system_mutex);

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++) {
		ArvInterface *interface;
		ArvDevice *device;

		if (interfaces[i].is_available) {
			interface = interfaces[i].get_interface_instance ();
			device = arv_interface_open_device (interface, device_id);
			if (device != NULL) {
				g_mutex_unlock (&arv_system_mutex);
				return device;
			}
		}
	}

	g_mutex_unlock (&arv_system_mutex);

	return NULL;
}

/**
 * arv_shutdown:
 *
 * Frees a number of ressources allocated by Aravis that would be otherwise
 * reported as memory leak by tools like Valgrind. The call to this function is
 * optional if you don't intend to check for memmory leaks.
 */

void
arv_shutdown (void)
{
	unsigned int i;

	g_mutex_lock (&arv_system_mutex);

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++)
		interfaces[i].destroy_interface_instance ();

	arv_debug_shutdown ();

	g_mutex_unlock (&arv_system_mutex);
}
