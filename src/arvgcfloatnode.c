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

/**
 * SECTION: arvgcfloatnode
 * @short_description: Class for Float nodes
 */

#include <arvgcfloatnode.h>
#include <arvgcfloat.h>
#include <arvgc.h>
#include <arvtools.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvGcNode implementation */

static void
arv_gc_float_node_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (node);
	double value;
	char *value_str = (char *) content;

	if (strcmp (name, "Value") == 0) {
		arv_str_parse_double (&value_str, &value);
		arv_force_g_value_to_double (&gc_float_node->value, value);
	} else if (strcmp (name, "Min") == 0) {
		arv_str_parse_double (&value_str, &value);
		arv_force_g_value_to_double (&gc_float_node->minimum, value);
	} else if (strcmp (name, "Max") == 0) {
		arv_str_parse_double (&value_str, &value);
		arv_force_g_value_to_double (&gc_float_node->maximum, value);
	} else if (strcmp (name, "Inc") == 0) {
		arv_force_g_value_to_int64 (&gc_float_node->increment,
					    g_ascii_strtoll (content, NULL, 0));
	} else if (strcmp (name, "pValue") == 0) {
		arv_force_g_value_to_string (&gc_float_node->value, content);
	} else if (strcmp (name, "pMin") == 0) {
		arv_force_g_value_to_string (&gc_float_node->minimum, content);
	} else if (strcmp (name, "pMax") == 0) {
		arv_force_g_value_to_string (&gc_float_node->maximum, content);
	} else if (strcmp (name, "pInc") == 0) {
		arv_force_g_value_to_string (&gc_float_node->increment, content);
	} else if (strcmp (name, "Unit") == 0) {
		g_free (gc_float_node->unit);
		gc_float_node->unit = g_strdup (content);
	} else
		ARV_GC_NODE_CLASS (parent_class)->add_element (node, name, content, attributes);
}

/* ArvGcFloatNode implementation */

static GType
arv_gc_float_node_get_value_type (ArvGcNode *node)
{
	return G_TYPE_DOUBLE;
}

ArvGcNode *
arv_gc_float_node_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_FLOAT_NODE, NULL);

	return node;
}

static void
arv_gc_float_node_init (ArvGcFloatNode *gc_float_node)
{
	g_value_init (&gc_float_node->value, G_TYPE_DOUBLE);
	g_value_init (&gc_float_node->minimum, G_TYPE_DOUBLE);
	g_value_init (&gc_float_node->maximum, G_TYPE_DOUBLE);
	g_value_init (&gc_float_node->increment, G_TYPE_INT64);

	g_value_set_double (&gc_float_node->value, 0);
	g_value_set_double (&gc_float_node->minimum, -G_MINDOUBLE);
	g_value_set_double (&gc_float_node->maximum, G_MAXDOUBLE);
	g_value_set_int64 (&gc_float_node->increment, 1);

	gc_float_node->unit = NULL;
}

static void
arv_gc_float_node_finalize (GObject *object)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (object);

	g_free (gc_float_node->unit);

	g_value_unset (&gc_float_node->value);
	g_value_unset (&gc_float_node->minimum);
	g_value_unset (&gc_float_node->maximum);
	g_value_unset (&gc_float_node->increment);

	parent_class->finalize (object);
}

static void
arv_gc_float_node_class_init (ArvGcFloatNodeClass *float_node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (float_node_class);
	ArvGcNodeClass *node_class = ARV_GC_NODE_CLASS (float_node_class);

	parent_class = g_type_class_peek_parent (float_node_class);

	object_class->finalize = arv_gc_float_node_finalize;

	node_class->add_element = arv_gc_float_node_add_element;
	node_class->get_value_type = arv_gc_float_node_get_value_type;
}

/* ArvGcFloat interface implementation */

static double
arv_gc_float_node_get_float_value (ArvGcFloat *gc_float)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_float));
	return arv_gc_get_double_from_value (genicam, &gc_float_node->value);
}

static void
arv_gc_float_node_set_float_value (ArvGcFloat *gc_float, double value)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_float));
	arv_gc_set_double_to_value (genicam, &gc_float_node->value, value);
}

static double
arv_gc_float_node_get_min (ArvGcFloat *gc_float)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_float));
	return arv_gc_get_double_from_value (genicam, &gc_float_node->minimum);
}

static double
arv_gc_float_node_get_max (ArvGcFloat *gc_float)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_float));
	return arv_gc_get_double_from_value (genicam, &gc_float_node->maximum);
}

static gint64
arv_gc_float_node_get_inc (ArvGcFloat *gc_float)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_float));
	return arv_gc_get_int64_from_value (genicam, &gc_float_node->increment);
}

static const char *
arv_gc_float_node_get_unit (ArvGcFloat *gc_float)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);

	return gc_float_node->unit;
}

static void
arv_gc_float_node_impose_min (ArvGcFloat *gc_float, double minimum)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_float));
	arv_gc_set_double_to_value (genicam, &gc_float_node->minimum, minimum);
}

static void
arv_gc_float_node_impose_max (ArvGcFloat *gc_float, double maximum)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_float));
	arv_gc_set_double_to_value (genicam, &gc_float_node->minimum, maximum);
}

static void
arv_gc_float_node_float_interface_init (ArvGcFloatInterface *interface)
{
	interface->get_value = arv_gc_float_node_get_float_value;
	interface->set_value = arv_gc_float_node_set_float_value;
	interface->get_min = arv_gc_float_node_get_min;
	interface->get_max = arv_gc_float_node_get_max;
	interface->get_inc = arv_gc_float_node_get_inc;
	interface->get_unit = arv_gc_float_node_get_unit;
	interface->impose_min = arv_gc_float_node_impose_min;
	interface->impose_max = arv_gc_float_node_impose_max;
}

G_DEFINE_TYPE_WITH_CODE (ArvGcFloatNode, arv_gc_float_node, ARV_TYPE_GC_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_FLOAT, arv_gc_float_node_float_interface_init))
