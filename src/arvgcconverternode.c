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
 * SECTION: arvgcconverternode
 * @short_description: Converter node class
 */

#include <arvgcconverterprivate.h>
#include <arvgcconverternode.h>
#include <arvgcfeaturenodeprivate.h>
#include <arvevaluator.h>
#include <arvgcfloat.h>

struct _ArvGcConverterNode {
	ArvGcConverter	converter;
};

struct _ArvGcConverterNodeClass {
	ArvGcConverterClass parent_class;
};

static const char *
arv_gc_converter_node_get_node_name (ArvDomNode *node)
{
	return "Converter";
}

ArvGcNode *
arv_gc_converter_node_new (void)
{
	return g_object_new (ARV_TYPE_GC_CONVERTER_NODE, NULL);
}

static void
arv_gc_converter_node_init (ArvGcConverterNode *gc_converter_node)
{
}

static void
arv_gc_converter_node_class_init (ArvGcConverterNodeClass *this_class)
{
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);

	dom_node_class->get_node_name = arv_gc_converter_node_get_node_name;
}

static double
arv_gc_converter_get_float_value (ArvGcFloat *gc_float, GError **error)
{
	return arv_gc_converter_convert_to_double (ARV_GC_CONVERTER (gc_float), ARV_GC_CONVERTER_NODE_TYPE_VALUE, error);
}

static double
arv_gc_converter_get_float_min (ArvGcFloat *gc_float, GError **error)
{
	GError *local_error = NULL;
	double a, b;

	/* TODO: we should use the Slope node here, instead of using MIN (min, max) */

	a = arv_gc_converter_convert_to_double (ARV_GC_CONVERTER (gc_float), ARV_GC_CONVERTER_NODE_TYPE_MIN, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return -G_MAXDOUBLE;
	}

	b = arv_gc_converter_convert_to_double (ARV_GC_CONVERTER (gc_float), ARV_GC_CONVERTER_NODE_TYPE_MAX, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return -G_MAXDOUBLE;
	}

	return MIN (a, b);
}

static double
arv_gc_converter_get_float_max (ArvGcFloat *gc_float, GError **error)
{
	GError *local_error = NULL;
	double a, b;

	/* TODO: we should use the Slope node here, instead of using MAX (min, max) */

	a = arv_gc_converter_convert_to_double (ARV_GC_CONVERTER (gc_float), ARV_GC_CONVERTER_NODE_TYPE_MIN, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_MAXDOUBLE;
	}

	b = arv_gc_converter_convert_to_double (ARV_GC_CONVERTER (gc_float), ARV_GC_CONVERTER_NODE_TYPE_MAX, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_MAXDOUBLE;
	}

	return MAX (a, b);
}

static double
_get_inc (ArvGcFloat *gc_float, GError **error)
{
	GError *local_error = NULL;
	ArvGcIsLinear is_linear;

	is_linear = arv_gc_converter_get_is_linear (ARV_GC_CONVERTER (gc_float), &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 1.0;
	}

	if (is_linear == ARV_GC_IS_LINEAR_NO)
		return 1.0;

	return arv_gc_converter_convert_to_double (ARV_GC_CONVERTER (gc_float), ARV_GC_CONVERTER_NODE_TYPE_INC, &local_error);
}

static ArvGcRepresentation
arv_gc_converter_get_float_representation (ArvGcFloat *gc_float, GError **error)
{
	return arv_gc_converter_get_representation (ARV_GC_CONVERTER (gc_float), error);
}

static const char *
arv_gc_converter_get_float_unit (ArvGcFloat *gc_float)
{
	return arv_gc_converter_get_unit (ARV_GC_CONVERTER (gc_float));
}

static ArvGcDisplayNotation
arv_gc_converter_get_float_display_notation (ArvGcFloat *gc_float)
{
	return arv_gc_converter_get_display_notation (ARV_GC_CONVERTER (gc_float));
}

static gint64
arv_gc_converter_get_float_display_precision (ArvGcFloat *gc_float)
{
	return arv_gc_converter_get_display_precision (ARV_GC_CONVERTER (gc_float));
}

static void
arv_gc_converter_set_float_value (ArvGcFloat *gc_float, double value, GError **error)
{
	arv_gc_converter_convert_from_double (ARV_GC_CONVERTER (gc_float), value, error);
}

static void
arv_gc_converter_node_float_interface_init (ArvGcFloatInterface *interface)
{
	interface->get_value = arv_gc_converter_get_float_value;
	interface->get_min = arv_gc_converter_get_float_min;
	interface->get_max = arv_gc_converter_get_float_max;
	interface->get_inc = _get_inc;
	interface->set_value = arv_gc_converter_set_float_value;
	interface->get_representation = arv_gc_converter_get_float_representation;
	interface->get_unit = arv_gc_converter_get_float_unit;
	interface->get_display_notation = arv_gc_converter_get_float_display_notation;
	interface->get_display_precision = arv_gc_converter_get_float_display_precision;
}

G_DEFINE_TYPE_WITH_CODE (ArvGcConverterNode, arv_gc_converter_node, ARV_TYPE_GC_CONVERTER,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_FLOAT,  arv_gc_converter_node_float_interface_init))
