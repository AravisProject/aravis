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

/* ========================================================================== */
/* Worker thread                                                              */
/* ========================================================================== */

static gpointer
arv_gentl_stream_worker (gpointer user_data)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) user_data;

	arv_trace_gentl ("%s: worker thread started for DS=%p", __FUNCTION__, ds);

	while (TRUE) {
		ArvBuffer *arv_buffer;
		ArvGentlBuffer *gentl_buf;
		const void *image_data;
		size_t image_size;
		ArvBufferStatus status;
		guint w;
		guint h;
		guint64 ts;
		guint64 fid;
		ArvPixelFormat pf;

		g_mutex_lock (&ds->lock);
		if (ds->stop_thread) {
			g_mutex_unlock (&ds->lock);
			break;
		}
		g_mutex_unlock (&ds->lock);

		/* Pop buffer from Aravis stream with timeout */
		arv_buffer = arv_stream_timeout_pop_buffer (ds->stream, 500000);

		if (arv_buffer == NULL)
			continue; /* timeout, check stop_thread */

		g_mutex_lock (&ds->lock);
		if (ds->stop_thread) {
			g_mutex_unlock (&ds->lock);
			arv_stream_push_buffer (ds->stream, arv_buffer);
			break;
		}
		g_mutex_unlock (&ds->lock);

		/* Get buffer status and image data */
		status = arv_buffer_get_status (arv_buffer);

		if (status == ARV_BUFFER_STATUS_SUCCESS) {
			image_data = arv_buffer_get_image_data (arv_buffer, &image_size);
			w = arv_buffer_get_image_width (arv_buffer);
			h = arv_buffer_get_image_height (arv_buffer);
			ts = arv_buffer_get_timestamp (arv_buffer);
			fid = arv_buffer_get_frame_id (arv_buffer);
			pf = arv_buffer_get_image_pixel_format (arv_buffer);

			arv_trace_gentl ("%s: got image: %u×%u, fmt=0x%x, size=%" G_GSIZE_FORMAT ", frame=%" G_GUINT64_FORMAT,
					 __FUNCTION__, w, h, pf, image_size, fid);

			g_mutex_lock (&ds->lock);
			ds->width = w;
			ds->height = h;
			ds->payload_size = image_size;
			ds->pixel_format = arv_gentl_pixel_format_from_aravis (pf);
			ds->frame_id_counter = fid;
			ds->started_count++;

			/* Try to get a queued buffer and reserve it while locked. */
			gentl_buf = g_queue_pop_head (ds->input_queue);
			if (gentl_buf != NULL)
				gentl_buf->state = ARV_GENTL_BUFFER_FILLING;
			g_mutex_unlock (&ds->lock);

			if (gentl_buf != NULL) {
				size_t copy_size;
				gboolean incomplete = FALSE;
				guint64 delivered_count;

				/* Determine copy size */
				if (image_size <= gentl_buf->size) {
					copy_size = image_size;
				} else {
					arv_warning_gentl ("%s: buffer too small for image (%zu < %zu)",
							   __FUNCTION__, gentl_buf->size, image_size);
					copy_size = gentl_buf->size;
					incomplete = TRUE;
				}

				/* Copy image data */
				memcpy (gentl_buf->base, image_data, copy_size);

				/* Fill buffer metadata */
				g_mutex_lock (&ds->lock);
				gentl_buf->size_filled = copy_size;
				gentl_buf->data_size = image_size;
				gentl_buf->frame_id = fid;
				gentl_buf->timestamp_ns = ts;
				gentl_buf->width = w;
				gentl_buf->height = h;
				gentl_buf->pixel_format = ds->pixel_format;
				gentl_buf->is_incomplete = incomplete;
				gentl_buf->state = ARV_GENTL_BUFFER_FILLED;
				g_queue_push_tail (ds->output_queue, gentl_buf);
				ds->delivered_count++;
				ds->acquisition_delivered_count++;
				if (ds->num_to_acquire != GENTL_INFINITE &&
				    ds->acquisition_delivered_count >= ds->num_to_acquire) {
					ds->acquisition_complete = TRUE;
					ds->stop_thread = TRUE;
				}
				delivered_count = ds->delivered_count;
				g_cond_broadcast (&ds->cond);
				g_mutex_unlock (&ds->lock);

				arv_trace_gentl ("%s: buffer %p filled and pushed to output_queue (total delivered=%" G_GUINT64_FORMAT ")",
						 __FUNCTION__, gentl_buf, delivered_count);
			} else {
				guint64 lost_count;

				g_mutex_lock (&ds->lock);
				ds->lost_count++;
				lost_count = ds->lost_count;
				g_mutex_unlock (&ds->lock);
				arv_warning_gentl ("%s: no queued buffer available, frame lost (lost=%" G_GUINT64_FORMAT ")",
						   __FUNCTION__, lost_count);
			}
		} else {
			arv_trace_gentl ("%s: buffer status=%d, skipping", __FUNCTION__, status);
		}

		/* Push ArvBuffer back to stream for reuse */
		arv_stream_push_buffer (ds->stream, arv_buffer);
	}

	arv_trace_gentl ("%s: worker thread exiting for DS=%p", __FUNCTION__, ds);

	return NULL;
}

/* ========================================================================== */
/* Internal helpers                                                           */
/* ========================================================================== */

static ArvGentlBuffer *
find_buffer_in_queue (GQueue *queue, BUFFER_HANDLE hBuffer)
{
	GList *iter;

	for (iter = queue->head; iter != NULL; iter = iter->next) {
		if ((BUFFER_HANDLE) iter->data == hBuffer)
			return (ArvGentlBuffer *) iter->data;
	}
	return NULL;
}

