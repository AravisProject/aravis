/* Aravis - Digital camera library
 *
 * Copyright © 2009-2012 Emmanuel Pacaud
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

#ifndef ARV_GC_PROPERTY_NODE_H
#define ARV_GC_PROPERTY_NODE_H

#include <arvtypes.h>
#include <arvdomelement.h>

G_BEGIN_DECLS

typedef enum {
	ARV_GC_PROPERTY_NODE_TYPE_UNKNOWN,
	ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION,
	ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP,
	ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME,
	ARV_GC_PROPERTY_NODE_TYPE_P_IS_IMPLEMENTED,
	ARV_GC_PROPERTY_NODE_TYPE_P_IS_AVAILABLE
} ArvGcPropertyNodeType;

#define ARV_TYPE_GC_PROPERTY_NODE             (arv_gc_property_node_get_type ())
#define ARV_GC_PROPERTY_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_PROPERTY_NODE, ArvGcPropertyNode))
#define ARV_GC_PROPERTY_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_PROPERTY_NODE, ArvGcPropertyNodeClass))
#define ARV_IS_GC_PROPERTY_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_PROPERTY_NODE))
#define ARV_IS_GC_PROPERTY_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_PROPERTY_NODE))
#define ARV_GC_PROPERTY_NODE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_PROPERTY_NODE, ArvGcPropertyNodeClass))

typedef struct _ArvGcPropertyNodePrivate ArvGcPropertyNodePrivate;
typedef struct _ArvGcPropertyNodeClass ArvGcPropertyNodeClass;

struct _ArvGcPropertyNode {
	ArvDomElement	base;

	ArvGcPropertyNodeType	type;
};

struct _ArvGcPropertyNodeClass {
	ArvDomElementClass parent_class;
};

GType arv_gc_property_node_get_type (void);

ArvGcPropertyNode * 	arv_gc_property_node_new_description 		(void);
ArvGcPropertyNode * 	arv_gc_property_node_new_tooltip 		(void);
ArvGcPropertyNode * 	arv_gc_property_node_new_display_name 		(void);
ArvGcPropertyNode * 	arv_gc_property_node_new_p_is_implemented 	(void);
ArvGcPropertyNode * 	arv_gc_property_node_new_p_is_available 	(void);
const char * 		arv_gc_property_node_get_content 		(ArvGcPropertyNode *node);

G_END_DECLS

#endif
