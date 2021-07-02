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

#ifndef ARV_MISC_H
#define ARV_MISC_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_TYPE_VALUE (arv_value_get_type())

GType arv_value_get_type (void);

typedef struct _ArvValue ArvValue;

ArvValue * 	arv_value_new_double 		(double v_double);
ArvValue * 	arv_value_new_int64 		(double v_int64);
void 		arv_value_free 			(ArvValue *value);
void 		arv_value_copy 			(ArvValue *to, const ArvValue *from);
void 		arv_value_set_int64 		(ArvValue *value, gint64 v_int64);
void 		arv_value_set_double 		(ArvValue *value, double v_double);
gint64 		arv_value_get_int64 		(ArvValue *value);
double 		arv_value_get_double 		(ArvValue *value);
gboolean 	arv_value_holds_int64 		(ArvValue *value);
double 		arv_value_holds_double 		(ArvValue *value);

void 		arv_copy_memory_with_endianness	(void *to, size_t to_size, guint to_endianness,
						 void *from, size_t from_size, guint from_endianness);

void * 		arv_decompress 			(void *input_buffer, size_t input_size, size_t *output_size);

const char * 	arv_pixel_format_to_gst_caps_string 		(ArvPixelFormat pixel_format);
ArvPixelFormat 	arv_pixel_format_from_gst_caps 			(const char *name, const char *format, int bpp, int depth);
const char * 	arv_pixel_format_to_gst_0_10_caps_string 	(ArvPixelFormat pixel_format);
ArvPixelFormat 	arv_pixel_format_from_gst_0_10_caps 		(const char *name, int bpp, int depth, guint32 fourcc);

const char *	arv_vendor_alias_lookup		(const char *vendor);

G_END_DECLS

#endif
