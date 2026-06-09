/* Aravis - Digital camera library
 *
 * Copyright © 2023 Václav Šmilauer
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Václav Šmilauer <eu@doxos.eu>
 */

#include"private.h"
#include<arvcamera.h>
#include <arvgvdevice.h>

/*
GenTL's Device is Aravis' Camera. GenTL's Device port is Aravis' Device.
*/

#define _DEV_CHECK_HANDLE GENTL_ENSURE_INIT; if(hDevice==NULL || !(ARV_IS_CAMERA(hDevice))) return GC_ERR_INVALID_HANDLE;

/* helper function to get string from camera via function all (e.g. arv_camera_get_device_id) and return it to the GenTL
 * buffer */

static GC_ERROR
gentl_to_buf__cam_string (DEV_HANDLE hDevice, const char* (*func)(ArvCamera*,GError**),
                          INFO_DATATYPE *piType, void *pBuffer, size_t *piSize)
{
	const char* n;

	/* Aravis requires a clean GError**; clear the stored GenTL last-error. */
	g_clear_error (&gentl_err);
	n=func(hDevice,&gentl_err);
	if(n==NULL)
		return GC_ERR_IO;

	return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,n,piSize,piType);
};

/* Identify the device with the most stable string available. Not every camera
 * exposes a live "DeviceID" feature (some USB3 Vision cameras do not), so fall
 * back to the serial number and then the model name. */
static const char *
arv_gentl_camera_best_id (ArvCamera *camera)
{
	const char *s;
	GError *err = NULL;

	s = arv_camera_get_device_id (camera, &err);
	if (s == NULL || err != NULL) {
		g_clear_error (&err);
		s = arv_camera_get_device_serial_number (camera, &err);
	}
	if (s == NULL || err != NULL) {
		g_clear_error (&err);
		s = arv_camera_get_model_name (camera, &err);
	}
	g_clear_error (&err);

	return s;
}

GC_ERROR
DevGetPort (DEV_HANDLE hDevice, PORT_HANDLE *phRemoteDevice)
{
	_DEV_CHECK_HANDLE;
	if (phRemoteDevice == NULL)
		return GC_ERR_INVALID_PARAMETER;
	arv_trace_gentl ("%s (hDevice=%s[%p])", __FUNCTION__, G_OBJECT_TYPE_NAME (hDevice), hDevice);

	*phRemoteDevice = arv_camera_get_device (hDevice);

	return GC_ERR_SUCCESS;
}

GC_ERROR
DevGetNumDataStreams (DEV_HANDLE hDevice, uint32_t *piNumDataStreams)
{
	_DEV_CHECK_HANDLE
	arv_trace_gentl("%s (hDevice=%s[%p])",__FUNCTION__,G_OBJECT_TYPE_NAME(hDevice),hDevice);

	if(piNumDataStreams==NULL)
		return GC_ERR_INVALID_PARAMETER;

	/* Aravis currently exposes one stream through this producer. */
	*piNumDataStreams = 1;
	arv_trace_gentl("   (returning %d)",*piNumDataStreams);

	return GC_ERR_SUCCESS;
}

GC_ERROR
DevGetDataStreamID (DEV_HANDLE hDevice, uint32_t iIndex, char *sDataStreamID, size_t *piSize)
{
	char buf[4];

	_DEV_CHECK_HANDLE
	arv_trace_gentl("%s (hDevice=%s[%p], iIndex=%d)",__FUNCTION__,G_OBJECT_TYPE_NAME(hDevice),hDevice,iIndex);

	if (iIndex >= 1)
		return GC_ERR_INVALID_INDEX;

	g_snprintf(buf, sizeof(buf), "%d", iIndex);

	/* GenTL identifies streams by strings, so we just use 0-based channel index */
	return gentl_to_buf(INFO_DATATYPE_STRING,sDataStreamID,buf,piSize,NULL);
}

