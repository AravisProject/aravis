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
 * SECTION:arvcamera
 * @short_description: Class for generic camera control
 *
 * #ArvCamera is a class for the generic control of cameras. It hides the
 * complexity of the genicam interface by providing a simple API, with the
 * drawback of not exposing all the available features. See #ArvDevice and
 * #ArvGc for a more advanced use of the Aravis library.
 */

#include <arvcamera.h>
#include <arvsystem.h>
#include <arvgvinterface.h>
#include <arvgccommand.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcenumeration.h>
#include <arvgcenumentry.h>
#include <arvgcstring.h>
#include <arvgc.h>
#include <arvdevice.h>
#include <arvenums.h>

/**
 * ArvCameraVendor:
 * @ARV_CAMERA_VENDOR_UNKNOWN: unknown camera vendor
 * @ARV_CAMERA_VENDOR_BASLER: Basler
 * @ARV_CAMERA_VENDOR_PROSILICA: Prosilica
 */

typedef enum {
	ARV_CAMERA_VENDOR_UNKNOWN,
	ARV_CAMERA_VENDOR_BASLER,
	ARV_CAMERA_VENDOR_PROSILICA
} ArvCameraVendor;

typedef enum {
	ARV_CAMERA_SERIES_UNKNOWN,
	ARV_CAMERA_SERIES_BASLER_ACE,
	ARV_CAMERA_SERIES_BASLER_SCOUT,
	ARV_CAMERA_SERIES_BASLER_OTHER,
	ARV_CAMERA_SERIES_PROSILICA_OTHER
} ArvCameraSeries;

static GObjectClass *parent_class = NULL;

struct _ArvCameraPrivate {
	ArvDevice *device;
	ArvGc *genicam;

	ArvCameraVendor vendor;
	ArvCameraSeries series;
};

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
 */

ArvStream *
arv_camera_create_stream (ArvCamera *camera, ArvStreamCallback callback, gpointer user_data)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_create_stream (camera->priv->device, callback, user_data);
}

/* Device control */

/**
 * arv_camera_get_vendor_name:
 * @camera: a #ArvCamera
 *
 * Returns: the camera vendor name.
 */

const char *
arv_camera_get_vendor_name (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_get_string_feature_value (camera->priv->device, "DeviceVendorName");
}

/**
 * arv_camera_get_model_name:
 * @camera: a #ArvCamera
 *
 * Returns: the camera model name.
 */

const char *
arv_camera_get_model_name (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_get_string_feature_value (camera->priv->device, "DeviceModelName");
}

/**
 * arv_camera_get_device_id:
 * @camera: a #ArvCamera
 *
 * Returns: the camera device ID.
 */

const char *
arv_camera_get_device_id (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_get_string_feature_value (camera->priv->device, "DeviceID");
}

/* Image format control */

/**
 * arv_camera_get_sensor_size:
 * @camera: a #ArvCamera
 * @width: (out): camera sensor width
 * @height: (out): camera sensor height
 */

void
arv_camera_get_sensor_size (ArvCamera *camera, gint *width, gint *height)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (width != NULL)
		*width = arv_device_get_integer_feature_value (camera->priv->device, "SensorWidth");
	if (height != NULL)
		*height = arv_device_get_integer_feature_value (camera->priv->device, "SensorHeight");
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
 */

void
arv_camera_set_region (ArvCamera *camera, gint x, gint y, gint width, gint height)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (x >= 0)
		arv_device_set_integer_feature_value (camera->priv->device, "OffsetX", x);
	if (y >= 0)
		arv_device_set_integer_feature_value (camera->priv->device, "OffsetY", y);
	if (width > 0)
		arv_device_set_integer_feature_value (camera->priv->device, "Width", width);
	if (height > 0)
		arv_device_set_integer_feature_value (camera->priv->device, "Height", height);
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
 */

void
arv_camera_get_region (ArvCamera *camera, gint *x, gint *y, gint *width, gint *height)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (x != NULL)
		*x = arv_device_get_integer_feature_value (camera->priv->device, "OffsetX");
	if (y != NULL)
		*y = arv_device_get_integer_feature_value (camera->priv->device, "OffsetY");
	if (width != NULL)
		*width = arv_device_get_integer_feature_value (camera->priv->device, "Width");
	if (height != NULL)
		*height = arv_device_get_integer_feature_value (camera->priv->device, "Height");
}