/* ========================================================================== */
/* DSGetInfo                                                                  */
/* ========================================================================== */

GC_ERROR
DSGetInfo (DS_HANDLE hDataStream, STREAM_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;

	_DS_CHECK_HANDLE;

	arv_trace_gentl ("%s (hDataStream=%p, iInfoCmd=%d)", __FUNCTION__, hDataStream, iInfoCmd);

	switch (iInfoCmd) {
		case STREAM_INFO_ID:
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, ds->stream_id, piSize, piType);

		case STREAM_INFO_NUM_DELIVERED: {
			uint64_t v;
			g_mutex_lock (&ds->lock);
			v = ds->delivered_count;
			g_mutex_unlock (&ds->lock);
			return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &v, piSize, piType);
		}
		case STREAM_INFO_NUM_UNDERRUN: {
			uint64_t v;
			g_mutex_lock (&ds->lock);
			v = ds->lost_count;
			g_mutex_unlock (&ds->lock);
			return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &v, piSize, piType);
		}
		case STREAM_INFO_NUM_ANNOUNCED: {
			size_t v;
			g_mutex_lock (&ds->lock);
			v = ds->announced_buffers->length;
			g_mutex_unlock (&ds->lock);
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case STREAM_INFO_NUM_QUEUED: {
			size_t v;
			GList *iter;
			g_mutex_lock (&ds->lock);
			v = ds->input_queue->length;
			for (iter = ds->announced_buffers->head; iter != NULL; iter = iter->next) {
				ArvGentlBuffer *buf = iter->data;
				if (buf->state == ARV_GENTL_BUFFER_FILLING)
					v++;
			}
			g_mutex_unlock (&ds->lock);
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case STREAM_INFO_NUM_AWAIT_DELIVERY: {
			size_t v;
			g_mutex_lock (&ds->lock);
			v = ds->output_queue->length;
			g_mutex_unlock (&ds->lock);
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case STREAM_INFO_NUM_STARTED: {
			uint64_t v;
			g_mutex_lock (&ds->lock);
			v = ds->started_count;
			g_mutex_unlock (&ds->lock);
			return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &v, piSize, piType);
		}
		case STREAM_INFO_PAYLOAD_SIZE: {
			size_t v;
			g_mutex_lock (&ds->lock);
			v = ds->payload_size;
			g_mutex_unlock (&ds->lock);
			/* The consumer queries the payload size before starting acquisition
			 * (to size the buffers it announces). If we have not computed it yet,
			 * ask the camera now using the current ROI / pixel format. */
			if (v == 0 && ds->camera != NULL) {
				GError *err = NULL;
				guint payload = arv_camera_get_payload (ds->camera, &err);
				if (err != NULL) {
					arv_warning_gentl ("%s: arv_camera_get_payload failed: %s",
							   __FUNCTION__, err->message);
					g_clear_error (&err);
				} else {
					g_mutex_lock (&ds->lock);
					ds->payload_size = payload;
					g_mutex_unlock (&ds->lock);
					v = payload;
				}
			}
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case STREAM_INFO_IS_GRABBING: {
			bool8_t v;
			g_mutex_lock (&ds->lock);
			v = ds->acquiring && !ds->stop_thread ? 1 : 0;
			g_mutex_unlock (&ds->lock);
			return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
		}
		case STREAM_INFO_DEFINES_PAYLOADSIZE: {
			bool8_t v = 1; /* we define payload size */
			return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
		}
		case STREAM_INFO_TLTYPE:
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer,
					     arv_camera_is_uv_device (ds->camera) ? "U3V" : "GEV",
					     piSize, piType);

		case STREAM_INFO_BUF_ANNOUNCE_MIN: {
			/* minimum number of buffers the consumer must announce/queue */
			size_t v = 1;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case STREAM_INFO_BUF_ALIGNMENT: {
			/* no special alignment requirement for our copy-based buffers */
			size_t v = 1;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case STREAM_INFO_NUM_CHUNKS_MAX: {
			size_t v = 0;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		default:
			GENTL_NYI_DETAIL ("iInfoCmd=%d", iInfoCmd);
	}
}

/* ========================================================================== */
/* DSAnnounceBuffer                                                           */
/* ========================================================================== */

GC_ERROR
DSAnnounceBuffer (DS_HANDLE hDataStream, void *pBuffer, size_t iSize, void *pPrivate, BUFFER_HANDLE *phBuffer)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;
	ArvGentlBuffer *buf;

	_DS_CHECK_HANDLE;

	arv_trace_gentl ("%s (hDataStream=%p, pBuffer=%p, iSize=%zu)",
			 __FUNCTION__, hDataStream, pBuffer, iSize);

	if (pBuffer == NULL || phBuffer == NULL)
		return GC_ERR_INVALID_PARAMETER;
	*phBuffer = NULL;
	if (iSize == 0)
		return GC_ERR_INVALID_PARAMETER;
	if (ds->payload_size > 0 && iSize < ds->payload_size)
		return GC_ERR_BUFFER_TOO_SMALL;

	buf = g_new0 (ArvGentlBuffer, 1);
	buf->base = pBuffer;
	buf->size = iSize;
	buf->user_ptr = pPrivate;
	buf->owns_memory = FALSE;
	buf->state = ARV_GENTL_BUFFER_ANNOUNCED;
	buf->is_incomplete = FALSE;

	arv_gentl_data_stream_attach_buffer (ds, buf);

	*phBuffer = (BUFFER_HANDLE) buf;

	arv_trace_gentl ("   (returning BUFFER_HANDLE=%p)", buf);

	return GC_ERR_SUCCESS;
}

/* ========================================================================== */
/* DSAllocAndAnnounceBuffer                                                   */
/* ========================================================================== */

GC_ERROR
DSAllocAndAnnounceBuffer (DS_HANDLE hDataStream, size_t iBufferSize, void *pPrivate, BUFFER_HANDLE *phBuffer)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;
	ArvGentlBuffer *buf;

	_DS_CHECK_HANDLE;

	arv_trace_gentl ("%s (hDataStream=%p, iBufferSize=%zu)",
			 __FUNCTION__, hDataStream, iBufferSize);

	if (phBuffer == NULL)
		return GC_ERR_INVALID_PARAMETER;
	*phBuffer = NULL;
	if (iBufferSize == 0)
		return GC_ERR_INVALID_PARAMETER;
	if (ds->payload_size > 0 && iBufferSize < ds->payload_size)
		return GC_ERR_BUFFER_TOO_SMALL;

	buf = g_new0 (ArvGentlBuffer, 1);
	buf->base = g_try_malloc (iBufferSize);
	if (buf->base == NULL) {
		g_free (buf);
		g_clear_error (&gentl_err);
		gentl_err = g_error_new (GENTL_ERROR, GC_ERR_OUT_OF_MEMORY,
					 "%s: failed to allocate buffer (%zu bytes)",
					 __FUNCTION__, iBufferSize);
		return GC_ERR_OUT_OF_MEMORY;
	}

	buf->size = iBufferSize;
	buf->user_ptr = pPrivate;
	buf->owns_memory = TRUE;
	buf->state = ARV_GENTL_BUFFER_ANNOUNCED;
	buf->is_incomplete = FALSE;

	arv_gentl_data_stream_attach_buffer (ds, buf);

	*phBuffer = (BUFFER_HANDLE) buf;

	arv_trace_gentl ("   (returning BUFFER_HANDLE=%p)", buf);

	return GC_ERR_SUCCESS;
}

/* ========================================================================== */
/* DSAnnounceCompositeBuffer                                                  */
/* ========================================================================== */

GC_ERROR
DSAnnounceCompositeBuffer (DS_HANDLE hDataStream, size_t iNumSegments, void **ppSegments, size_t *piSizes, void *pPrivate, BUFFER_HANDLE *phBuffer)
{
	_DS_CHECK_HANDLE;
	GENTL_NYI;
}

/* ========================================================================== */
/* DSQueueBuffer                                                              */
/* ========================================================================== */

GC_ERROR
DSQueueBuffer (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;
	ArvGentlBuffer *buf = (ArvGentlBuffer *) hBuffer;

	_DS_CHECK_HANDLE;
	_BUF_CHECK_HANDLE;

	arv_trace_gentl ("%s (hDataStream=%p, hBuffer=%p)", __FUNCTION__, hDataStream, hBuffer);

	g_mutex_lock (&ds->lock);
	if (find_buffer_in_queue (ds->announced_buffers, hBuffer) == NULL) {
		g_mutex_unlock (&ds->lock);
		arv_warning_gentl ("%s: buffer %p not found in announced_buffers", __FUNCTION__, hBuffer);
		return GC_ERR_INVALID_HANDLE;
	}
	/* A buffer may be (re)queued when it is freshly announced or after it has
	 * been delivered/filled and the consumer is done with it. Only reject
	 * buffers that are currently queued for or in the middle of acquisition. */
	if (buf->state == ARV_GENTL_BUFFER_FILLING ||
	    find_buffer_in_queue (ds->input_queue, hBuffer) != NULL ||
	    find_buffer_in_queue (ds->output_queue, hBuffer) != NULL) {
		g_mutex_unlock (&ds->lock);
		arv_warning_gentl ("%s: buffer %p already queued (state=%d)",
				   __FUNCTION__, hBuffer, buf->state);
		return GC_ERR_RESOURCE_IN_USE;
	}

	/* reset per-acquisition fields before re-queueing */
	buf->state = ARV_GENTL_BUFFER_QUEUED;
	buf->size_filled = 0;
	buf->data_size = 0;
	buf->is_incomplete = FALSE;

	g_queue_push_tail (ds->input_queue, buf);
	g_mutex_unlock (&ds->lock);

	return GC_ERR_SUCCESS;
}

/* ========================================================================== */
/* DSFlushQueue                                                               */
/* ========================================================================== */

GC_ERROR
DSFlushQueue (DS_HANDLE hDataStream, ACQ_QUEUE_TYPE iOperation)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;

	_DS_CHECK_HANDLE;

	arv_trace_gentl ("%s (hDataStream=%p, iOperation=%d)", __FUNCTION__, hDataStream, iOperation);

	g_mutex_lock (&ds->lock);

	switch (iOperation) {
		case ACQ_QUEUE_INPUT_TO_OUTPUT:
			/* Move all input buffers back to output, marking them as unfilled */
			while (!g_queue_is_empty (ds->input_queue)) {
				ArvGentlBuffer *buf = g_queue_pop_head (ds->input_queue);
				buf->state = ARV_GENTL_BUFFER_ANNOUNCED;
				buf->size_filled = 0;
				buf->data_size = 0;
				buf->is_incomplete = FALSE;
				g_queue_push_tail (ds->output_queue, buf);
			}
			g_cond_broadcast (&ds->cond);
			break;

		case ACQ_QUEUE_OUTPUT_DISCARD:
			/* Discard filled buffers, return them to announced state */
			while (!g_queue_is_empty (ds->output_queue)) {
				ArvGentlBuffer *buf = g_queue_pop_head (ds->output_queue);
				buf->state = ARV_GENTL_BUFFER_ANNOUNCED;
				buf->size_filled = 0;
				buf->data_size = 0;
				buf->is_incomplete = FALSE;
			}
			break;

		case ACQ_QUEUE_ALL_DISCARD:
			/* Discard all queued and output buffers */
			while (!g_queue_is_empty (ds->input_queue)) {
				ArvGentlBuffer *buf = g_queue_pop_head (ds->input_queue);
				buf->state = ARV_GENTL_BUFFER_ANNOUNCED;
				buf->size_filled = 0;
				buf->data_size = 0;
				buf->is_incomplete = FALSE;
			}
			while (!g_queue_is_empty (ds->output_queue)) {
				ArvGentlBuffer *buf = g_queue_pop_head (ds->output_queue);
				buf->state = ARV_GENTL_BUFFER_ANNOUNCED;
				buf->size_filled = 0;
				buf->data_size = 0;
				buf->is_incomplete = FALSE;
			}
			break;

		case ACQ_QUEUE_ALL_TO_INPUT: {
			GList *iter;

			/* A filling buffer cannot be moved atomically without cancelling
			 * the backend transfer. Refuse instead of creating duplicate
			 * ownership between the worker and the input queue. */
			for (iter = ds->announced_buffers->head; iter != NULL; iter = iter->next) {
				ArvGentlBuffer *buf = iter->data;
				if (buf->state == ARV_GENTL_BUFFER_FILLING) {
					g_mutex_unlock (&ds->lock);
					return GC_ERR_RESOURCE_IN_USE;
				}
			}
			g_queue_clear (ds->input_queue);
			g_queue_clear (ds->output_queue);
			for (iter = ds->announced_buffers->head; iter != NULL; iter = iter->next) {
				ArvGentlBuffer *buf = iter->data;
				buf->state = ARV_GENTL_BUFFER_QUEUED;
				buf->size_filled = 0;
				buf->data_size = 0;
				buf->is_incomplete = FALSE;
				g_queue_push_tail (ds->input_queue, buf);
			}
			break;
		}

		case ACQ_QUEUE_UNQUEUED_TO_INPUT: {
			GList *iter;

			for (iter = ds->announced_buffers->head; iter != NULL; iter = iter->next) {
				ArvGentlBuffer *buf = iter->data;
				if (buf->state == ARV_GENTL_BUFFER_FILLING ||
				    find_buffer_in_queue (ds->input_queue, buf) != NULL ||
				    find_buffer_in_queue (ds->output_queue, buf) != NULL)
					continue;
				buf->state = ARV_GENTL_BUFFER_QUEUED;
				buf->size_filled = 0;
				buf->data_size = 0;
				buf->is_incomplete = FALSE;
				g_queue_push_tail (ds->input_queue, buf);
			}
			break;
		}

		default:
			g_mutex_unlock (&ds->lock);
			return GC_ERR_INVALID_PARAMETER;
	}

	g_mutex_unlock (&ds->lock);
	return GC_ERR_SUCCESS;
}

