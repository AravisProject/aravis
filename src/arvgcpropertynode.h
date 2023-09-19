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
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_GC_PROPERTY_NODE_H
#define ARV_GC_PROPERTY_NODE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvgcenums.h>
#include <arvgcnode.h>

G_BEGIN_DECLS

typedef enum {
	ARV_GC_PROPERTY_NODE_TYPE_UNKNOWN	= 0,
	ARV_GC_PROPERTY_NODE_TYPE_VALUE,
	ARV_GC_PROPERTY_NODE_TYPE_ADDRESS,
	ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION,
	ARV_GC_PROPERTY_NODE_TYPE_VISIBILITY,
	ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP,
	ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME,
	ARV_GC_PROPERTY_NODE_TYPE_MINIMUM,
	ARV_GC_PROPERTY_NODE_TYPE_MAXIMUM,
	ARV_GC_PROPERTY_NODE_TYPE_SLOPE,
	ARV_GC_PROPERTY_NODE_TYPE_INCREMENT,
	ARV_GC_PROPERTY_NODE_TYPE_IS_LINEAR,
	ARV_GC_PROPERTY_NODE_TYPE_REPRESENTATION,
	ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NOTATION,
	ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_PRECISION,
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
	ARV_GC_PROPERTY_NODE_TYPE_IMPOSED_ACCESS_MODE,
	ARV_GC_PROPERTY_NODE_TYPE_CACHABLE,
	ARV_GC_PROPERTY_NODE_TYPE_POLLING_TIME,
	ARV_GC_PROPERTY_NODE_TYPE_ENDIANNESS,
	ARV_GC_PROPERTY_NODE_TYPE_SIGN,
	ARV_GC_PROPERTY_NODE_TYPE_LSB,
	ARV_GC_PROPERTY_NODE_TYPE_MSB,
	ARV_GC_PROPERTY_NODE_TYPE_BIT,
	ARV_GC_PROPERTY_NODE_TYPE_COMMAND_VALUE,
	ARV_GC_PROPERTY_NODE_TYPE_CHUNK_ID,
	ARV_GC_PROPERTY_NODE_TYPE_EVENT_ID,
	ARV_GC_PROPERTY_NODE_TYPE_VALUE_INDEXED,
	ARV_GC_PROPERTY_NODE_TYPE_VALUE_DEFAULT,
	ARV_GC_PROPERTY_NODE_TYPE_STREAMABLE,
        ARV_GC_PROPERTY_NODE_TYPE_IS_DEPRECATED,

	ARV_GC_PROPERTY_NODE_TYPE_P_UNKNONW	= 1000,
	ARV_GC_PROPERTY_NODE_TYPE_P_FEATURE,
	ARV_GC_PROPERTY_NODE_TYPE_P_VALUE,
	ARV_GC_PROPERTY_NODE_TYPE_P_ADDRESS,
	ARV_GC_PROPERTY_NODE_TYPE_P_IS_IMPLEMENTED,
	ARV_GC_PROPERTY_NODE_TYPE_P_IS_LOCKED,
	ARV_GC_PROPERTY_NODE_TYPE_P_IS_AVAILABLE,
	ARV_GC_PROPERTY_NODE_TYPE_P_SELECTED,
	ARV_GC_PROPERTY_NODE_TYPE_P_MINIMUM,
	ARV_GC_PROPERTY_NODE_TYPE_P_MAXIMUM,
	ARV_GC_PROPERTY_NODE_TYPE_P_INCREMENT,
	ARV_GC_PROPERTY_NODE_TYPE_P_INDEX,
	ARV_GC_PROPERTY_NODE_TYPE_P_LENGTH,
	ARV_GC_PROPERTY_NODE_TYPE_P_PORT,
	ARV_GC_PROPERTY_NODE_TYPE_P_VARIABLE,
	ARV_GC_PROPERTY_NODE_TYPE_P_INVALIDATOR,
	ARV_GC_PROPERTY_NODE_TYPE_P_COMMAND_VALUE,
	ARV_GC_PROPERTY_NODE_TYPE_P_VALUE_INDEXED,
	ARV_GC_PROPERTY_NODE_TYPE_P_VALUE_DEFAULT,
        ARV_GC_PROPERTY_NODE_TYPE_P_ALIAS,
        ARV_GC_PROPERTY_NODE_TYPE_P_CAST_ALIAS
} ArvGcPropertyNodeType;

