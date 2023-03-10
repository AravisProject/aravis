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
#include <arvgcfeaturenode.h>
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

GQuark
arv_device_error_quark (void)
{
	return g_quark_from_static_string ("arv-device-error-quark");
}

typedef struct {
	GError *init_error;
} ArvDevicePrivate;

static void arv_device_initable_iface_init (GInitableIface *iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ArvDevice, arv_device, G_TYPE_OBJECT,
				  G_ADD_PRIVATE (ArvDevice)
				  G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, arv_device_initable_iface_init))

/**
 * arv_device_create_stream: (skip)
 * @device: a #ArvDevice
 * @callback: (scope call): a frame processing callback
 * @user_data: (allow-none) (closure): user data for @callback
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Creates a new #ArvStream for video stream handling. See
 * @ArvStreamCallback for details regarding the callback function.
 *
 * Return value: (transfer full): a new #ArvStream.
 *
 * Since: 0.2.0
 */

ArvStream *
arv_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data, GError **error)
{
	return arv_device_create_stream_full(device, callback, user_data, NULL, error);
}

/**
 * arv_device_create_stream_full: (rename-to arv_device_create_stream)
 * @device: a #ArvDevice
 * @callback: (scope notified): a frame processing callback
 * @user_data: (allow-none) (closure): user data for @callback
 * @destroy: a #GDestroyNotify placeholder, %NULL to ignore
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Creates a new #ArvStream for video stream handling. See
 * @ArvStreamCallback for details regarding the callback function.
 *
 * Return value: (transfer full): a new #ArvStream.
 *
 * Since: 0.8.23
 */

ArvStream *
arv_device_create_stream_full (ArvDevice *device, ArvStreamCallback callback, void *user_data, GDestroyNotify destroy, GError **error)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	return ARV_DEVICE_GET_CLASS (device)->create_stream (device, callback, user_data, destroy, error);
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

/**
 * arv_device_get_feature_access_mode:
 * @device: a #ArvDevice
 * @feature: feature name
 *
 * Return value: The actual feature access mode, which is a combination of ImposedAccessMode property and actual
 * register access mode.
 *
 * Since: 0.8.22
 */

ArvGcAccessMode
arv_device_get_feature_access_mode (ArvDevice *device, const char *feature)
{
	ArvGcNode* node;

	g_return_val_if_fail (ARV_IS_DEVICE (device), ARV_GC_ACCESS_MODE_UNDEFINED);
	g_return_val_if_fail (feature != NULL, ARV_GC_ACCESS_MODE_UNDEFINED);

	node = arv_device_get_feature (device, feature);
	return ARV_IS_GC_FEATURE_NODE (node) && arv_gc_feature_node_get_actual_access_mode (ARV_GC_FEATURE_NODE (node));
}

/**
 * arv_device_is_feature_available:
 * @device: a #ArvDevice
 * @feature: feature name
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Return: %TRUE if feature is available, %FALSE if not or on error.
 *
 * Since: 0.8.0
 */

gboolean
arv_device_is_feature_available (ArvDevice *device, const char *feature, GError **error)
{
	ArvGcNode* node;

	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (feature != NULL, FALSE);

	node = arv_device_get_feature (device, feature);
	return ARV_IS_GC_FEATURE_NODE (node) && arv_gc_feature_node_is_available (ARV_GC_FEATURE_NODE (node), error);
}

/**
 * arv_device_is_feature_implemented:
 * @device: a #ArvDevice
 * @feature: feature name
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Return: %TRUE if feature is implemented, %FALSE if not or on error.
 *
 * Since: 0.8.23
 */