/* ========================================================================== */
/* DSRevokeBuffer                                                             */
/* ========================================================================== */

GC_ERROR
DSRevokeBuffer (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, void **ppBuffer, void **ppPrivate)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;
	ArvGentlBuffer *buf = (ArvGentlBuffer *) hBuffer;

	_DS_CHECK_HANDLE;
	_BUF_CHECK_HANDLE;

	arv_trace_gentl ("%s (hDataStream=%p, hBuffer=%p)", __FUNCTION__, hDataStream, hBuffer);

	g_mutex_lock (&ds->lock);
	if (find_buffer_in_queue (ds->announced_buffers, hBuffer) == NULL ||
	    buf->state == ARV_GENTL_BUFFER_REVOKED) {
		g_mutex_unlock (&ds->lock);
		arv_warning_gentl ("%s: buffer %p already revoked", __FUNCTION__, hBuffer);
		return GC_ERR_INVALID_HANDLE;
	}
	if (buf->state == ARV_GENTL_BUFFER_FILLING ||
	    find_buffer_in_queue (ds->input_queue, hBuffer) != NULL ||
	    find_buffer_in_queue (ds->output_queue, hBuffer) != NULL) {
		g_mutex_unlock (&ds->lock);
		return GC_ERR_RESOURCE_IN_USE;
	}

	/* Return buffer pointer and private data if requested */
	if (ppBuffer != NULL)
		*ppBuffer = buf->base;
	if (ppPrivate != NULL)
		*ppPrivate = buf->user_ptr;

	g_queue_remove (ds->announced_buffers, buf);
	g_queue_remove (ds->input_queue, buf);
	g_queue_remove (ds->output_queue, buf);
	buf->state = ARV_GENTL_BUFFER_REVOKED;
	g_mutex_unlock (&ds->lock);

	/* Free memory if we own it */
	if (buf->owns_memory && buf->base != NULL) {
		g_free (buf->base);
		buf->base = NULL;
	}

	g_free (buf);

	return GC_ERR_SUCCESS;
}

