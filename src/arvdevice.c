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
 * SECTION: arvdevice
 * @short_description: Abstract base class for device handling
 *
 * #ArvDevice is an abstract base class for the control of cameras. It provides
 * an easy access to the camera settings, and to its genicam interface for  more
 * advanced uses.
 */

#include <arvdevice.h>
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

static GObjectClass *parent_class = NULL;

struct  _ArvDevicePrivate {
	ArvDeviceStatus status;
	char *status_message;
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

static const char *
_get_genicam_xml (ArvDevice *device, size_t *size)
{
	*size = 0;

	return NULL;
}

/**
 * arv_device_get_genicam_xml:
 * @device: a #ArvDevice
 * @size: (out) (allow-none): placeholder for the returned data size (bytes) // BUG: (skip) seems ignored
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

	return ARV_DEVICE_GET_CLASS (device)->get_genicam_xml (device, size);
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

static void
_set_status (ArvDevice *device, ArvDeviceStatus status, const char *message)
{
	if (device->priv->status == ARV_DEVICE_STATUS_SUCCESS)
		return;

	arv_warning_device ("[ArvDevice::set_status] Status changed ('%s')", message);

	g_free (device->priv->status_message);
	device->priv->status = status;
	device->priv->status_message = g_strdup (message);
}

/**
 * arv_device_execute_command:
 * @device: a #ArvDevice
 * @feature: feature name
 *
 * Execute a genicam command. If an error occur, this function change the device status.
 *
 * Since: 0.2.0
 */

void
arv_device_execute_command (ArvDevice *device, const char *feature)
{
	ArvGcNode *node;

	g_return_if_fail (ARV_IS_DEVICE (device));

	node = arv_device_get_feature (device, feature);

	if (ARV_IS_GC_COMMAND (node)) {
		GError *error = NULL;

		arv_gc_command_execute (ARV_GC_COMMAND (node), &error);

		if (error != NULL) {
			_set_status (device, error->code, error->message);
			g_error_free (error);
		}
	} else
		arv_warning_device ("[ArvDevice::execute_command] Node '%s' is not a command",
				    feature);
}

/**
 * arv_device_set_boolean_feature_value:
 * @device: a #ArvDevice
 * @feature: feature name
 * @value: feature value
 *
 * Set the value of a boolean feature.
 * If this operation fails, the device status returned by arv_device_get_status() will be changed.
 *
 * Since: 0.6.0
 */

void
arv_device_set_boolean_feature_value (ArvDevice *device, const char *feature, gboolean value)
{
	ArvGcNode *node;
	GError *error = NULL;

	g_return_if_fail (ARV_IS_DEVICE (device));

	node = arv_device_get_feature (device, feature);

	if (ARV_IS_GC_INTEGER (node))
		arv_gc_boolean_set_value (ARV_GC_BOOLEAN (node), value, &error);
	else
		arv_warning_device ("[ArvDevice::set_boolean_feature_value] Node '%s' is not a boolean",
				    feature);

	if (error != NULL) {
		_set_status (device, error->code, error->message);
		g_error_free (error);
	}
}

/**
 * arv_device_get_boolean_feature_value:
 * @device: a #ArvDevice
 * @feature: feature name
 *
 * Returns: the feature value.
 *
 * If this operation fails, the device status returned by arv_device_get_status() will be changed.
 *
 * Since: 0.6.0
 */

gboolean
arv_device_get_boolean_feature_value (ArvDevice *device, const char *feature)
{
	ArvGcNode *node;
	GError *error = NULL;
	gboolean value = 0;

	g_return_val_if_fail (ARV_IS_DEVICE (device), 0);

	node = arv_device_get_feature (device, feature);

	if (ARV_IS_GC_BOOLEAN (node))
		value = arv_gc_boolean_get_value (ARV_GC_BOOLEAN (node), &error);
	else
		arv_warning_device ("[ArvDevice::get_boolean_feature_value] Node '%s' is not an boolean",
				    feature);

	if (error != NULL) {
		_set_status (device, error->code, error->message);
		g_error_free (error);
		return 0;
	}

	return value;
}

void
arv_device_set_string_feature_value (ArvDevice *device, const char *feature, const char *value)
{
	ArvGcNode *node;
	GError *error = NULL;

	g_return_if_fail (ARV_IS_DEVICE (device));

	node = arv_device_get_feature (device, feature);

	if (ARV_IS_GC_STRING (node))
		arv_gc_string_set_value (ARV_GC_STRING (node), value, &error);
	else
		arv_warning_device ("[ArvDevice::set_string_feature_value] Node '%s' is not a string",
				    feature);

	if (error != NULL) {
		_set_status (device, error->code, error->message);
		g_error_free (error);
	}
}

const char *
arv_device_get_string_feature_value (ArvDevice *device, const char *feature)
{
	ArvGcNode *node;
	GError *error = NULL;
	const char *string = NULL;

	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	node = arv_device_get_feature (device, feature);

	if (ARV_IS_GC_STRING (node))
		string = arv_gc_string_get_value (ARV_GC_STRING (node), &error);
	else {
		arv_warning_device ("[ArvDevice::get_string_feature_value] Node '%s' is not a string",
				    feature);
	}

	if (error != NULL) {
		_set_status (device, error->code, error->message);
		g_error_free (error);
		return NULL;
	}

	return string;
}

void
arv_device_set_integer_feature_value (ArvDevice *device, const char *feature, gint64 value)
{
	ArvGcNode *node;
	GError *error = NULL;

	g_return_if_fail (ARV_IS_DEVICE (device));

	node = arv_device_get_feature (device, feature);

	if (ARV_IS_GC_INTEGER (node))
		arv_gc_integer_set_value (ARV_GC_INTEGER (node), value, &error);
	else
		arv_warning_device ("[ArvDevice::set_integer_feature_value] Node '%s' is not an integer",
				    feature);

	if (error != NULL) {
		_set_status (device, error->code, error->message);
		g_error_free (error);
	}
}

gint64
arv_device_get_integer_feature_value (ArvDevice *device, const char *feature)
{
	ArvGcNode *node;
	GError *error = NULL;
	gint64 value = 0;

	g_return_val_if_fail (ARV_IS_DEVICE (device), 0);

	node = arv_device_get_feature (device, feature);

	if (ARV_IS_GC_INTEGER (node))
		value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), &error);
	else
		arv_warning_device ("[ArvDevice::get_integer_feature_value] Node '%s' is not an integer",
				    feature);

	if (error != NULL) {
		_set_status (device, error->code, error->message);
		g_error_free (error);
		return 0;
	}

	return value;
}

