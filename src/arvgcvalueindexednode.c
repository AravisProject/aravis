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
 * SECTION: arvgcvalueindexednode
 * @short_description: Class for Index nodes
 */

#include <arvgcvalueindexednode.h>
#include <arvgcpropertynode.h>
#include <arvgcinteger.h>
#include <arvgc.h>
#include <arvdomtext.h>
#include <arvmisc.h>
#include <string.h>

struct _ArvGcValueIndexedNode {
	ArvGcPropertyNode	base;

	char *index;
};

struct _ArvGcValueIndexedNodeClass {
	ArvGcPropertyNodeClass parent_class;
};

G_DEFINE_TYPE (ArvGcValueIndexedNode, arv_gc_value_indexed_node, ARV_TYPE_GC_PROPERTY_NODE)

/* ArvDomNode implementation */

static const char *
arv_gc_value_indexed_node_get_node_name (ArvDomNode *node)
{
	switch (arv_gc_property_node_get_node_type (ARV_GC_PROPERTY_NODE (node))) {
		case ARV_GC_PROPERTY_NODE_TYPE_VALUE_INDEXED:
			return "ValueIndexed";
		case ARV_GC_PROPERTY_NODE_TYPE_P_VALUE_INDEXED:
			return "pValueIndexed";
		default:
			g_assert_not_reached ();
	}
}

static gboolean
arv_gc_value_indexed_node_can_append_child (ArvDomNode *parent, ArvDomNode *child)
{
	return ARV_IS_DOM_TEXT (child);
}

/* ArvDomElement implementation */

static void
arv_gc_value_indexed_node_set_attribute (ArvDomElement *self, const char *name, const char *value)
{
	ArvGcValueIndexedNode *value_indexed_node = ARV_GC_VALUE_INDEXED_NODE (self);

	if (strcmp (name, "Index") == 0) {
		g_free (value_indexed_node->index);
		value_indexed_node->index = g_strdup (value);
	}
}

static const char *
arv_gc_value_indexed_node_get_attribute (ArvDomElement *self, const char *name)
{
	g_assert_not_reached ();

	return NULL;
}

/* ArvGcValueIndexedNode implementation */

gint64
arv_gc_value_indexed_node_get_index (ArvGcValueIndexedNode *value_indexed_node)
{
	gint64 index;

	g_return_val_if_fail (ARV_IS_GC_VALUE_INDEXED_NODE (value_indexed_node), 0);

	if (value_indexed_node->index == NULL)
		index = 0;
	else
		index = g_ascii_strtoll (value_indexed_node->index, NULL, 0);

	return index;
}

ArvGcNode *
arv_gc_value_indexed_node_new (void)
{
	return g_object_new (ARV_TYPE_GC_VALUE_INDEXED_NODE, "node-type", ARV_GC_PROPERTY_NODE_TYPE_VALUE_INDEXED, NULL);
}

ArvGcNode *
arv_gc_p_value_indexed_node_new (void)
{
	return g_object_new (ARV_TYPE_GC_VALUE_INDEXED_NODE, "node-type", ARV_GC_PROPERTY_NODE_TYPE_P_VALUE_INDEXED, NULL);
}

static void
arv_gc_value_indexed_node_init (ArvGcValueIndexedNode *value_indexed_node)
{
	value_indexed_node->index = NULL;
}

static void
arv_gc_value_indexed_node_finalize (GObject *object)
{
	ArvGcValueIndexedNode *value_indexed_node = ARV_GC_VALUE_INDEXED_NODE (object);

	g_clear_pointer (&value_indexed_node->index, g_free);

	G_OBJECT_CLASS (arv_gc_value_indexed_node_parent_class)->finalize (object);
}

static void
arv_gc_value_indexed_node_class_init (ArvGcValueIndexedNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvDomElementClass *dom_element_class = ARV_DOM_ELEMENT_CLASS (this_class);

	object_class->finalize = arv_gc_value_indexed_node_finalize;
	dom_node_class->get_node_name = arv_gc_value_indexed_node_get_node_name;
	dom_node_class->can_append_child = arv_gc_value_indexed_node_can_append_child;
	dom_element_class->set_attribute = arv_gc_value_indexed_node_set_attribute;
	dom_element_class->get_attribute = arv_gc_value_indexed_node_get_attribute;
}
