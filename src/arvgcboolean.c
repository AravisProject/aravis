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
 * SECTION: arvgcboolean
 * @short_description: Class for Boolean nodes
 */

#include <arvgcboolean.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvGcNode implementation */

static const char *
arv_gc_boolean_get_node_name (ArvGcNode *node)
{
	return "Boolean";
}

static void
arv_gc_boolean_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	ArvGcBoolean *gc_boolean = ARV_GC_BOOLEAN (node);

	if (strcmp (name, "Value") == 0) {
		arv_force_g_value_to_int64 (&gc_boolean->value,
					    g_ascii_strtoll (content, NULL, 0));
	} else if (strcmp (name, "pValue") == 0) {
		arv_force_g_value_to_string (&gc_boolean->value, content);
	} else if (strcmp (name, "OnValue") == 0) {
		gc_boolean->on_value = g_ascii_strtoll (content, NULL, 0);
	} else if (strcmp (name, "OffValue") == 0) {
		gc_boolean->off_value = g_ascii_strtoll (content, NULL, 0);
	} else
		ARV_GC_NODE_CLASS (parent_class)->add_element (node, name, content, attributes);
}

static void
arv_gc_boolean_set_value_from_string (ArvGcNode *node, const char *string)
{
	arv_gc_boolean_set_value (ARV_GC_BOOLEAN (node), g_strcmp0 (string, "true") == 0);
}

static const char *
arv_gc_boolean_get_value_as_string (ArvGcNode *node)
{
	return arv_gc_boolean_get_value (ARV_GC_BOOLEAN (node)) ? "true" : "false";
}

/* ArvGcBoolean implementation */

gboolean
arv_gc_boolean_get_value (ArvGcBoolean *gc_boolean)
{
	g_return_val_if_fail (ARV_IS_GC_BOOLEAN (gc_boolean), FALSE);

	return arv_gc_get_int64_from_value (arv_gc_node_get_genicam (ARV_GC_NODE (gc_boolean)),
					    &gc_boolean->value) == gc_boolean->on_value;
}

void
arv_gc_boolean_set_value (ArvGcBoolean *gc_boolean, gboolean v_boolean)
{
	g_return_if_fail (ARV_IS_GC_BOOLEAN (gc_boolean));

	arv_gc_set_int64_to_value (arv_gc_node_get_genicam (ARV_GC_NODE (gc_boolean)),
				   &gc_boolean->value,
				   v_boolean ? gc_boolean->on_value : gc_boolean->off_value);
}

ArvGcNode *
arv_gc_boolean_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_BOOLEAN, NULL);

	return node;
}

static void
arv_gc_boolean_init (ArvGcBoolean *gc_boolean)
{
	g_value_init (&gc_boolean->value, G_TYPE_INT64);
	g_value_set_int64 (&gc_boolean->value, 0);
	gc_boolean->on_value = 1;
	gc_boolean->off_value = 0;
}

static void
arv_gc_boolean_finalize (GObject *object)
{
	ArvGcBoolean *gc_boolean = ARV_GC_BOOLEAN (object);

	g_value_unset (&gc_boolean->value);

	parent_class->finalize (object);
}

static void
arv_gc_boolean_class_init (ArvGcBooleanClass *boolean_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (boolean_class);
	ArvGcNodeClass *node_class = ARV_GC_NODE_CLASS (boolean_class);

	parent_class = g_type_class_peek_parent (boolean_class);

	object_class->finalize = arv_gc_boolean_finalize;

	node_class->get_node_name = arv_gc_boolean_get_node_name;
	node_class->add_element = arv_gc_boolean_add_element;
	node_class->set_value_from_string = arv_gc_boolean_set_value_from_string;
	node_class->get_value_as_string = arv_gc_boolean_get_value_as_string;
}

/* ArvGcInteger interface implementation */

G_DEFINE_TYPE (ArvGcBoolean, arv_gc_boolean, ARV_TYPE_GC_NODE)
