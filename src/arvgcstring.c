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
 * SECTION: arvgcstring
 * @short_description: String interface
 */

#include <arvgcprivate.h>
#include <arvgcstring.h>
#include <arvmisc.h>
#include <arvgcfeaturenodeprivate.h>

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
 * The string content is still owned by the @gc_string object, which means the returned pointer may not be valid
 * anymore after a new call to this function. This function is not thread safe: the string may be changed by a
 * feature access from another thread. Use [method@ArvGcString.dup_value] instead.
 *
 * Returns: the string value.
 */

const char *
arv_gc_string_get_value (ArvGcString *gc_string, GError **error)
{
	ArvGc *genicam;
	const char *value = NULL;

	g_return_val_if_fail (ARV_IS_GC_STRING (gc_string), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	genicam = arv_gc_node_lock_genicam (gc_string);

        if (arv_gc_feature_node_check_read_access (ARV_GC_FEATURE_NODE (gc_string), error))
		value = ARV_GC_STRING_GET_IFACE (gc_string)->get_value (gc_string, error);

	arv_gc_node_unlock_genicam (genicam);

	return value;
}

/**
 * arv_gc_string_dup_value:
 * @gc_string: an object implementing #ArvGcString
 * @error: a #GError placeholder, or %NULL to ignore
 *
 * Thread safe version of [method@ArvGcString.get_value].
 *
 * Returns: (transfer full): a newly allocated copy of the string value, %NULL on error.
 *
 * Since: 0.10.0
 */

char *
arv_gc_string_dup_value (ArvGcString *gc_string, GError **error)
{
	ArvGc *genicam;
	char *value;

	g_return_val_if_fail (ARV_IS_GC_STRING (gc_string), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	genicam = arv_gc_node_lock_genicam (gc_string);
	value = g_strdup (arv_gc_string_get_value (gc_string, error));
	arv_gc_node_unlock_genicam (genicam);

	return value;
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
	ArvGc *genicam;

	g_return_if_fail (ARV_IS_GC_STRING (gc_string));
	g_return_if_fail (error == NULL || *error == NULL);

	genicam = arv_gc_node_lock_genicam (gc_string);

        if (arv_gc_feature_node_check_write_access (ARV_GC_FEATURE_NODE (gc_string), error))
		ARV_GC_STRING_GET_IFACE (gc_string)->set_value (gc_string, value, error);

	arv_gc_node_unlock_genicam (genicam);
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
	ArvGc *genicam;
	gint64 value = 0;

	g_return_val_if_fail (ARV_IS_GC_STRING (gc_string), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	string_interface = ARV_GC_STRING_GET_IFACE (gc_string);

	genicam = arv_gc_node_lock_genicam (gc_string);

	if (string_interface->get_max_length != NULL)
		value = string_interface->get_max_length (gc_string, error);

	arv_gc_node_unlock_genicam (genicam);

	return value;
}
