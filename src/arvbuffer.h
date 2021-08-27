/* Aravis - Digital camera library
 *
 * Copyright © 2009-2019 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_BUFFER_H
#define ARV_BUFFER_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>

G_BEGIN_DECLS

/**
 * ArvBufferStatus:
 * @ARV_BUFFER_STATUS_UNKNOWN: unknown status
 * @ARV_BUFFER_STATUS_SUCCESS: the buffer contains a valid image
 * @ARV_BUFFER_STATUS_CLEARED: the buffer is cleared
 * @ARV_BUFFER_STATUS_TIMEOUT: timeout was reached before all packets are received
 * @ARV_BUFFER_STATUS_MISSING_PACKETS: stream has missing packets
 * @ARV_BUFFER_STATUS_WRONG_PACKET_ID: stream has packet with wrong id
 * @ARV_BUFFER_STATUS_SIZE_MISMATCH: the received image didn't fit in the buffer data space
 * @ARV_BUFFER_STATUS_FILLING: the image is currently being filled
 * @ARV_BUFFER_STATUS_ABORTED: the filling was aborted before completion
 */

typedef enum {
	ARV_BUFFER_STATUS_UNKNOWN = -1,
	ARV_BUFFER_STATUS_SUCCESS = 0,
	ARV_BUFFER_STATUS_CLEARED,
	ARV_BUFFER_STATUS_TIMEOUT,
	ARV_BUFFER_STATUS_MISSING_PACKETS,
	ARV_BUFFER_STATUS_WRONG_PACKET_ID,
	ARV_BUFFER_STATUS_SIZE_MISMATCH,
	ARV_BUFFER_STATUS_FILLING,
	ARV_BUFFER_STATUS_ABORTED
} ArvBufferStatus;

/**
 * ArvBufferPayloadType:
 * @ARV_BUFFER_PAYLOAD_TYPE_UNKNOWN: unknown payload type
 * @ARV_BUFFER_PAYLOAD_TYPE_IMAGE: image data
 * @ARV_BUFFER_PAYLOAD_TYPE_RAWDATA: raw data
 * @ARV_BUFFER_PAYLOAD_TYPE_FILE: file
 * @ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA: chunk data
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA: extended chunk data
 * @ARV_BUFFER_PAYLOAD_TYPE_JPEG: JPEG data
 * @ARV_BUFFER_PAYLOAD_TYPE_JPEG2000: JPEG2000 data
 * @ARV_BUFFER_PAYLOAD_TYPE_H264: h264 data
 * @ARV_BUFFER_PAYLOAD_TYPE_MULTIZONE_IMAGE: multizone image
 * @ARV_BUFFER_PAYLOAD_TYPE_IMAGE_EXTENDED_CHUNK: image and chunk data
*/

typedef enum {
	ARV_BUFFER_PAYLOAD_TYPE_UNKNOWN = 		-1,
	ARV_BUFFER_PAYLOAD_TYPE_IMAGE =			0x0001,
	ARV_BUFFER_PAYLOAD_TYPE_RAWDATA = 		0x0002,
	ARV_BUFFER_PAYLOAD_TYPE_FILE = 			0x0003,
	ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA = 		0x0004,
	ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA = 	0x0005, /* Deprecated */
	ARV_BUFFER_PAYLOAD_TYPE_JPEG = 			0x0006,
	ARV_BUFFER_PAYLOAD_TYPE_JPEG2000 = 		0x0007,
	ARV_BUFFER_PAYLOAD_TYPE_H264 = 			0x0008,
	ARV_BUFFER_PAYLOAD_TYPE_MULTIZONE_IMAGE = 	0x0009,
	ARV_BUFFER_PAYLOAD_TYPE_IMAGE_EXTENDED_CHUNK = 	0x4001
} ArvBufferPayloadType;

#define ARV_TYPE_BUFFER             (arv_buffer_get_type ())
ARV_API G_DECLARE_FINAL_TYPE (ArvBuffer, arv_buffer, ARV, BUFFER, GObject)

typedef void (*ArvFrameCallback)	(ArvBuffer *buffer);

ARV_API ArvBuffer *		arv_buffer_new_allocate		(size_t size);
ARV_API ArvBuffer *		arv_buffer_new			(size_t size, void *preallocated);
ARV_API ArvBuffer * 		arv_buffer_new_full		(size_t size, void *preallocated,
								 void *user_data, GDestroyNotify user_data_destroy_func);

ARV_API ArvBufferStatus		arv_buffer_get_status		(ArvBuffer *buffer);

ARV_API const void *		arv_buffer_get_user_data	(ArvBuffer *buffer);

ARV_API ArvBufferPayloadType	arv_buffer_get_payload_type	(ArvBuffer *buffer);
ARV_API guint64			arv_buffer_get_timestamp	(ArvBuffer *buffer);
ARV_API void			arv_buffer_set_timestamp	(ArvBuffer *buffer, guint64 timestamp_ns);
ARV_API guint64			arv_buffer_get_system_timestamp	(ArvBuffer *buffer);
ARV_API void			arv_buffer_set_system_timestamp	(ArvBuffer *buffer, guint64 timestamp_ns);
ARV_API void			arv_buffer_set_frame_id		(ArvBuffer *buffer, guint64 frame_id);
ARV_API guint64 		arv_buffer_get_frame_id		(ArvBuffer *buffer);
ARV_API const void *		arv_buffer_get_data		(ArvBuffer *buffer, size_t *size);

ARV_API void			arv_buffer_get_image_region		(ArvBuffer *buffer, gint *x, gint *y, gint *width, gint *height);
ARV_API gint			arv_buffer_get_image_width		(ArvBuffer *buffer);
ARV_API gint			arv_buffer_get_image_height		(ArvBuffer *buffer);
ARV_API gint			arv_buffer_get_image_x			(ArvBuffer *buffer);
ARV_API gint			arv_buffer_get_image_y			(ArvBuffer *buffer);
ARV_API ArvPixelFormat		arv_buffer_get_image_pixel_format	(ArvBuffer *buffer);

ARV_API gboolean		arv_buffer_has_chunks		(ArvBuffer *buffer);
ARV_API const void *		arv_buffer_get_chunk_data	(ArvBuffer *buffer, guint64 chunk_id, size_t *size);

G_END_DECLS

#endif
