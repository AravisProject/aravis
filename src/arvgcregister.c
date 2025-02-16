/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
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

	ARV_GC_REGISTER_GET_IFACE (gc_register)->get (gc_register, buffer, length, error);
}

void
arv_gc_register_set (ArvGcRegister *gc_register, const void *buffer, guint64 length, GError **error)
{
	g_return_if_fail (ARV_IS_GC_REGISTER (gc_register));
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (length > 0);
	g_return_if_fail (error == NULL || *error == NULL);

	ARV_GC_REGISTER_GET_IFACE (gc_register)->set (gc_register, buffer, length, error);
}

guint64
arv_gc_register_get_address (ArvGcRegister *gc_register, GError **error)
{
	g_return_val_if_fail (ARV_IS_GC_REGISTER (gc_register), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	return ARV_GC_REGISTER_GET_IFACE (gc_register)->get_address (gc_register, error);
}

guint64
arv_gc_register_get_length (ArvGcRegister *gc_register, GError **error)
{
	g_return_val_if_fail (ARV_IS_GC_REGISTER (gc_register), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	return ARV_GC_REGISTER_GET_IFACE (gc_register)->get_length (gc_register, error);
}

/**
 * arv_gc_register_dup:
 * @gc_register: a #ArvGcRegister
 * @length: (out) (allow-none): register length
 * @error: a #GError placeholder
 *
 * Returns: the register feature content, must be freed using [func@GLib.free].
 *
 * Since: 0.8.31
 */

void *
arv_gc_register_dup (ArvGcRegister *gc_register, guint64 *length, GError **error)
{
        GError *local_error = NULL;
        void *buffer = NULL;
        guint64 register_length = 0;

        if (length != NULL)
                *length = 0;

        g_return_val_if_fail (ARV_IS_GC_REGISTER(gc_register), NULL);

        register_length = arv_gc_register_get_length(gc_register, &local_error);
        if (register_length < 65536 && local_error == NULL) {
                buffer = g_malloc (register_length);
                if (buffer != NULL)
                        arv_gc_register_get (gc_register, buffer, register_length, &local_error);
        }

        if (local_error != NULL) {
                g_clear_pointer (&buffer, g_free);
                register_length = 0;
                g_propagate_error (error, local_error);
        }

        if (length != NULL)
                *length = register_length;

        return buffer;
}

