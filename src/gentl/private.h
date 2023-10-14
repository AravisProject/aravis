#include"GenTL_v1_6.h"

#include<glib-object.h>

/*
logging macros
*/
#include<arvdebugprivate.h>

/*
In our code, GC_API is used in implementation, so only specifies return type.
This is to allow verbatim copy of the function signature into the implementation file.
*/
#undef GC_API
#define GC_API GC_ERROR

/*
Keep track of whether we've been initialized. This is required by the API (Aravis has nothing comparable).
*/
extern int gentl_GCInitLib;
/*
Almost all functions are supposed to return GC_ERROR_NOT_INITIALIZED without preceding GCInitLib call.
Wrap that into a macro.
*/
#define GENTL_ENSURE_INIT if(gentl_GCInitLib == 0) return GC_ERR_NOT_INITIALIZED


/*
Store last error (must be separate for each thread)
*/
extern __thread GError* gentl_err;

#define GENTL_ERROR gentl_error_quark ()
GQuark gentl_error_quark (void);

/*
Stubs for unimplemented functions.
*/
#define GENTL_NYI \
	arv_warning_gentl("%s not yet implemented.",__FUNCTION__); \
	gentl_err = g_error_new (GENTL_ERROR, GC_ERR_NOT_IMPLEMENTED, "%s not yet implemented in Aravis GenTL", __FUNCTION__); \
	return GC_ERR_NOT_IMPLEMENTED

#define GENTL_NYI_DETAIL(fmt,...) \
	arv_warning_gentl("%s: not yet implemented: " fmt,__FUNCTION__,__VA_ARGS__); \
	gentl_err = g_error_new (GENTL_ERROR, GC_ERR_NOT_IMPLEMENTED, "%s (Aravis GenTL) not yet implemented: " fmt,__FUNCTION__,__VA_ARGS__); \
	return GC_ERR_NOT_IMPLEMENTED


/*
Routine for copying anything into buffer.
*/
size_t gentl_buf_size(INFO_DATATYPE,const void*);
GC_ERROR gentl_to_buf(INFO_DATATYPE type, void* dst, const void* src, size_t* sz, INFO_DATATYPE *piType) G_GNUC_WARN_UNUSED_RESULT;

/*
Handle type for the transport layer (for the rest, we use corresponding Aravis types)
*/
struct _ArvTransportLayer{ GObject parent_instance; };
#define ARV_TYPE_TRANSPORT_LAYER (arv_transport_layer_get_type ())
G_DECLARE_FINAL_TYPE(ArvTransportLayer, arv_transport_layer, ARV, TRANSPORT_LAYER, GObject)
/* global instance */
extern ArvTransportLayer* gentl_transport_layer;
