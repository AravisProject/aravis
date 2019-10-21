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
 * SECTION: arvgcintegernode
 * @short_description: Class for Integer nodes
 */

#include <arvgcintegernode.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcselector.h>
#include <arvgcvalueindexednode.h>
#include <arvgcfeaturenodeprivate.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <string.h>
#include <stdlib.h>

struct _ArvGcIntegerNode {
	ArvGcFeatureNode	node;

	ArvGcPropertyNode *value;
	ArvGcPropertyNode *minimum;
	ArvGcPropertyNode *maximum;
	ArvGcPropertyNode *increment;
	ArvGcPropertyNode *unit;

	ArvGcPropertyNode *index;
	GSList *value_indexed_nodes;
	ArvGcPropertyNode *value_default;

	GSList *selecteds;		/* #ArvGcPropertyNode */
	GSList *selected_features;	/* #ArvGcFeatureNode */
};

struct _ArvGcIntegerNodeClass {
	ArvGcFeatureNodeClass parent_class;
};

static void arv_gc_integer_node_integer_interface_init (ArvGcIntegerInterface *interface);
static void arv_gc_integer_node_selector_interface_init (ArvGcSelectorInterface *interface);

G_DEFINE_TYPE_WITH_CODE (ArvGcIntegerNode, arv_gc_integer_node, ARV_TYPE_GC_FEATURE_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_integer_node_integer_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_SELECTOR, arv_gc_integer_node_selector_interface_init))

/* ArvDomNode implementation */

static const char *
arv_gc_integer_node_get_node_name (ArvDomNode *node)
{
	return "Integer";
}

static void
arv_gc_integer_node_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcIntegerNode *node = ARV_GC_INTEGER_NODE (self);

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_VALUE:
			case ARV_GC_PROPERTY_NODE_TYPE_P_VALUE:
				node->value = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_MINIMUM:
			case ARV_GC_PROPERTY_NODE_TYPE_P_MINIMUM:
				node->minimum = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_MAXIMUM:
			case ARV_GC_PROPERTY_NODE_TYPE_P_MAXIMUM:
				node->maximum = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_INCREMENT:
			case ARV_GC_PROPERTY_NODE_TYPE_P_INCREMENT:
				node->increment = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_UNIT:
				node->unit = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_INDEX:
				node->index = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_VALUE_INDEXED:
			case ARV_GC_PROPERTY_NODE_TYPE_P_VALUE_INDEXED:
				node->value_indexed_nodes = g_slist_prepend (node->value_indexed_nodes, property_node);
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_VALUE_DEFAULT:
			case ARV_GC_PROPERTY_NODE_TYPE_P_VALUE_DEFAULT:
				node->value_default = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_SELECTED:
				node->selecteds = g_slist_prepend (node->selecteds, property_node);
				break;
			default:
				ARV_DOM_NODE_CLASS (arv_gc_integer_node_parent_class)->post_new_child (self, child);
				break;
		}
	}
}

static void
arv_gc_integer_node_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcFeatureNode implementation */

static ArvGcPropertyNode *
_get_value_node (ArvGcIntegerNode *gc_integer_node, GError **error)
{
	GError *local_error = NULL;

	if (gc_integer_node->value != NULL)
		return gc_integer_node->value;

	if (gc_integer_node->index != NULL) {
		gint64 index;
		GSList *iter;

		index = arv_gc_property_node_get_int64 (ARV_GC_PROPERTY_NODE (gc_integer_node->index), &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return NULL;
		}

		for (iter = gc_integer_node->value_indexed_nodes; iter != NULL; iter = iter->next) {
			if (arv_gc_value_indexed_node_get_index (iter->data) == index)
				return iter->data;
		}

		if (gc_integer_node->value_default != NULL)
			return gc_integer_node->value_default;
	}

	return NULL;
}

/* ArvGcIntegerNode implementation */

ArvGcNode *
arv_gc_integer_node_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_INTEGER_NODE, NULL);

	return node;
}

static void
arv_gc_integer_node_init (ArvGcIntegerNode *gc_integer_node)
{
}

static void
arv_gc_integer_node_finalize (GObject *object)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (object);

	G_OBJECT_CLASS (arv_gc_integer_node_parent_class)->finalize (object);

	g_clear_pointer (&gc_integer_node->value_indexed_nodes, g_slist_free);
	g_clear_pointer (&gc_integer_node->selecteds, g_slist_free);
	g_clear_pointer (&gc_integer_node->selected_features, g_slist_free);
}

static void
arv_gc_integer_node_class_init (ArvGcIntegerNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);

	object_class->finalize = arv_gc_integer_node_finalize;
	dom_node_class->get_node_name = arv_gc_integer_node_get_node_name;
	dom_node_class->post_new_child = arv_gc_integer_node_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_integer_node_pre_remove_child;
}

/* ArvGcInteger interface implementation */

