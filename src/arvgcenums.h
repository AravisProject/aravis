/* Aravis - Digital camera library
 *
 * Copyright © 2009-2019 Emmanuel Pacaud
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

#ifndef ARV_GC_ENUMS_H
#define ARV_GC_ENUMS_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * ArvGcNameSpace:
 * @ARV_GC_NAME_SPACE_UNDEFINED: undefined name space
 * @ARV_GC_NAME_SPACE_STANDARD: Genicam standardized name space
 * @ARV_GC_NAME_SPACE_CUSTOM: non-standardized name space
 *
 * Name space types. Standardized name space features are documented in Genicam materials.
 */

typedef enum {
	ARV_GC_NAME_SPACE_UNDEFINED = -1,
	ARV_GC_NAME_SPACE_STANDARD,
	ARV_GC_NAME_SPACE_CUSTOM
} ArvGcNameSpace;

typedef enum {
	ARV_GC_ACCESS_MODE_UNDEFINED = -1,
	ARV_GC_ACCESS_MODE_RO,
	ARV_GC_ACCESS_MODE_WO,
	ARV_GC_ACCESS_MODE_RW
} ArvGcAccessMode;

typedef enum {
	ARV_GC_CACHABLE_UNDEFINED = -1,
	ARV_GC_CACHABLE_NO_CACHE,
	ARV_GC_CACHABLE_WRITE_THROUGH,
	ARV_GC_CACHABLE_WRITE_AROUND
} ArvGcCachable;

/**
 * ArvGcSign:
 * @ARV_GC_SIGNEDNESS_UNDEFINED: undefined sign
 * @ARV_GC_SIGNEDNESS_SIGNED: signed integer
 * @ARV_GC_SIGNEDNESS_UNSIGNED: unsigned integer
 */

typedef enum {
	ARV_GC_SIGNEDNESS_UNDEFINED = -1,
	ARV_GC_SIGNEDNESS_SIGNED,
	ARV_GC_SIGNEDNESS_UNSIGNED
} ArvGcSignedness;

typedef enum {
	ARV_GC_IS_LINEAR_UNDEFINED = -1,
	ARV_GC_IS_LINEAR_NO,
	ARV_GC_IS_LINEAR_YES
} ArvGcIsLinear;

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

G_END_DECLS

#endif
