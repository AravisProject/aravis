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

/**
 * SECTION: arvdevice
 * @short_description: Abstract base class for device handling
 *
 * #ArvDevice is an abstract base class for the control of cameras. It provides
 * an easy access to the camera settings, and to its genicam interface for  more
 * advanced uses.
 */

#include <arvdevice.h>
#include <arvdeviceprivate.h>
#include <arvgc.h>
#include <arvgccommand.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcboolean.h>
#include <arvgcenumeration.h>
#include <arvgcstring.h>
#include <arvstream.h>
#include <arvdebug.h>

enum {
	ARV_DEVICE_SIGNAL_CONTROL_LOST,
	ARV_DEVICE_SIGNAL_LAST
} ArvDeviceSignals;

static guint arv_device_signals[ARV_DEVICE_SIGNAL_LAST] = {0};

struct  _ArvDevicePrivate {
	int dummy;
};

GQuark
arv_device_error_quark (void)
{
	return g_quark_from_static_string ("arv-device-error-quark");
}

/**
 * arv_device_create_stream:
 * @device: a #ArvDevice
 * @callback: (scope call): a frame processing callback
 * @user_data: (allow-none) (closure): user data for @callback
 *
 * Creates a new #ArvStream for video stream handling. See
 * @ArvStreamCallback for details regarding the callback function.
 *
 * Return value: (transfer full): a new #ArvStream.
 *
 * Since: 0.2.0
 */

ArvStream *
arv_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	return ARV_DEVICE_GET_CLASS (device)->create_stream (device, callback, user_data);
}

/**
 * arv_device_read_memory:
 * @device: a #ArvDevice
 * @address: memory address
 * @size: number of bytes to read
 * @buffer: a buffer for the storage of the read data
 * @error: (out) (allow-none): a #GError placeholder
 *
 * Reads @size bytes from the device memory.
 *
 * Return value: (skip): TRUE on success.
 *
 * Since: 0.2.0
 **/

gboolean
arv_device_read_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (buffer != NULL, FALSE);
	g_return_val_if_fail (size > 0, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	return ARV_DEVICE_GET_CLASS (device)->read_memory (device, address, size, buffer, error);
}

/**
 * arv_device_write_memory:
 * @device: a #ArvDevice
 * @address: memory address
 * @size: size of the returned buffer
 * @buffer: (transfer full): the buffer read from memory
 * @error: (out) (allow-none): a #GError placeholder
 *
 * Writes @size bytes to the device memory.
 *
 * Return value: (skip): TRUE on success.
 *
 * Since: 0.2.0
 **/

gboolean
arv_device_write_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (buffer != NULL, FALSE);
	g_return_val_if_fail (size > 0, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	return ARV_DEVICE_GET_CLASS (device)->write_memory (device, address, size, buffer, error);
}

/**
 * arv_device_read_register:
 * @device: a #ArvDevice
 * @address: register address
 * @value: (out): a placeholder for the read value
 * @error: (out) (allow-none): a #GError placeholder
 *
 * Reads the value of a device register.
 *
 * Return value: (skip): TRUE on success.
 *
 * Since: 0.2.0
 **/

gboolean
arv_device_read_register (ArvDevice *device, guint64 address, guint32 *value, GError **error)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (value != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	return ARV_DEVICE_GET_CLASS (device)->read_register (device, address, value, error);
}

/**
 * arv_device_write_register:
 * @device: a #ArvDevice
 * @address: the register address
 * @value: value to write
 * @error: (out) (allow-none): a #GError placeholder
 *
 * Writes @value to a device register.
 *
 * Return value: (skip): TRUE on success.
 *
 * Since: 0.2.0
 **/

gboolean
arv_device_write_register (ArvDevice *device, guint64 address, guint32 value, GError **error)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	return ARV_DEVICE_GET_CLASS (device)->write_register (device, address, value, error);
}

