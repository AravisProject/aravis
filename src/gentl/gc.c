#include"private.h"

#include<stdio.h>
#include<assert.h>
#include<string.h>

#include<arv.h>
// for ARV_GVBS_XML_URL_SIZE
#include<arvgvcpprivate.h>
// for arv_genicam_parse_url
#include<arvmiscprivate.h>

/* minimal validating GenICam description used for System and Interface modules respectively */
const char _XML_TL[]=
"<RegisterDescription"
"  xmlns=\"http://www.genicam.org/GenApi/Version_1_1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
"  ModelName=\"GenTL_SystemModule\" VendorName=\"AravisProject\" StandardNameSpace=\"GEV\""
"  SchemaMajorVersion=\"1\" SchemaMinorVersion=\"1\" SchemaSubMinorVersion=\"0\" MajorVersion=\"1\" MinorVersion=\"0\" SubMinorVersion=\"0\""
"  ProductGuid=\"47374761-6ba9-4e90-a088-1f31da726d85\" VersionGuid=\"0f95f943-b926-4a26-83dc-dbb284dc8231\""
"  xsi:schemaLocation=\"http://www.genicam.org/GenApi/Version_1_1 http://www.genicam.org/GenApi/GenApiSchema_Version_1_1.xsd\""
"/>";

const char _XML_IF[]=
"<RegisterDescription"
"  xmlns=\"http://www.genicam.org/GenApi/Version_1_1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
"  ModelName=\"GenTL_InterfaceModule\" VendorName=\"AravisProject\" StandardNameSpace=\"GEV\""
"  SchemaMajorVersion=\"1\" SchemaMinorVersion=\"1\" SchemaSubMinorVersion=\"0\" MajorVersion=\"1\" MinorVersion=\"0\" SubMinorVersion=\"0\""
"  ProductGuid=\"2d44e0b5-40f2-4c0a-ae7d-6d463d6e516b\" VersionGuid=\"672223ba-b3a6-4bec-a381-b799f0bb566c\""
"  xsi:schemaLocation=\"http://www.genicam.org/GenApi/Version_1_1 http://www.genicam.org/GenApi/GenApiSchema_Version_1_1.xsd\""
"/>";

#define _GC_CHECK_HANDLE { GENTL_ENSURE_INIT; if(hPort==NULL) return GC_ERR_INVALID_HANDLE; }

GC_API GCGetInfo               ( TL_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize){
	arv_trace_gentl("%s (iInfoCmd=%d)",__FUNCTION__,iInfoCmd);
	GENTL_ENSURE_INIT;
	switch(iInfoCmd){
		case TL_INFO_ID:     return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,"Aravis-ID",piSize,piType);
		case TL_INFO_VENDOR: return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,"Aravis-Vendor",piSize,piType);
		case TL_INFO_MODEL:  return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,"Aravis-Model",piSize,piType);
		case TL_INFO_VERSION:return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,"Aravis-Version",piSize,piType);
		case TL_INFO_TLTYPE: return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,"Mixed",piSize,piType);
		case TL_INFO_NAME:   return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,"Aravis-SOMETHING.cti",piSize,piType);
		case TL_INFO_PATHNAME: return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,"/Aravis/somewhere",piSize,piType);
		case TL_INFO_DISPLAYNAME: return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,"Aravis-DisplayName",piSize,piType);
		case TL_INFO_CHAR_ENCODING: { int32_t e=TL_CHAR_ENCODING_UTF8; return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&e,piSize,piType); }
		case TL_INFO_GENTL_VER_MAJOR: { uint32_t v=0; return gentl_to_buf(INFO_DATATYPE_UINT32,pBuffer,&v,piSize,piType); }
		case TL_INFO_GENTL_VER_MINOR: { uint32_t v=1; return gentl_to_buf(INFO_DATATYPE_UINT32,pBuffer,&v,piSize,piType); }
		case TL_INFO_CUSTOM_ID:
		default:
			return GC_ERR_INVALID_PARAMETER;
	}
}

