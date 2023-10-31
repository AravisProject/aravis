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
 * SECTION: arvgcpropertynode
 * @short_description: Base class for Genicam property nodes
 *
 * #ArvGcPropertyNode provides a base class for the implementation of the different
 * types of Genicam property nodes (Value, pValue, Endianness...).
 */

#include <arvgcpropertynode.h>
#include <arvgcfeaturenode.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcboolean.h>
#include <arvgcregister.h>
#include <arvgcstring.h>
#include <arvgc.h>
#include <arvdomtext.h>
#include <arvmiscprivate.h>
#include <arvdebugprivate.h>
#include <arvenumtypes.h>
#include <string.h>

enum {
	ARV_GC_PROPERTY_NODE_PROPERTY_0,
	ARV_GC_PROPERTY_NODE_PROPERTY_TYPE
} ArvGcPropertyNodeProperties;

typedef struct {
	ArvGcNode base;

	ArvGcPropertyNodeType	type;

	char *name;

	gboolean value_data_up_to_date;
	char *value_data;
} ArvGcPropertyNodePrivate;

G_DEFINE_TYPE_WITH_CODE (ArvGcPropertyNode, arv_gc_property_node, ARV_TYPE_GC_NODE, G_ADD_PRIVATE (ArvGcPropertyNode))

/* ArvDomNode implementation */

static const char *
arv_gc_property_node_get_node_name (ArvDomNode *node)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (ARV_GC_PROPERTY_NODE (node));

	switch (priv->type) {
		case ARV_GC_PROPERTY_NODE_TYPE_VALUE:
			return "Value";
		case ARV_GC_PROPERTY_NODE_TYPE_ADDRESS:
			return "Address";
		case ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION:
			return "Description";
		case ARV_GC_PROPERTY_NODE_TYPE_VISIBILITY:
			return "Visibility";
		case ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP:
			return "ToolTip";
		case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME:
			return "DisplayName";
		case ARV_GC_PROPERTY_NODE_TYPE_MINIMUM:
			return "Min";
		case ARV_GC_PROPERTY_NODE_TYPE_MAXIMUM:
			return "Max";
		case ARV_GC_PROPERTY_NODE_TYPE_SLOPE:
			return "Slope";
		case ARV_GC_PROPERTY_NODE_TYPE_INCREMENT:
			return "Inc";
		case ARV_GC_PROPERTY_NODE_TYPE_IS_LINEAR:
			return "IsLinear";
		case ARV_GC_PROPERTY_NODE_TYPE_REPRESENTATION:
			return "Representation";
		case ARV_GC_PROPERTY_NODE_TYPE_UNIT:
			return "Unit";
		case ARV_GC_PROPERTY_NODE_TYPE_ON_VALUE:
			return "OnValue";
		case ARV_GC_PROPERTY_NODE_TYPE_OFF_VALUE:
			return "OffValue";
		case ARV_GC_PROPERTY_NODE_TYPE_LENGTH:
			return "Length";
		case ARV_GC_PROPERTY_NODE_TYPE_FORMULA:
			return "Formula";
		case ARV_GC_PROPERTY_NODE_TYPE_FORMULA_TO:
			return "FormulaTo";
		case ARV_GC_PROPERTY_NODE_TYPE_FORMULA_FROM:
			return "FormulaFrom";
		case ARV_GC_PROPERTY_NODE_TYPE_EXPRESSION:
			return "Expression";
		case ARV_GC_PROPERTY_NODE_TYPE_CONSTANT:
			return "Constant";
		case ARV_GC_PROPERTY_NODE_TYPE_ACCESS_MODE:
			return "AccessMode";
		case ARV_GC_PROPERTY_NODE_TYPE_IMPOSED_ACCESS_MODE:
			return "ImposedAccessMode";
		case ARV_GC_PROPERTY_NODE_TYPE_CACHABLE:
			return "Cachable";
		case ARV_GC_PROPERTY_NODE_TYPE_POLLING_TIME:
			return "PollingTime";
		case ARV_GC_PROPERTY_NODE_TYPE_ENDIANNESS:
			return "Endianess";
		case ARV_GC_PROPERTY_NODE_TYPE_SIGN:
			return "Sign";
		case ARV_GC_PROPERTY_NODE_TYPE_LSB:
			return "LSB";
		case ARV_GC_PROPERTY_NODE_TYPE_MSB:
			return "MSB";
		case ARV_GC_PROPERTY_NODE_TYPE_BIT:
			return "Bit";
		case ARV_GC_PROPERTY_NODE_TYPE_COMMAND_VALUE:
			return "CommandValue";
		case ARV_GC_PROPERTY_NODE_TYPE_CHUNK_ID:
			return "ChunkID";
		case ARV_GC_PROPERTY_NODE_TYPE_EVENT_ID:
			return "EventID";
		case ARV_GC_PROPERTY_NODE_TYPE_VALUE_DEFAULT:
			return "ValueDefault";
		case ARV_GC_PROPERTY_NODE_TYPE_STREAMABLE:
			return "Streamable";

		case ARV_GC_PROPERTY_NODE_TYPE_P_FEATURE:
			return "pFeature";
		case ARV_GC_PROPERTY_NODE_TYPE_P_VALUE:
			return "pValue";
		case ARV_GC_PROPERTY_NODE_TYPE_P_ADDRESS:
			return "pAddress";
		case ARV_GC_PROPERTY_NODE_TYPE_P_IS_IMPLEMENTED:
			return "pIsImplemented";
		case ARV_GC_PROPERTY_NODE_TYPE_P_IS_AVAILABLE:
			return "pIsAvailable";
		case ARV_GC_PROPERTY_NODE_TYPE_P_IS_LOCKED:
			return "pIsLocked";
		case ARV_GC_PROPERTY_NODE_TYPE_P_SELECTED:
			return "pSelected";
		case ARV_GC_PROPERTY_NODE_TYPE_P_MINIMUM:
			return "pMin";
		case ARV_GC_PROPERTY_NODE_TYPE_P_MAXIMUM:
			return "pMax";
		case ARV_GC_PROPERTY_NODE_TYPE_P_INCREMENT:
			return "pInc";
		case ARV_GC_PROPERTY_NODE_TYPE_P_LENGTH:
			return "pLength";
		case ARV_GC_PROPERTY_NODE_TYPE_P_PORT:
			return "pPort";
		case ARV_GC_PROPERTY_NODE_TYPE_P_VARIABLE:
			return "pVariable";
		case ARV_GC_PROPERTY_NODE_TYPE_P_COMMAND_VALUE:
			return "pCommandValue";
		case ARV_GC_PROPERTY_NODE_TYPE_P_VALUE_DEFAULT:
			return "pValueDefault";

		default:
			return "Unknown";
	}
}

