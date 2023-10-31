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
 * ArvChunkParser:
 *
 * [class@ArvChunkParser] provides a class for the instantiation of chunk parsers used for the extraction of chunk data
 * stored in the stream payload.
 *
 * Chunks are tagged blocks of data stored in a [class@ArvBuffer] containing a @ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA
 * payload. The tags allow a chunk parser to dissect the data payload into its elements and to identify the content.
 *
 * Chunk data are enabled using either [method@ArvCamera.set_chunks] or [method@ArvCamera.set_chunk_mode]. Both
 * functions are simple convenience wrappers that handle the setting of ChunkModeActive, ChunkSelector and ChunkEnable
 * GENICAM features.
 *
 * Here is an example of this API in use: [tests/arvchunkparsertest.c](https://github.com/AravisProject/aravis/blob/main/tests/arvchunkparsertest.c)
 */

#include <arvchunkparserprivate.h>
#include <arvbuffer.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcregister.h>
#include <arvgcstring.h>
#include <arvgcboolean.h>
#include <arvdebugprivate.h>

enum {
	ARV_CHUNK_PARSER_PROPERTY_0,
	ARV_CHUNK_PARSER_PROPERTY_GENICAM,
	ARV_CHUNK_PARSER_PROPERTY_LAST
} ArvChunkParserProperties;

GQuark
arv_chunk_parser_error_quark (void)
{
	return g_quark_from_static_string ("arv-chunk-parser-error-quark");
}

typedef struct {
	ArvGc *genicam;
} ArvChunkParserPrivate;

struct _ArvChunkParser {
	GObject	object;

	ArvChunkParserPrivate *priv;
};

struct _ArvChunkParserClass {
	GObjectClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvChunkParser, arv_chunk_parser, G_TYPE_OBJECT, G_ADD_PRIVATE (ArvChunkParser))

/**
 * arv_chunk_parser_get_boolean_value:
 * @parser: a #ArvChunkParser
 * @buffer: a #ArvBuffer with a #ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA payload
 * @chunk: chunk data name
 * @error: a #GError placeholder
 *
 * Returns: the boolean chunk data value.
 */

gboolean
arv_chunk_parser_get_boolean_value (ArvChunkParser *parser, ArvBuffer *buffer, const char *chunk, GError **error)
{
	ArvGcNode *node;
	gboolean value = FALSE;

	g_return_val_if_fail (ARV_IS_CHUNK_PARSER (parser), 0.0);
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0.0);

	node = arv_gc_get_node (parser->priv->genicam, chunk);
	arv_gc_set_buffer (parser->priv->genicam, buffer);

	if (ARV_IS_GC_BOOLEAN (node)) {
		GError *local_error = NULL;

		value = arv_gc_boolean_get_value (ARV_GC_BOOLEAN (node), &local_error);

		if (local_error != NULL) {
			arv_warning_chunk ("%s", local_error->message);
			g_propagate_error (error, local_error);
		}
	} else {
		g_set_error (error, ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_ERROR_INVALID_FEATURE_TYPE,
			     "[%s] Not a boolean", chunk);
	}

	return value;
}

/**
 * arv_chunk_parser_get_string_value:
 * @parser: a #ArvChunkParser
 * @buffer: a #ArvBuffer with a #ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA payload
 * @chunk: chunk data name
 * @error: a #GError placeholder
 *
 * Returns: the string chunk data value.
 */

const char *
arv_chunk_parser_get_string_value (ArvChunkParser *parser, ArvBuffer *buffer, const char *chunk, GError **error)
{
	ArvGcNode *node;
	const char *string = NULL;

	g_return_val_if_fail (ARV_IS_CHUNK_PARSER (parser), NULL);
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), NULL);

	node = arv_gc_get_node (parser->priv->genicam, chunk);
	arv_gc_set_buffer (parser->priv->genicam, buffer);

	if (ARV_IS_GC_STRING (node)) {
		GError *local_error = NULL;

		string = arv_gc_string_get_value (ARV_GC_STRING (node), &local_error);

		if (local_error != NULL) {
			arv_warning_chunk ("%s", local_error->message);
			g_propagate_error (error, local_error);
		}
	} else {
		g_set_error (error, ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_ERROR_INVALID_FEATURE_TYPE,
			     "[%s] Not a string", chunk);
	}

	return string;
}

/**
 * arv_chunk_parser_get_integer_value:
 * @parser: a #ArvChunkParser
 * @buffer: a #ArvBuffer with a #ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA payload
 * @chunk: chunk data name
 * @error: a #GError placeholder
 *
 * Returns: the integer chunk data integer.
 */

