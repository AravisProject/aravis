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

#include <arvinterface.h>

static GObjectClass *parent_class = NULL;

void
arv_interface_update_device_list (ArvInterface *interface)
{
	g_return_if_fail (ARV_IS_INTERFACE (interface));

	ARV_INTERFACE_GET_CLASS (interface)->update_device_list (interface);
}

ArvDevice *
arv_interface_new_device (ArvInterface *interface, const char *name)
{
	ArvDevice *device;

	g_return_val_if_fail (ARV_IS_INTERFACE (interface), NULL);

	device = ARV_INTERFACE_GET_CLASS (interface)->new_device (interface, name);

	if (device != NULL)
		return device;

	arv_interface_update_device_list (interface);

	return ARV_INTERFACE_GET_CLASS (interface)->new_device (interface, name);
}

static void
arv_interface_init (ArvInterface *interface)
{
}

static void
arv_interface_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_interface_class_init (ArvInterfaceClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_interface_finalize;
}

G_DEFINE_ABSTRACT_TYPE (ArvInterface, arv_interface, G_TYPE_OBJECT)
