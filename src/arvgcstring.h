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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_GC_STRING_H
#define ARV_GC_STRING_H

#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_STRING             		(arv_gc_string_get_type ())
#define ARV_GC_STRING(obj)             		(G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_STRING, ArvGcString))
#define ARV_IS_GC_STRING(obj)          		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_STRING))
#define ARV_GC_STRING_GET_INTERFACE(obj)   	(G_TYPE_INSTANCE_GET_INTERFACE((obj), ARV_TYPE_GC_STRING, ArvGcStringInterface))

typedef struct _ArvGcStringInterface ArvGcStringInterface;

struct _ArvGcStringInterface {
	GTypeInterface parent;

	const char *	(*get_value)		(ArvGcString *gc_string);
	void		(*set_value)		(ArvGcString *gc_string, const char *value);
	gint64		(*get_max_length)	(ArvGcString *gc_string);
};

GType arv_gc_string_get_type (void);

const char *	arv_gc_string_get_value		(ArvGcString *gc_string);
void		arv_gc_string_set_value		(ArvGcString *gc_string, const char *value);
gint64		arv_gc_string_get_max_length	(ArvGcString *gc_string);

G_END_DECLS

#endif