/* ArvDomElement implementation */

static gboolean
_can_append_child (ArvDomNode *self, ArvDomNode *child)
{
	return ARV_IS_DOM_TEXT (child);
}

static void
_post_new_child (ArvDomNode *parent, ArvDomNode *child)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (ARV_GC_PROPERTY_NODE (parent));

	priv->value_data_up_to_date = FALSE;
}

static void
_pre_remove_child (ArvDomNode *parent, ArvDomNode *child)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (ARV_GC_PROPERTY_NODE (parent));

	priv->value_data_up_to_date = FALSE;
}

/* ArvDomElement implementation */

static void
arv_gc_property_node_set_attribute (ArvDomElement *self, const char *name, const char *value)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (ARV_GC_PROPERTY_NODE (self));

	if (strcmp (name, "Name") == 0) {
		g_free (priv->name);
		priv->name = g_strdup (value);
	} else
		arv_info_dom ("[GcPropertyNode::set_attribute] Unknown attribute '%s'", name);
}

static const char *
arv_gc_property_node_get_attribute (ArvDomElement *self, const char *name)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (ARV_GC_PROPERTY_NODE (self));

	if (strcmp (name, "Name") == 0)
		return priv->name;

	arv_info_dom ("[GcPropertyNode::set_attribute] Unknown attribute '%s'", name);

	return NULL;
}

/* ArvGcPropertyNode implementation */

