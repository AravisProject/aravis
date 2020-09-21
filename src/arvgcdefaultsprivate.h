/* Aravis - Digital camera library
 *
 * Copyright © 2009-2020 Emmanuel Pacaud
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

#ifndef ARV_GC_DEFAULTS_PRIVATE_H
#define ARV_GC_DEFAULTS_PRIVATE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <glib-object.h>
#include <arvgcenums.h>

G_BEGIN_DECLS

/* Node default values */
#define ARV_GC_OFF_VALUE_DEFAULT		0
#define ARV_GC_ON_VALUE_DEFAULT			1
/* TODO #define ARV_GC_STREAMABLE_DEFAULT	ARV_GC_STREAMABLE_NO */
#define ARV_GC_DISPLAY_NOTATION_DEFAULT		ARV_GC_DISPLAY_NOTATION_AUTOMATIC
#define ARV_GC_DISPLAY_PRECISION_DEFAULT	6
#define ARV_GC_IS_LINEAR_DEFAULT		ARV_GC_IS_LINEAR_NO
#define ARV_GC_REPRESENTATION_DEFAULT		ARV_GC_REPRESENTATION_PURE_NUMBER
/* TODO #define ARV_GC_SLOPE_DEFAULT		ARV_GC_SLOPE_AUTOMATIC */
#define ARV_GC_UNIT_DEFAULT			""
/* TODO #define ARV_GC_IS_SELF_CLEARING_DEFAULT	ARV_GC_IS_SELF_CLEARING_NO */
#define ARV_GC_CACHABLE_DEFAULT			ARV_GC_CACHABLE_WRITE_AROUND
#define ARV_GC_SIGN_DEFAULT			ARV_GC_SIGNEDNESS_UNSIGNED
/* TODO #define ARV_GC_IS_DEPRECATED_DEFAULT	ARV_GC_IS_DEPRECATED_NO */
#define ARV_GC_IMPOSED_ACCESS_MODE_DEFAULT	ARV_GC_ACCESS_MODE_RW
#define ARV_GC_VISIBILITY_DEFAULT		ARV_GC_VISIBILITY_BEGINNER
/* TODO #define ARV_GC_CACHE_CHUNK_DATA		ARV_GC_CACHE_CHUNK_DATA_NO */
/* TODO #define ARV_GC_SWAP_ENDIANNESS_DEFAULT	ARV_GC_SWAP_ENDIANNESS_NO */
#define ARV_GC_FLOAT_MIN_DEFAULT		-G_MAXDOUBLE
#define ARV_GC_FLOAT_MAX_DEFAULT		G_MAXDOUBLE
#define ARV_GC_INTEGER_INC_DEFAULT		1
#define ARV_GC_INTEGER_MIN_DEFAULT		G_MININT64
#define ARV_GC_INTEGER_MAX_DEFAULT		G_MAXINT64

/* Node attribute defaults */
#define ARV_GC_NAME_SPACE_DEFAULT		ARV_GC_NAME_SPACE_CUSTOM
#define ARV_GC_MERGE_PRIORITY_DEFAULT		0

G_END_DECLS

#endif
