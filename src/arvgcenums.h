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

G_END_DECLS

#endif
