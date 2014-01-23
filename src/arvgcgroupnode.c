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
 * SECTION: arvgcgroupnode
 * @short_description: Class for Group nodes
 */

#include <arvgcgroupnode.h>
#include <arvgc.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_gc_group_node_get_node_name (ArvDomNode *node)
{
	return "Group";
}

/* ArvGcFeatureNode implementation */

static void
arv_gc_group_node_set_attribute (ArvDomElement *self, const char* name, const char *value)
{
	ArvGcGroupNode *node = ARV_GC_GROUP_NODE (self);

	if (strcmp (name, "Comment") == 0) {
		g_free (node->comment);
		node->comment = g_strdup (value);
	}
}

static const char *
arv_gc_group_node_get_attribute (ArvDomElement *self, const char *name)
{
	ArvGcGroupNode *node = ARV_GC_GROUP_NODE (self);

	if (strcmp (name, "ModelName") == 0)
		return node->comment;

	return NULL;
}

/* ArvGcGroupNode implementation */

ArvGcNode *
arv_gc_group_node_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_GROUP_NODE, NULL);

	return node;
}

static void
arv_gc_group_node_init (ArvGcGroupNode *gc_group_node)
{
}

static void
arv_gc_group_node_finalize (GObject *object)
{
	ArvGcGroupNode *group_node = ARV_GC_GROUP_NODE (object);

	g_free (group_node->comment);

	parent_class->finalize (object);
}

static void
arv_gc_group_node_class_init (ArvGcGroupNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvDomElementClass *dom_element_class = ARV_DOM_ELEMENT_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_group_node_finalize;
	dom_node_class->get_node_name = arv_gc_group_node_get_node_name;
	dom_element_class->set_attribute = arv_gc_group_node_set_attribute;
	dom_element_class->get_attribute = arv_gc_group_node_get_attribute;
}

G_DEFINE_TYPE (ArvGcGroupNode, arv_gc_group_node, ARV_TYPE_GC_FEATURE_NODE)