GC_API GCGetLastError          ( GC_ERROR *piErrorCode, char *sErrText, size_t *piSize ){
	if(piSize == NULL) return GC_ERR_INVALID_PARAMETER;
	if (gentl_err == NULL) {
		arv_trace_gentl("%s: No error.",__FUNCTION__);
		return gentl_to_buf(INFO_DATATYPE_STRING,sErrText,"No error.",piSize,NULL);
	}
	if(sErrText == NULL) {
		*piSize = strlen(gentl_err->message) + 1;
		arv_trace_gentl("   (%s: message length is %ld)",__FUNCTION__,*piSize);
		return GC_ERR_SUCCESS;
	}
	arv_trace_gentl("%s: %s",__FUNCTION__,gentl_err->message);
	return gentl_to_buf(INFO_DATATYPE_STRING,sErrText,gentl_err->message,piSize,NULL);
}

GC_API GCInitLib               ( void ){
	arv_trace_gentl(__FUNCTION__);
	return gentl_init();
}

GC_API GCCloseLib              ( void ){
	arv_trace_gentl(__FUNCTION__);
	return gentl_fini();
}


GC_API GCReadPort              ( PORT_HANDLE hPort, uint64_t iAddress, void *pBuffer, size_t *piSize ){
	arv_trace_gentl("%s (hPort=%s[%p],iAddress=%#lx,pBuffer=%p,piSize=%ld)",__FUNCTION__,G_OBJECT_TYPE_NAME(hPort),hPort,iAddress,pBuffer,*piSize);
	_GC_CHECK_HANDLE;
	if(ARV_IS_TRANSPORT_LAYER(hPort) || ARV_IS_INTERFACE(hPort)){
		if(iAddress>0) arv_warning_gentl("iAddress!=0 for system XML; value ignored.");
		return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,ARV_IS_TRANSPORT_LAYER(hPort)?_XML_TL:_XML_IF,piSize,NULL);
	}
	if(ARV_IS_CAMERA(hPort)){
		if(!arv_device_read_memory(arv_camera_get_device(hPort),iAddress,*piSize,pBuffer,&gentl_err)) return GC_ERR_IO;
		return GC_ERR_SUCCESS;
	}
	if(ARV_IS_DEVICE(hPort)){
		if(!arv_device_read_memory(ARV_DEVICE(hPort),iAddress,*piSize,pBuffer,&gentl_err)) return GC_ERR_IO;
		return GC_ERR_SUCCESS;
	}
	GENTL_NYI_DETAIL("only TL/IF/DEV/PORT ports implemented (hPort=%s[%p])",G_OBJECT_TYPE_NAME(hPort),hPort);
	#if 0
		// something like this for device port, in the future
	#endif
}
GC_API GCWritePort             ( PORT_HANDLE hPort, uint64_t iAddress, const void *pBuffer, size_t *piSize ){ GENTL_NYI; }
GC_API GCGetPortURL            ( PORT_HANDLE hPort, char *sURL, size_t *piSize ){ GENTL_NYI; }

GC_API GCGetPortInfo           ( PORT_HANDLE hPort, PORT_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize ){
	arv_trace_gentl("%s (hPort=%s[%p],iInfoCmd=%d,pBuffer=%p)",__FUNCTION__,G_OBJECT_TYPE_NAME(hPort),hPort,iInfoCmd,pBuffer);
	_GC_CHECK_HANDLE;
	if(ARV_IS_TRANSPORT_LAYER(hPort) || ARV_IS_INTERFACE(hPort)){
		switch(iInfoCmd){
			case PORT_INFO_PORTNAME: return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,"Aravis-GenTL",piSize,piType);
			default:
				GENTL_NYI_DETAIL("TL/IF port: hPort=%s[%p], iInfoCmd=%d",G_OBJECT_TYPE_NAME(hPort),hPort,iInfoCmd);
		}
	}
	if(ARV_IS_CAMERA(hPort)){
		switch(iInfoCmd){
			case PORT_INFO_PORTNAME:
				const char* id=arv_camera_get_device_id(hPort,&gentl_err);
				if(id==NULL) return GC_ERR_IO;
				return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,id,piSize,piType);
			default:
				GENTL_NYI_DETAIL("DEV port: hPort=%s[%p], iInfoCmd=%d",G_OBJECT_TYPE_NAME(hPort),hPort,iInfoCmd);
		}
	}
	if(ARV_IS_DEVICE(hPort)){
		switch(iInfoCmd){
			case PORT_INFO_PORTNAME: {
				const char* n;
				n=arv_device_get_string_feature_value(hPort,"DeviceID",&gentl_err);
				if(n==NULL) return GC_ERR_IO;
				return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,n,piSize,piType);
			}
			default:
				GENTL_NYI_DETAIL("PORT port: hPort=%s[%p], iInfoCmd=%d",G_OBJECT_TYPE_NAME(hPort),hPort,iInfoCmd);
		}
	}
	GENTL_NYI_DETAIL("only TL/IF/DEV ports implemented (hPort=%s[%p])",G_OBJECT_TYPE_NAME(hPort),hPort);
}
GC_API GCRegisterEvent         ( EVENTSRC_HANDLE hEventSrc, EVENT_TYPE iEventID, EVENT_HANDLE *phEvent ){
	arv_trace_gentl("%s (hEventSrc=%s[%p],iEventID=%d,phEvent=%p)",__FUNCTION__,G_OBJECT_TYPE_NAME(hEventSrc),hEventSrc,iEventID,phEvent);
	GENTL_NYI_DETAIL("hEventSrc=%s[%p]",G_OBJECT_TYPE_NAME(hEventSrc),hEventSrc);
}
GC_API GCUnregisterEvent       ( EVENTSRC_HANDLE hEventSrc, EVENT_TYPE iEventID ){ GENTL_NYI; }

