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

#ifndef ARV_DEBUG_PRIVATE_H
#define ARV_DEBUG_PRIVATE_H

#include <arvdebug.h>

G_BEGIN_DECLS

typedef enum {
	ARV_DEBUG_LEVEL_NONE,
	ARV_DEBUG_LEVEL_WARNING,
	ARV_DEBUG_LEVEL_INFO,
	ARV_DEBUG_LEVEL_DEBUG,
	ARV_DEBUG_LEVEL_TRACE,
	ARV_DEBUG_LEVEL_N_ELEMENTS
} ArvDebugLevel;

typedef struct {
	const char *name;
	const char *description;
	ArvDebugLevel level;
} ArvDebugCategoryInfos;

typedef enum {
	ARV_DEBUG_CATEGORY_INTERFACE,
	ARV_DEBUG_CATEGORY_DEVICE,
	ARV_DEBUG_CATEGORY_STREAM,
	ARV_DEBUG_CATEGORY_STREAM_THREAD,
	ARV_DEBUG_CATEGORY_CP,
	ARV_DEBUG_CATEGORY_SP,
	ARV_DEBUG_CATEGORY_GENICAM,
	ARV_DEBUG_CATEGORY_POLICIES,
	ARV_DEBUG_CATEGORY_CHUNK,
	ARV_DEBUG_CATEGORY_DOM,
	ARV_DEBUG_CATEGORY_EVALUATOR,
	ARV_DEBUG_CATEGORY_VIEWER,
	ARV_DEBUG_CATEGORY_MISC,
	ARV_DEBUG_CATEGORY_N_ELEMENTS
} ArvDebugCategory;

extern ArvDebugCategoryInfos arv_debug_category_infos[];

#define arv_warning_dom(...)		arv_warning (ARV_DEBUG_CATEGORY_DOM, __VA_ARGS__)
#define arv_info_dom(...)	 	arv_info (ARV_DEBUG_CATEGORY_DOM, __VA_ARGS__)
#define arv_debug_dom(...)		arv_debug (ARV_DEBUG_CATEGORY_DOM, __VA_ARGS__)

#define arv_warning_interface(...)	arv_warning (ARV_DEBUG_CATEGORY_INTERFACE, __VA_ARGS__)
#define arv_info_interface(...) 	arv_info (ARV_DEBUG_CATEGORY_INTERFACE, __VA_ARGS__)
#define arv_debug_interface(...)	arv_debug (ARV_DEBUG_CATEGORY_INTERFACE, __VA_ARGS__)

#define arv_warning_device(...)		arv_warning (ARV_DEBUG_CATEGORY_DEVICE, __VA_ARGS__)
#define arv_info_device(...) 		arv_info (ARV_DEBUG_CATEGORY_DEVICE, __VA_ARGS__)
#define arv_debug_device(...)		arv_debug (ARV_DEBUG_CATEGORY_DEVICE, __VA_ARGS__)

#define arv_warning_chunk(...)		arv_warning (ARV_DEBUG_CATEGORY_CHUNK, __VA_ARGS__)
#define arv_info_chunk(...) 		arv_info (ARV_DEBUG_CATEGORY_CHUNK, __VA_ARGS__)
#define arv_debug_chunk(...)		arv_debug (ARV_DEBUG_CATEGORY_CHUNK, __VA_ARGS__)

#define arv_warning_stream(...)		arv_warning (ARV_DEBUG_CATEGORY_STREAM, __VA_ARGS__)
#define arv_info_stream(...) 		arv_info (ARV_DEBUG_CATEGORY_STREAM, __VA_ARGS__)
#define arv_debug_stream(...)		arv_debug (ARV_DEBUG_CATEGORY_STREAM, __VA_ARGS__)

#define arv_warning_stream_thread(...)	arv_warning (ARV_DEBUG_CATEGORY_STREAM_THREAD, __VA_ARGS__)
#define arv_info_stream_thread(...) 	arv_info (ARV_DEBUG_CATEGORY_STREAM_THREAD, __VA_ARGS__)
#define arv_debug_stream_thread(...)	arv_debug (ARV_DEBUG_CATEGORY_STREAM_THREAD, __VA_ARGS__)

