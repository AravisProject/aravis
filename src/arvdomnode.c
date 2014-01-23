/* Aravis
 *
 * Copyright © 2007-2010 Emmanuel Pacaud
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
#include <arvdomnodelist.h>
#include <arvdomdocument.h>
#include <arvdebug.h>
#include <glib/gprintf.h>
#include <stdio.h>

/* ArvDomNodeChildList */


#define ARV_TYPE_DOM_NODE_CHILD_LIST             (arv_dom_node_child_list_get_type ())
#define ARV_DOM_NODE_CHILD_LIST(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_DOM_NODE_CHILD_LIST, ArvDomNodeChildList))
#define ARV_DOM_NODE_CHILD_LIST_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_DOM_NODE_CHILD_LIST, ArvDomNodeChildListClass))
#define ARV_IS_DOM_NODE_CHILD_LIST(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_DOM_NODE_CHILD_LIST))
#define ARV_IS_DOM_NODE_CHILD_LIST_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_DOM_NODE_CHILD_LIST))
#define ARV_DOM_NODE_CHILD_LIST_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_DOM_NODE_CHILD_LIST, ArvDomNodeChildListClass))

typedef struct _ArvDomNodeChildListClass ArvDomNodeChildListClass;

typedef struct {
	ArvDomNodeList base;

	ArvDomNode *parent_node;
} ArvDomNodeChildList;

struct _ArvDomNodeChildListClass {
	ArvDomNodeListClass parent_class;
};

GType arv_dom_node_child_list_get_type (void);

static GObjectClass *child_list_parent_class = NULL;

static void
arv_dom_node_child_list_weak_notify_cb (void *user_data, GObject *object)
{
	ArvDomNodeChildList *list = user_data;

	list->parent_node = NULL;
}

static ArvDomNode *
arv_dom_node_child_list_get_item (ArvDomNodeList *list, unsigned int index)
{
	ArvDomNodeChildList *child_list = ARV_DOM_NODE_CHILD_LIST (list);
	ArvDomNode *iter;
	unsigned int i = 0;

	if (child_list->parent_node == NULL)
		return NULL;

	for (iter = child_list->parent_node->first_child; iter != NULL; iter = iter->next_sibling) {
		if (i == index)
			return iter;
		i++;
	}

	return NULL;
}

static unsigned int
arv_dom_node_child_list_get_length (ArvDomNodeList *list)
{
	ArvDomNodeChildList *child_list = ARV_DOM_NODE_CHILD_LIST (list);
	ArvDomNode *iter;
	unsigned int length = 0;

	if (child_list->parent_node == NULL)
		return 0;

	for (iter = child_list->parent_node->first_child; iter != NULL; iter = iter->next_sibling)
		length++;

	return length;
}

ArvDomNodeList *
arv_dom_node_child_list_new (ArvDomNode *parent_node)
{
	ArvDomNodeChildList *list;

	g_return_val_if_fail (ARV_IS_DOM_NODE (parent_node), NULL);

	list = g_object_new (ARV_TYPE_DOM_NODE_CHILD_LIST, NULL);
	list->parent_node = parent_node;

	g_object_weak_ref (G_OBJECT (parent_node), arv_dom_node_child_list_weak_notify_cb, list);

	return ARV_DOM_NODE_LIST (list);
}

static void
arv_dom_node_child_list_init (ArvDomNodeChildList *list)
{
}

static void
arv_dom_node_child_list_finalize (GObject *object)
{
	ArvDomNodeChildList *list = ARV_DOM_NODE_CHILD_LIST (object);

	if (list->parent_node != NULL) {
		g_object_weak_unref (G_OBJECT (list->parent_node), arv_dom_node_child_list_weak_notify_cb, list);
		list->parent_node = NULL;
	}

	child_list_parent_class->finalize (object);
}

static void
arv_dom_node_child_list_class_init (ArvDomNodeChildListClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	ArvDomNodeListClass *node_list_class = ARV_DOM_NODE_LIST_CLASS (klass);

	child_list_parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = arv_dom_node_child_list_finalize;

	node_list_class->get_item = arv_dom_node_child_list_get_item;
	node_list_class->get_length = arv_dom_node_child_list_get_length;
}

G_DEFINE_TYPE (ArvDomNodeChildList, arv_dom_node_child_list, ARV_TYPE_DOM_NODE_LIST)

