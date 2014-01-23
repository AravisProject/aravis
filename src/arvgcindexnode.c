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
 * SECTION: arvgcindexnode
 * @short_description: Class for Index nodes
 */

#include <arvgcindexnode.h>
#include <arvgcpropertynode.h>
#include <arvgcinteger.h>
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

	if (strcmp (name, "Offset") == 0) {
		g_free (index_node->offset);
		index_node->offset = g_strdup (value);
		index_node->is_p_offset = FALSE;
	} else if (strcmp (name, "pOffset") == 0) {
		g_free (index_node->offset);
		index_node->offset = g_strdup (value);
		index_node->is_p_offset = TRUE;
	}
}

static const char *
arv_gc_index_node_get_attribute (ArvDomElement *self, const char *name)
{
	g_assert_not_reached ();

	return NULL;
}

/* ArvGcIndexNode implementation */

gint64
arv_gc_index_node_get_index (ArvGcIndexNode *index_node, gint64 default_offset, GError **error)
{
	gint64 offset;
	gint64 node_value;
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_INDEX_NODE (index_node), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	if (index_node->offset == NULL)
		offset = default_offset;
	else {
		if (index_node->is_p_offset) {
			ArvGcNode *node;
			ArvGc *genicam;

			genicam = arv_gc_node_get_genicam (ARV_GC_NODE (index_node));
			node = arv_gc_get_node (genicam, index_node->offset);
			offset = arv_gc_integer_get_value (ARV_GC_INTEGER (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);

				return 0;
			}
		} else
			offset = g_ascii_strtoll (index_node->offset, NULL, 0);
	}

	node_value = arv_gc_property_node_get_int64 (ARV_GC_PROPERTY_NODE (index_node), &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	return offset * node_value;
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
	index_node->offset = NULL;
	index_node->is_p_offset = FALSE;
}

static void
arv_gc_index_node_finalize (GObject *object)
{
	ArvGcIndexNode *index_node = ARV_GC_INDEX_NODE (object);

	g_free (index_node->offset);

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
