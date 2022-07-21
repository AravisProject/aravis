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

#ifndef ARV_GC_STRING_H
#define ARV_GC_STRING_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_STRING             		(arv_gc_string_get_type ())
ARV_API G_DECLARE_INTERFACE (ArvGcString, arv_gc_string, ARV, GC_STRING, GObject)

struct _ArvGcStringInterface {
	GTypeInterface parent;

	const char *	(*get_value)		(ArvGcString *gc_string, GError **error);
	void		(*set_value)		(ArvGcString *gc_string, const char *value, GError **error);
	gint64		(*get_max_length)	(ArvGcString *gc_string, GError **error);
};

ARV_API const char *	arv_gc_string_get_value		(ArvGcString *gc_string, GError **error);
ARV_API void		arv_gc_string_set_value		(ArvGcString *gc_string, const char *value, GError **error);
ARV_API gint64		arv_gc_string_get_max_length	(ArvGcString *gc_string, GError **error);

G_END_DECLS

#endif
