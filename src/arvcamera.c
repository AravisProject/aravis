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
 * SECTION:arvcamera
 * @short_description: Class for generic camera control
 *
 * [class@Aravis.Camera] is a class for the generic control of cameras. It hides the complexity of the genicam interface
 * by providing a simple API, with the drawback of not exposing all the available features. See [class@Aravis.Device]
 * and [class@Aravis.Gc] for a more advanced use of the Aravis library.
 */

#include <arvfeatures.h>
#include <arvdebug.h>
#include <arvcamera.h>
#include <arvsystem.h>
#include <arvgvinterface.h>
#include <arvgcboolean.h>
#include <arvgccommand.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcenumeration.h>
#include <arvgcenumentry.h>
#include <arvgcstring.h>
#include <arvbuffer.h>
#include <arvgc.h>
#include <arvgvdevice.h>
#if ARAVIS_HAS_USB
#include <arvuvdevice.h>
#endif
#include <arvenums.h>
#include <arvstr.h>

static void arv_camera_get_integer_bounds_as_gint (ArvCamera *camera, const char *feature, gint *min, gint *max, GError **error);
static void arv_camera_get_integer_bounds_as_guint (ArvCamera *camera, const char *feature, guint *min, guint *max, GError **error);
static void arv_camera_get_integer_bounds_as_double (ArvCamera *camera, const char *feature, double *min, double *max, GError **error);

/**
 * ArvCameraVendor:
 * @ARV_CAMERA_VENDOR_UNKNOWN: unknown camera vendor
 * @ARV_CAMERA_VENDOR_BASLER: Basler
 * @ARV_CAMERA_VENDOR_PROSILICA: Prosilica
 * @ARV_CAMERA_VENDOR_TIS: The Imaging Source
 * @ARV_CAMERA_VENDOR_POINT_GREY_FLIR: PointGrey / FLIR
 * @ARV_CAMERA_VENDOR_XIMEA: XIMEA GmbH
 * @ARV_CAMERA_VENDOR_MATRIX_VISION: Matrix Vision GmbH
 * @ARV_CAMERA_VENDOR_IMPERX: Imperx, Inc
 */

typedef enum {
	ARV_CAMERA_VENDOR_UNKNOWN,
	ARV_CAMERA_VENDOR_BASLER,
	ARV_CAMERA_VENDOR_DALSA,
	ARV_CAMERA_VENDOR_PROSILICA,
	ARV_CAMERA_VENDOR_TIS,
	ARV_CAMERA_VENDOR_POINT_GREY_FLIR,
	ARV_CAMERA_VENDOR_RICOH,
	ARV_CAMERA_VENDOR_XIMEA,
	ARV_CAMERA_VENDOR_MATRIX_VISION,
	ARV_CAMERA_VENDOR_IMPERX
} ArvCameraVendor;

typedef enum {
	ARV_CAMERA_SERIES_UNKNOWN,
	ARV_CAMERA_SERIES_BASLER_ACE,
	ARV_CAMERA_SERIES_BASLER_SCOUT,
	ARV_CAMERA_SERIES_BASLER_OTHER,
	ARV_CAMERA_SERIES_DALSA,
	ARV_CAMERA_SERIES_PROSILICA,
	ARV_CAMERA_SERIES_TIS,
	ARV_CAMERA_SERIES_POINT_GREY_FLIR,
	ARV_CAMERA_SERIES_RICOH,
	ARV_CAMERA_SERIES_XIMEA,
	ARV_CAMERA_SERIES_MATRIX_VISION,
	ARV_CAMERA_SERIES_IMPERX_CHEETAH,
	ARV_CAMERA_SERIES_IMPERX_OTHER
} ArvCameraSeries;

typedef struct {
	char *name;
	ArvDevice *device;
	ArvGc *genicam;

	ArvCameraVendor vendor;
	ArvCameraSeries series;

	gboolean has_serial_number;

	gboolean has_gain;
	gboolean gain_raw_as_float;
	gboolean gain_abs_as_float;

	gboolean has_brightness;
	gboolean has_black_level_raw;
        gboolean has_black_level;

	gboolean has_exposure_time;
	gboolean has_acquisition_frame_rate;
	gboolean has_acquisition_frame_rate_auto;
	gboolean has_acquisition_frame_rate_enabled;

        gboolean has_region_offset;

	GError *init_error;
} ArvCameraPrivate;

static void arv_camera_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (ArvCamera, arv_camera, G_TYPE_OBJECT,
			 G_ADD_PRIVATE (ArvCamera)
			 G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, arv_camera_initable_iface_init))

enum
{
	PROP_0,
	PROP_CAMERA_NAME,
	PROP_CAMERA_DEVICE
};

/**
 * arv_camera_create_stream: (skip)
 * @camera: a #ArvCamera
 * @callback: (scope call) (allow-none): a frame processing callback
 * @user_data: (closure) (allow-none): user data for @callback
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Creates a new [class@ArvStream] for video stream reception. See
 * [callback@ArvStreamCallback] for details regarding the callback function.
 *
 * Returns: (transfer full): a new [class@ArvStream], to be freed after use with [method@GObject.Object.unref].
 *
 * Since: 0.2.0
 */

ArvStream *
arv_camera_create_stream (ArvCamera *camera, ArvStreamCallback callback, gpointer user_data, GError **error)
{
	return arv_camera_create_stream_full(camera, callback, user_data, NULL, error);
}

/**
 * arv_camera_create_stream_full: (rename-to arv_camera_create_stream)
 * @camera: a #ArvCamera
 * @callback: (scope notified) (allow-none): a frame processing callback
 * @user_data: (closure) (allow-none): user data for @callback
 * @destroy: a #GDestroyNotify placeholder, %NULL to ignore
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Creates a new [class@ArvStream] for video stream reception. See
 * [callback@ArvStreamCallback] for details regarding the callback function.
 *
 * Returns: (transfer full): a new [class@ArvStream], to be freed after use with [method@GObject.Object.unref].
 *
 * Since: 0.8.23
 */

ArvStream *
arv_camera_create_stream_full (ArvCamera *camera, ArvStreamCallback callback, gpointer user_data, GDestroyNotify destroy, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_create_stream_full (priv->device, callback, user_data, destroy, error);
}

/* Device control */

/**
 * arv_camera_get_vendor_name:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the camera vendor name.
 *
 * Since: 0.8.0
 */

const char *
arv_camera_get_vendor_name (ArvCamera *camera, GError **error)
{
	return arv_camera_get_string (camera, "DeviceVendorName", error);
}

/**
 * arv_camera_get_model_name:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the camera model name.
 *
 * Since: 0.8.0
 */

const char *
arv_camera_get_model_name (ArvCamera *camera, GError **error)
{
	return arv_camera_get_string (camera, "DeviceModelName", error);
}

/**
 * arv_camera_get_device_serial_number:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the camera device serial number.
 *
 * Since: 0.8.8
 */

const char *
arv_camera_get_device_serial_number (ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	if (priv->has_serial_number)
	    return arv_camera_get_string (camera, "DeviceSerialNumber", error);

	return arv_camera_get_device_id (camera, error);
}

/**
 * arv_camera_get_device_id:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the camera device ID.
 *
 * Since: 0.8.0
 */

const char *
arv_camera_get_device_id (ArvCamera *camera, GError **error)
{
	return arv_camera_get_string (camera, "DeviceID", error);
}

/* Image format control */

/**
 * arv_camera_get_sensor_size:
 * @camera: a #ArvCamera
 * @width: (out): camera sensor width
 * @height: (out): camera sensor height
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Since: 0.8.0
 */

void
arv_camera_get_sensor_size (ArvCamera *camera, gint *width, gint *height, GError **error)
{
	GError *local_error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (width != NULL)
		*width = arv_camera_get_integer (camera, "SensorWidth", &local_error);
	if (height != NULL && local_error == NULL)
		*height = arv_camera_get_integer (camera, "SensorHeight", &local_error);

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

/**
 * arv_camera_is_region_offset_available:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: %TRUE if OffsetX and OffsetY features are available.
 *
 * Since: 0.8.22
 */

gboolean
arv_camera_is_region_offset_available (ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
        GError *local_error = NULL;
        gboolean has_offset_x, has_offset_y;

        g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

        if (!priv->has_region_offset)
                return FALSE;

        has_offset_x = arv_camera_is_feature_available (camera, "OffsetX", &local_error);
        if (local_error == NULL)
                has_offset_y = arv_camera_is_feature_available (camera, "OffsetY", &local_error);

        if (local_error != NULL) {
                g_propagate_error (error, local_error);
                return FALSE;
        }

        return has_offset_x && has_offset_y;
}

/**
 * arv_camera_set_region:
 * @camera: a #ArvCamera
 * @x: x offset
 * @y: y_offset
 * @width: region width
 * @height: region height
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Defines the region of interest which will be transmitted in the video
 * stream. Negative @x or @y values, or not strictly positive @width or @height values are ignored.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_region (ArvCamera *camera, gint x, gint y, gint width, gint height, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *local_error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));

        if (priv->has_region_offset) {
                if (x >= 0)
                        arv_camera_set_integer (camera, "OffsetX", 0, &local_error);
                if (y >= 0 && local_error == NULL)
                        arv_camera_set_integer (camera, "OffsetY", 0, &local_error);
        }

        if (width > 0 && local_error == NULL)
                arv_camera_set_integer (camera, "Width", width, &local_error);
        if (height > 0 && local_error == NULL)
                arv_camera_set_integer (camera, "Height", height, &local_error);

        if (priv->has_region_offset) {
                if (x >= 0 && local_error == NULL)
                        arv_camera_set_integer (camera, "OffsetX", x, &local_error);
                if (y >= 0 && local_error == NULL)
                        arv_camera_set_integer (camera, "OffsetY", y, &local_error);
        }

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

/**
 * arv_camera_get_region:
 * @camera: a #ArvCamera
 * @x: (out): x offset
 * @y: (out): y_offset
 * @width: (out): region width
 * @height: (out): region height
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the current region of interest.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_region (ArvCamera *camera, gint *x, gint *y, gint *width, gint *height, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *local_error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (x != NULL)
		*x = priv->has_region_offset ? arv_camera_get_integer (camera, "OffsetX", &local_error) : 0;
	if (y != NULL && local_error == NULL)
		*y = priv->has_region_offset ? arv_camera_get_integer (camera, "OffsetY", &local_error) : 0;
	if (width != NULL && local_error == NULL)
		*width = arv_camera_get_integer (camera, "Width", &local_error);
	if (height != NULL && local_error == NULL)
		*height = arv_camera_get_integer (camera, "Height", &local_error);

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

/**
 * arv_camera_get_x_offset_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum offset
 * @max: (out): maximum offset
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the valid range for image horizontal offset.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_x_offset_bounds (ArvCamera *camera, gint *min, gint *max, GError **error)
{
	arv_camera_get_integer_bounds_as_gint (camera, "OffsetX", min, max, error);
}

/**
 * arv_camera_get_x_offset_increment:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: horizontal offset value increment.
 *
 * Since: 0.8.0
 */

gint
arv_camera_get_x_offset_increment (ArvCamera *camera, GError **error)
{
	return arv_camera_get_integer_increment (camera, "OffsetX", error);
}

/**
 * arv_camera_get_y_offset_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum offset
 * @max: (out): maximum offset
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the valid range for image vertical offset.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_y_offset_bounds (ArvCamera *camera, gint *min, gint *max, GError **error)
{
	arv_camera_get_integer_bounds_as_gint (camera, "OffsetY", min, max, error);
}

/**
 * arv_camera_get_y_offset_increment:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: vertical offset value increment.
 *
 * Since: 0.8.0
 */

gint
arv_camera_get_y_offset_increment (ArvCamera *camera, GError **error)
{
	return arv_camera_get_integer_increment (camera, "OffsetY", error);
}

/**
 * arv_camera_get_width_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum width
 * @max: (out): maximum width
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the valid range for image width.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_width_bounds (ArvCamera *camera, gint *min, gint *max, GError **error)
{
	arv_camera_get_integer_bounds_as_gint (camera, "Width", min, max, error);
}

/**
 * arv_camera_get_width_increment:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: width value increment.
 *
 * Since: 0.8.0
 */

gint
arv_camera_get_width_increment (ArvCamera *camera, GError **error)
{
	return arv_camera_get_integer_increment (camera, "Width", error);
}

/**
 * arv_camera_get_height_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum height
 * @max: (out): maximum height
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the valid range for image height.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_height_bounds (ArvCamera *camera, gint *min, gint *max, GError **error)
{
	arv_camera_get_integer_bounds_as_gint (camera, "Height", min, max, error);
}

/**
 * arv_camera_get_height_increment:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: height value increment.
 *
 * Since: 0.8.0
 */

gint
arv_camera_get_height_increment (ArvCamera *camera, GError **error)
{
	return arv_camera_get_integer_increment (camera, "Height", error);
}

/**
 * arv_camera_set_binning:
 * @camera: a #ArvCamera
 * @dx: horizontal binning
 * @dy: vertical binning
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Defines binning in both directions. Not all cameras support this
 * feature. Negative @dx or @dy values are ignored.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_binning (ArvCamera *camera, gint dx, gint dy, GError **error)
{
	GError *local_error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (dx > 0)
		arv_camera_set_integer (camera, "BinningHorizontal", dx, &local_error);
	if (dy > 0 && local_error == NULL)
		arv_camera_set_integer (camera, "BinningVertical", dy, &local_error);

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

/**
 * arv_camera_get_binning:
 * @camera: a #ArvCamera
 * @dx: (out): horizontal binning placeholder
 * @dy: (out): vertical binning placeholder
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves binning in both directions.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_binning (ArvCamera *camera, gint *dx, gint *dy, GError **error)
{
	GError *local_error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (dx != NULL)
		*dx = arv_camera_get_integer (camera, "BinningHorizontal", &local_error);
	if (dy != NULL && local_error == NULL)
		*dy = arv_camera_get_integer (camera, "BinningVertical", &local_error);

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

/**
 * arv_camera_get_x_binning_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum binning
 * @max: (out): maximum binning
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the valid range for image horizontal binning.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_x_binning_bounds (ArvCamera *camera, gint *min, gint *max, GError **error)
{
	arv_camera_get_integer_bounds_as_gint (camera, "BinningHorizontal", min, max, error);
}

/**
 * arv_camera_get_x_binning_increment:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: horizontal binning value increment.
 *
 * Since: 0.8.0
 */

gint
arv_camera_get_x_binning_increment (ArvCamera *camera, GError **error)
{
	return arv_camera_get_integer_increment (camera, "BinningHorizontal", error);
}

/**
 * arv_camera_get_y_binning_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum binning
 * @max: (out): maximum binning
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the valid range for image vertical binning.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_y_binning_bounds (ArvCamera *camera, gint *min, gint *max, GError **error)
{
	arv_camera_get_integer_bounds_as_gint (camera, "BinningVertical", min, max, error);
}

/**
 * arv_camera_get_y_binning_increment:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: vertical binning value increment.
 *
 * Since: 0.8.0
 */

gint
arv_camera_get_y_binning_increment (ArvCamera *camera, GError **error)
{
	return arv_camera_get_integer_increment (camera, "BinningVertical", error);
}

/**
 * arv_camera_set_pixel_format:
 * @camera: a #ArvCamera
 * @format: pixel format
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Defines pixel format.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_pixel_format (ArvCamera *camera, ArvPixelFormat format, GError **error)
{
	arv_camera_set_integer (camera, "PixelFormat", format, error);
}

/**
 * arv_camera_set_pixel_format_from_string:
 * @camera: a #ArvCamera
 * @format: pixel format
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Defines pixel format described by a string.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_pixel_format_from_string (ArvCamera *camera, const char * format, GError **error)
{
        arv_camera_set_string (camera, "PixelFormat", format, error);
}

/**
 * arv_camera_get_pixel_format:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: pixel format.
 *
 * Since: 0.8.0
 */

ArvPixelFormat
arv_camera_get_pixel_format (ArvCamera *camera, GError **error)
{
	return arv_camera_get_integer (camera, "PixelFormat", error);
}

/**
 * arv_camera_get_pixel_format_as_string:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retuns: pixel format as string, NULL on error.
 *
 * Since: 0.8.0
 */

const char *
arv_camera_get_pixel_format_as_string (ArvCamera *camera, GError **error)
{
	return arv_camera_get_string (camera, "PixelFormat", error);
}

/**
 * arv_camera_dup_available_pixel_formats:
 * @camera: a #ArvCamera
 * @n_pixel_formats: (out): number of different pixel formats
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the list of all available pixel formats.
 *
 * Returns: (array length=n_pixel_formats) (transfer container): a newly allocated array of #ArvPixelFormat, to be freed after use with
 * g_free().
 *
 * Since: 0.8.0
 */

gint64 *
arv_camera_dup_available_pixel_formats (ArvCamera *camera, guint *n_pixel_formats, GError **error)
{
	return arv_camera_dup_available_enumerations (camera, "PixelFormat", n_pixel_formats, error);
}

/**
 * arv_camera_dup_available_pixel_formats_as_strings:
 * @camera: a #ArvCamera
 * @n_pixel_formats: (out): number of different pixel formats
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the list of all available pixel formats as strings.
 *
 * Returns: (array length=n_pixel_formats) (transfer container): a newly allocated array of strings, to be freed after use with
 * g_free().
 *
 * Since: 0.8.0
 */

const char **
arv_camera_dup_available_pixel_formats_as_strings (ArvCamera *camera, guint *n_pixel_formats, GError **error)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_camera_dup_available_enumerations_as_strings (camera, "PixelFormat", n_pixel_formats, error);
}

