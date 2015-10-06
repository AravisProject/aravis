/* Aravis - Digital camera library
 *
 * Copyright © 2009-2014 Emmanuel Pacaud
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
 * SECTION: arvchunkparser
 * @short_description: Parser for extraction of chunk data from buffers
 *
 * #ArvChunkParser provides a class for the instantiation of chunk parsers used
 * for the extraction of chunk data stored in the stream payload.
 *
 * Chunks are tagged blocks of data stored in a #ArvBuffer containing
 * a #ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA payload. The tags allow a chunk
 * parser to dissect the data payload into its elements and to identify the content.
 *
 * Chunk data are enabled using either arv_camera_set_chunks() or
 * arv_camera_set_chunk_mode(). Both functions are simple convenience wrappers
 * that handle the setting of ChunkModeActive, ChunkSelector and ChunkEnable
 * GENICAM features.
 *
 * <example id="arv-chunk-parser-test"><title>Example use of the ArvChunkParser API</title>
 * <programlisting>
 * <xi:include xmlns:xi="http://www.w3.org/2001/XInclude" parse="text" href="../../../../tests/arvchunkparsertest.c">
 *   <xi:fallback>FIXME: MISSING XINCLUDE CONTENT</xi:fallback>
 * </xi:include>
 * </programlisting>
 * </example>
 */

#include <arvchunkparserprivate.h>
#include <arvbuffer.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcstring.h>
#include <arvdebug.h>

enum {
	ARV_CHUNK_PARSER_PROPERTY_0,
	ARV_CHUNK_PARSER_PROPERTY_GENICAM,
	ARV_CHUNK_PARSER_PROPERTY_LAST
} ArvStreamProperties;

static GObjectClass *parent_class = NULL;

GQuark
arv_chunk_parser_error_quark (void)
{
	return g_quark_from_static_string ("arv-chunk-parser-error-quark");
}

struct _ArvChunkParserPrivate {
	ArvGc *genicam;
};

/**
 * arv_chunk_parser_get_string_value:
 * @parser: a #ArvChunkParser
 * @buffer: a #ArvBuffer with a #ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA payload
 * @chunk: chunk data name
 *
 * Gets the value of chunk data as a string.
 *
 * Returns: the chunk data string value.
 */

const char *
arv_chunk_parser_get_string_value (ArvChunkParser *parser, ArvBuffer *buffer, const char *chunk)
{
	ArvGcNode *node;
	const char *string = NULL;

	g_return_val_if_fail (ARV_IS_CHUNK_PARSER (parser), NULL);
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), NULL);

	node = arv_gc_get_node (parser->priv->genicam, chunk);
	arv_gc_set_buffer (parser->priv->genicam, buffer);

	if (ARV_IS_GC_STRING (node))
		string = arv_gc_string_get_value (ARV_GC_STRING (node), NULL);
	else
		arv_warning_device ("[ArvChunkParser::get_string_value] Node '%s' is not a string", chunk);

	return string;
}

/**
 * arv_chunk_parser_get_integer_value:
 * @parser: a #ArvChunkParser
 * @buffer: a #ArvBuffer with a #ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA payload
 * @chunk: chunk data name
 *
 * Gets the value of chunk data as an integer.
 *
 * Returns: the chunk data integer value.
 */

gint64
arv_chunk_parser_get_integer_value (ArvChunkParser *parser, ArvBuffer *buffer, const char *chunk)
{
	ArvGcNode *node;
	gint64 value = 0;

	g_return_val_if_fail (ARV_IS_CHUNK_PARSER (parser), 0.0);
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0.0);

	node = arv_gc_get_node (parser->priv->genicam, chunk);
	arv_gc_set_buffer (parser->priv->genicam, buffer);

	if (ARV_IS_GC_INTEGER (node))
		value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), NULL);
	else
		arv_warning_device ("[ArvChunkParser::get_integer_value] Node '%s' is not an integer", chunk);

	return value;
}

/**
 * arv_chunk_parser_get_float_value:
 * @parser: a #ArvChunkParser
 * @buffer: a #ArvBuffer with a #ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA payload
 * @chunk: chunk data name
 *
 * Gets the value of chunk data as a float.
 *
 * Returns: the chunk data float value.
 */

double
arv_chunk_parser_get_float_value (ArvChunkParser *parser, ArvBuffer *buffer, const char *chunk)
{
	ArvGcNode *node;
	double value = 0.0;

	g_return_val_if_fail (ARV_IS_CHUNK_PARSER (parser), 0.0);
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0.0);

	node = arv_gc_get_node (parser->priv->genicam, chunk);
	arv_gc_set_buffer (parser->priv->genicam, buffer);

	if (ARV_IS_GC_FLOAT (node))
		value = arv_gc_float_get_value (ARV_GC_FLOAT (node), NULL);
	else 
		arv_warning_chunk ("[ArvChunkParser::get_float_value] Node '%s' is not a float", chunk);

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
			parser->priv->genicam = g_object_ref (g_value_get_object (value));
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
	chunk_parser->priv = G_TYPE_INSTANCE_GET_PRIVATE (chunk_parser, ARV_TYPE_CHUNK_PARSER, ArvChunkParserPrivate);
}

static void
_finalize (GObject *object)
{
	ArvChunkParser *chunk_parser = ARV_CHUNK_PARSER (object);

	g_clear_object (&chunk_parser->priv->genicam);

	parent_class->finalize (object);
}

static void
arv_chunk_parser_class_init (ArvChunkParserClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	g_type_class_add_private (node_class, sizeof (ArvChunkParserPrivate));

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = _finalize;
	object_class->set_property = _set_property;
	object_class->get_property = _get_property;

	g_object_class_install_property (
		object_class, ARV_CHUNK_PARSER_PROPERTY_GENICAM,
		g_param_spec_object ("genicam", "genicam",
				     "Genicam instance", ARV_TYPE_GC,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)
		);
}

G_DEFINE_TYPE (ArvChunkParser, arv_chunk_parser, G_TYPE_OBJECT)
