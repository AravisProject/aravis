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
 * SECTION: arvgcpropertynode
 * @short_description: Base class for Genicam property nodes
 *
 * #ArvGcPropertyNode provides a base class for the implementation of the different
 * types of Genicam property nodes (Value, pValue, Endianess...).
 */

#include <arvgcpropertynode.h>
#include <arvgcfeaturenode.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcstring.h>
#include <arvgc.h>
#include <arvdomtext.h>
#include <arvmisc.h>
#include <arvdebug.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_gc_property_node_get_node_name (ArvDomNode *node)
{
	ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (node);

	switch (property_node->type) {
		case ARV_GC_PROPERTY_NODE_TYPE_VALUE:
			return "Value";
		case ARV_GC_PROPERTY_NODE_TYPE_ADDRESS:
			return "Address";
		case ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION:
			return "Description";
		case ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP:
			return "ToolTip";
		case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME:
			return "DisplayName";
		case ARV_GC_PROPERTY_NODE_TYPE_MINIMUM:
			return "Min";
		case ARV_GC_PROPERTY_NODE_TYPE_MAXIMUM:
			return "Max";
		case ARV_GC_PROPERTY_NODE_TYPE_INCREMENT:
			return "Inc";
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
		case ARV_GC_PROPERTY_NODE_TYPE_CACHABLE:
			return "Cachable";
		case ARV_GC_PROPERTY_NODE_TYPE_POLLING_TIME:
			return "PollingTime";
		case ARV_GC_PROPERTY_NODE_TYPE_ENDIANESS:
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
	ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (parent);

	property_node->value_data_up_to_date = FALSE;
}

static void
_pre_remove_child (ArvDomNode *parent, ArvDomNode *child)
{
	ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (parent);

	property_node->value_data_up_to_date = FALSE;
}

/* ArvDomElement implementation */

static void
arv_gc_property_node_set_attribute (ArvDomElement *self, const char *name, const char *value)
{
}

static const char *
arv_gc_property_node_get_attribute (ArvDomElement *self, const char *name)
{
	return NULL;
}

/* ArvGcPropertyNode implementation */

static const char *
_get_value_data (ArvGcPropertyNode *property_node)
{
	ArvDomNode *dom_node = ARV_DOM_NODE (property_node);

	if (!property_node->value_data_up_to_date) {
		ArvDomNode *iter;
		GString *string = g_string_new (NULL);

		for (iter = dom_node->first_child; iter != NULL; iter = iter->next_sibling)
			g_string_append (string, arv_dom_character_data_get_data (ARV_DOM_CHARACTER_DATA (iter)));
		g_free (property_node->value_data);
		property_node->value_data = string->str;
		g_string_free (string, FALSE);
		property_node->value_data_up_to_date = TRUE;
	}

	return property_node->value_data;
}

static void
_set_value_data (ArvGcPropertyNode *property_node, const char *data)
{
	ArvDomNode *dom_node = ARV_DOM_NODE (property_node);

	if (dom_node->first_child != NULL) {
		ArvDomNode *iter;

		arv_dom_character_data_set_data (ARV_DOM_CHARACTER_DATA (dom_node->first_child), data);
		for (iter = dom_node->first_child->next_sibling; iter != NULL; iter = iter->next_sibling)
			arv_dom_character_data_set_data (ARV_DOM_CHARACTER_DATA (iter), "");
	}

	g_free (property_node->value_data);
	property_node->value_data = g_strdup (data);
	property_node->value_data_up_to_date = TRUE;
}

static ArvDomNode *
_get_pvalue_node (ArvGcPropertyNode *property_node)
{
	ArvDomNode *pvalue_node;
	const char *node_name;
	ArvGc *genicam;

	if (property_node->type < ARV_GC_PROPERTY_NODE_TYPE_P_UNKNONW)
		return NULL;

	node_name = _get_value_data (property_node);
	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (property_node));
	pvalue_node = ARV_DOM_NODE (arv_gc_get_node (genicam, node_name));

	return pvalue_node;
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
		GError *local_error = NULL;
		gint64 value;

		value = arv_gc_integer_get_value (ARV_GC_INTEGER (pvalue_node), &local_error);

		if (local_error != NULL)
			g_propagate_error (error, local_error);

		return value;
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
		GError *local_error = NULL;

		arv_gc_integer_set_value (ARV_GC_INTEGER (pvalue_node), v_int64, &local_error);

		if (local_error != NULL)
			g_propagate_error (error, local_error);

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
		GError *local_error = NULL;
		double value;

		value = arv_gc_float_get_value (ARV_GC_FLOAT (pvalue_node), &local_error);

		if (local_error != NULL)
			g_propagate_error (error, local_error);

		return value;
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
		GError *local_error = NULL;

		arv_gc_float_set_value (ARV_GC_FLOAT (pvalue_node), v_double, &local_error);

		if (local_error != NULL)
			g_propagate_error (error, local_error);

		return;
	}

	arv_warning_genicam ("[GcPropertyNode::set_double] Invalid linked node '%s'",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (pvalue_node)));
}

