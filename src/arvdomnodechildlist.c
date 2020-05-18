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

#include <arvdomnodechildlist.h>
#include <arvdomnode.h>
#include <arvdomnodelist.h>

/**
 * SECTION:arvdomnodechildlist
 * @short_description: Class for DOM node child lists
 */

/* ArvDomNodeChildList */

struct  _ArvDomNodeChildList {
	ArvDomNodeList base;

	ArvDomNode *parent_node;
};

struct _ArvDomNodeChildListClass {
	ArvDomNodeListClass parent_class;
};

G_DEFINE_TYPE (ArvDomNodeChildList, arv_dom_node_child_list, ARV_TYPE_DOM_NODE_LIST)

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

	for (iter = arv_dom_node_get_first_child (arv_dom_node_get_parent_node (ARV_DOM_NODE (child_list)));
	     iter != NULL; iter = arv_dom_node_get_next_sibling (iter)) {
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

	for (iter = arv_dom_node_get_first_child (arv_dom_node_get_parent_node (ARV_DOM_NODE (child_list)));
	     iter != NULL;
	     iter = arv_dom_node_get_next_sibling (iter))
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

	G_OBJECT_CLASS (arv_dom_node_child_list_parent_class)->finalize (object);
}

static void
arv_dom_node_child_list_class_init (ArvDomNodeChildListClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	ArvDomNodeListClass *node_list_class = ARV_DOM_NODE_LIST_CLASS (klass);

	object_class->finalize = arv_dom_node_child_list_finalize;

	node_list_class->get_item = arv_dom_node_child_list_get_item;
	node_list_class->get_length = arv_dom_node_child_list_get_length;
}

