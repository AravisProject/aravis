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
 */

#include <arvchunkparser.h>

enum {
	ARV_CHUNK_PARSER_PROPERTY_0,
	ARV_CHUNK_PARSER_PROPERTY_GENICAM,
	ARV_CHUNK_PARSER_PROPERTY_LAST
} ArvStreamProperties;

static GObjectClass *parent_class = NULL;

struct _ArvChunkParserPrivate {
	ArvGc *genicam;
};

const char *
arv_chunk_parser_get_string_feature_value (ArvChunkParser *parser, ArvBuffer *buffer, const char *feature)
{
	return NULL;
}

gint64
arv_chunk_parser_get_integer_feature_value (ArvChunkParser *parser, ArvBuffer *buffer, const char *feature)
{
	return 0;
}

double
arv_chunk_parser_get_float_feature_value (ArvChunkParser *parser, ArvBuffer *buffer, const char *feature)
{
	return 0.0;
}

/**
 * arv_chunk_parser_new:
 * @genicam: a #ArvGc
 *
 * Creates a new chunk_parser.
 *
 * Returns: a new #ArvChunkParser object
 *
 * Since: 0.3.3
 */

ArvChunkParser *
arv_chunk_parser_new (ArvGc *genicam)
{
	ArvChunkParser *chunk_parser;

	g_return_val_if_fail (ARV_IS_GC (genicam), NULL);

	chunk_parser = g_object_new (ARV_TYPE_CHUNK_PARSER, "genicam", genicam, NULL);

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
