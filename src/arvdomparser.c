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
 * Author:
 * 	Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#include <arvdebugprivate.h>
#include <arvdomimplementation.h>
#include <arvdomnode.h>
#include <arvdomelement.h>
#include <arvdomparser.h>
#include <arvstr.h>
#include <libxml/parser.h>
#include <gio/gio.h>
#include <string.h>

typedef enum {
	STATE
} ArvDomSaxParserStateEnum;

typedef struct {
	ArvDomSaxParserStateEnum state;

	ArvDomDocument *document;
	ArvDomNode *current_node;

	gboolean is_error;

	int error_depth;

	GHashTable *entities;
} ArvDomSaxParserState;

static void
_free_entity (void *data)
{
	xmlEntity *entity = data;

	xmlFree ((xmlChar *) entity->name);
	xmlFree ((xmlChar *) entity->ExternalID);
	xmlFree ((xmlChar *) entity->SystemID);
	xmlFree (entity->content);
	xmlFree (entity->orig);
	g_free (entity);
}

static void
arv_dom_parser_start_document (void *user_data)
{
	ArvDomSaxParserState *state = user_data;

	state->state = STATE;
	state->is_error = FALSE;
	state->error_depth = 0;
	state->entities = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, _free_entity);
}

static void
arv_dom_parser_end_document (void *user_data)
{
	ArvDomSaxParserState *state = user_data;

	g_hash_table_unref (state->entities);
}

static void
arv_dom_parser_start_element(void *user_data,
			     const xmlChar *name,
			     const xmlChar **attrs)
{
	ArvDomSaxParserState *state = user_data;
	ArvDomNode *node;
	int i;

	if (state->is_error) {
		state->error_depth++;
		return;
	}

	if (state->document == NULL) {
		state->document = arv_dom_implementation_create_document (NULL, (char *) name);
		state->current_node = ARV_DOM_NODE (state->document);

		g_return_if_fail (ARV_IS_DOM_DOCUMENT (state->document));
	}

	node = ARV_DOM_NODE (arv_dom_document_create_element (ARV_DOM_DOCUMENT (state->document), (char *) name));

	if (ARV_IS_DOM_NODE (node) && arv_dom_node_append_child (state->current_node, node) != NULL) {
		if (attrs != NULL)
			for (i = 0; attrs[i] != NULL && attrs[i+1] != NULL; i += 2)
				arv_dom_element_set_attribute (ARV_DOM_ELEMENT (node),
							       (char *) attrs[i],
							       (char *) attrs[i+1]);

		state->current_node = node;
		state->is_error = FALSE;
		state->error_depth = 0;
	} else {
		state->is_error = TRUE;
		state->error_depth = 1;
	}
}

static void
arv_dom_parser_end_element (void *user_data,
			    const xmlChar *name)
{
	ArvDomSaxParserState *state = user_data;

	if (state->is_error) {
		state->error_depth--;
		if (state->error_depth > 0) {
			return;
		}

		state->is_error = FALSE;
		return;
	}

	state->current_node = arv_dom_node_get_parent_node (state->current_node);
}

static void
arv_dom_parser_characters (void *user_data, const xmlChar *ch, int len)
{
	ArvDomSaxParserState *state = user_data;

	if (!state->is_error) {
		ArvDomNode *node;
		char *text;

		text = g_strndup ((char *) ch, len);
		node = ARV_DOM_NODE (arv_dom_document_create_text_node (ARV_DOM_DOCUMENT (state->document), text));

		arv_dom_node_append_child (state->current_node, node);

		g_free (text);
	}
}

static void arv_dom_parser_warning (void *user_data, const char *msg, ...) G_GNUC_PRINTF(2,3);
static void arv_dom_parser_error (void *user_data, const char *msg, ...) G_GNUC_PRINTF(2,3);
static void arv_dom_parser_fatal_error (void *user_data, const char *msg, ...) G_GNUC_PRINTF(2,3);

static void
arv_dom_parser_warning (void *user_data, const char *msg, ...)
{
	va_list args;
        char *message;

	va_start(args, msg);
        message = g_strdup_vprintf (msg, args);
        arv_warning (ARV_DEBUG_CATEGORY_DOM, "[DomParser::parse] %s", message);
        g_free (message);
	va_end(args);
}

static void
arv_dom_parser_error (void *user_data, const char *msg, ...)
{
	va_list args;
        char *message;

	va_start(args, msg);
        message = g_strdup_vprintf (msg, args);
        arv_warning (ARV_DEBUG_CATEGORY_DOM, "[DomParser::parse] %s", message);
        g_free (message);
	va_end(args);
}

static void
arv_dom_parser_fatal_error (void *user_data, const char *msg, ...)
{
	va_list args;
        char *message;

	va_start(args, msg);
        message = g_strdup_vprintf (msg, args);
        arv_warning (ARV_DEBUG_CATEGORY_DOM, "[DomParser::parse] %s", message);
        g_free (message);
	va_end(args);
}

static xmlSAXHandler sax_handler = {
	.warning = arv_dom_parser_warning,
	.error = arv_dom_parser_error,
	.fatalError = arv_dom_parser_fatal_error,
	.startDocument = arv_dom_parser_start_document,
	.endDocument = arv_dom_parser_end_document,
	.startElement = arv_dom_parser_start_element,
	.endElement = arv_dom_parser_end_element,
	.characters = arv_dom_parser_characters
};