/**
 * arv_camera_get_width_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum width
 * @max: (out): maximum width
 *
 * Retrieves the valid range for image width.
 */

void
arv_camera_get_width_bounds (ArvCamera *camera, gint *min, gint *max)
{
	gint64 min64, max64;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_get_integer_feature_bounds (camera->priv->device, "Width", &min64, &max64);

	if (min != NULL)
		*min = min64;
	if (max != NULL)
		*max = max64;
}

/**
 * arv_camera_get_height_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum height
 * @max: (out): maximum height
 *
 * Retrieves the valid range for image height.
 */

void
arv_camera_get_height_bounds (ArvCamera *camera, gint *min, gint *max)
{
	gint64 min64, max64;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_get_integer_feature_bounds (camera->priv->device, "Height", &min64, &max64);

	if (min != NULL)
		*min = min64;
	if (max != NULL)
		*max = max64;
}

/**
 * arv_camera_set_binning:
 * @camera: a #ArvCamera
 * @dx: horizontal binning
 * @dy: vertical binning
 *
 * Defines binning in both directions. Not all cameras support this
 * feature.
 */

void
arv_camera_set_binning (ArvCamera *camera, gint dx, gint dy)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (dx > 0)
		arv_device_set_integer_feature_value (camera->priv->device, "BinningHorizontal", dx);
	if (dy > 0)
		arv_device_set_integer_feature_value (camera->priv->device, "BinningVertical", dy);
}

/**
 * arv_camera_get_binning:
 * @camera: a #ArvCamera
 * @dx: (out): horizontal binning placeholder
 * @dy: (out): vertical binning placeholder
 *
 * Retrieves binning in both directions.
 */

void
arv_camera_get_binning (ArvCamera *camera, gint *dx, gint *dy)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (dx != NULL)
		*dx = arv_device_get_integer_feature_value (camera->priv->device, "BinningHorizontal");
	if (dy != NULL)
		*dy = arv_device_get_integer_feature_value (camera->priv->device, "BinningVertical");
}

/**
 * arv_camera_set_pixel_format:
 * @camera: a #ArvCamera
 * @format: pixel format
 *
 * Defines pixel format.
 */

void
arv_camera_set_pixel_format (ArvCamera *camera, ArvPixelFormat format)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_integer_feature_value (camera->priv->device, "PixelFormat", format);
}

/**
 * arv_camera_get_pixel_format:
 * @camera: a #ArvCamera
 *
 * Returns: pixel format.
 */

ArvPixelFormat
arv_camera_get_pixel_format (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0);

	return arv_device_get_integer_feature_value (camera->priv->device, "PixelFormat");
}

/**
 * arv_camera_get_pixel_format_as_string:
 * @camera: a #ArvCamera
 *
 * Retuns: pixel format as string, NULL on error.
 */

const char *
arv_camera_get_pixel_format_as_string (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_get_string_feature_value (camera->priv->device, "PixelFormat");
}

/**
 * arv_camera_get_available_pixel_formats:
 * @camera: a #ArvCamera
 * @n_pixel_formats: (out): number of different pixel formats
 *
 * Retrieves the list of all available pixel formats.
 *
 * Returns: (array length=n_pixel_formats) (transfer full): a newly allocated array of #ArvPixelFormat
 */

gint64 *
arv_camera_get_available_pixel_formats (ArvCamera *camera, guint *n_pixel_formats)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_get_enumeration_feature_available_values (camera->priv->device, "PixelFormat", n_pixel_formats);
}

/* Acquisition control */

/**
 * arv_camera_start_acquisition:
 * @camera: a #ArvCamera
 *
 * Starts video stream acquisition.
 */

void
arv_camera_start_acquisition (ArvCamera *camera)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_execute_command (camera->priv->device, "AcquisitionStart");
}

/**
 * arv_camera_stop_acquisition:
 * @camera: a #ArvCamera
 *
 * Stops video stream acquisition.
 */

