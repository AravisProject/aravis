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

#include <arvgcinteger.h>
#include <arvtools.h>

static void
arv_gc_integer_default_init (ArvGcIntegerInterface *gc_integer_iface)
{
}

G_DEFINE_INTERFACE (ArvGcInteger, arv_gc_integer, G_TYPE_OBJECT)

guint64
arv_gc_integer_get_value (ArvGcInteger *gc_integer)
{
	return 0;
}

void
arv_gc_integer_set_value (ArvGcInteger *gc_integer, guint64 value)
{
}

guint64
arv_gc_integer_get_min (ArvGcInteger *gc_integer)
{
	return 0;
}

guint64
arv_gc_integer_get_max (ArvGcInteger *gc_integer)
{
	return 0;
}

const char *
arv_gc_integer_get_unit	(ArvGcInteger *gc_integer)
{
	return NULL;
}

void arv_gc_integer_impose_min (ArvGcInteger *gc_integer, guint64 minimum)
{
}

void arv_gc_integer_impose_max (ArvGcInteger *gc_integer, guint64 maximum)
{
}