gboolean
arv_device_is_feature_implemented (ArvDevice *device, const char *feature, GError **error)
{
	ArvGcNode* node;

	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (feature != NULL, FALSE);

	node = arv_device_get_feature (device, feature);
	return ARV_IS_GC_FEATURE_NODE (node) && arv_gc_feature_node_is_implemented (ARV_GC_FEATURE_NODE (node), error);
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
			     "[%s] Not found", feature);
		return NULL;
	}

	if (!(G_TYPE_CHECK_INSTANCE_TYPE ((node), node_type))) {
		g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_WRONG_FEATURE,
			     "[%s:%s] Not a %s", feature, G_OBJECT_TYPE_NAME (node), g_type_name (node_type));
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
 * arv_device_get_boolean_feature_value_gi: (rename-to arv_device_get_boolean_feature_value)
 * @device: a #ArvDevice
 * @feature: feature name
 * @value: (out): feature value
 * @error: a #GError placeholder
 *
 * Get the feature value, or %FALSE on error.
 *
 * Since: 0.8.0
 */

void
arv_device_get_boolean_feature_value_gi	(ArvDevice *device, const char *feature, gboolean *value, GError **error)
{
	g_return_if_fail (value != NULL);

	*value = arv_device_get_boolean_feature_value (device, feature, error);
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
 * arv_device_get_float_feature_increment:
 * @device: a #ArvDevice
 * @feature: feature name
 * @error: a #GError placeholder
 *
 * Not all float features have evenly distributed allowed values, which means the returned increment may not reflect the allowed value
 * set.
 *
 * Returns: feature value increment, or #G_MINDOUBLE on error.
 *
 * Since: 0.8.16
 */

double
arv_device_get_float_feature_increment (ArvDevice *device, const char *feature, GError **error)
{
	ArvGcNode *node;

	node = _get_feature (device, ARV_TYPE_GC_FLOAT, feature, error);
	if (node != NULL) {
		GError *local_error = NULL;
		double increment;

		increment = arv_gc_float_get_inc (ARV_GC_FLOAT (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return G_MINDOUBLE;
			}

			return increment;
	}

	return G_MINDOUBLE;
}

/**
 * arv_device_dup_available_enumeration_feature_values:
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
arv_device_dup_available_enumeration_feature_values (ArvDevice *device, const char *feature, guint *n_values, GError **error)
{
	ArvGcNode *node;

	if (n_values != NULL)
		*n_values = 0;

	node = _get_feature (device, ARV_TYPE_GC_ENUMERATION, feature, error);
	if (node != NULL)
		return arv_gc_enumeration_dup_available_int_values (ARV_GC_ENUMERATION (node), n_values, error);

	return NULL;
}

/**
 * arv_device_dup_available_enumeration_feature_values_as_strings:
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
arv_device_dup_available_enumeration_feature_values_as_strings (ArvDevice *device, const char *feature, guint *n_values, GError **error)
{
	ArvGcNode *node;

	if (n_values != NULL)
		*n_values = 0;

	node = _get_feature (device, ARV_TYPE_GC_ENUMERATION, feature, error);
	if (node != NULL)
		return arv_gc_enumeration_dup_available_string_values (ARV_GC_ENUMERATION (node), n_values, error);

	return NULL;
}

/**
 * arv_device_dup_available_enumeration_feature_values_as_display_names:
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
arv_device_dup_available_enumeration_feature_values_as_display_names (ArvDevice *device, const char *feature, guint *n_values, GError **error)
{
	ArvGcNode *node;

	if (n_values != NULL)
		*n_values = 0;

	node = _get_feature (device, ARV_TYPE_GC_ENUMERATION, feature, error);
	if (node != NULL)
		return arv_gc_enumeration_dup_available_display_names (ARV_GC_ENUMERATION (node), n_values, error);

	return NULL;
}

/**
 * arv_device_is_enumeration_entry_available:
 * @device: an #ArvDevice instance
 * @feature: enumeration feature name
 * @entry: entry name
 * @error: a #GError placeholder
 *
 * Returns: %TRUE if the feature and the feature entry are available
 *
 * Since: 0.8.17
 */

gboolean
arv_device_is_enumeration_entry_available (ArvDevice *device, const char *feature, const char *entry, GError **error)
{
        GError *local_error = NULL;
        const char **entries = NULL;
        guint n_entries = 0;
        gboolean is_available = FALSE;
        unsigned int i;

        if (!arv_device_is_feature_available (device, feature, &local_error)) {
                if (local_error != NULL)
                        g_propagate_error (error, local_error);
                return FALSE;
        }

        entries = arv_device_dup_available_enumeration_feature_values_as_strings (device, feature, &n_entries,
                                                                                  &local_error);

        if (local_error != NULL) {
                g_propagate_error (error, local_error);
                return FALSE;
        }

        for (i = 0; i < n_entries && !is_available; i++) {
                if (g_strcmp0 (entry, entries[i]) == 0)
                        is_available = TRUE;
        }
        g_free (entries);

        return is_available;
}

/**
 * arv_device_set_features_from_string:
 * @device: a #ArvDevice
 * @string: a space separated list of features assignments
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Set features from a string containing a list of space separated feature assignments or command names. For example:
 *
 * |[<!-- language="C" -->
 * arv_device_set_features_from_string (device, "Width=256 Height=256 PixelFormat='Mono8' TriggerStart", &error);
 * ]|
 *
 * Since: 0.8.0
 */

gboolean
arv_device_set_features_from_string (ArvDevice *device, const char *string, GError **error)
{
	GMatchInfo *match_info = NULL;
	GError *local_error = NULL;
	GRegex *regex;

	g_return_val_if_fail (ARV_IS_DEVICE (device), FALSE);

	if (string == NULL)
		return TRUE;

	regex = g_regex_new ("((?<Key>[^\\s\"'\\=]+)|\"(?<Key>[^\"]*)\"|'(?<Key>[^']*)')"
			     "(?:\\=((?<Value>[^\\s\"']+)|\"(?<Value>[^\"]*)\"|'(?<Value>[^']*)'))?",
			     G_REGEX_DUPNAMES, 0, NULL);

	if (g_regex_match (regex, string, 0, &match_info)) {
		while (g_match_info_matches (match_info) && local_error == NULL) {
			ArvGcNode *feature;
			char *key = g_match_info_fetch_named (match_info, "Key");
			char *value = g_match_info_fetch_named (match_info, "Value");
                        size_t key_length = key != NULL ? strlen (key) : 0;

                        if (key_length > 4 && key[0] == 'R' && key[1] == '[' && key[key_length - 1] == ']') {
                                char *end;
                                gint64 address;
                                gint64 int_value;

                                address = g_ascii_strtoll (&key[2], &end, 0);
                                if (end == NULL || end != key + key_length -1) {
                                        g_set_error (&local_error,
                                                     ARV_DEVICE_ERROR,
                                                     ARV_DEVICE_ERROR_INVALID_PARAMETER,
                                                     "Invalid address in %s", key);
                                } else {
                                        int_value = g_ascii_strtoll (value, &end, 0);
                                        if (end == NULL || end[0] != '\0') {
                                                g_set_error (&local_error,
                                                             ARV_DEVICE_ERROR,
                                                             ARV_DEVICE_ERROR_INVALID_PARAMETER,
                                                             "Invalid %s value for %s", value, key);
                                        } else {
                                                arv_device_write_register (device, address, int_value, &local_error);
                                        }
                                }
                        } else {
                                feature = arv_device_get_feature (device, key);
                                if (ARV_IS_GC_FEATURE_NODE (feature)) {
                                        if (ARV_IS_GC_COMMAND (feature)) {
                                                arv_device_execute_command (device, key, &local_error);
                                        } else if (value != NULL) {
                                                arv_gc_feature_node_set_value_from_string (ARV_GC_FEATURE_NODE (feature),
                                                                                           value, &local_error);
                                        } else {
                                                g_set_error (&local_error,
                                                             ARV_DEVICE_ERROR,
                                                             ARV_DEVICE_ERROR_INVALID_PARAMETER,
                                                             "[%s] Require a parameter value to set", key);
                                        }
                                } else
                                        g_set_error (&local_error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_FEATURE_NOT_FOUND,
                                                     "[%s] Not found", key);
                        }

			g_free (key);
			g_free (value);

			g_match_info_next (match_info, NULL);
		}
		g_match_info_unref (match_info);
	}

	g_regex_unref (regex);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return FALSE;
	}

	return TRUE;
}

/**
 * arv_device_set_register_cache_policy:
 * @device: a #ArvDevice
 * @policy: cache policy
 *
 * Sets the register cache policy.
 *
 * <warning><para>Be aware that some camera may have wrong Cachable properties defined in their Genicam metadata, which
 * may lead to incorrect readouts. Using the debug cache policy, and activating genicam debug output (export
 * ARV_DEBUG=genicam), can help you to check the cache validity. In this mode, every time the cache content is not in
 * sync with the actual register value, a debug message is printed on the console.</para></warning>
 *
 * Since: 0.8.0
 */

void
arv_device_set_register_cache_policy (ArvDevice *device, ArvRegisterCachePolicy policy)
{
	ArvGc *genicam;

	g_return_if_fail (ARV_IS_DEVICE (device));

	genicam = arv_device_get_genicam (device);
	arv_gc_set_register_cache_policy (genicam, policy);
}

/**
 * arv_device_set_range_check_policy:
 * @device: a #ArvDevice
 * @policy: range check policy
 *
 * Sets the range check policy. When enabled, before being set, the value of all nodes with an #ArvGcFloat or
 * #ArvGcInteger interface will be checked against their Min and Max properties.
 *
 * <warning><para>Be aware that some camera may have wrong definition of Min and Max, as this check is defined as not
 * mandatory in the Genicam specification. If this is the case, it will not possible to set the value of the features
 * with faulty Min or Max definition. Range check is disabled by default.</para></warning>
 *
 * Since: 0.8.6
 */

void
arv_device_set_range_check_policy (ArvDevice *device, ArvRangeCheckPolicy policy)
{
	ArvGc *genicam;

	g_return_if_fail (ARV_IS_DEVICE (device));

	genicam = arv_device_get_genicam (device);
	arv_gc_set_range_check_policy (genicam, policy);
}

/**
 * arv_device_set_access_check_policy:
 * @device: a #ArvDevice
 * @policy: access check policy
 *
 * Sets the feature access check policy. When enabled, before being accessed, the actual read/write access of register
 * is checked using AccessMode properties. On some devices, it helps to avoid forbidden writes to registers that may put
 * the device in a bad state.
 *
 * <warning><para>Access check is disabled by default.</para></warning>
 *
 * Since: 0.8.22
 */

void
arv_device_set_access_check_policy (ArvDevice *device, ArvAccessCheckPolicy policy)
{
	ArvGc *genicam;

	g_return_if_fail (ARV_IS_DEVICE (device));

	genicam = arv_device_get_genicam (device);
	arv_gc_set_access_check_policy (genicam, policy);
}

void
arv_device_emit_control_lost_signal (ArvDevice *device)
{
	g_return_if_fail (ARV_IS_DEVICE (device));

	g_signal_emit (device, arv_device_signals[ARV_DEVICE_SIGNAL_CONTROL_LOST], 0);
}


void arv_device_take_init_error (ArvDevice *device, GError *error)
{
	ArvDevicePrivate *priv = arv_device_get_instance_private (device);

	g_return_if_fail (ARV_IS_DEVICE (device));

	g_clear_error (&priv->init_error);
	priv->init_error = error;
}

static void
arv_device_init (ArvDevice *device)
{
}

static void
arv_device_finalize (GObject *object)
{
	ArvDevicePrivate *priv = arv_device_get_instance_private (ARV_DEVICE (object));

	g_clear_error (&priv->init_error);

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

static gboolean
arv_device_initable_init (GInitable     *initable,
			  GCancellable  *cancellable,
			  GError       **error)
{
	ArvDevicePrivate *priv = arv_device_get_instance_private (ARV_DEVICE (initable));

	g_return_val_if_fail (ARV_IS_DEVICE (initable), FALSE);

	if (cancellable != NULL)
	{
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
				     "Cancellable initialization not supported");
		return FALSE;
	}

	if (priv->init_error) {
		if (error != NULL)
			*error = g_error_copy (priv->init_error);
		return FALSE;
	}

	return TRUE;
}

static void
arv_device_initable_iface_init (GInitableIface *iface)
{
	iface->init = arv_device_initable_init;
}

