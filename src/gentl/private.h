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

#ifndef ARV_GENTL_PRIVATE_H
#define ARV_GENTL_PRIVATE_H

#include <arvgentlprivate.h>
#include <glib-object.h>

/*
logging macros
*/
#include <arvdebugprivate.h>

/*
Routine for copying anything into buffer. Implementation in private-buf.c
*/
size_t gentl_buf_size(INFO_DATATYPE,const void*);
GC_ERROR gentl_to_buf(INFO_DATATYPE type, void* dst, const void* src, size_t* sz, INFO_DATATYPE *piType) G_GNUC_WARN_UNUSED_RESULT;


#if defined(_WIN32) && defined(_MSC_VER)
	#define GENTL_THREAD_LOCAL_STORAGE __declspec(thread)
	/*
	MSVC compiler does not allow dllimport spec in the implementation file (but mingw w32 requires it);
	so we replace all GC_API (which is return type GC_ERROR plus dllimport declspec) by plain GC_ERROR.
	This is to allow verbatim copy of function signature from GenTL header into the implementation files.
	*/
	#undef GC_API
	#define GC_API GC_ERROR
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
#define GENTL_NYI                                                                                                                      \
	{                                                                                                                              \
		arv_warning_gentl ("%s not yet implemented.", __FUNCTION__);                                                           \
		g_clear_error (&gentl_err);                                                                                            \
		gentl_err = g_error_new (GENTL_ERROR, GC_ERR_NOT_IMPLEMENTED, "%s not yet implemented in Aravis GenTL", __FUNCTION__); \
		return GC_ERR_NOT_IMPLEMENTED;                                                                                         \
	}

#define GENTL_NYI_DETAIL(fmt, ...)                                                                                                                       \
	{                                                                                                                                                \
		arv_warning_gentl ("%s: not yet implemented: " fmt, __FUNCTION__, __VA_ARGS__);                                                          \
		g_clear_error (&gentl_err);                                                                                                              \
		gentl_err = g_error_new (GENTL_ERROR, GC_ERR_NOT_IMPLEMENTED, "%s (Aravis GenTL) not yet implemented: " fmt, __FUNCTION__, __VA_ARGS__); \
		return GC_ERR_NOT_IMPLEMENTED;                                                                                                           \
	}

/*
Handle type for the transport layer (for the rest, we use corresponding Aravis types)
*/
struct _ArvTransportLayer{ GObject parent_instance; };
#define ARV_TYPE_TRANSPORT_LAYER (arv_transport_layer_get_type ())
G_DECLARE_FINAL_TYPE(ArvTransportLayer, arv_transport_layer, ARV, TRANSPORT_LAYER, GObject)
/* global instance */
extern ArvTransportLayer* gentl_transport_layer;

/* ========================================================================== */
/* GenTL Buffer — plain struct (not GObject)                                  */
/* ========================================================================== */

typedef enum {
	ARV_GENTL_BUFFER_ANNOUNCED,
	ARV_GENTL_BUFFER_QUEUED,
	ARV_GENTL_BUFFER_FILLING,
	ARV_GENTL_BUFFER_FILLED,
	ARV_GENTL_BUFFER_REVOKED
} ArvGentlBufferState;

typedef struct _ArvGentlBuffer {
	void *base;
	size_t size;
	void *user_ptr;

	gboolean owns_memory;

	ArvGentlBufferState state;

	size_t size_filled;
	size_t data_size;
	guint64 frame_id;
	guint64 timestamp_ns;

	guint width;
	guint height;
	guint64 pixel_format;

	gboolean is_incomplete;
} ArvGentlBuffer;

/* ========================================================================== */
/* GenTL DataStream — GObject wrapping ArvStream                              */
/* ========================================================================== */

#define ARV_TYPE_GENTL_DATA_STREAM (arv_gentl_data_stream_get_type ())
G_DECLARE_FINAL_TYPE (ArvGentlDataStream, arv_gentl_data_stream, ARV, GENTL_DATA_STREAM, GObject)

