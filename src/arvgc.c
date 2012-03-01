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
 * SECTION:arvgc
 * @short_description: Genicam root document class
 *
 * #ArvGc implements the root document for the storage of the Genicam feature
 * nodes. It builds the node tree by parsing an xml file in the Genicam
 * standard format. See http://www.genicam.org.
 */

#include <arvgc.h>
#include <arvgcnode.h>
#include <arvgcpropertynode.h>
#include <arvgcindexnode.h>
#include <arvgcvariablenode.h>
#include <arvgcinvalidatornode.h>
#include <arvgcregisterdescriptionnode.h>
#include <arvgcgroupnode.h>
#include <arvgccategory.h>
#include <arvgcenumeration.h>
#include <arvgcenumentry.h>
#include <arvgcintegernode.h>
#include <arvgcfloatnode.h>
#include <arvgcregister.h>
#include <arvgccommand.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcboolean.h>
#include <arvgcswissknife.h>
#include <arvgcconverter.h>
#include <arvgcport.h>
#include <arvdebug.h>
#include <arvdomparser.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static gboolean
arv_gc_can_append_child (ArvDomNode *self, ArvDomNode *child)
{
	return ARV_IS_GC_NODE (child);
}

/* ArvDomDocument implementation */

static ArvDomElement *
arv_gc_create_element (ArvDomDocument *document, const char *tag_name)
{
	ArvGcNode *node = NULL;

	if (strcmp (tag_name, "Category") == 0)
		node = arv_gc_category_new ();
	else if (strcmp (tag_name, "Command") == 0)
		node = arv_gc_command_new ();
	else if (strcmp (tag_name, "Converter") == 0)
		node = arv_gc_converter_new ();
	else if (strcmp (tag_name, "IntConverter") == 0)
		node = arv_gc_converter_new_integer ();
	else if (strcmp (tag_name, "IntReg") == 0)
		node = arv_gc_register_new_integer ();
	else if (strcmp (tag_name, "MaskedIntReg") == 0)
		node = arv_gc_register_new_masked_integer ();
	else if (strcmp (tag_name, "FloatReg") == 0)
		node = arv_gc_register_new_float ();
	else if (strcmp (tag_name, "StringReg") == 0)
		node = arv_gc_register_new_string ();
	else if (strcmp (tag_name, "Integer") == 0)
		node = arv_gc_integer_node_new ();
	else if (strcmp (tag_name, "Float") == 0)
		node = arv_gc_float_node_new ();
	else if (strcmp (tag_name, "Boolean") == 0)
		node = arv_gc_boolean_new ();
	else if (strcmp (tag_name, "Enumeration") == 0)
		node = arv_gc_enumeration_new ();
	else if (strcmp (tag_name, "EnumEntry") == 0)
		node = arv_gc_enum_entry_new ();
	else if (strcmp (tag_name, "SwissKnife") == 0)
		node = arv_gc_swiss_knife_new ();
	else if (strcmp (tag_name, "IntSwissKnife") == 0)
		node = arv_gc_swiss_knife_new_integer ();
	else if (strcmp (tag_name, "Port") == 0)
		node = arv_gc_port_new ();
	else if (strcmp (tag_name, "pIndex") == 0)
		node = arv_gc_index_node_new ();
	else if (strcmp (tag_name, "RegisterDescription") == 0)
		node = arv_gc_register_description_node_new ();
	else if (strcmp (tag_name, "pFeature") == 0)
		node = arv_gc_property_node_new_p_feature ();
	else if (strcmp (tag_name, "Value") == 0)
		node = arv_gc_property_node_new_value ();
	else if (strcmp (tag_name, "pValue") == 0)
		node = arv_gc_property_node_new_p_value ();
	else if (strcmp (tag_name, "Address") == 0)
		node = arv_gc_property_node_new_address ();
	else if (strcmp (tag_name, "pAddress") == 0)
		node = arv_gc_property_node_new_p_address ();
	else if (strcmp (tag_name, "Description") == 0)
		node = arv_gc_property_node_new_description ();
	else if (strcmp (tag_name, "Tooltip") == 0)
		node = arv_gc_property_node_new_tooltip ();
	else if (strcmp (tag_name, "DisplayName") == 0)
		node = arv_gc_property_node_new_display_name ();
	else if (strcmp (tag_name, "Min") == 0)
		node = arv_gc_property_node_new_minimum ();
	else if (strcmp (tag_name, "pMin") == 0)
		node = arv_gc_property_node_new_p_minimum ();
	else if (strcmp (tag_name, "Max") == 0)
		node = arv_gc_property_node_new_maximum ();
	else if (strcmp (tag_name, "pMax") == 0)
		node = arv_gc_property_node_new_p_maximum ();
	else if (strcmp (tag_name, "Inc") == 0)
		node = arv_gc_property_node_new_increment ();
	else if (strcmp (tag_name, "pInc") == 0)
		node = arv_gc_property_node_new_p_increment ();
	else if (strcmp (tag_name, "Unit") == 0)
		node = arv_gc_property_node_new_unit ();
	else if (strcmp (tag_name, "OnValue") == 0)
		node = arv_gc_property_node_new_on_value ();
	else if (strcmp (tag_name, "OffValue") == 0)
		node = arv_gc_property_node_new_off_value ();
	else if (strcmp (tag_name, "pIsImplemented") == 0)
		node = arv_gc_property_node_new_p_is_implemented ();
	else if (strcmp (tag_name, "pIsAvailable") == 0)
		node = arv_gc_property_node_new_p_is_available ();
	else if (strcmp (tag_name, "Length") == 0)
		node = arv_gc_property_node_new_length ();
	else if (strcmp (tag_name, "pLength") == 0)
		node = arv_gc_property_node_new_p_length ();
	else if (strcmp (tag_name, "pPort") == 0)
		node = arv_gc_property_node_new_p_port ();
	else if (strcmp (tag_name, "pVariable") == 0)
		node = arv_gc_variable_node_new ();
	else if (strcmp (tag_name, "Formula") == 0)
		node = arv_gc_property_node_new_formula ();
	else if (strcmp (tag_name, "FormulaTo") == 0)
		node = arv_gc_property_node_new_formula_to ();
	else if (strcmp (tag_name, "FormulaFrom") == 0)
		node = arv_gc_property_node_new_formula_from ();
	else if (strcmp (tag_name, "Expression") == 0)
		node = arv_gc_property_node_new_expression ();
	else if (strcmp (tag_name, "Constant") == 0)
		node = arv_gc_property_node_new_constant ();

	else if (strcmp (tag_name, "AccessMode") == 0)
		node = arv_gc_property_node_new_access_mode ();
	else if (strcmp (tag_name, "Cachable") == 0)
		node = arv_gc_property_node_new_cachable ();
	else if (strcmp (tag_name, "PollingTime") == 0)
		node = arv_gc_property_node_new_polling_time ();
	else if (strcmp (tag_name, "Endianess") == 0)
		node = arv_gc_property_node_new_endianess ();
	else if (strcmp (tag_name, "Sign") == 0)
		node = arv_gc_property_node_new_sign ();
	else if (strcmp (tag_name, "LSB") == 0)
		node = arv_gc_property_node_new_lsb ();
	else if (strcmp (tag_name, "MSB") == 0)
		node = arv_gc_property_node_new_msb ();
	else if (strcmp (tag_name, "Bit") == 0)
		node = arv_gc_property_node_new_bit ();
	else if (strcmp (tag_name, "pInvalidator") == 0)
		node = arv_gc_invalidator_node_new ();

	else if (strcmp (tag_name, "Group") == 0)
		node = arv_gc_group_node_new ();
	else
		arv_debug_dom ("[Genicam::create_element] Unknow tag (%s)", tag_name);

	return ARV_DOM_ELEMENT (node);
}

