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
 * SECTION: arvgcconverter
 * @short_description: Class for Converter and IntConverter nodes
 */

#include <arvgcfeaturenodeprivate.h>
#include <arvgcconverterprivate.h>
#include <arvevaluator.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcdefaultsprivate.h>
#include <arvgc.h>
#include <arvdebugprivate.h>
#include <string.h>

typedef struct {
	GSList *variables;	/* ArvGcVariableNode list */
	GSList *constants;	/* ArvGcVariableNode list */
	GSList *expressions;	/* ArvGcVariableNode list */

	ArvGcPropertyNode *value;
	ArvGcPropertyNode *formula_to_node;
	ArvGcPropertyNode *formula_from_node;
	ArvGcPropertyNode *unit;
	ArvGcPropertyNode *representation;
	ArvGcPropertyNode *display_notation;
	ArvGcPropertyNode *display_precision;
	ArvGcPropertyNode *is_linear;
	ArvGcPropertyNode *slope;

	ArvEvaluator *formula_to;
	ArvEvaluator *formula_from;
} ArvGcConverterPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ArvGcConverter, arv_gc_converter, ARV_TYPE_GC_FEATURE_NODE,
				  G_ADD_PRIVATE (ArvGcConverter))

/* ArvDomNode implementation */

static void
arv_gc_converter_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (ARV_GC_CONVERTER (self));

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_P_VARIABLE:
				priv->variables = g_slist_prepend (priv->variables, property_node);
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_VALUE:
				priv->value = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_FORMULA_TO:
				priv->formula_to_node = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_FORMULA_FROM:
				priv->formula_from_node = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_EXPRESSION:
				priv->expressions = g_slist_prepend (priv->expressions, property_node);
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_CONSTANT:
				priv->constants = g_slist_prepend (priv->constants, property_node);
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_UNIT:
				priv->unit = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_REPRESENTATION:
				priv->representation = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NOTATION:
				priv->display_notation = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_PRECISION:
				priv->display_precision = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_IS_LINEAR:
				priv->is_linear = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_SLOPE:
				priv->slope = property_node;
				break;
			default:
				ARV_DOM_NODE_CLASS (arv_gc_converter_parent_class)->post_new_child (self, child);
				break;
		}
	}
}

static void
arv_gc_converter_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcConverter implementation */

static void
arv_gc_converter_init (ArvGcConverter *self)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (ARV_GC_CONVERTER (self));

	priv->formula_to = arv_evaluator_new (NULL);
	priv->formula_from = arv_evaluator_new (NULL);
	priv->value = NULL;
}

static ArvGcFeatureNode *
arv_gc_converter_get_linked_feature (ArvGcFeatureNode *gc_feature_node)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (ARV_GC_CONVERTER (gc_feature_node));
	ArvGcNode *pvalue_node = NULL;

	if (priv->value == NULL)
		return NULL;

	pvalue_node = arv_gc_property_node_get_linked_node (priv->value);
	if (ARV_IS_GC_FEATURE_NODE (pvalue_node))
		return ARV_GC_FEATURE_NODE (pvalue_node);

	return NULL;
}

static void
arv_gc_converter_finalize (GObject *object)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (ARV_GC_CONVERTER (object));

	g_slist_free (priv->variables);
	g_slist_free (priv->expressions);
	g_slist_free (priv->constants);

	g_object_unref (priv->formula_to);
	g_object_unref (priv->formula_from);

	G_OBJECT_CLASS (arv_gc_converter_parent_class)->finalize (object);
}

static void
arv_gc_converter_class_init (ArvGcConverterClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);

	object_class->finalize = arv_gc_converter_finalize;
	dom_node_class->post_new_child = arv_gc_converter_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_converter_pre_remove_child;
	gc_feature_node_class->get_linked_feature = arv_gc_converter_get_linked_feature;
}

/* ArvGcInteger interface implementation */