/**
 * arv_camera_dup_available_pixel_formats_as_display_names:
 * @camera: a #ArvCamera
 * @n_pixel_formats: (out): number of different pixel formats
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the list of all available pixel formats as display names.
 * In general, these human-readable strings cannot be used as settings.
 *
 * Returns: (array length=n_pixel_formats) (transfer container): a newly allocated array of string constants, to be freed after use with
 * g_free().
 *
 * Since: 0.8.0
 */

const char **
arv_camera_dup_available_pixel_formats_as_display_names (ArvCamera *camera, guint *n_pixel_formats, GError **error)
{
	return arv_camera_dup_available_enumerations_as_display_names (camera, "PixelFormat", n_pixel_formats, error);
}

/* Acquisition control */

/**
 * arv_camera_start_acquisition:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Starts video stream acquisition.
 *
 * Since: 0.8.0
 */

void
arv_camera_start_acquisition (ArvCamera *camera, GError **error)
{
	arv_camera_execute_command (camera, "AcquisitionStart", error);
}

/**
 * arv_camera_stop_acquisition:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Stops video stream acquisition.
 *
 * Since: 0.8.0
 */

void
arv_camera_stop_acquisition (ArvCamera *camera, GError **error)
{
	arv_camera_execute_command (camera, "AcquisitionStop", error);
}

/**
 * arv_camera_abort_acquisition:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Aborts video stream acquisition.
 *
 * Since: 0.8.0
 */

void
arv_camera_abort_acquisition (ArvCamera *camera, GError **error)
{
	arv_camera_execute_command (camera, "AcquisitionAbort", error);
}

/**
 * arv_camera_acquisition:
 * @camera: a #ArvCamera
 * @timeout: acquisition timeout in µs. Zero means no timeout.
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Acquire one image buffer.
 *
 * <warning>
 *   <para>arv_camera_acquisition() sets the camera in SingleFrame acquisition mode. You may have to put back the camera in
 *   Continuous acquisition mode for later operations, using arv_camera_set_acquisition_mode().</para>
 * </warning>
 *
 * Returns: (transfer full): A new #ArvBuffer, NULL on error. The returned buffer must be freed using g_object_unref().
 *
 * Since: 0.8.0
 */

ArvBuffer *
arv_camera_acquisition (ArvCamera *camera, guint64 timeout, GError **error)
{
	GError *local_error = NULL;
	ArvStream *stream;
	ArvBuffer *buffer = NULL;
	gint payload;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	stream = arv_camera_create_stream (camera, NULL, NULL, &local_error);
	if (ARV_IS_STREAM(stream)) {
		payload = arv_camera_get_payload (camera, &local_error);
		if (local_error == NULL) {
			arv_stream_push_buffer (stream,  arv_buffer_new (payload, NULL));
			arv_camera_set_acquisition_mode (camera, ARV_ACQUISITION_MODE_SINGLE_FRAME, &local_error);
                        if (local_error != NULL &&
                            local_error->code == ARV_GC_ERROR_ENUM_ENTRY_NOT_FOUND) {
                                g_clear_error (&local_error);
                                /* Some cameras don't support SingleFrame, fall back to Continuous */
                                arv_camera_set_acquisition_mode (camera, ARV_ACQUISITION_MODE_CONTINUOUS, &local_error);
                        }
		}
		if (local_error == NULL)
			arv_camera_start_acquisition (camera, &local_error);
		if (local_error == NULL) {
			if (timeout > 0)
				buffer = arv_stream_timeout_pop_buffer (stream, timeout);
			else
				buffer = arv_stream_pop_buffer (stream);
			arv_camera_stop_acquisition (camera, &local_error);
		}

		g_object_unref (stream);
	}

	if (local_error != NULL)
		g_propagate_error (error, local_error);

	return buffer;
}

/*
 * arv_camera_set_acquisition_mode:
 * @camera: a #ArvCamera
 * @acquisition_mode: acquisition mode
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Defines acquisition mode.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_acquisition_mode (ArvCamera *camera, ArvAcquisitionMode acquisition_mode, GError **error)
{
	arv_camera_set_string (camera, "AcquisitionMode", arv_acquisition_mode_to_string (acquisition_mode), error);
}

/**
 * arv_camera_get_acquisition_mode:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: acquisition mode.
 *
 * Since: 0.8.0
 */

ArvAcquisitionMode
arv_camera_get_acquisition_mode (ArvCamera *camera, GError **error)
{
	const char *string;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0);

	string = arv_camera_get_string (camera, "AcquisitionMode", error);

	return arv_acquisition_mode_from_string (string);
}

/**
 * arv_camera_set_frame_count:
 * @camera: a #ArvCamera
 * @frame_count: number of frames to capture in MultiFrame mode
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Sets the number of frames to capture in MultiFrame mode.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_frame_count (ArvCamera *camera, gint64 frame_count, GError **error)
{
	GError *local_error = NULL;
	gint64 minimum;
	gint64 maximum;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (frame_count <= 0)
		return;

	arv_camera_get_frame_count_bounds (camera, &minimum, &maximum, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	if (frame_count < minimum)
		frame_count = minimum;
	if (frame_count > maximum)
		frame_count = maximum;

	arv_camera_set_integer (camera, "AcquisitionFrameCount", frame_count, error);
}

/**
 * arv_camera_get_frame_count:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: number of frames to capture in MultiFrame mode.
 *
 * Since: 0.8.0
 */

gint64
arv_camera_get_frame_count (ArvCamera *camera, GError **error)
{
	return arv_camera_get_integer (camera, "AcquisitionFrameCount", error);
}

