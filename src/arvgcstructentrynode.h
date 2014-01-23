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

#ifndef ARV_GC_STRUCT_ENTRY_NODE_H
#define ARV_GC_STRUCT_ENTRY_NODE_H

#include <arvtypes.h>
#include <arvgcfeaturenode.h>
#include <arvgcpropertynode.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_STRUCT_ENTRY_NODE             (arv_gc_struct_entry_node_get_type ())
#define ARV_GC_STRUCT_ENTRY_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_STRUCT_ENTRY_NODE, ArvGcStructEntryNode))
#define ARV_GC_STRUCT_ENTRY_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_STRUCT_ENTRY_NODE, ArvGcStructEntryNodeClass))
#define ARV_IS_GC_STRUCT_ENTRY_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_STRUCT_ENTRY_NODE))
#define ARV_IS_GC_STRUCT_ENTRY_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_STRUCT_ENTRY_NODE))
#define ARV_GC_STRUCT_ENTRY_NODE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_STRUCT_ENTRY_NODE, ArvGcStructEntryNodeClass))

typedef struct _ArvGcStructEntryNodeClass ArvGcStructEntryNodeClass;

struct _ArvGcStructEntryNode {
	ArvGcFeatureNode	node;

	ArvGcPropertyNode *sign;
	ArvGcPropertyNode *lsb;
	ArvGcPropertyNode *msb;

	char v_string[G_ASCII_DTOSTR_BUF_SIZE];
};

struct _ArvGcStructEntryNodeClass {
	ArvGcFeatureNodeClass parent_class;
};

GType 		arv_gc_struct_entry_node_get_type 		(void);
ArvGcNode * 	arv_gc_struct_entry_node_new 			(void);

G_END_DECLS

#endif