void
arv_device_get_integer_feature_bounds (ArvDevice *device, const char *feature, gint64 *min, gint64 *max)
{
	ArvGcNode *node;
	GError *error = NULL;

	if (min != NULL)
		*min = -G_MAXINT64;
	if (max != NULL)
		*max = G_MAXINT64;

	g_return_if_fail (ARV_IS_DEVICE (device));

	node = arv_device_get_feature (device, feature);

	if (ARV_IS_GC_INTEGER (node)) {
		gint64 minimum;
		gint64 maximum;

		minimum = arv_gc_integer_get_min (ARV_GC_INTEGER (node), &error);

		if (error != NULL) {
			_set_status (device, error->code, error->message);
			g_error_free (error);

			return;
		}

		maximum = arv_gc_integer_get_max (ARV_GC_INTEGER (node), &error);

		if (error != NULL) {
			_set_status (device, error->code, error->message);
			g_error_free (error);

			return;
		}

		if (min != NULL)
			*min = minimum;
		if (max != NULL)
			*max = maximum;
	} else
		arv_warning_device ("[ArvDevice::get_integer_feature_bounds] Node '%s' is not an integer",
				    feature);
}

void
arv_device_set_float_feature_value (ArvDevice *device, const char *feature, double value)
{
	ArvGcNode *node;
	GError *error = NULL;

	g_return_if_fail (ARV_IS_DEVICE (device));

	node = arv_device_get_feature (device, feature);

	if (ARV_IS_GC_FLOAT (node))
		arv_gc_float_set_value (ARV_GC_FLOAT (node), value, &error);
	else
		arv_warning_device ("[ArvDevice::set_float_feature_value] Node '%s' is not a float",
				    feature);

	if (error != NULL) {
		_set_status (device, error->code, error->message);
		g_error_free (error);
	}
}

double
arv_device_get_float_feature_value (ArvDevice *device, const char *feature)
{
	ArvGcNode *node;
	GError *error = NULL;
	double value = 0.0;

	g_return_val_if_fail (ARV_IS_DEVICE (device), 0.0);

	node = arv_device_get_feature (device, feature);

	if (ARV_IS_GC_FLOAT (node))
		value = arv_gc_float_get_value (ARV_GC_FLOAT (node), &error);
	else
		arv_warning_device ("[ArvDevice::get_float_feature_value] Node '%s' is not a float",
				    feature);

	if (error != NULL) {
		_set_status (device, error->code, error->message);
		g_error_free (error);
		return 0.0;
	}

	return value;
}

