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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_TOOLS_H
#define ARV_TOOLS_H

#include <arvtypes.h>

G_BEGIN_DECLS

typedef struct _ArvStatistic ArvStatistic;

ArvStatistic *		arv_statistic_new 		(guint n_histograms, guint n_bins, guint bin_step, int offset);
void			arv_statistic_free		(ArvStatistic *statistic);
void 			arv_statistic_reset 		(ArvStatistic *statistic);
gboolean 		arv_statistic_fill 		(ArvStatistic *statistic, guint histogram_id, int value,
							 guint64 counter);
void 			arv_statistic_set_name 		(ArvStatistic *statistic, guint histogram_id, char const *name);

char *			arv_statistic_to_string 	(const ArvStatistic *statistic);

#define ARV_TYPE_VALUE (arv_value_get_type())

GType arv_value_get_type (void);

typedef struct _ArvValue ArvValue;
struct _ArvValue {
	GType type;
	union {
		gint64 v_int64;
		double v_double;
	} data;
};

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

void 		arv_copy_memory_with_endianess 	(void *to, size_t to_size, guint to_endianess,
						 void *from, size_t from_size, guint from_endianess);

void * 		arv_decompress 			(void *input_buffer, size_t input_size, size_t *output_size);

const char * 	arv_pixel_format_to_gst_caps_string 		(ArvPixelFormat pixel_format);
ArvPixelFormat 	arv_pixel_format_from_gst_caps 			(const char *name, const char *format);
const char * 	arv_pixel_format_to_gst_0_10_caps_string 	(ArvPixelFormat pixel_format);
ArvPixelFormat 	arv_pixel_format_from_gst_0_10_caps 		(const char *name, int bpp, int depth, guint32 fourcc);

/*
   Compatibility with old glib
 */

#if GLIB_CHECK_VERSION(2,36,0)
#define arv_g_type_init()
#else
#define arv_g_type_init() g_type_init()
#endif

#if GLIB_CHECK_VERSION(2,32,0)

#define ARV_DEFINE_STATIC_MUTEX(mutex) static GMutex mutex
#define arv_g_mutex_lock(mutex) g_mutex_lock(mutex)
#define arv_g_mutex_unlock(mutex) g_mutex_unlock(mutex)

#define arv_g_thread_init(vtable)
#define arv_g_thread_new(name,func,data) g_thread_new(name,func,data)

#else

#define ARV_DEFINE_STATIC_MUTEX(mutex) static GStaticMutex mutex = G_STATIC_MUTEX_INIT
#define arv_g_mutex_lock(mutex) g_static_mutex_lock(mutex)
#define arv_g_mutex_unlock(mutex) g_static_mutex_unlock(mutex)

#define arv_g_thread_init(vtable) g_thread_init(vtable)
#define arv_g_thread_new(name,func,data) g_thread_create(func,data,TRUE,NULL)

#endif

G_END_DECLS

#endif