/**
 * arv_device_get_genicam:
 * @device: a #ArvDevice
 *
 * Retrieves the genicam interface of the given device.
 *
 * Return value: (transfer none): the genicam interface.
 *
 * Since: 0.2.0
 */

ArvGc *
arv_device_get_genicam (ArvDevice *device)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	return ARV_DEVICE_GET_CLASS (device)->get_genicam (device);
}

/**
 * arv_device_get_genicam_xml:
 * @device: a #ArvDevice
 * @size: (out) (allow-none): placeholder for the returned data size (bytes)
 *
 * Gets the Genicam XML data stored in the device memory.
 *
 * Returns: (transfer none): a pointer to the Genicam XML data, owned by the device.
 *
 * Since: 0.2.0
 **/

const char *
arv_device_get_genicam_xml (ArvDevice *device, size_t *size)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);
	g_return_val_if_fail (size != NULL, NULL);

	if (ARV_DEVICE_GET_CLASS (device)->get_genicam_xml != NULL)
		return ARV_DEVICE_GET_CLASS (device)->get_genicam_xml (device, size);

	*size = 0;

	return NULL;
}

/**
 * arv_device_create_chunk_parser:
 * @device: a #ArvDevice
 *
 * Create a #ArvChunkParser object, to be used for chunk data extraction from #ArvBuffer.
 *
 * Returns: (transfer full): a new #ArvChunkParser object, NULL on error.
 *
 * Since: 0.4.0
 **/

ArvChunkParser *
arv_device_create_chunk_parser (ArvDevice *device)
{
	const char *xml = NULL;
	gsize size = 0;

	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	xml = arv_device_get_genicam_xml (device, &size);

	return arv_chunk_parser_new (xml, size);
}

/**
 * arv_device_get_feature:
 * @device: a #ArvDevice
 * @feature: feature name
 *
 * Return value: (transfer none): the genicam node corresponding to the feature name, NULL if not found.
 *
 * Since: 0.2.0
 */

ArvGcNode *
arv_device_get_feature (ArvDevice *device, const char *feature)
{
	ArvGc *genicam;

	genicam = arv_device_get_genicam (device);
	g_return_val_if_fail (ARV_IS_GC (genicam), NULL);

	return arv_gc_get_node (genicam, feature);
}

static void *
_get_feature (ArvDevice *device, GType node_type, const char *feature, GError **error)
{
	void *node;

	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);
	g_return_val_if_fail (feature != NULL, NULL);

	node = arv_device_get_feature (device, feature);

	if (node == NULL) {
		g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_FEATURE_NOT_FOUND,
			     "node '%s' not found", feature);
		return NULL;
	}

	if (!(G_TYPE_CHECK_INSTANCE_TYPE ((node), node_type))) {
		g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_WRONG_FEATURE,
			     "node '%s' [%s]  is not a %s", feature, G_OBJECT_TYPE_NAME (node), g_type_name (node_type));
		return NULL;
	}

	return node;
}

/**
 * arv_device_execute_command:
 * @device: a #ArvDevice
 * @feature: feature name
 * @error: a #GError placeholder
 *
 * Execute a genicam command.
 *
 * Since: 0.8.0
 */

void
arv_device_execute_command (ArvDevice *device, const char *feature, GError **error)
{
	ArvGcNode *node;

	node = _get_feature (device, ARV_TYPE_GC_COMMAND, feature, error);
	if (node != NULL)
		arv_gc_command_execute (ARV_GC_COMMAND (node), error);
}

/**
 * arv_device_set_boolean_feature_value:
 * @device: a #ArvDevice
 * @feature: feature name
 * @value: feature value
 * @error: a #GError placeholder
 *
 * Set the value of a boolean feature.
 *
 * Since: 0.8.0
 */

void
arv_device_set_boolean_feature_value (ArvDevice *device, const char *feature, gboolean value, GError **error)
{
	ArvGcNode *node;

	node = _get_feature (device, ARV_TYPE_GC_BOOLEAN, feature, error);
	if (node != NULL)
		arv_gc_boolean_set_value (ARV_GC_BOOLEAN (node), value, error);
}

