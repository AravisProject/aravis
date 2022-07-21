/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

/**
 * SECTION: arvgcfloat
 * @short_description: Float interface
 */

#include <arvgcfloat.h>
#include <arvgcfeaturenodeprivate.h>
#include <arvgcdefaultsprivate.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <stdio.h>
#include <arvdebugprivate.h>

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

        if (!arv_gc_feature_node_check_read_access (ARV_GC_FEATURE_NODE (gc_float), error))
                return 0.0;

	return ARV_GC_FLOAT_GET_IFACE (gc_float)->get_value (gc_float, error);
}

void
arv_gc_float_set_value (ArvGcFloat *gc_float, double value, GError **error)
{
	ArvGc *genicam;
	ArvRangeCheckPolicy policy;
	GError *local_error = NULL;

	g_return_if_fail (ARV_IS_GC_FLOAT (gc_float));
	g_return_if_fail (error == NULL || *error == NULL);

        if (!arv_gc_feature_node_check_write_access (ARV_GC_FEATURE_NODE (gc_float), error))
                return;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_float));
	g_return_if_fail (ARV_IS_GC (genicam));

	policy = arv_gc_get_range_check_policy (genicam);

	if (policy != ARV_RANGE_CHECK_POLICY_DISABLE) {
		ArvGcFloatInterface *iface = ARV_GC_FLOAT_GET_IFACE (gc_float);

		if (iface->get_min != NULL) {
			double min = iface->get_min (gc_float, &local_error);

			if (local_error == NULL && value < min) {
				g_set_error (&local_error, ARV_GC_ERROR, ARV_GC_ERROR_OUT_OF_RANGE,
					     "[%s] Value '%g' lower than allowed minimum '%g'",
					     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_float)), value, min);
			}
		}

		if (local_error == NULL && iface->get_max != NULL) {
			double max = iface->get_max (gc_float, &local_error);

			if (local_error == NULL && value > max) {
				g_set_error (&local_error, ARV_GC_ERROR, ARV_GC_ERROR_OUT_OF_RANGE,
					     "[%s] Value '%g' greater than allowed maximum '%g'",
					     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_float)), value, max);
			}
		}

		if (local_error != NULL) {
			if (policy == ARV_RANGE_CHECK_POLICY_DEBUG) {
				arv_warning_policies ("Range check (%s) ignored", local_error->message);
			} else if (policy == ARV_RANGE_CHECK_POLICY_ENABLE) {
				g_propagate_error (error, local_error);
				return;
			}
			g_clear_error (&local_error);
		}
	}

	ARV_GC_FLOAT_GET_IFACE (gc_float)->set_value (gc_float, value, error);
}

/**
 * arv_gc_float_get_min:
 * @gc_float: a #ArvGcFloat
 * @error: a #GError placeholder, NULL to ignore
 *
 * Returns: The feature Min property, -#G_MAXDOUBLE if not defined or on error
 */

double
arv_gc_float_get_min (ArvGcFloat *gc_float, GError **error)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), 0.0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0.0);

	float_interface = ARV_GC_FLOAT_GET_IFACE (gc_float);

	if (float_interface->get_min != NULL)
		return float_interface->get_min (gc_float, error);

	g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_PROPERTY_NOT_DEFINED, "[%s] <Min> node not found",
		     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_float)));

	return -G_MAXDOUBLE;
}

/**
 * arv_gc_float_get_max:
 * @gc_float: a #ArvGcFloat
 * @error: a #GError placeholder, NULL to ignore
 *
 * Returns: The feature Max property, #G_MAXDOUBLE if not defined or on error
 */

double
arv_gc_float_get_max (ArvGcFloat *gc_float, GError **error)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), 0.0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0.0);

	float_interface = ARV_GC_FLOAT_GET_IFACE (gc_float);

	if (float_interface->get_max != NULL)
		return float_interface->get_max (gc_float, error);

	g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_PROPERTY_NOT_DEFINED, "[%s] <Max> node not found",
		     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_float)));

	return G_MAXDOUBLE;
}

/**
 * arv_gc_float_get_inc:
 * @gc_float: a #ArvGcFloat
 * @error: a #GError placeholder, NULL to ignore
 *
 * Returns: The feature Inc property, #G_MINDOUBLE if not defined or on error
 */