static gint64
arv_gc_integer_node_get_integer_value (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	ArvGcPropertyNode *value_node;
	GError *local_error = NULL;
	gint64 value;

	value_node = _get_value_node (gc_integer_node, error);
	if (value_node == NULL)
		return 0;

	value = arv_gc_property_node_get_int64 (ARV_GC_PROPERTY_NODE (value_node), &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	return value;
}

static void
arv_gc_integer_node_set_integer_value (ArvGcInteger *gc_integer, gint64 value, GError **error)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	ArvGcPropertyNode *value_node;
	GError *local_error = NULL;

	value_node = _get_value_node (gc_integer_node, error);
	if (value_node == NULL)
		return;

	arv_gc_feature_node_increment_change_count (ARV_GC_FEATURE_NODE (gc_integer_node));
	arv_gc_property_node_set_int64 (ARV_GC_PROPERTY_NODE (value_node), value, &local_error);
	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

static gint64
arv_gc_integer_node_get_min (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	GError *local_error = NULL;
	gint64 value;

	if (gc_integer_node->minimum == NULL) {
		ArvGcPropertyNode *value_node;

		value_node = _get_value_node (gc_integer_node, &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return G_MININT64;
		}

		if (ARV_IS_GC_PROPERTY_NODE (value_node)) {
			ArvGcNode *linked_node = arv_gc_property_node_get_linked_node (value_node);

			if (ARV_IS_GC_INTEGER (linked_node))
				return arv_gc_integer_get_min (ARV_GC_INTEGER (linked_node), error);
			else if (ARV_IS_GC_FLOAT (linked_node))
				return arv_gc_float_get_min (ARV_GC_FLOAT (linked_node), error);
		}

		return G_MININT64;
	}

	value = arv_gc_property_node_get_int64 (ARV_GC_PROPERTY_NODE (gc_integer_node->minimum), &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_MININT64;
	}

	return value;
}

static gint64
arv_gc_integer_node_get_max (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	GError *local_error = NULL;
	gint64 value;

	if (gc_integer_node->maximum == NULL) {
		ArvGcPropertyNode *value_node;

		value_node = _get_value_node (gc_integer_node, &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return G_MAXINT64;
		}

		if (ARV_IS_GC_PROPERTY_NODE (value_node)) {
			ArvGcNode *linked_node = arv_gc_property_node_get_linked_node (value_node);

			if (ARV_IS_GC_INTEGER (linked_node))
				return arv_gc_integer_get_max (ARV_GC_INTEGER (linked_node), error);
			else if (ARV_IS_GC_FLOAT (linked_node))
				return arv_gc_float_get_max (ARV_GC_FLOAT (linked_node), error);
		}

		return G_MAXINT64;
	}

	value = arv_gc_property_node_get_int64 (ARV_GC_PROPERTY_NODE (gc_integer_node->maximum), &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_MAXINT64;
	}

	return value;
}

static gint64
arv_gc_integer_node_get_inc (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	GError *local_error = NULL;
	gint64 value;

	if (gc_integer_node->increment == NULL) {
		ArvGcPropertyNode *value_node;

		value_node = _get_value_node (gc_integer_node, &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return 1;
		}

		if (ARV_IS_GC_PROPERTY_NODE (value_node)) {
			ArvGcNode *linked_node = arv_gc_property_node_get_linked_node (value_node);

			if (ARV_IS_GC_INTEGER (linked_node))
				return arv_gc_integer_get_inc (ARV_GC_INTEGER (linked_node), error);
			else if (ARV_IS_GC_FLOAT (linked_node))
				return arv_gc_float_get_inc (ARV_GC_FLOAT (linked_node), error);
		}

		return 1;
	}

	value = arv_gc_property_node_get_int64 (ARV_GC_PROPERTY_NODE (gc_integer_node->increment), &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 1;
	}

	return value;
}

static const char *
arv_gc_integer_node_get_unit (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	GError *local_error = NULL;
	const char *string;

	if (gc_integer_node->unit == NULL)
		return NULL;

	string = arv_gc_property_node_get_string (ARV_GC_PROPERTY_NODE (gc_integer_node->unit), &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return NULL;
	}

	return string;
}

static void
arv_gc_integer_node_impose_min (ArvGcInteger *gc_integer, gint64 minimum, GError **error)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	GError *local_error = NULL;

	if (gc_integer_node->minimum == NULL)
		return;

	arv_gc_property_node_set_int64 (ARV_GC_PROPERTY_NODE (gc_integer_node->minimum), minimum, &local_error);

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

static void
arv_gc_integer_node_impose_max (ArvGcInteger *gc_integer, gint64 maximum, GError **error)
{
	ArvGcIntegerNode *gc_integer_node = ARV_GC_INTEGER_NODE (gc_integer);
	GError *local_error = NULL;

	if (gc_integer_node->maximum == NULL)
		return;

	arv_gc_property_node_set_int64 (ARV_GC_PROPERTY_NODE (gc_integer_node->maximum), maximum, &local_error);

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

static void
arv_gc_integer_node_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_integer_node_get_integer_value;
	interface->set_value = arv_gc_integer_node_set_integer_value;
	interface->get_min = arv_gc_integer_node_get_min;
	interface->get_max = arv_gc_integer_node_get_max;
	interface->get_inc = arv_gc_integer_node_get_inc;
	interface->get_unit = arv_gc_integer_node_get_unit;
	interface->impose_min = arv_gc_integer_node_impose_min;
	interface->impose_max = arv_gc_integer_node_impose_max;
}

const GSList *
arv_gc_integer_node_get_selected_features (ArvGcSelector *selector)
{
	ArvGcIntegerNode *integer = ARV_GC_INTEGER_NODE (selector);
	GSList *iter;

	g_clear_pointer (&integer->selected_features, g_slist_free);
	for (iter = integer->selecteds; iter != NULL; iter = iter->next) {
		ArvGcFeatureNode *feature_node = ARV_GC_FEATURE_NODE (arv_gc_property_node_get_linked_node (iter->data));
		if (ARV_IS_GC_FEATURE_NODE (feature_node))
			integer->selected_features = g_slist_prepend (integer->selected_features, feature_node);
	}

	return integer->selected_features;
}

static void
arv_gc_integer_node_selector_interface_init (ArvGcSelectorInterface *interface)
{
	interface->get_selected_features = arv_gc_integer_node_get_selected_features;
}
