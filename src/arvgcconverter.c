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
 * SECTION: arvgcconverter
 * @short_description: Class for Converter and IntConverter nodes
 */

#include <arvgcfeaturenode.h>
#include <arvgcvariablenode.h>
#include <arvgcconverter.h>
#include <arvevaluator.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgc.h>
#include <arvdebug.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_gc_converter_get_node_name (ArvDomNode *node)
{
	ArvGcConverter *gc_converter = ARV_GC_CONVERTER (node);

	if (gc_converter->value_type == G_TYPE_DOUBLE)
		return "Converter";

	return "IntConverter";
}

static void
arv_gc_converter_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcConverter *node = ARV_GC_CONVERTER (self);

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_P_VARIABLE:
				node->variables = g_slist_prepend (node->variables, property_node);
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_VALUE:
				node->value = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_FORMULA_TO:
				node->formula_to_node = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_FORMULA_FROM:
				node->formula_from_node = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_EXPRESSION:
			case ARV_GC_PROPERTY_NODE_TYPE_CONSTANT:
				arv_warning_genicam ("[GcConverter::post_new_child] Constant and Expression not yet implemented");
				break;
			default:
				ARV_DOM_NODE_CLASS (parent_class)->post_new_child (self, child);
				break;
		}
	}
}

static void
arv_gc_converter_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcFeatureNode implementation */

static GType
arv_gc_converter_node_get_value_type (ArvGcFeatureNode *node)
{
	ArvGcConverter *gc_converter = ARV_GC_CONVERTER (node);

	return gc_converter->value_type;
}

/* ArvGcConverter implementation */

ArvGcNode *
arv_gc_converter_new (void)
{
	ArvGcConverter *converter;

	converter = g_object_new (ARV_TYPE_GC_CONVERTER, NULL);
	converter->value_type = G_TYPE_DOUBLE;

	return ARV_GC_NODE (converter);
}

ArvGcNode *
arv_gc_converter_new_integer (void)
{
	ArvGcConverter *converter;

	converter = g_object_new (ARV_TYPE_GC_CONVERTER, NULL);
	converter->value_type = G_TYPE_INT64;

	return ARV_GC_NODE (converter);
}

static void
arv_gc_converter_init (ArvGcConverter *gc_converter)
{
	gc_converter->formula_to = arv_evaluator_new (NULL);
	gc_converter->formula_from = arv_evaluator_new (NULL);
	gc_converter->value = NULL;
}

static void
arv_gc_converter_finalize (GObject *object)
{
	ArvGcConverter *gc_converter = ARV_GC_CONVERTER (object);

	g_slist_free (gc_converter->variables);

	g_object_unref (gc_converter->formula_to);
	g_object_unref (gc_converter->formula_from);

	parent_class->finalize (object);
}

static void
arv_gc_converter_class_init (ArvGcConverterClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_converter_finalize;
	dom_node_class->get_node_name = arv_gc_converter_get_node_name;
	dom_node_class->post_new_child = arv_gc_converter_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_converter_pre_remove_child;
	gc_feature_node_class->get_value_type = arv_gc_converter_node_get_value_type;
}

/* ArvGcInteger interface implementation */

