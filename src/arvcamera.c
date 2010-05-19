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

static GObjectClass *parent_class = NULL;

struct _ArvCameraPrivate {
	ArvDevice *device;
	ArvGc *genicam;

	ArvCameraVendor vendor;
};

ArvStream *
arv_camera_create_stream (ArvCamera *camera, ArvStreamCallback callback, void *user_data)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_create_stream (camera->priv->device, callback, user_data);
}

/* Device control */

const char *
arv_camera_get_vendor_name (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_get_string_feature_value (camera->priv->device, "DeviceVendorName");
}

const char *
arv_camera_get_model_name (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_get_string_feature_value (camera->priv->device, "DeviceModelName");
}

const char *
arv_camera_get_device_id (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), NULL);

	return arv_device_get_string_feature_value (camera->priv->device, "DeviceID");
}

/* Image format control */

void
arv_camera_get_sensor_size (ArvCamera *camera, gint *width, gint *height)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (width != NULL)
		*width = arv_device_get_integer_feature_value (camera->priv->device, "SensorWidth");
	if (height != NULL)
		*height = arv_device_get_integer_feature_value (camera->priv->device, "SensorHeight");
}

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

void
arv_camera_set_binning (ArvCamera *camera, gint dx, gint dy)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (dx > 0)
		arv_device_set_integer_feature_value (camera->priv->device, "BinningHorizontal", dx);
	if (dy > 0)
		arv_device_set_integer_feature_value (camera->priv->device, "BinningVertical", dy);
}

void
arv_camera_get_binning (ArvCamera *camera, gint *dx, gint *dy)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	if (dx != NULL)
		*dx = arv_device_get_integer_feature_value (camera->priv->device, "BinningHorizontal");
	if (dy != NULL)
		*dy = arv_device_get_integer_feature_value (camera->priv->device, "BinningVertical");
}

void
arv_camera_set_pixel_format (ArvCamera *camera, ArvPixelFormat format)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_integer_feature_value (camera->priv->device, "PixelFormat", format);
}

ArvPixelFormat
arv_camera_get_pixel_format (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0);

	return arv_device_get_integer_feature_value (camera->priv->device, "PixelFormat");
}

/* Acquisition control */

void
arv_camera_start_acquisition (ArvCamera *camera)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_execute_command (camera->priv->device, "AcquisitionStart");
}

void
arv_camera_stop_acquisition (ArvCamera *camera)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_execute_command (camera->priv->device, "AcquisitionStop");
}

void
arv_camera_set_acquisition_mode (ArvCamera *camera, ArvAcquisitionMode mode)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_string_feature_value (camera->priv->device, "AcquisitionMode",
					     arv_acquisition_mode_to_string (mode));
}

ArvAcquisitionMode
arv_camera_get_acquisition_mode (ArvCamera *camera)
{
	const char *string;

	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0);

	string = arv_device_get_string_feature_value (camera->priv->device, "AcquisitionMode");

	return arv_acquisition_mode_from_string (string);
}

void
arv_camera_set_fixed_frame_rate (ArvCamera *camera, double frame_rate)
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

void
arv_camera_set_external_trigger (ArvCamera *camera, ArvTriggerSource source)
{
	const char *string;

	g_return_if_fail (ARV_IS_CAMERA (camera));

	string = arv_trigger_source_to_string (source);

	switch (camera->priv->vendor) {
		case ARV_CAMERA_VENDOR_BASLER:
			arv_device_set_integer_feature_value (camera->priv->device, "AcquisitionFrameRateEnable",
							      0);
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector",
							     "AcquisitionStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "On");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerActivation",
							     "RisingEdge");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSource", string);
			break;
		case ARV_CAMERA_VENDOR_PROSILICA:
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector",
							     "AcquisitionStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "Off");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector", "FrameStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "On");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerActivation",
							     "RisingEdge");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSource", string);
			break;
		case ARV_CAMERA_VENDOR_UNKNOWN:
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector",
							     "AcquisitionStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "Off");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSelector", "FrameStart");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerMode", "On");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerActivation",
							     "RisingEdge");
			arv_device_set_string_feature_value (camera->priv->device, "TriggerSource", string);
			break;
	}
}

void
arv_camera_set_exposure_time (ArvCamera *camera, double exposure_time_us)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_float_feature_value (camera->priv->device, "ExposureTimeAbs", exposure_time_us);
}

double
arv_camera_get_exposure_time (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0.0);

	return arv_device_get_float_feature_value (camera->priv->device, "ExposureTimeAbs");
}

/* Analog control */

void
arv_camera_set_gain (ArvCamera *camera, gint64 gain)
{
	g_return_if_fail (ARV_IS_CAMERA (camera));

	arv_device_set_integer_feature_value (camera->priv->device, "GainRaw", gain);
}

gint64
arv_camera_get_gain (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0.0);

	return arv_device_get_integer_feature_value (camera->priv->device, "GainRaw");
}

/* Transport layer control */

guint
arv_camera_get_payload (ArvCamera *camera)
{
	g_return_val_if_fail (ARV_IS_CAMERA (camera), 0);

	return arv_device_get_integer_feature_value (camera->priv->device, "PayloadSize");
}

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