/**
 * arv_camera_get_frame_count_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimal possible frame count
 * @max: (out): maximum possible frame count
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves allowed range for frame count.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_frame_count_bounds (ArvCamera *camera, gint64 *min, gint64 *max, GError **error)
{
	arv_camera_get_integer_bounds (camera, "AcquisitionFrameCount", min, max, error);
}

/**
 * arv_camera_set_frame_rate:
 * @camera: a #ArvCamera
 * @frame_rate: frame rate, in Hz
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Configures a fixed frame rate mode. Once acquisition start is triggered, the video stream will be acquired with the
 * given frame rate. A negative or zero @frame_rate value disables the frame rate limit.
 *
 * All triggers are disabled.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_frame_rate (ArvCamera *camera, double frame_rate, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *local_error = NULL;
	ArvGcNode *feature;
	double minimum;
	double maximum;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (frame_rate <= 0.0) {
		arv_camera_set_frame_rate_enable(camera, FALSE, &local_error);
		if (local_error != NULL)
		{
			g_propagate_error (error, local_error);
		}
		return;
	}

	/* Ignore the error in order to be able to change the frame rate during the acquisition, as some devices don't
	* allow to change TriggerMode if the acquisition is already started. */
	arv_camera_clear_triggers (camera, NULL);

	arv_camera_get_frame_rate_bounds (camera, &minimum, &maximum, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	if (frame_rate < minimum)
		frame_rate = minimum;
	if (frame_rate > maximum)
		frame_rate = maximum;

	switch (priv->vendor) {
		case ARV_CAMERA_VENDOR_BASLER:
			if (local_error == NULL){
				arv_camera_set_frame_rate_enable(camera, TRUE, &local_error);
			}
			if (local_error == NULL)
				arv_camera_set_float (camera,
						      priv->has_acquisition_frame_rate ?
						      "AcquisitionFrameRate":
						      "AcquisitionFrameRateAbs", frame_rate, &local_error);
			break;
		case ARV_CAMERA_VENDOR_PROSILICA:
			if (local_error == NULL)
				arv_camera_set_float (camera, "AcquisitionFrameRateAbs", frame_rate, &local_error);
			break;
		case ARV_CAMERA_VENDOR_TIS:
			if (local_error == NULL) {
				feature = arv_device_get_feature (priv->device, "FPS");
				if (ARV_IS_GC_ENUMERATION (feature)) {
					gint64 *values;
					guint n_values;
					guint i;

					values = arv_camera_dup_available_enumerations (camera, "FPS",
                                                                                        &n_values, &local_error);
					for (i = 0; i < n_values && local_error == NULL; i++) {
						if (values[i] > 0) {
							double e;

							e = (int)((10000000/(double) values[i]) * 100 + 0.5) / 100.0;
							if (e == frame_rate) {
								arv_camera_set_integer (camera, "FPS",
                                                                                        values[i], &local_error);
								break;
							}
						}
					}
					g_free (values);
				} else
					arv_camera_set_float (camera, "FPS", frame_rate, &local_error);
			}
			break;
		case ARV_CAMERA_VENDOR_POINT_GREY_FLIR:
			arv_camera_set_frame_rate_enable(camera, TRUE, &local_error);
			if (local_error == NULL && priv->has_acquisition_frame_rate_auto) {
				arv_camera_set_string (camera, "AcquisitionFrameRateAuto", "Off", &local_error);
			}

			if (local_error == NULL) {
				arv_camera_set_float (camera, "AcquisitionFrameRate", frame_rate, &local_error);
			}

			break;
		case ARV_CAMERA_VENDOR_DALSA:
		case ARV_CAMERA_VENDOR_RICOH:
		case ARV_CAMERA_VENDOR_XIMEA:
		case ARV_CAMERA_VENDOR_MATRIX_VISION:
		case ARV_CAMERA_VENDOR_IMPERX:
		case ARV_CAMERA_VENDOR_UNKNOWN:
			arv_camera_set_frame_rate_enable(camera, TRUE, &local_error);
			if (local_error == NULL)
				arv_camera_set_float (camera,
                                                      priv->has_acquisition_frame_rate ?
                                                      "AcquisitionFrameRate":
                                                      "AcquisitionFrameRateAbs", frame_rate, &local_error);
                        break;
        }

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

/**
 * arv_camera_get_frame_rate:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: actual frame rate, in Hz.
 *
 * Since: 0.8.0
 */

double
arv_camera_get_frame_rate (ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	ArvGcNode *feature;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0);

	switch (priv->vendor) {
		case ARV_CAMERA_VENDOR_PROSILICA:
			return arv_camera_get_float (camera, "AcquisitionFrameRateAbs", error);
		case ARV_CAMERA_VENDOR_TIS:
			{
				feature = arv_device_get_feature (priv->device, "FPS");
				if (ARV_IS_GC_ENUMERATION (feature)) {
					gint64 i;

					i = arv_camera_get_integer (camera, "FPS", error);

					if (i > 0)
						return (int)((10000000/(double) i) * 100 + 0.5) / 100.0;
					else
						return 0;
				} else
					return arv_camera_get_float (camera, "FPS", error);
			}
		case ARV_CAMERA_VENDOR_POINT_GREY_FLIR:
		case ARV_CAMERA_VENDOR_DALSA:
		case ARV_CAMERA_VENDOR_RICOH:
		case ARV_CAMERA_VENDOR_BASLER:
		case ARV_CAMERA_VENDOR_XIMEA:
		case ARV_CAMERA_VENDOR_MATRIX_VISION:
		case ARV_CAMERA_VENDOR_IMPERX:
		case ARV_CAMERA_VENDOR_UNKNOWN:
			return arv_camera_get_float (camera,
						     priv->has_acquisition_frame_rate ?
						     "AcquisitionFrameRate": "AcquisitionFrameRateAbs",
						     error);
	}

	return 0;
}

/**
 * arv_camera_get_frame_rate_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimal possible framerate
 * @max: (out): maximum possible framerate
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves allowed range for framerate.
 *
 * Since 0.8.0
 */

void
arv_camera_get_frame_rate_bounds (ArvCamera *camera, double *min, double *max, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	ArvGcNode *feature;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	switch (priv->vendor) {
		case ARV_CAMERA_VENDOR_TIS:
			feature = arv_device_get_feature (priv->device, "FPS");
			if (ARV_IS_GC_ENUMERATION (feature)) {
				GError *local_error = NULL;
				gint64 *values;
				guint n_values;
				guint i;

				values = arv_camera_dup_available_enumerations (camera, "FPS", &n_values, &local_error);
				if (local_error != NULL) {
					g_propagate_error (error, local_error);
					return;
				}

				if (max != NULL)
					*max = 0;
				if (min != NULL)
					*min = 0;

				for (i = 0; i < n_values; i++) {
					if (values[i] > 0) {
						double s;

						s = (int)((10000000/(double) values[i]) * 100 + 0.5) / 100.0;

						if (max != NULL && s > *max)
							*max = s;
						if (min != NULL && (*min == 0 || *min > s))
							*min = s;
					}
				}
				g_free (values);

				return;
			} else
				arv_camera_get_float_bounds (camera, "FPS", min, max, error);
			break;
		case ARV_CAMERA_VENDOR_PROSILICA:
			arv_camera_get_float_bounds (camera, "AcquisitionFrameRateAbs", min, max, error);
			break;
		case ARV_CAMERA_VENDOR_POINT_GREY_FLIR:
		case ARV_CAMERA_VENDOR_DALSA:
		case ARV_CAMERA_VENDOR_RICOH:
		case ARV_CAMERA_VENDOR_BASLER:
	        case ARV_CAMERA_VENDOR_XIMEA:
		case ARV_CAMERA_VENDOR_MATRIX_VISION:
		case ARV_CAMERA_VENDOR_IMPERX:
		case ARV_CAMERA_VENDOR_UNKNOWN:
			arv_camera_get_float_bounds (camera,
						     priv->has_acquisition_frame_rate ?
						     "AcquisitionFrameRate":
						     "AcquisitionFrameRateAbs",
						     min, max, error);
			break;
	}
}

/*
* arv_camera_set_frame_rate_enable:
* @camera: an #ArvCamera
* @enable: true to enable, false to disable
* @error: a #GError placeholer, %NULL to ignore
*
* Configures whether to enable the upper frame rate limit set by #arv_camera_set_frame_rate.
* Implements vendor specific quirks if needed.
* Since: 0.8.26
*/
void
arv_camera_set_frame_rate_enable(ArvCamera *camera, gboolean enable, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *local_error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));

  switch (priv->vendor) {
		case ARV_CAMERA_VENDOR_BASLER:
			if (local_error == NULL){
				if (arv_camera_is_feature_available (camera, "AcquisitionFrameRateEnable", &local_error)){
					/* enable is optional on some devices */
					if (local_error == NULL)
						arv_camera_set_boolean (camera, "AcquisitionFrameRateEnable", enable, &local_error);
				}
			}
			break;
		case ARV_CAMERA_VENDOR_POINT_GREY_FLIR:
			if (local_error == NULL) {
				if (priv->has_acquisition_frame_rate_enabled)
					arv_camera_set_boolean (camera, "AcquisitionFrameRateEnabled", enable, &local_error);
				else
					arv_camera_set_boolean (camera, "AcquisitionFrameRateEnable", enable, &local_error);
			}
			break;
		case ARV_CAMERA_VENDOR_DALSA:
		case ARV_CAMERA_VENDOR_RICOH:
		case ARV_CAMERA_VENDOR_XIMEA:
		case ARV_CAMERA_VENDOR_MATRIX_VISION:
		case ARV_CAMERA_VENDOR_IMPERX:
		case ARV_CAMERA_VENDOR_UNKNOWN:
			if (local_error == NULL) {
				if (arv_camera_is_feature_available (camera, "AcquisitionFrameRateEnable", &local_error)) {
					if (local_error == NULL)
						arv_camera_set_boolean (camera, "AcquisitionFrameRateEnable", TRUE, &local_error);
					}
			}
			break;
		case ARV_CAMERA_VENDOR_PROSILICA:
		case ARV_CAMERA_VENDOR_TIS:
		default:
			break; /* No specific frame rate enable code */
	}

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
	}
}

/**
 * arv_camera_set_trigger:
 * @camera: a #ArvCamera
 * @source: trigger source as string
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Configures the camera in trigger mode. Typical values for source are "Line1" or "Line2". See the camera documentation
 * for the allowed values.  Source can also be "Software". In this case, an acquisition is triggered by a call to
 * arv_camera_software_trigger().
 *
 * The trigger set is "FrameStart". "AcquisitionStart" is used as a fallback if "FrameStart" is not present.
 * All other triggers are disabled. "TriggerActivation" is set to rising edge.
 *
 * For an advanced trigger configuration, use the underlying #ArvDevice object returned by arv_camera_get_device().
 *
 * Since: 0.8.0
 */

void
arv_camera_set_trigger (ArvCamera *camera, const char *source, GError **error)
{
	GError *local_error = NULL;
	gboolean has_frame_start = FALSE;
        gboolean has_frame_burst_start = FALSE; /* Hikrobot, Basler devices */
	gboolean has_acquisition_start = FALSE; /* Smartek devices */
        gboolean has_trigger_selector = FALSE;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (source != NULL);

	if (arv_camera_is_feature_available (camera, "AcquisitionFrameRateEnable", NULL))
                arv_camera_set_boolean (camera, "AcquisitionFrameRateEnable", FALSE, &local_error);

        has_trigger_selector = arv_camera_is_feature_available(camera, "TriggerSelector", &local_error);

        if (has_trigger_selector == TRUE) {
                const char **triggers = NULL;
                guint n_triggers = 0;
                unsigned int i;

                triggers = arv_camera_dup_available_enumerations_as_strings (camera, "TriggerSelector", &n_triggers,
                                                                        &local_error);

                for (i = 0; i < n_triggers && local_error == NULL; i++) {
                        arv_camera_set_string (camera, "TriggerSelector", triggers[i], &local_error);
                        if (local_error == NULL) {
                                if (g_strcmp0 (triggers[i], "FrameStart") == 0)
                                        has_frame_start = TRUE;
                                else if (g_strcmp0 (triggers[i], "FrameBurstStart") == 0)
                                        has_frame_burst_start = TRUE;
                                else if (g_strcmp0 (triggers[i], "AcquisitionStart") == 0)
                                        has_acquisition_start = TRUE;
                                arv_camera_set_string (camera, "TriggerMode", "Off", &local_error);
                        }
                }

                g_free (triggers);
        }

        if (local_error == NULL) {
                if (has_trigger_selector == TRUE) {
                        if (has_frame_start) {
                                arv_camera_set_string (camera, "TriggerSelector", "FrameStart", &local_error);
                        } else if (has_frame_burst_start) {
                                arv_camera_set_string (camera, "TriggerSelector", "FrameBurstStart", &local_error);
                        } else if (has_acquisition_start) {
                                arv_camera_set_string (camera, "TriggerSelector", "AcquisitionStart", &local_error);
                        } else {
                                local_error = g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_FEATURE_NOT_FOUND,
                                                        "<FrameStart> or <AcquisisitonStart> feature missing "
                                                        "for trigger setting");
                        }
                }
                if (local_error == NULL)
                        arv_camera_set_string (camera, "TriggerMode", "On", &local_error);

                if (local_error == NULL &&
                    arv_camera_is_enumeration_entry_available (camera, "TriggerActivation",
                                                               "RisingEdge", NULL))
                        arv_camera_set_string (camera, "TriggerActivation", "RisingEdge", &local_error);

                if (local_error == NULL)
                        arv_camera_set_string (camera, "TriggerSource", source, &local_error);
        }

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

/**
 * arv_camera_set_trigger_source:
 * @camera: a #ArvCamera
 * @source: source name
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Sets the trigger source. This function doesn't check if the camera is configured
 * to actually use this source as a trigger.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_trigger_source (ArvCamera *camera, const char *source, GError **error)
{
	arv_camera_set_string (camera, "TriggerSource", source, error);
}

/**
 * arv_camera_get_trigger_source:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Gets the trigger source. This function doesn't check if the camera is configured
 * to actually use this source as a trigger.
 *
 * Returns: a string containing the trigger source name, NULL on error.
 *
 * Since: 0.8.0
 */

const char *
arv_camera_get_trigger_source (ArvCamera *camera, GError **error)
{
	return arv_camera_get_string (camera, "TriggerSource", error);
}

/**
 * arv_camera_dup_available_trigger_sources:
 * @camera: a #ArvCamera
 * @n_sources: (out): number of sources
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Gets the list of all available trigger sources.
 *
 * Returns: (array length=n_sources) (transfer container): a newly allocated array of strings, which must be freed using g_free().
 *
 * Since: 0.8.0
 */

const char **
arv_camera_dup_available_trigger_sources (ArvCamera *camera, guint *n_sources, GError **error)
{
	return arv_camera_dup_available_enumerations_as_strings (camera, "TriggerSource", n_sources, error);
}

/**
 * arv_camera_dup_available_triggers:
 * @camera: a #ArvCamera
 * @n_triggers: (out): number of available triggers
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Gets a list of all available triggers: FrameStart, ExposureActive, etc...
 *
 * Returns: (array length=n_triggers) (transfer container): a newly allocated array of strings, which must be freed using g_free().
 *
 * Since: 0.8.0
 */

const char **
arv_camera_dup_available_triggers (ArvCamera *camera, guint *n_triggers, GError **error)
{
	return arv_camera_dup_available_enumerations_as_strings (camera, "TriggerSelector", n_triggers, error);
}

/**
 * arv_camera_clear_triggers:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Disables all triggers.
 *
 * Since: 0.8.0
 */

