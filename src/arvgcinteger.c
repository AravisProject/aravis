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
 * SECTION: arvgcinteger
 * @short_description: Integer interface
 */

#include <arvgcinteger.h>
#include <arvmisc.h>

static void
arv_gc_integer_default_init (ArvGcIntegerInterface *gc_integer_iface)
{
}

G_DEFINE_INTERFACE (ArvGcInteger, arv_gc_integer, G_TYPE_OBJECT)

gint64
arv_gc_integer_get_value (ArvGcInteger *gc_integer, GError **error)
{
	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	return ARV_GC_INTEGER_GET_INTERFACE (gc_integer)->get_value (gc_integer, error);
}

void
arv_gc_integer_set_value (ArvGcInteger *gc_integer, gint64 value, GError **error)
{
	g_return_if_fail (ARV_IS_GC_INTEGER (gc_integer));
	g_return_if_fail (error == NULL || *error == NULL);

	ARV_GC_INTEGER_GET_INTERFACE (gc_integer)->set_value (gc_integer, value, error);
}

gint64
arv_gc_integer_get_min (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	integer_interface = ARV_GC_INTEGER_GET_INTERFACE (gc_integer);

	if (integer_interface->get_min != NULL)
		return integer_interface->get_min (gc_integer, error);
	else
		return G_MININT64;
}

gint64
arv_gc_integer_get_max (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	integer_interface = ARV_GC_INTEGER_GET_INTERFACE (gc_integer);

	if (integer_interface->get_max != NULL)
		return integer_interface->get_max (gc_integer, error);
	else
		return G_MAXINT64;
}

gint64
arv_gc_integer_get_inc (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	integer_interface = ARV_GC_INTEGER_GET_INTERFACE (gc_integer);

	if (integer_interface->get_inc != NULL)
		return integer_interface->get_inc (gc_integer, error);
	else
		return 1;
}

const char *
arv_gc_integer_get_unit	(ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	integer_interface = ARV_GC_INTEGER_GET_INTERFACE (gc_integer);

	if (integer_interface->get_unit != NULL)
		return integer_interface->get_unit (gc_integer, error);
	else
		return NULL;
}

void arv_gc_integer_impose_min (ArvGcInteger *gc_integer, gint64 minimum, GError **error)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_if_fail (ARV_IS_GC_INTEGER (gc_integer));
	g_return_if_fail (error == NULL || *error == NULL);

	integer_interface = ARV_GC_INTEGER_GET_INTERFACE (gc_integer);

	if (integer_interface->impose_min != NULL)
		integer_interface->impose_min (gc_integer, minimum, error);
}

void arv_gc_integer_impose_max (ArvGcInteger *gc_integer, gint64 maximum, GError **error)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_if_fail (ARV_IS_GC_INTEGER (gc_integer));
	g_return_if_fail (error == NULL || *error == NULL);

	integer_interface = ARV_GC_INTEGER_GET_INTERFACE (gc_integer);

	if (integer_interface->impose_max != NULL)
		integer_interface->impose_max (gc_integer, maximum, error);
}
