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
 * SECTION: arvgcintconverternode
 * @short_description: IntConverter node class
 */

#include <arvgcconverterprivate.h>
#include <arvgcintconverternode.h>
#include <arvgcfeaturenodeprivate.h>
#include <arvevaluator.h>
#include <arvgcinteger.h>

struct _ArvGcIntConverterNode {
	ArvGcConverter	converter;
};

struct _ArvGcIntConverterNodeClass {
	ArvGcConverterClass parent_class;
};

static const char *
arv_gc_int_converter_node_get_node_name (ArvDomNode *node)
{
	return "IntConverter";
}

ArvGcNode *
arv_gc_int_converter_node_new (void)
{
	return g_object_new (ARV_TYPE_GC_INT_CONVERTER_NODE, NULL);
}

static void
arv_gc_int_converter_node_init (ArvGcIntConverterNode *node)
{
}

static void
arv_gc_int_converter_node_class_init (ArvGcIntConverterNodeClass *this_class)
{
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);

	dom_node_class->get_node_name = arv_gc_int_converter_node_get_node_name;
}


static gint64
arv_gc_converter_get_integer_value (ArvGcInteger *gc_integer, GError **error)
{
	return arv_gc_converter_convert_to_int64 (ARV_GC_CONVERTER (gc_integer), ARV_GC_CONVERTER_NODE_TYPE_VALUE, error);
}

static gint64
arv_gc_converter_get_integer_min (ArvGcInteger *gc_integer, GError **error)
{
	GError *local_error = NULL;
	gint64 a, b;

	/* TODO: we should use the Slope node here, instead of using MIN (min, max) */

	a = arv_gc_converter_convert_to_int64 (ARV_GC_CONVERTER (gc_integer), ARV_GC_CONVERTER_NODE_TYPE_MIN, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_MININT64;
	}

	b = arv_gc_converter_convert_to_int64 (ARV_GC_CONVERTER (gc_integer), ARV_GC_CONVERTER_NODE_TYPE_MAX, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_MININT64;
	}

	return MIN (a, b);
}

static gint64
arv_gc_converter_get_integer_max (ArvGcInteger *gc_integer, GError **error)
{
	GError *local_error = NULL;
	gint64 a, b;

	/* TODO: we should use the Slope node here, instead of using MAX (min, max) */

	a = arv_gc_converter_convert_to_int64 (ARV_GC_CONVERTER (gc_integer), ARV_GC_CONVERTER_NODE_TYPE_MIN, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_MAXINT64;
	}

	b = arv_gc_converter_convert_to_int64 (ARV_GC_CONVERTER (gc_integer), ARV_GC_CONVERTER_NODE_TYPE_MAX, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_MAXINT64;
	}

	return MAX (a, b);
}

static gint64
_get_inc (ArvGcInteger *self, GError **error)
{
	/* Genicam 2.1.1 p. 41 says: The increment of an IntConvertor is always 1. */

	return 1;
}

static void
arv_gc_converter_set_integer_value (ArvGcInteger *gc_integer, gint64 value, GError **error)
{
	arv_gc_converter_convert_from_int64 (ARV_GC_CONVERTER (gc_integer), value, error);
}

static const char *
arv_gc_converter_get_integer_unit (ArvGcInteger *gc_integer, GError **error)
{
	return arv_gc_converter_get_unit (ARV_GC_CONVERTER (gc_integer), error);
}

static void
arv_gc_int_converter_node_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_converter_get_integer_value;
	interface->get_min = arv_gc_converter_get_integer_min;
	interface->get_max = arv_gc_converter_get_integer_max;
	interface->get_inc = _get_inc;
	interface->set_value = arv_gc_converter_set_integer_value;
	interface->get_unit = arv_gc_converter_get_integer_unit;
}

G_DEFINE_TYPE_WITH_CODE (ArvGcIntConverterNode, arv_gc_int_converter_node, ARV_TYPE_GC_CONVERTER,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER,   arv_gc_int_converter_node_integer_interface_init))

