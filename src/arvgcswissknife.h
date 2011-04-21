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

#ifndef ARV_GC_SWISS_KNIFE_H
#define ARV_GC_SWISS_KNIFE_H

#include <arvtypes.h>
#include <arvgcnode.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_SWISS_KNIFE             (arv_gc_swiss_knife_get_type ())
#define ARV_GC_SWISS_KNIFE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_SWISS_KNIFE, ArvGcSwissKnife))
#define ARV_GC_SWISS_KNIFE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_SWISS_KNIFE, ArvGcSwissKnifeClass))
#define ARV_IS_GC_SWISS_KNIFE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_SWISS_KNIFE))
#define ARV_IS_GC_SWISS_KNIFE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_SWISS_KNIFE))
#define ARV_GC_SWISS_KNIFE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_SWISS_KNIFE, ArvGcSwissKnifeClass))

typedef struct _ArvGcSwissKnifeClass ArvGcSwissKnifeClass;

struct _ArvGcSwissKnife {
	ArvGcNode	node;

	GType value_type;
	GSList *variables;
	ArvEvaluator *formula;
};

struct _ArvGcSwissKnifeClass {
	ArvGcNodeClass parent_class;
};

GType 		arv_gc_swiss_knife_get_type 	(void);

ArvGcNode * 	arv_gc_swiss_knife_new 		(void);
ArvGcNode * 	arv_gc_swiss_knife_new_integer 	(void);

G_END_DECLS

#endif
