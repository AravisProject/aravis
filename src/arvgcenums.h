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

#ifndef ARV_GC_ENUMS_H
#define ARV_GC_ENUMS_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <glib-object.h>
#include <arvapi.h>

G_BEGIN_DECLS

/**
 * ArvGcNameSpace:
 * @ARV_GC_NAME_SPACE_UNDEFINED: undefined name space
 * @ARV_GC_NAME_SPACE_STANDARD: Genicam standardized name space
 * @ARV_GC_NAME_SPACE_CUSTOM: non-standardized name space
 *
 * Specifies feature node or register name space type. Standard name space features are listed in
 * Genicam materials. Any other vendor-specific features should use custom name space type.
 */

typedef enum {
	ARV_GC_NAME_SPACE_UNDEFINED = -1,
	ARV_GC_NAME_SPACE_STANDARD,
	ARV_GC_NAME_SPACE_CUSTOM
} ArvGcNameSpace;

/**
 * ArvGcAccessMode:
 * @ARV_GC_ACCESS_MODE_UNDEFINED: undefined access mode
 * @ARV_GC_ACCESS_MODE_RO: read-only access
 * @ARV_GC_ACCESS_MODE_WO: write-only access
 * @ARV_GC_ACCESS_MODE_RW: read and write access
 *
 * Specifies access mode for feature nodes and registers.
 */

typedef enum {
	ARV_GC_ACCESS_MODE_UNDEFINED = -1,
	ARV_GC_ACCESS_MODE_RO,
	ARV_GC_ACCESS_MODE_WO,
	ARV_GC_ACCESS_MODE_RW
} ArvGcAccessMode;

ARV_API const char *            arv_gc_access_mode_to_string    (ArvGcAccessMode value);
ARV_API ArvGcAccessMode         arv_gc_access_mode_from_string  (const char *string);

/**
 * ArvGcCachable:
 * @ARV_GC_CACHABLE_UNDEFINED: undefined cache mode
 * @ARV_GC_CACHABLE_NO_CACHE: no value caching
 * @ARV_GC_CACHABLE_WRITE_THROUGH: write-through cache mode
 * @ARV_GC_CACHABLE_WRITE_AROUND: write-around cache mode
 *
 * Specifies caching mode for register values.
 */
typedef enum {
	ARV_GC_CACHABLE_UNDEFINED = -1,
	ARV_GC_CACHABLE_NO_CACHE,
	ARV_GC_CACHABLE_WRITE_THROUGH,
	ARV_GC_CACHABLE_WRITE_AROUND
} ArvGcCachable;

/**
 * ArvGcSignedness:
 * @ARV_GC_SIGNEDNESS_UNDEFINED: undefined sign
 * @ARV_GC_SIGNEDNESS_SIGNED: signed integer
 * @ARV_GC_SIGNEDNESS_UNSIGNED: unsigned integer
 *
 * Specifies signedness of integer registers. Per standard Genicam internally uses signed 64-bit
 * signed integers for representing all integer registers. Therefore unsigned 64-bit integers are
 * not available.
 */

typedef enum {
	ARV_GC_SIGNEDNESS_UNDEFINED = -1,
	ARV_GC_SIGNEDNESS_SIGNED,
	ARV_GC_SIGNEDNESS_UNSIGNED
} ArvGcSignedness;

/**
 * ArvGcIsLinear:
 * @ARV_GC_IS_LINEAR_UNDEFINED: undefined relationship between variables
 * @ARV_GC_IS_LINEAR_NO: non-linear relationship between variables
 * @ARV_GC_IS_LINEAR_YES: linear relationship between variables
 *
 * Describes relationship between TO and FROM variables in Converter feature nodes.
 */

typedef enum {
	ARV_GC_IS_LINEAR_UNDEFINED = -1,
	ARV_GC_IS_LINEAR_NO,
	ARV_GC_IS_LINEAR_YES
} ArvGcIsLinear;

