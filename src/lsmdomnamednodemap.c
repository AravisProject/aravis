/* Lasem
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author:
 * 	Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <lsmdomnamednodemap.h>
#include <lsmdomnode.h>

/* LsmDomNamedNodeMap implementation */

LsmDomNode *
lsm_dom_named_node_map_get_named_item (LsmDomNamedNodeMap *map, const char *name)
{
	g_return_val_if_fail (LSM_IS_DOM_NAMED_NODE_MAP (map), NULL);

	return LSM_DOM_NAMED_NODE_MAP_GET_CLASS (map)->get (map, name);
}

LsmDomNode *
lsm_dom_named_node_map_set_named_item (LsmDomNamedNodeMap *map, LsmDomNode *node)
{
	g_return_val_if_fail (LSM_IS_DOM_NAMED_NODE_MAP (map), NULL);

	return LSM_DOM_NAMED_NODE_MAP_GET_CLASS (map)->set (map, node);
}

LsmDomNode *
lsm_dom_named_node_map_remove_named_item (LsmDomNamedNodeMap *map, const char *name)
{
	g_return_val_if_fail (LSM_IS_DOM_NAMED_NODE_MAP (map), NULL);

	return LSM_DOM_NAMED_NODE_MAP_GET_CLASS (map)->remove (map, name);
}

LsmDomNode *
lsm_dom_named_node_map_get_item (LsmDomNamedNodeMap *map, unsigned int index)
{
	g_return_val_if_fail (LSM_IS_DOM_NAMED_NODE_MAP (map), NULL);

	return LSM_DOM_NAMED_NODE_MAP_GET_CLASS (map)->get_item (map, index);
}

unsigned int
lsm_dom_named_node_map_get_length (LsmDomNamedNodeMap *map)
{
	g_return_val_if_fail (LSM_IS_DOM_NAMED_NODE_MAP (map), 0);

	return LSM_DOM_NAMED_NODE_MAP_GET_CLASS (map)->get_length (map);
}

static void
lsm_dom_named_node_map_init (LsmDomNamedNodeMap *map)
{
}

/* LsmDomNamedNodeMap class */

static void
lsm_dom_named_node_map_class_init (LsmDomNamedNodeMapClass *klass)
{
}

G_DEFINE_ABSTRACT_TYPE (LsmDomNamedNodeMap, lsm_dom_named_node_map, G_TYPE_OBJECT)
