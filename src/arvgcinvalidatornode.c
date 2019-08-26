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
 * SECTION: arvgcinvalidatornode
 * @short_description: Class for Invalidator nodes
 */

#include <arvgcinvalidatornode.h>
#include <arvgcpropertynode.h>
#include <arvgcfeaturenode.h>
#include <arvgc.h>
#include <arvdomtext.h>
#include <arvmisc.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_gc_invalidator_node_get_node_name (ArvDomNode *node)
{
	return "pInvalidator";
}

/* ArvGcInvalidatorNode implementation */

gboolean
arv_gc_invalidator_has_changed (ArvGcInvalidatorNode *self)
{
	ArvGcNode *node;
	guint64 change_count;

	g_return_val_if_fail (ARV_IS_GC_INVALIDATOR_NODE (self), FALSE);

	node = arv_gc_property_node_get_linked_node (ARV_GC_PROPERTY_NODE (self));
	change_count = arv_gc_feature_node_get_change_count (ARV_GC_FEATURE_NODE (node));

	if (change_count != self->change_index) {
		self->change_index = change_count;

		return TRUE;
	}

	return FALSE;
}

ArvGcNode *
arv_gc_invalidator_node_new (void)
{
	ArvGcPropertyNode *node;

	node = g_object_new (ARV_TYPE_GC_INVALIDATOR_NODE, NULL);
	node->type = ARV_GC_PROPERTY_NODE_TYPE_P_INVALIDATOR;

	return ARV_GC_NODE (node);
}

static void
arv_gc_invalidator_node_init (ArvGcInvalidatorNode *invalidator_node)
{
}

static void
arv_gc_invalidator_node_class_init (ArvGcInvalidatorNodeClass *this_class)
{
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	dom_node_class->get_node_name = arv_gc_invalidator_node_get_node_name;
}

G_DEFINE_TYPE (ArvGcInvalidatorNode, arv_gc_invalidator_node, ARV_TYPE_GC_PROPERTY_NODE)
