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

#ifndef ARV_GC_REGISTER_NODE_H
#define ARV_GC_REGISTER_NODE_H

#include <arvtypes.h>
#include <arvgcfeaturenode.h>
#include <arvgcpropertynode.h>

G_BEGIN_DECLS

/**
 * ArvGcSign:
 * @ARV_GC_SIGN_SIGNED: signed integer
 * @ARV_GC_SIGN_UNSIGNED: unsigned integer
 */

typedef enum
{
	ARV_GC_SIGN_SIGNED,
	ARV_GC_SIGN_UNSIGNED
} ArvGcSign;

/**
 * ArvGcRegisterNodeType:
 * @ARV_GC_REGISTER_NODE_TYPE_REGISTER: Register node
 * @ARV_GC_REGISTER_NODE_TYPE_INTEGER: IntReg node
 * @ARV_GC_REGISTER_NODE_TYPE_MASKED_INTEGER: MaskedIntReg node
 * @ARV_GC_REGISTER_NODE_TYPE_FLOAT: FloatReg node
 * @ARV_GC_REGISTER_NODE_TYPE_STRING: StringReg node
 * @ARV_GC_REGISTER_NODE_TYPE_STRUCT_REGISTER: StructReg node
 */

typedef enum {
       ARV_GC_REGISTER_NODE_TYPE_REGISTER,
       ARV_GC_REGISTER_NODE_TYPE_INTEGER,
       ARV_GC_REGISTER_NODE_TYPE_MASKED_INTEGER,
       ARV_GC_REGISTER_NODE_TYPE_FLOAT,
       ARV_GC_REGISTER_NODE_TYPE_STRING,
       ARV_GC_REGISTER_NODE_TYPE_STRUCT_REGISTER
} ArvGcRegisterNodeType;

#define ARV_TYPE_GC_REGISTER_NODE             (arv_gc_register_node_get_type ())
#define ARV_GC_REGISTER_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_REGISTER_NODE, ArvGcRegisterNode))
#define ARV_GC_REGISTER_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_REGISTER_NODE, ArvGcRegisterNodeClass))
#define ARV_IS_GC_REGISTER_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_REGISTER_NODE))
#define ARV_IS_GC_REGISTER_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_REGISTER_NODE))
#define ARV_GC_REGISTER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_REGISTER_NODE, ArvGcRegisterNodeClass))

typedef struct _ArvGcRegisterNodeClass ArvGcRegisterNodeClass;

struct _ArvGcRegisterNode {
	ArvGcFeatureNode	node;

	ArvGcRegisterNodeType type;
	GType value_type;

	GSList *addresses;
	GSList *swiss_knives;
	ArvGcPropertyNode *index;
	ArvGcPropertyNode *length;
	ArvGcPropertyNode *port;
	ArvGcPropertyNode *access_mode;
	ArvGcPropertyNode *cachable;
	ArvGcPropertyNode *polling_time;
	ArvGcPropertyNode *endianess;
	ArvGcPropertyNode *sign;
	ArvGcPropertyNode *lsb;
	ArvGcPropertyNode *msb;

	GSList *invalidators;		/* ArvGcPropertyNode list */

	void *cache;
	size_t cache_size;
	gboolean is_cache_valid;

	char v_string[G_ASCII_DTOSTR_BUF_SIZE];
};

struct _ArvGcRegisterNodeClass {
	ArvGcFeatureNodeClass parent_class;
};

GType 		arv_gc_register_node_get_type 			(void);
ArvGcNode * 	arv_gc_register_node_new 			(void);
ArvGcNode * 	arv_gc_register_node_new_integer 		(void);
ArvGcNode * 	arv_gc_register_node_new_masked_integer 	(void);
ArvGcNode * 	arv_gc_register_node_new_float	 		(void);
ArvGcNode * 	arv_gc_register_node_new_string 		(void);
ArvGcNode * 	arv_gc_register_node_new_struct_register	(void);

gint64 		arv_gc_register_node_get_masked_integer_value 	(ArvGcRegisterNode *gc_register_node,
								 guint lsb, guint msb, GError **error);
void 		arv_gc_register_node_set_masked_integer_value 	(ArvGcRegisterNode *gc_register_node, 
								 guint lsb, guint msb, gint64 value, GError **error);

G_END_DECLS

#endif
