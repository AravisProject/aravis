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

#include <arvsystem.h>
#include <arvgvinterface.h>
#include <arvfakeinterface.h>
#include <arvdevice.h>

typedef struct {
	const char *interface_id;
	ArvInterface * 	(*get_interface_instance) 	(void);
	void 		(*destroy_interface_instance) 	(void);
} ArvInterfaceInfos;

ArvInterfaceInfos interfaces[] = {
	{"Fake", 		arv_fake_interface_get_instance,	arv_fake_interface_destroy_instance},
	{"GigE-Vision", 	arv_gv_interface_get_instance,		arv_gv_interface_destroy_instance}
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

void
arv_update_device_list (void)
{
	unsigned int i;

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++) {
		ArvInterface *interface;

		interface = interfaces[i].get_interface_instance ();
		arv_interface_update_device_list (interface);
	}
}

unsigned int
arv_get_n_devices (void)
{
	unsigned int n_devices = 0;
	unsigned int i;

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++) {
		ArvInterface *interface;

		interface = interfaces[i].get_interface_instance ();
		n_devices += arv_interface_get_n_devices (interface);
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

		interface = interfaces[i].get_interface_instance ();
		n_devices = arv_interface_get_n_devices (interface);

		if (index - offset < n_devices)
			return arv_interface_get_device_id (interface, index - offset);

		offset += n_devices;
	}

	return NULL;
}

/**
 * arv_open_device:
 * @device_id: (allow-none): a device identifier string
 * Return value: (transfer full): a new #ArvDevice instance
 *
 * Open a device corresponding to the given identifier. A null string makes this function return the first available device.
 */

ArvDevice *
arv_open_device (const char *device_id)
{
	unsigned int i;

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++) {
		ArvInterface *interface;
		ArvDevice *device;

		interface = interfaces[i].get_interface_instance ();
		device = arv_interface_open_device (interface, device_id);
		if (device != NULL)
			return device;
	}

	return NULL;
}

void
arv_shutdown (void)
{
	unsigned int i;

	for (i = 0; i < G_N_ELEMENTS (interfaces); i++)
		interfaces[i].destroy_interface_instance ();
}