static const char *
_get_value_data (ArvGcPropertyNode *property_node)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (property_node);
	ArvDomNode *dom_node = ARV_DOM_NODE (property_node);

	if (!priv->value_data_up_to_date) {
		ArvDomNode *iter;
		GString *string = g_string_new (NULL);

		for (iter = arv_dom_node_get_first_child (dom_node);
		     iter != NULL;
		     iter = arv_dom_node_get_next_sibling (iter))
			g_string_append (string, arv_dom_character_data_get_data (ARV_DOM_CHARACTER_DATA (iter)));
		g_free (priv->value_data);
		priv->value_data = arv_g_string_free_and_steal(string);
		priv->value_data_up_to_date = TRUE;
	}

	return priv->value_data;
}

static void
_set_value_data (ArvGcPropertyNode *property_node, const char *data)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (property_node);
	ArvDomNode *dom_node = ARV_DOM_NODE (property_node);

	if (arv_dom_node_get_first_child (dom_node) != NULL) {
		ArvDomNode *iter;

		arv_dom_character_data_set_data (ARV_DOM_CHARACTER_DATA (arv_dom_node_get_first_child (dom_node)), data);
		for (iter = arv_dom_node_get_next_sibling (arv_dom_node_get_first_child (dom_node));
		     iter != NULL;
		     iter = arv_dom_node_get_next_sibling (iter))
			arv_dom_character_data_set_data (ARV_DOM_CHARACTER_DATA (iter), "");
	}

	g_free (priv->value_data);
	priv->value_data = g_strdup (data);
	priv->value_data_up_to_date = TRUE;
}

static ArvDomNode *
_get_pvalue_node (ArvGcPropertyNode *property_node)
{
	ArvDomNode *pvalue_node;
	const char *node_name;
	ArvGc *genicam;

	if (arv_gc_property_node_get_node_type (property_node) < ARV_GC_PROPERTY_NODE_TYPE_P_UNKNONW)
		return NULL;

	node_name = _get_value_data (property_node);
	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (property_node));
	pvalue_node = ARV_DOM_NODE (arv_gc_get_node (genicam, node_name));

	return pvalue_node;
}

/**
 * arv_gc_property_node_get_name:
 * @node: a #ArvGcPropertyNode
 *
 * Returns: node Name property value.
 *
 * Since: 0.6.0
 */

const char *
arv_gc_property_node_get_name (ArvGcPropertyNode *node)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (node);

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (node), NULL);

	return priv->name;
}

const char *
arv_gc_property_node_get_string (ArvGcPropertyNode *node, GError **error)
{
	ArvDomNode *pvalue_node;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (node), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	pvalue_node = _get_pvalue_node (node);
	if (pvalue_node == NULL)
		return _get_value_data (node);

	if (ARV_IS_GC_STRING (pvalue_node)) {
		GError *local_error = NULL;
		const char *value;

		value = arv_gc_string_get_value (ARV_GC_STRING (pvalue_node), &local_error);

		if (local_error != NULL)
			g_propagate_error (error, local_error);

		return value;
	}

	arv_warning_genicam ("[GcPropertyNode::get_string] Invalid node '%s'",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (pvalue_node)));

	return NULL;
}

void
arv_gc_property_node_set_string (ArvGcPropertyNode *node, const char *string, GError **error)
{
	ArvDomNode *pvalue_node;

	g_return_if_fail (ARV_IS_GC_PROPERTY_NODE (node));
	g_return_if_fail (error == NULL || *error == NULL);

	pvalue_node = _get_pvalue_node (node);
	if (pvalue_node == NULL) {
		_set_value_data (node, string);
		return;
	}

	if (ARV_IS_GC_STRING (pvalue_node)) {
		GError *local_error = NULL;

		arv_gc_string_set_value (ARV_GC_STRING (pvalue_node), string, &local_error);

		if (local_error != NULL)
			g_propagate_error (error, local_error);

		return;
	}

	arv_warning_genicam ("[GcPropertyNode::set_string] Invalid linked node '%s'",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (pvalue_node)));
}

