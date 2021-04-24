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
 * Author:
 * 	Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION:arvdomnode
 * @short_description: Base class for DOM nodes
 */

#include <arvdomnode.h>
#include <arvdomnodechildlist.h>
#include <arvdomnodelist.h>
#include <arvdomdocument.h>
#include <arvdebugprivate.h>
#include <glib/gprintf.h>
#include <stdio.h>

typedef struct {
	ArvDomNode	*next_sibling;
	ArvDomNode	*previous_sibling;
	ArvDomNode	*parent_node;
	ArvDomNode	*first_child;
	ArvDomNode	*last_child;
} ArvDomNodePrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ArvDomNode, arv_dom_node, G_TYPE_OBJECT)

/* ArvDomNode implementation */

/**
 * arv_dom_node_get_node_name:
 * @self: a #ArvDomNode
 *
 * Gets the node name.
 *
 * Return value: the node name.
 */

const char*
arv_dom_node_get_node_name (ArvDomNode* self)
{
	ArvDomNodeClass *node_class = ARV_DOM_NODE_GET_CLASS (self);

	g_return_val_if_fail (node_class != NULL, NULL);

	if (node_class->get_node_name)
		return node_class->get_node_name (self);

	return NULL;
}

/**
 * arv_dom_node_get_node_value:
 * @self: a #ArvDomNode
 *
 * Gets the node value.
 *
 * Return value: the node value.
 */

const char*
arv_dom_node_get_node_value (ArvDomNode* self)
{
	ArvDomNodeClass *node_class = ARV_DOM_NODE_GET_CLASS (self);

	g_return_val_if_fail (node_class != NULL, NULL);

	if (node_class->get_node_value)
		return node_class->get_node_value (self);

	return NULL;
}

void
arv_dom_node_set_node_value (ArvDomNode* self, const char* new_value)
{
	ArvDomNodeClass *node_class = ARV_DOM_NODE_GET_CLASS (self);

	g_return_if_fail (node_class != NULL);
	g_return_if_fail (new_value != NULL);

	if (node_class->set_node_value)
		node_class->set_node_value (self, new_value);
}

ArvDomNodeType arv_dom_node_get_node_type (ArvDomNode* self)
{
	ArvDomNodeClass *node_class = ARV_DOM_NODE_GET_CLASS (self);

	g_return_val_if_fail (node_class != NULL, 0);

	if (node_class->get_node_type)
		return node_class->get_node_type (self);

	return 0;
}

/**
 * arv_dom_node_get_parent_node:
 * @self: a #ArvDomNode
 *
 * Get the parent node of @self.
 *
 * Returns: (transfer none): @self parent.
 */

ArvDomNode*
arv_dom_node_get_parent_node (ArvDomNode* self)
{
	ArvDomNodePrivate *priv = arv_dom_node_get_instance_private (self);

	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	return priv->parent_node;
}

/**
 * arv_dom_node_get_child_nodes:
 * @self: a #ArvDomNode
 *
 * Returns: (transfer none): a #ArvDomNodeList, NULL on error.
 */

ArvDomNodeList*
arv_dom_node_get_child_nodes (ArvDomNode* self)
{
	ArvDomNodeList *list;

	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	list = g_object_get_data (G_OBJECT (self), "child-nodes");

	if (list == NULL) {
		list = arv_dom_node_child_list_new (self);
		g_object_set_data_full (G_OBJECT (self), "child-nodes", list, g_object_unref);
	}

	return list;
}

/**
 * arv_dom_node_get_first_child:
 * @self: a #ArvDomNode
 *
 * Returns: (transfer none): @self first child.
 */

ArvDomNode*
arv_dom_node_get_first_child (ArvDomNode* self)
{
	ArvDomNodePrivate *priv = arv_dom_node_get_instance_private (self);

	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	return priv->first_child;
}

/**
 * arv_dom_node_get_last_child:
 * @self: a #ArvDomNode
 *
 * Returns: (transfer none): @self last child.
 */

ArvDomNode*
arv_dom_node_get_last_child (ArvDomNode* self)
{
	ArvDomNodePrivate *priv = arv_dom_node_get_instance_private (self);

	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	return priv->last_child;
}

/**
 * arv_dom_node_get_previous_sibling:
 * @self: a #ArvDomNode
 *
 * Returns: (transfer none): @self previous sibling.
 */

