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

#include <arvgcintegernode.h>
#include <arvgcinteger.h>
#include <arvtools.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvGcNode implementation */

static void
arv_gc_integer_node_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (node);

	if (strcmp (name, "Min") == 0) {
		arv_force_g_value_to_int64 (&gc_integer_node->minimum,
					    g_ascii_strtoll (content, NULL, 0));
	} else if (strcmp (name, "Max") == 0) {
		arv_force_g_value_to_int64 (&gc_integer_node->maximum,
					    g_ascii_strtoll (content, NULL, 0));
	} else if (strcmp (name, "Inc") == 0) {
		arv_force_g_value_to_int64 (&gc_integer_node->increment,
					    g_ascii_strtoll (content, NULL, 0));
	} else if (strcmp (name, "pMin") == 0) {
		arv_force_g_value_to_string (&gc_integer_node->minimum, content);
	} else if (strcmp (name, "pMax") == 0) {
		arv_force_g_value_to_string (&gc_integer_node->maximum, content);
	} else if (strcmp (name, "pInc") == 0) {
		arv_force_g_value_to_string (&gc_integer_node->increment, content);
	} else
		ARV_GC_NODE_CLASS (parent_class)->add_element (node, name, content, attributes);
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
	g_value_init (&gc_integer_node->minimum, G_TYPE_INT64);
	g_value_init (&gc_integer_node->maximum, G_TYPE_INT64);
	g_value_init (&gc_integer_node->increment, G_TYPE_INT64);

	g_value_set_int64 (&gc_integer_node->minimum, G_MININT64);
	g_value_set_int64 (&gc_integer_node->maximum, G_MAXINT64);
	g_value_set_int64 (&gc_integer_node->increment, 1);
}

static void
arv_gc_integer_node_finalize (GObject *object)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (object);

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

	node_class->add_element = arv_gc_integer_node_add_element;
}

/* ArvGcInteger interface implementation */

static guint64
arv_gc_integer_node_get_integer_value (ArvGcInteger *gc_integer)
{
	return 0;
}

static void
arv_gc_integer_node_set_integer_value (ArvGcInteger *gc_integer, guint64 value)
{
}


static void
arv_gc_integer_node_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_integer_node_get_integer_value;
	interface->set_value = arv_gc_integer_node_set_integer_value;
}

G_DEFINE_TYPE_WITH_CODE (ArvGcIntegerNode, arv_gc_integer_node, ARV_TYPE_GC_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_integer_node_integer_interface_init))
