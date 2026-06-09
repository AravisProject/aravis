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

/* ========================================================================== */
/* EventGetData                                                               */
/* ========================================================================== */

GC_ERROR
EventGetData (EVENT_HANDLE hEvent, void *pBuffer, size_t *piSize, uint64_t iTimeout)
{
	ArvGentlEvent *ev = (ArvGentlEvent *) hEvent;
	ArvGentlDataStream *ds;
	EVENT_NEW_BUFFER_DATA *new_buf_data;
	gint64 end_time;
	ArvGentlBuffer *buf = NULL;
	GC_ERROR result;

	if (ev == NULL || !ARV_IS_GENTL_EVENT (ev))
		return GC_ERR_INVALID_HANDLE;
	g_object_ref (ev);
	if (pBuffer == NULL || piSize == NULL) {
		result = GC_ERR_INVALID_PARAMETER;
		goto out;
	}

	arv_trace_gentl ("%s (hEvent=%p, piSize=%" G_GSIZE_FORMAT ", iTimeout=%" G_GUINT64_FORMAT ")",
			 __FUNCTION__, hEvent, *piSize, (guint64) iTimeout);

	/* Only EVENT_NEW_BUFFER is supported */
	if (ev->event_type != EVENT_NEW_BUFFER) {
		arv_warning_gentl ("%s: event type %d not supported", __FUNCTION__, ev->event_type);
		result = GC_ERR_NOT_IMPLEMENTED;
		goto out;
	}

	/* Check buffer size */
	if (*piSize < sizeof (EVENT_NEW_BUFFER_DATA)) {
		arv_warning_gentl ("%s: buffer too small (%zu < %zu)",
				   __FUNCTION__, *piSize, sizeof (EVENT_NEW_BUFFER_DATA));
		*piSize = sizeof (EVENT_NEW_BUFFER_DATA);
		result = GC_ERR_BUFFER_TOO_SMALL;
		goto out;
	}

	ds = ev->data_stream;
	if (ds == NULL) {
		result = GC_ERR_INVALID_HANDLE;
		goto out;
	}

	/* Wait for a buffer to be available */
	g_mutex_lock (&ds->lock);

	ev->state = ARV_GENTL_EVENT_STATE_WAITING;

	if (iTimeout == GENTL_INFINITE) {
		/* Wait indefinitely */
		while (g_queue_is_empty (ds->output_queue) && !ds->stop_thread &&
		       ev->state != ARV_GENTL_EVENT_STATE_KILLED) {
			g_cond_wait (&ds->cond, &ds->lock);
		}
	} else {
		/* Wait with timeout. The GenTL EventGetData timeout is given in
		 * milliseconds; g_get_monotonic_time() works in microseconds. */
		gint64 now = g_get_monotonic_time ();
		guint64 timeout_us = iTimeout > G_MAXINT64 / 1000 ?
					     G_MAXINT64 :
					     iTimeout * 1000;
		end_time = timeout_us > (guint64) (G_MAXINT64 - now) ?
				   G_MAXINT64 :
				   now + (gint64) timeout_us;
		while (g_queue_is_empty (ds->output_queue) && !ds->stop_thread &&
		       ev->state != ARV_GENTL_EVENT_STATE_KILLED) {
			if (!g_cond_wait_until (&ds->cond, &ds->lock, end_time))
				break; /* timed out — fall through to the checks below */
		}
	}

	/* Check for killed */
	if (ev->state == ARV_GENTL_EVENT_STATE_KILLED) {
		ev->state = ARV_GENTL_EVENT_STATE_IDLE;
		g_mutex_unlock (&ds->lock);
		arv_trace_gentl ("%s: event killed", __FUNCTION__);
		result = GC_ERR_ABORT;
		goto out;
	}

	/* If a buffer arrived, deliver it even if acquisition is being stopped. */
	if (g_queue_is_empty (ds->output_queue)) {
		gboolean stopped = ds->stop_thread;
		ev->state = ARV_GENTL_EVENT_STATE_IDLE;
		g_mutex_unlock (&ds->lock);
		if (stopped) {
			arv_trace_gentl ("%s: acquisition stopped, aborting wait", __FUNCTION__);
			result = GC_ERR_ABORT;
			goto out;
		}
		arv_trace_gentl ("%s: timeout after %" G_GUINT64_FORMAT " ms", __FUNCTION__, (guint64) iTimeout);
		result = GC_ERR_TIMEOUT;
		goto out;
	}

	/* Get buffer from output queue */
	buf = g_queue_pop_head (ds->output_queue);

	/* Fill EVENT_NEW_BUFFER_DATA */
	new_buf_data = (EVENT_NEW_BUFFER_DATA *) pBuffer;
	new_buf_data->BufferHandle = (BUFFER_HANDLE) buf;
	new_buf_data->UserPointer = buf->user_ptr;
	*piSize = sizeof (EVENT_NEW_BUFFER_DATA);

	ev->num_fired++;
	ev->state = ARV_GENTL_EVENT_STATE_IDLE;

	arv_trace_gentl ("%s: returning buffer %p, frame_id=%" G_GUINT64_FORMAT,
			 __FUNCTION__, buf, buf->frame_id);
	g_mutex_unlock (&ds->lock);

	result = GC_ERR_SUCCESS;

out:
	g_object_unref (ev);
	return result;
}

/* ========================================================================== */
/* EventGetDataInfo                                                           */
/* ========================================================================== */