ArvDomNode*
arv_dom_node_get_previous_sibling (ArvDomNode* self)
{
	ArvDomNodePrivate *priv = arv_dom_node_get_instance_private (self);

	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	return priv->previous_sibling;
}

/**
 * arv_dom_node_get_next_sibling:
 * @self: a #ArvDomNode
 *
 * Returns: (transfer none): @self next sibling.
 */

ArvDomNode*
arv_dom_node_get_next_sibling (ArvDomNode* self)
{
	ArvDomNodePrivate *priv = arv_dom_node_get_instance_private (self);

	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	return priv->next_sibling;
}

/*ArvDomNamedNodeMap**/
/*arv_dom_node_get_attributes (ArvDomNode* self)*/
/*{*/
/*        return ARV_DOM_NODE_GET_CLASS (self)->get_attributes (self);*/
/*}*/


/**
 * arv_dom_node_get_owner_document:
 * @self: a #ArvDomNode
 *
 * Returns: (transfer none): @self owner document.
 */

ArvDomDocument*
arv_dom_node_get_owner_document (ArvDomNode* self)
{
	ArvDomNode *parent;

	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	for (parent = self;
	     parent != NULL &&
	     !ARV_IS_DOM_DOCUMENT (parent);
	     parent = arv_dom_node_get_parent_node (parent));

	return ARV_DOM_DOCUMENT (parent);
}

/**
 * arv_dom_node_insert_before:
 * @self: a #ArvDomNode
 * @new_child: (transfer full): node to insert
 * @ref_child: (transfer none): reference node, i.e., the node before which the new node must be inserted.
 *
 * Inserts the node @new_child before the existing child node @ref_child. If
 * @ref_child is null, insert @new_child at the end of the list of children.
 * If the @new_child is already in the tree, it is first removed.
 *
 * Returns: (transfer none): the inserted node.
 */

/* TODO:
 * If @new_child is a #LsmDocumentFragment object, all of its children are inserted,
 * in the same order, before @ref_child.
 * Check if new_child is an ancestor of self.
 */

ArvDomNode*
arv_dom_node_insert_before (ArvDomNode* self, ArvDomNode* new_child, ArvDomNode* ref_child)
{
	ArvDomNodePrivate *priv = arv_dom_node_get_instance_private (self);
	ArvDomNodePrivate *new_child_priv = arv_dom_node_get_instance_private (new_child);
	ArvDomNodePrivate *ref_child_priv = arv_dom_node_get_instance_private (ref_child);
	ArvDomNodeClass *node_class;

	if (ref_child == NULL)
		arv_dom_node_append_child (self, new_child);

	g_return_val_if_fail (ARV_IS_DOM_NODE (new_child), NULL);

	if (new_child_priv->parent_node != NULL)
		arv_dom_node_remove_child (self, new_child);

	if (!ARV_IS_DOM_NODE (self)) {
		g_critical ("%s: self is not a ArvDomNode", G_STRFUNC);
		g_object_unref (new_child);
		return NULL;
	}

	if (!ARV_IS_DOM_NODE (ref_child)) {
		g_critical ("%s: ref_child is not a ArvDomNode", G_STRFUNC);
		g_object_unref (new_child);
		return NULL;
	}

	if (ref_child_priv->parent_node != self) {
		arv_info_dom ("[ArvDomNode::insert_before] Ref child '%s' doesn't belong to '%s'",
			   arv_dom_node_get_node_name (ref_child),
			   arv_dom_node_get_node_name (self));
		g_object_unref (new_child);
		return NULL;
	}

	if (!ARV_DOM_NODE_GET_CLASS (self)->can_append_child (self, new_child)) {
		arv_debug_dom ("[ArvDomNode::insert_before] Can't append '%s' to '%s'",
			   arv_dom_node_get_node_name (new_child),
			   arv_dom_node_get_node_name (self));
		g_object_unref (new_child);
		return NULL;
	}

	new_child_priv->parent_node = self;
	new_child_priv->next_sibling = ref_child;
	new_child_priv->previous_sibling = ref_child_priv->previous_sibling;

	if (ref_child_priv->previous_sibling == NULL)
		priv->first_child = new_child;
	else {
		ArvDomNodePrivate *previous_sibling_priv = arv_dom_node_get_instance_private (ref_child_priv->previous_sibling);

		previous_sibling_priv->next_sibling = new_child;
	}

	ref_child_priv->previous_sibling = new_child;

	node_class = ARV_DOM_NODE_GET_CLASS (self);

	if (node_class->post_new_child)
		node_class->post_new_child (self, new_child);

	arv_dom_node_changed (self);

	return new_child;
}

