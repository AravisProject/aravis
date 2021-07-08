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

#ifndef ARV_MISC_PRIVATE_H
#define ARV_MISC_PRIVATE_H

#include <arvmisc.h>

typedef struct _ArvHistogram ArvHistogram;

#define ARV_TYPE_HISTOGRAM (arv_histogram_get_type())

GType arv_histogram_get_type (void);

ArvHistogram *		arv_histogram_new 		(guint n_variables, guint n_bins, double bin_step, double offset);
ArvHistogram *          arv_histogram_ref               (ArvHistogram *histogram);
void                    arv_histogram_unref             (ArvHistogram *histogram);
void 			arv_histogram_reset 		(ArvHistogram *histogram);
gboolean 		arv_histogram_fill 		(ArvHistogram *histogram, guint histogram_id, int value);
void 			arv_histogram_set_variable_name	(ArvHistogram *histogram, guint histogram_id, char const *name);

char *			arv_histogram_to_string 	(const ArvHistogram *histogram);

struct _ArvValue {
	GType type;
	union {
		gint64 v_int64;
		double v_double;
	} data;
};

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

gboolean	arv_parse_genicam_url		(const char *url, gssize url_length,
						 char **scheme, char **authority, char **path,
						 char **query, char **fragment,
						 guint64 *address, guint64 *size);

void 		arv_copy_memory_with_endianness	(void *to, size_t to_size, guint to_endianness,
						 void *from, size_t from_size, guint from_endianness);

void * 		arv_decompress 			(void *input_buffer, size_t input_size, size_t *output_size);

const char *	arv_vendor_alias_lookup		(const char *vendor);

#if GLIB_CHECK_VERSION(2,68,0)
#define arv_memdup(p,s) g_memdup2(p,s)
#else
#define arv_memdup(p,s) g_memdup(p,(size_t) s)
#endif

#endif
