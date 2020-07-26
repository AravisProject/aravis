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
 * SECTION: arvgcswissknife
 * @short_description: Class for SwissKnife and IntSwissKnife nodes
 */

#include <arvgcswissknife.h>
#include <arvevaluator.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcport.h>
#include <arvgc.h>
#include <arvdebug.h>
#include <string.h>

typedef struct {
	GType value_type;
	GSList *variables;	/* ArvGcVariableNode list */
	GSList *constants;	/* ArvGcVariableNode list */
	GSList *expressions;	/* ArvGcVariableNode list */

	ArvGcPropertyNode *formula_node;
	ArvGcPropertyNode *unit;
	ArvGcPropertyNode *representation;

	ArvEvaluator *formula;
} ArvGcSwissKnifePrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ArvGcSwissKnife, arv_gc_swiss_knife, ARV_TYPE_GC_FEATURE_NODE, G_ADD_PRIVATE (ArvGcSwissKnife))

/* ArvDomNode implementation */

static void
arv_gc_swiss_knife_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcSwissKnifePrivate *priv = arv_gc_swiss_knife_get_instance_private (ARV_GC_SWISS_KNIFE (self));

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_P_VARIABLE:
				priv->variables = g_slist_prepend (priv->variables, property_node);
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_FORMULA:
				priv->formula_node = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_UNIT:
				priv->unit = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_REPRESENTATION:
				priv->representation = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_EXPRESSION:
				priv->expressions = g_slist_prepend (priv->expressions, property_node);
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_CONSTANT:
				priv->constants = g_slist_prepend (priv->constants, property_node);
				break;
			default:
				ARV_DOM_NODE_CLASS (arv_gc_swiss_knife_parent_class)->post_new_child (self, child);
				break;
		}
	}
}

static void
arv_gc_swiss_knife_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcFeatureNode implementation */

static void
arv_gc_swiss_knife_init (ArvGcSwissKnife *self)
{
	ArvGcSwissKnifePrivate *priv = arv_gc_swiss_knife_get_instance_private (self);

	priv->formula = arv_evaluator_new (NULL);
}

static ArvGcAccessMode
arv_gc_swiss_knife_get_access_mode (ArvGcFeatureNode *gc_feature_node)
{
	return ARV_GC_ACCESS_MODE_RO;
}

static void
arv_gc_swiss_knife_finalize (GObject *object)
{
	ArvGcSwissKnifePrivate *priv = arv_gc_swiss_knife_get_instance_private (ARV_GC_SWISS_KNIFE (object));

	g_slist_free (priv->variables);
	g_slist_free (priv->expressions);
	g_slist_free (priv->constants);

	g_clear_object (&priv->formula);

	G_OBJECT_CLASS (arv_gc_swiss_knife_parent_class)->finalize (object);
}

static void
arv_gc_swiss_knife_class_init (ArvGcSwissKnifeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);

	object_class->finalize = arv_gc_swiss_knife_finalize;
	dom_node_class->post_new_child = arv_gc_swiss_knife_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_swiss_knife_pre_remove_child;
	gc_feature_node_class->get_access_mode = arv_gc_swiss_knife_get_access_mode;
}

/* ArvGcInteger interface implementation */

static void
_update_variables (ArvGcSwissKnife *self, GError **error)
{
	ArvGcSwissKnifePrivate *priv = arv_gc_swiss_knife_get_instance_private (self);
	ArvGcNode *node;
	GError *local_error = NULL;
	GSList *iter;
	const char *expression;

	if (priv->formula_node != NULL)
		expression = arv_gc_property_node_get_string (priv->formula_node, &local_error);
	else
		expression = "";

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	arv_evaluator_set_expression (priv->formula, expression);

	for (iter = priv->expressions; iter != NULL; iter = iter->next) {
		const char *expression;
		const char *name;

		expression = arv_gc_property_node_get_string (ARV_GC_PROPERTY_NODE (iter->data), &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return;
		}

		name = arv_gc_property_node_get_name (iter->data);

		arv_evaluator_set_sub_expression (priv->formula, name, expression);
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

		arv_evaluator_set_constant (priv->formula, name, constant);
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

			arv_evaluator_set_int64_variable (priv->formula,
							  arv_gc_property_node_get_name (variable_node),
							  value);
		} else if (ARV_IS_GC_FLOAT (node)) {
			double value;

			value = arv_gc_float_get_value (ARV_GC_FLOAT (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			arv_evaluator_set_double_variable (priv->formula,
							  arv_gc_property_node_get_name (variable_node),
							  value);
		}
	}
}

gint64
arv_gc_swiss_knife_get_integer_value (ArvGcSwissKnife *self, GError **error)
{
	ArvGcSwissKnifePrivate *priv = arv_gc_swiss_knife_get_instance_private (self);
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_SWISS_KNIFE (self), 0);

	_update_variables (self, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	return arv_evaluator_evaluate_as_int64 (priv->formula, NULL);
}

double
arv_gc_swiss_knife_get_float_value (ArvGcSwissKnife *self, GError **error)
{
	ArvGcSwissKnifePrivate *priv = arv_gc_swiss_knife_get_instance_private (self);
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_SWISS_KNIFE (self), 0.0);

	_update_variables (self, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0.0;
	}

	return arv_evaluator_evaluate_as_double (priv->formula, NULL);
}

ArvGcRepresentation
arv_gc_swiss_knife_get_representation (ArvGcSwissKnife *self, GError **error)
{
	ArvGcSwissKnifePrivate *priv = arv_gc_swiss_knife_get_instance_private (self);

	g_return_val_if_fail (ARV_IS_GC_SWISS_KNIFE (self), ARV_GC_REPRESENTATION_UNDEFINED);

	if (priv->representation == NULL)
		return ARV_GC_REPRESENTATION_UNDEFINED;

	return arv_gc_property_node_get_representation (ARV_GC_PROPERTY_NODE (priv->representation), ARV_GC_REPRESENTATION_UNDEFINED);
}

const char *
arv_gc_swiss_knife_get_unit (ArvGcSwissKnife *self, GError **error)
{
	ArvGcSwissKnifePrivate *priv = arv_gc_swiss_knife_get_instance_private (self);

	g_return_val_if_fail (ARV_IS_GC_SWISS_KNIFE (self), NULL);

	if (priv->unit == NULL)
		return NULL;

	return arv_gc_property_node_get_string (ARV_GC_PROPERTY_NODE (priv->unit), error);
}