void
arv_camera_stop_acquisition (ArvCamera *camera)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_execute_command (camera->priv->device, "AcquisitionStop");
}

/*
 * arv_camera_set_acquisition_mode:
 * @camera: a #ArvCamera
 * @acquisition_mode: acquisition mode
 *
 * Defines acquisition mode.
 */

void
arv_camera_set_acquisition_mode (ArvCamera *camera, ArvAcquisitionMode acquisition_mode)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_string_feature_value (camera->priv->device, "AcquisitionMode",
					     arv_acquisition_mode_to_string (acquisition_mode));
}

/**
 * arv_camera_get_acquisition_mode:
 * @camera: a #ArvCamera
 *
 * Returns: acquisition mode.
 */

ArvAcquisitionMode
arv_camera_get_acquisition_mode (ArvCamera *camera)
{
	const char *string;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0);

	string = arv_device_get_string_feature_value (camera->priv->device, "AcquisitionMode");

	return arv_acquisition_mode_from_string (string);
}

/**
 * arv_camera_set_frame_rate: 
 * @camera: a #ArvCamera
 * @frame_rate: frame rate, in Hz
 *
 * Configures a fixed frame rate mode. Once acquisition start is triggered, the
 * video stream will be acquired with the given frame rate.
 */

void
arv_camera_set_frame_rate (ArvCamera *camera, double frame_rate)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (frame_rate <= 0.0)
		return;

	switch (camera->priv->vendor) {
		case ARV_CAMERA_VENDOR_BASLER:
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector",
							     "AcquisitionStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "Off");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector",
							     "FrameStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "Off");
			arv_device_set_integer_feature_value (camera->priv->device, "AcquisitionFrameRateEnable",
							      1);
			arv_device_set_float_feature_value (camera->priv->device, "AcquisitionFrameRateAbs",
							    frame_rate);
			break;
		case ARV_CAMERA_VENDOR_PROSILICA:
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector",
							     "FrameStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "Off");
			arv_device_set_float_feature_value (camera->priv->device, "AcquisitionFrameRateAbs",
							    frame_rate);
			break;
		case ARV_CAMERA_VENDOR_UNKNOWN:
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector",
							     "FrameStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "Off");
			arv_device_set_float_feature_value (camera->priv->device, "AcquisitionFrameRate", frame_rate);
			break;
	}
}

/**
 * arv_camera_get_frame_rate:
 * @camera: a #ArvCamera
 *
 * Returns: actual frame rate, in Hz.
 */

double
arv_camera_get_frame_rate (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), -1);

	switch (camera->priv->vendor) {
		case ARV_CAMERA_VENDOR_BASLER:
		case ARV_CAMERA_VENDOR_PROSILICA:
			return arv_device_get_float_feature_value (camera->priv->device, "AcquisitionFrameRateAbs");
		case ARV_CAMERA_VENDOR_UNKNOWN:
		default:
			return arv_device_get_float_feature_value (camera->priv->device, "AcquisitionFrameRate");
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
 */

void
arv_camera_set_trigger (ArvCamera *camera, const char *source)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (source != NULL);

	switch (camera->priv->vendor) {
		case ARV_CAMERA_VENDOR_BASLER:
			arv_device_set_integer_feature_value (camera->priv->device, "AcquisitionFrameRateEnable",
							      0);
		default:
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector",
							     "AcquisitionStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "Off");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector",
							     "FrameStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "On");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerActivation",
							     "RisingEdge");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSource", source);
			break;
	}
}

/**
 * arv_camera_set_trigger_source:
 * @camera: a #ArvCamera
 * @source: source name
 *
 * Sets the trigger source. This function doesn't check if the camera is configured
 * to actually use this source as a trigger.
 */

void
arv_camera_set_trigger_source (ArvCamera *camera, const char *source)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));
	g_return_if_fail (source != NULL);

	arv_device_set_string_feature_value (camera->priv->device, "TriggerSource", source);
}

/**
 * arv_camera_get_trigger_source:
 * @camera: a #ArvCamera
 *
 * Gets the trigger source. This function doesn't check if the camera is configured
 * to actually use this source as a trigger.
 *
 * Returns: a string containing the trigger source name, NULL on error.
 */

