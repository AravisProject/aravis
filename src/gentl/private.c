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

#include "private.h"
#include <arvcamera.h>
#include <arvstream.h>
#include <arvbuffer.h>
#include <arvsystem.h>
#include <arvinterface.h>

/* ========================================================================== */
/* ArvGentlEvent                                                              */
/* ========================================================================== */

G_DEFINE_TYPE (ArvGentlEvent, arv_gentl_event, G_TYPE_OBJECT);

static void
arv_gentl_event_finalize (GObject *object)
{
	ArvGentlEvent *self = ARV_GENTL_EVENT (object);

	g_clear_object (&self->data_stream);

	G_OBJECT_CLASS (arv_gentl_event_parent_class)->finalize (object);
}

static void
arv_gentl_event_class_init (ArvGentlEventClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = arv_gentl_event_finalize;
}

static void
arv_gentl_event_init (ArvGentlEvent *self)
{
	self->event_type = EVENT_ERROR;
	self->event_src = NULL;
	self->data_stream = NULL;
	self->state = ARV_GENTL_EVENT_STATE_IDLE;
	self->num_fired = 0;
}

/* ========================================================================== */
/* ArvTransportLayer                                                          */
/* ========================================================================== */

G_DEFINE_TYPE (ArvTransportLayer, arv_transport_layer, G_TYPE_OBJECT);

static void
arv_transport_layer_class_init (ArvTransportLayerClass *self)
{
}

static void
arv_transport_layer_init (ArvTransportLayer *self)
{
}

/* ========================================================================== */
/* ArvGentlDataStream                                                         */
/* ========================================================================== */

G_DEFINE_TYPE (ArvGentlDataStream, arv_gentl_data_stream, G_TYPE_OBJECT);

static void
arv_gentl_data_stream_init (ArvGentlDataStream *self)
{
	self->camera = NULL;
	self->stream = NULL;
	self->stream_id = NULL;

	g_mutex_init (&self->lock);
	g_cond_init (&self->cond);

	self->announced_buffers = g_queue_new ();
	self->input_queue = g_queue_new ();
	self->output_queue = g_queue_new ();

	self->acquiring = FALSE;
	self->stop_thread = FALSE;
	self->acquisition_complete = FALSE;
	self->num_to_acquire = GENTL_INFINITE;
	self->acquisition_delivered_count = 0;

	self->worker_thread = NULL;

	self->frame_id_counter = 0;
	self->started_count = 0;
	self->delivered_count = 0;
	self->lost_count = 0;

	self->payload_size = 0;
	self->width = 0;
	self->height = 0;
	self->pixel_format = 0;

	self->n_internal_buffers = 10;
}

static void
arv_gentl_data_stream_finalize (GObject *object)
{
	ArvGentlDataStream *self = ARV_GENTL_DATA_STREAM (object);

	arv_trace_gentl ("%s (%p) finalizing", __FUNCTION__, self);

	/* Stop acquisition if still running */
	if (self->acquiring) {
		g_mutex_lock (&self->lock);
		self->stop_thread = TRUE;
		g_cond_broadcast (&self->cond);
		g_mutex_unlock (&self->lock);

		if (self->camera != NULL)
			arv_camera_stop_acquisition (self->camera, NULL);

		if (self->worker_thread != NULL) {
			g_thread_join (self->worker_thread);
			self->worker_thread = NULL;
		}
	}

	/* Free all announced buffers */
	while (!g_queue_is_empty (self->announced_buffers)) {
		ArvGentlBuffer *buf = g_queue_pop_head (self->announced_buffers);
		if (buf->owns_memory && buf->base != NULL) {
			g_free (buf->base);
		}
		g_free (buf);
	}

	/* Free remaining queued and output buffers (they are in announced_buffers too) */
	g_queue_free (self->input_queue);
	g_queue_free (self->output_queue);
	g_queue_free (self->announced_buffers);

	/* The stream owns every ArvBuffer pushed to it. */
	g_clear_object (&self->stream);

	/* Release camera */
	g_clear_object (&self->camera);

	g_free (self->stream_id);

	g_mutex_clear (&self->lock);
	g_cond_clear (&self->cond);

	G_OBJECT_CLASS (arv_gentl_data_stream_parent_class)->finalize (object);

	arv_trace_gentl ("%s (%p) finalized", __FUNCTION__, self);
}

static void
arv_gentl_data_stream_class_init (ArvGentlDataStreamClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = arv_gentl_data_stream_finalize;
}

/* ========================================================================== */
/* Utility functions                                                          */
/* ========================================================================== */

ArvGentlDataStream *
arv_gentl_data_stream_new (ArvCamera *camera, const char *stream_id)
{
	ArvGentlDataStream *ds;

	ds = g_object_new (ARV_TYPE_GENTL_DATA_STREAM, NULL);
	ds->camera = g_object_ref (camera);
	ds->stream_id = g_strdup (stream_id);

	arv_trace_gentl ("%s: created DS %p for camera %p, stream_id=%s",
			 __FUNCTION__, ds, camera, stream_id);

	return ds;
}