GC_ERROR
EventGetDataInfo (EVENT_HANDLE hEvent, const void *pInBuffer, size_t iInSize, EVENT_DATA_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pOutBuffer, size_t *piOutSize)
{
	ArvGentlEvent *ev = (ArvGentlEvent *) hEvent;
	const EVENT_NEW_BUFFER_DATA *data;

	if (ev == NULL || !ARV_IS_GENTL_EVENT (ev))
		return GC_ERR_INVALID_HANDLE;
	if (pInBuffer == NULL || piOutSize == NULL)
		return GC_ERR_INVALID_PARAMETER;
	if (iInSize < sizeof (EVENT_NEW_BUFFER_DATA))
		return GC_ERR_BUFFER_TOO_SMALL;

	arv_trace_gentl ("%s (hEvent=%p, iInfoCmd=%d)", __FUNCTION__, hEvent, iInfoCmd);

	if (ev->event_type != EVENT_NEW_BUFFER)
		return GC_ERR_NOT_IMPLEMENTED;

	data = (const EVENT_NEW_BUFFER_DATA *) pInBuffer;

	switch (iInfoCmd) {
		case EVENT_DATA_ID: {
			int32_t v = EVENT_NEW_BUFFER;
			return gentl_to_buf (INFO_DATATYPE_INT32, pOutBuffer, &v, piOutSize, piType);
		}
		case EVENT_DATA_VALUE:
			/* For NEW_BUFFER, value is the buffer handle */
			return gentl_to_buf (INFO_DATATYPE_PTR, pOutBuffer, &data->BufferHandle, piOutSize, piType);
		case EVENT_DATA_NUMID: {
			uint64_t v = ev->num_fired;
			return gentl_to_buf (INFO_DATATYPE_UINT64, pOutBuffer, &v, piOutSize, piType);
		}
		default:
			return GC_ERR_NOT_IMPLEMENTED;
	}
}

/* ========================================================================== */
/* EventGetInfo                                                               */
/* ========================================================================== */

GC_ERROR
EventGetInfo (EVENT_HANDLE hEvent, EVENT_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize)
{
	ArvGentlEvent *ev = (ArvGentlEvent *) hEvent;

	if (ev == NULL || !ARV_IS_GENTL_EVENT (ev))
		return GC_ERR_INVALID_HANDLE;

	arv_trace_gentl ("%s (hEvent=%p, iInfoCmd=%d)", __FUNCTION__, hEvent, iInfoCmd);

	switch (iInfoCmd) {
		case EVENT_EVENT_TYPE: {
			int32_t v = ev->event_type;
			return gentl_to_buf (INFO_DATATYPE_INT32, pBuffer, &v, piSize, piType);
		}
		case EVENT_NUM_IN_QUEUE: {
			size_t v = 0;
			if (ev->data_stream != NULL) {
				g_mutex_lock (&ev->data_stream->lock);
				v = ev->data_stream->output_queue->length;
				g_mutex_unlock (&ev->data_stream->lock);
			}
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case EVENT_NUM_FIRED: {
			uint64_t v;
			if (ev->data_stream != NULL) {
				g_mutex_lock (&ev->data_stream->lock);
				v = ev->num_fired;
				g_mutex_unlock (&ev->data_stream->lock);
			} else {
				v = ev->num_fired;
			}
			return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &v, piSize, piType);
		}
		case EVENT_SIZE_MAX:
		case EVENT_INFO_DATA_SIZE_MAX: {
			size_t v = sizeof (EVENT_NEW_BUFFER_DATA);
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		default:
			GENTL_NYI_DETAIL ("iInfoCmd=%d", iInfoCmd);
	}
}

/* ========================================================================== */
/* EventFlush                                                                 */
/* ========================================================================== */

GC_ERROR
EventFlush (EVENT_HANDLE hEvent)
{
	ArvGentlEvent *ev = (ArvGentlEvent *) hEvent;
	ArvGentlDataStream *ds;

	if (ev == NULL || !ARV_IS_GENTL_EVENT (ev))
		return GC_ERR_INVALID_HANDLE;

	arv_trace_gentl ("%s (hEvent=%p)", __FUNCTION__, hEvent);

	ds = ev->data_stream;
	if (ds == NULL)
		return GC_ERR_SUCCESS;

	/* Discard all pending output buffers without destroying them */
	g_mutex_lock (&ds->lock);
	while (!g_queue_is_empty (ds->output_queue)) {
		ArvGentlBuffer *buf = g_queue_pop_head (ds->output_queue);
		buf->state = ARV_GENTL_BUFFER_ANNOUNCED;
		buf->size_filled = 0;
		buf->data_size = 0;
		buf->is_incomplete = FALSE;
	}
	g_mutex_unlock (&ds->lock);

	return GC_ERR_SUCCESS;
}

/* ========================================================================== */
/* EventKill                                                                  */
/* ========================================================================== */

GC_ERROR
EventKill (EVENT_HANDLE hEvent)
{
	ArvGentlEvent *ev = (ArvGentlEvent *) hEvent;

	if (ev == NULL || !ARV_IS_GENTL_EVENT (ev))
		return GC_ERR_INVALID_HANDLE;

	arv_trace_gentl ("%s (hEvent=%p)", __FUNCTION__, hEvent);

	/* Wake up any waiting EventGetData. EventGetData uses the DataStream lock,
	 * so the state and condition must be updated under that same lock. */
	if (ev->data_stream != NULL) {
		g_mutex_lock (&ev->data_stream->lock);
		ev->state = ARV_GENTL_EVENT_STATE_KILLED;
		g_cond_broadcast (&ev->data_stream->cond);
		g_mutex_unlock (&ev->data_stream->lock);
	} else {
		ev->state = ARV_GENTL_EVENT_STATE_KILLED;
	}

	return GC_ERR_SUCCESS;
}