/**
 * arv_dom_node_replace_child:
 * @self: a #ArvDomNode
 * @new_child: (transfer full): a replacement node
 * @old_child: (transfer none): node to replace
 *
 * Replaces the child node @old_child with @new_child in the list of children,
 * and returns the @old_child node.
 * If the @new_child is already in the tree, it is first removed.
 *
 * Returns: (transfer full): the replaced node.
 */

/* TODO:
 * Check if new_child is an ancestor of self.
 */

ArvDomNode*
arv_dom_node_replace_child (ArvDomNode* self, ArvDomNode* new_child, ArvDomNode* old_child)
{
	ArvDomNodePrivate *new_child_priv = arv_dom_node_get_instance_private (new_child);
	ArvDomNodePrivate *old_child_priv = arv_dom_node_get_instance_private (old_child);
	ArvDomNode *next_sibling;
	ArvDomNode *node;

	if (new_child == NULL)
		return arv_dom_node_remove_child (self, old_child);

	if (!ARV_IS_DOM_NODE (new_child)) {
		g_critical ("%s: new_child is not a ArvDomNode", G_STRFUNC);
		if (ARV_IS_DOM_NODE (old_child))
			g_object_unref (old_child);
		return NULL;
	}

	if (new_child_priv->parent_node != NULL)
		arv_dom_node_remove_child (self, new_child);

	if (old_child == NULL) {
		arv_info_dom ("[ArvDomNode::replace_child] old_child == NULL)");
		g_object_unref (new_child);
		return NULL;
	}

	if (!ARV_IS_DOM_NODE (old_child)) {
		g_critical ("%s: old_child is not a ArvDomNode", G_STRFUNC);
		g_object_unref (new_child);
		return NULL;
	}

	if (!ARV_IS_DOM_NODE (self)) {
		g_critical ("%s: self is not a ArvDomNode", G_STRFUNC);
		g_object_unref (new_child);
		g_object_unref (old_child);
		return NULL;
	}

	if (old_child_priv->parent_node != self) {
		g_object_unref (new_child);
		g_object_unref (old_child);
		return NULL;
	}

	next_sibling = old_child_priv->next_sibling;

	node = arv_dom_node_remove_child (self, old_child);
	if (node != old_child) {
		g_object_unref (new_child);
		g_object_unref (old_child);
		return NULL;
	}

	if (next_sibling == NULL)
		arv_dom_node_append_child (self, new_child);
	else
		arv_dom_node_insert_before (self, new_child, next_sibling);

	return old_child;
}

/**
 * arv_dom_node_remove_child:
 * @self: a #ArvDomNode
 * @old_child: (transfer none): node to remove.
 *
 * Removes the child node indicated by @old_child from the list of children, and returns it.
 *
 * Returns: (transfer full): the removed node.
 */

ArvDomNode*
arv_dom_node_remove_child (ArvDomNode* self, ArvDomNode* old_child)
{
	ArvDomNodePrivate *priv = arv_dom_node_get_instance_private (self);
	ArvDomNodePrivate *old_child_priv = arv_dom_node_get_instance_private (old_child);
	ArvDomNode *node;
	ArvDomNodeClass *node_class;

	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	if (old_child == NULL)
		return NULL;

	g_return_val_if_fail (ARV_IS_DOM_NODE (old_child), NULL);

	for (node = priv->first_child;
	     node != NULL && node != old_child;
	     node = arv_dom_node_get_next_sibling (node));

	if (node == NULL)
		return NULL;

	node_class = ARV_DOM_NODE_GET_CLASS (self);

	if (node_class->pre_remove_child)
		node_class->pre_remove_child (self, old_child);

	if (priv->first_child == old_child)
		priv->first_child = old_child_priv->next_sibling;
	if (priv->last_child == old_child)
		priv->last_child = old_child_priv->previous_sibling;

	if (old_child_priv->next_sibling != NULL) {
		ArvDomNodePrivate *next_sibling_priv = arv_dom_node_get_instance_private (old_child_priv->next_sibling);

		next_sibling_priv->previous_sibling = old_child_priv->previous_sibling;
	}
	if (old_child_priv->previous_sibling != NULL) {
		ArvDomNodePrivate *previous_sibling_priv = arv_dom_node_get_instance_private (old_child_priv->previous_sibling);

		previous_sibling_priv->next_sibling = old_child_priv->next_sibling;
	}

	old_child_priv->parent_node = NULL;
	old_child_priv->next_sibling = NULL;
	old_child_priv->previous_sibling = NULL;

	arv_dom_node_changed (self);

	return old_child;
}

