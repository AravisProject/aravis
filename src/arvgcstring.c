/* Aravis - Digital camera library
 *
 * Copyright © 2009-2019 Emmanuel Pacaud
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
 * SECTION: arvgcstring
 * @short_description: String interface
 */

#include <arvgcstring.h>
#include <arvmisc.h>

static void
arv_gc_string_default_init (ArvGcStringInterface *gc_string_iface)
{
}

G_DEFINE_INTERFACE (ArvGcString, arv_gc_string, G_TYPE_OBJECT)

/**
 * arv_gc_string_get_value:
 * @gc_string: an object implementing #ArvGcString
 * @error: a #GError placeholder, or %NULL to ignore
 *
 * <warning><para>Please note the string content is still owned by the @gc_string object, which means the returned pointer may not be still valid after a new call to this function.</para></warning>
 *
 * Returns: the string value.
 */

const char *
arv_gc_string_get_value (ArvGcString *gc_string, GError **error)
{
	g_return_val_if_fail (ARV_IS_GC_STRING (gc_string), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return ARV_GC_STRING_GET_IFACE (gc_string)->get_value (gc_string, error);
}

/**
 * arv_gc_string_set_value:
 * @gc_string: an object implementing #ArvGcString
 * @value: new string value
 * @error: a #GError placeholder, or %NULL to ignore
 *
 * Set @value as the new @gc_string value.
 */

void
arv_gc_string_set_value (ArvGcString *gc_string, const char *value, GError **error)
{
	g_return_if_fail (ARV_IS_GC_STRING (gc_string));
	g_return_if_fail (error == NULL || *error == NULL);

	ARV_GC_STRING_GET_IFACE (gc_string)->set_value (gc_string, value, error);
}

/**
 * arv_gc_string_get_max_length:
 * @gc_string: an object implementing #ArvGcString
 * @error: a #GError placeholder, or %NULL to ignore
 *
 * Returns: the maximum length @gc_string can store, excluding the NULL terminal character.
 */

gint64
arv_gc_string_get_max_length (ArvGcString *gc_string, GError **error)
{
	ArvGcStringInterface *string_interface;

	g_return_val_if_fail (ARV_IS_GC_STRING (gc_string), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	string_interface = ARV_GC_STRING_GET_IFACE (gc_string);

	if (string_interface->get_max_length != NULL)
		return string_interface->get_max_length (gc_string, error);
	else
		return 0;
}
