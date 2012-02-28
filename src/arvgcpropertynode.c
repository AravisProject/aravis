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
#include <arvdomtext.h>
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
		case ARV_GC_PROPERTY_NODE_TYPE_VALUE:
			return "Value";
		case ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION:
			return "Description";
		case ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP:
			return "Tooltip";
		case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME:
			return "DisplayName";
		case ARV_GC_PROPERTY_NODE_TYPE_MINIMUM:
			return "Min";
		case ARV_GC_PROPERTY_NODE_TYPE_MAXIMUM:
			return "Max";
		case ARV_GC_PROPERTY_NODE_TYPE_INCREMENT:
			return "Inc";
		case ARV_GC_PROPERTY_NODE_TYPE_ON_VALUE:
			return "OnValue";
		case ARV_GC_PROPERTY_NODE_TYPE_OFF_VALUE:
			return "OffValue";
		case ARV_GC_PROPERTY_NODE_TYPE_P_FEATURE:
			return "pFeature";
		case ARV_GC_PROPERTY_NODE_TYPE_P_VALUE:
			return "pValue";
		case ARV_GC_PROPERTY_NODE_TYPE_P_IS_IMPLEMENTED:
			return "pIsImplemented";
		case ARV_GC_PROPERTY_NODE_TYPE_P_IS_AVAILABLE:
			return "pIsAvailable";
		case ARV_GC_PROPERTY_NODE_TYPE_P_MINIMUM:
			return "pMin";
		case ARV_GC_PROPERTY_NODE_TYPE_P_MAXIMUM:
			return "pMax";
		case ARV_GC_PROPERTY_NODE_TYPE_P_INCREMENT:
			return "pInc";
		default:
			return "Unknown";
	}
}

/* ArvDomElement implementation */

static gboolean
arv_gc_property_node_can_append_child (ArvDomNode *self, ArvDomNode *child)
{
	return ARV_IS_DOM_TEXT (child);
}

/* ArvGcPropertyNode implementation */

const char *
arv_gc_property_node_get_string (ArvGcPropertyNode *node)
{
	ArvDomNode *child;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (node), NULL);

	child = arv_dom_node_get_first_child (ARV_DOM_NODE (node));
	if (child != NULL)
		return arv_dom_character_data_get_data (ARV_DOM_CHARACTER_DATA (child));

	return NULL;
}
	
void
arv_gc_property_node_set_string (ArvGcPropertyNode *node, const char *string)
{
	ArvDomNode *child;

	g_return_if_fail (ARV_IS_GC_PROPERTY_NODE (node));

	child = arv_dom_node_get_first_child (ARV_DOM_NODE (node));
	if (child != NULL)
		arv_dom_character_data_set_data (ARV_DOM_CHARACTER_DATA (child), string);
}

gint64
arv_gc_property_node_get_int64 (ArvGcPropertyNode *node)
{
	const char *string;

	string = arv_gc_property_node_get_string (node);

	if (string != NULL)
		return g_ascii_strtoll (string, NULL, 0);

	 return 0;
}

void
arv_gc_property_node_set_int64 (ArvGcPropertyNode *node, gint64 v_int64)
{
	ArvDomNode *child;

	g_return_if_fail (ARV_IS_GC_PROPERTY_NODE (node));

	child = arv_dom_node_get_first_child (ARV_DOM_NODE (node));
	if (child != NULL) {
		char *string = g_strdup_printf ("%" G_GINT64_FORMAT, v_int64);

		arv_dom_character_data_set_data (ARV_DOM_CHARACTER_DATA (child), string);
		g_free (string);
	}
}

ArvGcPropertyNodeType
arv_gc_property_node_get_node_type (ArvGcPropertyNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (node), ARV_GC_PROPERTY_NODE_TYPE_UNKNOWN);

	return node->type;
}

static ArvGcNode *
arv_gc_property_node_new (ArvGcPropertyNodeType type)
{
	ArvGcPropertyNode *node;

	node = g_object_new (ARV_TYPE_GC_PROPERTY_NODE, NULL);
	node->type = type;

	return ARV_GC_NODE (node);
}

ArvGcNode *
arv_gc_property_node_new_p_feature (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_FEATURE);
}

ArvGcNode *
arv_gc_property_node_new_value (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_VALUE);
}

ArvGcNode *
arv_gc_property_node_new_p_value (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_VALUE);
}

ArvGcNode *
arv_gc_property_node_new_description (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION);
}

ArvGcNode *
arv_gc_property_node_new_tooltip (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP);
}

ArvGcNode *
arv_gc_property_node_new_display_name (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME);
}

ArvGcNode *
arv_gc_property_node_new_p_is_implemented (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_IS_IMPLEMENTED);
}

ArvGcNode *
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
	dom_node_class->can_append_child = arv_gc_property_node_can_append_child;
}

G_DEFINE_TYPE (ArvGcPropertyNode, arv_gc_property_node, ARV_TYPE_GC_NODE)