ArvGcIsLinear
arv_gc_converter_get_is_linear (ArvGcConverter *gc_converter, GError **error)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (gc_converter);
	const char *string;

	g_return_val_if_fail (ARV_IS_GC_CONVERTER (gc_converter), ARV_GC_IS_LINEAR_NO);

	if (priv->is_linear == NULL)
		return ARV_GC_IS_LINEAR_NO;

	string = arv_gc_property_node_get_string (ARV_GC_PROPERTY_NODE (priv->is_linear), error);

	if (g_strcmp0 ("Yes", string) == 0)
		return ARV_GC_IS_LINEAR_YES;

	return ARV_GC_IS_LINEAR_NO;
}

gboolean
arv_gc_converter_update_from_variables (ArvGcConverter *gc_converter, ArvGcConverterNodeType node_type, GError **error)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (gc_converter);
	ArvGcNode *node = NULL;
	GError *local_error = NULL;
	GSList *iter;
	const char *expression;

	if (priv->formula_from_node != NULL)
		expression = arv_gc_property_node_get_string (priv->formula_from_node, &local_error);
	else
		expression = "";

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return FALSE;
	}

	arv_evaluator_set_expression (priv->formula_from, expression);

	for (iter = priv->expressions; iter != NULL; iter = iter->next) {
		const char *expression;
		const char *name;

		expression = arv_gc_property_node_get_string (ARV_GC_PROPERTY_NODE (iter->data), &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return FALSE;
		}

		name = arv_gc_property_node_get_name (iter->data);

		arv_evaluator_set_sub_expression (priv->formula_from, name, expression);
	}

	for (iter = priv->constants; iter != NULL; iter = iter->next) {
		const char *constant;
		const char *name;

		constant = arv_gc_property_node_get_string (ARV_GC_PROPERTY_NODE (iter->data), &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return FALSE;
		}

		name = arv_gc_property_node_get_name (iter->data);

		arv_evaluator_set_constant (priv->formula_from, name, constant);
	}

	for (iter = priv->variables; iter != NULL; iter = iter->next) {
		ArvGcPropertyNode *variable_node = iter->data;

		node = arv_gc_property_node_get_linked_node (ARV_GC_PROPERTY_NODE (variable_node));
		if (ARV_IS_GC_INTEGER (node)) {
			gint64 value;

			value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return FALSE;
			}

			arv_evaluator_set_int64_variable (priv->formula_from,
							  arv_gc_property_node_get_name (variable_node),
							  value);
		} else if (ARV_IS_GC_FLOAT (node)) {
			double value;

			value =  arv_gc_float_get_value (ARV_GC_FLOAT (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return FALSE;
			}

			arv_evaluator_set_double_variable (priv->formula_from,
							  arv_gc_property_node_get_name (variable_node),
							  value);
		}
	}

	if (priv->value != NULL) {
		node = arv_gc_property_node_get_linked_node (priv->value);

		if (ARV_IS_GC_INTEGER (node)) {
			gint64 value;

			switch (node_type) {
				case ARV_GC_CONVERTER_NODE_TYPE_MIN:
					value = arv_gc_integer_get_min (ARV_GC_INTEGER (node), &local_error);
					/* Default minimum, don't convert it */
					if (value == G_MININT64)
						return FALSE;
					break;
				case ARV_GC_CONVERTER_NODE_TYPE_MAX:
					value = arv_gc_integer_get_max (ARV_GC_INTEGER (node), &local_error);
					/* Default maximum, don't convert it */
					if (value == G_MAXINT64)
						return FALSE;
					break;
				case ARV_GC_CONVERTER_NODE_TYPE_INC:
					value = arv_gc_integer_get_inc (ARV_GC_INTEGER (node), &local_error);
					break;
				default:
					value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), &local_error);
					break;
			}

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return FALSE;
			}

			arv_evaluator_set_int64_variable (priv->formula_from, "TO", value);
		} else if (ARV_IS_GC_FLOAT (node)) {
			double value;

			switch (node_type) {
				case ARV_GC_CONVERTER_NODE_TYPE_MIN:
					value = arv_gc_float_get_min (ARV_GC_FLOAT (node), &local_error);
					/* Default minimum, don't convert it */
					if (value == -G_MAXDOUBLE)
						return FALSE;
					break;
				case ARV_GC_CONVERTER_NODE_TYPE_MAX:
					value = arv_gc_float_get_max (ARV_GC_FLOAT (node), &local_error);
					/* Default maximum, don't convert it */
					if (value == G_MAXDOUBLE)
						return FALSE;
					break;
				case ARV_GC_CONVERTER_NODE_TYPE_INC:
					value = arv_gc_float_get_inc (ARV_GC_FLOAT (node), &local_error);
					break;
				default:
					value =  arv_gc_float_get_value (ARV_GC_FLOAT (node), &local_error);
					break;
			}

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return FALSE;
			}

			arv_evaluator_set_double_variable (priv->formula_from, "TO", value);
		} else {
			arv_warning_genicam ("[GcConverter::set_value] Invalid pValue node '%s'",
					     arv_gc_property_node_get_string (priv->value, NULL));
			g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_INVALID_PVALUE,
				     "pValue node '%s' of '%s' is invalid",
				     arv_gc_property_node_get_string (priv->value, NULL),
				     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_converter)));
			return FALSE;
		}
	} else {
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_PVALUE_NOT_DEFINED,
			     "pValue node of '%s' converter is not defined",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_converter)));
		return FALSE;
	}

	return TRUE;
}