/* ========================================================================== */
/* DSStartAcquisition                                                         */
/* ========================================================================== */

GC_ERROR
DSStartAcquisition (DS_HANDLE hDataStream, ACQ_START_FLAGS iStartFlags, uint64_t iNumToAcquire)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;
	GError *err = NULL;
	guint payload = 0;
	guint n_buffers = 0;

	_DS_CHECK_HANDLE;

	arv_trace_gentl ("%s (hDataStream=%p, iStartFlags=%d, iNumToAcquire=%" G_GUINT64_FORMAT ")",
			 __FUNCTION__, hDataStream, iStartFlags, (guint64) iNumToAcquire);

	if (iStartFlags != ACQ_START_FLAGS_DEFAULT)
		return GC_ERR_INVALID_PARAMETER;
	if (iNumToAcquire == 0)
		return GC_ERR_INVALID_PARAMETER;

	g_mutex_lock (&ds->lock);
	if (ds->acquiring) {
		g_mutex_unlock (&ds->lock);
		arv_warning_gentl ("%s: already acquiring", __FUNCTION__);
		return GC_ERR_RESOURCE_IN_USE;
	}
	g_mutex_unlock (&ds->lock);

	/* Clean up a stream left by a previous failed start attempt. */
	g_clear_object (&ds->stream);

	/* Get payload size before starting (may need to set pixel format first) */
	payload = arv_camera_get_payload (ds->camera, &err);
	if (err != NULL) {
		arv_warning_gentl ("%s: arv_camera_get_payload failed: %s",
				   __FUNCTION__, err->message);
		g_clear_error (&err);
		/* Continue anyway, we'll update later from the first image */
	} else {
		ds->payload_size = payload;
	}

	/* Create internal ArvBuffers and push to stream */
	/* First, create the ArvStream */
	ds->stream = arv_camera_create_stream (ds->camera, NULL, NULL, NULL, &err);
	if (ds->stream == NULL) {
		arv_warning_gentl ("%s: arv_camera_create_stream failed: %s",
				   __FUNCTION__, err ? err->message : "unknown");
		if (err) g_clear_error (&err);
		return GC_ERR_IO;
	}

	/* Create internal buffers and push to stream */
	for (guint i = 0; i < ds->n_internal_buffers; i++) {
		size_t buf_size = payload > 0 ? payload : (1024 * 1024 * 8); /* 8 MB fallback */
		ArvBuffer *arv_buf = arv_buffer_new_allocate (buf_size);
		if (arv_buf != NULL) {
			arv_stream_push_buffer (ds->stream, arv_buf);
			n_buffers++;
		}
	}

	arv_trace_gentl ("%s: created %u internal buffers of %u bytes",
			 __FUNCTION__, n_buffers, payload);

	if (n_buffers == 0) {
		g_clear_object (&ds->stream);
		return GC_ERR_OUT_OF_MEMORY;
	}

	/* Start camera acquisition */
	if (!arv_camera_start_acquisition (ds->camera, &err)) {
		arv_warning_gentl ("%s: arv_camera_start_acquisition failed: %s",
				   __FUNCTION__, err ? err->message : "unknown");
		if (err) g_clear_error (&err);
		g_clear_object (&ds->stream);
		return GC_ERR_IO;
	}

	/* Start worker thread */
	g_mutex_lock (&ds->lock);
	ds->stop_thread = FALSE;
	ds->acquisition_complete = FALSE;
	ds->num_to_acquire = iNumToAcquire;
	ds->acquisition_delivered_count = 0;
	ds->worker_thread = g_thread_new ("arv_gentl_stream_worker",
					  arv_gentl_stream_worker, ds);
	ds->acquiring = TRUE;
	g_mutex_unlock (&ds->lock);

	arv_trace_gentl ("%s: acquisition started, worker thread running", __FUNCTION__);

	return GC_ERR_SUCCESS;
}

