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
 * SECTION: arvgcfloatnode
 * @short_description: Class for Float nodes
 */

#include <arvgcfloatnode.h>
#include <arvgcfloat.h>
#include <arvgcinteger.h>
#include <arvgcvalueindexednode.h>
#include <arvgcfeaturenodeprivate.h>
#include <arvgcdefaultsprivate.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <arvstr.h>
#include <string.h>

struct _ArvGcFloatNode {
	ArvGcFeatureNode	node;

	ArvGcPropertyNode *value;
	ArvGcPropertyNode *minimum;
	ArvGcPropertyNode *maximum;
	ArvGcPropertyNode *increment;
	ArvGcPropertyNode *unit;
	ArvGcPropertyNode *representation;
	ArvGcPropertyNode *display_notation;
	ArvGcPropertyNode *display_precision;

	ArvGcPropertyNode *index;
	GSList *value_indexed_nodes;
	ArvGcPropertyNode *value_default;
};

struct _ArvGcFloatNodeClass {
	ArvGcFeatureNodeClass parent_class;
};

static void arv_gc_float_node_float_interface_init (ArvGcFloatInterface *interface);

G_DEFINE_TYPE_WITH_CODE (ArvGcFloatNode, arv_gc_float_node, ARV_TYPE_GC_FEATURE_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_FLOAT, arv_gc_float_node_float_interface_init))

/* ArvDomNode implementation */

static const char *
arv_gc_float_node_get_node_name (ArvDomNode *node)
{
	return "Float";
}

static void
arv_gc_float_node_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcFloatNode *node = ARV_GC_FLOAT_NODE (self);

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
			case ARV_GC_PROPERTY_NODE_TYPE_REPRESENTATION:
				node->representation = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NOTATION:
				node->display_notation = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_PRECISION:
				node->display_precision = property_node;
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
			default:
				ARV_DOM_NODE_CLASS (arv_gc_float_node_parent_class)->post_new_child (self, child);
				break;
		}
	}
}

static void
arv_gc_float_node_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcFeatureNode implementation */

static ArvGcPropertyNode *
_get_value_node (ArvGcFloatNode *gc_float_node, GError **error)
{
	GError *local_error = NULL;

	if (gc_float_node->value != NULL)
		return gc_float_node->value;

	if (gc_float_node->index != NULL) {
		gint64 index;
		GSList *iter;

		index = arv_gc_property_node_get_int64 (ARV_GC_PROPERTY_NODE (gc_float_node->index), &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return NULL;
		}

		for (iter = gc_float_node->value_indexed_nodes; iter != NULL; iter = iter->next)
			if (arv_gc_value_indexed_node_get_index (iter->data) == index)
				return iter->data;

		if (gc_float_node->value_default != NULL)
			return gc_float_node->value_default;
	}

	return NULL;
}

/* ArvGcFloatNode implementation */

ArvGcNode *
arv_gc_float_node_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_FLOAT_NODE, NULL);

	return node;
}

static ArvGcFeatureNode *
arv_gc_float_node_get_pointed_node (ArvGcFeatureNode *gc_feature_node)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_feature_node);
	ArvGcNode *pvalue_node = NULL;

	if (gc_float_node->value == NULL)
		return NULL;

	pvalue_node = arv_gc_property_node_get_linked_node (gc_float_node->value);
	if (ARV_IS_GC_FEATURE_NODE (pvalue_node))
		return ARV_GC_FEATURE_NODE (pvalue_node);

	return NULL;
}

static void
arv_gc_float_node_init (ArvGcFloatNode *gc_float_node)
{
}

static void
arv_gc_float_node_finalize (GObject *object)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (object);

	G_OBJECT_CLASS (arv_gc_float_node_parent_class)->finalize (object);

	g_slist_free (gc_float_node->value_indexed_nodes);
}

static void
arv_gc_float_node_class_init (ArvGcFloatNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);

	object_class->finalize = arv_gc_float_node_finalize;
	dom_node_class->get_node_name = arv_gc_float_node_get_node_name;
	dom_node_class->post_new_child = arv_gc_float_node_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_float_node_pre_remove_child;
	gc_feature_node_class->get_pointed_node = arv_gc_float_node_get_pointed_node;
}

/* ArvGcFloat interface implementation */

static double
arv_gc_float_node_get_float_value (ArvGcFloat *gc_float, GError **error)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	ArvGcPropertyNode *value_node;
	GError *local_error = NULL;
	double value;

	value_node = _get_value_node (gc_float_node, error);
	if (value_node == NULL)
		return 0.0;

	value = arv_gc_property_node_get_double (ARV_GC_PROPERTY_NODE (value_node), &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0.0;
	}

	return value;
}

static void
arv_gc_float_node_set_float_value (ArvGcFloat *gc_float, double value, GError **error)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	ArvGcPropertyNode *value_node;
	GError *local_error = NULL;

	value_node = _get_value_node (gc_float_node, error);
	if (value_node == NULL)
		return;

	arv_gc_feature_node_increment_change_count (ARV_GC_FEATURE_NODE (gc_float));
	arv_gc_property_node_set_double (ARV_GC_PROPERTY_NODE (value_node), value, &local_error);
	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

