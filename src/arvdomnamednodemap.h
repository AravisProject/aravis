/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
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
 * 	Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_DOM_NAMED_NODE_MAP_H
#define ARV_DOM_NAMED_NODE_MAP_H

#include <arvapi.h>
#include <arvtypes.h>

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

G_BEGIN_DECLS

#define ARV_TYPE_DOM_NAMED_NODE_MAP             (arv_dom_named_node_map_get_type ())
ARV_API G_DECLARE_DERIVABLE_TYPE (ArvDomNamedNodeMap, arv_dom_named_node_map, ARV, DOM_NAMED_NODE_MAP, GObject)

struct _ArvDomNamedNodeMapClass {
	GObjectClass parent_class;

	ArvDomNode *	(*get) 			(ArvDomNamedNodeMap *map, const char *name);
	ArvDomNode *	(*set) 			(ArvDomNamedNodeMap *map, ArvDomNode *node);
	ArvDomNode *	(*remove) 		(ArvDomNamedNodeMap *map, const char *name);
	ArvDomNode *	(*get_item) 		(ArvDomNamedNodeMap *map, unsigned int index);
	unsigned int	(*get_length)		(ArvDomNamedNodeMap *map);
};

ARV_API ArvDomNode *		arv_dom_named_node_map_get_named_item		(ArvDomNamedNodeMap *map, const char *name);
ARV_API ArvDomNode *		arv_dom_named_node_map_set_named_item		(ArvDomNamedNodeMap *map, ArvDomNode *item);
ARV_API ArvDomNode *		arv_dom_named_node_map_remove_named_item	(ArvDomNamedNodeMap *map, const char *name);
ARV_API ArvDomNode *		arv_dom_named_node_map_get_item 		(ArvDomNamedNodeMap *map, unsigned int index);
ARV_API unsigned int		arv_dom_named_node_map_get_length		(ArvDomNamedNodeMap *map);

G_END_DECLS

#endif
