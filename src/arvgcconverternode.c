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
 * SECTION: arvgcconverternoe
 * @short_description: Converter node class
 */

#include <arvgcconverterprivate.h>
#include <arvgcconverternode.h>
#include <arvgcfeaturenodeprivate.h>
#include <arvevaluator.h>
#include <arvgcfloat.h>

static const char *
arv_gc_converter_node_get_node_name (ArvDomNode *node)
{
	return "Converter";
}

static GType
arv_gc_converter_node_get_value_type (ArvGcFeatureNode *node)
{
	return G_TYPE_DOUBLE;
}

static void
arv_gc_converter_node_set_value_from_string (ArvGcFeatureNode *node, const char *string, GError **error)
{
	arv_gc_float_set_value (ARV_GC_FLOAT (node), g_ascii_strtod (string, NULL),error);
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
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);

	dom_node_class->get_node_name = arv_gc_converter_node_get_node_name;
	gc_feature_node_class->get_value_type = arv_gc_converter_node_get_value_type;
	gc_feature_node_class->set_value_from_string = arv_gc_converter_node_set_value_from_string;
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

static const char *
arv_gc_converter_get_float_unit (ArvGcFloat *gc_float, GError **error)
{
	return arv_gc_converter_get_unit (ARV_GC_CONVERTER (gc_float), error);
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
	interface->set_value = arv_gc_converter_set_float_value;
	interface->get_unit = arv_gc_converter_get_float_unit;
}

G_DEFINE_TYPE_WITH_CODE (ArvGcConverterNode, arv_gc_converter_node, ARV_TYPE_GC_CONVERTER,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_FLOAT,  arv_gc_converter_node_float_interface_init))
