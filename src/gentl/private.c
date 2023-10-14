#include"private.h"


G_DEFINE_TYPE(ArvTransportLayer,arv_transport_layer,G_TYPE_OBJECT);
static void arv_transport_layer_class_init(ArvTransportLayerClass*){}
static void arv_transport_layer_init(ArvTransportLayer*){}

int gentl_GCInitLib = 0;
ArvTransportLayer* gentl_transport_layer = NULL;
__thread GError* gentl_err = NULL;

GQuark
gentl_error_quark (void)
{
  return g_quark_from_static_string ("gentl-error-quark");
}


size_t gentl_buf_size(INFO_DATATYPE type, const void* data){
	switch(type){
		case INFO_DATATYPE_UNKNOWN: return 1; /* ?? */
		case INFO_DATATYPE_STRING: return strlen(data)+1;
		case INFO_DATATYPE_STRINGLIST: g_critical("%s: returning zero for for INFO_DATATYPE_STRINGLIST (not implemented), expect trouble!",__FUNCTION__); return 0;
		case INFO_DATATYPE_INT16:
		case INFO_DATATYPE_UINT16:
			return 2;
		case INFO_DATATYPE_INT32:
		case INFO_DATATYPE_UINT32:
			return 4;
		case INFO_DATATYPE_INT64:
		case INFO_DATATYPE_UINT64:
		case INFO_DATATYPE_FLOAT64:
			return 8;
		case INFO_DATATYPE_PTR: return sizeof(void*);
		case INFO_DATATYPE_BOOL8: return 1;
		case INFO_DATATYPE_SIZET: return sizeof(size_t);
		case INFO_DATATYPE_BUFFER:
			g_critical("%s: returning zero for for INFO_DATATYPE_BUFFER (not implemented), expect trouble!",__FUNCTION__);
			return 0;
		case INFO_DATATYPE_PTRDIFF: return sizeof(ptrdiff_t);
		default: return 0;
	}
}


GC_ERROR gentl_to_buf(INFO_DATATYPE type, void* dst, const void* src, size_t* sz, INFO_DATATYPE *piType){
	size_t szSrc;
	if(sz==NULL) return GC_ERR_INVALID_PARAMETER;
	if(piType!=NULL) *piType=type;
	szSrc=gentl_buf_size(type,src);
	if(sz==0){ arv_warning_gentl("Datatype %d: zero size (buffer copy unhandled).",type); return GC_ERR_NOT_IMPLEMENTED; }
	/* the call only queries about necessary storage */
	if(dst==NULL){
		*sz=szSrc;
		arv_trace_gentl("   (returning required buffer size %ld)",szSrc);
		return GC_ERR_SUCCESS; 
	}
	if(szSrc>*sz) return GC_ERR_BUFFER_TOO_SMALL;
	*sz=szSrc;
	switch(type){
		case INFO_DATATYPE_STRING:
			arv_trace_gentl("   (returning %ld-byte string: '%s')",szSrc,(const char*)src);
			strcpy(dst,src);
			break;
		case INFO_DATATYPE_INT16:
			*(int16_t*)dst=*(int16_t*)src; break;
			arv_trace_gentl("   (returning int16_t: %d",*(int16_t*)dst);
			break;
		case INFO_DATATYPE_UINT16: *(uint16_t*)dst=*(uint16_t*)src; break;
		case INFO_DATATYPE_INT32:
			*(int32_t*)dst=*(int32_t*)src; break;
			arv_trace_gentl("   (returning int32_t: %d",*(int32_t*)dst);
			break;
		case INFO_DATATYPE_UINT32:
			*(uint32_t*)dst=*(uint32_t*)src;
			arv_trace_gentl("   (returning uint32_t: %d",*(uint32_t*)dst);
			break;
		case INFO_DATATYPE_INT64:  *(int64_t*)dst=*(int64_t*)src; break;
		case INFO_DATATYPE_UINT64: *(uint64_t*)dst=*(uint64_t*)src; break;
		case INFO_DATATYPE_FLOAT64:*(double*)dst=*(double*)src; break;
		case INFO_DATATYPE_PTR:    dst=(void*)src; break;
		case INFO_DATATYPE_BOOL8:  *(char*)dst=*(char*)src; break;
		case INFO_DATATYPE_SIZET:  *(size_t*)dst=*(size_t*)src; break;
		case INFO_DATATYPE_PTRDIFF:*(ptrdiff_t*)dst=*(ptrdiff_t*)src; break;
		default:
			arv_warning_gentl("Datatype %d: buffer copy not implemented.",type); return GC_ERR_NOT_IMPLEMENTED;
	}
	return GC_ERR_SUCCESS;
}