static GQuark
arv_dom_document_error_quark (void)
{
	return g_quark_from_static_string ("arv-dom-error-quark");
}

#define ARV_DOM_DOCUMENT_ERROR arv_dom_document_error_quark ()

typedef enum {
	ARV_DOM_DOCUMENT_ERROR_INVALID_XML
} ArvDomDocumentError;

#if LIBXML_VERSION >= 21100
static ArvDomDocument *
_parse_memory (ArvDomDocument *document, ArvDomNode *node,
	       const void *buffer, int size, GError **error)
{
	static ArvDomSaxParserState state;
        xmlParserCtxt *xml_parser_ctxt;

	state.document = document;
	if (node != NULL)
		state.current_node = node;
	else
		state.current_node = ARV_DOM_NODE (document);

	if (size < 0)
		size = strlen (buffer);

        xml_parser_ctxt = xmlNewSAXParserCtxt (&sax_handler, &state);
        if (xml_parser_ctxt == NULL) {
                g_set_error (error,
                             ARV_DOM_DOCUMENT_ERROR,
                             ARV_DOM_DOCUMENT_ERROR_INVALID_XML,
                             "Failed to create parser context");
                return NULL;
        }

        xmlCtxtReadMemory (xml_parser_ctxt, buffer, size, NULL, NULL, 0);

        if (!xml_parser_ctxt->wellFormed) {
                if (state.document !=  NULL)
                        g_object_unref (state.document);
                state.document = NULL;

                arv_warning_dom ("[DomParser::parse] Invalid document");

                g_set_error (error,
                             ARV_DOM_DOCUMENT_ERROR,
                             ARV_DOM_DOCUMENT_ERROR_INVALID_XML,
                             "Invalid document");
        }

        xmlFreeParserCtxt(xml_parser_ctxt);

	return state.document;
}
#else
static ArvDomDocument *
_parse_memory (ArvDomDocument *document, ArvDomNode *node,
	       const void *buffer, int size, GError **error)
{
	static ArvDomSaxParserState state;

	state.document = document;
	if (node != NULL)
		state.current_node = node;
	else
		state.current_node = ARV_DOM_NODE (document);

	if (size < 0)
		size = strlen (buffer);

	if (xmlSAXUserParseMemory (&sax_handler, &state, buffer, size) < 0) {
		if (state.document !=  NULL)
			g_object_unref (state.document);
		state.document = NULL;

		arv_warning_dom ("[ArvDomParser::from_memory] Invalid document");

		g_set_error (error,
			     ARV_DOM_DOCUMENT_ERROR,
			     ARV_DOM_DOCUMENT_ERROR_INVALID_XML,
			     "Invalid document");
	}

	return state.document;
}
#endif

/**
 * arv_dom_document_append_from_memory:
 * @document: a #ArvDomDocument
 * @node: a #ArvDomNode
 * @buffer: a memory buffer holding xml data
 * @size: size of the xml data, in bytes
 * @error: an error placeholder
 *
 * Append a chunk of xml tree to an existing document. The resulting nodes will be appended to
 * @node, or to @document if @node == NULL.
 *
 * Size set to a negative value indicated an unknow xml data size.
 */

void
arv_dom_document_append_from_memory (ArvDomDocument *document, ArvDomNode *node,
				     const void *buffer, int size, GError **error)
{
	g_return_if_fail (ARV_IS_DOM_DOCUMENT (document));
	g_return_if_fail (ARV_IS_DOM_NODE (node) || node == NULL);
	g_return_if_fail (buffer != NULL);

	_parse_memory (document, node, buffer, size, error);
}

ArvDomDocument *
arv_dom_document_new_from_memory (const void *buffer, int size, GError **error)
{
	g_return_val_if_fail (buffer != NULL, NULL);

	return _parse_memory (NULL, NULL, buffer, size, error);
}

static ArvDomDocument *
arv_dom_document_new_from_file (GFile *file, GError **error)
{
	ArvDomDocument *document;
	gsize size = 0;
	char *contents = NULL;

	if (!g_file_load_contents (file, NULL, &contents, &size, NULL, error))
		return NULL;

	document = arv_dom_document_new_from_memory (contents, size, error);

	g_free (contents);

	return document;
}

ArvDomDocument *
arv_dom_document_new_from_path (const char *path, GError **error)
{
	ArvDomDocument *document;
	GFile *file;

	g_return_val_if_fail (path != NULL, NULL);

	file = g_file_new_for_path (path);

	document = arv_dom_document_new_from_file (file, error);

	g_object_unref (file);

	if (document != NULL)
		arv_dom_document_set_path (document, path);

	return document;
}

ArvDomDocument *
arv_dom_document_new_from_url (const char *url, GError **error)
{
	ArvDomDocument *document;
	GFile *file;

	g_return_val_if_fail (url != NULL, NULL);

	file = g_file_new_for_uri (url);

	document = arv_dom_document_new_from_file (file, error);

	g_object_unref (file);

	if (document != NULL)
		arv_dom_document_set_url (document, url);

	return document;
}
