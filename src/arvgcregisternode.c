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

#include <arvgcregisternode.h>

static GObjectClass *parent_class = NULL;

ArvGcNode *
arv_gc_register_node_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_REGISTER_NODE, NULL);

	return node;
}

static void
arv_gc_register_node_init (ArvGcRegisterNode *gc_register_node)
{
}

static void
arv_gc_register_node_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_gc_register_node_class_init (ArvGcRegisterNodeClass *register_node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (register_node_class);

	parent_class = g_type_class_peek_parent (register_node_class);

	object_class->finalize = arv_gc_register_node_finalize;
}

G_DEFINE_TYPE (ArvGcRegisterNode, arv_gc_register_node, ARV_TYPE_GC_NODE)
