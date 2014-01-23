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
#include <arvgvinterface.h>
#include <arvfakeinterface.h>
#include <arvdevice.h>
#include <arvdebug.h>
#include <string.h>

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
	{	.interface_id = "GigEVision",
		.is_available = TRUE,
		.get_interface_instance = arv_gv_interface_get_instance,
		.destroy_interface_instance = arv_gv_interface_destroy_instance
	}
};

unsigned int
arv_get_n_interfaces (void)
{
	return G_N_ELEMENTS (interfaces);
}

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
 * Enable an interface by name. By default, all interfaces are enabled, except "Fake".
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
 * Disable an interface by name. By default, all interfaces are enabled, except "Fake".
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

void
arv_update_device_list (void)
{
	unsigned int i;

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++) {
		ArvInterface *interface;
		
		if (interfaces[i].is_available) {
			interface = interfaces[i].get_interface_instance ();
			arv_interface_update_device_list (interface);
		}
	}
}

unsigned int
arv_get_n_devices (void)
{
	unsigned int n_devices = 0;
	unsigned int i;

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++) {
		ArvInterface *interface;

		if (interfaces[i].is_available) {
			interface = interfaces[i].get_interface_instance ();
			n_devices += arv_interface_get_n_devices (interface);
		}
	}

	return n_devices;
}

const char *
arv_get_device_id (unsigned int index)
{
	unsigned int offset = 0;
	unsigned int i;

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++) {
		ArvInterface *interface;
		unsigned int n_devices;

		if (interfaces[i].is_available) {
			interface = interfaces[i].get_interface_instance ();
			n_devices = arv_interface_get_n_devices (interface);

			if (index - offset < n_devices)
				return arv_interface_get_device_id (interface, index - offset);

			offset += n_devices;
		}
	}

	return NULL;
}

/**
 * arv_open_device:
 * @device_id: (allow-none): a device identifier string
 *
 * Open a device corresponding to the given identifier. A null string makes this function return the first available device.
 *
 * Return value: (transfer full): a new #ArvDevice instance
 */

ArvDevice *
arv_open_device (const char *device_id)
{
	unsigned int i;

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++) {
		ArvInterface *interface;
		ArvDevice *device;

		if (interfaces[i].is_available) {
			interface = interfaces[i].get_interface_instance ();
			device = arv_interface_open_device (interface, device_id);
			if (device != NULL)
				return device;
		}
	}

	return NULL;
}

void
arv_shutdown (void)
{
	unsigned int i;

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++)
		interfaces[i].destroy_interface_instance ();

	arv_debug_shutdown ();
}