static void
_update_from_variables (ArvGcConverter *gc_converter, GError **error)
{
	ArvGcNode *node = NULL;
	GError *local_error = NULL;
	GSList *iter;
	const char *expression;

	if (gc_converter->formula_from_node != NULL)
		expression = arv_gc_property_node_get_string (gc_converter->formula_from_node, &local_error);
	else
		expression = "";

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	arv_evaluator_set_expression (gc_converter->formula_from, expression);

	for (iter = gc_converter->variables; iter != NULL; iter = iter->next) {
		ArvGcVariableNode *variable_node = iter->data;

		node = arv_gc_property_node_get_linked_node (ARV_GC_PROPERTY_NODE (variable_node));
		if (arv_gc_feature_node_get_value_type (ARV_GC_FEATURE_NODE (node)) == G_TYPE_INT64) {
			gint64 value;

			value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			arv_evaluator_set_int64_variable (gc_converter->formula_from,
							  arv_gc_variable_node_get_name (variable_node),
							  value);
		} else if (arv_gc_feature_node_get_value_type (ARV_GC_FEATURE_NODE (node)) == G_TYPE_DOUBLE) {
			double value;

			value =  arv_gc_float_get_value (ARV_GC_FLOAT (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			arv_evaluator_set_double_variable (gc_converter->formula_from,
							  arv_gc_variable_node_get_name (variable_node),
							  value);
		}
	}

	if (gc_converter->value != NULL) {
		node = arv_gc_property_node_get_linked_node (gc_converter->value);

		if (arv_gc_feature_node_get_value_type (ARV_GC_FEATURE_NODE (node)) == G_TYPE_INT64) {
			gint64 value;

			value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			arv_evaluator_set_int64_variable (gc_converter->formula_from, "TO", value);
		} else if (arv_gc_feature_node_get_value_type (ARV_GC_FEATURE_NODE (node)) == G_TYPE_DOUBLE) {
			double value;

			value = arv_gc_float_get_value (ARV_GC_FLOAT (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			arv_evaluator_set_double_variable (gc_converter->formula_from, "TO", value);
		} else
			arv_warning_genicam ("[GcConverter::set_value] Invalid pValue node '%s'",
					     gc_converter->value);
	}
}

static void
_update_to_variables (ArvGcConverter *gc_converter, GError **error)
{
	ArvGcNode *node;
	GError *local_error = NULL;
	GSList *iter;
	const char *expression;

	if (gc_converter->formula_to_node != NULL)
		expression = arv_gc_property_node_get_string (gc_converter->formula_to_node, &local_error);
	else
		expression = "";

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	arv_evaluator_set_expression (gc_converter->formula_to, expression);

	for (iter = gc_converter->variables; iter != NULL; iter = iter->next) {
		ArvGcVariableNode *variable_node = iter->data;

		node = arv_gc_property_node_get_linked_node (ARV_GC_PROPERTY_NODE (variable_node));
		if (arv_gc_feature_node_get_value_type (ARV_GC_FEATURE_NODE (node)) == G_TYPE_INT64) {
			gint64 value;

			value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			arv_evaluator_set_int64_variable (gc_converter->formula_to,
							  arv_gc_variable_node_get_name (variable_node),
							  value);
		} else if (arv_gc_feature_node_get_value_type (ARV_GC_FEATURE_NODE (node)) == G_TYPE_DOUBLE) {
			double value;

			value =  arv_gc_float_get_value (ARV_GC_FLOAT (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			arv_evaluator_set_double_variable (gc_converter->formula_to,
							  arv_gc_variable_node_get_name (variable_node),
							  value);
		}
	}

	if (gc_converter->value != NULL) {
		node = arv_gc_property_node_get_linked_node (gc_converter->value);

		if (arv_gc_feature_node_get_value_type (ARV_GC_FEATURE_NODE (node)) == G_TYPE_INT64) {
			arv_gc_integer_set_value (ARV_GC_INTEGER (node),
						  arv_evaluator_evaluate_as_int64 (gc_converter->formula_to, NULL),
						  &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}
		} else if (arv_gc_feature_node_get_value_type (ARV_GC_FEATURE_NODE (node)) == G_TYPE_DOUBLE) {
			arv_gc_float_set_value (ARV_GC_FLOAT (node),
						arv_evaluator_evaluate_as_double (gc_converter->formula_to, NULL),
						&local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}
		} else
			arv_warning_genicam ("[GcConverter::set_value] Invalid pValue node '%s'",
					     gc_converter->value);
	}
}

static gint64
arv_gc_converter_get_integer_value (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcConverter *gc_converter = ARV_GC_CONVERTER (gc_integer);
	GError *local_error = NULL;

	_update_from_variables (gc_converter, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	return arv_evaluator_evaluate_as_int64 (gc_converter->formula_from, NULL);
}

static void
arv_gc_converter_set_integer_value (ArvGcInteger *gc_integer, gint64 value, GError **error)
{
	ArvGcConverter *gc_converter = ARV_GC_CONVERTER (gc_integer);

	arv_evaluator_set_int64_variable (gc_converter->formula_to,
					  "FROM", value);

	_update_to_variables (gc_converter, error);
}

static void
arv_gc_converter_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_converter_get_integer_value;
	interface->set_value = arv_gc_converter_set_integer_value;
}

static double
arv_gc_converter_get_float_value (ArvGcFloat *gc_float, GError **error)
{
	ArvGcConverter *gc_converter = ARV_GC_CONVERTER (gc_float);
	GError *local_error = NULL;

	_update_from_variables (gc_converter, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0.0;
	}

	return arv_evaluator_evaluate_as_double (gc_converter->formula_from, NULL);
}

static void
arv_gc_converter_set_float_value (ArvGcFloat *gc_float, double value, GError **error)
{
	ArvGcConverter *gc_converter = ARV_GC_CONVERTER (gc_float);

	arv_evaluator_set_double_variable (gc_converter->formula_to,
					  "FROM", value);

	_update_to_variables (gc_converter, error);
}

static void
arv_gc_converter_float_interface_init (ArvGcFloatInterface *interface)
{
	interface->get_value = arv_gc_converter_get_float_value;
	interface->set_value = arv_gc_converter_set_float_value;
}

G_DEFINE_TYPE_WITH_CODE (ArvGcConverter, arv_gc_converter, ARV_TYPE_GC_FEATURE_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_converter_integer_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_FLOAT,   arv_gc_converter_float_interface_init))
