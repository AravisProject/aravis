/* Aravis - Digital camera library
 *
 * Copyright © 2009-2012 Emmanuel Pacaud
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
 * SECTION: arvgcvariablenode
 * @short_description: Class for VariableNode elements
 */

#include <arvgcvariablenode.h>
#include <arvgcpropertynode.h>
#include <arvgc.h>
#include <arvdomtext.h>
#include <arvmisc.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_gc_variable_node_get_node_name (ArvDomNode *node)
{
	return "pVariable";
}

static gboolean
arv_gc_variable_node_can_append_child (ArvDomNode *parent, ArvDomNode *child)
{
	return ARV_IS_DOM_TEXT (child);
}

/* ArvDomElement implementation */

static void
arv_gc_variable_node_set_attribute (ArvDomElement *self, const char *name, const char *value)
{
	ArvGcVariableNode *variable_node = ARV_GC_VARIABLE_NODE (self);

	if (strcmp (name, "Name") == 0) {
		g_free (variable_node->name);
		variable_node->name = g_strdup (value);
	}
}

static const char *
arv_gc_variable_node_get_attribute (ArvDomElement *self, const char *name)
{
	g_assert_not_reached ();

	return NULL;
}

/* ArvGcVariableNode implementation */

const char *
arv_gc_variable_node_get_name (ArvGcVariableNode *variable_node)
{
	g_return_val_if_fail (ARV_IS_GC_VARIABLE_NODE (variable_node), NULL);

	return variable_node->name;
}

ArvGcNode *
arv_gc_variable_node_new (void)
{
	ArvGcPropertyNode *node;

	node = g_object_new (ARV_TYPE_GC_VARIABLE_NODE, NULL);
	node->type = ARV_GC_PROPERTY_NODE_TYPE_P_VARIABLE;

	return ARV_GC_NODE (node);
}

static void
arv_gc_variable_node_init (ArvGcVariableNode *variable_node)
{
}

static void
arv_gc_variable_node_finalize (GObject *object)
{
	ArvGcVariableNode *variable_node = ARV_GC_VARIABLE_NODE (object);

	g_free (variable_node->name);

	parent_class->finalize (object);
}

static void
arv_gc_variable_node_class_init (ArvGcVariableNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvDomElementClass *dom_element_class = ARV_DOM_ELEMENT_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_variable_node_finalize;
	dom_node_class->get_node_name = arv_gc_variable_node_get_node_name;
	dom_node_class->can_append_child = arv_gc_variable_node_can_append_child;
	dom_element_class->set_attribute = arv_gc_variable_node_set_attribute;
	dom_element_class->get_attribute = arv_gc_variable_node_get_attribute;
}

G_DEFINE_TYPE (ArvGcVariableNode, arv_gc_variable_node, ARV_TYPE_GC_PROPERTY_NODE)
