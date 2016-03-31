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
 * SECTION: arvuvinterface
 * @short_description: USB3 camera interface
 */

#include <arvuvinterface.h>
#include <arvdebug.h>
#include <arvmisc.h>

/* ArvUvInterface implementation */

static GObjectClass *parent_class = NULL;

struct _ArvUvInterfacePrivate {
	GHashTable *devices;
};

static void
arv_uv_interface_update_device_list (ArvInterface *interface, GArray *device_ids)
{
}

static ArvDevice *
arv_uv_interface_open_device (ArvInterface *interface, const char *device_id)
{
	return NULL;
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
}

static void
arv_uv_interface_finalize (GObject *object)
{
	parent_class->finalize (object);
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
