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

#ifndef ARV_DOM_NODE_LIST_H
#define ARV_DOM_NODE_LIST_H

#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_TYPE_DOM_NODE_LIST             (arv_dom_node_list_get_type ())
#define ARV_DOM_NODE_LIST(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_DOM_NODE_LIST, ArvDomNodeList))
#define ARV_DOM_NODE_LIST_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_DOM_NODE_LIST, ArvDomNodeListClass))
#define ARV_IS_DOM_NODE_LIST(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_DOM_NODE_LIST))
#define ARV_IS_DOM_NODE_LIST_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_DOM_NODE_LIST))
#define ARV_DOM_NODE_LIST_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_DOM_NODE_LIST, ArvDomNodeListClass))

typedef struct _ArvDomNodeListClass ArvDomNodeListClass;

struct _ArvDomNodeList {
	GObject	object;
};

struct _ArvDomNodeListClass {
	GObjectClass parent_class;

	ArvDomNode *	(*get_item) 		(ArvDomNodeList *list, unsigned int index);
	unsigned int	(*get_length)		(ArvDomNodeList *list);
};

GType arv_dom_node_list_get_type (void);

ArvDomNode *		arv_dom_node_list_get_item 		(ArvDomNodeList *list, unsigned int index);
unsigned int		arv_dom_node_list_get_length		(ArvDomNodeList *list);

G_END_DECLS

#endif
