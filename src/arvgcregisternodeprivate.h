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

#ifndef ARV_GC_REGISTER_NODE_PRIVATE_H
#define ARV_GC_REGISTER_NODE_PRIVATE_H

#include <arvgcregisternode.h>

gint64 		arv_gc_register_node_get_masked_integer_value 	(ArvGcRegisterNode *gc_register_node,
								 guint lsb, guint msb,
								 ArvGcSignedness signedness, guint endianness,
								 ArvGcCachable cachable,
								 gboolean is_masked, GError **error);
void 		arv_gc_register_node_set_masked_integer_value 	(ArvGcRegisterNode *gc_register_node,
								 guint lsb, guint msb,
								 ArvGcSignedness signedness, guint endianness,
								 ArvGcCachable cachable,
								 gboolean is_masked,
								 gint64 value, GError **error);
guint 		arv_gc_register_node_get_endianness 		(ArvGcRegisterNode *register_node);


#endif
