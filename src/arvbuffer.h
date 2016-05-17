/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
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

#include <arvtypes.h>

G_BEGIN_DECLS

typedef void (*ArvFrameCallback)	(ArvBuffer *buffer);

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
	ARV_BUFFER_PAYLOAD_TYPE_MULTIZONE_IMAGE = 	0x0009
} ArvBufferPayloadType;

#define ARV_TYPE_BUFFER             (arv_buffer_get_type ())
#define ARV_BUFFER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_BUFFER, ArvBuffer))
#define ARV_BUFFER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_BUFFER, ArvBufferClass))
#define ARV_IS_BUFFER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_BUFFER))
#define ARV_IS_BUFFER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_BUFFER))
#define ARV_BUFFER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_BUFFER, ArvBufferClass))

typedef struct _ArvBufferPrivate ArvBufferPrivate;
typedef struct _ArvBufferClass ArvBufferClass;

struct _ArvBuffer {
	GObject	object;

	ArvBufferPrivate *priv;
};

struct _ArvBufferClass {
	GObjectClass parent_class;
};

GType arv_buffer_get_type (void);

ArvBuffer *		arv_buffer_new_allocate		(size_t size);
ArvBuffer *		arv_buffer_new 			(size_t size, void *preallocated);
ArvBuffer * 		arv_buffer_new_full		(size_t size, void *preallocated,
						 	void *user_data, GDestroyNotify user_data_destroy_func);

ArvBufferStatus		arv_buffer_get_status		(ArvBuffer *buffer);

const void *		arv_buffer_get_user_data	(ArvBuffer *buffer);

ArvBufferPayloadType	arv_buffer_get_payload_type	(ArvBuffer *buffer);
guint64			arv_buffer_get_timestamp	(ArvBuffer *buffer);
void			arv_buffer_set_timestamp	(ArvBuffer *buffer, guint64 timestamp_ns);
guint32 		arv_buffer_get_frame_id 	(ArvBuffer *buffer);
const void *		arv_buffer_get_data		(ArvBuffer *buffer, size_t *size);

void			arv_buffer_get_image_region		(ArvBuffer *buffer, gint *x, gint *y, gint *width, gint *height);
gint			arv_buffer_get_image_width		(ArvBuffer *buffer);
gint			arv_buffer_get_image_height		(ArvBuffer *buffer);
gint			arv_buffer_get_image_x			(ArvBuffer *buffer);
gint			arv_buffer_get_image_y			(ArvBuffer *buffer);
ArvPixelFormat		arv_buffer_get_image_pixel_format	(ArvBuffer *buffer);

const void *		arv_buffer_get_chunk_data	(ArvBuffer *buffer, guint64 chunk_id, size_t *size);

G_END_DECLS

#endif