const char *
arv_camera_get_trigger_source (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_get_string_feature_value (camera->priv->device, "TriggerSource");
}

/**
 * arv_camera_software_trigger:
 * @camera: a #ArvCamera
 *
 * Sends a software trigger command to @camera. The camera must be previously
 * configured to use a software trigger, using @arv_camera_set_trigger().
 */

void
arv_camera_software_trigger (ArvCamera *camera)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_execute_command (camera->priv->device, "TriggerSoftware");
}

/**
 * arv_camera_set_exposure_time:
 * @camera: a #ArvCamera
 * @exposure_time_us: exposure time, in µs
 *
 * Sets exposure time. User should take care to set a value compatible with
 * the desired frame rate.
 */

void
arv_camera_set_exposure_time (ArvCamera *camera, double exposure_time_us)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (exposure_time_us <= 0)
		return;

	switch (camera->priv->series) {
		case ARV_CAMERA_SERIES_BASLER_SCOUT:
			arv_device_set_float_feature_value (camera->priv->device, "ExposureTimeBaseAbs",
							    exposure_time_us);
			arv_device_set_integer_feature_value (camera->priv->device, "ExposureTimeRaw", 1);
			break;
		case ARV_CAMERA_SERIES_BASLER_ACE:
		default:
			arv_device_set_float_feature_value (camera->priv->device, "ExposureTimeAbs", exposure_time_us);
			break;
	}
}

/**
 * arv_camera_get_exposure_time:
 * @camera: a #ArvCamera
 *
 * Returns: current exposure time, in µs.
 */

double
arv_camera_get_exposure_time (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0.0);

	return arv_device_get_float_feature_value (camera->priv->device, "ExposureTimeAbs");
}

/**
 * arv_camera_get_exposure_time_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum exposure time
 * @max: (out): maximum exposure time
 *
 * Retrieves exposure time bounds, in µs.
 */

void
arv_camera_get_exposure_time_bounds (ArvCamera *camera, double *min, double *max)
{
	gint64 int_min, int_max;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	switch (camera->priv->series) {
		case ARV_CAMERA_SERIES_BASLER_SCOUT:
			arv_device_get_float_feature_bounds (camera->priv->device, "ExposureTimeBaseAbs", min, max);
			break;
		case ARV_CAMERA_SERIES_BASLER_ACE:
			arv_device_get_integer_feature_bounds (camera->priv->device, "ExposureTimeRaw",
							       &int_min,
							       &int_max);
			if (min != NULL)
				*min = int_min;
			if (max != NULL)
				*max = int_max;
			break;
		default:
			arv_device_get_float_feature_bounds (camera->priv->device, "ExposureTimeAbs", min, max);
			break;
	}
}

/**
 * arv_camera_set_exposure_time_auto:
 * @camera: a #ArvCamera
 * @auto_mode: auto exposure mode selection
 *
 * Configures automatic exposure feature.
 **/

void
arv_camera_set_exposure_time_auto (ArvCamera *camera, ArvAuto auto_mode)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_string_feature_value (camera->priv->device, "ExposureAuto", arv_auto_to_string (auto_mode));
}

/**
 * arv_camera_get_exposure_time_auto:
 * @camera: a #ArvCamera
 *
 * Returns: auto exposure mode selection
 **/

ArvAuto
arv_camera_get_exposure_time_auto (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), ARV_AUTO_OFF);

	return arv_auto_from_string (arv_device_get_string_feature_value (camera->priv->device, "ExposureAuto"));
}

/* Analog control */

/**
 * arv_camera_set_gain:
 * @camera: a #ArvCamera
 * @gain: gain value
 *
 * Sets the gain of the ADC converter.
 */

void
arv_camera_set_gain (ArvCamera *camera, gint gain)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (gain < 0)
		return;

	arv_device_set_integer_feature_value (camera->priv->device, "GainRaw", gain);
}

/**
 * arv_camera_get_gain:
 * @camera: a #ArvCamera
 *
 * Returns: the current gain setting.
 */

