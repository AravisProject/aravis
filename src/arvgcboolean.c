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

/**
 * SECTION: arvgcboolean
 * @short_description: Class for Boolean nodes
 */

#include <arvgcboolean.h>
#include <arvgcinteger.h>
#include <arvgcfeaturenodeprivate.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

struct _ArvGcBoolean {
	ArvGcFeatureNode	node;

	ArvGcPropertyNode *value;
	ArvGcPropertyNode *on_value;
	ArvGcPropertyNode *off_value;
};

struct _ArvGcBooleanClass {
	ArvGcFeatureNodeClass parent_class;
};

G_DEFINE_TYPE (ArvGcBoolean, arv_gc_boolean, ARV_TYPE_GC_FEATURE_NODE);

/* ArvDomNode implementation */

static const char *
arv_gc_boolean_get_node_name (ArvDomNode *node)
{
	return "Boolean";
}

static void
arv_gc_boolean_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcBoolean *node = ARV_GC_BOOLEAN (self);

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_VALUE:
			case ARV_GC_PROPERTY_NODE_TYPE_P_VALUE:
				node->value = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_ON_VALUE:
				node->on_value = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_OFF_VALUE:
				node->off_value = property_node;
				break;
			default:
				ARV_DOM_NODE_CLASS (parent_class)->post_new_child (self, child);
				break;
		}
	}
}

static void
arv_gc_boolean_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcBoolean implementation */

static gint64
arv_gc_boolean_get_on_value (ArvGcBoolean *gc_boolean, GError **error)
{
	gint64 on_value;
	GError *local_error = NULL;

	if (gc_boolean->on_value == NULL)
		return 1;

	on_value = arv_gc_property_node_get_int64 (gc_boolean->on_value, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 1;
	}

	return on_value;
}

static gint64
arv_gc_boolean_get_off_value (ArvGcBoolean *gc_boolean, GError **error)
{
	gint64 off_value;
	GError *local_error = NULL;

	if (gc_boolean->off_value == NULL)
		return 0;

	off_value = arv_gc_property_node_get_int64 (gc_boolean->off_value, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	return off_value;
}

/**
 * arv_gc_boolean_get_value:
 * @gc_boolean: a #ArvGcBoolean
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the feature value.
 *
 * Since: 0.8.0
 */

gboolean
arv_gc_boolean_get_value (ArvGcBoolean *gc_boolean, GError **error)
{
	gboolean value;
	gint64 on_value;
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_BOOLEAN (gc_boolean), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	if (gc_boolean->value == NULL)
		return FALSE;

	value = arv_gc_property_node_get_int64 (gc_boolean->value, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return FALSE;
	}

       	on_value = arv_gc_boolean_get_on_value (gc_boolean, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return FALSE;
	}

	return value == on_value;
}

/**
 * arv_gc_boolean_get_value_gi: (rename-to arv_gc_boolean_get_value)
 * @gc_boolean: a #ArvGcBoolean
 * @value: (out): feature value
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Get the feature value.
 *
 * Since: 0.8.0
 */

void
arv_gc_boolean_get_value_gi (ArvGcBoolean *gc_boolean, gboolean *value, GError **error)
{
	g_return_if_fail (value != NULL);

	*value = arv_gc_boolean_get_value (gc_boolean, error);
}

void
arv_gc_boolean_set_value (ArvGcBoolean *gc_boolean, gboolean v_boolean, GError **error)
{
	gboolean value;
	GError *local_error = NULL;

	g_return_if_fail (ARV_IS_GC_BOOLEAN (gc_boolean));
	g_return_if_fail (error == NULL || *error == NULL);

	if (v_boolean)
		value = arv_gc_boolean_get_on_value (gc_boolean, &local_error);
	else
		value = arv_gc_boolean_get_off_value (gc_boolean, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	arv_gc_feature_node_increment_change_count (ARV_GC_FEATURE_NODE (gc_boolean));
	arv_gc_property_node_set_int64 (gc_boolean->value, value, &local_error);

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

static ArvGcFeatureNode *
arv_gc_boolean_get_pointed_node (ArvGcFeatureNode *gc_feature_node)
{
	ArvGcBoolean *gc_boolean = ARV_GC_BOOLEAN (gc_feature_node);
	ArvGcNode *pvalue_node = NULL;

	if (gc_boolean->value == NULL)
		return NULL;

	pvalue_node = arv_gc_property_node_get_linked_node (gc_boolean->value);
	if (ARV_IS_GC_FEATURE_NODE (pvalue_node))
		return ARV_GC_FEATURE_NODE (pvalue_node);

	return NULL;
}

ArvGcNode *
arv_gc_boolean_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_BOOLEAN, NULL);

	return node;
}

static void
arv_gc_boolean_init (ArvGcBoolean *gc_boolean)
{
}

static void
arv_gc_boolean_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_gc_boolean_class_init (ArvGcBooleanClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_boolean_finalize;
	dom_node_class->get_node_name = arv_gc_boolean_get_node_name;
	dom_node_class->post_new_child = arv_gc_boolean_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_boolean_pre_remove_child;
	gc_feature_node_class->get_pointed_node = arv_gc_boolean_get_pointed_node;
}
