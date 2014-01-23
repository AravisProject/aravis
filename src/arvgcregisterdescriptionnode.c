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

/**
 * SECTION: arvgcregisterdescriptionnode
 * @short_description: Class for RegisterDescription nodes
 */

#include <arvgcregisterdescriptionnode.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_gc_register_description_node_get_node_name (ArvDomNode *node)
{
	return "RegisterDescription";
}

static void
arv_gc_register_description_node_set_attribute (ArvDomElement *self, const char* name, const char *value)
{
	ArvGcRegisterDescriptionNode *node = ARV_GC_REGISTER_DESCRIPTION_NODE (self);

	if (strcmp (name, "ModelName") == 0) {
		g_free (node->model_name);
		node->model_name = g_strdup (value);
	} else if (strcmp (name, "SchemaMajorVersion") == 0) {
		node->major_version = g_ascii_strtoll (value, NULL, 0);
	} else if (strcmp (name, "SchemaMinorVersion") == 0) {
		node->minor_version = g_ascii_strtoll (value, NULL, 0);
	} else if (strcmp (name, "SchemaSubMinorVersion") == 0) {
		node->subminor_version = g_ascii_strtoll (value, NULL, 0);
	} else
		ARV_DOM_ELEMENT_CLASS (parent_class)->set_attribute (self, name, value);
}

static const char *
arv_gc_register_description_node_get_attribute (ArvDomElement *self, const char *name)
{
	ArvGcRegisterDescriptionNode *node = ARV_GC_REGISTER_DESCRIPTION_NODE (self);

	if (strcmp (name, "ModelName") == 0)
		return node->model_name;
	else
		return ARV_DOM_ELEMENT_CLASS (parent_class)->get_attribute (self, name);
}

/* ArvGcRegisterDescriptionNode implementation */

gboolean
arv_gc_register_description_node_check_schema_version (ArvGcRegisterDescriptionNode *node,
						       guint required_major,
						       guint required_minor, 
						       guint required_subminor)
{
	g_return_val_if_fail (ARV_IS_GC_REGISTER_DESCRIPTION_NODE (node), FALSE);

	if (node->major_version < required_major)
		return FALSE;
	if (node->major_version > required_major)
		return TRUE;

	if (node->minor_version < required_minor)
		return FALSE;
	if (node->minor_version > required_minor)
		return FALSE;

	if (node->subminor_version < required_subminor)
		return FALSE;

	return TRUE;
}

ArvGcNode *
arv_gc_register_description_node_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_REGISTER_DESCRIPTION_NODE, NULL);

	return node;
}

static void
arv_gc_register_description_node_init (ArvGcRegisterDescriptionNode *gc_register_description_node)
{
	gc_register_description_node->major_version = 0;
	gc_register_description_node->minor_version = 0;
	gc_register_description_node->subminor_version = 0;
}

static void
arv_gc_register_description_node_finalize (GObject *object)
{
	ArvGcRegisterDescriptionNode *node = ARV_GC_REGISTER_DESCRIPTION_NODE (object);

	g_free (node->model_name);

	parent_class->finalize (object);
}

static void
arv_gc_register_description_node_class_init (ArvGcRegisterDescriptionNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvDomElementClass *dom_element_class = ARV_DOM_ELEMENT_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_register_description_node_finalize;
	dom_node_class->get_node_name = arv_gc_register_description_node_get_node_name;
	dom_element_class->set_attribute = arv_gc_register_description_node_set_attribute;
	dom_element_class->get_attribute = arv_gc_register_description_node_get_attribute;
}

/* ArvGcInteger interface implementation */

G_DEFINE_TYPE (ArvGcRegisterDescriptionNode, arv_gc_register_description_node, ARV_TYPE_GC_FEATURE_NODE)
