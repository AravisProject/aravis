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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_GC_INTEGER_NODE_H
#define ARV_GC_INTEGER_NODE_H

#include <arvtypes.h>
#include <arvgcfeaturenode.h>
#include <arvgcpropertynode.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_INTEGER_NODE             (arv_gc_integer_node_get_type ())
#define ARV_GC_INTEGER_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_INTEGER_NODE, ArvGcIntegerNode))
#define ARV_GC_INTEGER_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_INTEGER_NODE, ArvGcIntegerNodeClass))
#define ARV_IS_GC_INTEGER_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_INTEGER_NODE))
#define ARV_IS_GC_INTEGER_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_INTEGER_NODE))
#define ARV_GC_INTEGER_NODE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_INTEGER_NODE, ArvGcIntegerNodeClass))

typedef struct _ArvGcIntegerNodeClass ArvGcIntegerNodeClass;

struct _ArvGcIntegerNode {
	ArvGcFeatureNode	node;

	ArvGcPropertyNode *value;
	ArvGcPropertyNode *minimum;
	ArvGcPropertyNode *maximum;
	ArvGcPropertyNode *increment;
	ArvGcPropertyNode *unit;
};

struct _ArvGcIntegerNodeClass {
	ArvGcFeatureNodeClass parent_class;
};

GType 		arv_gc_integer_node_get_type 	(void);
ArvGcNode * 	arv_gc_integer_node_new 	(void);

G_END_DECLS

#endif