gint64
arv_gc_property_node_get_int64 (ArvGcPropertyNode *node, GError **error)
{
	ArvDomNode *pvalue_node;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (node), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	pvalue_node = _get_pvalue_node (node);
	if (pvalue_node == NULL)
		return g_ascii_strtoll (_get_value_data (node), NULL, 0);

	if (ARV_IS_GC_INTEGER (pvalue_node)) {
		return arv_gc_integer_get_value (ARV_GC_INTEGER (pvalue_node), error);
	} else if (ARV_IS_GC_FLOAT (pvalue_node)) {
		return (gint64) arv_gc_float_get_value (ARV_GC_FLOAT (pvalue_node), error);
	} else if (ARV_IS_GC_BOOLEAN (pvalue_node)) {
		return arv_gc_boolean_get_value (ARV_GC_BOOLEAN (pvalue_node), error) ? 1 : 0;
	}

	arv_warning_genicam ("[GcPropertyNode::get_int64] Invalid node '%s'",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (pvalue_node)));

	return 0;
}

void
arv_gc_property_node_set_int64 (ArvGcPropertyNode *node, gint64 v_int64, GError **error)
{
	ArvDomNode *pvalue_node;

	g_return_if_fail (ARV_IS_GC_PROPERTY_NODE (node));
	g_return_if_fail (error == NULL || *error == NULL);

	pvalue_node = _get_pvalue_node (node);
	if (pvalue_node == NULL) {
		char *buffer;

		buffer = g_strdup_printf ("%" G_GINT64_FORMAT, v_int64);
		_set_value_data (node, buffer);
		g_free (buffer);
		return ;
	}

	if (ARV_IS_GC_INTEGER (pvalue_node)) {
		arv_gc_integer_set_value (ARV_GC_INTEGER (pvalue_node), v_int64, error);
		return;
	} else if (ARV_IS_GC_FLOAT (pvalue_node)) {
		arv_gc_float_set_value (ARV_GC_FLOAT (pvalue_node), (double) v_int64, error);
		return;
	}

	arv_warning_genicam ("[GcPropertyNode::set_int64] Invalid linked node '%s'",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (pvalue_node)));
}

double
arv_gc_property_node_get_double (ArvGcPropertyNode *node, GError **error)
{
	ArvDomNode *pvalue_node;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (node), 0.0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0.0);

	pvalue_node = _get_pvalue_node (node);
	if (pvalue_node == NULL)
		return g_ascii_strtod (_get_value_data (node), NULL);


	if (ARV_IS_GC_FLOAT (pvalue_node)) {
		return arv_gc_float_get_value (ARV_GC_FLOAT (pvalue_node), error);
	} else if (ARV_IS_GC_INTEGER (pvalue_node)) {
		return arv_gc_integer_get_value (ARV_GC_INTEGER (pvalue_node), error);
	}

	arv_warning_genicam ("[GcPropertyNode::get_double] Invalid node '%s'",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (pvalue_node)));

	return 0.0;
}

void
arv_gc_property_node_set_double (ArvGcPropertyNode *node, double v_double, GError **error)
{
	ArvDomNode *pvalue_node;

	g_return_if_fail (ARV_IS_GC_PROPERTY_NODE (node));
	g_return_if_fail (error == NULL || *error == NULL);

	pvalue_node = _get_pvalue_node (node);
	if (pvalue_node == NULL) {
		char buffer[G_ASCII_DTOSTR_BUF_SIZE];

		g_ascii_dtostr (buffer, G_ASCII_DTOSTR_BUF_SIZE, v_double);
		_set_value_data (node, buffer);
		return ;
	}

	if (ARV_IS_GC_FLOAT (pvalue_node)) {
		arv_gc_float_set_value (ARV_GC_FLOAT (pvalue_node), v_double, error);
		return;
	} else if (ARV_IS_GC_INTEGER (pvalue_node)) {
		arv_gc_integer_set_value (ARV_GC_INTEGER (pvalue_node), v_double, error);
		return;
	}

	arv_warning_genicam ("[GcPropertyNode::set_double] Invalid linked node '%s'",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (pvalue_node)));
}

ArvGcPropertyNodeType
arv_gc_property_node_get_node_type (ArvGcPropertyNode *node)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (node);

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (node), ARV_GC_PROPERTY_NODE_TYPE_UNKNOWN);

	return priv->type;
}

