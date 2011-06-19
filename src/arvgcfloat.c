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

/**
 * SECTION: arvgcfloat
 * @short_description: Float interface
 */

#include <arvgcfloat.h>
#include <arvtools.h>

static void
arv_gc_float_default_init (ArvGcFloatInterface *gc_float_iface)
{
}

G_DEFINE_INTERFACE (ArvGcFloat, arv_gc_float, G_TYPE_OBJECT)

double
arv_gc_float_get_value (ArvGcFloat *gc_float)
{
	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), 0.0);

	return ARV_GC_FLOAT_GET_INTERFACE (gc_float)->get_value (gc_float);
}

void
arv_gc_float_set_value (ArvGcFloat *gc_float, double value)
{
	g_return_if_fail (ARV_IS_GC_FLOAT (gc_float));

	ARV_GC_FLOAT_GET_INTERFACE (gc_float)->set_value (gc_float, value);
}

double
arv_gc_float_get_min (ArvGcFloat *gc_float)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), 0);

	float_interface = ARV_GC_FLOAT_GET_INTERFACE (gc_float);

	if (float_interface->get_min != NULL)
		return float_interface->get_min (gc_float);
	else
		return -G_MAXDOUBLE;
}

double
arv_gc_float_get_max (ArvGcFloat *gc_float)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), 0);

	float_interface = ARV_GC_FLOAT_GET_INTERFACE (gc_float);

	if (float_interface->get_max != NULL)
		return float_interface->get_max (gc_float);
	else
		return G_MAXDOUBLE;
}

gint64
arv_gc_float_get_inc (ArvGcFloat *gc_float)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), 0);

	float_interface = ARV_GC_FLOAT_GET_INTERFACE (gc_float);

	if (float_interface->get_inc != NULL)
		return float_interface->get_inc (gc_float);
	else
		return 1;
}

const char *
arv_gc_float_get_unit	(ArvGcFloat *gc_float)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), 0);

	float_interface = ARV_GC_FLOAT_GET_INTERFACE (gc_float);

	if (float_interface->get_unit != NULL)
		return float_interface->get_unit (gc_float);
	else
		return NULL;
}

void arv_gc_float_impose_min (ArvGcFloat *gc_float, double minimum)
{
	ArvGcFloatInterface *float_interface;

	g_return_if_fail (ARV_IS_GC_FLOAT (gc_float));

	float_interface = ARV_GC_FLOAT_GET_INTERFACE (gc_float);

	if (float_interface->impose_min != NULL)
		float_interface->impose_min (gc_float, minimum);
}

void arv_gc_float_impose_max (ArvGcFloat *gc_float, double maximum)
{
	ArvGcFloatInterface *float_interface;

	g_return_if_fail (ARV_IS_GC_FLOAT (gc_float));

	float_interface = ARV_GC_FLOAT_GET_INTERFACE (gc_float);

	if (float_interface->impose_max != NULL)
		float_interface->impose_max (gc_float, maximum);
}