void
arv_camera_clear_triggers (ArvCamera* camera, GError **error)
{
	GError *local_error = NULL;

        g_return_if_fail (ARV_IS_CAMERA (camera));

        if (arv_camera_is_feature_available(camera, "TriggerSelector", &local_error)) {
                const char **triggers;
                guint n_triggers;
                unsigned i;
                triggers = arv_camera_dup_available_triggers (camera, &n_triggers, &local_error);
                for (i = 0; i < n_triggers && local_error == NULL; i++) {
                        arv_camera_set_string (camera, "TriggerSelector", triggers[i], &local_error);
                        if (local_error == NULL)
                                arv_camera_set_string (camera, "TriggerMode", "Off", &local_error);
                }

                g_free (triggers);
        } else
                arv_camera_set_string (camera, "TriggerMode", "Off", &local_error);

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

/**
 * arv_camera_is_software_trigger_supported:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: %TRUE if software trigger is supported by @camera.
 *
 * Since: 0.8.17
 */

gboolean
arv_camera_is_software_trigger_supported (ArvCamera *camera, GError **error)
{
        return arv_camera_is_feature_implemented (camera, "TriggerSoftware", error);
}

/**
 * arv_camera_software_trigger:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Sends a software trigger command to @camera. The camera must be previously
 * configured to use a software trigger, using @arv_camera_set_trigger().
 *
 * Since: 0.8.0
 */

void
arv_camera_software_trigger (ArvCamera *camera, GError **error)
{
	arv_camera_execute_command (camera, "TriggerSoftware", error);
}

/**
 * arv_camera_set_exposure_time:
 * @camera: a #ArvCamera
 * @exposure_time_us: exposure time, in µs
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Sets exposure time. User should take care to set a value compatible with
 * the desired frame rate. Negative @exposure_time_us is ignored.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_exposure_time (ArvCamera *camera, double exposure_time_us, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *local_error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (exposure_time_us <= 0)
		return;

	switch (priv->series) {
		case ARV_CAMERA_SERIES_BASLER_SCOUT:
			arv_camera_set_float (camera, "ExposureTimeBaseAbs", exposure_time_us, &local_error);
			if (local_error == NULL)
				arv_camera_set_integer (camera, "ExposureTimeRaw", 1, &local_error);
			break;
		case ARV_CAMERA_SERIES_RICOH:
			arv_camera_set_integer (camera, "ExposureTimeRaw", exposure_time_us, &local_error);
			break;
		case ARV_CAMERA_SERIES_XIMEA:
			arv_camera_set_integer (camera, "ExposureTime", exposure_time_us, &local_error);
			break;
		case ARV_CAMERA_SERIES_IMPERX_CHEETAH:
		case ARV_CAMERA_SERIES_MATRIX_VISION:
			arv_camera_set_string (camera, "ExposureMode", "Timed", &local_error);
			if (local_error == NULL)
				arv_camera_set_float (camera, "ExposureTime", exposure_time_us, &local_error);
			break;
		case ARV_CAMERA_SERIES_BASLER_ACE:
		default:
			arv_camera_set_float (camera,
					      priv->has_exposure_time ?
					      "ExposureTime" :
					      "ExposureTimeAbs", exposure_time_us, &local_error);
			break;
	}

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

/**
 * arv_camera_get_exposure_time:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: current exposure time, in µs.
 *
 * Since: 0.8.0
 */

double
arv_camera_get_exposure_time (ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0.0);

	switch (priv->series) {
		case ARV_CAMERA_SERIES_XIMEA:
			return arv_camera_get_integer (camera,"ExposureTime", error);
		case ARV_CAMERA_SERIES_RICOH:
			return arv_camera_get_integer (camera,"ExposureTimeRaw", error);
		default:
			return arv_camera_get_float (camera,
						     priv->has_exposure_time ?
						     "ExposureTime" :
						     "ExposureTimeAbs", error);
	}
}

/**
 * arv_camera_get_exposure_time_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum exposure time
 * @max: (out): maximum exposure time
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves exposure time bounds, in µs.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_exposure_time_bounds (ArvCamera *camera, double *min, double *max, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	switch (priv->series) {
		case ARV_CAMERA_SERIES_BASLER_SCOUT:
			arv_camera_get_float_bounds (camera,
						     priv->has_exposure_time ?
						     "ExposureTime" :
						     "ExposureTimeBaseAbs",
						     min, max, error);
			break;
		case ARV_CAMERA_SERIES_BASLER_ACE:
			if (priv->has_exposure_time)
				arv_camera_get_float_bounds (camera, "ExposureTime", min, max, error);
			else
				arv_camera_get_integer_bounds_as_double (camera, "ExposureTimeRaw", min, max, error);
			break;
		case ARV_CAMERA_SERIES_XIMEA:
			arv_camera_get_integer_bounds_as_double (camera, "ExposureTime", min, max, error);
			break;
		case ARV_CAMERA_SERIES_RICOH:
			arv_camera_get_integer_bounds_as_double (camera, "ExposureTimeRaw", min, max, error);
			break;
		default:
			arv_camera_get_float_bounds (camera,
						     priv->has_exposure_time ?
						     "ExposureTime" :
						     "ExposureTimeAbs",
						     min, max, error);
			break;
	}
}

/**
 * arv_camera_set_exposure_time_auto:
 * @camera: a #ArvCamera
 * @auto_mode: auto exposure mode selection
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Configures automatic exposure feature.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_exposure_time_auto (ArvCamera *camera, ArvAuto auto_mode, GError **error)
{
	arv_camera_set_string (camera, "ExposureAuto", arv_auto_to_string (auto_mode), error);
}

/**
 * arv_camera_get_exposure_time_auto:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: auto exposure mode selection
 *
 * Since: 0.8.0
 */

ArvAuto
arv_camera_get_exposure_time_auto (ArvCamera *camera, GError **error)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), ARV_AUTO_OFF);

	return arv_auto_from_string (arv_camera_get_string (camera, "ExposureAuto", error));
}

/*
 * arv_camera_set_exposure_mode:
 * @camera: a #ArvCamera
 * @exspoure_mode: exposure mode
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Defines acquisition mode.
 *
 * Since: 0.8.7
 */

void
arv_camera_set_exposure_mode (ArvCamera *camera, ArvExposureMode acquisition_mode, GError **error)
{
	arv_camera_set_string (camera, "ExposureMode", arv_exposure_mode_to_string (acquisition_mode), error);
}

/* Analog control */

/**
 * arv_camera_set_gain:
 * @camera: a #ArvCamera
 * @gain: gain value
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Sets the gain of the ADC converter. Negative @gain is ignored.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_gain (ArvCamera *camera, double gain, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (gain < 0)
		return;

	if (priv->has_gain)
		arv_camera_set_float (camera, "Gain", gain, error);
	else {
		if (priv->gain_raw_as_float)
			arv_camera_set_float (camera, "GainRaw", gain, error);
		else if (priv->gain_abs_as_float)
			arv_camera_set_float (camera, "GainAbs", gain, error);
		else
			arv_camera_set_integer (camera, "GainRaw", gain, error);
	}
}

/**
 * arv_camera_get_gain:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the current gain setting.
 *
 * Since: 0.8.0
 */

double
arv_camera_get_gain (ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0.0);

	if (priv->has_gain)
		return arv_camera_get_float (camera, "Gain", error);
	else if (priv->gain_raw_as_float)
		return arv_camera_get_float (camera, "GainRaw", error);
	else if (priv->gain_abs_as_float)
		return arv_camera_get_float (camera, "GainAbs", error);

	return arv_camera_get_integer (camera, "GainRaw", error);
}

/**
 * arv_camera_get_gain_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum gain
 * @max: (out): maximum gain
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves gain bounds.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_gain_bounds (ArvCamera *camera, double *min, double *max, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (priv->has_gain) {
		arv_camera_get_float_bounds (camera, "Gain", min, max, error);
        return;
    } else if (priv->gain_abs_as_float) {
        arv_camera_get_float_bounds (camera, "GainAbs", min, max, error);
		return;
	} else if (priv->gain_raw_as_float) {
		arv_camera_get_float_bounds (camera, "GainRaw", min, max, error);
		return;
	}

	arv_camera_get_integer_bounds_as_double (camera, "GainRaw", min, max, error);

	return;
}

/**
 * arv_camera_set_gain_auto:
 * @camera: a #ArvCamera
 * @auto_mode: auto gain mode selection
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Configures automatic gain feature.
 *
 * Since: 0.8.0
 **/

void
arv_camera_set_gain_auto (ArvCamera *camera, ArvAuto auto_mode, GError **error)
{
	arv_camera_set_string (camera, "GainAuto", arv_auto_to_string (auto_mode), error);
}

/**
 * arv_camera_get_gain_auto:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: auto gain mode selection
 *
 * Since: 0.8.0
 **/

ArvAuto
arv_camera_get_gain_auto (ArvCamera *camera, GError **error)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), ARV_AUTO_OFF);

	return arv_auto_from_string (arv_camera_get_string (camera, "GainAuto", error));
}

/**
 * arv_camera_select_gain:
 * @camera: a #ArvCamera
 * @selector: gain selector
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Configures Gain Selector feature.
 *
 * Since: 0.8.27
 **/

void
arv_camera_select_gain (ArvCamera *camera, const char *selector, GError **error)
{
	arv_camera_set_string (camera, "GainSelector", selector, error);
}

/**
 * arv_camera_dup_available_gains:
 * @camera: a #ArvCamera
 * @n_selectors: (out): number of different gain selectors
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the list of all available gain selectors as strings.
 *
 * Returns: (array length=n_selectors) (transfer container): a newly allocated array of strings, to be freed after use with
 * g_free().
 *
 * Since: 0.8.27
 */

const char **
arv_camera_dup_available_gains (ArvCamera *camera, guint *n_selectors, GError **error)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_camera_dup_available_enumerations_as_strings (camera, "GainSelector", n_selectors, error);
}

/**
 * arv_camera_is_black_level_available:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: %TRUE if BlackLevel feature is available.
 *
 * Since: 0.8.19
 */

gboolean
arv_camera_is_black_level_available (ArvCamera *camera, GError **error) {
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	if (priv->has_brightness)
		return arv_camera_is_feature_available (camera, "Brightness", error);
	else if (priv->has_black_level_raw)
		return arv_camera_is_feature_available (camera, "BlackLevelRaw", error);
	else if (priv->has_black_level)
		return arv_camera_is_feature_available (camera, "BlackLevel", error);

        return FALSE;
}

/**
 * arv_camera_set_black_level:
 * @camera: a #ArvCamera
 * @blacklevel: blacklevel value
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Since: 0.8.19
 */

void
arv_camera_set_black_level (ArvCamera *camera, double blacklevel, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (priv->has_brightness)
		arv_camera_set_float (camera, "Brightness", blacklevel, error);
	else if (priv->has_black_level_raw)
		arv_camera_set_integer (camera, "BlackLevelRaw", blacklevel, error);
	else
		arv_camera_set_float (camera, "BlackLevel", blacklevel, error);
}

/**
 * arv_camera_get_black_level:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the current blacklevel setting.
 *
 * Since: 0.8.19
 */

double
arv_camera_get_black_level (ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0.0);

	if (priv->has_brightness)
		return arv_camera_get_float (camera, "Brightness", error);
	else if (priv->has_black_level_raw)
		return arv_camera_get_integer (camera, "BlackLevelRaw", error);
	else
		return arv_camera_get_float (camera, "BlackLevel", error);
}

/**
 * arv_camera_select_black_level:
 * @camera: a #ArvCamera
 * @selector: black level selection
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Configures Black Level Selector feature.
 *
 * Since: 0.8.27
 **/

void
arv_camera_select_black_level (ArvCamera *camera, const char *selector, GError **error)
{
	arv_camera_set_string (camera, "BlackLevelSelector", selector, error);
}

/**
 * arv_camera_dup_available_black_levels:
 * @camera: a #ArvCamera
 * @n_selectors: (out): number of different black level selectors
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the list of all available black level selectors as strings.
 *
 * Returns: (array length=n_selectors) (transfer container): a newly allocated array of strings, to be freed after use with
 * g_free().
 *
 * Since: 0.8.27
 */

const char **
arv_camera_dup_available_black_levels (ArvCamera *camera, guint *n_selectors, GError **error)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_camera_dup_available_enumerations_as_strings (camera, "BlackLevelSelector", n_selectors, error);
}

/**
 * arv_camera_get_black_level_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum blacklevel
 * @max: (out): maximum blacklevel
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves blacklevel bounds.
 *
 * Since: 0.8.19
 */

void
arv_camera_get_black_level_bounds (ArvCamera *camera, double *min, double *max, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (priv->has_brightness)
		arv_camera_get_float_bounds (camera, "Brightness", min, max, error);
	else if (priv->has_black_level_raw)
		arv_camera_get_integer_bounds_as_double (camera, "BlackLevelRaw", min, max, error);
	else
		arv_camera_get_float_bounds (camera, "BlackLevel", min, max, error);
}

/**
 * arv_camera_set_black_level_auto:
 * @camera: a #ArvCamera
 * @auto_mode: auto black_level mode selection
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Configures automatic black level feature.
 *
 * Since: 0.8.19
 **/

