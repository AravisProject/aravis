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
 * SECTION:arvcamera
 * @short_description: Class for generic camera control
 *
 * #ArvCamera is a class for the generic control of cameras. It hides the
 * complexity of the genicam interface by providing a simple API, with the
 * drawback of not exposing all the available features. See #ArvDevice and
 * #ArvGc for a more advanced use of the Aravis library.
 *
 * <example id="arv-example"><title>Example use of the ArvCamera API</title>
 * <programlisting>
 * <xi:include xmlns:xi="http://www.w3.org/2001/XInclude" parse="text" href="../../../../tests/arvexample.c">
 *   <xi:fallback>FIXME: MISSING XINCLUDE CONTENT</xi:fallback>
 * </xi:include>
 * </programlisting>
 * </example>
 */

#include <arvfeatures.h>
#include <arvdebug.h>
#include <arvcamera.h>
#include <arvsystem.h>
#include <arvgvinterface.h>
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

static void arv_camera_get_integer_bounds_as_gint (ArvCamera *camera, const char *feature, gint *min, gint *max);
static void arv_camera_get_integer_bounds_as_guint (ArvCamera *camera, const char *feature, guint *min, guint *max);
static void arv_camera_get_integer_bounds_as_double (ArvCamera *camera, const char *feature, double *min, double *max);

/**
 * ArvCameraVendor:
 * @ARV_CAMERA_VENDOR_UNKNOWN: unknown camera vendor
 * @ARV_CAMERA_VENDOR_BASLER: Basler
 * @ARV_CAMERA_VENDOR_PROSILICA: Prosilica
 * @ARV_CAMERA_VENDOR_TIS: The Imaging Source
 * @ARV_CAMERA_VENDOR_POINT_GREY_FLIR: PointGrey / FLIR
 * @ARV_CAMERA_VENDOR_XIMEA: XIMEA GmbH
 * @ARV_CAMERA_VENDOR_MATRIX_VISION: Matrix Vision GmbH
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
	ARV_CAMERA_VENDOR_MATRIX_VISION
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
	ARV_CAMERA_SERIES_MATRIX_VISION
} ArvCameraSeries;

typedef struct {
	ArvDevice *device;
	ArvGc *genicam;

	ArvCameraVendor vendor;
	ArvCameraSeries series;

	gboolean has_gain;
	gboolean has_exposure_time;
	gboolean has_acquisition_frame_rate;
	gboolean has_acquisition_frame_rate_enabled;

	GError *error;	/* First error that changed camera status. Cleared after status read.  */
} ArvCameraPrivate;

G_DEFINE_TYPE_WITH_CODE (ArvCamera, arv_camera, G_TYPE_OBJECT, G_ADD_PRIVATE (ArvCamera))

enum
{
	PROP_0,
	PROP_CAMERA_DEVICE
};

/**
 * arv_camera_create_stream:
 * @camera: a #ArvCamera
 * @callback: (scope call) (allow-none): a frame processing callback
 * @user_data: (closure) (allow-none): user data for @callback
 *
 * Creates a new #ArvStream for video stream handling. See
 * #ArvStreamCallback for details regarding the callback function.
 *
 * Returns: (transfer full): a new #ArvStream.
 *
 * Since: 0.2.0
 */

ArvStream *
arv_camera_create_stream (ArvCamera *camera, ArvStreamCallback callback, gpointer user_data)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_create_stream (priv->device, callback, user_data);
}

/* Device control */

/**
 * arv_camera_get_vendor_name:
 * @camera: a #ArvCamera
 *
 * Returns: the camera vendor name.
 *
 * Since: 0.2.0
 */

const char *
arv_camera_get_vendor_name (ArvCamera *camera)
{
	return arv_camera_get_string (camera, "DeviceVendorName");
}

/**
 * arv_camera_get_model_name:
 * @camera: a #ArvCamera
 *
 * Returns: the camera model name.
 *
 * Since: 0.2.0
 */

const char *
arv_camera_get_model_name (ArvCamera *camera)
{
	return arv_camera_get_string (camera, "DeviceModelName");
}

/**
 * arv_camera_get_device_id:
 * @camera: a #ArvCamera
 *
 * Returns: the camera device ID.
 *
 * Since: 0.2.0
 */

const char *
arv_camera_get_device_id (ArvCamera *camera)
{
	return arv_camera_get_string (camera, "DeviceID");
}

/* Image format control */

/**
 * arv_camera_get_sensor_size:
 * @camera: a #ArvCamera
 * @width: (out): camera sensor width
 * @height: (out): camera sensor height
 *
 * Since: 0.2.0
 */

void
arv_camera_get_sensor_size (ArvCamera *camera, gint *width, gint *height)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (width != NULL)
		*width = arv_camera_get_integer (camera, "SensorWidth");
	if (height != NULL)
		*height = arv_camera_get_integer (camera, "SensorHeight");
}

/**
 * arv_camera_set_region:
 * @camera: a #ArvCamera
 * @x: x offset
 * @y: y_offset
 * @width: region width
 * @height: region height
 *
 * Defines the region of interest which will be transmitted in the video
 * stream.
 *
 * Since: 0.2.0
 */

void
arv_camera_set_region (ArvCamera *camera, gint x, gint y, gint width, gint height)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (x >= 0)
		arv_camera_set_integer (camera, "OffsetX", 0);
	if (y >= 0)
		arv_camera_set_integer (camera, "OffsetY", 0);
	if (width > 0)
		arv_camera_set_integer (camera, "Width", width);
	if (height > 0)
		arv_camera_set_integer (camera, "Height", height);
	if (x >= 0)
		arv_camera_set_integer (camera, "OffsetX", x);
	if (y >= 0)
		arv_camera_set_integer (camera, "OffsetY", y);
}

/**
 * arv_camera_get_region:
 * @camera: a #ArvCamera
 * @x: (out): x offset
 * @y: (out): y_offset
 * @width: (out): region width
 * @height: (out): region height
 *
 * Retrieves the current region of interest.
 *
 * Since: 0.2.0
 */

void
arv_camera_get_region (ArvCamera *camera, gint *x, gint *y, gint *width, gint *height)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (x != NULL)
		*x = arv_camera_get_integer (camera, "OffsetX");
	if (y != NULL)
		*y = arv_camera_get_integer (camera, "OffsetY");
	if (width != NULL)
		*width = arv_camera_get_integer (camera, "Width");
	if (height != NULL)
		*height = arv_camera_get_integer (camera, "Height");
}

/**
 * arv_camera_get_x_offset_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum offset
 * @max: (out): maximum offset
 *
 * Retrieves the valid range for image horizontal offset.
 *
 * Since: 0.6.0
 */

void
arv_camera_get_x_offset_bounds (ArvCamera *camera, gint *min, gint *max)
{
	arv_camera_get_integer_bounds_as_gint (camera, "OffsetX", min, max);
}

/**
 * arv_camera_get_x_offset_increment:
 * @camera: a #ArvCamera
 *
 * Returns: horizontal offset value increment.
 *
 * Since: 0.8.0
 */

gint
arv_camera_get_x_offset_increment (ArvCamera *camera)
{
	return arv_camera_get_integer_increment (camera, "OffsetX");
}

/**
 * arv_camera_get_y_offset_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum offset
 * @max: (out): maximum offset
 *
 * Retrieves the valid range for image vertical offset.
 *
 * Since: 0.6.0
 */

void
arv_camera_get_y_offset_bounds (ArvCamera *camera, gint *min, gint *max)
{
	arv_camera_get_integer_bounds_as_gint (camera, "OffsetY", min, max);
}

/**
 * arv_camera_get_y_offset_increment:
 * @camera: a #ArvCamera
 *
 * Returns: vertical offset value increment.
 *
 * Since: 0.8.0
 */

gint
arv_camera_get_y_offset_increment (ArvCamera *camera)
{
	return arv_camera_get_integer_increment (camera, "OffsetY");
}

/**
 * arv_camera_get_width_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum width
 * @max: (out): maximum width
 *
 * Retrieves the valid range for image width.
 *
 * Since: 0.6.0
 */

void
arv_camera_get_width_bounds (ArvCamera *camera, gint *min, gint *max)
{
	arv_camera_get_integer_bounds_as_gint (camera, "Width", min, max);
}

