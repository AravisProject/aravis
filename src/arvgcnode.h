/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_GC_NODE_H
#define ARV_GC_NODE_H

#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_NODE             (arv_gc_node_get_type ())
#define ARV_GC_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_NODE, ArvGcNode))
#define ARV_GC_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_NODE, ArvGcNodeClass))
#define ARV_IS_GC_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_NODE))
#define ARV_IS_GC_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_NODE))
#define ARV_GC_NODE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_NODE, ArvGcNodeClass))

typedef struct _ArvGcNodePrivate ArvGcNodePrivate;
typedef struct _ArvGcNodeClass ArvGcNodeClass;

struct _ArvGcNode {
	GObject	object;

	ArvGcNodePrivate *priv;
};

struct _ArvGcNodeClass {
	GObjectClass parent_class;

	void		(*set_attribute)		(ArvGcNode *node, const char *name, const char *value);
	void 		(*add_element)			(ArvGcNode *node, const char *name, const char *content,
							 const char **attributes);
	GType		(*get_value_type)		(ArvGcNode *node);
};

GType arv_gc_node_get_type (void);

ArvGcNode * 	arv_gc_node_new 			(void);
GType 		arv_gc_node_get_value_type 		(ArvGcNode *node);
void		arv_gc_node_set_genicam			(ArvGcNode *node, ArvGc *genicam);
ArvGc * 	arv_gc_node_get_genicam			(ArvGcNode *node);
const char *	arv_gc_node_get_name			(ArvGcNode *node);
const char *	arv_gc_node_get_tooltip			(ArvGcNode *node);
const char *	arv_gc_node_get_description		(ArvGcNode *node);
void		arv_gc_node_set_attribute 		(ArvGcNode *node, const char *name, const char *value);
void 		arv_gc_node_add_element 		(ArvGcNode *node, const char *name, const char *content,
							 const char **attributes);
void 		arv_gc_node_add_child 			(ArvGcNode *node, ArvGcNode *child);
const GSList *	arv_gc_node_get_childs 			(ArvGcNode *node);

G_END_DECLS

#endif
