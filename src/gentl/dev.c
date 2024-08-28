/* Aravis - Digital camera library
 *
 * Copyright © 2023 Václav Šmilauer
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
 * Authors: Václav Šmilauer <eu@doxos.eu>
 */

#include"private.h"
#include<arvcamera.h>

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

	n=func(hDevice,&gentl_err);
	if(n==NULL)
                return GC_ERR_IO;

	return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,n,piSize,piType);
};

GC_API
DevGetPort (DEV_HANDLE hDevice, PORT_HANDLE *phRemoteDevice)
{
        _DEV_CHECK_HANDLE;
        arv_trace_gentl("%s (hDevice=%s[%p])",__FUNCTION__,G_OBJECT_TYPE_NAME(hDevice),hDevice);

        *phRemoteDevice=arv_camera_get_device(hDevice);

        return GC_ERR_SUCCESS;
}

GC_API
DevGetNumDataStreams (DEV_HANDLE hDevice, uint32_t *piNumDataStreams)
{
	GError* err;

	_DEV_CHECK_HANDLE
	arv_trace_gentl("%s (hDevice=%s[%p])",__FUNCTION__,G_OBJECT_TYPE_NAME(hDevice),hDevice);

	if(piNumDataStreams==NULL)
                return GC_ERR_INVALID_PARAMETER;
	if(!arv_camera_is_gv_device(hDevice))
                arv_warning_gentl("%s: not a GV device: expect trouble.",__FUNCTION__);

        /* GenTL provides no way to clear last error; so there would be a warning here "CRTITICAL **:
         * arv_gc_integer_get_value: assertion 'error == NULL || *error == NULL' failed" — which only means that a
         * previous error is stored in gentl_error (as required by the specification). In this case,
         * arv_camera_gv_get_n_stream_channels will report failure, that's why we use a separate error pointer just for
         * the call, and store it in gentl_err if it fails.  */
	err=NULL;
	*piNumDataStreams=arv_camera_gv_get_n_stream_channels(hDevice,&err);
	if(err!=NULL) {
                *gentl_err=*err; return GC_ERR_IO;
        }
	arv_trace_gentl("   (returning %d)",*piNumDataStreams);

	return GC_ERR_SUCCESS;
}

GC_API
DevGetDataStreamID (DEV_HANDLE hDevice, uint32_t iIndex, char *sDataStreamID, size_t *piSize)
{
	GError* err = NULL;
	gint chan;
	char buf[4];

	_DEV_CHECK_HANDLE
	arv_trace_gentl("%s (hDevice=%s[%p], iIndex=%d)",__FUNCTION__,G_OBJECT_TYPE_NAME(hDevice),hDevice,iIndex);

	chan = arv_camera_gv_get_n_stream_channels(hDevice,&err);

	if (err != NULL) {
                *gentl_err=*err;
                return GC_ERR_IO;
        }
	if(chan==0)
                return GC_ERR_NOT_IMPLEMENTED;
	if (iIndex >= chan)
                return GC_ERR_INVALID_INDEX;

	g_snprintf(buf, sizeof(buf), "%d", iIndex);

	/* GenTL identifies streams by strings, so we just use 0-based channel index */
	return gentl_to_buf(INFO_DATATYPE_STRING,sDataStreamID,buf,piSize,NULL);
}

GC_API
DevOpenDataStream (DEV_HANDLE hDevice, const char *sDataStreamID, DS_HANDLE *phDataStream)
{
	GENTL_NYI;
}

GC_API
DevGetInfo ( DEV_HANDLE hDevice, DEVICE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
	_DEV_CHECK_HANDLE;
	arv_trace_gentl("%s (hDevice=%s[%p],iInfoCmd=%d)",__FUNCTION__,G_OBJECT_TYPE_NAME(hDevice),hDevice,iInfoCmd);

	switch(iInfoCmd){
		case DEVICE_INFO_ID:
			return gentl_to_buf__cam_string(hDevice,arv_camera_get_device_id,piType,pBuffer,piSize);
		default:
			GENTL_NYI_DETAIL("iInfoCmd=%d",iInfoCmd);
	}
}

GC_API
DevClose (DEV_HANDLE hDevice)
{
        GENTL_NYI;
}

/* GenTL v1.4 */

GC_API
DevGetParentIF (DEV_HANDLE hDevice, IF_HANDLE *phIface)
{
        GENTL_NYI;
}
