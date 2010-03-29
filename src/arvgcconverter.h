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

#ifndef ARV_GC_CONVERTER_H
#define ARV_GC_CONVERTER_H

#include <arvtypes.h>
#include <arvgcnode.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_CONVERTER             (arv_gc_converter_get_type ())
#define ARV_GC_CONVERTER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_CONVERTER, ArvGcConverter))
#define ARV_GC_CONVERTER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_CONVERTER, ArvGcConverterClass))
#define ARV_IS_GC_CONVERTER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_CONVERTER))
#define ARV_IS_GC_CONVERTER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_CONVERTER))
#define ARV_GC_CONVERTER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_CONVERTER, ArvGcConverterClass))

typedef struct _ArvGcConverterClass ArvGcConverterClass;

struct _ArvGcConverter {
	ArvGcNode	node;

	gboolean is_integer;

	char *value;
	GSList *variables;
	ArvEvaluator *formula_to;
	ArvEvaluator *formula_from;
};

struct _ArvGcConverterClass {
	ArvGcNodeClass parent_class;
};

GType 		arv_gc_converter_get_type 	(void);

ArvGcNode * 	arv_gc_converter_new 		(void);
ArvGcNode * 	arv_gc_int_converter_new 	(void);

G_END_DECLS

#endif
