/* Aravis - Digital camera library
 *
 * Copyright © 2009-2019 Emmanuel Pacaud
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

#ifndef ARV_GC_CONVERTER_PRIVATE_H
#define ARV_GC_CONVERTER_PRIVATE_H

#include <arvgcconverter.h>
#include <arvgcenums.h>

G_BEGIN_DECLS

typedef enum {
	ARV_GC_CONVERTER_NODE_TYPE_VALUE,
	ARV_GC_CONVERTER_NODE_TYPE_MIN,
	ARV_GC_CONVERTER_NODE_TYPE_MAX,
	ARV_GC_CONVERTER_NODE_TYPE_INC
} ArvGcConverterNodeType;

ArvGcRepresentation	arv_gc_converter_get_representation 	(ArvGcConverter *gc_converter, GError **error);
const char * 		arv_gc_converter_get_unit	 	(ArvGcConverter *gc_converter, GError **error);
ArvGcIsLinear		arv_gc_converter_get_is_linear		(ArvGcConverter *gc_converter, GError **error);

gint64 			arv_gc_converter_convert_to_int64 	(ArvGcConverter *gc_converter, ArvGcConverterNodeType node_type, GError **error);
double 			arv_gc_converter_convert_to_double 	(ArvGcConverter *gc_converter, ArvGcConverterNodeType node_type, GError **error);
void 			arv_gc_converter_convert_from_int64 	(ArvGcConverter *gc_converter, gint64 value, GError **error);
void			arv_gc_converter_convert_from_double 	(ArvGcConverter *gc_converter, double value, GError **error);

G_END_DECLS

#endif