/**
 * ArvGcVisibility:
 * @ARV_GC_VISIBILITY_UNDEFINED: undefined feature visibility level
 * @ARV_GC_VISIBILITY_INVISIBLE: feature should be not be visible in user interface
 * @ARV_GC_VISIBILITY_GURU: very advanced feature to be shown to very experienced users
 * @ARV_GC_VISIBILITY_EXPERT: advanced feature to be shown to expert users
 * @ARV_GC_VISIBILITY_BEGINNER: basic feature to be shown to all users
 *
 * Specifies feature node recommended visibility in user interfaces.
 */

typedef enum {
	ARV_GC_VISIBILITY_UNDEFINED = -1,
	ARV_GC_VISIBILITY_INVISIBLE,
	ARV_GC_VISIBILITY_GURU,
	ARV_GC_VISIBILITY_EXPERT,
	ARV_GC_VISIBILITY_BEGINNER
} ArvGcVisibility;

/**
 * ArvGcRepresentation:
 * @ARV_GC_REPRESENTATION_UNDEFINED: undefined representation
 * @ARV_GC_REPRESENTATION_LINEAR: number presented on linear scale (e.g. on a linear slider)
 * @ARV_GC_REPRESENTATION_LOGARITHMIC: number presented on logarithmic scale (e.g. on a logarithmic slider)
 * @ARV_GC_REPRESENTATION_BOOLEAN: binary choice (e.g. a checkbox)
 * @ARV_GC_REPRESENTATION_PURE_NUMBER: number presented in an editable field (e.g. a spinbox)
 * @ARV_GC_REPRESENTATION_HEX_NUMBER: number presented in hexadecimal format
 * @ARV_GC_REPRESENTATION_IPV4_ADDRESS: IPv4 address
 * @ARV_GC_REPRESENTATION_MAC_ADDRESS: MAC address
 *
 * Number representation formats.
 *
 * Since: 0.8.0
 */

typedef enum {
	ARV_GC_REPRESENTATION_UNDEFINED = -1,
	ARV_GC_REPRESENTATION_LINEAR,
	ARV_GC_REPRESENTATION_LOGARITHMIC,
	ARV_GC_REPRESENTATION_BOOLEAN,
	ARV_GC_REPRESENTATION_PURE_NUMBER,
	ARV_GC_REPRESENTATION_HEX_NUMBER,
	ARV_GC_REPRESENTATION_IPV4_ADDRESS,
	ARV_GC_REPRESENTATION_MAC_ADDRESS
} ArvGcRepresentation;

/**
 * ArvGcDisplayNotation:
 * @ARV_GC_DISPLAY_NOTATION_UNDEFINED: undefined number notation
 * @ARV_GC_DISPLAY_NOTATION_AUTOMATIC: automatically detect whether to use fixed or scientific number notation
 * @ARV_GC_DISPLAY_NOTATION_FIXED: used fixed (i.e. decimal) notation for displaying numbers
 * @ARV_GC_DISPLAY_NOTATION_SCIENTIFIC: use scientific notation for displaying numbers
 *
 * Number display notations for showing numbers in user interfaces.
 *
 * Since: 0.8.0
 */

typedef enum {
	ARV_GC_DISPLAY_NOTATION_UNDEFINED = -1,
	ARV_GC_DISPLAY_NOTATION_AUTOMATIC,
	ARV_GC_DISPLAY_NOTATION_FIXED,
	ARV_GC_DISPLAY_NOTATION_SCIENTIFIC
} ArvGcDisplayNotation;

/**
 * ArvGcStreamable:
 * @ARV_GC_STREAMABLE_UNDEFINED: undefined streamable
 * @ARV_GC_STREAMABLE_NO: the feature can't be used for camera state persistence
 * @ARV_GC_STREAMABLE_YES: the feature can be used for camera state persistence
 *
 * Denotes that the corresponding feature is prepared to be stored to and loaded from a file via the node tree.
 * The idea is to persist the state of a camera by storing the features marked as Streamable and restore the state by
 * writing those features back to the node tree.
 *
 * Since: 0.8.8
 */
typedef enum {
	ARV_GC_STREAMABLE_UNDEFINED = -1,
	ARV_GC_STREAMABLE_NO,
	ARV_GC_STREAMABLE_YES
} ArvGcStreamable;

G_END_DECLS

#endif