/**
 * arv_gc_property_node_get_linked_node:
 * @node: a #ArvGcPropertyNode
 *
 * Returns: (transfer none): the #ArvGcNode which @node points to, %NULL if the property is not a pointer.
 */

ArvGcNode *
arv_gc_property_node_get_linked_node (ArvGcPropertyNode *node)
{
	ArvGc *genicam;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (node), NULL);

	if (arv_gc_property_node_get_node_type (node) <= ARV_GC_PROPERTY_NODE_TYPE_P_UNKNONW)
		return NULL;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (node));
	return arv_gc_get_node (genicam, _get_value_data (node));
}

static ArvGcNode *
arv_gc_property_node_new (ArvGcPropertyNodeType type)
{
	return g_object_new (ARV_TYPE_GC_PROPERTY_NODE, "node-type", type, NULL);
}

ArvGcNode *
arv_gc_property_node_new_p_feature (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_FEATURE);
}

ArvGcNode *
arv_gc_property_node_new_value (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_VALUE);
}

ArvGcNode *
arv_gc_property_node_new_p_value (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_VALUE);
}

ArvGcNode *
arv_gc_property_node_new_address (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_ADDRESS);
}

ArvGcNode *
arv_gc_property_node_new_p_address (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_ADDRESS);
}

ArvGcNode *
arv_gc_property_node_new_description (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION);
}

ArvGcNode *
arv_gc_property_node_new_visibility (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_VISIBILITY);
}

ArvGcVisibility
arv_gc_property_node_get_visibility (ArvGcPropertyNode *self, ArvGcVisibility default_value)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (self);
	const char *value;

	if (self == NULL)
		return default_value;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (self), default_value);
	g_return_val_if_fail (priv->type == ARV_GC_PROPERTY_NODE_TYPE_VISIBILITY, ARV_GC_VISIBILITY_UNDEFINED);

	value = _get_value_data (self);

	if (g_strcmp0 (value, "Invisible") == 0)
		return ARV_GC_VISIBILITY_INVISIBLE;
	else if (g_strcmp0 (value, "Guru") == 0)
		return ARV_GC_VISIBILITY_GURU;
	else if (g_strcmp0 (value, "Expert") == 0)
		return ARV_GC_VISIBILITY_EXPERT;
	else if (g_strcmp0 (value, "Beginner") == 0)
		return ARV_GC_VISIBILITY_BEGINNER;

	return ARV_GC_VISIBILITY_UNDEFINED;
}

ArvGcNode *
arv_gc_property_node_new_tooltip (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP);
}

ArvGcNode *
arv_gc_property_node_new_display_name (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME);
}

ArvGcNode *
arv_gc_property_node_new_minimum (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_MINIMUM);
}

ArvGcNode *
arv_gc_property_node_new_p_minimum (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_MINIMUM);
}

ArvGcNode *
arv_gc_property_node_new_maximum (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_MAXIMUM);
}

ArvGcNode *
arv_gc_property_node_new_p_maximum (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_MAXIMUM);
}

ArvGcNode *
arv_gc_property_node_new_slope (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_SLOPE);
}

ArvGcNode *
arv_gc_property_node_new_increment (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_INCREMENT);
}

ArvGcNode *
arv_gc_property_node_new_p_increment (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_INCREMENT);
}

ArvGcNode *
arv_gc_property_node_new_is_linear (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_IS_LINEAR);
}

ArvGcNode *
arv_gc_property_node_new_representation (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_REPRESENTATION);
}

