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

/**
 * SECTION: arvgccategory
 * @short_description: Class for Category nodes
 */

#include <arvgccategory.h>
#include <arvgc.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvGcNode implementation */

static const char *
arv_gc_category_get_node_name (ArvGcNode *node)
{
	return "Category";
}

static void
arv_gc_category_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	ArvGcCategory *category = ARV_GC_CATEGORY (node);

	if (strcmp (name, "pFeature") == 0) {
		category->features = g_slist_append (category->features, g_strdup (content));
	} else
		ARV_GC_NODE_CLASS (parent_class)->add_element (node, name, content, attributes);
}

/* ArvGcCategory implementation */

/**
 * arv_gc_category_get_features:
 * @category: a #ArvGcCategory
 *
 * Get a list of strings with the name of the features listed in the given category node.
 *
 * Returns: a list of strings.
 */

const GSList *
arv_gc_category_get_features (ArvGcCategory *category)
{
	g_return_val_if_fail (ARV_IS_GC_CATEGORY (category), NULL);

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
	ArvGcCategory *gc_category = ARV_GC_CATEGORY (object);
	GSList *iter;

	for (iter = gc_category->features; iter != NULL; iter = iter->next)
		g_free (iter->data);
	g_slist_free (gc_category->features);

	parent_class->finalize (object);
}

static void
arv_gc_category_class_init (ArvGcCategoryClass *category_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (category_class);
	ArvGcNodeClass *node_class = ARV_GC_NODE_CLASS (category_class);

	parent_class = g_type_class_peek_parent (category_class);

	object_class->finalize = arv_gc_category_finalize;

	node_class->get_node_name = arv_gc_category_get_node_name;
	node_class->add_element = arv_gc_category_add_element;
}

G_DEFINE_TYPE (ArvGcCategory, arv_gc_category, ARV_TYPE_GC_NODE)
