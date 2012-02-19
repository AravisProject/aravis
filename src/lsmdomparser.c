/* Aravis
 *
 * Copyright © 2007-2009 Emmanuel Pacaud
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
 * Author:
 * 	Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <lsmdebug.h>
#include <lsmdomimplementation.h>
#include <lsmdomnode.h>
#include <lsmmathmlpresentationtoken.h>
#include <lsmmathmlentitydictionary.h>
#include <lsmsvgtextelement.h>
#include <lsmstr.h>
#include <libxml/parser.h>
#include <gio/gio.h>
#include <string.h>
#include <../itex2mml/itex2MML.h>

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

void
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
	state->document = NULL;
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

		g_return_if_fail (LSM_IS_DOM_DOCUMENT (state->document));
	}

	node = ARV_DOM_NODE (arv_dom_document_create_element (ARV_DOM_DOCUMENT (state->document), (char *) name));

	if (LSM_IS_DOM_NODE (node) && arv_dom_node_append_child (state->current_node, node) != NULL) {
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

static xmlEntityPtr
arv_dom_parser_get_entity (void *user_data, const xmlChar *name)
{
	ArvDomSaxParserState *state = user_data;
	xmlEntity *entity;
	const char *utf8;

	entity = g_hash_table_lookup (state->entities, name);
	if (entity != NULL)
		return entity;

	utf8 = lsm_mathml_entity_get_utf8 ((char *) name);
	if (utf8 != NULL) {
		entity = xmlNewEntity (NULL, name, XML_INTERNAL_GENERAL_ENTITY, NULL, NULL, (xmlChar *) utf8);

		g_hash_table_insert (state->entities, (char *) name, entity);

		return entity;
	}

	return xmlGetPredefinedEntity(name);
}

void
arv_dom_parser_declare_entity (void * user_data, const xmlChar * name, int type,
			       const xmlChar * publicId, const xmlChar * systemId,
			       xmlChar * content)
{
	ArvDomSaxParserState *state = user_data;

	if (content != NULL) {
		xmlEntity *entity;

		entity = xmlNewEntity (NULL, name, type, publicId, systemId, content);

		g_hash_table_insert (state->entities, (char *) name, entity);
	}
}

#if 1
static void
arv_dom_parser_warning (void *user_data, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	g_logv("XML", G_LOG_LEVEL_WARNING, msg, args);
	va_end(args);
}

static void
arv_dom_parser_error (void *user_data, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	g_logv("XML", G_LOG_LEVEL_CRITICAL, msg, args);
	va_end(args);
}

static void
arv_dom_parser_fatal_error (void *user_data, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	g_logv("XML", G_LOG_LEVEL_ERROR, msg, args);
	va_end(args);
}
#endif

static xmlSAXHandler sax_handler = {
#if 1
	.warning = arv_dom_parser_warning,
	.error = arv_dom_parser_error,
	.fatalError = arv_dom_parser_fatal_error,
#endif
	.startDocument = arv_dom_parser_start_document,
	.endDocument = arv_dom_parser_end_document,
	.startElement = arv_dom_parser_start_element,
	.endElement = arv_dom_parser_end_element,
	.characters = arv_dom_parser_characters,
	.getEntity = arv_dom_parser_get_entity,
	.entityDecl = arv_dom_parser_declare_entity
};

static GQuark
arv_dom_document_error_quark (void)
{
	static GQuark q = 0;

        if (q == 0) {
                q = g_quark_from_static_string ("lsm-dom-error-quark");
        }

        return q;
}

#define ARV_DOM_DOCUMENT_ERROR arv_dom_document_error_quark ()

typedef enum {
	ARV_DOM_DOCUMENT_ERROR_INVALID_XML
} ArvDomDocumentError;

ArvDomDocument *
arv_dom_document_new_from_memory (const void *buffer, int size, GError **error)
{
	static ArvDomSaxParserState state;

	g_return_val_if_fail (buffer != NULL, NULL);

	state.document = NULL;

	if (size < 0)
		size = strlen (buffer);

	if (xmlSAXUserParseMemory (&sax_handler, &state, buffer, size) < 0) {
		if (state.document !=  NULL)
			g_object_unref (state.document);
		state.document = NULL;

		lsm_debug_dom ("[ArvDomParser::from_memory] Invalid document");

		g_set_error (error,
			     ARV_DOM_DOCUMENT_ERROR,
			     ARV_DOM_DOCUMENT_ERROR_INVALID_XML,
			     "Invalid document.");
	}

	return state.document;
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

void
arv_dom_document_save_to_stream (ArvDomDocument *document, GOutputStream *stream, GError **error)
{
	g_return_if_fail (LSM_IS_DOM_DOCUMENT (document));
	g_return_if_fail (G_IS_OUTPUT_STREAM (stream));

	arv_dom_node_write_to_stream (ARV_DOM_NODE (document), stream, error);
}

void
arv_dom_document_save_to_memory	(ArvDomDocument *document, void **buffer, int *size, GError **error)
{
	GOutputStream *stream;

	if (buffer != NULL)
		*buffer = NULL;
	if (size != NULL)
		*size = 0;

	g_return_if_fail (document != NULL);
	g_return_if_fail (buffer != NULL);

	stream = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
	if (stream == NULL) {
		*buffer = NULL;
		if (size != NULL)
			*size = 0;
		return;
	}

	arv_dom_document_save_to_stream (document, G_OUTPUT_STREAM (stream), error);
	g_output_stream_close (G_OUTPUT_STREAM (stream), NULL, error);

	if (size != NULL)
		*size = g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (stream));
	*buffer = g_memory_output_stream_steal_data (G_MEMORY_OUTPUT_STREAM (stream));

	g_object_unref (stream);
}

void
arv_dom_document_save_to_path (ArvDomDocument *document, const char *path, GError **error)
{
	GFile *file;
	GFileOutputStream *stream;

	g_return_if_fail (path != NULL);

	file = g_file_new_for_path (path);
	stream = g_file_create (file, G_FILE_CREATE_REPLACE_DESTINATION, NULL, error);
	if (stream != NULL) {
		arv_dom_document_save_to_stream (document, G_OUTPUT_STREAM (stream), error);
		g_object_unref (stream);
	}
	g_object_unref (file);
}

void
arv_dom_document_save_to_url (ArvDomDocument *document, const char *path, GError **error)
{
	GFile *file;
	GFileOutputStream *stream;

	g_return_if_fail (path != NULL);

	file = g_file_new_for_uri (path);
	stream = g_file_create (file, G_FILE_CREATE_REPLACE_DESTINATION, NULL, error);
	if (stream != NULL) {
		arv_dom_document_save_to_stream (document, G_OUTPUT_STREAM (stream), error);
		g_object_unref (stream);
	}
	g_object_unref (file);
}
