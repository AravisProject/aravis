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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvgcfeaturenode
 * @short_description: Base class for all Genicam nodes
 *
 * #ArvGcPropertyNode provides a base class for the implementation of the different
 * types of Genicam node.
 */

#include <arvgcpropertynode.h>
#include <arvgcfeaturenode.h>
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
			return "Tooltip";
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
arv_gc_property_node_can_append_child (ArvDomNode *self, ArvDomNode *child)
{
	return ARV_IS_DOM_TEXT (child);
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

static ArvDomNode *
arv_gc_property_node_get_value_node (ArvGcPropertyNode *property_node)
{
	ArvDomNode *child;
	ArvDomNode *value_node;
	const char *node_name;
	ArvGc *genicam;

	child = arv_dom_node_get_first_child (ARV_DOM_NODE (property_node));
	if (child == NULL)
		return NULL;

	if (property_node->type < ARV_GC_PROPERTY_NODE_TYPE_P_UNKNONW)
		return ARV_DOM_NODE (child);

	node_name = arv_dom_character_data_get_data (ARV_DOM_CHARACTER_DATA (child));
	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (property_node));
	value_node = ARV_DOM_NODE (arv_gc_get_node (genicam, node_name));

	return value_node;
}

const char *
arv_gc_property_node_get_string (ArvGcPropertyNode *node)
{
	ArvDomNode *value_node;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (node), NULL);

	value_node = arv_gc_property_node_get_value_node (node);
	if (value_node == NULL)
		return NULL;

	if (ARV_IS_DOM_TEXT (value_node))
		return arv_dom_character_data_get_data (ARV_DOM_CHARACTER_DATA (value_node));

	return arv_gc_feature_node_get_value_as_string (ARV_GC_FEATURE_NODE (value_node));
}
	
void
arv_gc_property_node_set_string (ArvGcPropertyNode *node, const char *string)
{
	ArvDomNode *value_node;

	g_return_if_fail (ARV_IS_GC_PROPERTY_NODE (node));

	value_node = arv_gc_property_node_get_value_node (node);
	if (value_node == NULL)
		return;

	if (ARV_IS_DOM_TEXT (value_node)) {
		arv_dom_character_data_set_data (ARV_DOM_CHARACTER_DATA (value_node), string);
		return;
	}

	arv_gc_feature_node_set_value_from_string (ARV_GC_FEATURE_NODE (value_node), string);
}

gint64
arv_gc_property_node_get_int64 (ArvGcPropertyNode *node)
{
	const char *string;

	string = arv_gc_property_node_get_string (node);

	if (string != NULL)
		return g_ascii_strtoll (string, NULL, 0);

	 return 0;
}

void
arv_gc_property_node_set_int64 (ArvGcPropertyNode *node, gint64 v_int64)
{
	char *buffer;

	g_return_if_fail (ARV_IS_GC_PROPERTY_NODE (node));

	buffer = g_strdup_printf ("%" G_GINT64_FORMAT, v_int64);
	arv_gc_property_node_set_string (node, buffer);
	g_free (buffer);
}

double
arv_gc_property_node_get_double (ArvGcPropertyNode *node)
{
	const char *string;

	string = arv_gc_property_node_get_string (node);

	if (string != NULL)
		return g_ascii_strtod (string, NULL);

	 return 0.0;
}

void
arv_gc_property_node_set_double (ArvGcPropertyNode *node, double v_double)
{
	char buffer[G_ASCII_DTOSTR_BUF_SIZE];

	g_return_if_fail (ARV_IS_GC_PROPERTY_NODE (node));

	g_ascii_dtostr (buffer, G_ASCII_DTOSTR_BUF_SIZE, v_double);
	arv_gc_property_node_set_string (node, buffer);
}

ArvGcPropertyNodeType
arv_gc_property_node_get_node_type (ArvGcPropertyNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (node), ARV_GC_PROPERTY_NODE_TYPE_UNKNOWN);

	return node->type;
}

ArvGcNode *
arv_gc_property_node_get_linked_node (ArvGcPropertyNode *node)
{
	ArvGc *genicam;
	ArvDomNode *child;

	g_return_val_if_fail (ARV_IS_GC_PROPERTY_NODE (node), NULL);
	g_return_val_if_fail (node->type > ARV_GC_PROPERTY_NODE_TYPE_P_UNKNONW, NULL);

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (node));
	child = arv_dom_node_get_first_child (ARV_DOM_NODE (node));
	return arv_gc_get_node (genicam, arv_dom_character_data_get_data (ARV_DOM_CHARACTER_DATA (child)));
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

static void
arv_gc_property_node_init (ArvGcPropertyNode *gc_property_node)
{
	gc_property_node->type = 0;
}

static void
arv_gc_property_node_finalize (GObject *object)
{
	parent_class->finalize (object);
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
	dom_node_class->can_append_child = arv_gc_property_node_can_append_child;
	dom_element_class->set_attribute = arv_gc_property_node_set_attribute;
	dom_element_class->get_attribute = arv_gc_property_node_get_attribute;
}

G_DEFINE_TYPE (ArvGcPropertyNode, arv_gc_property_node, ARV_TYPE_GC_NODE)
