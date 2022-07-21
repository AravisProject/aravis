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

#ifndef ARV_GC_FLOAT_H
#define ARV_GC_FLOAT_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvgcenums.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_FLOAT             	(arv_gc_float_get_type ())
ARV_API G_DECLARE_INTERFACE (ArvGcFloat, arv_gc_float, ARV, GC_FLOAT, GObject)

struct _ArvGcFloatInterface {
	GTypeInterface parent;

	double			(*get_value)		(ArvGcFloat *gc_float, GError **error);
	void			(*set_value)		(ArvGcFloat *gc_float, double value, GError **error);
	double			(*get_min)		(ArvGcFloat *gc_float, GError **error);
	double			(*get_max)		(ArvGcFloat *gc_float, GError **error);
	double			(*get_inc)		(ArvGcFloat *gc_float, GError **error);
	ArvGcRepresentation	(*get_representation)	(ArvGcFloat *gc_float);
	ArvGcDisplayNotation	(*get_display_notation) (ArvGcFloat *gc_float);
	gint64			(*get_display_precision)(ArvGcFloat *gc_float);
	const char *		(*get_unit)		(ArvGcFloat *gc_float);
	void			(*impose_min)		(ArvGcFloat *gc_float, double minimum, GError **error);
	void			(*impose_max)		(ArvGcFloat *gc_float, double maximum, GError **error);
};

ARV_API double			arv_gc_float_get_value			(ArvGcFloat *gc_float, GError **error);
ARV_API void			arv_gc_float_set_value			(ArvGcFloat *gc_float, double value, GError **error);
ARV_API double			arv_gc_float_get_min			(ArvGcFloat *gc_float, GError **error);
ARV_API double			arv_gc_float_get_max			(ArvGcFloat *gc_float, GError **error);
ARV_API double			arv_gc_float_get_inc			(ArvGcFloat *gc_float, GError **error);
ARV_API ArvGcRepresentation	arv_gc_float_get_representation		(ArvGcFloat *gc_float);
ARV_API const char *		arv_gc_float_get_unit			(ArvGcFloat *gc_float);
ARV_API ArvGcDisplayNotation	arv_gc_float_get_display_notation	(ArvGcFloat *gc_float);
ARV_API gint64			arv_gc_float_get_display_precision	(ArvGcFloat *gc_float);
ARV_API void			arv_gc_float_impose_min			(ArvGcFloat *gc_float, double minimum, GError **error);
ARV_API void			arv_gc_float_impose_max			(ArvGcFloat *gc_float, double maximum, GError **error);

/* FIXME has_inc is missing */

G_END_DECLS

#endif