gint64
arv_chunk_parser_get_integer_value (ArvChunkParser *parser, ArvBuffer *buffer, const char *chunk, GError **error)
{
	ArvGcNode *node;
	gint64 value = 0;

	g_return_val_if_fail (ARV_IS_CHUNK_PARSER (parser), 0.0);
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0.0);

	node = arv_gc_get_node (parser->priv->genicam, chunk);
	arv_gc_set_buffer (parser->priv->genicam, buffer);

	if (ARV_IS_GC_INTEGER (node)) {
		GError *local_error = NULL;

		value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), &local_error);

		if (local_error != NULL) {
			arv_warning_chunk ("%s", local_error->message);
			g_propagate_error (error, local_error);
		}
	} else {
		g_set_error (error, ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_ERROR_INVALID_FEATURE_TYPE,
			     "[%s] Not an integer", chunk);
	}

	return value;
}

/**
 * arv_chunk_parser_get_float_value:
 * @parser: a #ArvChunkParser
 * @buffer: a #ArvBuffer with a #ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA payload
 * @chunk: chunk data name
 * @error: a #GError placeholder
 *
 * Returns: the float chunk data value.
 */

double
arv_chunk_parser_get_float_value (ArvChunkParser *parser, ArvBuffer *buffer, const char *chunk, GError **error)
{
	ArvGcNode *node;
	double value = 0.0;

	g_return_val_if_fail (ARV_IS_CHUNK_PARSER (parser), 0.0);
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0.0);

	node = arv_gc_get_node (parser->priv->genicam, chunk);
	arv_gc_set_buffer (parser->priv->genicam, buffer);

	if (ARV_IS_GC_FLOAT (node)) {
		GError *local_error = NULL;

		value = arv_gc_float_get_value (ARV_GC_FLOAT (node), &local_error);

		if (local_error != NULL) {
			arv_warning_chunk ("%s", local_error->message);
			g_propagate_error (error, local_error);
		}
	} else {
		g_set_error (error, ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_ERROR_INVALID_FEATURE_TYPE,
			     "[%s] Not a float", chunk);
	}

	return value;
}

/**
 * arv_chunk_parser_new:
 * @xml: XML genicam data
 * @size: genicam data size, -1 if NULL terminated
 *
 * Creates a new chunk_parser.
 *
 * Returns: a new #ArvChunkParser object
 *
 * Since: 0.4.0
 */

ArvChunkParser *
arv_chunk_parser_new (const char *xml, gsize size)
{
	ArvChunkParser *chunk_parser;
	ArvGc *genicam;

	genicam = arv_gc_new (NULL, xml, size);

	g_return_val_if_fail (ARV_IS_GC (genicam), NULL);

	chunk_parser = g_object_new (ARV_TYPE_CHUNK_PARSER, "genicam", genicam, NULL);

	g_object_unref (genicam);

	return chunk_parser;
}

static void
_set_property (GObject * object, guint prop_id,
	       const GValue * value, GParamSpec * pspec)
{
	ArvChunkParser *parser = ARV_CHUNK_PARSER (object);

	switch (prop_id) {
		case ARV_CHUNK_PARSER_PROPERTY_GENICAM:
			g_clear_object (&parser->priv->genicam);
			parser->priv->genicam = g_value_dup_object (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
_get_property (GObject * object, guint prop_id,
			 GValue * value, GParamSpec * pspec)
{
	ArvChunkParser *parser = ARV_CHUNK_PARSER (object);

	switch (prop_id) {
		case ARV_CHUNK_PARSER_PROPERTY_GENICAM:
			g_value_set_object (value, parser->priv->genicam);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_chunk_parser_init (ArvChunkParser *chunk_parser)
{
	chunk_parser->priv = arv_chunk_parser_get_instance_private (chunk_parser);
}

static void
_finalize (GObject *object)
{
	ArvChunkParser *chunk_parser = ARV_CHUNK_PARSER (object);

	g_clear_object (&chunk_parser->priv->genicam);

	G_OBJECT_CLASS (arv_chunk_parser_parent_class)->finalize (object);
}

static void
arv_chunk_parser_class_init (ArvChunkParserClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	object_class->finalize = _finalize;
	object_class->set_property = _set_property;
	object_class->get_property = _get_property;

	/**
	 * ArvChunkParser:genicam:
	 *
	 * Internal Genicam object
	 *
	 * Stability: Private
	 */

	g_object_class_install_property (
		object_class, ARV_CHUNK_PARSER_PROPERTY_GENICAM,
		g_param_spec_object ("genicam", "genicam",
				     "Genicam instance", ARV_TYPE_GC,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)
		);
}