/**
 * arv_camera_get_width_increment:
 * @camera: a #ArvCamera
 *
 * Returns: width value increment.
 *
 * Since: 0.8.0
 */

gint
arv_camera_get_width_increment (ArvCamera *camera)
{
	return arv_camera_get_integer_increment (camera, "Width");
}

/**
 * arv_camera_get_height_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum height
 * @max: (out): maximum height
 *
 * Retrieves the valid range for image height.
 *
 * Since: 0.6.0
 */

void
arv_camera_get_height_bounds (ArvCamera *camera, gint *min, gint *max)
{
	arv_camera_get_integer_bounds_as_gint (camera, "Height", min, max);
}

/**
 * arv_camera_get_height_increment:
 * @camera: a #ArvCamera
 *
 * Returns: height value increment.
 *
 * Since: 0.8.0
 */

gint
arv_camera_get_height_increment (ArvCamera *camera)
{
	return arv_camera_get_integer_increment (camera, "Height");
}

/**
 * arv_camera_set_binning:
 * @camera: a #ArvCamera
 * @dx: horizontal binning
 * @dy: vertical binning
 *
 * Defines binning in both directions. Not all cameras support this
 * feature.
 *
 * Since: 0.6.0
 */

void
arv_camera_set_binning (ArvCamera *camera, gint dx, gint dy)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (dx > 0)
		arv_camera_set_integer (camera, "BinningHorizontal", dx);
	if (dy > 0)
		arv_camera_set_integer (camera, "BinningVertical", dy);
}

/**
 * arv_camera_get_binning:
 * @camera: a #ArvCamera
 * @dx: (out): horizontal binning placeholder
 * @dy: (out): vertical binning placeholder
 *
 * Retrieves binning in both directions.
 *
 * Since: 0.2.0
 */

void
arv_camera_get_binning (ArvCamera *camera, gint *dx, gint *dy)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (dx != NULL)
		*dx = arv_camera_get_integer (camera, "BinningHorizontal");
	if (dy != NULL)
		*dy = arv_camera_get_integer (camera, "BinningVertical");
}

/**
 * arv_camera_get_x_binning_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum binning
 * @max: (out): maximum binning
 *
 * Retrieves the valid range for image horizontal binning.
 *
 * Since: 0.6.0
 */

void
arv_camera_get_x_binning_bounds (ArvCamera *camera, gint *min, gint *max)
{
	arv_camera_get_integer_bounds_as_gint (camera, "BinningHorizontal", min, max);
}

/**
 * arv_camera_get_x_binning_increment:
 * @camera: a #ArvCamera
 *
 * Returns: horizontal binning value increment.
 *
 * Since: 0.8.0
 */

gint
arv_camera_get_x_binning_increment (ArvCamera *camera)
{
	return arv_camera_get_integer_increment (camera, "BinningHorizontal");
}

/**
 * arv_camera_get_y_binning_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum binning
 * @max: (out): maximum binning
 *
 * Retrieves the valid range for image vertical binning.
 *
 * Since: 0.6.0
 */

void
arv_camera_get_y_binning_bounds (ArvCamera *camera, gint *min, gint *max)
{
	arv_camera_get_integer_bounds_as_gint (camera, "BinningVertical", min, max);
}

/**
 * arv_camera_get_y_binning_increment:
 * @camera: a #ArvCamera
 *
 * Returns: vertical binning value increment.
 *
 * Since: 0.8.0
 */

gint
arv_camera_get_y_binning_increment (ArvCamera *camera)
{
	return arv_camera_get_integer_increment (camera, "BinningVertical");
}

/**
 * arv_camera_set_pixel_format:
 * @camera: a #ArvCamera
 * @format: pixel format
 *
 * Defines pixel format.
 *
 * Since: 0.2.0
 */

void
arv_camera_set_pixel_format (ArvCamera *camera, ArvPixelFormat format)
{
	arv_camera_set_integer (camera, "PixelFormat", format);
}

/**
 * arv_camera_set_pixel_format_from_string:
 * @camera: a #ArvCamera
 * @format: pixel format
 *
 * Defines pixel format described by a string.
 *
 * Since: 0.2.0
 */

void
arv_camera_set_pixel_format_from_string (ArvCamera *camera, const char * format)
{
        arv_camera_set_string (camera, "PixelFormat", format);
}

/**
 * arv_camera_get_pixel_format:
 * @camera: a #ArvCamera
 *
 * Returns: pixel format.
 *
 * Since: 0.2.0
 */

ArvPixelFormat
arv_camera_get_pixel_format (ArvCamera *camera)
{
	return arv_camera_get_integer (camera, "PixelFormat");
}

/**
 * arv_camera_get_pixel_format_as_string:
 * @camera: a #ArvCamera
 *
 * Retuns: pixel format as string, NULL on error.
 *
 * Since: 0.2.0
 */

const char *
arv_camera_get_pixel_format_as_string (ArvCamera *camera)
{
	return arv_camera_get_string (camera, "PixelFormat");
}

/**
 * arv_camera_get_available_pixel_formats:
 * @camera: a #ArvCamera
 * @n_pixel_formats: (out): number of different pixel formats
 *
 * Retrieves the list of all available pixel formats.
 *
 * Returns: (array length=n_pixel_formats) (transfer container): a newly allocated array of #ArvPixelFormat
 *
 * Since: 0.2.0
 */

gint64 *
arv_camera_get_available_pixel_formats (ArvCamera *camera, guint *n_pixel_formats)
{
	return arv_camera_get_available_enumerations (camera, "PixelFormat", n_pixel_formats);
}

/**
 * arv_camera_get_available_pixel_formats_as_strings:
 * @camera: a #ArvCamera
 * @n_pixel_formats: (out): number of different pixel formats
 *
 * Retrieves the list of all available pixel formats as strings.
 *
 * Returns: (array length=n_pixel_formats) (transfer container): a newly allocated array of strings.
 *
 * Since: 0.2.0
 */

const char **
arv_camera_get_available_pixel_formats_as_strings (ArvCamera *camera, guint *n_pixel_formats)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_camera_get_available_enumerations_as_strings (camera, "PixelFormat", n_pixel_formats);
}

/**
 * arv_camera_get_available_pixel_formats_as_display_names:
 * @camera: a #ArvCamera
 * @n_pixel_formats: (out): number of different pixel formats
 *
 * Retrieves the list of all available pixel formats as display names.
 * In general, these human-readable strings cannot be used as settings.
 *
 * Returns: (array length=n_pixel_formats) (transfer container): a newly allocated array of string constants.
 *
 * Since: 0.2.0
 */

const char **
arv_camera_get_available_pixel_formats_as_display_names (ArvCamera *camera, guint *n_pixel_formats)
{
	return arv_camera_get_available_enumerations_as_display_names (camera, "PixelFormat", n_pixel_formats);
}

/* Acquisition control */

/**
 * arv_camera_start_acquisition:
 * @camera: a #ArvCamera
 *
 * Starts video stream acquisition.
 *
 * Since: 0.2.0
 */

void
arv_camera_start_acquisition (ArvCamera *camera)
{
	arv_camera_execute_command (camera, "AcquisitionStart");
}

/**
 * arv_camera_stop_acquisition:
 * @camera: a #ArvCamera
 *
 * Stops video stream acquisition.
 *
 * Since: 0.2.0
 */

void
arv_camera_stop_acquisition (ArvCamera *camera)
{
	arv_camera_execute_command (camera, "AcquisitionStop");
}

/**
 * arv_camera_abort_acquisition:
 * @camera: a #ArvCamera
 *
 * Aborts video stream acquisition.
 *
 * Since: 0.4.0
 */

void
arv_camera_abort_acquisition (ArvCamera *camera)
{
	arv_camera_execute_command (camera, "AcquisitionAbort");
}

/**
 * arv_camera_acquisition:
 * @camera: a #ArvCamera
 * @timeout: acquisition timeout in µs. Zero means no timeout.
 *
 * Acquire one image buffer.
 *
 * Returns: (transfer full): A new #ArvBuffer, NULL on error. The returned buffer must be freed using g_object_unref().
 *
 * Since: 0.6.0
 */