void
arv_camera_set_black_level_auto (ArvCamera *camera, ArvAuto auto_mode, GError **error)
{
	arv_camera_set_string (camera, "BlackLevelAuto", arv_auto_to_string (auto_mode), error);
}

/**
 * arv_camera_get_black_level_auto:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: auto black level mode selection
 *
 * Since: 0.8.19
 **/

ArvAuto
arv_camera_get_black_level_auto (ArvCamera *camera, GError **error)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), ARV_AUTO_OFF);

	return arv_auto_from_string (arv_camera_get_string (camera, "BlackLevelAuto", error));
}

/* Component control */

/**
 * arv_camera_is_component_available:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ingore
 *
 * Returns: %TRUE if Component features are available.
 *
 * Since: 0.8.25
 */

gboolean
arv_camera_is_component_available (ArvCamera *camera, GError **error)
{
        return arv_camera_is_feature_available (camera, "ComponentSelector", error);
}

/**
 * arv_camera_dup_available_components:
 * @camera: a #ArvCamera
 * @n_components: (out) (optional) : number of available components
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the list of available components.
 *
 * Returns: (array length=n_components) (transfer container): a newly allocated array of strings, to be freed after use with
 * g_free().
 *
 * Since: 0.8.23
 */

const char **
arv_camera_dup_available_components (ArvCamera *camera, guint *n_components, GError **error)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_camera_dup_available_enumerations_as_strings (camera, "ComponentSelector", n_components, error);
}

/**
 * arv_camera_select_component:
 * @camera: a #ArvCamera
 * @component: component to select
 * @flags: a #ArvComponentSelectionFlags
 * @component_id: (out) (optional): a placeholder for the component id
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Select and enable or disable the given @component.
 *
 * Returns: %TRUE if the component is enabled
 *
 * Since: 0.8.25
 */

gboolean
arv_camera_select_component (ArvCamera *camera, const char *component,
                             ArvComponentSelectionFlags flags, guint *component_id,
                             GError **error)
{
        GError *local_error = NULL;
        gboolean is_enabled = !(flags == ARV_COMPONENT_SELECTION_FLAGS_DISABLE);

        g_return_val_if_fail (ARV_IS_CAMERA(camera), FALSE);

        if (flags == ARV_COMPONENT_SELECTION_FLAGS_EXCLUSIVE_ENABLE ||
            flags == ARV_COMPONENT_SELECTION_FLAGS_ENABLE_ALL) {
                guint n_components, i;
                const char **components = arv_camera_dup_available_components(camera, &n_components, &local_error);

                for (i = 0; i < n_components && local_error == NULL; i++) {
                        arv_camera_set_string (camera, "ComponentSelector", components[i], &local_error);
                        if (local_error == NULL)
                                arv_camera_set_boolean(camera, "ComponentEnable",
                                                       flags == ARV_COMPONENT_SELECTION_FLAGS_ENABLE_ALL,
                                                       &local_error);
                }
        }

        if (local_error == NULL) arv_camera_set_string (camera, "ComponentSelector", component, &local_error);
        if (local_error == NULL &&
            flags != ARV_COMPONENT_SELECTION_FLAGS_NONE &&
            flags != ARV_COMPONENT_SELECTION_FLAGS_ENABLE_ALL)
                arv_camera_set_boolean(camera, "ComponentEnable",
                                       flags == ARV_COMPONENT_SELECTION_FLAGS_DISABLE ? FALSE: TRUE,
                                       &local_error);

        if (component_id != NULL && local_error == NULL)
                *component_id = arv_camera_get_integer(camera, "ComponentIDValue", &local_error);

        if (local_error == NULL && flags == ARV_COMPONENT_SELECTION_FLAGS_NONE)
                is_enabled = arv_camera_get_boolean(camera, "ComponentEnable", &local_error);

        if (local_error != NULL) {
                g_propagate_error(error, local_error);
                return FALSE;
        }

        return is_enabled;
}

/**
 * arv_camera_select_and_enable_component:
 * @camera: a #ArvCamera
 * @component: component to select
 * @disable_others: %TRUE to disable all the other components
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Select and enable the given component.
 *
 * Since: 0.8.23
 */

void
arv_camera_select_and_enable_component (ArvCamera *camera, const char *component, gboolean disable_others, GError **error)
{
        arv_camera_select_component(camera, component,
                                    disable_others ?
                                    ARV_COMPONENT_SELECTION_FLAGS_EXCLUSIVE_ENABLE :
                                    ARV_COMPONENT_SELECTION_FLAGS_ENABLE,
                                    NULL, error);
}

/* Transport layer control */

/**
 * arv_camera_get_payload:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves the size needed for the storage of an image. This value is used
 * for the creation of the stream buffers.
 *
 * Returns: frame storage size, in bytes.
 *
 * Since: 0.8.0
 */

guint
arv_camera_get_payload (ArvCamera *camera, GError **error)
{
	return arv_camera_get_integer (camera, "PayloadSize", error);
}

/**
 * arv_camera_get_device:
 * @camera: a #ArvCamera
 *
 * Retrieves the #ArvDevice object for more complete access to camera features.
 *
 * Returns: (transfer none): underlying device object.
 *
 * Since: 0.2.0
 */

ArvDevice *
arv_camera_get_device (ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return priv->device;
}

/**
 * arv_camera_is_frame_rate_available:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ingore
 *
 * Returns: %TRUE if FrameRate feature is available
 *
 * Since: 0.8.0
 */

gboolean
arv_camera_is_frame_rate_available (ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	switch (priv->vendor) {
		case ARV_CAMERA_VENDOR_PROSILICA:
			return arv_camera_is_feature_available (camera, "AcquisitionFrameRateAbs", error);
		case ARV_CAMERA_VENDOR_TIS:
			return arv_camera_is_feature_available (camera, "FPS", error);
		case ARV_CAMERA_VENDOR_POINT_GREY_FLIR:
		case ARV_CAMERA_VENDOR_DALSA:
		case ARV_CAMERA_VENDOR_RICOH:
		case ARV_CAMERA_VENDOR_BASLER:
	        case ARV_CAMERA_VENDOR_XIMEA:
		case ARV_CAMERA_VENDOR_MATRIX_VISION:
		case ARV_CAMERA_VENDOR_IMPERX:
		case ARV_CAMERA_VENDOR_UNKNOWN:
			return arv_camera_is_feature_available (camera,
								priv->has_acquisition_frame_rate ?
								"AcquisitionFrameRate": "AcquisitionFrameRateAbs",
								error);
	}

	return FALSE;
}
/**
 * arv_camera_is_exposure_time_available:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ingore
 *
 * Returns: %TRUE if Exposure Time feature is available.
 *
 * Since: 0.8.0
 */

gboolean
arv_camera_is_exposure_time_available (ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	switch (priv->series) {
		case ARV_CAMERA_SERIES_XIMEA:
			return arv_camera_is_feature_available (camera, "ExposureTime", error);
		case ARV_CAMERA_SERIES_RICOH:
			return arv_camera_is_feature_available (camera, "ExposureTimeRaw", error);
		case ARV_CAMERA_SERIES_IMPERX_CHEETAH:
			return arv_camera_is_feature_available (camera, "ExposureMode", error);
		default:
			return arv_camera_is_feature_available (camera,
								priv->has_exposure_time ?  "ExposureTime" : "ExposureTimeAbs",
								error);
	}
}

/**
 * arv_camera_is_exposure_auto_available:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ingore
 *
 * Returns: %TRUE if Exposure Auto feature is available.
 *
 * Since: 0.8.0
 */

gboolean
arv_camera_is_exposure_auto_available (ArvCamera *camera, GError **error)
{
	return arv_camera_is_feature_available (camera, "ExposureAuto", error);
}

/**
 * arv_camera_is_gain_available:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ingore
 *
 * Returns: %TRUE if Gain feature is available.
 *
 * Since: 0.8.0
 */

gboolean
arv_camera_is_gain_available (ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	if (priv->has_gain)
		return arv_camera_is_feature_available (camera, "Gain", error);

	if (priv->gain_raw_as_float)
		return arv_camera_is_feature_available (camera, "GainRaw", error);
	if (priv->gain_abs_as_float)
		return arv_camera_is_feature_available (camera, "GainAbs", error);

	return arv_camera_is_feature_available (camera, "GainRaw", error);
}

/**
 * arv_camera_is_gain_auto_available:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ingore
 *
 * Returns: %TRUE if auto gain feature is available.
 *
 * Since: 0.8.0
 */

gboolean
arv_camera_is_gain_auto_available (ArvCamera *camera, GError **error)
{
	return arv_camera_is_feature_available (camera, "GainAuto", error);
}

/**
 * arv_camera_is_black_level_auto_available:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ingore
 *
 * Returns: %TRUE if auto black level feature is available.
 *
 * Since: 0.8.19
 */

gboolean
arv_camera_is_black_level_auto_available (ArvCamera *camera, GError **error)
{
	return arv_camera_is_feature_available (camera, "BlackLevelAuto", error);
}

/**
 * arv_camera_is_binning_available:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ingore
 *
 * Returns: %TRUE if Binning feature is available.
 *
 * Since: 0.8.0
 */

gboolean
arv_camera_is_binning_available (ArvCamera *camera, GError **error)
{
	GError *local_error = NULL;
	gboolean horizontal = FALSE;
	gboolean vertical = FALSE;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	horizontal = arv_camera_is_feature_available (camera, "BinningHorizontal", &local_error);
	if (local_error == NULL)
		vertical = arv_camera_is_feature_available (camera, "BinningVertical", &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return FALSE;
	}

	return horizontal && vertical;
}

/**
 * arv_camera_execute_command:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @error: a #GError placeholder, %NULL to ingore
 *
 * Execute a Genicam command.
 *
 * Since: 0.8.0
 */

void
arv_camera_execute_command (ArvCamera *camera, const char *feature, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_execute_command (priv->device, feature, error);
}

/**
 * arv_camera_set_boolean:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @value: new feature value
 * @error: a #GError placeholder, %NULL to ingore
 *
 * Set a boolean feature value.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_boolean (ArvCamera *camera, const char *feature, gboolean value, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_boolean_feature_value (priv->device, feature, value, error);
}

/**
 * arv_camera_get_boolean:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the boolean feature value, %FALSE on error.
 *
 * Since: 0.8.0
 */

gboolean
arv_camera_get_boolean (ArvCamera *camera, const char *feature, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	return arv_device_get_boolean_feature_value (priv->device, feature, error);
}

/**
 * arv_camera_get_boolean_gi: (rename-to arv_camera_get_boolean)
 * @camera: a #ArvCamera
 * @feature: feature name
 * @value: (out): output value
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Since: 0.8.0
 */

void
arv_camera_get_boolean_gi (ArvCamera *camera, const char *feature, gboolean *value, GError **error)
{
	g_return_if_fail (value != NULL);

	*value = arv_camera_get_boolean (camera, feature, error);
}

/**
 * arv_camera_set_string:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @value: new feature value
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Set an string feature value.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_string (ArvCamera *camera, const char *feature, const char *value, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_string_feature_value (priv->device, feature, value, error);
}

/**
 * arv_camera_get_string:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the string feature value, %NULL on error.
 *
 * Since: 0.8.0
 */

const char *
arv_camera_get_string (ArvCamera *camera, const char *feature, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	return arv_device_get_string_feature_value (priv->device, feature, error);
}

/**
 * arv_camera_set_integer:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @value: new feature value
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Set an integer feature value.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_integer (ArvCamera *camera, const char *feature, gint64 value, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_integer_feature_value (priv->device, feature, value, error);
}

/**
 * arv_camera_get_integer:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the integer feature value, 0 on error.
 *
 * Since: 0.8.0
 */

gint64
arv_camera_get_integer (ArvCamera *camera, const char *feature, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0);

	return arv_device_get_integer_feature_value (priv->device, feature, error);
}

/**
 * arv_camera_get_integer_bounds:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @min: (out): minimum feature value
 * @max: (out): maximum feature value
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves integer feature bounds.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_integer_bounds (ArvCamera *camera, const char *feature, gint64 *min, gint64 *max, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	if (min != NULL)
		*min = G_MININT64;
	if (max != NULL)
		*max = G_MAXINT64;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_get_integer_feature_bounds (priv->device, feature, min, max, error);
}

/**
 * arv_camera_get_integer_increment:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: @feature value increment, or 1 on error.
 *
 * Since: 0.8.0
 */

gint64
arv_camera_get_integer_increment (ArvCamera *camera, const char *feature, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 1);
	g_return_val_if_fail (feature != NULL, 1);

	return arv_device_get_integer_feature_increment (priv->device, feature, error);
}