static double
arv_gc_float_node_get_min (ArvGcFloat *gc_float, GError **error)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	GError *local_error = NULL;
	double value;

	if (gc_float_node->minimum == NULL) {
		ArvGcPropertyNode *value_node;

		value_node = _get_value_node (gc_float_node, &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return -G_MAXDOUBLE;
		}

		if (ARV_IS_GC_PROPERTY_NODE (value_node)) {
			ArvGcNode *linked_node = arv_gc_property_node_get_linked_node (value_node);

			if (ARV_IS_GC_INTEGER (linked_node))
				return arv_gc_integer_get_min (ARV_GC_INTEGER (linked_node), error);
			else if (ARV_IS_GC_FLOAT (linked_node))
				return arv_gc_float_get_min (ARV_GC_FLOAT (linked_node), error);
		}
		return -G_MAXDOUBLE;
	}

	value = arv_gc_property_node_get_double (ARV_GC_PROPERTY_NODE (gc_float_node->minimum), &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return -G_MAXDOUBLE;
	}

	return value;
}

static double
arv_gc_float_node_get_max (ArvGcFloat *gc_float, GError **error)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	GError *local_error = NULL;
	double value;

	if (gc_float_node->maximum == NULL) {
		ArvGcPropertyNode *value_node;

		value_node = _get_value_node (gc_float_node, &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return G_MAXDOUBLE;
		}

		if (ARV_IS_GC_PROPERTY_NODE (value_node)) {
			ArvGcNode *linked_node = arv_gc_property_node_get_linked_node (value_node);

			if (ARV_IS_GC_INTEGER (linked_node))
				return arv_gc_integer_get_max (ARV_GC_INTEGER (linked_node), error);
			else if (ARV_IS_GC_FLOAT (linked_node))
				return arv_gc_float_get_max (ARV_GC_FLOAT (linked_node), error);
		}
		return G_MAXDOUBLE;
	}

	value = arv_gc_property_node_get_double (ARV_GC_PROPERTY_NODE (gc_float_node->maximum), &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_MAXDOUBLE;
	}

	return value;
}

static double
arv_gc_float_node_get_inc (ArvGcFloat *gc_float, GError **error)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	GError *local_error = NULL;
	double value;

	if (gc_float_node->increment == NULL) {
		ArvGcPropertyNode *value_node;

		value_node = _get_value_node (gc_float_node, &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return 1.0;
		}

		if (ARV_IS_GC_PROPERTY_NODE (value_node)) {
			ArvGcNode *linked_node = arv_gc_property_node_get_linked_node (value_node);

			if (ARV_IS_GC_INTEGER (linked_node))
				return arv_gc_integer_get_inc (ARV_GC_INTEGER (linked_node), error);
			else if (ARV_IS_GC_FLOAT (linked_node))
				return arv_gc_float_get_inc (ARV_GC_FLOAT (linked_node), error);
		}

		return 1.0;
	}

	value = arv_gc_property_node_get_double (ARV_GC_PROPERTY_NODE (gc_float_node->increment), &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 1.0;
	}

	return value;
}

static ArvGcRepresentation
arv_gc_float_node_get_representation (ArvGcFloat *gc_float)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);

	if (gc_float_node->representation == NULL)
		return ARV_GC_REPRESENTATION_UNDEFINED;

	return arv_gc_property_node_get_representation (ARV_GC_PROPERTY_NODE (gc_float_node->representation), ARV_GC_REPRESENTATION_UNDEFINED);
}

static const char *
arv_gc_float_node_get_unit (ArvGcFloat *gc_float)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	const char *string;

	if (gc_float_node->unit == NULL)
		return NULL;

	string = arv_gc_property_node_get_string (ARV_GC_PROPERTY_NODE (gc_float_node->unit), NULL);

	return string;
}

static ArvGcDisplayNotation
arv_gc_float_node_get_display_notation (ArvGcFloat *gc_float)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);

	if (gc_float_node->display_notation == NULL)
		return ARV_GC_DISPLAY_NOTATION_DEFAULT;

	return arv_gc_property_node_get_display_notation (ARV_GC_PROPERTY_NODE (gc_float_node->display_notation), ARV_GC_DISPLAY_NOTATION_DEFAULT);
}

static gint64
arv_gc_float_node_get_display_precision (ArvGcFloat *gc_float)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);

	if (gc_float_node->display_precision == NULL)
		return ARV_GC_DISPLAY_PRECISION_DEFAULT;

	return arv_gc_property_node_get_display_precision (ARV_GC_PROPERTY_NODE (gc_float_node->display_precision), ARV_GC_DISPLAY_PRECISION_DEFAULT);
}

static void
arv_gc_float_node_impose_min (ArvGcFloat *gc_float, double minimum, GError **error)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	GError *local_error = NULL;

	if (gc_float_node->minimum == NULL)
		return;

	arv_gc_property_node_set_double (ARV_GC_PROPERTY_NODE (gc_float_node->minimum), minimum, &local_error);

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

static void
arv_gc_float_node_impose_max (ArvGcFloat *gc_float, double maximum, GError **error)
{
	ArvGcFloatNode *gc_float_node = ARV_GC_FLOAT_NODE (gc_float);
	GError *local_error = NULL;

	if (gc_float_node->maximum == NULL)
		return;

	arv_gc_property_node_set_double (ARV_GC_PROPERTY_NODE (gc_float_node->maximum), maximum, &local_error);

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

static void
arv_gc_float_node_float_interface_init (ArvGcFloatInterface *interface)
{
	interface->get_value = arv_gc_float_node_get_float_value;
	interface->set_value = arv_gc_float_node_set_float_value;
	interface->get_min = arv_gc_float_node_get_min;
	interface->get_max = arv_gc_float_node_get_max;
	interface->get_inc = arv_gc_float_node_get_inc;
	interface->get_representation = arv_gc_float_node_get_representation;
	interface->get_unit = arv_gc_float_node_get_unit;
	interface->get_display_notation = arv_gc_float_node_get_display_notation;
	interface->get_display_precision = arv_gc_float_node_get_display_precision;
	interface->impose_min = arv_gc_float_node_impose_min;
	interface->impose_max = arv_gc_float_node_impose_max;
}
