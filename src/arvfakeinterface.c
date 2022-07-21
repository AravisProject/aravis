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
 * SECTION: arvfakeinterface
 * @short_description: Fake interface
 */

#include <arvfakeinterfaceprivate.h>
#include <arvfakedeviceprivate.h>
#include <arvinterfaceprivate.h>
#include <arvfakedevice.h>
#include <arvdebug.h>
#include <arvmisc.h>

#define ARV_FAKE_DEVICE_ID 	        "Fake_1"
#define ARV_FAKE_PHYSICAL_ID 	        "Fake_1"
#define ARV_FAKE_ADDRESS 	        "0.0.0.0"
#define ARV_FAKE_VENDOR 	        "Aravis"
#define ARV_FAKE_MANUFACTURER_INFO      "none"
#define ARV_FAKE_MODEL 		        "Fake"
#define ARV_FAKE_SERIAL		        "1"

struct _ArvFakeInterface {
	ArvInterface	interface;
};

struct _ArvFakeInterfaceClass {
	ArvInterfaceClass parent_class;
};

G_DEFINE_TYPE (ArvFakeInterface, arv_fake_interface, ARV_TYPE_INTERFACE)

static void
arv_fake_interface_update_device_list (ArvInterface *interface, GArray *device_ids)
{
	ArvInterfaceDeviceIds *ids;

	g_assert (device_ids->len == 0);

	ids = g_new0 (ArvInterfaceDeviceIds, 1);

	ids->device = g_strdup (ARV_FAKE_DEVICE_ID);
	ids->physical = g_strdup (ARV_FAKE_PHYSICAL_ID);
	ids->address = g_strdup (ARV_FAKE_ADDRESS);
	ids->vendor = g_strdup (ARV_FAKE_VENDOR);
        ids->manufacturer_info = g_strdup (ARV_FAKE_MANUFACTURER_INFO);
	ids->model = g_strdup (ARV_FAKE_MODEL);
	ids->serial_nbr = g_strdup (ARV_FAKE_SERIAL);

	g_array_append_val (device_ids, ids);
}

static ArvDevice *
arv_fake_interface_open_device (ArvInterface *interface, const char *device_id, GError **error)
{
	if (g_strcmp0 (device_id, ARV_FAKE_DEVICE_ID) == 0 ||
	    g_strcmp0 (device_id, ARV_FAKE_PHYSICAL_ID) == 0)
		return arv_fake_device_new ("1", error);

	return NULL;
}

static ArvInterface *arv_fake_interface = NULL;
static GMutex arv_fake_interface_mutex;

/**
 * arv_fake_interface_get_instance:
 *
 * Gets the unique instance of the fake interface.
 *
 * Returns: (transfer none): a #ArvInterface singleton.
 */

ArvInterface *
arv_fake_interface_get_instance (void)
{
	g_mutex_lock (&arv_fake_interface_mutex);

	if (arv_fake_interface == NULL)
		arv_fake_interface = g_object_new (ARV_TYPE_FAKE_INTERFACE, NULL);

	g_mutex_unlock (&arv_fake_interface_mutex);

	return ARV_INTERFACE (arv_fake_interface);
}

void
arv_fake_interface_destroy_instance (void)
{
	g_mutex_lock (&arv_fake_interface_mutex);

	if (arv_fake_interface != NULL) {
		g_object_unref (arv_fake_interface);
		arv_fake_interface = NULL;
	}

	g_mutex_unlock (&arv_fake_interface_mutex);
}

static void
arv_fake_interface_init (ArvFakeInterface *fake_interface)
{
}

static void
arv_fake_interface_finalize (GObject *object)
{
	G_OBJECT_CLASS (arv_fake_interface_parent_class)->finalize (object);
}

static void
arv_fake_interface_class_init (ArvFakeInterfaceClass *fake_interface_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (fake_interface_class);
	ArvInterfaceClass *interface_class = ARV_INTERFACE_CLASS (fake_interface_class);

	object_class->finalize = arv_fake_interface_finalize;

	interface_class->update_device_list = arv_fake_interface_update_device_list;
	interface_class->open_device = arv_fake_interface_open_device;

	interface_class->protocol = "Fake";
}
