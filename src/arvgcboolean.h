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

#ifndef ARV_GC_BOOLEAN_H
#define ARV_GC_BOOLEAN_H

#include <arvtypes.h>
#include <arvgcfeaturenode.h>
#include <arvgcpropertynode.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_BOOLEAN             (arv_gc_boolean_get_type ())
#define ARV_GC_BOOLEAN(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_BOOLEAN, ArvGcBoolean))
#define ARV_GC_BOOLEAN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_BOOLEAN, ArvGcBooleanClass))
#define ARV_IS_GC_BOOLEAN(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_BOOLEAN))
#define ARV_IS_GC_BOOLEAN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_BOOLEAN))
#define ARV_GC_BOOLEAN_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_BOOLEAN, ArvGcBooleanClass))

typedef struct _ArvGcBooleanClass ArvGcBooleanClass;

struct _ArvGcBoolean {
	ArvGcFeatureNode	node;

	ArvGcPropertyNode *value;
	ArvGcPropertyNode *on_value;
	ArvGcPropertyNode *off_value;
};

struct _ArvGcBooleanClass {
	ArvGcFeatureNodeClass parent_class;
};

GType 		arv_gc_boolean_get_type 	(void);
ArvGcNode * 	arv_gc_boolean_new 		(void);

gboolean 	arv_gc_boolean_get_value 	(ArvGcBoolean *gc_boolean, GError **error);
void 		arv_gc_boolean_set_value 	(ArvGcBoolean *gc_boolean, gboolean v_boolean, GError **error);

G_END_DECLS

#endif
