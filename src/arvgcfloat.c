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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvgcfloat
 * @short_description: Float interface
 */

#include <arvgcfloat.h>
#include <arvmisc.h>

static void
arv_gc_float_default_init (ArvGcFloatInterface *gc_float_iface)
{
}

G_DEFINE_INTERFACE (ArvGcFloat, arv_gc_float, G_TYPE_OBJECT)

double
arv_gc_float_get_value (ArvGcFloat *gc_float, GError **error)
{
	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), 0.0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0.0);

	return ARV_GC_FLOAT_GET_INTERFACE (gc_float)->get_value (gc_float, error);
}

void
arv_gc_float_set_value (ArvGcFloat *gc_float, double value, GError **error)
{
	g_return_if_fail (ARV_IS_GC_FLOAT (gc_float));
	g_return_if_fail (error == NULL || *error == NULL);

	ARV_GC_FLOAT_GET_INTERFACE (gc_float)->set_value (gc_float, value, error);
}

double
arv_gc_float_get_min (ArvGcFloat *gc_float, GError **error)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), 0.0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0.0);

	float_interface = ARV_GC_FLOAT_GET_INTERFACE (gc_float);

	if (float_interface->get_min != NULL)
		return float_interface->get_min (gc_float, error);
	else
		return -G_MAXDOUBLE;
}

double
arv_gc_float_get_max (ArvGcFloat *gc_float, GError **error)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), 0.0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0.0);

	float_interface = ARV_GC_FLOAT_GET_INTERFACE (gc_float);

	if (float_interface->get_max != NULL)
		return float_interface->get_max (gc_float, error);
	else
		return G_MAXDOUBLE;
}

double
arv_gc_float_get_inc (ArvGcFloat *gc_float, GError **error)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), 0.0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0.0);

	float_interface = ARV_GC_FLOAT_GET_INTERFACE (gc_float);

	if (float_interface->get_inc != NULL)
		return float_interface->get_inc (gc_float, error);
	else
		return 1;
}

const char *
arv_gc_float_get_unit	(ArvGcFloat *gc_float, GError **error)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	float_interface = ARV_GC_FLOAT_GET_INTERFACE (gc_float);

	if (float_interface->get_unit != NULL)
		return float_interface->get_unit (gc_float, error);
	else
		return NULL;
}

void arv_gc_float_impose_min (ArvGcFloat *gc_float, double minimum, GError **error)
{
	ArvGcFloatInterface *float_interface;

	g_return_if_fail (ARV_IS_GC_FLOAT (gc_float));
	g_return_if_fail (error == NULL || *error == NULL);

	float_interface = ARV_GC_FLOAT_GET_INTERFACE (gc_float);

	if (float_interface->impose_min != NULL)
		float_interface->impose_min (gc_float, minimum, error);
}

void arv_gc_float_impose_max (ArvGcFloat *gc_float, double maximum, GError **error)
{
	ArvGcFloatInterface *float_interface;

	g_return_if_fail (ARV_IS_GC_FLOAT (gc_float));
	g_return_if_fail (error == NULL || *error == NULL);

	float_interface = ARV_GC_FLOAT_GET_INTERFACE (gc_float);

	if (float_interface->impose_max != NULL)
		float_interface->impose_max (gc_float, maximum, error);
}
