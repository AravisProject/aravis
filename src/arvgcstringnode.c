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

/**
 * SECTION: arvgcstringnode
 * @short_description: Class for String nodes
 */

#include <arvgcstringnode.h>
#include <arvgcstring.h>
#include <arvgcinteger.h>
#include <arvgcvalueindexednode.h>
#include <arvgcfeaturenodeprivate.h>
#include <arvgcdefaultsprivate.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <arvstr.h>
#include <string.h>

struct _ArvGcStringNode {
	ArvGcFeatureNode	node;

	ArvGcPropertyNode *value;
};

struct _ArvGcStringNodeClass {
	ArvGcFeatureNodeClass parent_class;
};

static void arv_gc_string_node_string_interface_init (ArvGcStringInterface *interface);

G_DEFINE_TYPE_WITH_CODE (ArvGcStringNode, arv_gc_string_node, ARV_TYPE_GC_FEATURE_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_STRING, arv_gc_string_node_string_interface_init))

/* ArvDomNode implementation */

static const char *
arv_gc_string_node_get_node_name (ArvDomNode *node)
{
	return "String";
}

static void
arv_gc_string_node_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcStringNode *node = ARV_GC_STRING_NODE (self);

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_VALUE:
			case ARV_GC_PROPERTY_NODE_TYPE_P_VALUE:
				node->value = property_node;
				break;
			default:
				ARV_DOM_NODE_CLASS (arv_gc_string_node_parent_class)->post_new_child (self, child);
				break;
		}
	}
}

static void
arv_gc_string_node_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcStringNode implementation */

ArvGcNode *
arv_gc_string_node_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_STRING_NODE, NULL);

	return node;
}

static ArvGcFeatureNode *
arv_gc_string_node_get_linked_feature (ArvGcFeatureNode *gc_feature_node)
{
	ArvGcStringNode *gc_string_node = ARV_GC_STRING_NODE (gc_feature_node);
	ArvGcNode *pvalue_node = NULL;

	if (gc_string_node->value == NULL)
		return NULL;

	pvalue_node = arv_gc_property_node_get_linked_node (gc_string_node->value);
	if (ARV_IS_GC_FEATURE_NODE (pvalue_node))
		return ARV_GC_FEATURE_NODE (pvalue_node);

	return NULL;
}

static void
arv_gc_string_node_init (ArvGcStringNode *gc_string_node)
{
}

static void
arv_gc_string_node_finalize (GObject *object)
{
	G_OBJECT_CLASS (arv_gc_string_node_parent_class)->finalize (object);
}

static void
arv_gc_string_node_class_init (ArvGcStringNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);

	object_class->finalize = arv_gc_string_node_finalize;
	dom_node_class->get_node_name = arv_gc_string_node_get_node_name;
	dom_node_class->post_new_child = arv_gc_string_node_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_string_node_pre_remove_child;
	gc_feature_node_class->get_linked_feature = arv_gc_string_node_get_linked_feature;
        gc_feature_node_class->default_access_mode = ARV_GC_ACCESS_MODE_RW;
}

/* ArvGcString interface implementation */

static const char *
arv_gc_string_node_get_string_value (ArvGcString *gc_string, GError **error)
{
	ArvGcStringNode *gc_string_node = ARV_GC_STRING_NODE (gc_string);
	GError *local_error = NULL;
	const char *value = NULL;

	if (ARV_IS_GC_PROPERTY_NODE (gc_string_node->value)) {
		value = arv_gc_property_node_get_string (gc_string_node->value, &local_error);
		if (local_error != NULL) {
			g_propagate_prefixed_error (error, local_error, "[%s] ",
                                                    arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_string)));
			return NULL;
		}
	}

	return value;
}

static void
arv_gc_string_node_set_string_value (ArvGcString *gc_string, const char *value, GError **error)
{
	ArvGcStringNode *gc_string_node = ARV_GC_STRING_NODE (gc_string);
	GError *local_error = NULL;

	if (ARV_IS_GC_PROPERTY_NODE (gc_string_node->value)) {
		arv_gc_property_node_set_string (gc_string_node->value, value, &local_error);
                if (local_error != NULL)
                        g_propagate_prefixed_error (error, local_error, "[%s] ",
                                                    arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_string)));
        }
}

static gint64
arv_gc_string_node_get_max_length (ArvGcString *gc_string, GError **error)
{
	ArvGcStringNode *gc_string_node = ARV_GC_STRING_NODE (gc_string);

	if (ARV_IS_GC_PROPERTY_NODE (gc_string_node->value)) {
		ArvGcNode *linked_node = arv_gc_property_node_get_linked_node (gc_string_node->value);

		if (ARV_IS_GC_STRING (linked_node))
			return arv_gc_string_get_max_length (ARV_GC_STRING (linked_node), error);
	}

	return G_MAXINT64;
}

static void
arv_gc_string_node_string_interface_init (ArvGcStringInterface *interface)
{
	interface->get_value = arv_gc_string_node_get_string_value;
	interface->set_value = arv_gc_string_node_set_string_value;
	interface->get_max_length = arv_gc_string_node_get_max_length;
}
