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
 * SECTION: arvfakeinterface
 * @short_description: Fake camera interface
 */

#include <arvfakeinterface.h>
#include <arvfakedevice.h>
#include <arvdebug.h>
#include <arvmisc.h>

#define ARV_FAKE_DEVICE_ID "Fake_1"
#define ARV_FAKE_PHYSICAL_ID "Fake_1"
#define ARV_FAKE_ADDRESS "0.0.0.0"

static GObjectClass *parent_class = NULL;

struct _ArvFakeInterfacePrivate {
	GHashTable *devices;
};

static void
arv_fake_interface_update_device_list (ArvInterface *interface, GArray *device_ids)
{
	ArvInterfaceDeviceIds *ids;

	ids = g_new0 (ArvInterfaceDeviceIds, 1);

	g_array_set_size (device_ids, 0);

	ids->device = g_strdup (ARV_FAKE_DEVICE_ID);
	ids->physical = g_strdup (ARV_FAKE_PHYSICAL_ID);
	ids->address = g_strdup (ARV_FAKE_ADDRESS);

	g_array_append_val (device_ids, ids);
}

static ArvDevice *
arv_fake_interface_open_device (ArvInterface *interface, const char *device_id)
{
	if (g_strcmp0 (device_id, ARV_FAKE_DEVICE_ID) == 0)
		return arv_fake_device_new ("1");
	if (g_strcmp0 (device_id, ARV_FAKE_PHYSICAL_ID) == 0)
		return arv_fake_device_new ("1");

	return NULL;
}

static ArvInterface *fake_interface = NULL;
ARV_DEFINE_STATIC_MUTEX (fake_interface_mutex);

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
	arv_g_mutex_lock (&fake_interface_mutex);

	if (fake_interface == NULL)
		fake_interface = g_object_new (ARV_TYPE_FAKE_INTERFACE, NULL);

	arv_g_mutex_unlock (&fake_interface_mutex);

	return ARV_INTERFACE (fake_interface);
}

void
arv_fake_interface_destroy_instance (void)
{
	arv_g_mutex_lock (&fake_interface_mutex);

	if (fake_interface != NULL) {
		g_object_unref (fake_interface);
		fake_interface = NULL;
	}

	arv_g_mutex_unlock (&fake_interface_mutex);
}

static void
arv_fake_interface_init (ArvFakeInterface *fake_interface)
{
	fake_interface->priv = G_TYPE_INSTANCE_GET_PRIVATE (fake_interface, ARV_TYPE_FAKE_INTERFACE, ArvFakeInterfacePrivate);

	fake_interface->priv->devices = NULL;
}

static void
arv_fake_interface_finalize (GObject *object)
{
	ArvFakeInterface *fake_interface = ARV_FAKE_INTERFACE (object);

	if (fake_interface->priv->devices != NULL) {
		g_hash_table_unref (fake_interface->priv->devices);
		fake_interface->priv->devices = NULL;
	}

	parent_class->finalize (object);
}

static void
arv_fake_interface_class_init (ArvFakeInterfaceClass *fake_interface_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (fake_interface_class);
	ArvInterfaceClass *interface_class = ARV_INTERFACE_CLASS (fake_interface_class);

	g_type_class_add_private (fake_interface_class, sizeof (ArvFakeInterfacePrivate));

	parent_class = g_type_class_peek_parent (fake_interface_class);

	object_class->finalize = arv_fake_interface_finalize;

	interface_class->update_device_list = arv_fake_interface_update_device_list;
	interface_class->open_device = arv_fake_interface_open_device;
}

G_DEFINE_TYPE (ArvFakeInterface, arv_fake_interface, ARV_TYPE_INTERFACE)