ArvBuffer *
arv_camera_acquisition (ArvCamera *camera, guint64 timeout)
{
	ArvStream *stream;
	ArvBuffer *buffer;
	gint payload;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	stream = arv_camera_create_stream (camera, NULL, NULL);
	payload = arv_camera_get_payload (camera);
	arv_stream_push_buffer (stream,  arv_buffer_new (payload, NULL));
	arv_camera_set_acquisition_mode (camera, ARV_ACQUISITION_MODE_SINGLE_FRAME);
	arv_camera_start_acquisition (camera);
	if (timeout > 0)
		buffer = arv_stream_timeout_pop_buffer (stream, timeout);
	else
		buffer = arv_stream_pop_buffer (stream);
	arv_camera_stop_acquisition (camera);
	g_object_unref (stream);

	return buffer;
}

/*
 * arv_camera_set_acquisition_mode:
 * @camera: a #ArvCamera
 * @acquisition_mode: acquisition mode
 *
 * Defines acquisition mode.
 *
 * Since: 0.2.0
 */

void
arv_camera_set_acquisition_mode (ArvCamera *camera, ArvAcquisitionMode acquisition_mode)
{
	arv_camera_set_string (camera, "AcquisitionMode", arv_acquisition_mode_to_string (acquisition_mode));
}

/**
 * arv_camera_get_acquisition_mode:
 * @camera: a #ArvCamera
 *
 * Returns: acquisition mode.
 *
 * Since: 0.2.0
 */

ArvAcquisitionMode
arv_camera_get_acquisition_mode (ArvCamera *camera)
{
	const char *string;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0);

	string = arv_camera_get_string (camera, "AcquisitionMode");

	return arv_acquisition_mode_from_string (string);
}

/**
 * arv_camera_set_frame_count:
 * @camera: a #ArvCamera
 * @frame_count: number of frames to capture in MultiFrame mode
 *
 * Sets the number of frames to capture in MultiFrame mode.
 *
 * Since: 0.6.0
 */

void
arv_camera_set_frame_count (ArvCamera *camera, gint64 frame_count)
{
	gint64 minimum;
	gint64 maximum;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (frame_count <= 0)
		return;

	arv_camera_get_frame_count_bounds (camera, &minimum, &maximum);

	if (frame_count < minimum)
		frame_count = minimum;
	if (frame_count > maximum)
		frame_count = maximum;

	arv_camera_set_integer (camera, "AcquisitionFrameCount", frame_count);
}

/**
 * arv_camera_get_frame_count:
 * @camera: a #ArvCamera
 *
 * Returns: number of frames to capture in MultiFrame mode.
 *
 * Since: 0.6.0
 */

gint64
arv_camera_get_frame_count (ArvCamera *camera)
{
	return arv_camera_get_integer (camera, "AcquisitionFrameCount");
}

/**
 * arv_camera_get_frame_count_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimal possible frame count
 * @max: (out): maximum possible frame count
 *
 * Retrieves allowed range for frame count.
 *
 * Since: 0.6.0
 */

void
arv_camera_get_frame_count_bounds (ArvCamera *camera, gint64 *min, gint64 *max)
{
	arv_camera_get_integer_bounds (camera, "AcquisitionFrameCount", min, max);
}

/**
 * arv_camera_set_frame_rate:
 * @camera: a #ArvCamera
 * @frame_rate: frame rate, in Hz
 *
 * Configures a fixed frame rate mode. Once acquisition start is triggered, the
 * video stream will be acquired with the given frame rate.
 *
 * Since: 0.2.0
 */

void
arv_camera_set_frame_rate (ArvCamera *camera, double frame_rate)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	ArvGcNode *feature;
	double minimum;
	double maximum;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (frame_rate <= 0.0)
		return;

	arv_camera_get_frame_rate_bounds (camera, &minimum, &maximum);

	if (frame_rate < minimum)
		frame_rate = minimum;
	if (frame_rate > maximum)
		frame_rate = maximum;

	switch (priv->vendor) {
		case ARV_CAMERA_VENDOR_BASLER:
			arv_camera_set_string (camera, "TriggerSelector", "AcquisitionStart");
			arv_camera_set_string (camera, "TriggerMode", "Off");
			arv_camera_set_string (camera, "TriggerSelector", "FrameStart");
			arv_camera_set_string (camera, "TriggerMode", "Off");
			arv_camera_set_boolean (camera, "AcquisitionFrameRateEnable", TRUE);
			arv_camera_set_float (camera,
					      priv->has_acquisition_frame_rate ?
					      "AcquisitionFrameRate":
					      "AcquisitionFrameRateAbs", frame_rate);
			break;
		case ARV_CAMERA_VENDOR_PROSILICA:
			arv_camera_set_string (camera, "TriggerSelector",
					       "FrameStart");
			arv_camera_set_string (camera, "TriggerMode", "Off");
			arv_camera_set_float (camera, "AcquisitionFrameRateAbs",
					      frame_rate);
			break;
		case ARV_CAMERA_VENDOR_TIS:
			arv_camera_set_string (camera, "TriggerSelector", "FrameStart");
			arv_camera_set_string (camera, "TriggerMode", "Off");
			feature = arv_device_get_feature (priv->device, "FPS");
			if (ARV_IS_GC_ENUMERATION (feature)) {
				gint64 *values;
				guint n_values;
				guint i;

				values = arv_camera_get_available_enumerations (camera, "FPS", &n_values);
				for (i = 0; i < n_values; i++) {
					if (values[i] > 0) {
						double e;

						e = (int)((10000000/(double) values[i]) * 100 + 0.5) / 100.0;
						if (e == frame_rate) {
							arv_camera_set_integer (camera, "FPS", values[i]);
							break;
						}
					}
				}
				g_free (values);
			} else
				arv_camera_set_float (camera, "FPS", frame_rate);
			break;
		case ARV_CAMERA_VENDOR_POINT_GREY_FLIR:
			arv_camera_set_string (camera, "TriggerSelector", "FrameStart");
			arv_camera_set_string (camera, "TriggerMode", "Off");
			if (priv->has_acquisition_frame_rate_enabled)
				arv_camera_set_integer (camera, "AcquisitionFrameRateEnabled", 1);
			else
				arv_camera_set_boolean (camera, "AcquisitionFrameRateEnable", TRUE);
			arv_camera_set_string (camera, "AcquisitionFrameRateAuto", "Off");
			arv_camera_set_float (camera, "AcquisitionFrameRate", frame_rate);
			break;
		case ARV_CAMERA_VENDOR_DALSA:
		case ARV_CAMERA_VENDOR_RICOH:
		case ARV_CAMERA_VENDOR_XIMEA:
		case ARV_CAMERA_VENDOR_MATRIX_VISION:
		case ARV_CAMERA_VENDOR_UNKNOWN:
			arv_camera_set_string (camera, "TriggerSelector", "FrameStart");
			arv_camera_set_string (camera, "TriggerMode", "Off");
			arv_camera_set_float (camera,
					      priv->has_acquisition_frame_rate ?
					      "AcquisitionFrameRate":
					      "AcquisitionFrameRateAbs", frame_rate);
			break;
	}
}

/**
 * arv_camera_get_frame_rate:
 * @camera: a #ArvCamera
 *
 * Returns: actual frame rate, in Hz.
 *
 * Since: 0.2.0
 */

double
arv_camera_get_frame_rate (ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	ArvGcNode *feature;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), -1.0);

	switch (priv->vendor) {
		case ARV_CAMERA_VENDOR_PROSILICA:
			return arv_camera_get_float (camera, "AcquisitionFrameRateAbs");
		case ARV_CAMERA_VENDOR_TIS:
			feature = arv_device_get_feature (priv->device, "FPS");
			if (ARV_IS_GC_ENUMERATION (feature)) {
				gint64 i;

				i = arv_camera_get_integer (camera, "FPS");
				if (i > 0)
					return (int)((10000000/(double) i) * 100 + 0.5) / 100.0;
				else
					return 0;
			} else
				return arv_camera_get_float (camera, "FPS");
		case ARV_CAMERA_VENDOR_POINT_GREY_FLIR:
		case ARV_CAMERA_VENDOR_DALSA:
		case ARV_CAMERA_VENDOR_RICOH:
		case ARV_CAMERA_VENDOR_BASLER:
	        case ARV_CAMERA_VENDOR_XIMEA:
		case ARV_CAMERA_VENDOR_MATRIX_VISION:
	        case ARV_CAMERA_VENDOR_UNKNOWN:
			return arv_camera_get_float (camera,
						     priv->has_acquisition_frame_rate ?
						     "AcquisitionFrameRate":
						     "AcquisitionFrameRateAbs");
	}

	return -1.0;
}

