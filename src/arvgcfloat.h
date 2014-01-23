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

#ifndef ARV_GC_FLOAT_H
#define ARV_GC_FLOAT_H

#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_FLOAT             	(arv_gc_float_get_type ())
#define ARV_GC_FLOAT(obj)             	(G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_FLOAT, ArvGcFloat))
#define ARV_IS_GC_FLOAT(obj)          	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_FLOAT))
#define ARV_GC_FLOAT_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), ARV_TYPE_GC_FLOAT, ArvGcFloatInterface))

typedef struct _ArvGcFloatInterface ArvGcFloatInterface;

struct _ArvGcFloatInterface {
	GTypeInterface parent;

	double		(*get_value)		(ArvGcFloat *gc_float, GError **error);
	void		(*set_value)		(ArvGcFloat *gc_float, double value, GError **error);
	double		(*get_min)		(ArvGcFloat *gc_float, GError **error);
	double		(*get_max)		(ArvGcFloat *gc_float, GError **error);
	double		(*get_inc)		(ArvGcFloat *gc_float, GError **error);
	const char *	(*get_unit)		(ArvGcFloat *gc_float, GError **error);
	void		(*impose_min)		(ArvGcFloat *gc_float, double minimum, GError **error);
	void		(*impose_max)		(ArvGcFloat *gc_float, double maximum, GError **error);
};

GType arv_gc_float_get_type (void);

double		arv_gc_float_get_value		(ArvGcFloat *gc_float, GError **error);
void		arv_gc_float_set_value		(ArvGcFloat *gc_float, double value, GError **error);
double		arv_gc_float_get_min		(ArvGcFloat *gc_float, GError **error);
double		arv_gc_float_get_max		(ArvGcFloat *gc_float, GError **error);
double		arv_gc_float_get_inc		(ArvGcFloat *gc_float, GError **error);
const char *	arv_gc_float_get_unit		(ArvGcFloat *gc_float, GError **error);
void		arv_gc_float_impose_min		(ArvGcFloat *gc_float, double minimum, GError **error);
void		arv_gc_float_impose_max		(ArvGcFloat *gc_float, double maximum, GError **error);

/* FIXME get_representation, has_inc and get_inc are missing */

G_END_DECLS

#endif
