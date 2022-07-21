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

#ifndef ARV_GC_INTEGER_H
#define ARV_GC_INTEGER_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvgcenums.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_INTEGER             	(arv_gc_integer_get_type ())
ARV_API G_DECLARE_INTERFACE (ArvGcInteger, arv_gc_integer, ARV, GC_INTEGER, GObject)

struct _ArvGcIntegerInterface {
	GTypeInterface parent;

	gint64			(*get_value)		(ArvGcInteger *gc_integer, GError **error);
	void			(*set_value)		(ArvGcInteger *gc_integer, gint64 value, GError **error);
	gint64			(*get_min)		(ArvGcInteger *gc_integer, GError **error);
	gint64			(*get_max)		(ArvGcInteger *gc_integer, GError **error);
	gint64			(*get_inc)		(ArvGcInteger *gc_integer, GError **error);
	ArvGcRepresentation	(*get_representation)	(ArvGcInteger *gc_integer);
	const char *		(*get_unit)		(ArvGcInteger *gc_integer);
	void			(*impose_min)		(ArvGcInteger *gc_integer, gint64 minimum, GError **error);
	void			(*impose_max)		(ArvGcInteger *gc_integer, gint64 maximum, GError **error);
};

ARV_API gint64			arv_gc_integer_get_value		(ArvGcInteger *gc_integer, GError **error);
ARV_API void			arv_gc_integer_set_value		(ArvGcInteger *gc_integer, gint64 value, GError **error);
ARV_API gint64			arv_gc_integer_get_min			(ArvGcInteger *gc_integer, GError **error);
ARV_API gint64			arv_gc_integer_get_max			(ArvGcInteger *gc_integer, GError **error);
ARV_API gint64			arv_gc_integer_get_inc			(ArvGcInteger *gc_integer, GError **error);
ARV_API ArvGcRepresentation	arv_gc_integer_get_representation	(ArvGcInteger *gc_integer);
ARV_API const char *		arv_gc_integer_get_unit			(ArvGcInteger *gc_integer);
ARV_API void			arv_gc_integer_impose_min		(ArvGcInteger *gc_integer, gint64 minimum, GError **error);
ARV_API void			arv_gc_integer_impose_max		(ArvGcInteger *gc_integer, gint64 maximum, GError **error);

G_END_DECLS

#endif