/**
 * arv_device_get_boolean_feature_value:
 * @device: a #ArvDevice
 * @feature: feature name
 * @error: a #GError placeholder
 *
 * Returns: the feature value, %FALSE on error.
 *
 * Since: 0.8.0
 */

gboolean
arv_device_get_boolean_feature_value (ArvDevice *device, const char *feature, GError **error)
{
	ArvGcNode *node;

	node = _get_feature (device, ARV_TYPE_GC_BOOLEAN, feature, error);
	if (node != NULL)
		return arv_gc_boolean_get_value (ARV_GC_BOOLEAN (node), error);

	return FALSE;
}

/**
 * arv_device_set_string_feature_value:
 * @device: a #ArvDevice
 * @feature: feature name
 * @value: new feature value
 * @error: a #GError placeholder
 *
 * Set the string feature value.
 *
 * Since: 0.8.0
 */

void
arv_device_set_string_feature_value (ArvDevice *device, const char *feature, const char *value, GError **error)
{
	ArvGcNode *node;

	node = _get_feature (device, ARV_TYPE_GC_STRING, feature, error);
	if (node != NULL)
		arv_gc_string_set_value (ARV_GC_STRING (node), value, error);
}

/**
 * arv_device_get_string_feature_value:
 * @device: a #ArvDevice
 * @feature: feature name
 * @error: a #GError placeholder
 *
 * Returns: the string feature value, %NULL on error.
 *
 * Since: 0.8.0
 */

const char *
arv_device_get_string_feature_value (ArvDevice *device, const char *feature, GError  **error)
{
	ArvGcNode *node;

	node = _get_feature (device, ARV_TYPE_GC_STRING, feature, error);
	if (node != NULL)
		return arv_gc_string_get_value (ARV_GC_STRING (node), error);

	return NULL;
}

/**
 * arv_device_set_integer_feature_value:
 * @device: a #ArvDevice
 * @feature: feature name
 * @value: new feature value
 * @error: a #GError placeholder
 *
 * Set the integer feature value.
 *
 * Since: 0.8.0
 */

void
arv_device_set_integer_feature_value (ArvDevice *device, const char *feature, gint64 value, GError **error)
{
	ArvGcNode *node;

	node = _get_feature (device, ARV_TYPE_GC_INTEGER, feature, error);
	if (node != NULL)
		arv_gc_integer_set_value (ARV_GC_INTEGER (node), value, error);
}

/**
 * arv_device_get_integer_feature_value:
 * @device: a #ArvDevice
 * @feature: feature name
 * @error: a #GError placeholder
 *
 * Returns: the integer feature value, 0 on error.
 *
 * Since: 0.8.0
 */

gint64
arv_device_get_integer_feature_value (ArvDevice *device, const char *feature, GError **error)
{
	ArvGcNode *node;

	node = _get_feature (device, ARV_TYPE_GC_INTEGER, feature, error);
	if (node != NULL)
		return arv_gc_integer_get_value (ARV_GC_INTEGER (node), error);

	return 0;
}

/**
 * arv_device_get_integer_feature_bounds:
 * @device: a #ArvDevice
 * @feature: feature name
 * @min: (out): minimum feature value
 * @max: (out): maximum feature value
 * @error: a #GError placeholder
 *
 * Retrieves feature bounds.
 *
 * Since: 0.8.0
 */

