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


#ifndef ARV_GC_FEATURE_NODE_PRIVATE_H
#define ARV_GC_FEATURE_NODE_PRIVATE_H

#include <arvgcfeaturenode.h>
#include <arvgc.h>
#include <arvgcenums.h>

G_BEGIN_DECLS

void			arv_gc_feature_node_increment_change_count	(ArvGcFeatureNode *gc_feature_node);
guint64 		arv_gc_feature_node_get_change_count 		(ArvGcFeatureNode *gc_feature_node);

static inline gboolean
arv_gc_feature_node_check_write_access (ArvGcFeatureNode *gc_feature_node, GError **error)
{
	ArvGc *genicam;
        ArvAccessCheckPolicy policy;

        g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node), FALSE);
	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_feature_node));
	g_return_val_if_fail (ARV_IS_GC (genicam), FALSE);

        policy = arv_gc_get_access_check_policy (genicam);
        if (policy != ARV_ACCESS_CHECK_POLICY_ENABLE)
                return TRUE;

        if (arv_gc_feature_node_get_actual_access_mode (gc_feature_node) != ARV_GC_ACCESS_MODE_RO)
                return TRUE;

        g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_READ_ONLY, "[%s] Write error on read only feature",
                     arv_gc_feature_node_get_name (gc_feature_node));

        return FALSE;
}

static inline gboolean
arv_gc_feature_node_check_read_access (ArvGcFeatureNode *gc_feature_node, GError **error)
{
	ArvGc *genicam;
        ArvAccessCheckPolicy policy;

        g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node), FALSE);
	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_feature_node));
	g_return_val_if_fail (ARV_IS_GC (genicam), FALSE);

        policy = arv_gc_get_access_check_policy (genicam);
        if (policy != ARV_ACCESS_CHECK_POLICY_ENABLE)
                return TRUE;

        if (arv_gc_feature_node_get_actual_access_mode (gc_feature_node) != ARV_GC_ACCESS_MODE_WO)
                return TRUE;

        g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_READ_ONLY, "[%s] Read error on write only feature",
                     arv_gc_feature_node_get_name (gc_feature_node));

        return FALSE;
}

G_END_DECLS

#endif
