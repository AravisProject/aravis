/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_GC_ENUM_ENTRY_H
#define ARV_GC_ENUM_ENTRY_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvgcfeaturenode.h>
#include <arvgcpropertynode.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_ENUM_ENTRY             (arv_gc_enum_entry_get_type ())
ARV_API G_DECLARE_FINAL_TYPE (ArvGcEnumEntry, arv_gc_enum_entry, ARV, GC_ENUM_ENTRY, ArvGcFeatureNode)

ARV_API ArvGcNode *	arv_gc_enum_entry_new		(void);

ARV_API gint64		arv_gc_enum_entry_get_value	(ArvGcEnumEntry *entry, GError **error);

G_END_DECLS

#endif
