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

#ifndef ARV_GC_ENUMERATION_H
#define ARV_GC_ENUMERATION_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvtypes.h>
#include <arvgcfeaturenode.h>
#include <arvgcpropertynode.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_ENUMERATION (arv_gc_enumeration_get_type ())
G_DECLARE_FINAL_TYPE (ArvGcEnumeration, arv_gc_enumeration, ARV, GC_ENUMERATION, ArvGcFeatureNode)

ArvGcNode * 	arv_gc_enumeration_new 				(void);

const GSList *	arv_gc_enumeration_get_entries			(ArvGcEnumeration *enumeration);

const char *	arv_gc_enumeration_get_string_value		(ArvGcEnumeration *enumeration, GError **error);
gboolean	arv_gc_enumeration_set_string_value		(ArvGcEnumeration *enumeration, const char *value, GError **error);
gint64 		arv_gc_enumeration_get_int_value		(ArvGcEnumeration *enumeration, GError **error);
gboolean	arv_gc_enumeration_set_int_value		(ArvGcEnumeration *enumeration, gint64 value, GError **error);
gint64 *	arv_gc_enumeration_dup_available_int_values	(ArvGcEnumeration *enumeration,	guint *n_values, GError **error);
const char **	arv_gc_enumeration_dup_available_string_values	(ArvGcEnumeration *enumeration,	guint *n_values, GError **error);
const char **	arv_gc_enumeration_dup_available_display_names 	(ArvGcEnumeration *enumeration, guint *n_values, GError **error);

G_END_DECLS

#endif
