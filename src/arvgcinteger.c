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
 * SECTION: arvgcinteger
 * @short_description: Integer interface
 */

#include <arvgcinteger.h>
#include <arvgcfeaturenode.h>
#include <arvgc.h>
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

	return ARV_GC_INTEGER_GET_IFACE (gc_integer)->get_value (gc_integer, error);
}

void
arv_gc_integer_set_value (ArvGcInteger *gc_integer, gint64 value, GError **error)
{
	g_return_if_fail (ARV_IS_GC_INTEGER (gc_integer));
	g_return_if_fail (error == NULL || *error == NULL);

	ARV_GC_INTEGER_GET_IFACE (gc_integer)->set_value (gc_integer, value, error);
}

gint64
arv_gc_integer_get_min (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	integer_interface = ARV_GC_INTEGER_GET_IFACE (gc_integer);

	if (integer_interface->get_min != NULL)
		return integer_interface->get_min (gc_integer, error);

	g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_PROPERTY_NOT_DEFINED, "<Min> node not found for '%s'",
		     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_integer)));

	return G_MININT64;
}

gint64
arv_gc_integer_get_max (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	integer_interface = ARV_GC_INTEGER_GET_IFACE (gc_integer);

	if (integer_interface->get_max != NULL)
		return integer_interface->get_max (gc_integer, error);

	g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_PROPERTY_NOT_DEFINED, "<Max> node not found for '%s'",
		     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_integer)));

	return G_MAXINT64;
}

gint64
arv_gc_integer_get_inc (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	integer_interface = ARV_GC_INTEGER_GET_IFACE (gc_integer);

	if (integer_interface->get_inc != NULL)
		return integer_interface->get_inc (gc_integer, error);

	g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_PROPERTY_NOT_DEFINED, "<Inc> node not found for '%s'",
		     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_integer)));

	return 1;
}

/**
 * arv_gc_integer_get_representation:
 * @gc_integer: a #ArvGcInteger
 * @error: return location for a GError, or NULL
 *
 * Get number representation format.
 *
 * Returns: Number representation format as #ArvGcRepresentation.
 */

ArvGcRepresentation
arv_gc_integer_get_representation (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	integer_interface = ARV_GC_INTEGER_GET_IFACE (gc_integer);

	if (integer_interface->get_representation != NULL)
		return integer_interface->get_representation (gc_integer, error);

	return ARV_GC_REPRESENTATION_UNDEFINED;
}

const char *
arv_gc_integer_get_unit	(ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	integer_interface = ARV_GC_INTEGER_GET_IFACE (gc_integer);

	if (integer_interface->get_unit != NULL)
		return integer_interface->get_unit (gc_integer, error);

	g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_PROPERTY_NOT_DEFINED, "<Unit> node not found for '%s'",
		     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_integer)));

	return NULL;
}

void arv_gc_integer_impose_min (ArvGcInteger *gc_integer, gint64 minimum, GError **error)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_if_fail (ARV_IS_GC_INTEGER (gc_integer));
	g_return_if_fail (error == NULL || *error == NULL);

	integer_interface = ARV_GC_INTEGER_GET_IFACE (gc_integer);

	if (integer_interface->impose_min != NULL)
		integer_interface->impose_min (gc_integer, minimum, error);
	else
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_PROPERTY_NOT_DEFINED, "<Min> node not found for '%s'",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_integer)));
}

void arv_gc_integer_impose_max (ArvGcInteger *gc_integer, gint64 maximum, GError **error)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_if_fail (ARV_IS_GC_INTEGER (gc_integer));
	g_return_if_fail (error == NULL || *error == NULL);

	integer_interface = ARV_GC_INTEGER_GET_IFACE (gc_integer);

	if (integer_interface->impose_max != NULL)
		integer_interface->impose_max (gc_integer, maximum, error);
	else
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_PROPERTY_NOT_DEFINED, "<Max> node not found for '%s'",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_integer)));
}
