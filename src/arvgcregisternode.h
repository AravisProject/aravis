/* Aravis - Digital camera library
 *
 * Copyright © 2009-2019 Emmanuel Pacaud
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

#ifndef ARV_GC_REGISTER_NODE_H
#define ARV_GC_REGISTER_NODE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvtypes.h>
#include <arvgcfeaturenode.h>
#include <arvgcpropertynode.h>
#include <arvgcenums.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_REGISTER_NODE             (arv_gc_register_node_get_type ())
#define ARV_GC_REGISTER_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_REGISTER_NODE, ArvGcRegisterNode))
#define ARV_GC_REGISTER_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_REGISTER_NODE, ArvGcRegisterNodeClass))
#define ARV_IS_GC_REGISTER_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_REGISTER_NODE))
#define ARV_IS_GC_REGISTER_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_REGISTER_NODE))
#define ARV_GC_REGISTER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_REGISTER_NODE, ArvGcRegisterNodeClass))

typedef struct _ArvGcRegisterNodeClass ArvGcRegisterNodeClass;

struct _ArvGcRegisterNode {
	ArvGcFeatureNode	node;

	GSList *addresses;
	GSList *swiss_knives;
	ArvGcPropertyNode *index;
	ArvGcPropertyNode *length;
	ArvGcPropertyNode *port;
	ArvGcPropertyNode *access_mode;
	ArvGcPropertyNode *cachable;
	ArvGcPropertyNode *polling_time;
	ArvGcPropertyNode *endianess;

	GSList *invalidators;		/* #ArvGcPropertyNode */

	gboolean cached;
	GHashTable *caches;
	guint n_cache_hits;
	guint n_cache_misses;

	char v_string[G_ASCII_DTOSTR_BUF_SIZE];
};

struct _ArvGcRegisterNodeClass {
	ArvGcFeatureNodeClass parent_class;

	ArvGcCachable default_cachable;
};

GType 		arv_gc_register_node_get_type 			(void);
ArvGcNode * 	arv_gc_register_node_new 			(void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ArvGcRegisterNode, g_object_unref)

G_END_DECLS

#endif
