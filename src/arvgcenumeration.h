/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_GC_ENUMERATION_H
#define ARV_GC_ENUMERATION_H

#include <arvtypes.h>
#include <arvgcnode.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_ENUMERATION             (arv_gc_enumeration_get_type ())
#define ARV_GC_ENUMERATION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_ENUMERATION, ArvGcEnumeration))
#define ARV_GC_ENUMERATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_ENUMERATION, ArvGcEnumerationClass))
#define ARV_IS_GC_ENUMERATION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_ENUMERATION))
#define ARV_IS_GC_ENUMERATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_ENUMERATION))
#define ARV_GC_ENUMERATION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_ENUMERATION, ArvGcEnumerationClass))

typedef struct _ArvGcEnumerationClass ArvGcEnumerationClass;

struct _ArvGcEnumeration {
	ArvGcNode	node;

	GValue value;
};

struct _ArvGcEnumerationClass {
	ArvGcNodeClass parent_class;
};

GType 		arv_gc_enumeration_get_type 	(void);

ArvGcNode * 	arv_gc_enumeration_new 	(void);
const char *	arv_gc_enumeration_get_string_value	(ArvGcEnumeration *enumeration);
void		arv_gc_enumeration_set_string_value	(ArvGcEnumeration *enumeration, const char *value);
gint64 		arv_gc_enumeration_get_int_value	(ArvGcEnumeration *enumeration);
void		arv_gc_enumeration_set_int_value	(ArvGcEnumeration *enumeration, gint64 value);
const GSList *	arv_gc_enumeration_get_entries		(ArvGcEnumeration *enumeration);

G_END_DECLS

#endif
