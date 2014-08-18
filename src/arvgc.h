/* Aravis - Digital camera library
 *
 * Copyright © 2009-2012 Emmanuel Pacaud
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

#ifndef ARV_GC_H
#define ARV_GC_H

#include <arvtypes.h>
#include <arvdomdocument.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC             (arv_gc_get_type ())
#define ARV_GC(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC, ArvGc))
#define ARV_GC_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC, ArvGcClass))
#define ARV_IS_GC(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC))
#define ARV_IS_GC_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC))
#define ARV_GC_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC, ArvGcClass))

typedef struct _ArvGcPrivate ArvGcPrivate;
typedef struct _ArvGcClass ArvGcClass;

struct _ArvGc {
	ArvDomDocument base;

	ArvGcPrivate *priv;
};

struct _ArvGcClass {
	ArvDomDocumentClass parent_class;
};

GType arv_gc_get_type (void);

ArvGc * 		arv_gc_new 			(ArvDevice *device, const void *xml, size_t size);
void 			arv_gc_register_feature_node 	(ArvGc *genicam, ArvGcFeatureNode *node);
void 			arv_gc_set_default_node_data 	(ArvGc *genicam, const char *node_name, const char *node_data);
ArvGcNode *		arv_gc_get_node			(ArvGc *genicam, const char *name);
ArvDevice *		arv_gc_get_device		(ArvGc *genicam);
void			arv_gc_set_buffer		(ArvGc *genicam, ArvBuffer *buffer);
ArvBuffer *		arv_gc_get_buffer		(ArvGc *genicam);

G_END_DECLS

#endif