#define ARV_TYPE_GC_PROPERTY_NODE             (arv_gc_property_node_get_type ())
ARV_API G_DECLARE_DERIVABLE_TYPE (ArvGcPropertyNode, arv_gc_property_node, ARV, GC_PROPERTY_NODE, ArvGcNode)

struct _ArvGcPropertyNodeClass {
	ArvGcNodeClass parent_class;
};

ARV_API ArvGcNode *		arv_gc_property_node_new_p_feature 		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_value 			(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_value		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_address		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_address		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_description 		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_visibility		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_tooltip 		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_display_name 		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_minimum		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_minimum		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_maximum		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_maximum		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_slope			(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_increment		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_increment		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_is_linear		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_representation		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_display_notation	(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_display_precision	(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_unit			(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_on_value		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_off_value		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_is_implemented	(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_is_available		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_is_locked		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_selected		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_length			(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_length		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_port			(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_variable		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_formula		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_formula_to		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_formula_from		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_expression		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_constant		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_access_mode		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_imposed_access_mode	(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_cachable		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_polling_time		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_endianness		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_sign			(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_lsb			(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_msb			(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_bit			(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_command_value		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_command_value	(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_chunk_id		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_event_id		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_value_default		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_value_default	(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_streamable		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_is_deprecated		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_alias		(void);
ARV_API ArvGcNode *		arv_gc_property_node_new_p_cast_alias		(void);

ARV_API const char *		arv_gc_property_node_get_name			(ArvGcPropertyNode *node);

ARV_API const char *		arv_gc_property_node_get_string			(ArvGcPropertyNode *node, GError **error);
ARV_API void			arv_gc_property_node_set_string			(ArvGcPropertyNode *node, const char *string, GError **error);
ARV_API gint64			arv_gc_property_node_get_int64			(ArvGcPropertyNode *node, GError **error);
ARV_API void			arv_gc_property_node_set_int64			(ArvGcPropertyNode *node, gint64 v_int64, GError **error);
ARV_API double			arv_gc_property_node_get_double			(ArvGcPropertyNode *node, GError **error);
ARV_API void			arv_gc_property_node_set_double			(ArvGcPropertyNode *node, double v_double, GError **error);

ARV_API guint			arv_gc_property_node_get_endianness		(ArvGcPropertyNode *self, guint default_value);
ARV_API ArvGcSignedness		arv_gc_property_node_get_sign			(ArvGcPropertyNode *self, ArvGcSignedness default_value);
ARV_API guint			arv_gc_property_node_get_lsb			(ArvGcPropertyNode *self, guint default_value);
ARV_API guint			arv_gc_property_node_get_msb			(ArvGcPropertyNode *self, guint default_value);
ARV_API ArvGcCachable		arv_gc_property_node_get_cachable		(ArvGcPropertyNode *self, ArvGcCachable default_value);
ARV_API ArvGcAccessMode		arv_gc_property_node_get_access_mode		(ArvGcPropertyNode *self, ArvGcAccessMode default_value);
ARV_API ArvGcVisibility		arv_gc_property_node_get_visibility		(ArvGcPropertyNode *self, ArvGcVisibility default_value);
ARV_API ArvGcRepresentation	arv_gc_property_node_get_representation		(ArvGcPropertyNode *self, ArvGcRepresentation default_value);
ARV_API ArvGcDisplayNotation	arv_gc_property_node_get_display_notation	(ArvGcPropertyNode *self, ArvGcDisplayNotation default_value);
ARV_API gint64			arv_gc_property_node_get_display_precision	(ArvGcPropertyNode *self, gint64 default_value);
ARV_API ArvGcStreamable		arv_gc_property_node_get_streamable		(ArvGcPropertyNode *self, ArvGcStreamable default_value);

ARV_API ArvGcNode *		arv_gc_property_node_get_linked_node		(ArvGcPropertyNode *node);
ARV_API ArvGcPropertyNodeType	arv_gc_property_node_get_node_type		(ArvGcPropertyNode *node);

G_END_DECLS

#endif