/**
 * arv_camera_get_frame_rate_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimal possible framerate
 * @max: (out): maximum possible framerate
 *
 * Retrieves allowed range for framerate.
 *
 * Since 0.2.2
 */

void
arv_camera_get_frame_rate_bounds (ArvCamera *camera, double *min, double *max)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	ArvGcNode *feature;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	switch (priv->vendor) {
		case ARV_CAMERA_VENDOR_TIS:
			feature = arv_device_get_feature (priv->device, "FPS");
			if (ARV_IS_GC_ENUMERATION (feature)) {
				gint64 *values;
				guint n_values;
				guint i;

				values = arv_camera_get_available_enumerations (camera, "FPS", &n_values);
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
			} else
				arv_camera_get_float_bounds (camera, "FPS", min, max);
			break;
		case ARV_CAMERA_VENDOR_PROSILICA:
			arv_camera_get_float_bounds (camera, "AcquisitionFrameRateAbs", min, max);
			break;
		case ARV_CAMERA_VENDOR_POINT_GREY_FLIR:
		case ARV_CAMERA_VENDOR_DALSA:
		case ARV_CAMERA_VENDOR_RICOH:
		case ARV_CAMERA_VENDOR_BASLER:
	        case ARV_CAMERA_VENDOR_XIMEA:
		case ARV_CAMERA_VENDOR_MATRIX_VISION:
		case ARV_CAMERA_VENDOR_UNKNOWN:
			arv_camera_get_float_bounds (camera,
						     priv->has_acquisition_frame_rate ?
						     "AcquisitionFrameRate":
						     "AcquisitionFrameRateAbs",
						     min, max);
			break;
	}
}

/**
 * arv_camera_set_trigger:
 * @camera: a #ArvCamera
 * @source: trigger source as string
 *
 * Configures the camera in trigger mode. Typical values for source are "Line1"
 * or "Line2". See the camera documentation for the allowed values.
 * Activation is set to rising edge. It can be changed by accessing the
 * underlying device object.
 *
 * Source can also be "Software". In this case, an acquisition is triggered
 * by a call to arv_camera_software_trigger().
 *
 * Since: 0.2.0
 */

void
arv_camera_set_trigger (ArvCamera *camera, const char *source)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (source != NULL);

	if (priv->vendor == ARV_CAMERA_VENDOR_BASLER)
		arv_camera_set_integer (camera, "AcquisitionFrameRateEnable", 0);

	arv_camera_set_string (camera, "TriggerSelector", "AcquisitionStart");
	arv_camera_set_string (camera, "TriggerMode", "Off");
	arv_camera_set_string (camera, "TriggerSelector", "FrameStart");
	arv_camera_set_string (camera, "TriggerMode", "On");
	arv_camera_set_string (camera, "TriggerActivation", "RisingEdge");
	arv_camera_set_string (camera, "TriggerSource", source);
}

/**
 * arv_camera_set_trigger_source:
 * @camera: a #ArvCamera
 * @source: source name
 *
 * Sets the trigger source. This function doesn't check if the camera is configured
 * to actually use this source as a trigger.
 *
 * Since: 0.2.0
 */

void
arv_camera_set_trigger_source (ArvCamera *camera, const char *source)
{
	arv_camera_set_string (camera, "TriggerSource", source);
}

/**
 * arv_camera_get_trigger_source:
 * @camera: a #ArvCamera
 *
 * Gets the trigger source. This function doesn't check if the camera is configured
 * to actually use this source as a trigger.
 *
 * Returns: a string containing the trigger source name, NULL on error.
 *
 * Since: 0.2.0
 */

const char *
arv_camera_get_trigger_source (ArvCamera *camera)
{
	return arv_camera_get_string (camera, "TriggerSource");
}

/**
 * arv_camera_get_available_trigger_sources:
 * @camera: a #ArvCamera
 * @n_sources: (out): number of sources
 *
 * Gets the list of all available trigger sources.
 *
 * Returns: (array length=n_sources) (transfer container): a newly allocated array of strings, which must be freed using g_free().
 *
 * Since: 0.6.0
 */

const char **
arv_camera_get_available_trigger_sources (ArvCamera *camera, guint *n_sources)
{
	return arv_camera_get_available_enumerations_as_strings (camera, "TriggerSource", n_sources);
}

/**
 * arv_camera_get_available_triggers:
 * @camera: a #ArvCamera
 * @n_triggers: (out): number of available triggers
 *
 * Gets a list of all available triggers: FrameStart, ExposureActive, etc...
 *
 * Returns: (array length=n_triggers) (transfer container): a newly allocated array of strings, which must be freed using g_free().
 *
 * Since: 0.6.0
 */

const char **
arv_camera_get_available_triggers (ArvCamera *camera, guint *n_triggers)
{
	return arv_camera_get_available_enumerations_as_strings (camera, "TriggerSelector", n_triggers);
}

/**
 * arv_camera_clear_triggers:
 * @camera: a #ArvCamera
 *
 * Disables all triggers.
 *
 * Since: 0.6.0
 */

void
arv_camera_clear_triggers (ArvCamera* camera)
{
	const char **triggers;
	guint n_triggers;
	unsigned i;

        g_return_if_fail (ARV_IS_CAMERA (camera));

	triggers = arv_camera_get_available_enumerations_as_strings (camera, "TriggerSelector", &n_triggers);

	for (i = 0; i< n_triggers; i++) {
		arv_camera_set_string (camera, "TriggerSelector", triggers[i]);
		arv_camera_set_string (camera, "TriggerMode", "Off");
	}
}

/**
 * arv_camera_software_trigger:
 * @camera: a #ArvCamera
 *
 * Sends a software trigger command to @camera. The camera must be previously
 * configured to use a software trigger, using @arv_camera_set_trigger().
 *
 * Since: 0.2.0
 */

void
arv_camera_software_trigger (ArvCamera *camera)
{
	arv_camera_execute_command (camera, "TriggerSoftware");
}

/**
 * arv_camera_set_exposure_time:
 * @camera: a #ArvCamera
 * @exposure_time_us: exposure time, in µs
 *
 * Sets exposure time. User should take care to set a value compatible with
 * the desired frame rate.
 *
 * Since: 0.2.0
 */

void
arv_camera_set_exposure_time (ArvCamera *camera, double exposure_time_us)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (exposure_time_us <= 0)
		return;

	switch (priv->series) {
		case ARV_CAMERA_SERIES_BASLER_SCOUT:
			arv_camera_set_float (camera, "ExposureTimeBaseAbs", exposure_time_us);
			arv_camera_set_integer (camera, "ExposureTimeRaw", 1);
			break;
		case ARV_CAMERA_SERIES_RICOH:
			arv_camera_set_integer (camera, "ExposureTimeRaw", exposure_time_us);
			break;
		case ARV_CAMERA_SERIES_XIMEA:
			arv_camera_set_integer (camera, "ExposureTime", exposure_time_us);
			break;
		case ARV_CAMERA_SERIES_MATRIX_VISION:
			arv_camera_set_string (camera, "ExposureMode", "Timed");
			arv_camera_set_float (camera, "ExposureTime", exposure_time_us);
			break;
		case ARV_CAMERA_SERIES_BASLER_ACE:
		default:
			arv_camera_set_float (camera,
					      priv->has_exposure_time ?
					      "ExposureTime" :
					      "ExposureTimeAbs", exposure_time_us);
			break;
	}
}

/**
 * arv_camera_get_exposure_time:
 * @camera: a #ArvCamera
 *
 * Returns: current exposure time, in µs.
 *
 * Since: 0.2.0
 */