double
arv_gc_float_get_inc (ArvGcFloat *gc_float, GError **error)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), G_MINDOUBLE);
	g_return_val_if_fail (error == NULL || *error == NULL, G_MINDOUBLE);

	float_interface = ARV_GC_FLOAT_GET_IFACE (gc_float);

	if (float_interface->get_inc != NULL)
		return float_interface->get_inc (gc_float, error);

	g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_PROPERTY_NOT_DEFINED, "[%s] <Inc> node not found",
		     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_float)));

	return G_MINDOUBLE;
}

/**
 * arv_gc_float_get_representation:
 * @gc_float: a #ArvGcFloat
 *
 * Get number representation format.
 *
 * Returns: Number representation format as #ArvGcRepresentation.
 */

ArvGcRepresentation
arv_gc_float_get_representation (ArvGcFloat *gc_float)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), ARV_GC_REPRESENTATION_UNDEFINED);

	float_interface = ARV_GC_FLOAT_GET_IFACE (gc_float);

	if (float_interface->get_representation != NULL)
		return float_interface->get_representation (gc_float);

	return ARV_GC_REPRESENTATION_UNDEFINED;
}

const char *
arv_gc_float_get_unit	(ArvGcFloat *gc_float)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), NULL);

	float_interface = ARV_GC_FLOAT_GET_IFACE (gc_float);

	if (float_interface->get_unit != NULL)
		return float_interface->get_unit (gc_float);

	return NULL;
}

/**
 * arv_gc_float_get_display_notation:
 * @gc_float: a #ArvGcFloat
 *
 * Get number display notation.
 *
 * Returns: Number display notation as #ArvGcDisplayNotation.
 *
 * Since: 0.8.0
 */

ArvGcDisplayNotation
arv_gc_float_get_display_notation (ArvGcFloat *gc_float)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), ARV_GC_DISPLAY_NOTATION_DEFAULT);

	float_interface = ARV_GC_FLOAT_GET_IFACE (gc_float);

	if (float_interface->get_display_notation != NULL)
		return float_interface->get_display_notation (gc_float);

	return ARV_GC_DISPLAY_NOTATION_DEFAULT;
}

/**
 * arv_gc_float_get_display_precision:
 * @gc_float: a #ArvGcFloat
 *
 * Gets number of digits to show in user interface. This number should always be positive and represent
 * total number of digits on left and right side of decimal.
 *
 * Returns: Number of digits to show.
 *
 * Since: 0.8.0
 */

gint64
arv_gc_float_get_display_precision (ArvGcFloat *gc_float)
{
	ArvGcFloatInterface *float_interface;

	g_return_val_if_fail (ARV_IS_GC_FLOAT (gc_float), ARV_GC_DISPLAY_PRECISION_DEFAULT);

	float_interface = ARV_GC_FLOAT_GET_IFACE (gc_float);

	if (float_interface->get_display_precision != NULL)
		return float_interface->get_display_precision (gc_float);

	return ARV_GC_DISPLAY_PRECISION_DEFAULT;
}

void arv_gc_float_impose_min (ArvGcFloat *gc_float, double minimum, GError **error)
{
	ArvGcFloatInterface *float_interface;

	g_return_if_fail (ARV_IS_GC_FLOAT (gc_float));
	g_return_if_fail (error == NULL || *error == NULL);

	float_interface = ARV_GC_FLOAT_GET_IFACE (gc_float);

	if (float_interface->impose_min != NULL)
		float_interface->impose_min (gc_float, minimum, error);
	else
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_PROPERTY_NOT_DEFINED, "[%s] <Min> node not found",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_float)));
}

void arv_gc_float_impose_max (ArvGcFloat *gc_float, double maximum, GError **error)
{
	ArvGcFloatInterface *float_interface;

	g_return_if_fail (ARV_IS_GC_FLOAT (gc_float));
	g_return_if_fail (error == NULL || *error == NULL);

	float_interface = ARV_GC_FLOAT_GET_IFACE (gc_float);

	if (float_interface->impose_max != NULL)
		float_interface->impose_max (gc_float, maximum, error);
	else
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_PROPERTY_NOT_DEFINED, "[%s] <Max> node not found",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_float)));
}