static void
arv_camera_get_integer_bounds_as_gint (ArvCamera *camera, const char *feature, gint *min, gint *max, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *local_error = NULL;
	gint64 min64, max64;

	if (min != NULL)
		*min = G_MININT;
	if (max != NULL)
		*max = G_MAXINT;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (feature != NULL);

	arv_device_get_integer_feature_bounds (priv->device, feature, &min64, &max64, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
	} else {
		if (min != NULL)
			*min = CLAMP (min64, G_MININT, G_MAXINT);
		if (max != NULL)
			*max = CLAMP (max64, G_MININT, G_MAXINT);
	}
}

static void
arv_camera_get_integer_bounds_as_guint (ArvCamera *camera, const char *feature, guint *min, guint *max, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *local_error = NULL;
	gint64 min64, max64;

	if (min != NULL)
		*min = 0;
	if (max != NULL)
		*max = G_MAXUINT;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (feature != NULL);

	arv_device_get_integer_feature_bounds (priv->device, feature, &min64, &max64, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
	} else {
		if (min != NULL)
			*min = CLAMP (min64, 0, G_MAXUINT);
		if (max != NULL)
			*max = CLAMP (max64, 0, G_MAXUINT);
	}
}

static void
arv_camera_get_integer_bounds_as_double (ArvCamera *camera, const char *feature, double *min, double *max, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *local_error = NULL;
	gint64 min64, max64;

	if (min != NULL)
		*min = -G_MAXDOUBLE;
	if (max != NULL)
		*max = G_MAXDOUBLE;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (feature != NULL);

	arv_device_get_integer_feature_bounds (priv->device, feature, &min64, &max64, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
	} else {
		if (min != NULL)
			*min = min64;
		if (max != NULL)
			*max = max64;
	}
}

/**
 * arv_camera_set_float:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @value: new feature value
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Set a float feature value.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_float (ArvCamera *camera, const char *feature, double value, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_float_feature_value (priv->device, feature, value, error);
}

/**
 * arv_camera_get_float:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the float feature value, 0.0 on error.
 *
 * Since: 0.8.0
 */

double
arv_camera_get_float (ArvCamera *camera, const char *feature, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0.0);

	return arv_device_get_float_feature_value (priv->device, feature, error);
}

/**
 * arv_camera_get_float_bounds:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @min: (out): minimum feature value
 * @max: (out): maximum feature value
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Retrieves float feature bounds.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_float_bounds (ArvCamera *camera, const char *feature, double *min, double *max, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	if (min != NULL)
		*min = -G_MAXDOUBLE;
	if (max != NULL)
		*max = G_MAXDOUBLE;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_get_float_feature_bounds (priv->device, feature, min, max, error);
}

/**
 * arv_camera_get_float_increment:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: @feature value increment, or #G_MINDOUBLE on error.
 *
 * Since: 0.8.16
 */

double
arv_camera_get_float_increment (ArvCamera *camera, const char *feature, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 1);
	g_return_val_if_fail (feature != NULL, 1);

	return arv_device_get_float_feature_increment (priv->device, feature, error);
}

/**
 * arv_camera_dup_available_enumerations:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @n_values: placeholder for the number of returned values
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Get all the available values of @feature, as 64 bit integers.
 *
 * Returns: (array length=n_values) (transfer container): a newly created array of integers, which must freed after use using g_free, or
 * NULL on error.
 *
 * Since: 0.8.0
 */

gint64 *
arv_camera_dup_available_enumerations (ArvCamera *camera, const char *feature, guint *n_values, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	if (n_values != NULL)
		*n_values = 0;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_dup_available_enumeration_feature_values (priv->device, feature, n_values, error);
}

/**
 * arv_camera_dup_available_enumerations_as_strings:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @n_values: placeholder for the number of returned values
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Get all the available values of @feature, as strings.
 *
 * Returns: (array length=n_values) (transfer container): a newly created array of const strings, which must freed after use using g_free,
 * or %NULL on error.
 *
 * Since: 0.8.0
 */

const char **
arv_camera_dup_available_enumerations_as_strings (ArvCamera *camera, const char *feature, guint *n_values, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	if (n_values != NULL)
		*n_values = 0;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_dup_available_enumeration_feature_values_as_strings (priv->device, feature, n_values, error);
}

/**
 * arv_camera_dup_available_enumerations_as_display_names:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @n_values: placeholder for the number of returned values
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Get display names of all the available entries of @feature.
 *
 * Returns: (array length=n_values) (transfer container): a newly created array of const strings, to be freed after use using g_free, or
 * %NULL on error.
 *
 * Since: 0.8.0
 */

const char **
arv_camera_dup_available_enumerations_as_display_names (ArvCamera *camera, const char *feature, guint *n_values, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	if (n_values != NULL)
		*n_values = 0;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_dup_available_enumeration_feature_values_as_display_names (priv->device, feature, n_values, error);
}

/**
 * arv_camera_is_enumeration_entry_available:
 * @camera: a #ArvCamera
 * @feature: enumeration feature name
 * @entry: entry name
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: %TRUE if the feature and the feature entry are available
 *
 * Since: 0.8.17
 */

gboolean
arv_camera_is_enumeration_entry_available (ArvCamera *camera, const char *feature, const char *entry, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

        g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

        return arv_device_is_enumeration_entry_available (priv->device, feature, entry, error);
}

/**
 * arv_camera_is_feature_available:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Return: %TRUE if feature is available, %FALSE if not or on error.
 *
 * Since: 0.8.0
 */

gboolean
arv_camera_is_feature_available (ArvCamera *camera, const char *feature, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	return arv_device_is_feature_available (priv->device, feature, error);
}

/**
 * arv_camera_is_feature_implemented:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Return: %TRUE if feature is implemented, %FALSE if not or on error.
 *
 * Since: 0.8.23
 */

gboolean
arv_camera_is_feature_implemented (ArvCamera *camera, const char *feature, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	return arv_device_is_feature_implemented (priv->device, feature, error);
}

/**
 * arv_camera_set_register_cache_policy:
 * @camera: a #ArvCamera
 * @policy: cache policy
 *
 * Sets the Genicam register cache policy.
 *
 * <warning><para>Be aware that some camera may have wrong Cachable properties defined in their Genicam metadata, which
 * may lead to incorrect readouts. Using the debug cache policy, and activating genicam debug output (export
 * ARV_DEBUG=genicam), can help you to check the cache validity. In this mode, every time the cache content is not in
 * sync with the actual register value, a debug message is printed on the console.</para></warning>
 *
 * Since: 0.8.8
 */

void
arv_camera_set_register_cache_policy (ArvCamera *camera, ArvRegisterCachePolicy policy)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_register_cache_policy (priv->device, policy);
}

/**
 * arv_camera_set_range_check_policy:
 * @camera: a #ArvCamera
 * @policy: range check policy
 *
 * Sets the feature range check policy. When enabled, before being set, the value of all nodes with an #ArvGcFloat or
 * #ArvGcInteger interface will be checked against their Min and Max properties.
 *
 * <warning><para>Be aware that some camera may have wrong definition of Min and Max, as this check is defined as not
 * mandatory in the Genicam specification. If this is the case, it will not possible to set the value of the features
 * with faulty Min or Max definition. Range check is disabled by default.</para></warning>
 *
 * Since: 0.8.8
 */

void
arv_camera_set_range_check_policy (ArvCamera *camera, ArvRangeCheckPolicy policy)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_range_check_policy (priv->device, policy);
}

/**
 * arv_camera_set_access_check_policy:
 * @camera: a #ArvCamera
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
arv_camera_set_access_check_policy (ArvCamera *camera, ArvAccessCheckPolicy policy)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_access_check_policy (priv->device, policy);
}



/**
 * arv_camera_is_gv_device:
 * @camera: a #ArvCamera
 *
 * Returns: %TRUE if @camera is a GigEVision device.
 *
 * Since: 0.4.0
 */

gboolean
arv_camera_is_gv_device	(ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	return ARV_IS_GV_DEVICE (priv->device);
}

/**
 * arv_camera_gv_get_n_network_interfaces:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the number of device network interfaces.
 *
 * Since: 0.8.25
 */

gint
arv_camera_gv_get_n_network_interfaces (ArvCamera *camera, GError **error)
{
	g_return_val_if_fail (arv_camera_is_gv_device (camera), 0);

	return arv_camera_get_integer (camera, "ArvGevNumberOfNetworkInterfaces", error);
}

/**
 * arv_camera_gv_get_n_stream_channels:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the number of supported stream channels.
 *
 * Since: 0.8.0
 */

gint
arv_camera_gv_get_n_stream_channels (ArvCamera *camera, GError **error)
{
	g_return_val_if_fail (arv_camera_is_gv_device (camera), 0);

	return arv_camera_get_integer (camera, "ArvGevStreamChannelCount", error);
}

/**
 * arv_camera_gv_select_stream_channel:
 * @camera: a #ArvCamera
 * @channel_id: id of the channel to select
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Select the current stream channel. Negative @channel_id is ignored.
 *
 * Since: 0.8.0
 */

void
arv_camera_gv_select_stream_channel (ArvCamera *camera, gint channel_id, GError **error)
{
	if (channel_id < 0)
		return;

	g_return_if_fail (arv_camera_is_gv_device (camera));

	arv_camera_set_integer (camera, "ArvGevStreamChannelSelector", channel_id, error);
}

/**
 * arv_camera_gv_get_current_stream_channel:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: The current stream channel id.
 *
 * Since: 0.8.0
 */

int
arv_camera_gv_get_current_stream_channel (ArvCamera *camera, GError **error)
{
	g_return_val_if_fail (arv_camera_is_gv_device (camera), 0);

	return arv_camera_get_integer (camera, "ArvGevStreamChannelSelector", error);
}

/**
 * arv_camera_gv_set_multipart:
 * @camera: a #ArvCamera
 * @enable: %TRUE to enable multipart
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Control multipart payload support
 *
 * Since: 0.8.23
 */

void
arv_camera_gv_set_multipart (ArvCamera *camera, gboolean enable, GError **error)
{
	g_return_if_fail (arv_camera_is_gv_device (camera));

        arv_camera_set_boolean (camera, "ArvGevSCCFGMultipart", enable, error);
}

/**
 * arv_camera_gv_get_multipart:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: %TRUE if multipart payload support is enabled.
 *
 * Since: 0.8.23
 */

gboolean
arv_camera_gv_get_multipart (ArvCamera *camera, GError **error)
{
	g_return_val_if_fail (arv_camera_is_gv_device (camera), FALSE);

        return arv_camera_get_boolean (camera, "ArvGevSCCFGMultipart", error);
}

/**
 * arv_camera_gv_is_multipart_supported:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: %TRUE if multipart payload is supported by @camera.
 *
 * Since: 0.8.23
 */

gboolean
arv_camera_gv_is_multipart_supported (ArvCamera *camera, GError **error)
{
        GError *local_error = NULL;
        gboolean is_supported;

	g_return_val_if_fail (arv_camera_is_gv_device (camera), FALSE);

        is_supported = arv_camera_is_feature_implemented (camera, "ArvGevSCCFGMultipart", &local_error);

        /* Ignore invalid address error, the needed registers are optional */
        if (local_error != NULL) {
                if (local_error->domain == ARV_DEVICE_ERROR &&
                    (local_error->code == ARV_DEVICE_ERROR_PROTOCOL_ERROR_INVALID_ADDRESS ||
                     local_error->code == ARV_DEVICE_ERROR_PROTOCOL_ERROR_ACCESS_DENIED))
                        g_clear_error (&local_error);
                else
                        g_propagate_error(error, local_error);
        }

        return is_supported;
}

/**
 * arv_camera_gv_set_packet_delay:
 * @camera: a #ArvCamera
 * @delay_ns: inter packet delay, in nanoseconds
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Configure the inter packet delay to insert between each packet for the current stream
 * channel. This can be used as a crude flow-control mechanism if the application or the network
 * infrastructure cannot keep up with the packets coming from the device. Negative @delay_ns is ignored.
 *
 * Since: 0.8.0
 */

void
arv_camera_gv_set_packet_delay (ArvCamera *camera, gint64 delay_ns, GError **error)
{
	GError *local_error = NULL;
	gint64 tick_frequency;
	gint64 value;

	if (delay_ns < 0)
		return;

	g_return_if_fail (arv_camera_is_gv_device (camera));

	tick_frequency = arv_camera_get_integer (camera, "ArvGevTimestampTickFrequency", &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	if (tick_frequency <= 0) {
		if (!arv_camera_is_feature_available (camera, "GevSCPD", NULL))
			g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_FEATURE_NOT_FOUND,
				     "GevSCPD not not found");
		else
			g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
				     "Device returned an invalid timestamp tick frequency");
		return;
	}

	value = tick_frequency * delay_ns / 1000000000LL;
	arv_camera_set_integer (camera, "GevSCPD", value, error);
}

/**
 * arv_camera_gv_get_packet_delay:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: The inter packet delay, in nanoseconds.
 *
 * Since: 0.8.0
 */