ArvGcRepresentation
arv_gc_property_node_get_representation (ArvGcPropertyNode *self, ArvGcRepresentation default_value)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (self);
	const char *value;

	if (self == NULL)
		return default_value;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (self), default_value);
	g_return_val_if_fail (priv->type == ARV_GC_PROPERTY_NODE_TYPE_REPRESENTATION, default_value);

	value = _get_value_data (self);

	if (g_strcmp0 (value, "Linear") == 0)
		return ARV_GC_REPRESENTATION_LINEAR;
	else if (g_strcmp0 (value, "Logarithmic") == 0)
		return ARV_GC_REPRESENTATION_LOGARITHMIC;
	else if (g_strcmp0 (value, "Boolean") == 0)
		return ARV_GC_REPRESENTATION_BOOLEAN;
	else if (g_strcmp0 (value, "PureNumber") == 0)
		return ARV_GC_REPRESENTATION_PURE_NUMBER;
	else if (g_strcmp0 (value, "HexNumber") == 0)
		return ARV_GC_REPRESENTATION_HEX_NUMBER;
	else if (g_strcmp0 (value, "IPV4Address") == 0)
		return ARV_GC_REPRESENTATION_IPV4_ADDRESS;
	else if (g_strcmp0 (value, "MACAddress") == 0)
		return ARV_GC_REPRESENTATION_MAC_ADDRESS;

	return default_value;
}

ArvGcNode *
arv_gc_property_node_new_display_notation (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NOTATION);
}

ArvGcDisplayNotation
arv_gc_property_node_get_display_notation (ArvGcPropertyNode *self, ArvGcDisplayNotation default_value)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (self);
	const char *value;

	if (self == NULL)
		return default_value;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (self), default_value);
	g_return_val_if_fail (priv->type == ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NOTATION, default_value);

	value = _get_value_data (self);

	if (g_strcmp0 (value, "Automatic") == 0)
		return ARV_GC_DISPLAY_NOTATION_AUTOMATIC;
	else if (g_strcmp0 (value, "Fixed") == 0)
		return ARV_GC_DISPLAY_NOTATION_FIXED;
	else if (g_strcmp0 (value, "Scientific") == 0)
		return ARV_GC_DISPLAY_NOTATION_SCIENTIFIC;

	return default_value;
}

ArvGcNode *
arv_gc_property_node_new_display_precision (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_PRECISION);
}

gint64
arv_gc_property_node_get_display_precision (ArvGcPropertyNode *self, gint64 default_value)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (self);

	if (self == NULL)
		return default_value;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (self), default_value);
	g_return_val_if_fail (priv->type == ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_PRECISION, default_value);

	return g_ascii_strtoll (_get_value_data (self), NULL, 0);
}

ArvGcNode *
arv_gc_property_node_new_unit (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_UNIT);
}

ArvGcNode *
arv_gc_property_node_new_on_value (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_ON_VALUE);
}

ArvGcNode *
arv_gc_property_node_new_off_value (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_OFF_VALUE);
}

ArvGcNode *
arv_gc_property_node_new_p_is_implemented (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_IS_IMPLEMENTED);
}

ArvGcNode *
arv_gc_property_node_new_p_is_available (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_IS_AVAILABLE);
}

ArvGcNode *
arv_gc_property_node_new_p_is_locked (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_IS_LOCKED);
}

ArvGcNode *
arv_gc_property_node_new_p_selected (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_SELECTED);
}

ArvGcNode *
arv_gc_property_node_new_length (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_LENGTH);
}

ArvGcNode *
arv_gc_property_node_new_p_length (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_LENGTH);
}

ArvGcNode *
arv_gc_property_node_new_p_port (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_PORT);
}

ArvGcNode *
arv_gc_property_node_new_p_variable (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_VARIABLE);
}

ArvGcNode *
arv_gc_property_node_new_formula (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_FORMULA);
}

ArvGcNode *
arv_gc_property_node_new_formula_to (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_FORMULA_TO);
}

ArvGcNode *
arv_gc_property_node_new_formula_from (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_FORMULA_FROM);
}

ArvGcNode *
arv_gc_property_node_new_expression (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_EXPRESSION);
}

ArvGcNode *
arv_gc_property_node_new_constant (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_CONSTANT);
}

ArvGcNode *
arv_gc_property_node_new_access_mode (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_ACCESS_MODE);
}

ArvGcNode *
arv_gc_property_node_new_imposed_access_mode (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_IMPOSED_ACCESS_MODE);
}

