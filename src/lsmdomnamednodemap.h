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

#ifndef LSM_DOM_NAMED_NODE_MAP_H
#define LSM_DOM_NAMED_NODE_MAP_H

#include <lsmdomtypes.h>

G_BEGIN_DECLS

#define LSM_TYPE_DOM_NAMED_NODE_MAP             (lsm_dom_named_node_map_get_type ())
#define LSM_DOM_NAMED_NODE_MAP(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), LSM_TYPE_DOM_NAMED_NODE_MAP, LsmDomNamedNodeMap))
#define LSM_DOM_NAMED_NODE_MAP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), LSM_TYPE_DOM_NAMED_NODE_MAP, LsmDomNamedNodeMapClass))
#define LSM_IS_DOM_NAMED_NODE_MAP(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LSM_TYPE_DOM_NAMED_NODE_MAP))
#define LSM_IS_DOM_NAMED_NODE_MAP_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), LSM_TYPE_DOM_NAMED_NODE_MAP))
#define LSM_DOM_NAMED_NODE_MAP_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), LSM_TYPE_DOM_NAMED_NODE_MAP, LsmDomNamedNodeMapClass))

typedef struct _LsmDomNamedNodeMapClass LsmDomNamedNodeMapClass;

struct _LsmDomNamedNodeMap {
	GObject	object;
};

struct _LsmDomNamedNodeMapClass {
	GObjectClass parent_class;

	LsmDomNode *	(*get) 			(LsmDomNamedNodeMap *map, const char *name);
	LsmDomNode *	(*set) 			(LsmDomNamedNodeMap *map, LsmDomNode *node);
	LsmDomNode *	(*remove) 		(LsmDomNamedNodeMap *map, const char *name);
	LsmDomNode *	(*get_item) 		(LsmDomNamedNodeMap *map, unsigned int index);
	unsigned int	(*get_length)		(LsmDomNamedNodeMap *map);
};

GType lsm_dom_named_node_map_get_type (void);

LsmDomNode *		lsm_dom_named_node_map_get_named_item 		(LsmDomNamedNodeMap *map, const char *name);
LsmDomNode *		lsm_dom_named_node_map_set_named_item 		(LsmDomNamedNodeMap *map, LsmDomNode *item);
LsmDomNode *		lsm_dom_named_node_map_remove_named_item	(LsmDomNamedNodeMap *map, const char *name);
LsmDomNode *		lsm_dom_named_node_map_get_item 		(LsmDomNamedNodeMap *map, unsigned int index);
unsigned int		lsm_dom_named_node_map_get_length		(LsmDomNamedNodeMap *map);

G_END_DECLS

#endif
