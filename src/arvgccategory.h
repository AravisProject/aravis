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

#ifndef ARV_GC_CATEGORY_H
#define ARV_GC_CATEGORY_H

#include <arvtypes.h>
#include <arvgcfeaturenode.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_CATEGORY             (arv_gc_category_get_type ())
#define ARV_GC_CATEGORY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_CATEGORY, ArvGcCategory))
#define ARV_GC_CATEGORY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_CATEGORY, ArvGcCategoryClass))
#define ARV_IS_GC_CATEGORY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_CATEGORY))
#define ARV_IS_GC_CATEGORY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_CATEGORY))
#define ARV_GC_CATEGORY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_CATEGORY, ArvGcCategoryClass))

typedef struct _ArvGcCategoryClass ArvGcCategoryClass;

struct _ArvGcCategory {
	ArvGcFeatureNode	base;

	GSList *features;
};

struct _ArvGcCategoryClass {
	ArvGcFeatureNodeClass parent_class;
};

GType 		arv_gc_category_get_type 	(void);
ArvGcNode * 	arv_gc_category_new 		(void);
const GSList * 	arv_gc_category_get_features 	(ArvGcCategory *category);

G_END_DECLS

#endif
