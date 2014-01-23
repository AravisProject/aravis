/* Aravis - Digital camera library
 *
 * Copyright © 2009-2012 Emmanuel Pacaud
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
 * SECTION: arvgcregister
 * @short_description: Register interface
 */

#include <arvgcregister.h>
#include <arvmisc.h>

static void
arv_gc_register_default_init (ArvGcRegisterInterface *gc_register_iface)
{
}

G_DEFINE_INTERFACE (ArvGcRegister, arv_gc_register, G_TYPE_OBJECT)

void
arv_gc_register_get (ArvGcRegister *gc_register, void *buffer, guint64 length, GError **error)
{
	g_return_if_fail (ARV_IS_GC_REGISTER (gc_register));
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (length > 0);
	g_return_if_fail (error == NULL || *error == NULL);

	ARV_GC_REGISTER_GET_INTERFACE (gc_register)->get (gc_register, buffer, length, error);
}

void
arv_gc_register_set (ArvGcRegister *gc_register, void *buffer, guint64 length, GError **error)
{
	g_return_if_fail (ARV_IS_GC_REGISTER (gc_register));
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (length > 0);
	g_return_if_fail (error == NULL || *error == NULL);

	ARV_GC_REGISTER_GET_INTERFACE (gc_register)->set (gc_register, buffer, length, error);
}

guint64
arv_gc_register_get_address (ArvGcRegister *gc_register, GError **error)
{
	g_return_val_if_fail (ARV_IS_GC_REGISTER (gc_register), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	return ARV_GC_REGISTER_GET_INTERFACE (gc_register)->get_address (gc_register, error);
}

guint64
arv_gc_register_get_length (ArvGcRegister *gc_register, GError **error)
{
	g_return_val_if_fail (ARV_IS_GC_REGISTER (gc_register), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	return ARV_GC_REGISTER_GET_INTERFACE (gc_register)->get_length (gc_register, error);
}