void
arv_device_get_float_feature_bounds (ArvDevice *device, const char *feature, double *min, double *max)
{
	ArvGcNode *node;
	GError *error = NULL;

	if (min != NULL)
		*min = G_MINDOUBLE;
	if (max != NULL)
		*max = G_MAXDOUBLE;

	g_return_if_fail (ARV_IS_DEVICE (device));

	node = arv_device_get_feature (device, feature);

	if (ARV_IS_GC_FLOAT (node)) {
		double minimum;
		double maximum;

		minimum = arv_gc_float_get_min (ARV_GC_FLOAT (node), &error);

		if (error != NULL) {
			_set_status (device, error->code, error->message);
			g_error_free (error);

			return;
		}

		maximum = arv_gc_float_get_max (ARV_GC_FLOAT (node), &error);

		if (error != NULL) {
			_set_status (device, error->code, error->message);
			g_error_free (error);

			return;
		}

		if (min != NULL)
			*min = minimum;
		if (max != NULL)
			*max = maximum;
	} else
		arv_warning_device ("[ArvDevice::get_float_feature_bounds] Node '%s' is not a float",
				    feature);
}

gint64 *
arv_device_get_available_enumeration_feature_values (ArvDevice *device, const char *feature, guint *n_values)
{
	ArvGcNode *node;
	GError *error = NULL;
	gint64 *values = NULL;

	if (n_values != NULL)
		*n_values = 0;

	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	node = arv_device_get_feature (device, feature);

	if (ARV_IS_GC_ENUMERATION (node))
		values = arv_gc_enumeration_get_available_int_values (ARV_GC_ENUMERATION (node), n_values, &error);
	else
		arv_warning_device ("[ArvDevice::get_enumeration_feature_available_values] Node '%s' is not an enumeration",
				    feature);

	if (error != NULL) {
		_set_status (device, error->code, error->message);
		g_error_free (error);

		return NULL;
	}

	return values;
}

/**
 * arv_device_get_available_enumeration_feature_values_as_strings:
 * @device: an #ArvDevice
 * @feature: feature name
 * @n_values: placeholder for the number of returned values
 *
 * Get all the available values of @feature, as strings.
 *
 * Returns: (array length=n_values) (transfer container): a newly created array of const strings, which must freed after use using g_free.
 *
 * Since: 0.2.0
 */

const char **
arv_device_get_available_enumeration_feature_values_as_strings (ArvDevice *device, const char *feature, guint *n_values)
{
	ArvGcNode *node;
	GError *error = NULL;
	const char ** strings = NULL;

	if (n_values != NULL)
		*n_values = 0;

	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	node = arv_device_get_feature (device, feature);

	if (ARV_IS_GC_ENUMERATION (node))
		strings = arv_gc_enumeration_get_available_string_values (ARV_GC_ENUMERATION (node), n_values, &error);
	else
		arv_warning_device ("[ArvDevice::get_enumeration_feature_available_strings] Node '%s' is not an enumeration",
				    feature);

	if (error != NULL) {
		_set_status (device, error->code, error->message);
		g_error_free (error);

		return NULL;
	}

	return strings;
}

ArvDeviceStatus
arv_device_get_status (ArvDevice *device)
{
	ArvDeviceStatus status;

	g_return_val_if_fail (ARV_IS_DEVICE (device), ARV_DEVICE_STATUS_UNKNOWN);

	status = device->priv->status;

	g_free (device->priv->status_message);
	device->priv->status = ARV_DEVICE_STATUS_SUCCESS;
	device->priv->status_message = NULL;

	return status;
}

void
arv_device_emit_control_lost_signal (ArvDevice *device)
{
	g_return_if_fail (ARV_IS_DEVICE (device));

	g_signal_emit (device, arv_device_signals[ARV_DEVICE_SIGNAL_CONTROL_LOST], 0);
}

static void
arv_device_init (ArvDevice *device)
{
	device->priv = G_TYPE_INSTANCE_GET_PRIVATE (device, ARV_TYPE_DEVICE, ArvDevicePrivate);

	device->priv->status = ARV_DEVICE_STATUS_SUCCESS;
	device->priv->status_message = NULL;
}

static void
arv_device_finalize (GObject *object)
{
	ArvDevice *device = ARV_DEVICE (object);

	g_free (device->priv->status_message);

	parent_class->finalize (object);
}

static void
arv_device_class_init (ArvDeviceClass *device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (device_class);

	g_type_class_add_private (device_class, sizeof (ArvDevicePrivate));

	parent_class = g_type_class_peek_parent (device_class);

	object_class->finalize = arv_device_finalize;

	device_class->get_genicam_xml = _get_genicam_xml;

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

G_DEFINE_ABSTRACT_TYPE (ArvDevice, arv_device, G_TYPE_OBJECT)