gint
arv_camera_get_gain (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0.0);

	return arv_device_get_integer_feature_value (camera->priv->device, "GainRaw");
}

/**
 * arv_camera_get_gain_bounds:
 * @camera: a #ArvCamera
 * @min: (out): minimum gain
 * @max: (out): maximum gain
 *
 * Retrieves gain bounds.
 */

void
arv_camera_get_gain_bounds (ArvCamera *camera, gint *min, gint *max)
{
	gint64 min64, max64;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_get_integer_feature_bounds (camera->priv->device, "GainRaw", &min64, &max64);

	if (min != NULL)
		*min = min64;
	if (max != NULL)
		*max = max64;
}

/**
 * arv_camera_set_gain_auto:
 * @camera: a #ArvCamera
 * @auto_mode: auto gaine mode selection
 *
 * Configures automatic gain feature.
 **/

void
arv_camera_set_gain_auto (ArvCamera *camera, ArvAuto auto_mode)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_string_feature_value (camera->priv->device, "GainAuto", arv_auto_to_string (auto_mode));
}

/**
 * arv_camera_get_gain_auto:
 * @camera: a #ArvCamera
 *
 * Returns: auto gain mode selection
 **/

ArvAuto
arv_camera_get_gain_auto (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), ARV_AUTO_OFF);

	return arv_auto_from_string (arv_device_get_string_feature_value (camera->priv->device, "GainAuto"));
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
 */

guint
arv_camera_get_payload (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0);

	return arv_device_get_integer_feature_value (camera->priv->device, "PayloadSize");
}

/**
 * arv_camera_get_device:
 * @camera: a #ArvCamera
 *
 * Retrieves the #ArvDevice object for more complete access to camera features.
 *
 * Returns: (transfer none): underlying device object.
 */

ArvDevice *
arv_camera_get_device (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return camera->priv->device;
}

/**
 * arv_camera_new:
 * @name: (allow-none): name of the camera.
 *
 * Creates a new #ArvCamera. If @name is null, it will instantiate the
 * first available camera.
 *
 * Returns: a new #ArvCamera.
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

	return camera;
}

static void
arv_camera_init (ArvCamera *camera)
{
	camera->priv = G_TYPE_INSTANCE_GET_PRIVATE (camera, ARV_TYPE_CAMERA, ArvCameraPrivate);
}

static void
arv_camera_finalize (GObject *object)
{
	ArvCamera *camera = ARV_CAMERA (object);

	g_object_unref (camera->priv->device);

	parent_class->finalize (object);
}

static GObject *
arv_camera_constructor (GType gtype, guint n_properties, GObjectConstructParam *properties)
{
	GObject *object;
	ArvCamera *camera;
	ArvCameraVendor vendor;
	ArvCameraSeries series;
	const char *vendor_name;
	const char *model_name;

	/* always call parent constructor */
	object = parent_class->constructor(gtype, n_properties, properties);

	camera = ARV_CAMERA (object);

	if (!camera->priv->device)
		camera->priv->device = arv_open_device (NULL);

	if (!ARV_IS_DEVICE (camera->priv->device))
		return NULL;

	camera->priv->genicam = arv_device_get_genicam (camera->priv->device);

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
		series = ARV_CAMERA_SERIES_PROSILICA_OTHER;
	} else {
		vendor = ARV_CAMERA_VENDOR_UNKNOWN;
		series = ARV_CAMERA_SERIES_UNKNOWN;
	}

	camera->priv->vendor = vendor;
	camera->priv->series = series;

    return object;
}

static void
arv_camera_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ArvCamera *camera = ARV_CAMERA (object);

	switch (prop_id)
	{
		case PROP_CAMERA_DEVICE:
			camera->priv->device = g_value_get_object (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_camera_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	ArvCamera *camera = ARV_CAMERA (object);

	switch (prop_id)
	{
		case PROP_CAMERA_DEVICE:
            g_value_set_object (value, camera->priv->device);
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

	g_type_class_add_private (camera_class, sizeof (ArvCameraPrivate));

	parent_class = g_type_class_peek_parent (camera_class);

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

G_DEFINE_TYPE (ArvCamera, arv_camera, G_TYPE_OBJECT)