ArvGcPropertyNodeType
arv_gc_property_node_get_node_type (ArvGcPropertyNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (node), ARV_GC_PROPERTY_NODE_TYPE_UNKNOWN);

	return node->type;
}

/**
 * arv_gc_property_node_get_linked_node:
 * @node: a #ArvGcPropertyNode
 *
 * Returns: (transfer none): the #ArvGcNode which @node points to.
 */

ArvGcNode *
arv_gc_property_node_get_linked_node (ArvGcPropertyNode *node)
{
	ArvGc *genicam;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (node), NULL);
	g_return_val_if_fail (node->type > ARV_GC_PROPERTY_NODE_TYPE_P_UNKNONW, NULL);

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (node));
	return arv_gc_get_node (genicam, _get_value_data (node));
}

static ArvGcNode *
arv_gc_property_node_new (ArvGcPropertyNodeType type)
{
	ArvGcPropertyNode *node;

	node = g_object_new (ARV_TYPE_GC_PROPERTY_NODE, NULL);
	node->type = type;

	return ARV_GC_NODE (node);
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
arv_gc_property_node_new_cachable (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_CACHABLE);
}

ArvGcNode *
arv_gc_property_node_new_polling_time (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_POLLING_TIME);
}

ArvGcNode *
arv_gc_property_node_new_endianess (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_ENDIANESS);
}

ArvGcNode *
arv_gc_property_node_new_sign (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_SIGN);
}

ArvGcNode *
arv_gc_property_node_new_lsb (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_LSB);
}

ArvGcNode *
arv_gc_property_node_new_msb (void)
{
	return arv_gc_property_node_new (ARV_GC_PROPERTY_NODE_TYPE_MSB);
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

static void
arv_gc_property_node_init (ArvGcPropertyNode *gc_property_node)
{
	gc_property_node->type = 0;
	gc_property_node->value_data = NULL;
	gc_property_node->value_data_up_to_date = FALSE;
}

static void
arv_gc_property_node_finalize (GObject *object)
{
	ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (object);

	parent_class->finalize (object);

	g_free (property_node->value_data);
}

static void
arv_gc_property_node_class_init (ArvGcPropertyNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvDomElementClass *dom_element_class = ARV_DOM_ELEMENT_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_property_node_finalize;
	dom_node_class->get_node_name = arv_gc_property_node_get_node_name;
	dom_node_class->can_append_child = _can_append_child;
	dom_node_class->post_new_child = _post_new_child;
	dom_node_class->pre_remove_child = _pre_remove_child;
	dom_element_class->set_attribute = arv_gc_property_node_set_attribute;
	dom_element_class->get_attribute = arv_gc_property_node_get_attribute;
}

G_DEFINE_TYPE (ArvGcPropertyNode, arv_gc_property_node, ARV_TYPE_GC_NODE)