/* GenTL v1.1 */
GC_API GCGetNumPortURLs        ( PORT_HANDLE hPort, uint32_t *piNumURLs ){
	if(hPort==NULL) return GC_ERR_INVALID_HANDLE; // this apparently sometimes happens?!
	arv_trace_gentl("%s (hPort=%s[%p])",__FUNCTION__,G_OBJECT_TYPE_NAME(hPort),hPort);
	_GC_CHECK_HANDLE;
	if(piNumURLs==NULL) return GC_ERR_INVALID_PARAMETER;
	if(ARV_IS_TRANSPORT_LAYER(hPort)) { *piNumURLs=1; }
	else if(ARV_IS_INTERFACE(hPort)){ *piNumURLs=arv_interface_get_n_devices(ARV_INTERFACE(hPort)); }
	else if(ARV_IS_CAMERA(hPort)){ *piNumURLs=1; }
	else if(ARV_IS_DEVICE(hPort)){ *piNumURLs=1; }
	else { GENTL_NYI_DETAIL("only TL/IF/DEV/PORT ports implemented (hPort=%s[%p])",G_OBJECT_TYPE_NAME(hPort),hPort); }
	arv_trace_gentl("   (returning %d)",*piNumURLs);
	return GC_ERR_SUCCESS;
}

