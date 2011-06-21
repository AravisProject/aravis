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
 * SECTION: arvgcintegernode
 * @short_description: Class for Integer nodes
 */

#include <arvgcintegernode.h>
#include <arvgcinteger.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <string.h>
#include <stdlib.h>

static GObjectClass *parent_class = NULL;

/* ArvGcNode implementation */

static const char *
arv_gc_integer_node_get_node_name (ArvGcNode *node)
{
	return "Integer";
}

static void
arv_gc_integer_node_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (node);

	if (strcmp (name, "Value") == 0) {
		arv_force_g_value_to_int64 (&gc_integer_node->value,
					    g_ascii_strtoll (content, NULL, 0));
	} else if (strcmp (name, "Min") == 0) {
		arv_force_g_value_to_int64 (&gc_integer_node->minimum,
					    g_ascii_strtoll (content, NULL, 0));
	} else if (strcmp (name, "Max") == 0) {
		arv_force_g_value_to_int64 (&gc_integer_node->maximum,
					    g_ascii_strtoll (content, NULL, 0));
	} else if (strcmp (name, "Inc") == 0) {
		arv_force_g_value_to_int64 (&gc_integer_node->increment,
					    g_ascii_strtoll (content, NULL, 0));
	} else if (strcmp (name, "pValue") == 0) {
		arv_force_g_value_to_string (&gc_integer_node->value, content);
	} else if (strcmp (name, "pMin") == 0) {
		arv_force_g_value_to_string (&gc_integer_node->minimum, content);
	} else if (strcmp (name, "pMax") == 0) {
		arv_force_g_value_to_string (&gc_integer_node->maximum, content);
	} else if (strcmp (name, "pInc") == 0) {
		arv_force_g_value_to_string (&gc_integer_node->increment, content);
	} else if (strcmp (name, "Unit") == 0) {
		g_free (gc_integer_node->unit);
		gc_integer_node->unit = g_strdup (content);
	} else
		ARV_GC_NODE_CLASS (parent_class)->add_element (node, name, content, attributes);
}

static GType
arv_gc_integer_node_get_value_type (ArvGcNode *node)
{
	return G_TYPE_INT64;
}

static void
arv_gc_integer_node_set_value_from_string (ArvGcNode *node, const char *string)
{
	arv_gc_integer_set_value (ARV_GC_INTEGER (node), g_ascii_strtoll (string, NULL, 0));
}

static const char *
arv_gc_integer_node_get_value_as_string (ArvGcNode *node)
{
	ArvGcIntegerNode *integer_node = ARV_GC_INTEGER_NODE (node);

	g_snprintf (integer_node->v_string, G_ASCII_DTOSTR_BUF_SIZE,
		    "%" G_GINT64_FORMAT, arv_gc_integer_get_value (ARV_GC_INTEGER (node)));

	return integer_node->v_string;
}

/* ArvGcIntegerNode implementation */

ArvGcNode *
arv_gc_integer_node_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_INTEGER_NODE, NULL);

	return node;
}

static void
arv_gc_integer_node_init (ArvGcIntegerNode *gc_integer_node)
{
	g_value_init (&gc_integer_node->value, G_TYPE_INT64);
	g_value_init (&gc_integer_node->minimum, G_TYPE_INT64);
	g_value_init (&gc_integer_node->maximum, G_TYPE_INT64);
	g_value_init (&gc_integer_node->increment, G_TYPE_INT64);

	g_value_set_int64 (&gc_integer_node->value, 0);
	g_value_set_int64 (&gc_integer_node->minimum, G_MININT64);
	g_value_set_int64 (&gc_integer_node->maximum, G_MAXINT64);
	g_value_set_int64 (&gc_integer_node->increment, 1);

	gc_integer_node->unit = NULL;
}

static void
arv_gc_integer_node_finalize (GObject *object)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (object);

	g_free (gc_integer_node->unit);

	g_value_unset (&gc_integer_node->value);
	g_value_unset (&gc_integer_node->minimum);
	g_value_unset (&gc_integer_node->maximum);
	g_value_unset (&gc_integer_node->increment);

	parent_class->finalize (object);
}

static void
arv_gc_integer_node_class_init (ArvGcIntegerNodeClass *integer_node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (integer_node_class);
	ArvGcNodeClass *node_class = ARV_GC_NODE_CLASS (integer_node_class);

	parent_class = g_type_class_peek_parent (integer_node_class);

	object_class->finalize = arv_gc_integer_node_finalize;

	node_class->get_node_name = arv_gc_integer_node_get_node_name;
	node_class->add_element = arv_gc_integer_node_add_element;
	node_class->get_value_type = arv_gc_integer_node_get_value_type;
	node_class->set_value_from_string = arv_gc_integer_node_set_value_from_string;
	node_class->get_value_as_string = arv_gc_integer_node_get_value_as_string;
}

/* ArvGcInteger interface implementation */

static gint64
arv_gc_integer_node_get_integer_value (ArvGcInteger *gc_integer)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_integer));
	return arv_gc_get_int64_from_value (genicam, &gc_integer_node->value);
}

static void
arv_gc_integer_node_set_integer_value (ArvGcInteger *gc_integer, gint64 value)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_integer));
	arv_gc_set_int64_to_value (genicam, &gc_integer_node->value, value);
}

static gint64
arv_gc_integer_node_get_min (ArvGcInteger *gc_integer)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_integer));
	return arv_gc_get_int64_from_value (genicam, &gc_integer_node->minimum);
}

static gint64
arv_gc_integer_node_get_max (ArvGcInteger *gc_integer)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_integer));
	return arv_gc_get_int64_from_value (genicam, &gc_integer_node->maximum);
}

static gint64
arv_gc_integer_node_get_inc (ArvGcInteger *gc_integer)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_integer));
	return arv_gc_get_int64_from_value (genicam, &gc_integer_node->increment);
}

static const char *
arv_gc_integer_node_get_unit (ArvGcInteger *gc_integer)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);

	return gc_integer_node->unit;
}

static void
arv_gc_integer_node_impose_min (ArvGcInteger *gc_integer, gint64 minimum)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_integer));
	arv_gc_set_int64_to_value (genicam, &gc_integer_node->minimum, minimum);
}

static void
arv_gc_integer_node_impose_max (ArvGcInteger *gc_integer, gint64 maximum)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_integer));
	arv_gc_set_int64_to_value (genicam, &gc_integer_node->minimum, maximum);
}

static void
arv_gc_integer_node_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_integer_node_get_integer_value;
	interface->set_value = arv_gc_integer_node_set_integer_value;
	interface->get_min = arv_gc_integer_node_get_min;
	interface->get_max = arv_gc_integer_node_get_max;
	interface->get_inc = arv_gc_integer_node_get_inc;
	interface->get_unit = arv_gc_integer_node_get_unit;
	interface->impose_min = arv_gc_integer_node_impose_min;
	interface->impose_max = arv_gc_integer_node_impose_max;
}

G_DEFINE_TYPE_WITH_CODE (ArvGcIntegerNode, arv_gc_integer_node, ARV_TYPE_GC_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_integer_node_integer_interface_init))