static GObjectClass *parent_class = NULL;

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
	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	return self->parent_node;
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
	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	return self->first_child;
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
	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	return self->last_child;
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
	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	return self->previous_sibling;
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
	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	return self->next_sibling;
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
	     parent = parent->parent_node);

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
	ArvDomNodeClass *node_class;

	if (ref_child == NULL)
		arv_dom_node_append_child (self, new_child);

	g_return_val_if_fail (ARV_IS_DOM_NODE (new_child), NULL);

	if (new_child->parent_node != NULL)
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

	if (ref_child->parent_node != self) {
		arv_debug_dom ("[ArvDomNode::insert_before] Ref child '%s' doesn't belong to '%s'",
			   arv_dom_node_get_node_name (ref_child),
			   arv_dom_node_get_node_name (self));
		g_object_unref (new_child);
		return NULL;
	}

	if (!ARV_DOM_NODE_GET_CLASS (self)->can_append_child (self, new_child)) {
		arv_log_dom ("[ArvDomNode::insert_before] Can't append '%s' to '%s'",
			   arv_dom_node_get_node_name (new_child),
			   arv_dom_node_get_node_name (self));
		g_object_unref (new_child);
		return NULL;
	}

	new_child->parent_node = self;
	new_child->next_sibling = ref_child;
	new_child->previous_sibling = ref_child->previous_sibling;

	if (ref_child->previous_sibling == NULL)
		self->first_child = new_child;
	else
		ref_child->previous_sibling->next_sibling = new_child;

	ref_child->previous_sibling = new_child;

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

	if (new_child->parent_node != NULL)
		arv_dom_node_remove_child (self, new_child);

	if (old_child == NULL) {
		arv_debug_dom ("[ArvDomNode::replace_child] old_child == NULL)");
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

	if (old_child->parent_node != self) {
		g_object_unref (new_child);
		g_object_unref (old_child);
		return NULL;
	}

	next_sibling = old_child->next_sibling;

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
	ArvDomNode *node;
	ArvDomNodeClass *node_class;

	g_return_val_if_fail (ARV_IS_DOM_NODE (self), NULL);

	if (old_child == NULL)
		return NULL;

	g_return_val_if_fail (ARV_IS_DOM_NODE (old_child), NULL);

	for (node = self->first_child;
	     node != NULL && node != old_child;
	     node = node->next_sibling);

	if (node == NULL)
		return NULL;

	node_class = ARV_DOM_NODE_GET_CLASS (self);

	if (node_class->pre_remove_child)
		node_class->pre_remove_child (self, old_child);

	if (self->first_child == old_child)
		self->first_child = old_child->next_sibling;
	if (self->last_child == old_child)
		self->last_child = old_child->previous_sibling;

	if (old_child->next_sibling != NULL)
		old_child->next_sibling->previous_sibling = old_child->previous_sibling;
	if (old_child->previous_sibling != NULL)
		old_child->previous_sibling->next_sibling = old_child->next_sibling;

	old_child->parent_node = NULL;
	old_child->next_sibling = NULL;
	old_child->previous_sibling = NULL;

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
	ArvDomNodeClass *node_class;

	if (new_child == NULL)
		return NULL;

	g_return_val_if_fail (ARV_IS_DOM_NODE (new_child), NULL);

	if (!ARV_IS_DOM_NODE (self)) {
		g_critical ("%s: self is not a ArvDomNode", G_STRFUNC);
		g_object_unref (new_child);
		return NULL;
	}

	if (new_child->parent_node != NULL)
		arv_dom_node_remove_child (self, new_child);

	if (!ARV_DOM_NODE_GET_CLASS (self)->can_append_child (self, new_child)) {
		arv_log_dom ("[ArvDomNode::append_child] Can't append '%s' to '%s'",
			       arv_dom_node_get_node_name (new_child),
			       arv_dom_node_get_node_name (self));
		g_object_unref (new_child);
		return NULL;
	}

	if (self->first_child == NULL)
		self->first_child = new_child;
	if (self->last_child != NULL)
		self->last_child->next_sibling = new_child;

	new_child->parent_node = self;
	new_child->next_sibling = NULL;
	new_child->previous_sibling = self->last_child;
	self->last_child = new_child;

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
	ArvDomNode *parent_node;
	ArvDomNode *child_node;
	ArvDomNodeClass *node_class;

	g_return_if_fail (ARV_IS_DOM_NODE (self));

	node_class = ARV_DOM_NODE_GET_CLASS (self);

	if (node_class->changed)
		node_class->changed (self);

	child_node = self;
	for (parent_node = self->parent_node;
	       parent_node != NULL;
	       parent_node = parent_node->parent_node) {
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
	g_return_val_if_fail (ARV_IS_DOM_NODE (self), FALSE);

	return self->first_child != NULL;
}

static void
arv_dom_node_write_to_stream_default (ArvDomNode *self, GOutputStream *stream, GError **error)
{
	ArvDomNode *child;

	for (child = self->first_child; child != NULL; child = child->next_sibling)
		arv_dom_node_write_to_stream (child, stream, error);
}

void
arv_dom_node_write_to_stream (ArvDomNode *self, GOutputStream *stream, GError **error)
{
	ArvDomNodeClass *node_class;

	g_return_if_fail (ARV_IS_DOM_NODE (self));
	g_return_if_fail (G_IS_OUTPUT_STREAM (stream));

	node_class = ARV_DOM_NODE_GET_CLASS (self);
	if (node_class->write_to_stream != NULL)
		node_class->write_to_stream (self, stream, error);
}

static void
arv_dom_node_init (ArvDomNode *node)
{
}

static void
arv_dom_node_finalize (GObject *object)
{
	ArvDomNode *node = ARV_DOM_NODE (object);
	ArvDomNode *child, *next_child;

	child = node->first_child;
	while (child != NULL) {
		next_child = child->next_sibling;
		g_object_unref (child);
		child = next_child;
	}

	parent_class->finalize (object);
}

/* ArvDomNode class */

static void
arv_dom_node_class_init (ArvDomNodeClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_dom_node_finalize;

	node_class->can_append_child = arv_dom_node_can_append_child_default;
	node_class->write_to_stream = arv_dom_node_write_to_stream_default;
}

G_DEFINE_ABSTRACT_TYPE (ArvDomNode, arv_dom_node, G_TYPE_OBJECT)
