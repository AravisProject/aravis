/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
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

#include <arvgc.h>
#include <arvgcintegernode.h>
#include <arvgcregister.h>
#include <arvgccommand.h>
#include <arvgcinteger.h>
#include <arvgcswissknife.h>
#include <arvgcconverter.h>
#include <arvgcport.h>
#include <arvdebug.h>
#include <libxml/parser.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

static ArvGcNode *
arv_gc_create_node (ArvGc *genicam, const char *type)
{
	ArvGcNode *node = NULL;

	g_return_val_if_fail (type != NULL, NULL);

	if (strcmp (type, "Category") == 0)
		node = arv_gc_node_new ();
	else if (strcmp (type, "Command") == 0)
		node = arv_gc_command_new ();
	else if (strcmp (type, "Converter") == 0)
		node = arv_gc_converter_new ();
	else if (strcmp (type, "IntConverter") == 0)
		node = arv_gc_int_converter_new ();
	else if (strcmp (type, "IntReg") == 0)
		node = arv_gc_integer_register_new ();
	else if (strcmp (type, "MaskedIntReg") == 0)
		node = arv_gc_masked_integer_register_new ();
	else if (strcmp (type, "FloatReg") == 0)
		node = arv_gc_float_register_new ();
	else if (strcmp (type, "StringReg") == 0)
		node = arv_gc_string_register_new ();
	else if (strcmp (type, "Integer") == 0)
		node = arv_gc_integer_node_new ();
	else if (strcmp (type, "Float") == 0)
		node = arv_gc_node_new ();
	else if (strcmp (type, "Enumeration") == 0)
		node = arv_gc_node_new ();
	else if (strcmp (type, "SwissKnife") == 0)
		node = arv_gc_swiss_knife_new ();
	else if (strcmp (type, "IntSwissKnife") == 0)
		node = arv_gc_int_swiss_knife_new ();
	else if (strcmp (type, "Port") == 0)
		node = arv_gc_port_new ();
	else
		arv_debug (ARV_DEBUG_LEVEL_STANDARD,
			   "[Gc::create_node] Unknown node type (%s)", type);

	if (node != NULL) {
		arv_gc_node_set_genicam (node, genicam);
		arv_debug (ARV_DEBUG_LEVEL_STANDARD,
			   "[Gc::create_node] Node '%s' created", type);
	}

	return node;
}

typedef struct {
	int level;
	ArvGc *genicam;
	ArvGcNode *current_node;
	int current_node_level;

	const char *current_element;
	char **current_attrs;
	GString *current_content;
} ArvGcParserState;

static void
arv_gc_parser_start_document (void *user_data)
{
	ArvGcParserState *state = user_data;

	state->level = 0;
	state->current_node = NULL;
	state->current_node_level = -1;
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
	ArvGcNode *node;

	state->level++;

	if (state->current_node == NULL) {
	       if (state->level > 1) {
		       node = arv_gc_create_node (state->genicam, (char *) name);

		       if (node != NULL) {
			       int i;
			       for (i = 0; attrs[i] != NULL && attrs[i+1] != NULL; i += 2)
				       arv_gc_node_set_attribute (node, (char *) attrs[i], (char *) attrs[i+1]);

			       state->current_node = node;
			       state->current_node_level = state->level;
		       }
	       }
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

static void
arv_gc_parser_end_element (void *user_data,
			   const xmlChar *name)
{
	ArvGcParserState *state = user_data;

	if (state->current_node_level == state->level) {
		const char *node_name;

		node_name = arv_gc_node_get_name (state->current_node);
		if (node_name != NULL) {
			g_hash_table_insert (state->genicam->nodes, (char *) node_name, state->current_node);
			arv_debug (ARV_DEBUG_LEVEL_STANDARD,
				   "[GcParser::start_element] Insert node '%s'", node_name);
		} else
			g_object_unref (state->current_node);

		state->current_node_level = -1;
		state->current_node = NULL;
	} else if (state->current_node_level < state->level &&
		   state->current_node != NULL) {
		arv_gc_node_add_element (state->current_node, state->current_element,
					 state->current_content->str, (const char **) state->current_attrs);
	}

	state->level--;
}

static void
arv_gc_parser_characters (void *user_data,
			  const xmlChar *text,
			  int length)
{
	ArvGcParserState *state = user_data;

	if (state->current_node != NULL)
		g_string_append_len (state->current_content, (char *) text, length);
}

static void
arv_gc_parser_warning (void *user_data, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	g_logv("Genicam", G_LOG_LEVEL_WARNING, msg, args);
	va_end(args);
}

static void
arv_gc_parser_error (void *user_data, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	g_logv("Genicam", G_LOG_LEVEL_CRITICAL, msg, args);
	va_end(args);
}

static void
arv_gc_parser_fatal_error (void *user_data, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	g_logv("Genicam", G_LOG_LEVEL_ERROR, msg, args);
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

ArvGcNode *
arv_gc_get_node	(ArvGc *genicam, const char *name)
{
	g_return_val_if_fail (ARV_IS_GC (genicam), NULL);

	return g_hash_table_lookup (genicam->nodes, name);
}

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
			arv_debug (ARV_DEBUG_LEVEL_STANDARD, "[Gc::set_int64_to_value] Invalid node '%s'",
				   arv_gc_node_get_name (node));
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
			arv_debug (ARV_DEBUG_LEVEL_STANDARD, "[Gc::set_int64_to_value] Invalid node '%s'",
				   arv_gc_node_get_name (node));
	}
}

ArvGc *
arv_gc_new (ArvDevice *device, char *xml, size_t size)
{
	ArvGc *genicam;

	g_return_val_if_fail (xml != NULL, NULL);
	if (size == 0)
		size = strlen (xml);

	genicam = g_object_new (ARV_TYPE_GC, NULL);
	g_return_val_if_fail (genicam != NULL, NULL);
	genicam->device = device;

	arv_gc_parse_xml (genicam, xml, size);

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

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_gc_finalize;
}

G_DEFINE_TYPE (ArvGc, arv_gc, G_TYPE_OBJECT)
