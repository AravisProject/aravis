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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvgcregisterdescriptionnode
 * @short_description: Class for Register Description elements
 */

#include <arvgcregisterdescriptionnode.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_gc_register_description_node_get_node_name (ArvDomNode *node)
{
	return "RegisterDescription";
}

/* ArvGcRegisterDescriptionNode implementation */

ArvDomElement *
arv_gc_register_description_node_new (void)
{
	ArvDomElement *element;

	element = g_object_new (ARV_TYPE_GC_REGISTER_DESCRIPTION_NODE, NULL);

	return element;
}

static void
arv_gc_register_description_node_init (ArvGcRegisterDescriptionNode *gc_register_description_node)
{
}

static void
arv_gc_register_description_node_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_gc_register_description_node_class_init (ArvGcRegisterDescriptionNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_register_description_node_finalize;
	dom_node_class->get_node_name = arv_gc_register_description_node_get_node_name;
}

/* ArvGcInteger interface implementation */

G_DEFINE_TYPE (ArvGcRegisterDescriptionNode, arv_gc_register_description_node, ARV_TYPE_GC_NODE)
