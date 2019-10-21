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

#ifndef ARV_GC_FEATURE_NODE_H
#define ARV_GC_FEATURE_NODE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvtypes.h>
#include <arvgcnode.h>
#include <arvgcenums.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_FEATURE_NODE             (arv_gc_feature_node_get_type ())
G_DECLARE_DERIVABLE_TYPE (ArvGcFeatureNode, arv_gc_feature_node, ARV, GC_FEATURE_NODE, ArvGcNode)

struct _ArvGcFeatureNodeClass {
	ArvGcNodeClass parent_class;
};

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

G_END_DECLS

#endif
