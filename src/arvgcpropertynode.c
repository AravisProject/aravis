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
 * SECTION: arvgcfeaturenode
 * @short_description: Base class for all Genicam nodes
 *
 * #ArvGcPropertyNode provides a base class for the implementation of the different
 * types of Genicam node.
 */

#include <arvgcpropertynode.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <arvdebug.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_gc_property_node_get_node_name (ArvDomNode *node)
{
	ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (node);

	switch (property_node->type) {
		case ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION:
			return "Description";
		case ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP:
			return "Tooltip";
		case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME:
			return "DisplayName";
		case ARV_GC_PROPERTY_NODE_TYPE_P_IS_IMPLEMENTED:
			return "pIsImplemented";
		case ARV_GC_PROPERTY_NODE_TYPE_P_IS_AVAILABLE:
			return "pIsAvailable";
		default:
			return "Unknown";
	}
}

/* ArvGcPropertyNode implementation */

const char *
arv_gc_property_node_get_content (ArvGcPropertyNode *node)
{
	g_assert_not_reached ();
	return NULL;
}

static ArvGcPropertyNode *
arv_gc_property_node_new (ArvGcPropertyNodeType type)
{
	ArvGcPropertyNode *node;

	node = g_object_new (ARV_TYPE_GC_PROPERTY_NODE, NULL);
	node->type = type;

	return node;
}

ArvGcPropertyNode *
arv_gc_property_node_new_description (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION);
}

ArvGcPropertyNode *
arv_gc_property_node_new_tooltip (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP);
}

ArvGcPropertyNode *
arv_gc_property_node_new_display_name (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME);
}

ArvGcPropertyNode *
arv_gc_property_node_new_p_is_implemented (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_IS_IMPLEMENTED);
}

ArvGcPropertyNode *
arv_gc_property_node_new_p_is_available (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_IS_AVAILABLE);
}

static void
arv_gc_property_node_init (ArvGcPropertyNode *gc_property_node)
{
	gc_property_node->type = 0;
}

static void
arv_gc_property_node_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_gc_property_node_class_init (ArvGcPropertyNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_property_node_finalize;
	dom_node_class->get_node_name = arv_gc_property_node_get_node_name;
}

G_DEFINE_TYPE (ArvGcPropertyNode, arv_gc_property_node, ARV_TYPE_DOM_ELEMENT)
