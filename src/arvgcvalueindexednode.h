/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_GC_VALUE_INDEXED_NODE_H
#define ARV_GC_VALUE_INDEXED_NODE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvgcpropertynode.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_VALUE_INDEXED_NODE (arv_gc_value_indexed_node_get_type ())
ARV_API G_DECLARE_FINAL_TYPE (ArvGcValueIndexedNode, arv_gc_value_indexed_node, ARV, GC_VALUE_INDEXED_NODE, ArvGcPropertyNode)

ARV_API ArvGcNode *	arv_gc_value_indexed_node_new		(void);
ARV_API ArvGcNode *	arv_gc_p_value_indexed_node_new		(void);
ARV_API gint64		arv_gc_value_indexed_node_get_index	(ArvGcValueIndexedNode *value_indexed_node);

G_END_DECLS

#endif
