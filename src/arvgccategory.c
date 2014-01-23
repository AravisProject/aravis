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
 * SECTION: arvgccategory
 * @short_description: Class for Category nodes
 */

#include <arvgccategory.h>
#include <arvgcpropertynode.h>
#include <arvgc.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_gc_category_get_node_name (ArvDomNode *node)
{
	return "Category";
}

static gboolean
arv_gc_category_can_append_child (ArvDomNode *parent, ArvDomNode *child)
{
	return ARV_IS_GC_PROPERTY_NODE (child) &&
		arv_gc_property_node_get_node_type (ARV_GC_PROPERTY_NODE (child)) == ARV_GC_PROPERTY_NODE_TYPE_P_FEATURE;
}

/* ArvGcCategory implementation */

static void
_free_features (ArvGcCategory *category)
{
	GSList *iter;

	for (iter = category->features; iter != NULL; iter = iter->next)
		g_free (iter->data);
	g_slist_free (category->features);
	category->features = NULL;
}

/**
 * arv_gc_category_get_features:
 * @category: a #ArvGcCategory
 *
 * Get a list of strings with the name of the features listed in the given category node.
 *
 * Returns: (element-type utf8) (transfer none): a list of strings.
 */

const GSList *
arv_gc_category_get_features (ArvGcCategory *category)
{
	ArvDomNode *iter;
	ArvGcNode *node;

	g_return_val_if_fail (ARV_IS_GC_CATEGORY (category), NULL);

	_free_features (category);

	for (iter = arv_dom_node_get_first_child (ARV_DOM_NODE (category));
	     iter != NULL;
	     iter = arv_dom_node_get_next_sibling (iter)) {
		node = arv_gc_property_node_get_linked_node (ARV_GC_PROPERTY_NODE (iter));
		if (ARV_IS_GC_FEATURE_NODE (node)) {
			char *name;

			name = g_strdup (arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (node)));
			category->features = g_slist_append (category->features, name);
		}
	}

	return category->features;
}

ArvGcNode *
arv_gc_category_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_CATEGORY, NULL);

	return node;
}

static void
arv_gc_category_init (ArvGcCategory *gc_category)
{
	gc_category->features = NULL;
}

static void
arv_gc_category_finalize (GObject *object)
{
	ArvGcCategory *category = ARV_GC_CATEGORY (object);

	_free_features (category);

	parent_class->finalize (object);
}

static void
arv_gc_category_class_init (ArvGcCategoryClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_category_finalize;
	dom_node_class->get_node_name = arv_gc_category_get_node_name;
	dom_node_class->can_append_child = arv_gc_category_can_append_child;
}

G_DEFINE_TYPE (ArvGcCategory, arv_gc_category, ARV_TYPE_GC_FEATURE_NODE)