double
arv_camera_get_exposure_time (ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0.0);

	switch (priv->series) {
		case ARV_CAMERA_SERIES_XIMEA:
			return arv_camera_get_integer (camera,"ExposureTime");
		case ARV_CAMERA_SERIES_RICOH:
			return arv_camera_get_integer (camera,"ExposureTimeRaw");
		default:
			return arv_camera_get_float (camera,
						     priv->has_exposure_time ?
						     "ExposureTime" :
						     "ExposureTimeAbs");
	}
}

/**
 * arv_camera_get_exposure_time_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum exposure time
 * @max: (out): maximum exposure time
 *
 * Retrieves exposure time bounds, in µs.
 *
 * Since: 0.2.0
 */

void
arv_camera_get_exposure_time_bounds (ArvCamera *camera, double *min, double *max)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	switch (priv->series) {
		case ARV_CAMERA_SERIES_BASLER_SCOUT:
			arv_camera_get_float_bounds (camera,
						     priv->has_exposure_time ?
						     "ExposureTime" :
						     "ExposureTimeBaseAbs",
						     min, max);
			break;
		case ARV_CAMERA_SERIES_BASLER_ACE:
			if (priv->has_exposure_time)
				arv_camera_get_float_bounds (camera, "ExposureTime", min, max);
			else
				arv_camera_get_integer_bounds_as_double (camera, "ExposureTimeRaw", min, max);
			break;
		case ARV_CAMERA_SERIES_XIMEA:
			arv_camera_get_integer_bounds_as_double (camera, "ExposureTime", min, max);
			break;
		case ARV_CAMERA_SERIES_RICOH:
			arv_camera_get_integer_bounds_as_double (camera, "ExposureTimeRaw", min, max);
			break;
		default:
			arv_camera_get_float_bounds (camera,
						     priv->has_exposure_time ?
						     "ExposureTime" :
						     "ExposureTimeAbs",
						     min, max);
			break;
	}
}

/**
 * arv_camera_set_exposure_time_auto:
 * @camera: a #ArvCamera
 * @auto_mode: auto exposure mode selection
 *
 * Configures automatic exposure feature.
 *
 * Since: 0.2.0
 */

void
arv_camera_set_exposure_time_auto (ArvCamera *camera, ArvAuto auto_mode)
{
	arv_camera_set_string (camera, "ExposureAuto", arv_auto_to_string (auto_mode));
}

/**
 * arv_camera_get_exposure_time_auto:
 * @camera: a #ArvCamera
 *
 * Returns: auto exposure mode selection
 *
 * Since: 0.2.0
 */

ArvAuto
arv_camera_get_exposure_time_auto (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), ARV_AUTO_OFF);

	return arv_auto_from_string (arv_camera_get_string (camera, "ExposureAuto"));
}

/* Analog control */

/**
 * arv_camera_set_gain:
 * @camera: a #ArvCamera
 * @gain: gain value
 *
 * Sets the gain of the ADC converter.
 *
 * Since: 0.2.0
 */

void
arv_camera_set_gain (ArvCamera *camera, double gain)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (gain < 0)
		return;

	if (priv->has_gain)
		arv_camera_set_float (camera, "Gain", gain);
	else
		arv_camera_set_integer (camera, "GainRaw", gain);
}

/**
 * arv_camera_get_gain:
 * @camera: a #ArvCamera
 *
 * Returns: the current gain setting.
 *
 * Since: 0.2.0
 */

double
arv_camera_get_gain (ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0.0);

	if (priv->has_gain)
		return arv_camera_get_float (camera, "Gain");

	return arv_camera_get_integer (camera, "GainRaw");
}

/**
 * arv_camera_get_gain_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum gain
 * @max: (out): maximum gain
 *
 * Retrieves gain bounds.
 *
 * Since: 0.2.0
 */

void
arv_camera_get_gain_bounds (ArvCamera *camera, double *min, double *max)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (priv->has_gain) {
		arv_camera_get_float_bounds (camera, "Gain", min, max);
		return;
	}

	arv_camera_get_integer_bounds_as_double (camera, "GainRaw", min, max);

	return;
}

/**
 * arv_camera_set_gain_auto:
 * @camera: a #ArvCamera
 * @auto_mode: auto gain mode selection
 *
 * Configures automatic gain feature.
 *
 * Since: 0.2.0
 **/

void
arv_camera_set_gain_auto (ArvCamera *camera, ArvAuto auto_mode)
{
	arv_camera_set_string (camera, "GainAuto", arv_auto_to_string (auto_mode));
}

/**
 * arv_camera_get_gain_auto:
 * @camera: a #ArvCamera
 *
 * Returns: auto gain mode selection
 *
 * Since: 0.2.0
 **/

ArvAuto
arv_camera_get_gain_auto (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), ARV_AUTO_OFF);

	return arv_auto_from_string (arv_camera_get_string (camera, "GainAuto"));
}

/* Transport layer control */

/**
 * arv_camera_get_payload:
 * @camera: a #ArvCamera
 *
 * Retrieves the size needed for the storage of an image. This value is used
 * for the creation of the stream buffers.
 *
 * Returns: frame storage size, in bytes.
 *
 * Since: 0.2.0
 */

guint
arv_camera_get_payload (ArvCamera *camera)
{
	return arv_camera_get_integer (camera, "PayloadSize");
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
 *
 * Returns: %TRUE if FrameRate feature is available
 *
 * Since: 0.2.0
 */

gboolean
arv_camera_is_frame_rate_available (ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	switch (priv->vendor) {
		case ARV_CAMERA_VENDOR_PROSILICA:
			return arv_device_get_feature (priv->device, "AcquisitionFrameRateAbs") != NULL;
		case ARV_CAMERA_VENDOR_TIS:
			return arv_device_get_feature (priv->device, "FPS") != NULL;
		case ARV_CAMERA_VENDOR_POINT_GREY_FLIR:
		case ARV_CAMERA_VENDOR_DALSA:
		case ARV_CAMERA_VENDOR_RICOH:
		case ARV_CAMERA_VENDOR_BASLER:
	        case ARV_CAMERA_VENDOR_XIMEA:
		case ARV_CAMERA_VENDOR_MATRIX_VISION:
		case ARV_CAMERA_VENDOR_UNKNOWN:
			return arv_device_get_feature (priv->device,
						       priv->has_acquisition_frame_rate ?
						       "AcquisitionFrameRate":
						       "AcquisitionFrameRateAbs") != NULL;
	}

	return FALSE;
}
/**
 * arv_camera_is_exposure_time_available:
 * @camera: a #ArvCamera
 *
 * Returns: %TRUE if Exposure Time feature is available.
 *
 * Since: 0.2.0
 */

gboolean
arv_camera_is_exposure_time_available (ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	switch (priv->vendor) {
		case ARV_CAMERA_VENDOR_XIMEA:
			return arv_device_get_feature (priv->device, "ExposureTime") != NULL;
		case ARV_CAMERA_VENDOR_RICOH:
			return arv_device_get_feature (priv->device, "ExposureTimeRaw") != NULL;
		default:
			return arv_device_get_feature (priv->device,
						       priv->has_exposure_time ?
						       "ExposureTime" :
						       "ExposureTimeAbs") != NULL;
	}
}

/**
 * arv_camera_is_exposure_auto_available:
 * @camera: a #ArvCamera
 *
 * Returns: %TRUE if Exposure Auto feature is available.
 *
 * Since: 0.2.0
 */

gboolean
arv_camera_is_exposure_auto_available (ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	return arv_device_get_feature (priv->device, "ExposureAuto") != NULL;
}

/**
 * arv_camera_is_gain_available:
 * @camera: a #ArvCamera
 *
 * Returns: %TRUE if Gain feature is available.
 *
 * Since: 0.2.0
 */

gboolean
arv_camera_is_gain_available (ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	if (priv->has_gain)
		return arv_device_get_feature (priv->device, "Gain") != NULL;

	return arv_device_get_feature (priv->device, "GainRaw") != NULL;
}

/**
 * arv_camera_is_gain_auto_available:
 * @camera: a #ArvCamera
 *
 * Returns: %TRUE if Gain feature is available.
 *
 * Since: 0.2.0
 */

gboolean
arv_camera_is_gain_auto_available (ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	return arv_device_get_feature (priv->device, "GainAuto") != NULL;
}

/**
 * arv_camera_is_binning_available:
 * @camera: a #ArvCamera
 *
 * Returns: %TRUE if Binning feature is available.
 *
 * Since: 0.6.0
 */

gboolean
arv_camera_is_binning_available (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	return arv_camera_is_feature_available (camera, "BinningHorizontal") &&
		arv_camera_is_feature_available (camera, "BinningVertical");
}

static void
_update_status (ArvCamera *camera, const GError *error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	if (error == NULL)
		return;

	if (priv->error != NULL) {
		arv_warning_device ("[ArvCamera:_update_status] %s", error->message);
	} else {
		arv_warning_device ("[ArvCamera:_update_status] %s (Status changed)", error->message);

		g_clear_error (&priv->error);
		priv->error = g_error_copy (error);
	}
}

/**
 * arv_camera_execute_command:
 * @camera: a #ArvCamera
 * @feature: feature name
 *
 * Execute a Genicam command.
 *
 * Since: 0.8.0
 */

void
arv_camera_execute_command (ArvCamera *camera, const char *feature)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (feature != NULL);

	arv_device_execute_command (priv->device, feature, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}
}

