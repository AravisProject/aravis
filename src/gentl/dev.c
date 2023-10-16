#include"private.h"
#include<arvcamera.h>

/*
GenTL's Device is Aravis' Camera. GenTL's Device port is Aravis' Device.
*/

#define _DEV_CHECK_HANDLE GENTL_ENSURE_INIT; if(hDevice==NULL || !(ARV_IS_CAMERA(hDevice))) return GC_ERR_INVALID_HANDLE;

/*
helper function to get string from camera via function all (e.g. arv_camera_get_device_id) and return it to the GenTL buffer
*/
GC_ERROR gentl_to_buf__cam_string(DEV_HANDLE hDevice, const char* (*func)(ArvCamera*,GError**), INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);

GC_ERROR gentl_to_buf__cam_string(DEV_HANDLE hDevice, const char* (*func)(ArvCamera*,GError**), INFO_DATATYPE *piType, void *pBuffer, size_t *piSize){
	const char* n;
	n=func(hDevice,&gentl_err);
	if(n==NULL) return GC_ERR_IO;
	return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,n,piSize,piType);
};

GC_API DevGetPort              ( DEV_HANDLE hDevice, PORT_HANDLE *phRemoteDevice ){
	_DEV_CHECK_HANDLE;
	arv_trace_gentl("%s (hDevice=%s[%p])",__FUNCTION__,G_OBJECT_TYPE_NAME(hDevice),hDevice);
	*phRemoteDevice=arv_camera_get_device(hDevice);
	return GC_ERR_SUCCESS;
}

GC_API DevGetNumDataStreams    ( DEV_HANDLE hDevice, uint32_t *piNumDataStreams ){
	_DEV_CHECK_HANDLE
	arv_trace_gentl("%s (hDevice=%s[%p])",__FUNCTION__,G_OBJECT_TYPE_NAME(hDevice),hDevice);
	*piNumDataStreams=arv_camera_gv_get_n_stream_channels(hDevice,&gentl_err);
	if(*piNumDataStreams==0) return GC_ERR_IO;
	return GC_ERR_SUCCESS;
}
GC_API DevGetDataStreamID      ( DEV_HANDLE hDevice, uint32_t iIndex, char *sDataStreamID, size_t *piSize ){ GENTL_NYI; }
GC_API DevOpenDataStream       ( DEV_HANDLE hDevice, const char *sDataStreamID, DS_HANDLE *phDataStream ){ GENTL_NYI; }

GC_API DevGetInfo              ( DEV_HANDLE hDevice, DEVICE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize ){
	_DEV_CHECK_HANDLE;
	arv_trace_gentl("%s (hDevice=%s[%p],iInfoCmd=%d)",__FUNCTION__,G_OBJECT_TYPE_NAME(hDevice),hDevice,iInfoCmd);
	switch(iInfoCmd){
		case DEVICE_INFO_ID:
			return gentl_to_buf__cam_string(hDevice,arv_camera_get_device_id,piType,pBuffer,piSize);
		default:
			GENTL_NYI_DETAIL("iInfoCmd=%d",iInfoCmd);
	}
}

GC_API DevClose                ( DEV_HANDLE hDevice ){ GENTL_NYI; }
/* GenTL v1.4 */
GC_API DevGetParentIF          ( DEV_HANDLE hDevice, IF_HANDLE *phIface ){ GENTL_NYI; }

