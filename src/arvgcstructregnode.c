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

#include <arvgcstructregnode.h>

static void arv_gc_struct_reg_node_init (ArvGcStructRegNode *self);

G_DEFINE_TYPE (ArvGcStructRegNode, arv_gc_struct_reg_node, ARV_TYPE_GC_REGISTER_NODE)

static const char *
arv_gc_struct_reg_node_get_node_name (ArvDomNode *node)
{
	return "StructReg";
}

/**
 * arv_gc_struct_reg_node_new:
 *
 * Returns: (transfer full): a new StructReg #ArvGcNode
 *
 * Since:0.8.0
 */

ArvGcNode *
arv_gc_struct_reg_node_new	(void)
{
	return g_object_new (ARV_TYPE_GC_STRUCT_REG_NODE, NULL);
}

static void
arv_gc_struct_reg_node_init (ArvGcStructRegNode *self)
{
}

static void
arv_gc_struct_reg_node_finalize (GObject *self)
{
	G_OBJECT_CLASS (arv_gc_struct_reg_node_parent_class)->finalize (self);
}

static void
arv_gc_struct_reg_node_class_init (ArvGcStructRegNodeClass *this_class)
{
	ArvGcRegisterNodeClass *gc_register_node_class = ARV_GC_REGISTER_NODE_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);

	object_class->finalize = arv_gc_struct_reg_node_finalize;
	dom_node_class->get_node_name = arv_gc_struct_reg_node_get_node_name;
	gc_register_node_class->default_cachable = ARV_GC_CACHABLE_NO_CACHE;
}