/**
 * arv_camera_set_boolean:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @value: new feature value
 *
 * Set a boolean feature value.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_boolean (ArvCamera *camera, const char *feature, gboolean value)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (feature != NULL);

	arv_device_set_boolean_feature_value (priv->device, feature, value, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}
}

/**
 * arv_camera_get_boolean:
 * @camera: a #ArvCamera
 * @feature: feature name
 *
 * Returns: the boolean feature value, %FALSE on error.
 *
 * Since: 0.8.0
 */

gboolean
arv_camera_get_boolean (ArvCamera *camera, const char *feature)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;
	gboolean value;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);
	g_return_val_if_fail (feature != NULL, FALSE);

	value = arv_device_get_boolean_feature_value (priv->device, feature, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}

	return value;
}

/**
 * arv_camera_set_string:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @value: new feature value
 *
 * Set an string feature value.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_string (ArvCamera *camera, const char *feature, const char *value)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (feature != NULL);
	g_return_if_fail (value != NULL);

	arv_device_set_string_feature_value (priv->device, feature, value, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}
}

/**
 * arv_camera_get_string:
 * @camera: a #ArvCamera
 * @feature: feature name
 *
 * Returns: the string feature value, %NULL on error.
 *
 * Since: 0.8.0
 */

const char *
arv_camera_get_string (ArvCamera *camera, const char *feature)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;
	const char *value;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);
	g_return_val_if_fail (feature != NULL, FALSE);

	value = arv_device_get_string_feature_value (priv->device, feature, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}

	return value;
}

/**
 * arv_camera_set_integer:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @value: new feature value
 *
 * Set an integer feature value.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_integer (ArvCamera *camera, const char *feature, gint64 value)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (feature != NULL);

	arv_device_set_integer_feature_value (priv->device, feature, value, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}
}

/**
 * arv_camera_get_integer:
 * @camera: a #ArvCamera
 * @feature: feature name
 *
 * Returns: the integer feature value, 0 on error.
 *
 * Since: 0.8.0
 */

gint64
arv_camera_get_integer (ArvCamera *camera, const char *feature)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;
	gint64 value;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0);
	g_return_val_if_fail (feature != NULL, 0);

	value = arv_device_get_integer_feature_value (priv->device, feature, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}

	return value;
}

/**
 * arv_camera_get_integer_bounds:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @min: (out): minimum feature value
 * @max: (out): maximum feature value
 *
 * Retrieves integer feature bounds.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_integer_bounds (ArvCamera *camera, const char *feature, gint64 *min, gint64 *max)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (feature != NULL);

	arv_device_get_integer_feature_bounds (priv->device, feature, min, max, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}
}

/**
 * arv_camera_get_integer_increment:
 * @camera: a #ArvCamera
 * @feature: feature name
 *
 * Returns: @feature value increment.
 *
 * Since: 0.8.0
 */

gint64
arv_camera_get_integer_increment (ArvCamera *camera, const char *feature)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;
	gint64 value = 1;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 1);
	g_return_val_if_fail (feature != NULL, 1);

	value = arv_device_get_integer_feature_increment (priv->device, feature, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}

	return value;
}

static void
arv_camera_get_integer_bounds_as_gint (ArvCamera *camera, const char *feature, gint *min, gint *max)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;
	gint64 min64, max64;

	if (min != NULL)
		*min = G_MININT;
	if (max != NULL)
		*max = G_MAXINT;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (feature != NULL);

	arv_device_get_integer_feature_bounds (priv->device, feature, &min64, &max64, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	} else {
		if (min != NULL)
			*min = CLAMP (min64, G_MININT, G_MAXINT);
		if (max != NULL)
			*max = CLAMP (max64, G_MININT, G_MAXINT);
	}
}

static void
arv_camera_get_integer_bounds_as_guint (ArvCamera *camera, const char *feature, guint *min, guint *max)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;
	gint64 min64, max64;

	if (min != NULL)
		*min = 0;
	if (max != NULL)
		*max = G_MAXUINT;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (feature != NULL);

	arv_device_get_integer_feature_bounds (priv->device, feature, &min64, &max64, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	} else {
		if (min != NULL)
			*min = CLAMP (min64, 0, G_MAXUINT);
		if (max != NULL)
			*max = CLAMP (max64, 0, G_MAXUINT);
	}
}

static void
arv_camera_get_integer_bounds_as_double (ArvCamera *camera, const char *feature, double *min, double *max)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;
	gint64 min64, max64;

	if (min != NULL)
		*min = G_MININT64;
	if (max != NULL)
		*max = G_MAXINT64;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (feature != NULL);

	arv_device_get_integer_feature_bounds (priv->device, feature, &min64, &max64, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
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
 *
 * Set a float feature value.
 *
 * Since: 0.8.0
 */

void
arv_camera_set_float (ArvCamera *camera, const char *feature, double value)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (feature != NULL);

	arv_device_set_float_feature_value (priv->device, feature, value, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}
}

/**
 * arv_camera_get_float:
 * @camera: a #ArvCamera
 * @feature: feature name
 *
 * Returns: the float feature value, 0.0 on error.
 *
 * Since: 0.8.0
 */

double
arv_camera_get_float (ArvCamera *camera, const char *feature)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;
	double value;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0.0);
	g_return_val_if_fail (feature != NULL, 0.0);

	value = arv_device_get_float_feature_value (priv->device, feature, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}

	return value;
}

/**
 * arv_camera_get_float_bounds:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @min: (out): minimum feature value
 * @max: (out): maximum feature value
 *
 * Retrieves float feature bounds.
 *
 * Since: 0.8.0
 */

void
arv_camera_get_float_bounds (ArvCamera *camera, const char *feature, double *min, double *max)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;

	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (feature != NULL);

	arv_device_get_float_feature_bounds (priv->device, feature, min, max, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}
}

/**
 * arv_camera_get_available_enumerations:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @n_values: placeholder for the number of returned values
 *
 * Get all the available values of @feature, as 64 bit integers.
 *
 * Returns: (array length=n_values) (transfer container): a newly created array of integers, which must freed after use using g_free, or
 * NULL on error.
 *
 * Since: 0.8.0
 */

gint64 *
arv_camera_get_available_enumerations (ArvCamera *camera, const char *feature, guint *n_values)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;
	gint64 *values;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);
	g_return_val_if_fail (feature != NULL, NULL);

	values = arv_device_get_available_enumeration_feature_values (priv->device, feature, n_values, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}

	return values;
}

/**
 * arv_camera_get_available_enumerations_as_strings:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @n_values: placeholder for the number of returned values
 *
 * Get all the available values of @feature, as strings.
 *
 * Returns: (array length=n_values) (transfer container): a newly created array of const strings, which must freed after use using g_free,
 * or %NULL on error.
 *
 * Since: 0.8.0
 */

const char **
arv_camera_get_available_enumerations_as_strings (ArvCamera *camera, const char *feature, guint *n_values)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;
	const char **strings;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);
	g_return_val_if_fail (feature != NULL, NULL);

	strings = arv_device_get_available_enumeration_feature_values_as_strings (priv->device,
										  feature, n_values, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}

	return strings;
}

