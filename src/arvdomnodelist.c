/* Aravis
 *
 * Copyright © 2010 Emmanuel Pacaud
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

#include <arvdomnodelist.h>
#include <arvdomnode.h>

/* ArvDomNodeList implementation */

/**
 * arv_dom_node_list_get_item:
 * @list: a #ArvDomNodeList
 * @index: item index
 *
 * Get one of the item of @list.
 *
 * Returns: (transfer none): item corresponding to index, NULL on error.
 */

ArvDomNode *
arv_dom_node_list_get_item (ArvDomNodeList *list, unsigned int index)
{
	g_return_val_if_fail (ARV_IS_DOM_NODE_LIST (list), NULL);

	return ARV_DOM_NODE_LIST_GET_CLASS (list)->get_item (list, index);
}

unsigned int
arv_dom_node_list_get_length (ArvDomNodeList *list)
{
	g_return_val_if_fail (ARV_IS_DOM_NODE_LIST (list), 0);

	return ARV_DOM_NODE_LIST_GET_CLASS (list)->get_length (list);
}

static void
arv_dom_node_list_init (ArvDomNodeList *list)
{
}

/* ArvDomNodeList class */

static void
arv_dom_node_list_class_init (ArvDomNodeListClass *klass)
{
}

G_DEFINE_ABSTRACT_TYPE (ArvDomNodeList, arv_dom_node_list, G_TYPE_OBJECT)
