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
 * SECTION: arvgcnode
 * @short_description: Base class for all Genicam nodes
 *
 * #ArvGcNode provides a base class for the implementation of the different
 * types of Genicam node.
 */

#include <arvgcnode.h>
#include <arvgc.h>
#include <arvdebug.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

struct _ArvGcNodePrivate {
	ArvGc *genicam;
	char *name;
	ArvGcNameSpace name_space;
	char *tooltip;
	char *description;
	char *display_name;

	unsigned int n_childs;
	GSList *childs;

	gint modification_count;
};

const char *
arv_gc_node_get_name (ArvGcNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_NODE (node), NULL);

	return node->priv->name;
}

const char *
arv_gc_node_get_tooltip (ArvGcNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_NODE (node), NULL);

	return node->priv->tooltip;
}

const char *
arv_gc_node_get_description (ArvGcNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_NODE (node), NULL);

	return node->priv->description;
}

const char *
arv_gc_node_get_display_name (ArvGcNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_NODE (node), NULL);

	return node->priv->display_name;
}

static void
_set_attribute (ArvGcNode *node, const char *name, const char *value)
{
	if (strcmp (name, "Name") == 0) {
		g_free (node->priv->name);
		node->priv->name = g_strdup (value);
	} if (strcmp (name, "NameSpace") == 0) {
		if (g_strcmp0 (value, "Standard") == 0)
			node->priv->name_space = ARV_GC_NAME_SPACE_STANDARD;
		else
			node->priv->name_space = ARV_GC_NAME_SPACE_CUSTOM;
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
		g_free (node->priv->tooltip);
		node->priv->tooltip = g_strdup (content);
	} else if (strcmp (name, "Description") == 0) {
		g_free (node->priv->description);
		node->priv->description = g_strdup (content);
	} else if (strcmp (name, "DisplayName") == 0) {
		g_free (node->priv->display_name);
		node->priv->display_name = g_strdup (content);
	}
}

void
arv_gc_node_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	g_return_if_fail (ARV_IS_GC_NODE (node));
	g_return_if_fail (name != NULL);

	arv_log_genicam ("[GcNode::add_element] Add %s [%s]",
			 name, content);

	ARV_GC_NODE_GET_CLASS (node)->add_element (node, name, content, attributes);
}

gboolean
arv_gc_node_can_add_child (ArvGcNode *node, ArvGcNode *child)
{
	ArvGcNodeClass *node_class;

	g_return_val_if_fail (ARV_IS_GC_NODE (node), FALSE);
	g_return_val_if_fail (ARV_IS_GC_NODE (child), FALSE);

	node_class = ARV_GC_NODE_GET_CLASS (node);
	if (node_class->can_add_child == NULL)
		return FALSE;

	return node_class->can_add_child (node, child);
}

void
arv_gc_node_add_child (ArvGcNode *node, ArvGcNode *child)
{
	g_return_if_fail (ARV_IS_GC_NODE (node));
	g_return_if_fail (ARV_IS_GC_NODE (child));

	node->priv->childs = g_slist_append (node->priv->childs, child);
	node->priv->n_childs++;
}

const GSList *
arv_gc_node_get_childs (ArvGcNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_NODE (node), NULL);

	return node->priv->childs;
}

unsigned int
arv_gc_node_get_n_childs (ArvGcNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_NODE (node), 0);

	return node->priv->n_childs;
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

	node->priv->genicam = genicam;
}

/**
 * arv_gc_node_get_genicam:
 * @gc_node: a #ArvGcNode
 * Return value: (transfer none): the parent #ArvGc
 *
 * Retrieves the parent genicam document of @node.
 */

ArvGc *
arv_gc_node_get_genicam	(ArvGcNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_NODE (node), NULL);

	return node->priv->genicam;
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

void
arv_gc_node_inc_modification_count (ArvGcNode *gc_node)
{
	g_return_if_fail (ARV_IS_GC_NODE (gc_node));

	gc_node->priv->modification_count++;
}

gint
arv_gc_node_get_modification_count (ArvGcNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_NODE (node), 0);

	return node->priv->modification_count;
}

static void
arv_gc_node_init (ArvGcNode *gc_node)
{
	gc_node->priv = G_TYPE_INSTANCE_GET_PRIVATE (gc_node, ARV_TYPE_GC_NODE, ArvGcNodePrivate);

	gc_node->priv->name = NULL;
	gc_node->priv->tooltip = NULL;
	gc_node->priv->description = NULL;
	gc_node->priv->display_name = NULL;
	gc_node->priv->childs = NULL;
	gc_node->priv->n_childs = 0;

	gc_node->priv->modification_count = 0;
}

static void
arv_gc_node_finalize (GObject *object)
{
	ArvGcNode *node = ARV_GC_NODE (object);
	GSList *iter;

	for (iter = node->priv->childs; iter != NULL; iter = iter->next)
		g_object_unref (iter->data);
	g_slist_free (node->priv->childs);
	node->priv->n_childs = 0;

	g_free (node->priv->name);
	g_free (node->priv->tooltip);
	g_free (node->priv->description);
	g_free (node->priv->display_name);

	parent_class->finalize (object);
}

static void
arv_gc_node_class_init (ArvGcNodeClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	g_type_class_add_private (node_class, sizeof (ArvGcNodePrivate));

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_gc_node_finalize;

	node_class->set_attribute = _set_attribute;
	node_class->add_element = _add_element;
	node_class->get_value_type = NULL;
}

G_DEFINE_TYPE (ArvGcNode, arv_gc_node, G_TYPE_OBJECT)
