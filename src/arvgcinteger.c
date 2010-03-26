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
	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), 0);

	return ARV_GC_INTEGER_GET_INTERFACE (gc_integer)->get_value (gc_integer);
}

void
arv_gc_integer_set_value (ArvGcInteger *gc_integer, guint64 value)
{
	g_return_if_fail (ARV_IS_GC_INTEGER (gc_integer));

	ARV_GC_INTEGER_GET_INTERFACE (gc_integer)->set_value (gc_integer, value);
}

guint64
arv_gc_integer_get_min (ArvGcInteger *gc_integer)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), 0);

	integer_interface = ARV_GC_INTEGER_GET_INTERFACE (gc_integer);

	if (integer_interface->get_min != NULL)
		return integer_interface->get_min (gc_integer);
	else
		return 0;
}

guint64
arv_gc_integer_get_max (ArvGcInteger *gc_integer)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), 0);

	integer_interface = ARV_GC_INTEGER_GET_INTERFACE (gc_integer);

	if (integer_interface->get_max != NULL)
		return integer_interface->get_max (gc_integer);
	else
		return G_MAXUINT64;
}

const char *
arv_gc_integer_get_unit	(ArvGcInteger *gc_integer)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_val_if_fail (ARV_IS_GC_INTEGER (gc_integer), 0);

	integer_interface = ARV_GC_INTEGER_GET_INTERFACE (gc_integer);

	if (integer_interface->get_unit != NULL)
		return integer_interface->get_unit (gc_integer);
	else
		return NULL;
}

void arv_gc_integer_impose_min (ArvGcInteger *gc_integer, guint64 minimum)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_if_fail (ARV_IS_GC_INTEGER (gc_integer));

	integer_interface = ARV_GC_INTEGER_GET_INTERFACE (gc_integer);

	if (integer_interface->impose_min != NULL)
		integer_interface->impose_min (gc_integer, minimum);
}

void arv_gc_integer_impose_max (ArvGcInteger *gc_integer, guint64 maximum)
{
	ArvGcIntegerInterface *integer_interface;

	g_return_if_fail (ARV_IS_GC_INTEGER (gc_integer));

	integer_interface = ARV_GC_INTEGER_GET_INTERFACE (gc_integer);

	if (integer_interface->impose_max != NULL)
		integer_interface->impose_max (gc_integer, maximum);
}
