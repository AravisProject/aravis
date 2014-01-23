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

#include <arvdomnamednodemap.h>
#include <arvdomnode.h>

/* ArvDomNamedNodeMap implementation */


/**
 * arv_dom_named_node_map_get_named_item:
 * @map: a #ArvDomNamedNodeMap
 * @name: name of the element to look for.
 *
 * Returns: (transfer none): a #ArvDomElement.
 */

ArvDomNode *
arv_dom_named_node_map_get_named_item (ArvDomNamedNodeMap *map, const char *name)
{
	g_return_val_if_fail (ARV_IS_DOM_NAMED_NODE_MAP (map), NULL);

	return ARV_DOM_NAMED_NODE_MAP_GET_CLASS (map)->get (map, name);
}

/**
 * arv_dom_named_node_map_set_named_item:
 * @map: a #ArvDomNamedNodeMap
 * @item: a node to insert
 *
 * Returns: (transfer none): same as @node on success.
 */

ArvDomNode *
arv_dom_named_node_map_set_named_item (ArvDomNamedNodeMap *map, ArvDomNode *item)
{
	g_return_val_if_fail (ARV_IS_DOM_NAMED_NODE_MAP (map), NULL);

	return ARV_DOM_NAMED_NODE_MAP_GET_CLASS (map)->set (map, item);
}

/**
 * arv_dom_named_node_map_remove_named_item:
 * @map: a #ArvDomNamedNodeMap
 * @name: name of the node to remove
 *
 * Returns: (transfer none): the removed node.
 */

ArvDomNode *
arv_dom_named_node_map_remove_named_item (ArvDomNamedNodeMap *map, const char *name)
{
	g_return_val_if_fail (ARV_IS_DOM_NAMED_NODE_MAP (map), NULL);

	return ARV_DOM_NAMED_NODE_MAP_GET_CLASS (map)->remove (map, name);
}

/**
 * arv_dom_named_node_map_get_item:
 * @map: a #ArvDomNamedNodeMap
 * @index: an index
 *
 * Returns: (transfer none): the @ArvDomNode corresponding to @index.
 */

ArvDomNode *
arv_dom_named_node_map_get_item (ArvDomNamedNodeMap *map, unsigned int index)
{
	g_return_val_if_fail (ARV_IS_DOM_NAMED_NODE_MAP (map), NULL);

	return ARV_DOM_NAMED_NODE_MAP_GET_CLASS (map)->get_item (map, index);
}

unsigned int
arv_dom_named_node_map_get_length (ArvDomNamedNodeMap *map)
{
	g_return_val_if_fail (ARV_IS_DOM_NAMED_NODE_MAP (map), 0);

	return ARV_DOM_NAMED_NODE_MAP_GET_CLASS (map)->get_length (map);
}

static void
arv_dom_named_node_map_init (ArvDomNamedNodeMap *map)
{
}

/* ArvDomNamedNodeMap class */

static void
arv_dom_named_node_map_class_init (ArvDomNamedNodeMapClass *klass)
{
}

G_DEFINE_ABSTRACT_TYPE (ArvDomNamedNodeMap, arv_dom_named_node_map, G_TYPE_OBJECT)