gint64
arv_camera_gv_get_packet_delay (ArvCamera *camera, GError **error)
{
	GError *local_error = NULL;
	gint64 tick_frequency;
	gint64 value;
	gboolean available;

	g_return_val_if_fail (arv_camera_is_gv_device (camera), 0);

	available = arv_camera_is_feature_available (camera, "GevSCPD", &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	if (!available)
		return 0;

	tick_frequency = arv_camera_get_integer (camera, "ArvGevTimestampTickFrequency", &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	if (tick_frequency <= 0)
		return 0;

	value = arv_camera_get_integer (camera, "GevSCPD", &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	return value * 1000000000LL / tick_frequency;
}

/**
 * arv_camera_gv_set_packet_size:
 * @camera: a #ArvCamera
 * @packet_size: packet size, in bytes
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Specifies the stream packet size, in bytes, to send on the selected channel for a GVSP transmitter
 * or specifies the maximum packet size supported by a GVSP receiver.
 *
 * This does not include data leader and data trailer and the last data packet which might be of
 * smaller size (since packet size is not necessarily a multiple of block size for stream channel).
 * Negative @packet_size is ignored.
 *
 * Since: 0.8.0
 */

void
arv_camera_gv_set_packet_size (ArvCamera *camera, gint packet_size, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	if (packet_size <= 0)
		return;

	g_return_if_fail (arv_camera_is_gv_device (camera));

	arv_gv_device_set_packet_size (ARV_GV_DEVICE (priv->device), packet_size, error);
}

/**
 * arv_camera_gv_get_packet_size:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: The stream packet size, in bytes.
 *
 * Since: 0.8.0
 */

guint
arv_camera_gv_get_packet_size (ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (arv_camera_is_gv_device (camera), 0);

	return arv_gv_device_get_packet_size (ARV_GV_DEVICE (priv->device), error);
}

/**
 * arv_camera_gv_auto_packet_size:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Automatically determine the biggest packet size that can be used data
 * streaming, and set GevSCPSPacketSize value accordingly. This function relies
 * on the GevSCPSFireTestPacket feature. If this feature is not available, the
 * packet size will be set to a default value (1500 bytes).
 *
 * Returns: The packet size, in bytes.
 *
 * Since: 0.8.0
 */

guint
arv_camera_gv_auto_packet_size (ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (arv_camera_is_gv_device (camera), 0);

	return arv_gv_device_auto_packet_size (ARV_GV_DEVICE (priv->device), error);
}

/**
 * arv_camera_gv_set_stream_options:
 * @camera: a #ArvCamera
 * @options: option for stream creation
 *
 * Sets the options used during stream object creation. These options mus be
 * set before the call to arv_camera_create_stream().
 *
 * Since: 0.6.0
 */

void
arv_camera_gv_set_stream_options (ArvCamera *camera, ArvGvStreamOption options)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (arv_camera_is_gv_device (camera));

	arv_gv_device_set_stream_options (ARV_GV_DEVICE (priv->device), options);
}

/**
 * arv_camera_gv_set_packet_size_adjustment:
 * @camera: a #ArvCamera
 * @adjustment: a #ArvGvPacketSizeAdjustment option
 *
 * Sets the option for packet size adjustment that happens at stream object creation.
 *
 * Since: 0.8.3
 */

void
arv_camera_gv_set_packet_size_adjustment (ArvCamera *camera, ArvGvPacketSizeAdjustment adjustment)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (arv_camera_is_gv_device (camera));

	arv_gv_device_set_packet_size_adjustment (ARV_GV_DEVICE (priv->device), adjustment);
}

/**
 * arv_camera_gv_get_persistent_ip:
 * @camera: a #ArvCamera
 * @ip: (out): a IP address placeholder
 * @mask: (out) (optional): a netmask placeholder, %NULL to ignore
 * @gateway: (out) (optional): a gateway IP address placeholder, %NULL to ignore
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Get the persistent IP address setting of camera.
 *
 * Since: 0.8.22
 */

void
arv_camera_gv_get_persistent_ip (ArvCamera *camera,
                                 GInetAddress **ip, GInetAddressMask **mask, GInetAddress **gateway,
                                 GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (arv_camera_is_gv_device (camera));

	arv_gv_device_get_persistent_ip (ARV_GV_DEVICE (priv->device), ip, mask, gateway, error);
}

/**
 * arv_camera_gv_set_persistent_ip_from_string:
 * @camera: a #ArvCamera
 * @ip: IPv4 address in string format
 * @mask: netmask in string format
 * @gateway: Gateway IPv4 address in string format
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Sets the persistent IP address to camera.
 *
 * Since: 0.8.22
 */

void
arv_camera_gv_set_persistent_ip_from_string (ArvCamera *camera,
                                             const char *ip, const char *mask, const char *gateway,
                                             GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (arv_camera_is_gv_device (camera));

	arv_gv_device_set_persistent_ip_from_string (ARV_GV_DEVICE (priv->device), ip, mask, gateway, error);
}

/**
 * arv_camera_gv_set_persistent_ip:
 * @camera: a #ArvCamera
 * @ip: IPv4 address
 * @mask: Netmask
 * @gateway: Gateway IPv4 address
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Sets the persistent IP address to camera.
 *
 * Since: 0.8.22
 */

void
arv_camera_gv_set_persistent_ip (ArvCamera *camera,
                                 GInetAddress *ip, GInetAddressMask *mask, GInetAddress *gateway,
                                 GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (arv_camera_is_gv_device (camera));

	arv_gv_device_set_persistent_ip (ARV_GV_DEVICE (priv->device), ip, mask, gateway, error);
}

/**
 * arv_camera_gv_get_ip_configuration_mode:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Get the IP address configuration mode.
 *
 * Returns: IP address configuration mode
 *
 * Since: 0.8.22
 */

ArvGvIpConfigurationMode
arv_camera_gv_get_ip_configuration_mode(ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (arv_camera_is_gv_device (camera), 0);

	return arv_gv_device_get_ip_configuration_mode (ARV_GV_DEVICE (priv->device), error);
}

/**
 * arv_camera_gv_set_ip_configuration_mode:
 * @camera: a #ArvCamera
 * @mode: IP address configuration mode
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Sets the IP address configuration mode.
 * Available modes are ARV_GV_IP_CONFIGURATION_MODE_DHCP, ARV_GV_IP_CONFIGURATION_MODE_PERSISTENT_IP,
 * ARV_GV_IP_CONFIGURATION_MODE_LLA
 *
 * Since: 0.8.22
 */

void
arv_camera_gv_set_ip_configuration_mode (ArvCamera *camera, ArvGvIpConfigurationMode mode, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (arv_camera_is_gv_device (camera));

	arv_gv_device_set_ip_configuration_mode (ARV_GV_DEVICE (priv->device), mode, error);
}

/**
 * arv_camera_is_uv_device:
 * @camera: a #ArvCamera
 *
 * Returns: %TRUE if @camera is a USB3Vision device.
 *
 * Since: 0.6.0
 */

gboolean
arv_camera_is_uv_device	(ArvCamera *camera)
{
#if ARAVIS_HAS_USB
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
#endif

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

#if ARAVIS_HAS_USB
	return ARV_IS_UV_DEVICE (priv->device);
#else
	return FALSE;
#endif
}

/**
 * arv_camera_uv_is_bandwidth_control_available:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: wether bandwidth limits are available on this camera
 *
 * Since: 0.8.0
 */

gboolean
arv_camera_uv_is_bandwidth_control_available (ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (arv_camera_is_uv_device (camera), FALSE);

	switch (priv->vendor) {
		case ARV_CAMERA_VENDOR_XIMEA:
			return arv_camera_is_feature_available (camera, "DeviceLinkThroughputLimit", error);
		default:
			return FALSE;
	}
}


/**
 * arv_camera_uv_set_bandwidth:
 * @camera: a #ArvCamera
 * @bandwidth: Bandwith limit, in megabits/sec
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Set the bandwith limit or, if @bandwith is not strictly positive, disable the limit.
 *
 * Since: 0.8.0
 */

void
arv_camera_uv_set_bandwidth (ArvCamera *camera, guint bandwidth, GError **error)
{
	GError *local_error = NULL;

	g_return_if_fail (arv_camera_is_uv_device (camera));

	if (bandwidth > 0) {
		arv_camera_set_integer (camera, "DeviceLinkThroughputLimit", bandwidth, &local_error);
		if (local_error == NULL)
			arv_camera_set_integer (camera, "DeviceLinkThroughputLimitMode", 1, &local_error);
	} else {
		arv_camera_set_integer (camera, "DeviceLinkThroughputLimitMode", 0, &local_error);
	}

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

/**
 * arv_camera_uv_get_bandwidth:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: the current bandwidth limit
 *
 * Since: 0.8.0
 */

guint
arv_camera_uv_get_bandwidth (ArvCamera *camera, GError **error)
{
	g_return_val_if_fail (arv_camera_is_uv_device (camera), 0);

	return arv_camera_get_integer (camera, "DeviceLinkThroughputLimit", error);
}

/**
 * arv_camera_uv_get_bandwidth_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum bandwidth
 * @max: (out): maximum bandwidth
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Since: 0.8.0
 */

void
arv_camera_uv_get_bandwidth_bounds (ArvCamera *camera, guint *min, guint *max, GError **error)
{
        if (min != NULL)
                *min = 0;
        if (max != NULL)
                *max = 0;

	g_return_if_fail (arv_camera_is_uv_device (camera));

	arv_camera_get_integer_bounds_as_guint (camera, "DeviceLinkThroughputLimit", min, max, error);
}

/**
 * arv_camera_uv_set_usb_mode:
 * @camera: a #ArvCamera
 * @usb_mode: a #ArvUvUsbMode option
 *
 * Since: 0.8.17
 */

void
arv_camera_uv_set_usb_mode (ArvCamera *camera, ArvUvUsbMode usb_mode)
{
#if ARAVIS_HAS_USB
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
#endif

	g_return_if_fail (arv_camera_is_uv_device (camera));

#if ARAVIS_HAS_USB
	arv_uv_device_set_usb_mode (ARV_UV_DEVICE (priv->device), usb_mode);
#endif
}

/**
 * arv_camera_are_chunks_available:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: %TRUE if chunk data are available
 *
 * Since: 0.8.8
 */

gboolean
arv_camera_are_chunks_available	(ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

        if (!arv_camera_is_feature_available (camera, "ChunkModeActive", error))
                return FALSE;
        return ARV_IS_GC_ENUMERATION (arv_device_get_feature (priv->device, "ChunkSelector"));
}

/**
 * arv_camera_set_chunk_mode:
 * @camera: a #ArvCamera
 * @is_active: wether to enable chunk data mode
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Controls wether chunk data mode is active. When active, chunk data
 * are appended to image data in #ArvBuffer. A #ArvChunkParser must be used in
 * order to extract chunk data.
 *
 * Since: 0.8.0
 **/

void
arv_camera_set_chunk_mode (ArvCamera *camera, gboolean is_active, GError **error)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_camera_set_boolean (camera, "ChunkModeActive", is_active, error);
}

/**
 * arv_camera_get_chunk_mode:
 * @camera: a #ArvCamera
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Check wether chunk data mode is active. Please see arv_camera_set_chunk_mode().
 *
 * Returns: %TRUE if chunk data mode is active.
 *
 * Since: 0.8.0
 **/

gboolean
arv_camera_get_chunk_mode (ArvCamera *camera, GError **error)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	return arv_camera_get_boolean (camera, "ChunkModeActive", error);
}

/**
 * arv_camera_set_chunk_state:
 * @camera: a #ArvCamera
 * @chunk: chunk data name
 * @is_enabled: wether to enable this chunk
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Sets state of a chunk data. Chunk data are be embedded in #ArvBuffer only
 * if chunk mode is active. Please see arv_camera_set_chunk_mode().
 *
 * Since: 0.8.0
 **/

void
arv_camera_set_chunk_state (ArvCamera *camera, const char *chunk, gboolean is_enabled, GError **error)
{
	GError *local_error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (chunk != NULL && chunk[0] != '\0');

	arv_camera_set_string (camera, "ChunkSelector", chunk, &local_error);
	if (local_error == NULL)
		arv_camera_set_boolean (camera, "ChunkEnable", is_enabled, &local_error);

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

/**
 * arv_camera_get_chunk_state:
 * @camera: a #ArvCamera
 * @chunk: chunk data name
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Gets state of chunk data. Chunk data are be embedded in #ArvBuffer only
 * if chunk mode is active. Please see arv_camera_set_chunk_mode().
 *
 * Returns: %TRUE if @chunk is enabled.
 *
 * Since: 0.8.0
 */

gboolean
arv_camera_get_chunk_state (ArvCamera *camera, const char *chunk, GError **error)
{
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);
	g_return_val_if_fail (chunk != NULL && chunk[0] != '\0', FALSE);

	arv_camera_set_string (camera, "ChunkSelector", chunk, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return FALSE;
	}

	return arv_camera_get_boolean (camera, "ChunkEnable", error);
}