/* ArvGc implementation */

/**
 * arv_gc_get_node:
 * @genicam: a #ArvGc object
 * @name: node name
 * Return value: (transfer none): a #ArvGcNode, null if not found.
 *
 * Retrieves a genicam node by name.
 */

ArvGcNode *
arv_gc_get_node	(ArvGc *genicam, const char *name)
{
	g_return_val_if_fail (ARV_IS_GC (genicam), NULL);
	g_return_val_if_fail (name != NULL, NULL);

	return g_hash_table_lookup (genicam->nodes, name);
}

/**
 * arv_gc_get_device:
 * @genicam: a #ArvGc object
 * Return value: (transfer none): a #ArvDevice.
 *
 * Retrieves the device handled by this genicam interface. The device is used for register access.
 */

ArvDevice *
arv_gc_get_device (ArvGc *genicam)
{
	g_return_val_if_fail (ARV_IS_GC (genicam), NULL);

	return genicam->device;
}

gint64
arv_gc_get_int64_from_value (ArvGc *genicam, GValue *value)
{
	g_return_val_if_fail (ARV_IS_GC (genicam), 0);
	g_return_val_if_fail (G_IS_VALUE (value), 0);

	if (G_VALUE_HOLDS_INT64 (value))
		return g_value_get_int64 (value);
	else if (G_VALUE_HOLDS_STRING (value)) {
		ArvGcNode *node;

		node = arv_gc_get_node (genicam, g_value_get_string (value));
		if (ARV_IS_GC_INTEGER (node))
			return arv_gc_integer_get_value (ARV_GC_INTEGER (node));
		else
			arv_warning_genicam ("[Gc::set_int64_to_value] Invalid node '%s'",
					     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (node)));
	}

	return 0;
}