void
arv_gentl_data_stream_attach_buffer (ArvGentlDataStream *ds, ArvGentlBuffer *buf)
{
	g_mutex_lock (&ds->lock);
	g_queue_push_tail (ds->announced_buffers, buf);
	arv_trace_gentl ("%s: buf=%p attached to ds=%p (total announced=%u)",
			 __FUNCTION__, buf, ds, ds->announced_buffers->length);
	g_mutex_unlock (&ds->lock);
}

void
arv_gentl_data_stream_detach_buffer (ArvGentlDataStream *ds, ArvGentlBuffer *buf)
{
	g_mutex_lock (&ds->lock);
	g_queue_remove (ds->announced_buffers, buf);
	g_queue_remove (ds->input_queue, buf);
	g_queue_remove (ds->output_queue, buf);
	g_mutex_unlock (&ds->lock);
}

/* ========================================================================== */
/* Pixel format mapping (minimal — just what Harvester needs)                 */
/* ========================================================================== */

uint64_t
arv_gentl_pixel_format_from_aravis (guint32 aravis_pixel_format)
{
	/* ArvPixelFormat values use the PFNC 32-bit namespace. */
	return aravis_pixel_format;
}

/* ========================================================================== */
/* Interface helpers                                                          */
/* ========================================================================== */

/* Return the Aravis interface id string ("Fake", "GigEVision", ...) for an
 * opened ArvInterface handle, or NULL if not found. The ArvInterface instances
 * are singletons, so pointer comparison against arv_get_interface() works. */
const char *
arv_gentl_interface_get_id (ArvInterface *iface)
{
	unsigned int i, n;

	if (iface == NULL)
		return NULL;

	n = arv_get_n_interfaces ();
	for (i = 0; i < n; i++) {
		if (arv_get_interface_by_id (arv_get_interface_id (i)) == iface)
			return arv_get_interface_id (i);
	}

	return NULL;
}

/* Map an Aravis protocol string to a GenTL standard TLType string. */
const char *
arv_gentl_tltype_from_protocol (const char *protocol)
{
	if (protocol == NULL)
		return "Custom";
	if (g_strcmp0 (protocol, "GigEVision") == 0 || g_strcmp0 (protocol, "Fake") == 0)
		return "GEV";
	if (g_strcmp0 (protocol, "USB3Vision") == 0)
		return "U3V";
	if (g_strcmp0 (protocol, "Mixed") == 0 || g_strcmp0 (protocol, "GenTL") == 0)
		return "Mixed";

	return "Custom";
}

/* ========================================================================== */
/* Global variables                                                           */
/* ========================================================================== */

/* global variables */
int gentl_GCInitLib = 0;

/* singleton for the transport layer */
ArvTransportLayer* gentl_transport_layer = NULL;

/* error data must be thread-local as per GenTL spec */
GENTL_THREAD_LOCAL_STORAGE GError* gentl_err = NULL;

/* Map handle (pointer) to ArvGentlHandleEvents containing all events associated to this handle. */
GHashTable *gentl_events = NULL;
GMutex gentl_mutex;

static void
arv_gentl_handle_events_free (gpointer data)
{
	struct ArvGentlHandleEvents *events = data;
	unsigned int i;

	for (i = EVENT_ERROR; i <= EVENT_MODULE; i++)
		g_clear_object (&events->ev[i]);
	g_free (events);
}

GQuark
gentl_error_quark (void)
{
  return g_quark_from_static_string ("gentl-error-quark");
}

GC_ERROR
gentl_init (void)
{
	GC_ERROR result = GC_ERR_SUCCESS;

	g_mutex_lock (&gentl_mutex);
	if (gentl_GCInitLib != 0) {
		result = GC_ERR_RESOURCE_IN_USE;
	} else {
		gentl_events = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL,
						      arv_gentl_handle_events_free);
		g_atomic_int_set (&gentl_GCInitLib, 1);
	}
	g_mutex_unlock (&gentl_mutex);

	return result;
}

GC_ERROR
gentl_fini (void)
{
	GC_ERROR result = GC_ERR_SUCCESS;

	g_mutex_lock (&gentl_mutex);
	if (gentl_GCInitLib == 0) {
		result = GC_ERR_NOT_INITIALIZED;
	} else {
		g_atomic_int_set (&gentl_GCInitLib, 0);
		g_clear_pointer (&gentl_events, g_hash_table_destroy);
	}
	g_mutex_unlock (&gentl_mutex);

	return result;
}

gboolean
gentl_is_initialized (void)
{
	return g_atomic_int_get (&gentl_GCInitLib) == 1;
}
