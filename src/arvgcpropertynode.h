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

#ifndef ARV_GC_PROPERTY_NODE_H
#define ARV_GC_PROPERTY_NODE_H

#include <arvtypes.h>
#include <arvgcnode.h>

G_BEGIN_DECLS

typedef enum {
	ARV_GC_PROPERTY_NODE_TYPE_UNKNOWN	= 0,
	ARV_GC_PROPERTY_NODE_TYPE_VALUE,
	ARV_GC_PROPERTY_NODE_TYPE_ADDRESS,
	ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION,
	ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP,
	ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME,
	ARV_GC_PROPERTY_NODE_TYPE_MINIMUM,
	ARV_GC_PROPERTY_NODE_TYPE_MAXIMUM,
	ARV_GC_PROPERTY_NODE_TYPE_INCREMENT,
	ARV_GC_PROPERTY_NODE_TYPE_UNIT,
	ARV_GC_PROPERTY_NODE_TYPE_ON_VALUE,
	ARV_GC_PROPERTY_NODE_TYPE_OFF_VALUE,
	ARV_GC_PROPERTY_NODE_TYPE_LENGTH,
	ARV_GC_PROPERTY_NODE_TYPE_FORMULA,
	ARV_GC_PROPERTY_NODE_TYPE_FORMULA_TO,
	ARV_GC_PROPERTY_NODE_TYPE_FORMULA_FROM,
	ARV_GC_PROPERTY_NODE_TYPE_EXPRESSION,
	ARV_GC_PROPERTY_NODE_TYPE_CONSTANT,
	ARV_GC_PROPERTY_NODE_TYPE_ACCESS_MODE,
	ARV_GC_PROPERTY_NODE_TYPE_CACHABLE,
	ARV_GC_PROPERTY_NODE_TYPE_POLLING_TIME,
	ARV_GC_PROPERTY_NODE_TYPE_ENDIANESS,
	ARV_GC_PROPERTY_NODE_TYPE_SIGN,
	ARV_GC_PROPERTY_NODE_TYPE_LSB,
	ARV_GC_PROPERTY_NODE_TYPE_MSB,
	ARV_GC_PROPERTY_NODE_TYPE_BIT,
	ARV_GC_PROPERTY_NODE_TYPE_COMMAND_VALUE,
	ARV_GC_PROPERTY_NODE_TYPE_CHUNK_ID,

	ARV_GC_PROPERTY_NODE_TYPE_P_UNKNONW	= 1000,
	ARV_GC_PROPERTY_NODE_TYPE_P_FEATURE,
	ARV_GC_PROPERTY_NODE_TYPE_P_VALUE,
	ARV_GC_PROPERTY_NODE_TYPE_P_ADDRESS,
	ARV_GC_PROPERTY_NODE_TYPE_P_IS_IMPLEMENTED,
	ARV_GC_PROPERTY_NODE_TYPE_P_IS_LOCKED,
	ARV_GC_PROPERTY_NODE_TYPE_P_IS_AVAILABLE,
	ARV_GC_PROPERTY_NODE_TYPE_P_MINIMUM,
	ARV_GC_PROPERTY_NODE_TYPE_P_MAXIMUM,
	ARV_GC_PROPERTY_NODE_TYPE_P_INCREMENT,
	ARV_GC_PROPERTY_NODE_TYPE_P_INDEX,
	ARV_GC_PROPERTY_NODE_TYPE_P_LENGTH,
	ARV_GC_PROPERTY_NODE_TYPE_P_PORT,
	ARV_GC_PROPERTY_NODE_TYPE_P_VARIABLE,
	ARV_GC_PROPERTY_NODE_TYPE_P_INVALIDATOR,
	ARV_GC_PROPERTY_NODE_TYPE_P_COMMAND_VALUE
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
	ArvGcNode base;

	ArvGcPropertyNodeType	type;

	gboolean value_data_up_to_date;
	char *value_data;
};

struct _ArvGcPropertyNodeClass {
	ArvGcNodeClass parent_class;
};

