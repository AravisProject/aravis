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

#ifndef ARV_DOM_NODE_LIST_H
#define ARV_DOM_NODE_LIST_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_TYPE_DOM_NODE_LIST             (arv_dom_node_list_get_type ())
ARV_API G_DECLARE_DERIVABLE_TYPE (ArvDomNodeList, arv_dom_node_list, ARV, DOM_NODE_LIST, GObject)

struct _ArvDomNodeListClass {
	GObjectClass parent_class;

	ArvDomNode *	(*get_item) 		(ArvDomNodeList *list, unsigned int index);
	unsigned int	(*get_length)		(ArvDomNodeList *list);
};

ARV_API ArvDomNode *		arv_dom_node_list_get_item		(ArvDomNodeList *list, unsigned int index);
ARV_API unsigned int		arv_dom_node_list_get_length		(ArvDomNodeList *list);

G_END_DECLS

#endif