#define arv_warning_cp(...)		arv_warning (ARV_DEBUG_CATEGORY_CP, __VA_ARGS__)
#define arv_info_cp(...) 		arv_info (ARV_DEBUG_CATEGORY_CP, __VA_ARGS__)
#define arv_debug_cp(...)		arv_debug (ARV_DEBUG_CATEGORY_CP, __VA_ARGS__)
#define arv_trace_cp(...)		arv_trace (ARV_DEBUG_CATEGORY_CP, __VA_ARGS__)

#define arv_warning_sp(...)		arv_warning (ARV_DEBUG_CATEGORY_SP, __VA_ARGS__)
#define arv_info_sp(...) 		arv_info (ARV_DEBUG_CATEGORY_SP, __VA_ARGS__)
#define arv_debug_sp(...)		arv_debug (ARV_DEBUG_CATEGORY_SP, __VA_ARGS__)
#define arv_trace_sp(...)		arv_trace (ARV_DEBUG_CATEGORY_SP, __VA_ARGS__)

#define arv_warning_genicam(...)	arv_warning (ARV_DEBUG_CATEGORY_GENICAM, __VA_ARGS__)
#define arv_info_genicam(...) 		arv_info (ARV_DEBUG_CATEGORY_GENICAM, __VA_ARGS__)
#define arv_debug_genicam(...)		arv_debug (ARV_DEBUG_CATEGORY_GENICAM, __VA_ARGS__)

#define arv_warning_policies(...)	arv_warning (ARV_DEBUG_CATEGORY_POLICIES, __VA_ARGS__)
#define arv_info_policies(...) 		arv_info (ARV_DEBUG_CATEGORY_POLICIES, __VA_ARGS__)
#define arv_debug_policies(...)		arv_debug (ARV_DEBUG_CATEGORY_POLICIES, __VA_ARGS__)

#define arv_warning_evaluator(...)	arv_warning (ARV_DEBUG_CATEGORY_EVALUATOR, __VA_ARGS__)
#define arv_info_evaluator(...) 	arv_info (ARV_DEBUG_CATEGORY_EVALUATOR, __VA_ARGS__)
#define arv_debug_evaluator(...)	arv_debug (ARV_DEBUG_CATEGORY_EVALUATOR, __VA_ARGS__)

#define arv_warning_misc(...)		arv_warning (ARV_DEBUG_CATEGORY_MISC, __VA_ARGS__)
#define arv_info_misc(...) 		arv_info (ARV_DEBUG_CATEGORY_MISC, __VA_ARGS__)
#define arv_debug_misc(...)		arv_debug (ARV_DEBUG_CATEGORY_MISC, __VA_ARGS__)

#define arv_warning_viewer(...)		arv_warning (ARV_DEBUG_CATEGORY_VIEWER, __VA_ARGS__)
#define arv_info_viewer(...)	 	arv_info (ARV_DEBUG_CATEGORY_VIEWER, __VA_ARGS__)
#define arv_debug_viewer(...)		arv_debug (ARV_DEBUG_CATEGORY_VIEWER, __VA_ARGS__)

gboolean	arv_debug_check			(ArvDebugCategory category, ArvDebugLevel level);

/* private, but used by viewer */
ARV_API void	arv_warning 			(ArvDebugCategory category, const char *format, ...) G_GNUC_PRINTF (2,3);
ARV_API void	arv_info 			(ArvDebugCategory category, const char *format, ...) G_GNUC_PRINTF (2,3);
ARV_API void	arv_debug 			(ArvDebugCategory category, const char *format, ...) G_GNUC_PRINTF (2,3);
ARV_API void	arv_trace			(ArvDebugCategory category, const char *format, ...) G_GNUC_PRINTF (2,3);

/* private, but export for use in debugging tools */
ARV_API void	arv_debug_print_infos		(void);

G_END_DECLS

#endif

