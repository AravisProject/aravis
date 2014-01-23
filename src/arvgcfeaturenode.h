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

#ifndef ARV_GC_FEATURE_NODE_H
#define ARV_GC_FEATURE_NODE_H

#include <arvtypes.h>
#include <arvgcnode.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_FEATURE_NODE             (arv_gc_feature_node_get_type ())
#define ARV_GC_FEATURE_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_FEATURE_NODE, ArvGcFeatureNode))
#define ARV_GC_FEATURE_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_FEATURE_NODE, ArvGcFeatureNodeClass))
#define ARV_IS_GC_FEATURE_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_FEATURE_NODE))
#define ARV_IS_GC_FEATURE_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_FEATURE_NODE))
#define ARV_GC_FEATURE_NODE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_FEATURE_NODE, ArvGcFeatureNodeClass))

typedef struct _ArvGcFeatureNodePrivate ArvGcFeatureNodePrivate;
typedef struct _ArvGcFeatureNodeClass ArvGcFeatureNodeClass;

struct _ArvGcFeatureNode {
	ArvGcNode base;

	ArvGcFeatureNodePrivate *priv;
};

struct _ArvGcFeatureNodeClass {
	ArvGcNodeClass parent_class;

	GType		(*get_value_type)		(ArvGcFeatureNode *gc_feature_node);

	void		(*set_value_from_string)	(ArvGcFeatureNode *gc_feature_node, const char *string, GError **error);
	const char *	(*get_value_as_string)		(ArvGcFeatureNode *gc_feature_node, GError **error);
};

GType 			arv_gc_feature_node_get_type 			(void);

ArvGcFeatureNode * 	arv_gc_feature_node_new 			(void);

const char *		arv_gc_feature_node_get_name			(ArvGcFeatureNode *gc_feature_node);

const char *		arv_gc_feature_node_get_tooltip			(ArvGcFeatureNode *gc_feature_node, GError **error);
const char *		arv_gc_feature_node_get_description		(ArvGcFeatureNode *gc_feature_node, GError **error);
const char *		arv_gc_feature_node_get_display_name		(ArvGcFeatureNode *gc_feature_node, GError **error);

gboolean		arv_gc_feature_node_is_available		(ArvGcFeatureNode *gc_feature_node, GError **error);
gboolean		arv_gc_feature_node_is_implemented		(ArvGcFeatureNode *gc_feature_node, GError **error);
gboolean		arv_gc_feature_node_is_locked			(ArvGcFeatureNode *gc_feature_node, GError **error);
void			arv_gc_feature_node_set_value_from_string	(ArvGcFeatureNode *gc_feature_node, const char *string,
									 GError **error);
const char *		arv_gc_feature_node_get_value_as_string		(ArvGcFeatureNode *gc_feature_node, GError **error);

GType 			arv_gc_feature_node_get_value_type 		(ArvGcFeatureNode *gc_feature_node);

void 			arv_gc_feature_node_inc_modification_count 	(ArvGcFeatureNode *gc_feature_node);
gint 			arv_gc_feature_node_get_modification_count 	(ArvGcFeatureNode *gc_feature_node);

G_END_DECLS

#endif
