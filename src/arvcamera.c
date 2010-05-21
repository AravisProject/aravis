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

#include <arvcamera.h>
#include <arvsystem.h>
#include <arvgvinterface.h>
#include <arvgccommand.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcenumeration.h>
#include <arvgcstring.h>
#include <arvgc.h>
#include <arvdevice.h>
#include <arvenums.h>

static GObjectClass *parent_class = NULL;

struct _ArvCameraPrivate {
	ArvDevice *device;
	ArvGc *genicam;

	ArvCameraVendor vendor;
};

/**
 * arv_camera_create_stream:
 * @camera: a #ArvCamera
 * @callback: a frame processing callback
 * @user_data: closure
 * Return value: a new #ArvStream.
 *
 * Creates a new #ArvStream for video stream handling. See
 * @arv_device_create_stream for details regarding the callback function.
 */

ArvStream *
arv_camera_create_stream (ArvCamera *camera, ArvStreamCallback callback, void *user_data)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_create_stream (camera->priv->device, callback, user_data);
}

/* Device control */

/**
 * arv_camera_get_vendor_name:
 * @camera: a #ArvCamera
 * Return value: the camera vendor name.
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
 * Return value: the camera model name.
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
 * Return value: the camera device ID.
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
 * @width: camera sensor width placeholder
 * @height: camera sensor height placeholder
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
arv_camera_set_region (ArvCamera *camera, int x, int y, int width, int height)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (x > 0)
		arv_device_set_integer_feature_value (camera->priv->device, "OffsetX", x);
	if (y > 0)
		arv_device_set_integer_feature_value (camera->priv->device, "OffsetY", y);
	if (width > 0)
		arv_device_set_integer_feature_value (camera->priv->device, "Width", width);
	if (height > 0)
		arv_device_set_integer_feature_value (camera->priv->device, "Height", height);
}

/**
 * arv_camera_get_region:
 * @camera: a #ArvCamera
 * @x: x offset placeholder
 * @y: y_offset placeholder
 * @width: region width placeholder
 * @height: region height placeholder
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
 * arv_camera_set_binning:
 * @camera: a #ArvCamera
 * @dx: horizontal binning
 * @dy: vertical binning
 *
 * Defines the binning in both directions. Not all cameras support this
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
 * @dx: horizontal binning placeholder
 * @dy: vertical binning placeholder
 *
 * Retrieves the binning in both directions.
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
 * Defines the pixel format.
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
 * Retrieves the pixel format.
 */

ArvPixelFormat
arv_camera_get_pixel_format (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0);

	return arv_device_get_integer_feature_value (camera->priv->device, "PixelFormat");
}

/* Acquisition control */

/**
 * arv_camera_start_acquisition:
 * @camera: a #ArvCamera
 *
 * Starts the video stream acquisition.
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
 * Stops the video stream acquisition.
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
 * Defines the acquisition mode.
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
 * Return value: the acquisition mode.
 *
 * Retrieves the acquisition mode.
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

	switch (camera->priv->vendor) {
		case ARV_CAMERA_VENDOR_BASLER:
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector",
							     "AcquisitionStart");
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
 * Return value: the actual frame rate, in Hz.
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
 * or "Line2". See the camera documentation for the allowed values. The
 * activation is set to rising edge. It can be changed by accessing the
 * underlying device object.
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
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector",
							     "AcquisitionStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "On");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerActivation",
							     "RisingEdge");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSource", source);
			break;
		case ARV_CAMERA_VENDOR_PROSILICA:
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector",
							     "AcquisitionStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "Off");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector", "FrameStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "On");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerActivation",
							     "RisingEdge");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSource", source);
			break;
		case ARV_CAMERA_VENDOR_UNKNOWN:
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector",
							     "AcquisitionStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "Off");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector", "FrameStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "On");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerActivation",
							     "RisingEdge");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSource", source);
			break;
	}
}

/**
 * arv_camera_set_exposure_time:
 * @camera: a #ArvCamera
 * @exposure_time_us: exposure time, in µs
 *
 * Sets the exposure time. User should take care to set a value compatible with
 * the desired frame rate.
 */

void
arv_camera_set_exposure_time (ArvCamera *camera, double exposure_time_us)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_float_feature_value (camera->priv->device, "ExposureTimeAbs", exposure_time_us);
}

/**
 * arv_camera_get_exposure_time:
 * @camera: a #ArvCamera
 * Return value: the current exposure time, in µs.
 */

double
arv_camera_get_exposure_time (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0.0);

	return arv_device_get_float_feature_value (camera->priv->device, "ExposureTimeAbs");
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
arv_camera_set_gain (ArvCamera *camera, gint64 gain)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_integer_feature_value (camera->priv->device, "GainRaw", gain);
}

/**
 * arv_camera_get_gain:
 * @camera: a #ArvCamera
 * Return value: the current gain setting.
 */

gint64
arv_camera_get_gain (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0.0);

	return arv_device_get_integer_feature_value (camera->priv->device, "GainRaw");
}

/* Transport layer control */

/**
 * arv_camera_get_payload:
 * @camera: a #ArvCamera
 * Return value: the frame storage size, in bytes.
 * 
 * Retrieves the size needed for the storage of an image. This value is used
 * for the creation of the stream buffers.
 */

guint
arv_camera_get_payload (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0);

	return arv_device_get_integer_feature_value (camera->priv->device, "PayloadSize");
}

/**
 * arv_camera_new:
 * @name: (allow-none): name of the camera.
 * Return value: a new #ArvCamera.
 *
 * Creates a new #ArvCamera. If @name is null, it will instantiate the
 * first available camera
 */

ArvCamera *
arv_camera_new (const char *name)
{
	ArvCamera *camera;
	ArvDevice *device;
	ArvCameraVendor vendor;
	const char *vendor_name;

	device = arv_new_device (name);

	if (!ARV_IS_DEVICE (device))
		return NULL;

	camera = g_object_new (ARV_TYPE_CAMERA, NULL);
	camera->priv->device = device;
	camera->priv->genicam = arv_device_get_genicam (device);

	vendor_name = arv_camera_get_vendor_name (camera);
	if (g_strcmp0 (vendor_name, "Basler") == 0)
		vendor = ARV_CAMERA_VENDOR_BASLER;
	else if (g_strcmp0 (vendor_name, "Prosilica") == 0)
		vendor = ARV_CAMERA_VENDOR_PROSILICA;
	else
		vendor = ARV_CAMERA_VENDOR_UNKNOWN;

	camera->priv->vendor = vendor;

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

static void
arv_camera_class_init (ArvCameraClass *camera_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (camera_class);

	g_type_class_add_private (camera_class, sizeof (ArvCameraPrivate));

	parent_class = g_type_class_peek_parent (camera_class);

	object_class->finalize = arv_camera_finalize;
}

G_DEFINE_TYPE (ArvCamera, arv_camera, G_TYPE_OBJECT)
