/* Aravis - Digital camera library
 *
 * Copyright © 2009-2011 Emmanuel Pacaud
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

#ifndef ARV_DEBUG_H
#define ARV_DEBUG_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	ARV_DEBUG_LEVEL_NONE,
	ARV_DEBUG_LEVEL_WARNING,
	ARV_DEBUG_LEVEL_DEBUG,
	ARV_DEBUG_LEVEL_LOG,
	ARV_DEBUG_LEVEL_COUNT
} ArvDebugLevel;

typedef struct {
	char *name;
	ArvDebugLevel level;
} ArvDebugCategory;

extern ArvDebugCategory arv_debug_category_dom;
extern ArvDebugCategory arv_debug_category_interface;
extern ArvDebugCategory arv_debug_category_device;
extern ArvDebugCategory arv_debug_category_chunk;
extern ArvDebugCategory arv_debug_category_stream;
extern ArvDebugCategory arv_debug_category_stream_thread;
extern ArvDebugCategory arv_debug_category_gvcp;
extern ArvDebugCategory arv_debug_category_gvsp;
extern ArvDebugCategory arv_debug_category_genicam;
extern ArvDebugCategory arv_debug_category_evaluator;
extern ArvDebugCategory arv_debug_category_misc;

#define arv_debug_dom(...)	 	arv_debug (&arv_debug_category_interface, __VA_ARGS__)
#define arv_log_dom(...)		arv_log (&arv_debug_category_interface, __VA_ARGS__)
#define arv_warning_dom(...)		arv_warning (&arv_debug_category_interface, __VA_ARGS__)

#define arv_debug_interface(...) 	arv_debug (&arv_debug_category_interface, __VA_ARGS__)
#define arv_log_interface(...)		arv_log (&arv_debug_category_interface, __VA_ARGS__)
#define arv_warning_interface(...)	arv_warning (&arv_debug_category_interface, __VA_ARGS__)

#define arv_debug_device(...) 		arv_debug (&arv_debug_category_device, __VA_ARGS__)
#define arv_log_device(...)		arv_log (&arv_debug_category_device, __VA_ARGS__)
#define arv_warning_device(...)		arv_warning (&arv_debug_category_device, __VA_ARGS__)

#define arv_debug_chunk(...) 		arv_debug (&arv_debug_category_chunk, __VA_ARGS__)
#define arv_log_chunk(...)		arv_log (&arv_debug_category_chunk, __VA_ARGS__)
#define arv_warning_chunk(...)		arv_warning (&arv_debug_category_chunk, __VA_ARGS__)

#define arv_debug_stream(...) 		arv_debug (&arv_debug_category_stream, __VA_ARGS__)
#define arv_log_stream(...)		arv_log (&arv_debug_category_stream, __VA_ARGS__)
#define arv_warning_stream(...)		arv_warning (&arv_debug_category_stream, __VA_ARGS__)

#define arv_debug_stream_thread(...) 	arv_debug (&arv_debug_category_stream_thread, __VA_ARGS__)
#define arv_log_stream_thread(...)	arv_log (&arv_debug_category_stream_thread, __VA_ARGS__)
#define arv_warning_stream_thread(...)	arv_warning (&arv_debug_category_stream_thread, __VA_ARGS__)

#define arv_debug_gvcp(...) 		arv_debug (&arv_debug_category_gvcp, __VA_ARGS__)
#define arv_log_gvcp(...)		arv_log (&arv_debug_category_gvcp, __VA_ARGS__)
#define arv_warning_gvcp(...)		arv_warning (&arv_debug_category_gvcp, __VA_ARGS__)

#define arv_debug_gvsp(...) 		arv_debug (&arv_debug_category_gvsp, __VA_ARGS__)
#define arv_log_gvsp(...)		arv_log (&arv_debug_category_gvsp, __VA_ARGS__)
#define arv_warning_gvsp(...)		arv_warning (&arv_debug_category_gvsp, __VA_ARGS__)

#define arv_debug_genicam(...) 		arv_debug (&arv_debug_category_genicam, __VA_ARGS__)
#define arv_log_genicam(...)		arv_log (&arv_debug_category_genicam, __VA_ARGS__)
#define arv_warning_genicam(...)		arv_warning (&arv_debug_category_genicam, __VA_ARGS__)

#define arv_debug_evaluator(...) 	arv_debug (&arv_debug_category_evaluator, __VA_ARGS__)
#define arv_log_evaluator(...)		arv_log (&arv_debug_category_evaluator, __VA_ARGS__)
#define arv_warning_evaluator(...)	arv_warning (&arv_debug_category_evaluator, __VA_ARGS__)

#define arv_debug_misc(...) 		arv_debug (&arv_debug_category_misc, __VA_ARGS__)
#define arv_log_misc(...)		arv_log (&arv_debug_category_misc, __VA_ARGS__)
#define arv_warning_misc(...)		arv_warning (&arv_debug_category_misc, __VA_ARGS__)

void 		arv_warning 			(ArvDebugCategory *category, const char *format, ...);
void 		arv_debug 			(ArvDebugCategory *category, const char *format, ...);
void 		arv_log 			(ArvDebugCategory *category, const char *format, ...);

gboolean	arv_debug_check			(ArvDebugCategory *category, ArvDebugLevel level);
void		arv_debug_enable		(const char *category_selection);
void 		arv_debug_shutdown 		(void);

G_END_DECLS

#endif