struct _ArvGentlDataStream {
	GObject parent_instance;

	/* Camera reference */
	ArvCamera *camera;

	/* The Aravis stream created during start acquisition */
	ArvStream *stream;

	/* Stream ID string */
	char *stream_id;

	/* Buffer queues */
	GMutex lock; /* protects all queues and state below */
	GCond cond;  /* signalled when a buffer becomes available in output_queue */

	GQueue *announced_buffers; /* all buffers ever announced (ArvGentlBuffer*) */
	GQueue *input_queue;	   /* buffers queued by consumer, waiting for data */
	GQueue *output_queue;	   /* buffers filled with data, waiting for EventGetData */

	/* Acquisition state */
	gboolean acquiring;
	gboolean stop_thread;
	gboolean acquisition_complete;
	guint64 num_to_acquire;
	guint64 acquisition_delivered_count;

	/* Worker thread */
	GThread *worker_thread;

	/* Statistics */
	guint64 frame_id_counter;
	guint64 started_count;
	guint64 delivered_count;
	guint64 lost_count;

	/* Image info (set after first buffer) */
	size_t payload_size;
	guint width;
	guint height;
	guint64 pixel_format;

	/* Number of internal buffers */
	guint n_internal_buffers;
};

/* Handle validation macros */
#define _DS_CHECK_HANDLE                                                    \
	GENTL_ENSURE_INIT;                                                  \
	if (hDataStream == NULL || !ARV_IS_GENTL_DATA_STREAM (hDataStream)) \
		return GC_ERR_INVALID_HANDLE;

#define _BUF_CHECK_HANDLE \
	if (hBuffer == NULL) return GC_ERR_INVALID_HANDLE;

/* ========================================================================== */
/* GenTL Event — extended with event-specific data                            */
/* ========================================================================== */

typedef enum {
	ARV_GENTL_EVENT_STATE_IDLE,
	ARV_GENTL_EVENT_STATE_WAITING,
	ARV_GENTL_EVENT_STATE_KILLED
} ArvGentlEventState;

/*
Handle type for events.
*/
struct _ArvGentlEvent {
	GObject parent_instance;

	/* Event type */
	EVENT_TYPE event_type;

	/* Handle that this event is registered on (DS_HANDLE, DEV_HANDLE, etc.) */
	void *event_src;

	/* For EVENT_NEW_BUFFER: reference to the DataStream */
	ArvGentlDataStream *data_stream;

	/* State */
	ArvGentlEventState state;

	guint num_fired;
};

#define ARV_TYPE_GENTL_EVENT (arv_gentl_event_get_type ())
G_DECLARE_FINAL_TYPE(ArvGentlEvent, arv_gentl_event, ARV, GENTL_EVENT, GObject)

/* ========================================================================== */
/* Utility functions                                                          */
/* ========================================================================== */

ArvGentlDataStream *arv_gentl_data_stream_new (ArvCamera *camera, const char *stream_id);
void arv_gentl_data_stream_attach_buffer (ArvGentlDataStream *ds, ArvGentlBuffer *buf);
void arv_gentl_data_stream_detach_buffer (ArvGentlDataStream *ds, ArvGentlBuffer *buf);

/* Pixel format mapping */
uint64_t arv_gentl_pixel_format_from_aravis (guint32 aravis_pixel_format);

/* Interface helpers */
const char *arv_gentl_interface_get_id (ArvInterface *iface);
const char *arv_gentl_tltype_from_protocol (const char *protocol);

/*
Map handle to ArvGentlHandleEvents (extended).
*/
struct ArvGentlHandleEvents {
	/* EVENT_MODULE is the last one in EVENT_TYPE_LIST */
	ArvGentlEvent *ev[EVENT_MODULE + 1];
};

extern GHashTable *gentl_events;
extern GMutex gentl_mutex;

#endif