/**
 * arv_camera_get_available_enumerations_as_display_names:
 * @camera: a #ArvCamera
 * @feature: feature name
 * @n_values: placeholder for the number of returned values
 *
 * Get display names of all the available entries of @feature.
 *
 * Returns: (array length=n_values) (transfer container): a newly created array of const strings, to be freed after use using g_free, or
 * %NULL on error.
 *
 * Since: 0.8.0
 */

const char **
arv_camera_get_available_enumerations_as_display_names (ArvCamera *camera, const char *feature, guint *n_values)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	GError *error = NULL;
	const char **strings;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);
	g_return_val_if_fail (feature != NULL, NULL);

	strings = arv_device_get_available_enumeration_feature_values_as_display_names (priv->device,
											feature, n_values, &error);
	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}

	return strings;
}

/**
 * arv_camera_is_feature_available:
 * @camera: a #ArvCamera
 * @feature: feature name
 *
 * Return: %TRUE if feature is available, %FALSE if not or on error.
 *
 * Since: 0.8.0
 */

gboolean
arv_camera_is_feature_available (ArvCamera *camera, const char *feature)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);
	ArvGcNode* node;
	GError *error = NULL;
	gboolean available = FALSE;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);
	g_return_val_if_fail (feature != NULL, FALSE);

	node = arv_device_get_feature (priv->device, feature);
	available = ARV_IS_GC_FEATURE_NODE (node) && arv_gc_feature_node_is_available (ARV_GC_FEATURE_NODE (node), &error);

	if (error != NULL) {
		_update_status (camera, error);
		g_clear_error (&error);
	}

	return available;
}

ArvStatus
arv_camera_get_status (ArvCamera *camera, GError **error)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (ARV_IS_CAMERA (camera), ARV_STATUS_UNKNOWN);

	if (priv->error == NULL)
		return ARV_STATUS_SUCCESS;

	g_propagate_error (error, priv->error);
	priv->error = NULL;

	return ARV_STATUS_PROTOCOL_ERROR;
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
 * arv_camera_gv_get_n_stream_channels:
 * @camera: a #ArvCamera
 *
 * Returns: the number of supported stream channels.
 *
 * Since: 0.4.0
 */

gint
arv_camera_gv_get_n_stream_channels (ArvCamera *camera)
{
	g_return_val_if_fail (arv_camera_is_gv_device (camera), 0);

	return arv_camera_get_integer (camera, "GevStreamChannelCount");
}

/**
 * arv_camera_gv_select_stream_channel:
 * @camera: a #ArvCamera
 * @channel_id: id of the channel to select
 *
 * Select the current stream channel.
 *
 * Since: 0.4.0
 */

void
arv_camera_gv_select_stream_channel (ArvCamera *camera, gint channel_id)
{
	if (channel_id < 0)
		return;

	g_return_if_fail (arv_camera_is_gv_device (camera));

	arv_camera_set_integer (camera, "GevStreamChannelSelector", channel_id);
}

/**
 * arv_camera_gv_get_current_stream_channel:
 * @camera: a #ArvCamera
 *
 * Returns: The current stream channel id.
 *
 * Since: 0.4.0
 */

int
arv_camera_gv_get_current_stream_channel (ArvCamera *camera)
{
	g_return_val_if_fail (arv_camera_is_gv_device (camera), 0);

	return arv_camera_get_integer (camera, "GevStreamChannelSelector");
}

/**
 * arv_camera_gv_set_packet_delay:
 * @camera: a #ArvCamera
 * @delay_ns: inter packet delay, in nanoseconds
 *
 * Configure the inter packet delay to insert between each packet for the current stream
 * channel. This can be used as a crude flow-control mechanism if the application or the network
 * infrastructure cannot keep up with the packets coming from the device.
 *
 * Since: 0.4.0
 */

void
arv_camera_gv_set_packet_delay (ArvCamera *camera, gint64 delay_ns)
{
	gint64 tick_frequency;
	gint64 value;

	if (delay_ns < 0)
		return;

	g_return_if_fail (arv_camera_is_gv_device (camera));

	tick_frequency = arv_camera_get_integer (camera, "GevTimestampTickFrequency");
	if (tick_frequency <= 0)
		return;

	value = tick_frequency * delay_ns / 1000000000LL;
	arv_camera_set_integer (camera, "GevSCPD", value);
}

/**
 * arv_camera_gv_get_packet_delay:
 * @camera: a #ArvCamera
 *
 * Returns: The inter packet delay, in nanoseconds.
 *
 * Since: 0.4.0
 */

gint64
arv_camera_gv_get_packet_delay (ArvCamera *camera)
{
	gint64 tick_frequency;
	gint64 value;

	g_return_val_if_fail (arv_camera_is_gv_device (camera), 0);

	tick_frequency = arv_camera_get_integer (camera, "GevTimestampTickFrequency");
	if (tick_frequency <= 0)
		return 0;

	value = arv_camera_get_integer (camera, "GevSCPD");

	return value * 1000000000LL / tick_frequency;
}

/**
 * arv_camera_gv_set_packet_size:
 * @camera: a #ArvCamera
 * @packet_size: packet size, in bytes
 *
 * Specifies the stream packet size, in bytes, to send on the selected channel for a GVSP transmitter
 * or specifies the maximum packet size supported by a GVSP receiver.
 *
 * This does not include data leader and data trailer and the last data packet which might be of
 * smaller size (since packet size is not necessarily a multiple of block size for stream channel).
 *
 * Since: 0.4.0
 */

void
arv_camera_gv_set_packet_size (ArvCamera *camera, gint packet_size)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	if (packet_size <= 0)
		return;

	g_return_if_fail (arv_camera_is_gv_device (camera));

	arv_gv_device_set_packet_size (ARV_GV_DEVICE (priv->device), packet_size);
}

/**
 * arv_camera_gv_get_packet_size:
 * @camera: a #ArvCamera
 *
 * Returns: The stream packet size, in bytes.
 *
 * Since: 0.4.0
 */

guint
arv_camera_gv_get_packet_size (ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (arv_camera_is_gv_device (camera), 0);

	return arv_gv_device_get_packet_size (ARV_GV_DEVICE (priv->device));
}

/**
 * arv_camera_gv_auto_packet_size:
 * @camera: a #ArvCamera
 *
 * Automatically determine the biggest packet size that can be used data
 * streaming, and set GevSCPSPacketSize value accordingly. This function relies
 * on the GevSCPSFireTestPacket feature. If this feature is not available, the
 * packet size will be set to a default value (1500 bytes).
 *
 * Returns: The packet size, in bytes.
 *
 * Since: 0.6.0
 */

guint
arv_camera_gv_auto_packet_size (ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (arv_camera_is_gv_device (camera), 0);

	return arv_gv_device_auto_packet_size (ARV_GV_DEVICE (priv->device));
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
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

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
 *
 * Returns: wether bandwidth limits are available on this camera
 *
 * Since: 0.6.0
 */

gboolean
arv_camera_uv_is_bandwidth_control_available (ArvCamera *camera)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (camera);

	g_return_val_if_fail (arv_camera_is_uv_device (camera), FALSE);

	switch (priv->vendor) {
		case ARV_CAMERA_VENDOR_XIMEA:
			return arv_device_get_feature(priv->device, "DeviceLinkThroughputLimit") != NULL;
		default:
			return FALSE;
	}
}


/**
 * arv_camera_uv_set_bandwidth:
 * @camera: a #ArvCamera
 * @bandwidth: Desired bandwith limit in megabits/sec. Set to 0 to disable limit mode.
 *
 * Since: 0.6.0
 */

void
arv_camera_uv_set_bandwidth (ArvCamera *camera, guint bandwidth)
{
	g_return_if_fail (arv_camera_is_uv_device (camera));

	if (bandwidth > 0) {
		arv_camera_set_integer (camera, "DeviceLinkThroughputLimit", bandwidth);
		arv_camera_set_integer (camera, "DeviceLinkThroughputLimitMode", 1);
	} else {
		arv_camera_set_integer (camera, "DeviceLinkThroughputLimitMode", 0);
	}

}

/**
 * arv_camera_uv_get_bandwidth:
 * @camera: a #ArvCamera
 *
 * Returns: the current bandwidth limit
 *
 * Since: 0.6.0
 */