ArvGcAccessMode
arv_gc_property_node_get_access_mode (ArvGcPropertyNode *self, ArvGcAccessMode default_value)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (self);
	const char *value;

	if (self == NULL)
		return default_value;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (self), default_value);
	g_return_val_if_fail (priv->type == ARV_GC_PROPERTY_NODE_TYPE_ACCESS_MODE ||
			      priv->type == ARV_GC_PROPERTY_NODE_TYPE_IMPOSED_ACCESS_MODE, default_value);

	value = _get_value_data (self);

	if (g_strcmp0 (value, "RO") == 0)
		return ARV_GC_ACCESS_MODE_RO;
	else if (g_strcmp0 (value, "WO") == 0)
		return ARV_GC_ACCESS_MODE_WO;
	else if (g_strcmp0 (value, "RW") == 0)
		return ARV_GC_ACCESS_MODE_RW;

	return default_value;
}

ArvGcNode *
arv_gc_property_node_new_cachable (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_CACHABLE);
}

ArvGcCachable
arv_gc_property_node_get_cachable (ArvGcPropertyNode *self, ArvGcCachable default_value)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (self);
	const char *value;

	if (self == NULL)
		return default_value;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (self), default_value);
	g_return_val_if_fail (priv->type == ARV_GC_PROPERTY_NODE_TYPE_CACHABLE, default_value);

	value = _get_value_data (self);

	if (g_strcmp0 (value, "WriteAround") == 0)
		return ARV_GC_CACHABLE_WRITE_AROUND;
	else if (g_strcmp0 (value, "WriteThrough") == 0)
		return ARV_GC_CACHABLE_WRITE_THROUGH;

	return ARV_GC_CACHABLE_NO_CACHE;
}

ArvGcNode *
arv_gc_property_node_new_polling_time (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_POLLING_TIME);
}

ArvGcNode *
arv_gc_property_node_new_endianness (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_ENDIANNESS);
}

guint
arv_gc_property_node_get_endianness (ArvGcPropertyNode *self, guint default_value)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (self);

	if (self == NULL)
		return default_value;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (self), default_value);
	g_return_val_if_fail (priv->type == ARV_GC_PROPERTY_NODE_TYPE_ENDIANNESS, default_value);

	if (g_strcmp0 (_get_value_data (self), "BigEndian") == 0)
		return G_BIG_ENDIAN;

	return G_LITTLE_ENDIAN;
}

ArvGcNode *
arv_gc_property_node_new_sign (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_SIGN);
}

ArvGcSignedness
arv_gc_property_node_get_sign (ArvGcPropertyNode *self, ArvGcSignedness default_value)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (self);

	if (self == NULL)
		return default_value;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (self), default_value);
	g_return_val_if_fail (priv->type == ARV_GC_PROPERTY_NODE_TYPE_SIGN, default_value);

	if (g_strcmp0 (_get_value_data (self), "Unsigned") == 0)
		return ARV_GC_SIGNEDNESS_UNSIGNED;

	return ARV_GC_SIGNEDNESS_SIGNED;
}

ArvGcNode *
arv_gc_property_node_new_lsb (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_LSB);
}

guint
arv_gc_property_node_get_lsb (ArvGcPropertyNode *self, guint default_value)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (self);

	if (self == NULL)
		return default_value;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (self), default_value);
	g_return_val_if_fail (priv->type == ARV_GC_PROPERTY_NODE_TYPE_LSB ||
			      priv->type == ARV_GC_PROPERTY_NODE_TYPE_BIT, default_value);

	return g_ascii_strtoll (_get_value_data (self), NULL, 10);
}

ArvGcNode *
arv_gc_property_node_new_msb (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_MSB);
}

guint
arv_gc_property_node_get_msb (ArvGcPropertyNode *self, guint default_value)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (self);

	if (self == NULL)
		return default_value;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (self), default_value);
	g_return_val_if_fail (priv->type == ARV_GC_PROPERTY_NODE_TYPE_MSB ||
			      priv->type == ARV_GC_PROPERTY_NODE_TYPE_BIT, default_value);

	return g_ascii_strtoll (_get_value_data (self), NULL, 10);
}

ArvGcNode *
arv_gc_property_node_new_bit (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_BIT);
}

ArvGcNode *
arv_gc_property_node_new_command_value (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_COMMAND_VALUE);
}

ArvGcNode *
arv_gc_property_node_new_p_command_value (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_COMMAND_VALUE);
}