/**
 * arv_camera_set_chunks:
 * @camera: a #ArvCamera
 * @chunk_list: chunk data names, as a comma or space separated list
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Convenience function for enabling a set of chunk data. Chunk mode is activated, or deactivated
 * if @chunk_list is %NULL or empty. All chunk data not listed are disabled.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_chunks (ArvCamera *camera, const char *chunk_list, GError **error)
{
	GError *local_error = NULL;
	const char **available_chunks;
	char **chunks;
	char *striped_chunk_list;
	guint i, j;
	guint n_values;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (chunk_list == NULL) {
		arv_camera_set_chunk_mode (camera, FALSE, error);
		return;
	}

	striped_chunk_list = g_strdup (chunk_list);
	arv_str_strip (striped_chunk_list, " ,:;", ',');
	chunks = g_strsplit_set (striped_chunk_list, " ,:;", -1);
	g_free (striped_chunk_list);

        if (chunks == NULL || chunks[0] == NULL) {
                g_strfreev (chunks);
		arv_camera_set_chunk_mode (camera, FALSE, error);
		return;
        }

	arv_camera_set_chunk_mode (camera, TRUE, &local_error);

        if (local_error == NULL) {
                available_chunks = arv_camera_dup_available_enumerations_as_strings (camera,
                                                                                     "ChunkSelector",
                                                                                     &n_values, &local_error);
                for (j = 0; chunks[j] != NULL && local_error == NULL; j++) {
                        gboolean found = FALSE;
                        for (i = 0; i < n_values && local_error == NULL; i++) {
                                if (g_strcmp0 (available_chunks[i], chunks[j]) == 0) {
                                        found = TRUE;
                                        break;
                                }
                        }
                        if (!found) {
                                g_set_error (&local_error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_FEATURE_NOT_FOUND,
                                             "%s not found in available chunks", chunks[j]);
                        }
                }

                for (i = 0; i < n_values && local_error == NULL; i++) {
                        gboolean found = FALSE;
                        for (j = 0; chunks[j] != NULL && local_error == NULL; j++) {
                                if (g_strcmp0 (available_chunks[i], chunks[j]) == 0) {
                                        found = TRUE;
                                        break;
                                }
                        }
                        /* This is normally not needed, but is there as a workaround for an issue with a Basler
                         * acA1300-30gc which seems to reset ChunkActiveMode on every ChunkEnable=false */
                        arv_camera_set_chunk_mode (camera, TRUE, &local_error);
                        if (local_error == NULL)
                                arv_camera_set_chunk_state (camera, available_chunks[i], found, &local_error);
                }

                g_free (available_chunks);
        }

	g_strfreev (chunks);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}
}

/**
 * arv_camera_create_chunk_parser:
 * @camera: a #ArvCamera
 *
 * Creates a new [class@ArvChunkParser] object, used for the extraction of chunk data from [class@ArvBuffer].
 *
 * Returns: (transfer full): a new [class@ArvChunkParser].
 *
 * Since: 0.4.0
 */

ArvChunkParser *
arv_camera_create_chunk_parser (ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_create_chunk_parser (priv->device);
}

/**
 * arv_camera_new:
 * @name: (allow-none): name of the camera.
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Creates a new #ArvCamera. If @name is null, it will instantiate the
 * first available camera.
 *
 * If the camera is a GigEVision, @name can be either:
 *
 * - &lt;vendor&gt;-&lt;model&gt;-&lt;serial&gt;
 * - &lt;vendor_alias&gt;-&lt;serial&gt;
 * - &lt;vendor&gt;-&lt;serial&gt;
 * - &lt;user_id&gt;
 * - &lt;ip_address&gt;
 * - &lt;mac_address&gt;
 *
 * For example:
 *
 * - The Imaging Source Europe GmbH-DFK 33GX265-39020369
 * - The Imaging Source Europe GmbH-39020369
 * - TIS-39020369
 * - 192.168.0.2
 * - 00:07:48:af:a2:61
 *
 * If the camera is a USB3Vision device, @name is either:
 *
 * - &lt;vendor_alias&gt;-&lt;serial&gt;
 * - &lt;vendor&gt;-&lt;serial&gt;
 *
 * Returns: a new #ArvCamera.
 *
 * Since: 0.8.0
 */

ArvCamera *
arv_camera_new (const char *name, GError **error)
{
	/* if you need to apply or test for fixups based on the camera model
	   please do so in arv_camera_constructed and not here, as this breaks
	   objects created with g_object_new, which includes but is not limited to
	   introspection users */

	return g_initable_new (ARV_TYPE_CAMERA, NULL, error, "name", name, NULL);
}

/**
 * arv_camera_new_with_device:
 * @device: (transfer none): a #ArvDevice
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Creates a new #ArvCamera, using @device as its internal device object.
 *
 * Returns: a new #ArvCamera.
 *
 * Since: 0.8.6
 */

ArvCamera *
arv_camera_new_with_device (ArvDevice *device, GError **error)
{
	g_return_val_if_fail (ARV_IS_DEVICE (device), NULL);

	/* if you need to apply or test for fixups based on the camera model
	   please do so in arv_camera_constructed and not here, as this breaks
	   objects created with g_object_new, which includes but is not limited to
	   introspection users */

	return g_initable_new (ARV_TYPE_CAMERA, NULL, error, "device", device, NULL);
}

static void
arv_camera_init (ArvCamera *camera)
{
}

static void
arv_camera_finalize (GObject *object)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (ARV_CAMERA (object));

	g_clear_pointer (&priv->name, g_free);
	g_clear_object (&priv->device);
	g_clear_error (&priv->init_error);

	G_OBJECT_CLASS (arv_camera_parent_class)->finalize (object);
}

static void
arv_camera_constructed (GObject *object)
{
	ArvCamera *camera = ARV_CAMERA (object);
	ArvCameraPrivate *priv;
	ArvCameraVendor vendor;
	ArvCameraSeries series;
	const char *vendor_name;
	const char *model_name;
	GError *error = NULL;

        G_OBJECT_CLASS (arv_camera_parent_class)->constructed (object);

	priv = arv_camera_get_instance_private (camera);

	if (!priv->device)
		priv->device = arv_open_device (priv->name, &error);

	if (!ARV_IS_DEVICE (priv->device)) {
		if (error == NULL) {
			if (priv->name != NULL)
				error = g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
						     "Device '%s' not found", priv->name);
			else
				error = g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
						     "No supported device found");
		}

		g_clear_error (&priv->init_error);
		priv->init_error = error;

		return;
	}

	priv->genicam = arv_device_get_genicam (priv->device);

	vendor_name = arv_camera_get_vendor_name (camera, NULL);
	model_name = arv_camera_get_model_name (camera, NULL);

	if (g_strcmp0 (vendor_name, "Basler") == 0) {
		vendor = ARV_CAMERA_VENDOR_BASLER;
		if (g_str_has_prefix (model_name, "acA"))
			series = ARV_CAMERA_SERIES_BASLER_ACE;
		else if (g_str_has_prefix (model_name, "scA"))
			series = ARV_CAMERA_SERIES_BASLER_SCOUT;
		else
			series = ARV_CAMERA_SERIES_BASLER_OTHER;
	} else if (g_strcmp0 (vendor_name, "Prosilica") == 0) {
		vendor = ARV_CAMERA_VENDOR_PROSILICA;
		series = ARV_CAMERA_SERIES_PROSILICA;
	} else if (g_strcmp0 (vendor_name, "The Imaging Source Europe GmbH") == 0) {
		vendor = ARV_CAMERA_VENDOR_TIS;
		series = ARV_CAMERA_SERIES_TIS;
	} else if (g_strcmp0 (vendor_name, "DALSA") == 0) {
		vendor = ARV_CAMERA_VENDOR_DALSA;
		series = ARV_CAMERA_SERIES_DALSA;
	} else if (g_strcmp0 (vendor_name, "Point Grey Research") == 0 || g_strcmp0 (vendor_name, "FLIR") == 0) {
		vendor = ARV_CAMERA_VENDOR_POINT_GREY_FLIR;
		series = ARV_CAMERA_SERIES_POINT_GREY_FLIR;
	} else if (g_strcmp0 (vendor_name, "Ricoh Company, Ltd.") == 0) {
		vendor = ARV_CAMERA_VENDOR_RICOH;
		series = ARV_CAMERA_SERIES_RICOH;
	} else if (g_strcmp0 (vendor_name, "XIMEA GmbH") == 0) {
		vendor = ARV_CAMERA_VENDOR_XIMEA;
		series = ARV_CAMERA_SERIES_XIMEA;
	} else if (g_strcmp0 (vendor_name, "MATRIX VISION GmbH") == 0) {
		vendor = ARV_CAMERA_VENDOR_MATRIX_VISION;
		series = ARV_CAMERA_SERIES_MATRIX_VISION;
	} else if (g_strcmp0 (vendor_name, "Imperx, Inc") == 0) {
		vendor = ARV_CAMERA_VENDOR_IMPERX;
		if (g_str_has_prefix (model_name, "POE-C"))
			series = ARV_CAMERA_SERIES_IMPERX_CHEETAH;
		else
			series = ARV_CAMERA_SERIES_IMPERX_OTHER;
	} else {
		vendor = ARV_CAMERA_VENDOR_UNKNOWN;
		series = ARV_CAMERA_SERIES_UNKNOWN;
	}

	priv->vendor = vendor;
	priv->series = series;

	priv->has_serial_number = ARV_IS_GC_STRING (arv_device_get_feature (priv->device, "DeviceSerialNumber"));

	priv->has_gain = ARV_IS_GC_FLOAT (arv_device_get_feature (priv->device, "Gain"));
	priv->gain_raw_as_float = ARV_IS_GC_FLOAT (arv_device_get_feature (priv->device, "GainRaw"));
	priv->gain_abs_as_float = ARV_IS_GC_FLOAT (arv_device_get_feature (priv->device, "GainAbs"));

	priv->has_brightness = ARV_IS_GC_FLOAT (arv_device_get_feature (priv->device, "Brightness"));
	priv->has_black_level_raw = ARV_IS_GC_INTEGER (arv_device_get_feature (priv->device, "BlackLevelRaw"));
	priv->has_black_level = ARV_IS_GC_FLOAT (arv_device_get_feature (priv->device, "BlackLevel"));

	priv->has_exposure_time = ARV_IS_GC_FLOAT (arv_device_get_feature (priv->device, "ExposureTime"));
	priv->has_acquisition_frame_rate = ARV_IS_GC_FLOAT (arv_device_get_feature (priv->device,
										    "AcquisitionFrameRate"));
	priv->has_acquisition_frame_rate_auto = ARV_IS_GC_STRING (arv_device_get_feature (priv->device,
											  "AcquisitionFrameRateAuto"));
	priv->has_acquisition_frame_rate_enabled = ARV_IS_GC_BOOLEAN (arv_device_get_feature (priv->device,
											      "AcquisitionFrameRateEnabled"));

        priv->has_region_offset = ARV_IS_GC_INTEGER(arv_device_get_feature(priv->device, "OffsetX")) &&
                ARV_IS_GC_INTEGER(arv_device_get_feature(priv->device, "OffsetY"));
}

static void
arv_camera_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (ARV_CAMERA (object));

	switch (prop_id)
	{
		case PROP_CAMERA_DEVICE:
			priv->device = g_value_dup_object (value);
			break;
		case PROP_CAMERA_NAME:
			g_free (priv->name);
			priv->name = g_value_dup_string (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_camera_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (ARV_CAMERA (object));

	switch (prop_id)
	{
		case PROP_CAMERA_DEVICE:
			g_value_set_object (value, priv->device);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_camera_class_init (ArvCameraClass *camera_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (camera_class);

	object_class->finalize = arv_camera_finalize;
	object_class->constructed = arv_camera_constructed;
	object_class->set_property = arv_camera_set_property;
	object_class->get_property = arv_camera_get_property;

	/**
	 * ArvCamera:name:
	 *
	 * Internal device name for object construction
	 *
	 * Stability: Private
	 */

	g_object_class_install_property
		(object_class,
		 PROP_CAMERA_NAME,
		 g_param_spec_string ("name",
				      "Camera name",
				      "The camera name",
				      NULL,
				      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * ArvCamera:device:
	 *
	 * Internal device object
	 *
	 * Stability: Private
	 */

	g_object_class_install_property
		(object_class,
		 PROP_CAMERA_DEVICE,
		 g_param_spec_object ("device",
				      "Device",
				      "The device associated with this camera",
				      ARV_TYPE_DEVICE,
				      G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static gboolean
arv_camera_initable_init (GInitable     *initable,
			  GCancellable  *cancellable,
			  GError       **error)
{
	ArvCamera *self = ARV_CAMERA (initable);
	ArvCameraPrivate *priv = arv_camera_get_instance_private (ARV_CAMERA (initable));

	g_return_val_if_fail (ARV_IS_CAMERA (self), FALSE);

	if (cancellable != NULL)
	{
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
				     "Cancellable initialization not supported");
		return FALSE;
	}

	if (priv->init_error != NULL) {
		if (error != NULL)
			*error = g_error_copy (priv->init_error);
		 return FALSE;
	}

	return TRUE;
}


static void
arv_camera_initable_iface_init (GInitableIface *iface)
{
	iface->init = arv_camera_initable_init;
}
