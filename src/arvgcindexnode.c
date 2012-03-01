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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvgcindex_node
 * @short_description: Class for IndexNode elements
 */

#include <arvgcindexnode.h>
#include <arvgcpropertynode.h>
#include <arvgc.h>
#include <arvdomtext.h>
#include <arvmisc.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_gc_index_node_get_node_name (ArvDomNode *node)
{
	return "pIndex";
}

static gboolean
arv_gc_index_node_can_append_child (ArvDomNode *parent, ArvDomNode *child)
{
	return ARV_IS_DOM_TEXT (child);
}

/* ArvDomElement implementation */

static void
arv_gc_index_node_set_attribute (ArvDomElement *self, const char *name, const char *value)
{
	ArvGcIndexNode *index_node = ARV_GC_INDEX_NODE (self);

	if (strcmp (name, "Offset") == 0)
		arv_force_g_value_to_int64 (&index_node->offset, g_ascii_strtoll (value, NULL, 0));
	else if (strcmp (name, "pOffset") == 0) 
		arv_force_g_value_to_string (&index_node->offset, value);
	else
		ARV_DOM_ELEMENT_CLASS (parent_class)->set_attribute (self, name, value);
}

static const char *
arv_gc_index_node_get_attribute (ArvDomElement *self, const char *name)
{
	g_assert_not_reached ();

	return NULL;
}

/* ArvGcIndexNode implementation */

gint64
arv_gc_index_node_get_index (ArvGcIndexNode *index_node, gint64 default_offset)
{
	gint64 offset;

	g_return_val_if_fail (ARV_IS_GC_INDEX_NODE (index_node), 0);

	if (G_VALUE_HOLDS_BOOLEAN (&index_node->offset))
		offset = default_offset;
	else {
		ArvGc *genicam;

		genicam = arv_gc_node_get_genicam (ARV_GC_NODE (index_node));
	       	offset = arv_gc_get_int64_from_value (genicam, &index_node->offset);
	}

	return offset * arv_gc_property_node_get_int64 (ARV_GC_PROPERTY_NODE (index_node));
}

ArvGcNode *
arv_gc_index_node_new (void)
{
	ArvGcPropertyNode *node;

	node = g_object_new (ARV_TYPE_GC_INDEX_NODE, NULL);
	node->type = ARV_GC_PROPERTY_NODE_TYPE_P_INDEX;

	return ARV_GC_NODE (node);
}

static void
arv_gc_index_node_init (ArvGcIndexNode *index_node)
{
	g_value_init (&index_node->offset, G_TYPE_BOOLEAN);
	g_value_set_boolean (&index_node->offset, FALSE);
}

static void
arv_gc_index_node_finalize (GObject *object)
{
	ArvGcIndexNode *index_node = ARV_GC_INDEX_NODE (object);

	g_value_unset (&index_node->offset);

	parent_class->finalize (object);
}

static void
arv_gc_index_node_class_init (ArvGcIndexNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvDomElementClass *dom_element_class = ARV_DOM_ELEMENT_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_index_node_finalize;
	dom_node_class->get_node_name = arv_gc_index_node_get_node_name;
	dom_node_class->can_append_child = arv_gc_index_node_can_append_child;
	dom_element_class->set_attribute = arv_gc_index_node_set_attribute;
	dom_element_class->get_attribute = arv_gc_index_node_get_attribute;
}

G_DEFINE_TYPE (ArvGcIndexNode, arv_gc_index_node, ARV_TYPE_GC_PROPERTY_NODE)
