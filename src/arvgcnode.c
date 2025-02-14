/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

/**
 * SECTION: arvgcnode
 * @short_description: Base class for all Genicam nodes
 *
 * #ArvGcNode provides a base class for the implementation of the different
 * types of Genicam nodes.
 */

#include <arvgcnode.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <arvdebug.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

/* ArvDomElement implementation */

/* ArvGcNode implementation */

/**
 * arv_gc_node_get_genicam:
 * @gc_node: a #ArvGcNode
 *
 * Retrieves the parent genicam document of @gc_node.
 *
 * Return value: (transfer none): the parent #ArvGc
 */

ArvGc *
arv_gc_node_get_genicam	(ArvGcNode *gc_node)
{
	return ARV_GC (arv_dom_node_get_owner_document (ARV_DOM_NODE (gc_node)));
}

static void
arv_gc_node_init (ArvGcNode *gc_node)
{
}

static void
arv_gc_node_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_gc_node_class_init (ArvGcNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_node_finalize;
}

G_DEFINE_ABSTRACT_TYPE (ArvGcNode, arv_gc_node, ARV_TYPE_DOM_ELEMENT)
