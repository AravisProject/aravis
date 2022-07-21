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

#ifndef ARV_CHUNK_PARSER_H
#define ARV_CHUNK_PARSER_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvgc.h>

G_BEGIN_DECLS

#define ARV_CHUNK_PARSER_ERROR arv_chunk_parser_error_quark()

ARV_API GQuark		arv_chunk_parser_error_quark		(void);

/**
 * ArvChunkParserError:
 * @ARV_CHUNK_PARSER_ERROR_INVALID_FEATURE_TYPE: invalid feature type
 * @ARV_CHUNK_PARSER_ERROR_BUFFER_NOT_FOUND: a buffer is not attached to the chunk parser
 * @ARV_CHUNK_PARSER_ERROR_CHUNK_NOT_FOUND: the requested chunk is not found in the buffer data
 */

typedef enum {
	ARV_CHUNK_PARSER_ERROR_INVALID_FEATURE_TYPE,
	ARV_CHUNK_PARSER_ERROR_BUFFER_NOT_FOUND,
	ARV_CHUNK_PARSER_ERROR_CHUNK_NOT_FOUND
} ArvChunkParserError;

#define ARV_TYPE_CHUNK_PARSER             (arv_chunk_parser_get_type ())
ARV_API G_DECLARE_FINAL_TYPE (ArvChunkParser, arv_chunk_parser, ARV, CHUNK_PARSER, GObject)

ARV_API ArvChunkParser *	arv_chunk_parser_new 			(const char *xml, gsize size);
ARV_API gboolean		arv_chunk_parser_get_boolean_value	(ArvChunkParser *parser, ArvBuffer *buffer,
									 const char *chunk, GError **error);
ARV_API const char *		arv_chunk_parser_get_string_value	(ArvChunkParser *parser, ArvBuffer *buffer,
									 const char *chunk, GError **error);
ARV_API gint64			arv_chunk_parser_get_integer_value	(ArvChunkParser *parser, ArvBuffer *buffer,
									 const char *chunk, GError **error);
ARV_API double			arv_chunk_parser_get_float_value	(ArvChunkParser *parser, ArvBuffer *buffer,
									 const char *chunk, GError **error);

G_END_DECLS

#endif