double
arv_gc_converter_convert_to_double (ArvGcConverter *gc_converter, ArvGcConverterNodeType node_type, GError **error)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (gc_converter);
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_CONVERTER (gc_converter), 0.0);

	if (!arv_gc_converter_update_from_variables (gc_converter, node_type, &local_error)) {
		if (local_error != NULL)
			g_propagate_error (error, local_error);

		switch (node_type) {
			case ARV_GC_CONVERTER_NODE_TYPE_MIN:
				return -G_MAXDOUBLE;
			case ARV_GC_CONVERTER_NODE_TYPE_MAX:
				return G_MAXDOUBLE;
			default:
				return 0.0;
		}
	}

	return arv_evaluator_evaluate_as_double (priv->formula_from, NULL);
}

gint64
arv_gc_converter_convert_to_int64 (ArvGcConverter *gc_converter, ArvGcConverterNodeType node_type, GError **error)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (gc_converter);
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_CONVERTER (gc_converter), 0);

	if (!arv_gc_converter_update_from_variables (gc_converter, node_type, &local_error)) {
		if (local_error != NULL)
			g_propagate_error (error, local_error);

		switch (node_type) {
			case ARV_GC_CONVERTER_NODE_TYPE_MIN:
				return G_MININT64;
			case ARV_GC_CONVERTER_NODE_TYPE_MAX:
				return G_MAXINT64;
			default:
				return 0;
		}
	}

	return arv_evaluator_evaluate_as_double (priv->formula_from, NULL);
}

static void
arv_gc_converter_update_to_variables (ArvGcConverter *gc_converter, GError **error)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (gc_converter);
	ArvGcNode *node;
	GError *local_error = NULL;
	GSList *iter;
	const char *expression;

	if (priv->formula_to_node != NULL)
		expression = arv_gc_property_node_get_string (priv->formula_to_node, &local_error);
	else
		expression = "";

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	arv_evaluator_set_expression (priv->formula_to, expression);

	for (iter = priv->expressions; iter != NULL; iter = iter->next) {
		const char *expression;
		const char *name;

		expression = arv_gc_property_node_get_string (ARV_GC_PROPERTY_NODE (iter->data), &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return;
		}

		name = arv_gc_property_node_get_name (iter->data);

		arv_evaluator_set_sub_expression (priv->formula_to, name, expression);
	}

	for (iter = priv->constants; iter != NULL; iter = iter->next) {
		const char *constant;
		const char *name;

		constant = arv_gc_property_node_get_string (ARV_GC_PROPERTY_NODE (iter->data), &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return;
		}

		name = arv_gc_property_node_get_name (iter->data);

		arv_evaluator_set_constant (priv->formula_to, name, constant);
	}

	for (iter = priv->variables; iter != NULL; iter = iter->next) {
		ArvGcPropertyNode *variable_node = iter->data;

		node = arv_gc_property_node_get_linked_node (ARV_GC_PROPERTY_NODE (variable_node));
		if (ARV_IS_GC_INTEGER (node)) {
			gint64 value;

			value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			arv_evaluator_set_int64_variable (priv->formula_to,
							  arv_gc_property_node_get_name (variable_node),
							  value);
		} else if (ARV_IS_GC_FLOAT (node)) {
			double value;

			value =  arv_gc_float_get_value (ARV_GC_FLOAT (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			arv_evaluator_set_double_variable (priv->formula_to,
							   arv_gc_property_node_get_name (variable_node),
							   value);
		}
	}

	if (priv->value != NULL) {
		node = arv_gc_property_node_get_linked_node (priv->value);

		if (ARV_IS_GC_INTEGER (node)) {
			arv_gc_integer_set_value (ARV_GC_INTEGER (node),
						  arv_evaluator_evaluate_as_double (priv->formula_to, NULL),
						  &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}
		} else if (ARV_IS_GC_FLOAT (node)) {
			arv_gc_float_set_value (ARV_GC_FLOAT (node),
						arv_evaluator_evaluate_as_double (priv->formula_to, NULL),
						&local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}
		} else
			arv_warning_genicam ("[GcConverter::set_value] Invalid pValue node '%s'",
					     arv_gc_property_node_get_string (priv->value, NULL));
	}
}

void
arv_gc_converter_convert_from_double (ArvGcConverter *gc_converter, double value, GError **error)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (gc_converter);

	g_return_if_fail (ARV_IS_GC_CONVERTER (gc_converter));

	arv_gc_feature_node_increment_change_count (ARV_GC_FEATURE_NODE (gc_converter));
	arv_evaluator_set_double_variable (priv->formula_to, "FROM", value);
	arv_gc_converter_update_to_variables (gc_converter, error);
}

