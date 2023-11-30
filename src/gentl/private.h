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

#ifndef ARV_GENTL_PRIVATE_H
#define ARV_GENTL_PRIVATE_H

/*
MinGW needs <stdint.h> prior to GenTL_v1_6.h for uint64_t and others;
since GenTL_v1_6.h may not be modified as per license, include here instead.
*/
#include<stdint.h>

#include"GenTL_v1_6.h"

#include<glib-object.h>

/*
logging macros
*/
#include<arvdebugprivate.h>

/*
Routine for copying anything into buffer. Implementation in private-buf.c
*/
size_t gentl_buf_size(INFO_DATATYPE,const void*);
GC_ERROR gentl_to_buf(INFO_DATATYPE type, void* dst, const void* src, size_t* sz, INFO_DATATYPE *piType) G_GNUC_WARN_UNUSED_RESULT;


#if defined(_WIN32) && defined(_MSC_VER)
	#define GENTL_THREAD_LOCAL_STORAGE __declspec(thread)
#else
	#define GENTL_THREAD_LOCAL_STORAGE __thread
#endif

GC_ERROR gentl_init (void);
GC_ERROR gentl_fini (void);
gboolean gentl_is_initialized (void);

/*
Almost all functions are supposed to return GC_ERROR_NOT_INITIALIZED without preceding GCInitLib call.
Wrap that into a macro.
*/
#define GENTL_ENSURE_INIT if (!gentl_is_initialized()) return GC_ERR_NOT_INITIALIZED;

/*
Store last error (must be separate for each thread); this is accessible from other .c files
*/
extern GENTL_THREAD_LOCAL_STORAGE GError* gentl_err;

#define GENTL_ERROR gentl_error_quark ()
GQuark gentl_error_quark (void);

/*
Stubs for unimplemented functions.
*/
#define GENTL_NYI { \
	arv_warning_gentl("%s not yet implemented.",__FUNCTION__); \
	gentl_err = g_error_new (GENTL_ERROR, GC_ERR_NOT_IMPLEMENTED, "%s not yet implemented in Aravis GenTL", __FUNCTION__); \
	return GC_ERR_NOT_IMPLEMENTED; }

#define GENTL_NYI_DETAIL(fmt,...) { \
	arv_warning_gentl("%s: not yet implemented: " fmt,__FUNCTION__,__VA_ARGS__); \
	gentl_err = g_error_new (GENTL_ERROR, GC_ERR_NOT_IMPLEMENTED, "%s (Aravis GenTL) not yet implemented: " fmt,__FUNCTION__,__VA_ARGS__); \
	return GC_ERR_NOT_IMPLEMENTED; }


/*
Handle type for the transport layer (for the rest, we use corresponding Aravis types)
*/
struct _ArvTransportLayer{ GObject parent_instance; };
#define ARV_TYPE_TRANSPORT_LAYER (arv_transport_layer_get_type ())
G_DECLARE_FINAL_TYPE(ArvTransportLayer, arv_transport_layer, ARV, TRANSPORT_LAYER, GObject)
/* global instance */
extern ArvTransportLayer* gentl_transport_layer;

/*
Handle type for events; this is a stub class; it should include event data as necessary.
*/
struct _ArvGentlEvent{ GObject parent_instance; };
#define ARV_TYPE_TRANSPORT_LAYER (arv_transport_layer_get_type ())
G_DECLARE_FINAL_TYPE(ArvGentlEvent, arv_gentl_event, ARV, GENTL_EVENT, GObject)






#endif
