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

#include <arvgcnode.h>
#include <arvgc.h>
#include <arvdebug.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

const char *
arv_gc_node_get_name (ArvGcNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_NODE (node), NULL);

	return node->name;
}

static void
_set_attribute (ArvGcNode *node, const char *name, const char *value)
{
	if (strcmp (name, "Name") == 0) {
		g_free (node->name);
		node->name = g_strdup (value);
	} if (strcmp (name, "NameSpace") == 0) {
		if (g_strcmp0 (value, "Standard") == 0)
			node->name_space = ARV_GC_NAME_SPACE_STANDARD;
		else
			node->name_space = ARV_GC_NAME_SPACE_CUSTOM;
	      }
}

void
arv_gc_node_set_attribute (ArvGcNode *node, const char *name, const char *value)
{
	g_return_if_fail (ARV_IS_GC_NODE (node));
	g_return_if_fail (name != NULL);

	ARV_GC_NODE_GET_CLASS (node)->set_attribute (node, name, value);
}

static void
_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	if (strcmp (name, "ToolTip") == 0) {
		g_free (node->tooltip);
		node->tooltip = g_strdup (content);
	}
}

void
arv_gc_node_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	g_return_if_fail (ARV_IS_GC_NODE (node));
	g_return_if_fail (name != NULL);

	arv_debug (ARV_DEBUG_LEVEL_STANDARD, "[GcNode::add_element] Add %s [%s]",
		   name, content);

	ARV_GC_NODE_GET_CLASS (node)->add_element (node, name, content, attributes);
}

ArvGcNode *
arv_gc_node_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_NODE, NULL);

	return node;
}

void
arv_gc_node_set_genicam	(ArvGcNode *node, ArvGc *genicam)
{
	g_return_if_fail (ARV_IS_GC_NODE (node));
	g_return_if_fail (genicam == NULL || ARV_IS_GC (genicam));

	node->genicam = genicam;
}

ArvGc *
arv_gc_node_get_genicam	(ArvGcNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_NODE (node), NULL);

	return node->genicam;
}

GType
arv_gc_node_get_value_type (ArvGcNode *node)
{
	ArvGcNodeClass *node_class;

	g_return_val_if_fail (ARV_IS_GC_NODE (node), 0);

	node_class = ARV_GC_NODE_GET_CLASS (node);
	if (node_class->get_value_type != NULL)
		return node_class->get_value_type (node);

	return 0;
}

static void
arv_gc_node_init (ArvGcNode *gc_node)
{
}

static void
arv_gc_node_finalize (GObject *object)
{
	ArvGcNode *node = ARV_GC_NODE (object);

	g_free (node->name);
	g_free (node->tooltip);

	parent_class->finalize (object);
}

static void
arv_gc_node_class_init (ArvGcNodeClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_gc_node_finalize;

	node_class->set_attribute = _set_attribute;
	node_class->add_element = _add_element;
	node_class->get_value_type = NULL;
}

G_DEFINE_TYPE (ArvGcNode, arv_gc_node, G_TYPE_OBJECT)