void
arv_gc_converter_convert_from_int64 (ArvGcConverter *gc_converter, gint64 value, GError **error)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (gc_converter);

	g_return_if_fail (ARV_IS_GC_CONVERTER (gc_converter));

	arv_gc_feature_node_increment_change_count (ARV_GC_FEATURE_NODE (gc_converter));
	arv_evaluator_set_int64_variable (priv->formula_to, "FROM", value);
	arv_gc_converter_update_to_variables (gc_converter, error);
}

ArvGcRepresentation
arv_gc_converter_get_representation (ArvGcConverter *gc_converter)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (gc_converter);

	g_return_val_if_fail (ARV_IS_GC_CONVERTER (gc_converter), ARV_GC_REPRESENTATION_UNDEFINED);

	if (priv->representation == NULL)
		return ARV_GC_REPRESENTATION_UNDEFINED;

	return arv_gc_property_node_get_representation (ARV_GC_PROPERTY_NODE (priv->representation), ARV_GC_REPRESENTATION_UNDEFINED);
}

const char *
arv_gc_converter_get_unit (ArvGcConverter *gc_converter)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (gc_converter);

	g_return_val_if_fail (ARV_IS_GC_CONVERTER (gc_converter), NULL);

	if (priv->unit == NULL)
		return NULL;

	return arv_gc_property_node_get_string (ARV_GC_PROPERTY_NODE (priv->unit), NULL);
}

ArvGcDisplayNotation
arv_gc_converter_get_display_notation (ArvGcConverter *gc_converter)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (gc_converter);

	g_return_val_if_fail (ARV_IS_GC_CONVERTER (gc_converter), ARV_GC_DISPLAY_NOTATION_DEFAULT);

	if (priv->display_notation == NULL)
		return ARV_GC_DISPLAY_NOTATION_DEFAULT;

	return arv_gc_property_node_get_display_notation (ARV_GC_PROPERTY_NODE (priv->display_notation), ARV_GC_DISPLAY_NOTATION_DEFAULT);
}

gint64
arv_gc_converter_get_display_precision (ArvGcConverter *gc_converter)
{
	ArvGcConverterPrivate *priv = arv_gc_converter_get_instance_private (gc_converter);

	g_return_val_if_fail (ARV_IS_GC_CONVERTER (gc_converter), ARV_GC_DISPLAY_PRECISION_DEFAULT);

	if (priv->display_precision == NULL)
		return ARV_GC_DISPLAY_PRECISION_DEFAULT;

	return arv_gc_property_node_get_display_precision (ARV_GC_PROPERTY_NODE (priv->display_precision), ARV_GC_DISPLAY_PRECISION_DEFAULT);
}