GC_API GCGetPortURLInfo        ( PORT_HANDLE hPort, uint32_t iURLIndex, URL_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize ){
	arv_trace_gentl("%s (hPort=%s[%p],iURLIndex=%d,iInfoCmd=%d)",__FUNCTION__,G_OBJECT_TYPE_NAME(hPort),hPort,iURLIndex,iInfoCmd);
	_GC_CHECK_HANDLE;
	if(ARV_IS_TRANSPORT_LAYER(hPort) || ARV_IS_INTERFACE(hPort)){
		switch(iInfoCmd){
			case URL_INFO_URL: {
				char buf[100];
				gboolean tl=ARV_IS_TRANSPORT_LAYER(hPort);
				g_snprintf(buf,sizeof(buf),tl?"Local:aravis-gentl-transport.xml;0;%ld":"Local:aravis-gentl-interface.xml;0;%ld",(tl?sizeof(_XML_TL):sizeof(_XML_IF))+1);
				return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,buf,piSize,piType);
			}
			default: GENTL_NYI_DETAIL("TL/IF port; iInfoCmd=%d",iInfoCmd);
		}
	}
	if(ARV_IS_CAMERA(hPort)){
		switch(iInfoCmd){
			case URL_INFO_URL: {
				ArvDevice* dev;
				const char *url;
				dev=arv_camera_get_device(ARV_CAMERA(hPort));
				url=arv_dom_document_get_url(ARV_DOM_DOCUMENT(arv_device_get_genicam(dev)));
				return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,url,piSize,piType);
			}
			default: GENTL_NYI_DETAIL("DEV port, iInfoCmd=%d",iInfoCmd);
		}
	}
	if(ARV_IS_DEVICE(hPort)){
		switch(iInfoCmd){
			case URL_INFO_URL: {
				const char* url;
				url=arv_dom_document_get_url(ARV_DOM_DOCUMENT(arv_device_get_genicam(ARV_DEVICE(hPort))));
				return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,url,piSize,piType);
			}
			default: GENTL_NYI_DETAIL("PORT port, iInfoCmd=%d",iInfoCmd);
		}
	}
	GENTL_NYI_DETAIL("only TL/IF/DEV ports implemented (hPort=%s[%p])",G_OBJECT_TYPE_NAME(hPort),hPort);
	#if 0
		#if 0
			const char *url=NULL;
		#else
			char url[ARV_GVBS_XML_URL_SIZE];
		#endif
		char *scheme = NULL;
		char *path=NULL;
		guint64 file_address;
		guint64 file_size;
		ArvDevice* dev;
		ArvGc* genicam;
		ArvGcRegisterDescriptionNode *regs;

		arv_trace_gentl("%s (hPort=%p,iURLIndex=%d,iInfoCmd=%d,pBuffer=%p)",__FUNCTION__,hPort,iURLIndex,iInfoCmd,pBuffer);
		GENTL_ENSURE_INIT;
		dev=arv_open_device(arv_get_device_id(iURLIndex),&gentl_err);
		if(!dev) return GC_ERR_IO;
		genicam=arv_device_get_genicam(dev);
		#if 0
			url=arv_dom_document_get_url(ARV_DOM_DOCUMENT(genicam));
			if(!url) arv_warning_gentl("url is NULL?");
		#else
			if(!arv_device_read_memory(ARV_DEVICE(dev),ARV_GVBS_XML_URL_0_OFFSET,ARV_GVBS_XML_URL_SIZE,url,&gentl_err)) return GC_ERR_IO;
			url[ARV_GVBS_XML_URL_SIZE - 1] = '\0';
		#endif
		regs=ARV_GC_REGISTER_DESCRIPTION_NODE(arv_dom_document_get_document_element(ARV_DOM_DOCUMENT(genicam)));
		/* use this private function, to retrieve scheme, file_address, file_size from the URL */
		arv_parse_genicam_url(url, -1, &scheme, NULL, &path, NULL, NULL, &file_address, &file_size);
		switch(iInfoCmd){
			case URL_INFO_URL: return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,url,piSize,piType);
			case URL_INFO_SCHEMA_VER_MAJOR: {
				int32_t v=arv_gc_register_description_node_get_schema_major_version(regs);
				return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&v,piSize,piType);
			}
			case URL_INFO_SCHEMA_VER_MINOR: {
				int32_t v=arv_gc_register_description_node_get_schema_minor_version(regs);
				return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&v,piSize,piType);
			}
			case URL_INFO_FILE_VER_MAJOR: {
				int32_t v=arv_gc_register_description_node_get_major_version(regs);
				return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&v,piSize,piType);
			}
			case URL_INFO_FILE_VER_MINOR: {
				int32_t v=arv_gc_register_description_node_get_minor_version(regs);
				return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&v,piSize,piType);
			}
			case URL_INFO_FILE_VER_SUBMINOR: {
				int32_t v=arv_gc_register_description_node_get_subminor_version(regs);
				return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&v,piSize,piType);
			}
			case URL_INFO_FILE_SHA1_HASH:    return GC_ERR_NOT_IMPLEMENTED; /* gentl_to_buf(INFO_DATATYPE_BUFFER,pBuffer,...,piSize,piType); */
			case URL_INFO_FILE_REGISTER_ADDRESS: return gentl_to_buf(INFO_DATATYPE_UINT64,pBuffer,&file_address,piSize,piType);
			case URL_INFO_FILE_SIZE:         return gentl_to_buf(INFO_DATATYPE_UINT64,pBuffer,&file_size,piSize,piType);
			case URL_INFO_SCHEME:       return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&scheme,piSize,piType);
			case URL_INFO_FILENAME:          return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,path,piSize,piType);
			default:
				return GC_ERR_INVALID_PARAMETER;
		}
	#endif
}

GC_API GCReadPortStacked       ( PORT_HANDLE hPort, PORT_REGISTER_STACK_ENTRY *pEntries, size_t *piNumEntries ){ GENTL_NYI; }
GC_API GCWritePortStacked      ( PORT_HANDLE hPort, PORT_REGISTER_STACK_ENTRY *pEntries, size_t *piNumEntries ){ GENTL_NYI; }