void
arv_gc_set_int64_to_value (ArvGc *genicam, GValue *value, gint64 v_int64)
{
	g_return_if_fail (ARV_IS_GC (genicam));
	g_return_if_fail (G_IS_VALUE (value));

	if (G_VALUE_HOLDS_INT64 (value))
		return g_value_set_int64 (value, v_int64);
	else if (G_VALUE_HOLDS_STRING (value)) {
		ArvGcNode *node;

		node = arv_gc_get_node (genicam, g_value_get_string (value));
		if (ARV_IS_GC_INTEGER (node))
			arv_gc_integer_set_value (ARV_GC_INTEGER (node), v_int64);
		else
			arv_warning_genicam ("[Gc::set_int64_to_value] Invalid node '%s'",
					     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (node)));
	}
}

double
arv_gc_get_double_from_value (ArvGc *genicam, GValue *value)
{
	g_return_val_if_fail (ARV_IS_GC (genicam), 0);
	g_return_val_if_fail (G_IS_VALUE (value), 0);

	if (G_VALUE_HOLDS_DOUBLE (value))
		return g_value_get_double (value);
	else if (G_VALUE_HOLDS_STRING (value)) {
		ArvGcNode *node;

		node = arv_gc_get_node (genicam, g_value_get_string (value));
		if (ARV_IS_GC_FLOAT (node))
			return arv_gc_float_get_value (ARV_GC_FLOAT (node));
		else
			arv_warning_genicam ("[Gc::set_double_to_value] Invalid node '%s'",
					     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (node)));
	}

	return 0.0;
}

void
arv_gc_set_double_to_value (ArvGc *genicam, GValue *value, double v_double)
{
	g_return_if_fail (ARV_IS_GC (genicam));
	g_return_if_fail (G_IS_VALUE (value));

	if (G_VALUE_HOLDS_DOUBLE (value))
		return g_value_set_double (value, v_double);
	else if (G_VALUE_HOLDS_STRING (value)) {
		ArvGcNode *node;

		node = arv_gc_get_node (genicam, g_value_get_string (value));
		if (ARV_IS_GC_FLOAT (node))
			arv_gc_float_set_value (ARV_GC_FLOAT (node), v_double);
		else
			arv_warning_genicam ("[Gc::set_double_to_value] Invalid node '%s'",
					     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (node)));
	}
}

void
arv_gc_register_feature_node (ArvGc *genicam, ArvGcFeatureNode *node)
{
	const char *name;

	g_return_if_fail (ARV_IS_GC (genicam));
	g_return_if_fail (ARV_IS_GC_FEATURE_NODE (node));


	name = arv_gc_feature_node_get_name (node);
	if (name == NULL)
		return;

	g_object_ref (node);

	g_hash_table_remove (genicam->nodes, (char *) name);
	g_hash_table_insert (genicam->nodes, (char *) name, node);

	arv_log_genicam ("[Gc::register_feature_node] Register node '%s'", name);
}

ArvGc *
arv_gc_new (ArvDevice *device, const void *xml, size_t size)
{
	ArvDomDocument *document;
	ArvGc *genicam;

	document = arv_dom_document_new_from_memory (xml, size, NULL);
	if (!ARV_IS_GC (document)) {
		if (document != NULL)
			g_object_unref (document);
		return NULL;
	}

	genicam = ARV_GC (document);
	genicam->device = device;

	return genicam;
}