/* ========================================================================== */
/* DSStopAcquisition                                                          */
/* ========================================================================== */

GC_ERROR
DSStopAcquisition (DS_HANDLE hDataStream, ACQ_STOP_FLAGS iStopFlags)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;

	_DS_CHECK_HANDLE;

	arv_trace_gentl ("%s (hDataStream=%p, iStopFlags=%d)", __FUNCTION__, hDataStream, iStopFlags);

	if (iStopFlags != ACQ_STOP_FLAGS_DEFAULT && iStopFlags != ACQ_STOP_FLAGS_KILL)
		return GC_ERR_INVALID_PARAMETER;

	g_mutex_lock (&ds->lock);
	if (!ds->acquiring) {
		g_mutex_unlock (&ds->lock);
		arv_warning_gentl ("%s: not acquiring", __FUNCTION__);
		return GC_ERR_RESOURCE_IN_USE;
	}

	/* Signal worker thread to stop */
	ds->stop_thread = TRUE;
	g_cond_broadcast (&ds->cond);
	g_mutex_unlock (&ds->lock);

	/* Stop camera acquisition */
	if (ds->camera != NULL) {
		GError *err = NULL;
		if (!arv_camera_stop_acquisition (ds->camera, &err)) {
			arv_warning_gentl ("%s: arv_camera_stop_acquisition failed: %s",
					   __FUNCTION__, err ? err->message : "unknown");
			if (err) g_clear_error (&err);
		}
	}

	/* Join worker thread */
	if (ds->worker_thread != NULL) {
		g_thread_join (ds->worker_thread);
		ds->worker_thread = NULL;
	}

	g_mutex_lock (&ds->lock);
	ds->acquiring = FALSE;
	ds->acquisition_complete = FALSE;
	ds->num_to_acquire = GENTL_INFINITE;
	ds->acquisition_delivered_count = 0;
	g_mutex_unlock (&ds->lock);

	/* Discard any remaining queued/output buffers */
	DSFlushQueue (hDataStream, ACQ_QUEUE_ALL_DISCARD);

	/* Release stream. Destroying the stream object stops its acquisition
	 * thread (see arv_stream_stop_acquisition docs), so we must not call it
	 * explicitly here — doing so would stop the thread twice and trigger a
	 * "thread != NULL" assertion in the stream backend. */
	g_clear_object (&ds->stream);

	arv_trace_gentl ("%s: acquisition stopped", __FUNCTION__);

	return GC_ERR_SUCCESS;
}

/* ========================================================================== */
/* DSGetBufferInfo                                                            */
/* ========================================================================== */