void
arv_device_get_integer_feature_bounds (ArvDevice *device, const char *feature, gint64 *min, gint64 *max, GError **error)
{
	ArvGcNode *node;

	if (min != NULL)
		*min = G_MININT64;
	if (max != NULL)
		*max = G_MAXINT64;

	node = _get_feature (device, ARV_TYPE_GC_INTEGER, feature, error);
	if (node != NULL) {
		GError *local_error = NULL;

		if (min != NULL) {
			gint64 minimum;

			minimum = arv_gc_integer_get_min (ARV_GC_INTEGER (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			*min = minimum;
		}

		if (max != NULL) {
			gint64 maximum;

			maximum = arv_gc_integer_get_max (ARV_GC_INTEGER (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			*max = maximum;
		}
	}
}

/**
 * arv_device_get_integer_feature_increment:
 * @device: a #ArvDevice
 * @feature: feature name
 * @error: a #GError placeholder
 *
 * Not all integer features have evenly distributed allowed values, which means the returned increment may not reflect the allowed value
 * set.
 *
 * Returns: feature value increment, or 1 on error.
 *
 * Since: 0.8.0
 */

gint64
arv_device_get_integer_feature_increment (ArvDevice *device, const char *feature, GError **error)
{
	ArvGcNode *node;

	node = _get_feature (device, ARV_TYPE_GC_INTEGER, feature, error);
	if (node != NULL) {
		GError *local_error = NULL;
		gint64 increment;

		increment = arv_gc_integer_get_inc (ARV_GC_INTEGER (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return 1;
			}

			return increment;
	}

	return 1;
}

/**
 * arv_device_set_float_feature_value:
 * @device: a #ArvDevice
 * @feature: feature name
 * @value: new feature value
 * @error: a #GError placeholder
 *
 * Set the float feature value.
 *
 * Since: 0.8.0
 */

void
arv_device_set_float_feature_value (ArvDevice *device, const char *feature, double value, GError **error)
{
	ArvGcNode *node;

	node = _get_feature (device, ARV_TYPE_GC_FLOAT, feature, error);
	if (node != NULL)
		arv_gc_float_set_value (ARV_GC_FLOAT (node), value, error);
}

/**
 * arv_device_get_float_feature_value:
 * @device: a #ArvDevice
 * @feature: feature name
 * @error: a #GError placeholder
 *
 * Returns: the float feature value, 0.0 on error.
 *
 * Since: 0.8.0
 */

double
arv_device_get_float_feature_value (ArvDevice *device, const char *feature, GError **error)
{
	ArvGcNode *node;

	node = _get_feature (device, ARV_TYPE_GC_FLOAT, feature, error);
	if (node != NULL)
		return  arv_gc_float_get_value (ARV_GC_FLOAT (node), error);

	return 0.0;
}

/**
 * arv_device_get_float_feature_bounds:
 * @device: a #ArvDevice
 * @feature: feature name
 * @min: (out): minimum feature value
 * @max: (out): maximum feature value
 * @error: a #GError placeholder
 *
 * Retrieves feature bounds.
 *
 * Since: 0.8.0
 */

void
arv_device_get_float_feature_bounds (ArvDevice *device, const char *feature, double *min, double *max, GError **error)
{
	ArvGcNode *node;

	if (min != NULL)
		*min = -G_MAXDOUBLE;
	if (max != NULL)
		*max = G_MAXDOUBLE;

	node = _get_feature (device, ARV_TYPE_GC_FLOAT, feature, error);
	if (node != NULL) {
		GError *local_error = NULL;

		if (min != NULL) {
			double minimum;

			minimum = arv_gc_float_get_min (ARV_GC_FLOAT (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			*min = minimum;
		}

		if (max != NULL) {
			double maximum;

			maximum = arv_gc_float_get_max (ARV_GC_FLOAT (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			*max = maximum;
		}
	}
}

/**
 * arv_device_get_available_enumeration_feature_values:
 * @device: an #ArvDevice
 * @feature: feature name
 * @n_values: placeholder for the number of returned values
 * @error: a #GError placeholder
 *
 * Get all the available values of @feature, as integers.
 *
 * Returns: (array length=n_values) (transfer container): a newly created array of 64 bit integers, which must freed after use using g_free,
 * or NULL on error.
 *
 * Since: 0.8.0
 */

gint64 *
arv_device_get_available_enumeration_feature_values (ArvDevice *device, const char *feature, guint *n_values, GError **error)
{
	ArvGcNode *node;

	if (n_values != NULL)
		*n_values = 0;

	node = _get_feature (device, ARV_TYPE_GC_ENUMERATION, feature, error);
	if (node != NULL)
		return arv_gc_enumeration_get_available_int_values (ARV_GC_ENUMERATION (node), n_values, error);

	return NULL;
}

/**
 * arv_device_get_available_enumeration_feature_values_as_strings:
 * @device: an #ArvDevice
 * @feature: feature name
 * @n_values: placeholder for the number of returned values
 * @error: a #GError placeholder
 *
 * Get all the available values of @feature, as strings.
 *
 * Returns: (array length=n_values) (transfer container): a newly created array of const strings, which must freed after use using g_free,
 * or NULL on error.
 *
 * Since: 0.8.0
 */

const char **
arv_device_get_available_enumeration_feature_values_as_strings (ArvDevice *device, const char *feature, guint *n_values, GError **error)
{
	ArvGcNode *node;

	if (n_values != NULL)
		*n_values = 0;

	node = _get_feature (device, ARV_TYPE_GC_ENUMERATION, feature, error);
	if (node != NULL)
		return arv_gc_enumeration_get_available_string_values (ARV_GC_ENUMERATION (node), n_values, error);

	return NULL;
}

/**
 * arv_device_get_available_enumeration_feature_values_as_display_names:
 * @device: an #ArvDevice
 * @feature: feature name
 * @n_values: placeholder for the number of returned values
 * @error: a #GError placeholder
 *
 * Get display names of all the available entries of @feature.
 *
 * Returns: (array length=n_values) (transfer container): a newly created array of const strings, to be freed after use using g_free, or
 * %NULL on error.
 *
 * Since: 0.8.0
 */

const char **
arv_device_get_available_enumeration_feature_values_as_display_names (ArvDevice *device, const char *feature, guint *n_values, GError **error)
{
	ArvGcNode *node;

	if (n_values != NULL)
		*n_values = 0;

	node = _get_feature (device, ARV_TYPE_GC_ENUMERATION, feature, error);
	if (node != NULL)
		return arv_gc_enumeration_get_available_display_names (ARV_GC_ENUMERATION (node), n_values, error);

	return NULL;
}
void
arv_device_set_register_cache_policy (ArvDevice *device, ArvRegisterCachePolicy policy)
{
	ArvGc *genicam;

	g_return_if_fail (ARV_IS_DEVICE (device));

	genicam = arv_device_get_genicam (device);
	arv_gc_set_register_cache_policy (genicam, policy);
}

void
arv_device_emit_control_lost_signal (ArvDevice *device)
{
	g_return_if_fail (ARV_IS_DEVICE (device));

	g_signal_emit (device, arv_device_signals[ARV_DEVICE_SIGNAL_CONTROL_LOST], 0);
}

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ArvDevice, arv_device, G_TYPE_OBJECT, G_ADD_PRIVATE (ArvDevice))

static void
arv_device_init (ArvDevice *device)
{
	device->priv = arv_device_get_instance_private (device);
}

static void
arv_device_finalize (GObject *object)
{
	G_OBJECT_CLASS (arv_device_parent_class)->finalize (object);
}

static void
arv_device_class_init (ArvDeviceClass *device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (device_class);

	object_class->finalize = arv_device_finalize;

	/**
	 * ArvDevice::control-lost:
	 * @device:a #ArvDevice
	 *
	 * Signal that the control of the device is lost.
	 *
	 * This signal may be emited from a thread different than the main one,
	 * so please take care to shared data access from the callback.
	 *
	 * Since: 0.2.0
	 */

	arv_device_signals[ARV_DEVICE_SIGNAL_CONTROL_LOST] =
		g_signal_new ("control-lost",
			      G_TYPE_FROM_CLASS (device_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ArvDeviceClass, control_lost),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE);
}
