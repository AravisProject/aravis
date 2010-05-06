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

#include <arvdevice.h>
#include <arvgc.h>
#include <arvstream.h>

static GObjectClass *parent_class = NULL;

ArvStream *
arv_device_new_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	return ARV_DEVICE_GET_CLASS (device)->new_stream (device, callback, user_data);
}

gboolean
arv_device_read_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (buffer != NULL, FALSE);
	g_return_val_if_fail (size > 0, FALSE);

	return ARV_DEVICE_GET_CLASS (device)->read_memory (device, address, size, buffer);
}

gboolean
arv_device_write_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (buffer != NULL, FALSE);
	g_return_val_if_fail (size > 0, FALSE);

	return ARV_DEVICE_GET_CLASS (device)->write_memory (device, address, size, buffer);
}

gboolean
arv_device_read_register (ArvDevice *device, guint32 address, guint32 *value)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	return ARV_DEVICE_GET_CLASS (device)->read_register (device, address, value);
}

gboolean
arv_device_write_register (ArvDevice *device, guint32 address, guint32 value)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);

	return ARV_DEVICE_GET_CLASS (device)->write_register (device, address, value);
}

ArvGc *
arv_device_get_genicam (ArvDevice *device)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	return ARV_DEVICE_GET_CLASS (device)->get_genicam (device);
}

static void
arv_device_init (ArvDevice *device)
{
}

static void
arv_device_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_device_class_init (ArvDeviceClass *device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (device_class);

	parent_class = g_type_class_peek_parent (device_class);

	object_class->finalize = arv_device_finalize;
}

G_DEFINE_ABSTRACT_TYPE (ArvDevice, arv_device, G_TYPE_OBJECT)