GC_ERROR
DSGetBufferInfo (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, BUFFER_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;
	ArvGentlBuffer *buf = (ArvGentlBuffer *) hBuffer;
	ArvGentlBuffer info;
	gboolean in_input;
	gboolean in_output;

	_DS_CHECK_HANDLE;
	_BUF_CHECK_HANDLE;

	arv_trace_gentl ("%s (hDataStream=%p, hBuffer=%p, iInfoCmd=%d)",
			 __FUNCTION__, hDataStream, hBuffer, iInfoCmd);

	g_mutex_lock (&ds->lock);
	if (find_buffer_in_queue (ds->announced_buffers, hBuffer) == NULL) {
		g_mutex_unlock (&ds->lock);
		return GC_ERR_INVALID_HANDLE;
	}
	info = *buf;
	in_input = find_buffer_in_queue (ds->input_queue, hBuffer) != NULL;
	in_output = find_buffer_in_queue (ds->output_queue, hBuffer) != NULL;
	g_mutex_unlock (&ds->lock);

	switch (iInfoCmd) {
		case BUFFER_INFO_BASE:
			return gentl_to_buf (INFO_DATATYPE_PTR, pBuffer, &info.base, piSize, piType);

		case BUFFER_INFO_SIZE: {
			size_t v = info.size;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_USER_PTR:
			return gentl_to_buf (INFO_DATATYPE_PTR, pBuffer, &info.user_ptr, piSize, piType);

		case BUFFER_INFO_TIMESTAMP: {
			uint64_t v = info.timestamp_ns;
			return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_TIMESTAMP_NS: {
			uint64_t v = info.timestamp_ns;
			return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_FRAMEID: {
			uint64_t v = info.frame_id;
			return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_SIZE_FILLED: {
			size_t v = info.size_filled;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_IS_INCOMPLETE: {
			bool8_t v = info.is_incomplete ? 1 : 0;
			return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_WIDTH: {
			size_t v = info.width;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_HEIGHT: {
			size_t v = info.height;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_PIXELFORMAT: {
			uint64_t v = info.pixel_format;
			return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_PIXELFORMAT_NAMESPACE: {
			uint64_t v = 4; /* PIXELFORMAT_NAMESPACE_PFNC_32BIT */
			return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_IS_QUEUED: {
			bool8_t v = (in_input || in_output ||
				     info.state == ARV_GENTL_BUFFER_FILLING) ?
					    1 :
					    0;
			return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_IS_ACQUIRING: {
			bool8_t v = info.state == ARV_GENTL_BUFFER_FILLING ? 1 : 0;
			return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_NEW_DATA: {
			bool8_t v = (info.state == ARV_GENTL_BUFFER_FILLED) ? 1 : 0;
			return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_PAYLOADTYPE: {
			size_t v = PAYLOAD_TYPE_IMAGE;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_IMAGEPRESENT: {
			bool8_t v = info.state == ARV_GENTL_BUFFER_FILLED ? 1 : 0;
			return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_DATA_SIZE: {
			size_t v = info.data_size;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_XOFFSET:
		case BUFFER_INFO_YOFFSET:
		case BUFFER_INFO_XPADDING:
		case BUFFER_INFO_YPADDING:
		case BUFFER_INFO_IMAGEOFFSET:
		case BUFFER_INFO_DELIVERED_CHUNKPAYLOADSIZE: {
			size_t v = 0;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_DELIVERED_IMAGEHEIGHT: {
			size_t v = info.height;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_PIXEL_ENDIANNESS: {
			uint64_t v = 0; /* PIXELENDIANNESS_UNKNOWN */
			return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_DATA_LARGER_THAN_BUFFER: {
			bool8_t v = info.data_size > info.size ? 1 : 0;
			return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_CONTAINS_CHUNKDATA:
		case BUFFER_INFO_IS_COMPOSITE: {
			bool8_t v = 0;
			return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
		}
		case BUFFER_INFO_CHUNKLAYOUTID:
		case BUFFER_INFO_FILENAME:
			return GC_ERR_NOT_AVAILABLE;
		case BUFFER_INFO_TLTYPE: {
			const char *v = arv_camera_is_uv_device (ds->camera) ? "U3V" : "GEV";
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, v, piSize, piType);
		}
		default:
			GENTL_NYI_DETAIL ("iInfoCmd=%d", iInfoCmd);
	}
}

/* ========================================================================== */
/* DSGetBufferID                                                              */
/* ========================================================================== */

GC_ERROR
DSGetBufferID (DS_HANDLE hDataStream, uint32_t iIndex, BUFFER_HANDLE *phBuffer)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;

	_DS_CHECK_HANDLE;
	if (phBuffer == NULL)
		return GC_ERR_INVALID_PARAMETER;
	*phBuffer = NULL;

	g_mutex_lock (&ds->lock);
	if (iIndex >= ds->announced_buffers->length) {
		g_mutex_unlock (&ds->lock);
		return GC_ERR_INVALID_INDEX;
	}
	*phBuffer = g_queue_peek_nth (ds->announced_buffers, iIndex);
	g_mutex_unlock (&ds->lock);

	return GC_ERR_SUCCESS;
}

/* ========================================================================== */
/* DSClose                                                                    */
/* ========================================================================== */

GC_ERROR
DSClose (DS_HANDLE hDataStream)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;

	_DS_CHECK_HANDLE;

	arv_trace_gentl ("%s (hDataStream=%p)", __FUNCTION__, hDataStream);

	/* Stop acquisition if still running */
	if (ds->acquiring)
		DSStopAcquisition (hDataStream, ACQ_STOP_FLAGS_DEFAULT);

	/* Unref the GObject — this triggers finalize */
	g_object_unref (ds);

	return GC_ERR_SUCCESS;
}

/* ========================================================================== */
/* DSGetBufferChunkData                                                       */
/* ========================================================================== */

GC_ERROR
DSGetBufferChunkData (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, SINGLE_CHUNK_DATA *pChunkData, size_t *piNumChunks)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;

	_DS_CHECK_HANDLE;
	_BUF_CHECK_HANDLE;

	if (piNumChunks == NULL)
		return GC_ERR_INVALID_PARAMETER;

	g_mutex_lock (&ds->lock);
	if (find_buffer_in_queue (ds->announced_buffers, hBuffer) == NULL) {
		g_mutex_unlock (&ds->lock);
		return GC_ERR_INVALID_HANDLE;
	}
	g_mutex_unlock (&ds->lock);

	(void) pChunkData;
	*piNumChunks = 0;

	return GC_ERR_SUCCESS;
}

/* ========================================================================== */
/* DSGetParentDev                                                             */
/* ========================================================================== */

GC_ERROR
DSGetParentDev (DS_HANDLE hDataStream, DEV_HANDLE *phDevice)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;

	_DS_CHECK_HANDLE;

	arv_trace_gentl ("%s (hDataStream=%p)", __FUNCTION__, hDataStream);

	if (phDevice == NULL)
		return GC_ERR_INVALID_PARAMETER;

	*phDevice = (DEV_HANDLE) ds->camera;

	return GC_ERR_SUCCESS;
}

/* ========================================================================== */
/* DSGetNumBufferParts                                                        */
/* ========================================================================== */

GC_ERROR
DSGetNumBufferParts (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t *piNumParts)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;

	_DS_CHECK_HANDLE;
	_BUF_CHECK_HANDLE;

	if (piNumParts == NULL)
		return GC_ERR_INVALID_PARAMETER;

	g_mutex_lock (&ds->lock);
	if (find_buffer_in_queue (ds->announced_buffers, hBuffer) == NULL) {
		g_mutex_unlock (&ds->lock);
		return GC_ERR_INVALID_HANDLE;
	}
	g_mutex_unlock (&ds->lock);

	/* The producer currently exposes a single image part. */
	*piNumParts = 1;

	return GC_ERR_SUCCESS;
}

/* ========================================================================== */
/* DSGetBufferPartInfo                                                        */
/* ========================================================================== */

GC_ERROR
DSGetBufferPartInfo (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t iPartIndex, BUFFER_PART_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;
	ArvGentlBuffer *buf = (ArvGentlBuffer *) hBuffer;
	ArvGentlBuffer info;

	_DS_CHECK_HANDLE;
	_BUF_CHECK_HANDLE;

	if (iPartIndex >= 1)
		return GC_ERR_INVALID_INDEX;

	arv_trace_gentl ("%s (hDataStream=%p, hBuffer=%p, iPartIndex=%u, iInfoCmd=%d)",
			 __FUNCTION__, hDataStream, hBuffer, iPartIndex, iInfoCmd);

	g_mutex_lock (&ds->lock);
	if (find_buffer_in_queue (ds->announced_buffers, hBuffer) == NULL) {
		g_mutex_unlock (&ds->lock);
		return GC_ERR_INVALID_HANDLE;
	}
	info = *buf;
	g_mutex_unlock (&ds->lock);

	switch (iInfoCmd) {
		case BUFFER_PART_INFO_BASE:
			return gentl_to_buf (INFO_DATATYPE_PTR, pBuffer, &info.base, piSize, piType);

		case BUFFER_PART_INFO_DATA_SIZE:
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &info.size_filled, piSize, piType);

		case BUFFER_PART_INFO_DATA_TYPE: {
			size_t v = 0x0001; /* 2D image */
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case BUFFER_PART_INFO_DATA_FORMAT:
			return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &info.pixel_format, piSize, piType);

		case BUFFER_PART_INFO_DATA_FORMAT_NAMESPACE: {
			uint64_t v = 4; /* PIXELFORMAT_NAMESPACE_PFNC_32BIT */
			return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &v, piSize, piType);
		}
		case BUFFER_PART_INFO_WIDTH: {
			size_t v = info.width;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}

		case BUFFER_PART_INFO_HEIGHT: {
			size_t v = info.height;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case BUFFER_PART_INFO_XOFFSET:
		case BUFFER_PART_INFO_YOFFSET:
		case BUFFER_PART_INFO_XPADDING:
		case BUFFER_PART_INFO_SOURCE_ID: {
			size_t v = 0;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case BUFFER_PART_INFO_DELIVERED_IMAGEHEIGHT: {
			size_t v = info.height;
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &v, piSize, piType);
		}
		case BUFFER_PART_INFO_REGION_ID:
		case BUFFER_PART_INFO_DATA_PURPOSE_ID:
			return GC_ERR_NOT_AVAILABLE;

		default:
			GENTL_NYI_DETAIL ("iInfoCmd=%d", iInfoCmd);
	}
}

/* ========================================================================== */
/* GenTL v1.5+ stubs                                                          */
/* ========================================================================== */

GC_ERROR
DSGetBufferInfoStacked (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, DS_BUFFER_INFO_STACKED *pInfoStacked, size_t iNumInfos)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;
	size_t i;

	_DS_CHECK_HANDLE;
	_BUF_CHECK_HANDLE;

	if (iNumInfos > 0 && pInfoStacked == NULL)
		return GC_ERR_INVALID_PARAMETER;

	g_mutex_lock (&ds->lock);
	if (find_buffer_in_queue (ds->announced_buffers, hBuffer) == NULL) {
		g_mutex_unlock (&ds->lock);
		return GC_ERR_INVALID_HANDLE;
	}
	g_mutex_unlock (&ds->lock);

	for (i = 0; i < iNumInfos; i++) {
		BUFFER_INFO_CMD command;
		INFO_DATATYPE type;
		void *buffer;
		size_t size;
		GC_ERROR result;

		memcpy (&command, &pInfoStacked[i].iInfoCmd, sizeof command);
		memcpy (&type, &pInfoStacked[i].iType, sizeof type);
		memcpy (&buffer, &pInfoStacked[i].pBuffer, sizeof buffer);
		memcpy (&size, &pInfoStacked[i].iSize, sizeof size);
		result = DSGetBufferInfo (hDataStream, hBuffer, command, &type, buffer, &size);
		memcpy (&pInfoStacked[i].iType, &type, sizeof type);
		memcpy (&pInfoStacked[i].iSize, &size, sizeof size);
		memcpy (&pInfoStacked[i].iResult, &result, sizeof result);
	}

	return GC_ERR_SUCCESS;
}

GC_ERROR
DSGetBufferPartInfoStacked (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, DS_BUFFER_PART_INFO_STACKED *pInfoStacked, size_t iNumInfos)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;
	size_t i;

	_DS_CHECK_HANDLE;
	_BUF_CHECK_HANDLE;

	if (iNumInfos > 0 && pInfoStacked == NULL)
		return GC_ERR_INVALID_PARAMETER;

	g_mutex_lock (&ds->lock);
	if (find_buffer_in_queue (ds->announced_buffers, hBuffer) == NULL) {
		g_mutex_unlock (&ds->lock);
		return GC_ERR_INVALID_HANDLE;
	}
	g_mutex_unlock (&ds->lock);

	for (i = 0; i < iNumInfos; i++) {
		uint32_t part_index;
		BUFFER_PART_INFO_CMD command;
		INFO_DATATYPE type;
		void *buffer;
		size_t size;
		GC_ERROR result;

		memcpy (&part_index, &pInfoStacked[i].iPartIndex, sizeof part_index);
		memcpy (&command, &pInfoStacked[i].iInfoCmd, sizeof command);
		memcpy (&type, &pInfoStacked[i].iType, sizeof type);
		memcpy (&buffer, &pInfoStacked[i].pBuffer, sizeof buffer);
		memcpy (&size, &pInfoStacked[i].iSize, sizeof size);
		result = DSGetBufferPartInfo (hDataStream, hBuffer, part_index, command, &type,
					      buffer, &size);
		memcpy (&pInfoStacked[i].iType, &type, sizeof type);
		memcpy (&pInfoStacked[i].iSize, &size, sizeof size);
		memcpy (&pInfoStacked[i].iResult, &result, sizeof result);
	}

	return GC_ERR_SUCCESS;
}

GC_ERROR
DSGetNumFlows (DS_HANDLE hDataStream, uint32_t *piNumFlows)
{
	_DS_CHECK_HANDLE;

	if (piNumFlows == NULL)
		return GC_ERR_INVALID_PARAMETER;

	*piNumFlows = 0;
	return GC_ERR_NO_DATA;
}

GC_ERROR
DSGetFlowInfo (DS_HANDLE hDataStream, uint32_t iFlowIndex, FLOW_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize)
{
	_DS_CHECK_HANDLE;

	(void) iFlowIndex;
	(void) iInfoCmd;
	(void) piType;
	(void) pBuffer;
	(void) piSize;

	return GC_ERR_NO_DATA;
}

GC_ERROR
DSGetNumBufferSegments (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t *piNumSegments)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;

	_DS_CHECK_HANDLE;
	_BUF_CHECK_HANDLE;

	if (piNumSegments == NULL)
		return GC_ERR_INVALID_PARAMETER;

	g_mutex_lock (&ds->lock);
	if (find_buffer_in_queue (ds->announced_buffers, hBuffer) == NULL) {
		g_mutex_unlock (&ds->lock);
		return GC_ERR_INVALID_HANDLE;
	}
	g_mutex_unlock (&ds->lock);

	*piNumSegments = 1;
	return GC_ERR_SUCCESS;
}

GC_ERROR
DSGetBufferSegmentInfo (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t iSegmentIndex, SEGMENT_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize)
{
	ArvGentlDataStream *ds = (ArvGentlDataStream *) hDataStream;
	ArvGentlBuffer *buf = (ArvGentlBuffer *) hBuffer;
	ArvGentlBuffer info;

	_DS_CHECK_HANDLE;
	_BUF_CHECK_HANDLE;

	if (iSegmentIndex != 0)
		return GC_ERR_INVALID_INDEX;

	g_mutex_lock (&ds->lock);
	if (find_buffer_in_queue (ds->announced_buffers, hBuffer) == NULL) {
		g_mutex_unlock (&ds->lock);
		return GC_ERR_INVALID_HANDLE;
	}
	info = *buf;
	g_mutex_unlock (&ds->lock);

	switch (iInfoCmd) {
		case SEGMENT_INFO_BASE:
			return gentl_to_buf (INFO_DATATYPE_PTR, pBuffer, &info.base, piSize, piType);
		case SEGMENT_INFO_SIZE:
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &info.size, piSize, piType);
		case SEGMENT_INFO_IS_INCOMPLETE: {
			bool8_t v = info.is_incomplete ? 1 : 0;
			return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
		}
		case SEGMENT_INFO_SIZE_FILLED:
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &info.size_filled, piSize, piType);
		case SEGMENT_INFO_DATA_SIZE:
			return gentl_to_buf (INFO_DATATYPE_SIZET, pBuffer, &info.data_size, piSize, piType);
		default:
			return GC_ERR_INVALID_PARAMETER;
	}
}