GC_ERROR
DevOpenDataStream (DEV_HANDLE hDevice, const char *sDataStreamID, DS_HANDLE *phDataStream)
{
	ArvGentlDataStream *ds;

	_DEV_CHECK_HANDLE;

	arv_trace_gentl ("%s (hDevice=%s[%p], sDataStreamID=%s)",
			 __FUNCTION__, G_OBJECT_TYPE_NAME (hDevice), hDevice,
			 sDataStreamID ? sDataStreamID : "(null)");

	if (phDataStream == NULL)
		return GC_ERR_INVALID_PARAMETER;
	*phDataStream = NULL;

	/* "0" is currently the only supported stream ID. */
	if (sDataStreamID == NULL || sDataStreamID[0] == '\0')
		return GC_ERR_INVALID_PARAMETER;
	if (g_strcmp0 (sDataStreamID, "0") != 0)
		return GC_ERR_INVALID_ID;

	/* Create the DataStream */
	ds = arv_gentl_data_stream_new (hDevice, sDataStreamID);
	if (ds == NULL) {
		g_clear_error (&gentl_err);
		gentl_err = g_error_new (GENTL_ERROR, GC_ERR_IO,
					 "%s: failed to create DataStream", __FUNCTION__);
		return GC_ERR_IO;
	}

	*phDataStream = (DS_HANDLE) ds;

	arv_trace_gentl ("   (returning DS_HANDLE=%p)", ds);

	return GC_ERR_SUCCESS;
}

GC_ERROR
DevGetInfo ( DEV_HANDLE hDevice, DEVICE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
	_DEV_CHECK_HANDLE;
	arv_trace_gentl("%s (hDevice=%s[%p],iInfoCmd=%d)",__FUNCTION__,G_OBJECT_TYPE_NAME(hDevice),hDevice,iInfoCmd);

	switch(iInfoCmd){
		case DEVICE_INFO_ID:
		case DEVICE_INFO_DISPLAYNAME:
		case DEVICE_INFO_USER_DEFINED_NAME: {
			const char *id = arv_gentl_camera_best_id (hDevice);
			if (id == NULL)
				return GC_ERR_IO;
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, id, piSize, piType);
		}
		case DEVICE_INFO_VENDOR:
			return gentl_to_buf__cam_string (hDevice, arv_camera_get_vendor_name, piType, pBuffer, piSize);
		case DEVICE_INFO_MODEL:
			return gentl_to_buf__cam_string (hDevice, arv_camera_get_model_name, piType, pBuffer, piSize);
		case DEVICE_INFO_SERIAL_NUMBER:
			return gentl_to_buf__cam_string (hDevice, arv_camera_get_device_serial_number, piType, pBuffer, piSize);
		case DEVICE_INFO_TLTYPE: {
			const char *tltype = arv_camera_is_uv_device (hDevice) ? "U3V" : "GEV";
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, tltype, piSize, piType);
		}
		case DEVICE_INFO_VERSION:
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, "1.0", piSize, piType);
		case DEVICE_INFO_ACCESS_STATUS: {
			int32_t rw = DEVICE_ACCESS_STATUS_READWRITE;
			return gentl_to_buf (INFO_DATATYPE_INT32, pBuffer, &rw, piSize, piType);
		}
		case DEVICE_INFO_TIMESTAMP_FREQUENCY: {
			ArvDevice *device = arv_camera_get_device (hDevice);
			guint64 frequency;

			if (!ARV_IS_GV_DEVICE (device))
				return GC_ERR_NOT_AVAILABLE;
			g_clear_error (&gentl_err);
			frequency = arv_gv_device_get_timestamp_tick_frequency (
				ARV_GV_DEVICE (device), &gentl_err);
			if (gentl_err != NULL)
				return GC_ERR_IO;
			return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &frequency,
					     piSize, piType);
		}
		default:
			GENTL_NYI_DETAIL("iInfoCmd=%d",iInfoCmd);
	}
}

GC_ERROR
DevClose (DEV_HANDLE hDevice)
{
	_DEV_CHECK_HANDLE;

	arv_trace_gentl ("%s (hDevice=%s[%p])", __FUNCTION__,
			 G_OBJECT_TYPE_NAME (hDevice), hDevice);

	/* Close the camera. The DataStream handle should be closed
	 * before calling DevClose. If there's still an active DataStream, the
	 * consumer should have called DSClose first. */

	g_object_unref (hDevice);

	return GC_ERR_SUCCESS;
}

/* GenTL v1.4 */

GC_ERROR
DevGetParentIF (DEV_HANDLE hDevice, IF_HANDLE *phIface)
{
	IF_HANDLE parent;

	_DEV_CHECK_HANDLE;
	if (phIface == NULL)
		return GC_ERR_INVALID_PARAMETER;

	parent = g_object_get_data (G_OBJECT (hDevice), "arv-gentl-parent-interface");
	if (parent == NULL)
		return GC_ERR_NOT_AVAILABLE;

	*phIface = parent;
	return GC_ERR_SUCCESS;
}