guint
arv_camera_uv_get_bandwidth (ArvCamera *camera)
{
	g_return_val_if_fail (arv_camera_is_uv_device (camera), 0);

	return arv_camera_get_integer (camera, "DeviceLinkThroughputLimit");
}

/**
 * arv_camera_uv_get_bandwidth_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum bandwidth
 * @max: (out): maximum bandwidth
 *
 * Since: 0.6.0
 */

void
arv_camera_uv_get_bandwidth_bounds (ArvCamera *camera, guint *min, guint *max)
{
	arv_camera_get_integer_bounds_as_guint (camera, "DeviceLinkThroughputLimit", min, max);
}

/**
 * arv_camera_set_chunk_mode:
 * @camera: a #ArvCamera
 * @is_active: wether to enable chunk data mode
 *
 * Controls wether chunk data mode is active. When active, chunk data
 * are appended to image data in #ArvBuffer. A #ArvChunkParser must be used in
 * order to extract chunk data.
 *
 * Since: 0.4.0
 **/

void
arv_camera_set_chunk_mode (ArvCamera *camera, gboolean is_active)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_camera_set_integer (camera, "ChunkModeActive", is_active ? 1 : 0);
}

/**
 * arv_camera_get_chunk_mode:
 * @camera: a #ArvCamera
 *
 * Check wether chunk data mode is active. Please see arv_camera_set_chunk_mode().
 *
 * Returns: %TRUE if chunk data mode is active.
 *
 * Since: 0.4.0
 **/

gboolean
arv_camera_get_chunk_mode (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);

	return arv_camera_get_integer (camera, "ChunkModeActive");
}

/**
 * arv_camera_set_chunk_state:
 * @camera: a #ArvCamera
 * @chunk: chunk data name
 * @is_enabled: wether to enable this chunk
 *
 * Sets state of a chunk data. Chunk data are be embedded in #ArvBuffer only
 * if chunk mode is active. Please see arv_camera_set_chunk_mode().
 *
 * Since: 0.4.0
 **/

void
arv_camera_set_chunk_state (ArvCamera *camera, const char *chunk, gboolean is_enabled)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (chunk != NULL && chunk[0] != '\0');

	arv_camera_set_string (camera, "ChunkSelector", chunk);
	arv_camera_set_integer (camera, "ChunkEnable", is_enabled ? 1 : 0);
}

/**
 * arv_camera_get_chunk_state:
 * @camera: a #ArvCamera
 * @chunk: chunk data name
 *
 * Gets state of chunk data. Chunk data are be embedded in #ArvBuffer only
 * if chunk mode is active. Please see arv_camera_set_chunk_mode().
 *
 * Returns: %TRUE if @chunk is enabled.
 *
 * Since: 0.4.0
 */

gboolean
arv_camera_get_chunk_state (ArvCamera *camera, const char *chunk)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), FALSE);
	g_return_val_if_fail (chunk != NULL && chunk[0] != '\0', FALSE);

	arv_camera_set_string (camera, "ChunkSelector", chunk);
	return arv_camera_get_integer (camera, "ChunkEnable");
}

/**
 * arv_camera_set_chunks:
 * @camera: a #ArvCamera
 * @chunk_list: chunk data names, as a comma or space separated list
 *
 * Convenience function for enabling a set of chunk data. Chunk mode is activated, or deactivated
 * if @chunk_list is %NULL or empty. All chunk data not listed are disabled.
 *
 * Since: 0.4.0
 */

void
arv_camera_set_chunks (ArvCamera *camera, const char *chunk_list)
{
	const char **available_chunks;
	char **chunks;
	char *striped_chunk_list;
	gboolean enable_chunk_data = FALSE;
	guint i;
	guint n_values;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (chunk_list == NULL) {
		arv_camera_set_chunk_mode (camera, FALSE);
		return;
	}

	available_chunks = arv_camera_get_available_enumerations_as_strings (camera, "ChunkSelector", &n_values);
	for (i = 0; i < n_values; i++) {
		arv_camera_set_chunk_state (camera, available_chunks[i], FALSE);
	}
	g_free (available_chunks);

	striped_chunk_list = g_strdup (chunk_list);
	arv_str_strip (striped_chunk_list, " ,:;", ',');
	chunks = g_strsplit_set (striped_chunk_list, " ,:;", -1);
	g_free (striped_chunk_list);

	for (i = 0; chunks[i] != NULL; i++) {
		arv_camera_set_chunk_state (camera, chunks[i], TRUE);
		enable_chunk_data = TRUE;
	}

	g_strfreev (chunks);

	arv_camera_set_chunk_mode (camera, enable_chunk_data);
}

/**
 * arv_camera_create_chunk_parser:
 * @camera: a #ArvCamera
 *
 * Creates a new #ArvChunkParser object, used for the extraction of chunk data from #ArvBuffer.
 *
 * Returns: (transfer full): a new #ArvChunkParser.
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
 *
 * Creates a new #ArvCamera. If @name is null, it will instantiate the
 * first available camera.
 *
 * Returns: a new #ArvCamera.
 *
 * Since: 0.2.0
 */

ArvCamera *
arv_camera_new (const char *name)
{
	ArvCamera *camera;
	ArvDevice *device;

	device = arv_open_device (name);

	if (!ARV_IS_DEVICE (device))
		return NULL;

	camera = g_object_new (ARV_TYPE_CAMERA, "device", device, NULL);

	/* if you need to apply or test for fixups based on the camera model
	   please do so in arv_camera_constructor and not here, as this breaks
	   objects created with g_object_new, which includes but is not limited to
	   introspection users */

	return camera;
}

static void
arv_camera_init (ArvCamera *camera)
{
}

static void
arv_camera_finalize (GObject *object)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (ARV_CAMERA (object));

	g_clear_object (&priv->device);
	g_clear_error (&priv->error);

	G_OBJECT_CLASS (arv_camera_parent_class)->finalize (object);
}

static GObject *
arv_camera_constructor (GType gtype, guint n_properties, GObjectConstructParam *properties)
{
	ArvCameraPrivate *priv;
	GObject *object;
	ArvCamera *camera;
	ArvCameraVendor vendor;
	ArvCameraSeries series;
	const char *vendor_name;
	const char *model_name;

	object = G_OBJECT_CLASS (arv_camera_parent_class)->constructor (gtype, n_properties, properties);

	camera = ARV_CAMERA (object);
	priv = arv_camera_get_instance_private (camera);

	if (!priv->device)
		priv->device = arv_open_device (NULL);

	if (!ARV_IS_DEVICE (priv->device))
		return NULL;

	priv->genicam = arv_device_get_genicam (priv->device);

	vendor_name = arv_camera_get_vendor_name (camera);
	model_name = arv_camera_get_model_name (camera);

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
	} else {
		vendor = ARV_CAMERA_VENDOR_UNKNOWN;
		series = ARV_CAMERA_SERIES_UNKNOWN;
	}

	priv->vendor = vendor;
	priv->series = series;

	priv->has_gain = ARV_IS_GC_FLOAT (arv_device_get_feature (priv->device, "Gain"));
	priv->has_exposure_time = ARV_IS_GC_FLOAT (arv_device_get_feature (priv->device, "ExposureTime"));
	priv->has_acquisition_frame_rate = ARV_IS_GC_FLOAT (arv_device_get_feature (priv->device,
										    "AcquisitionFrameRate"));
	priv->has_acquisition_frame_rate_enabled = ARV_IS_GC_INTEGER (arv_device_get_feature (priv->device,
											      "AcquisitionFrameRateEnabled"));

    return object;
}

static void
arv_camera_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ArvCameraPrivate *priv = arv_camera_get_instance_private (ARV_CAMERA (object));

	switch (prop_id)
	{
		case PROP_CAMERA_DEVICE:
			priv->device = g_value_get_object (value);
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
	object_class->constructor = arv_camera_constructor;
	object_class->set_property = arv_camera_set_property;
	object_class->get_property = arv_camera_get_property;

	g_object_class_install_property (object_class,
					 PROP_CAMERA_DEVICE,
					 g_param_spec_object ("device",
							      "device",
							      "the device associated with this camera",
							      ARV_TYPE_DEVICE,
							      G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}