/**
 * arv_dom_node_append_child:
 * @self: a #ArvDomNode
 * @new_child: (transfer full): node to append
 *
 * Adds the node @new_child to the end of the list of children of this node.
 * If the @new_child is already in the tree, it is first removed.
 *
 * Returns: (transfer none): the added node.
 */

ArvDomNode *
arv_dom_node_append_child (ArvDomNode* self, ArvDomNode* new_child)
{
	ArvDomNodePrivate *priv = arv_dom_node_get_instance_private (self);
	ArvDomNodePrivate *new_child_priv = arv_dom_node_get_instance_private (new_child);
	ArvDomNodeClass *node_class;

	if (new_child == NULL)
		return NULL;

	g_return_val_if_fail (ARV_IS_DOM_NODE (new_child), NULL);

	if (!ARV_IS_DOM_NODE (self)) {
		g_critical ("%s: self is not a ArvDomNode", G_STRFUNC);
		g_object_unref (new_child);
		return NULL;
	}

	if (new_child_priv->parent_node != NULL)
		arv_dom_node_remove_child (self, new_child);

	if (!ARV_DOM_NODE_GET_CLASS (self)->can_append_child (self, new_child)) {
		arv_debug_dom ("[ArvDomNode::append_child] Can't append '%s' to '%s'",
			       arv_dom_node_get_node_name (new_child),
			       arv_dom_node_get_node_name (self));
		g_object_unref (new_child);
		return NULL;
	}

	if (priv->first_child == NULL)
		priv->first_child = new_child;
	if (priv->last_child != NULL) {
		ArvDomNodePrivate *last_child_priv = arv_dom_node_get_instance_private (priv->last_child);

		last_child_priv->next_sibling = new_child;
	}

	new_child_priv->parent_node = self;
	new_child_priv->next_sibling = NULL;
	new_child_priv->previous_sibling = priv->last_child;
	priv->last_child = new_child;

	node_class = ARV_DOM_NODE_GET_CLASS (self);

	if (node_class->post_new_child)
		node_class->post_new_child (self, new_child);

	arv_dom_node_changed (self);

	return new_child;
}

static gboolean
arv_dom_node_can_append_child_default (ArvDomNode *self, ArvDomNode* new_child)
{
	return FALSE;
}

void
arv_dom_node_changed (ArvDomNode *self)
{
	ArvDomNodePrivate *priv = arv_dom_node_get_instance_private (self);
	ArvDomNode *parent_node;
	ArvDomNode *child_node;
	ArvDomNodeClass *node_class;

	g_return_if_fail (ARV_IS_DOM_NODE (self));

	node_class = ARV_DOM_NODE_GET_CLASS (self);

	if (node_class->changed)
		node_class->changed (self);

	child_node = self;
	for (parent_node = priv->parent_node;
	       parent_node != NULL;
	       parent_node = arv_dom_node_get_parent_node (parent_node)) {
		node_class = ARV_DOM_NODE_GET_CLASS (parent_node);
		if (node_class->child_changed == NULL ||
		    !node_class->child_changed (parent_node, child_node))
			break;
		child_node = parent_node;
	}
}

gboolean
arv_dom_node_has_child_nodes (ArvDomNode* self)
{
	ArvDomNodePrivate *priv = arv_dom_node_get_instance_private (self);

	g_return_val_if_fail (ARV_IS_DOM_NODE (self), FALSE);

	return priv->first_child != NULL;
}

static void
arv_dom_node_init (ArvDomNode *node)
{
}

static void
arv_dom_node_finalize (GObject *object)
{
	ArvDomNode *node = ARV_DOM_NODE (object);
	ArvDomNodePrivate *priv = arv_dom_node_get_instance_private (node);
	ArvDomNode *child, *next_child;

	child = priv->first_child;
	while (child != NULL) {
		next_child = arv_dom_node_get_next_sibling (child);
		g_object_unref (child);
		child = next_child;
	}

	G_OBJECT_CLASS (arv_dom_node_parent_class)->finalize (object);
}

/* ArvDomNode class */

static void
arv_dom_node_class_init (ArvDomNodeClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	object_class->finalize = arv_dom_node_finalize;

	node_class->can_append_child = arv_dom_node_can_append_child_default;
}