static void
arv_gc_init (ArvGc *genicam)
{
	genicam->nodes = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_object_unref);
}

static void
arv_gc_finalize (GObject *object)
{
	ArvGc *genicam = ARV_GC (object);

	g_hash_table_unref (genicam->nodes);

	parent_class->finalize (object);
}

static void
arv_gc_class_init (ArvGcClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);
	ArvDomNodeClass *d_node_class = ARV_DOM_NODE_CLASS (node_class);
	ArvDomDocumentClass *d_document_class = ARV_DOM_DOCUMENT_CLASS (node_class);

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_gc_finalize;
	d_node_class->can_append_child = arv_gc_can_append_child;
	d_document_class->create_element = arv_gc_create_element;
}

G_DEFINE_TYPE (ArvGc, arv_gc, ARV_TYPE_DOM_DOCUMENT)
#if 0
static ArvGcFeatureNode *
arv_gc_create_node (ArvGc *genicam, const char *type)
{
	ArvGcFeatureNode *node = NULL;

	g_return_val_if_fail (type != NULL, NULL);

	if (strcmp (type, "Category") == 0)
		node = arv_gc_category_new ();
	else if (strcmp (type, "Command") == 0)
		node = arv_gc_command_new ();
	else if (strcmp (type, "Converter") == 0)
		node = arv_gc_converter_new ();
	else if (strcmp (type, "IntConverter") == 0)
		node = arv_gc_converter_new_integer ();
	else if (strcmp (type, "IntReg") == 0)
		node = arv_gc_register_new_integer ();
	else if (strcmp (type, "MaskedIntReg") == 0)
		node = arv_gc_register_new_masked_integer ();
	else if (strcmp (type, "FloatReg") == 0)
		node = arv_gc_register_new_float ();
	else if (strcmp (type, "StringReg") == 0)
		node = arv_gc_register_new_string ();
	else if (strcmp (type, "Integer") == 0)
		node = arv_gc_integer_node_new ();
	else if (strcmp (type, "Float") == 0)
		node = arv_gc_float_node_new ();
	else if (strcmp (type, "Boolean") == 0)
		node = arv_gc_boolean_new ();
	else if (strcmp (type, "Enumeration") == 0)
		node = arv_gc_enumeration_new ();
	else if (strcmp (type, "EnumEntry") == 0)
		node = arv_gc_enum_entry_new ();
	else if (strcmp (type, "SwissKnife") == 0)
		node = arv_gc_swiss_knife_new ();
	else if (strcmp (type, "IntSwissKnife") == 0)
		node = arv_gc_swiss_knife_new_integer ();
	else if (strcmp (type, "Port") == 0)
		node = arv_gc_port_new ();

	if (node != NULL) {
		arv_gc_feature_node_set_genicam (node, genicam);
		arv_log_genicam ("[Gc::create_node] Node '%s' created", type);
	}

	return node;
}

typedef struct {
	int level;
	ArvGc *genicam;
	ArvGcFeatureNode *level_2_node;
	ArvGcFeatureNode *level_3_node;

	const char *current_element;
	char **current_attrs;
	GString *current_content;
} ArvGcParserState;

static void
arv_gc_parser_start_document (void *user_data)
{
	ArvGcParserState *state = user_data;

	state->level = 0;
	state->level_2_node = NULL;
	state->level_3_node = NULL;
	state->current_element = NULL;
	state->current_attrs = NULL;
	state->current_content = g_string_new ("");
}

static void
arv_gc_parser_end_document (void *user_data)
{
	ArvGcParserState *state = user_data;

	g_string_free (state->current_content, TRUE);
	g_strfreev (state->current_attrs);
}