ArvGcNode *
arv_gc_property_node_new_chunk_id (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_CHUNK_ID);
}

ArvGcNode *
arv_gc_property_node_new_event_id (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_EVENT_ID);
}

ArvGcNode *
arv_gc_property_node_new_value_default (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_VALUE_DEFAULT);
}

ArvGcNode *
arv_gc_property_node_new_p_value_default (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_VALUE_DEFAULT);
}

ArvGcStreamable
arv_gc_property_node_get_streamable (ArvGcPropertyNode *self, ArvGcStreamable default_value)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (self);
	const char *value;

	if (self == NULL)
		return default_value;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (self), default_value);
	g_return_val_if_fail (priv->type == ARV_GC_PROPERTY_NODE_TYPE_STREAMABLE, default_value);

	value = _get_value_data (self);

	if (g_strcmp0 (value, "Yes") == 0)
		return ARV_GC_STREAMABLE_YES;
	else if (g_strcmp0 (value, "No") == 0)
		return ARV_GC_STREAMABLE_NO;

	return ARV_GC_STREAMABLE_NO;
}

ArvGcNode *
arv_gc_property_node_new_streamable (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_STREAMABLE);
}

ArvGcNode *
arv_gc_property_node_new_is_deprecated (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_IS_DEPRECATED);
}

ArvGcNode *
arv_gc_property_node_new_p_alias (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_ALIAS);
}

ArvGcNode *
arv_gc_property_node_new_p_cast_alias (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_P_CAST_ALIAS);
}

static void
_set_property (GObject * object, guint prop_id,
	       const GValue * value, GParamSpec * pspec)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (ARV_GC_PROPERTY_NODE (object));

	switch (prop_id) {
		case ARV_GC_PROPERTY_NODE_PROPERTY_TYPE:
			priv->type = g_value_get_enum (value);
			break;
		default:
			g_assert_not_reached ();
	}
}

static void
_get_property (GObject * object, guint prop_id,
	       GValue * value, GParamSpec * pspec)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (ARV_GC_PROPERTY_NODE (object));

	switch (prop_id) {
		case ARV_GC_PROPERTY_NODE_PROPERTY_TYPE:
			g_value_set_enum (value, priv->type);
			break;
		default:
			g_assert_not_reached ();
	}
}

static void
arv_gc_property_node_init (ArvGcPropertyNode *self)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (self);

	priv->type = ARV_GC_PROPERTY_NODE_TYPE_UNKNOWN;
	priv->value_data = NULL;
	priv->value_data_up_to_date = FALSE;
}

static void
arv_gc_property_node_finalize (GObject *object)
{
	ArvGcPropertyNodePrivate *priv = arv_gc_property_node_get_instance_private (ARV_GC_PROPERTY_NODE (object));

	G_OBJECT_CLASS (arv_gc_property_node_parent_class)->finalize (object);

	g_free (priv->value_data);
	g_free (priv->name);
}

static void
arv_gc_property_node_class_init (ArvGcPropertyNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvDomElementClass *dom_element_class = ARV_DOM_ELEMENT_CLASS (this_class);

	object_class->finalize = arv_gc_property_node_finalize;
	object_class->set_property = _set_property;
	object_class->get_property = _get_property;
	dom_node_class->get_node_name = arv_gc_property_node_get_node_name;
	dom_node_class->can_append_child = _can_append_child;
	dom_node_class->post_new_child = _post_new_child;
	dom_node_class->pre_remove_child = _pre_remove_child;
	dom_element_class->set_attribute = arv_gc_property_node_set_attribute;
	dom_element_class->get_attribute = arv_gc_property_node_get_attribute;

	g_object_class_install_property (
		object_class, ARV_GC_PROPERTY_NODE_PROPERTY_TYPE,
		g_param_spec_enum ("node-type", "Node type",
				   "Actual node type",
				   ARV_TYPE_GC_PROPERTY_NODE_TYPE,
				   ARV_GC_PROPERTY_NODE_TYPE_UNKNOWN,
				   G_PARAM_READWRITE |  G_PARAM_CONSTRUCT_ONLY)
		);
}