GType arv_gc_property_node_get_type (void);

ArvGcNode * 	arv_gc_property_node_new_p_feature 		(void);
ArvGcNode * 	arv_gc_property_node_new_value 			(void);
ArvGcNode * 	arv_gc_property_node_new_p_value		(void);
ArvGcNode * 	arv_gc_property_node_new_address		(void);
ArvGcNode * 	arv_gc_property_node_new_p_address		(void);
ArvGcNode * 	arv_gc_property_node_new_description 		(void);
ArvGcNode * 	arv_gc_property_node_new_tooltip 		(void);
ArvGcNode * 	arv_gc_property_node_new_display_name 		(void);
ArvGcNode * 	arv_gc_property_node_new_minimum		(void);
ArvGcNode * 	arv_gc_property_node_new_p_minimum		(void);
ArvGcNode * 	arv_gc_property_node_new_maximum		(void);
ArvGcNode * 	arv_gc_property_node_new_p_maximum		(void);
ArvGcNode * 	arv_gc_property_node_new_increment		(void);
ArvGcNode * 	arv_gc_property_node_new_p_increment		(void);
ArvGcNode * 	arv_gc_property_node_new_unit			(void);
ArvGcNode * 	arv_gc_property_node_new_on_value 		(void);
ArvGcNode * 	arv_gc_property_node_new_off_value 		(void);
ArvGcNode * 	arv_gc_property_node_new_p_is_implemented 	(void);
ArvGcNode * 	arv_gc_property_node_new_p_is_available 	(void);
ArvGcNode * 	arv_gc_property_node_new_p_is_locked	 	(void);
ArvGcNode * 	arv_gc_property_node_new_length			(void);
ArvGcNode * 	arv_gc_property_node_new_p_length		(void);
ArvGcNode * 	arv_gc_property_node_new_p_port 		(void);
ArvGcNode * 	arv_gc_property_node_new_formula		(void);
ArvGcNode * 	arv_gc_property_node_new_formula_to		(void);
ArvGcNode * 	arv_gc_property_node_new_formula_from		(void);
ArvGcNode * 	arv_gc_property_node_new_expression		(void);
ArvGcNode * 	arv_gc_property_node_new_constant		(void);
ArvGcNode * 	arv_gc_property_node_new_access_mode		(void);
ArvGcNode * 	arv_gc_property_node_new_cachable		(void);
ArvGcNode * 	arv_gc_property_node_new_polling_time		(void);
ArvGcNode * 	arv_gc_property_node_new_endianess		(void);
ArvGcNode * 	arv_gc_property_node_new_sign			(void);
ArvGcNode * 	arv_gc_property_node_new_lsb			(void);
ArvGcNode * 	arv_gc_property_node_new_msb			(void);
ArvGcNode * 	arv_gc_property_node_new_bit			(void);
ArvGcNode * 	arv_gc_property_node_new_command_value		(void);
ArvGcNode * 	arv_gc_property_node_new_p_command_value	(void);
ArvGcNode * 	arv_gc_property_node_new_chunk_id 		(void);

const char * 		arv_gc_property_node_get_string 	(ArvGcPropertyNode *node, GError **error);
void	 		arv_gc_property_node_set_string 	(ArvGcPropertyNode *node, const char *string, GError **error);
gint64			arv_gc_property_node_get_int64		(ArvGcPropertyNode *node, GError **error);
void			arv_gc_property_node_set_int64		(ArvGcPropertyNode *node, gint64 v_int64, GError **error);
double 			arv_gc_property_node_get_double 	(ArvGcPropertyNode *node, GError **error);
void 			arv_gc_property_node_set_double 	(ArvGcPropertyNode *node, double v_double, GError **error);

ArvGcNode *		arv_gc_property_node_get_linked_node	(ArvGcPropertyNode *node);
ArvGcPropertyNodeType	arv_gc_property_node_get_node_type	(ArvGcPropertyNode *node);

G_END_DECLS

#endif