static void
arv_gc_parser_start_element(void *user_data,
			    const xmlChar *name,
			    const xmlChar **attrs)
{
	ArvGcParserState *state = user_data;
	ArvGcFeatureNode *node;

	/* Just ignore Group elements */
	if (g_strcmp0 ((char *) name, "Group") == 0)
		return;

	state->level++;

	node = arv_gc_create_node (state->genicam, (char *) name);
	if (node != NULL) {
		int i;
		for (i = 0; attrs[i] != NULL && attrs[i+1] != NULL; i += 2)
			arv_gc_feature_node_set_attribute (node, (char *) attrs[i], (char *) attrs[i+1]);
	}

	if (state->level == 2) {
		state->level_2_node = node;
		if (node != NULL)
			g_object_ref (node);
	} else if (state->level > 2 && state->level_2_node != NULL) {
		if (state->level == 3 && node != NULL) {
			state->level_3_node = node;
			g_object_ref (node);
		} else {
			int n_elements, i;
			state->current_element = (const char *) name;

			g_strfreev (state->current_attrs);

			if (attrs != NULL) {
				for (n_elements = 0; attrs[n_elements] != NULL; n_elements++);
				n_elements++;

				state->current_attrs = g_new (char *, n_elements);
				for (i = 0; i < n_elements; i++)
					state->current_attrs[i] = g_strdup ((char *) attrs[i]);
			} else {
				state->current_attrs = g_new (char *, 1);
				state->current_attrs[0] = NULL;
			}

			g_string_erase (state->current_content, 0, -1);
		}
	}

	if (node != NULL)
		g_object_unref (node);
}

static void
arv_gc_parser_insert_node (ArvGcParserState *state, ArvGcFeatureNode *node)
{
	const char *node_name;

	node_name = arv_gc_feature_node_get_name (node);
	if (node_name != NULL) {
		g_hash_table_insert (state->genicam->nodes, (char *) node_name, node);
		arv_log_genicam ("[GcParser::end_element] Insert node '%s'", node_name);
	} else
		g_object_unref (node);
}

static void
arv_gc_parser_end_element (void *user_data,
			   const xmlChar *name)
{
	ArvGcParserState *state = user_data;

	/* Just ignore Group elements */
	if (g_strcmp0 ((char *) name, "Group") == 0)
		return;

	if (state->level == 2) {
		if (state->level_2_node != NULL) {
			arv_gc_parser_insert_node (state, state->level_2_node);
			state->level_2_node = NULL;
		}
	} else if (state->level > 2) {
		if (state->level == 3 && state->level_3_node != NULL) {
			if (state->level_2_node != NULL &&
			    arv_gc_feature_node_can_add_child (state->level_2_node, state->level_3_node))
				arv_gc_feature_node_add_child (state->level_2_node, state->level_3_node);
			else
				g_object_unref (state->level_3_node);
			state->level_3_node = NULL;
		} else if (state->level == 3 && state->level_2_node != NULL) {
			arv_gc_feature_node_add_element (state->level_2_node, state->current_element,
						 state->current_content->str, (const char **) state->current_attrs);
		} else if (state->level == 4 && state->level_3_node != NULL) {
			arv_gc_feature_node_add_element (state->level_3_node, state->current_element,
						 state->current_content->str, (const char **) state->current_attrs);
		}
	}

	state->level--;
}

static void
arv_gc_parser_characters (void *user_data,
			  const xmlChar *text,
			  int length)
{
	ArvGcParserState *state = user_data;

	g_string_append_len (state->current_content, (char *) text, length);
}

static void
arv_gc_parser_warning (void *user_data, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	g_logv("GcParser", G_LOG_LEVEL_WARNING, msg, args);
	va_end(args);
}

static void
arv_gc_parser_error (void *user_data, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	g_logv("GcParser", G_LOG_LEVEL_CRITICAL, msg, args);
	va_end(args);
}

static void
arv_gc_parser_fatal_error (void *user_data, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	g_logv("GcParser", G_LOG_LEVEL_ERROR, msg, args);
	va_end(args);
}

static xmlSAXHandler sax_handler = {
	.warning = arv_gc_parser_warning,
	.error = arv_gc_parser_error,
	.fatalError = arv_gc_parser_fatal_error,
	.startDocument = arv_gc_parser_start_document,
	.endDocument = arv_gc_parser_end_document,
	.startElement = arv_gc_parser_start_element,
	.endElement = arv_gc_parser_end_element,
	.characters = arv_gc_parser_characters
};

static void
arv_gc_parse_xml (ArvGc *genicam, const char *xml, size_t size)
{
	static ArvGcParserState state;

	state.genicam = genicam;

	xmlSAXUserParseMemory (&sax_handler, &state, xml, size);
}
#endif
