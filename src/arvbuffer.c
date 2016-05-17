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

/**
 * SECTION: arvbuffer
 * @short_description: Buffer for storage of video frames
 *
 * #ArvBuffer provides a class for the instantiation of buffers used for the
 * storage of the separate images of the video stream. The actual data space
 * may either be allocated by #ArvBuffer during an object instatiation, of
 * preallocated. #ArvBuffer also allows the transmission of image metadata,
 * such as offsets and size of the transmitted region of interrest, pixel
 * format and time stamp.
 */

#include <arvbufferprivate.h>

static GObjectClass *parent_class = NULL;

/**
 * arv_buffer_new_full:
 * @size: payload size
 * @preallocated: (transfer none): preallocated memory buffer
 * @user_data: (transfer none): a pointer to user data associated to this buffer
 * @user_data_destroy_func: an optional user data destroy callback
 *
 * Creates a new buffer for the storage of the video stream images. 
 * The data space can be either preallocated, and the caller is responsible
 * for it's deallocation, or allocated by this function. If it is the case,
 * data memory will be freed when the buffer is destroyed.
 *
 * If @user_data_destroy_func is non NULL, it will be called in order to destroy
 * user_data when the buffer is destroyed.
 *
 * Returns: a new #ArvBuffer object
 *
 * Since: 0.2.0
 */

ArvBuffer *
arv_buffer_new_full (size_t size, void *preallocated, void *user_data, GDestroyNotify user_data_destroy_func)
{
	ArvBuffer *buffer;

	buffer = g_object_new (ARV_TYPE_BUFFER, NULL);
	buffer->priv->size = size;
	buffer->priv->user_data = user_data;
	buffer->priv->user_data_destroy_func = user_data_destroy_func;

	if (preallocated != NULL) {
		buffer->priv->is_preallocated = TRUE;
		buffer->priv->data = preallocated;
	} else {
		buffer->priv->is_preallocated = FALSE;
		buffer->priv->data = g_malloc (size);
	}

	return buffer;
}

/**
 * arv_buffer_new:
 * @size: payload size
 * @preallocated: (transfer none): preallocated memory buffer
 *
 * Creates a new buffer for the storage of the video stream images. 
 * The data space can be either preallocated, and the caller is responsible
 * for it's deallocation, or allocated by this function. If it is the case,
 * data memory will be freed when the buffer is destroyed.
 *
 * Returns: a new #ArvBuffer object
 *
 * Since: 0.2.0
 */

ArvBuffer *
arv_buffer_new (size_t size, void *preallocated)
{
	return arv_buffer_new_full (size, preallocated, NULL, NULL);
}

/**
 * arv_buffer_new_allocate:
 * @size: payload size
 *
 * Creates a new buffer for the storage of the video stream images. 
 * The data space is allocated by this function, and will
 * be freed when the buffer is destroyed.
 *
 * Returns: a new #ArvBuffer object
 *
 * Since: 0.2.3
 */

ArvBuffer *
arv_buffer_new_allocate (size_t size)
{
	return arv_buffer_new_full (size, NULL, NULL, NULL);
}

/**
 * arv_buffer_clear:
 * @buffer: a #ArvBuffer
 *
 * Clears the buffer status.
 *
 * Since: 0.2.0
 */

void
arv_buffer_clear (ArvBuffer *buffer)
{
	g_return_if_fail (ARV_IS_BUFFER (buffer));

	buffer->priv->status = ARV_BUFFER_STATUS_CLEARED;
}

/**
 * arv_buffer_get_data:
 * @buffer: a #ArvBuffer
 * @size: (allow-none): location to store data size, or %NULL
 *
 * Buffer data accessor.
 *
 * Returns: (array length=size) (element-type guint8): a pointer to the buffer data.
 *
 * Since: 0.4.0
 **/

const void *
arv_buffer_get_data (ArvBuffer *buffer, size_t *size)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), NULL);

	if (size != NULL)
		*size = buffer->priv->size;

	return buffer->priv->data;
}

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint32 id;
	guint32 size;
} ArvChunkInfos;

/**
 * arv_buffer_get_chunk_data:
 * @buffer: a #ArvBuffer
 * @chunk_id: chunk id
 * @size: (allow-none): location to store chunk data size, or %NULL
 *
 * Chunk data accessor.
 *
 * Returns: (array length=size) (element-type guint8): a pointer to the chunk data.
 *
 * Since: 0.4.0
 **/

const void *
arv_buffer_get_chunk_data (ArvBuffer *buffer, guint64 chunk_id, size_t *size)
{
	ArvChunkInfos *infos;
	char *data;
	ptrdiff_t offset;

	if (size != NULL)
		*size = 0;

	g_return_val_if_fail (ARV_IS_BUFFER (buffer), NULL);
	g_return_val_if_fail (buffer->priv->data != NULL, NULL);
	g_return_val_if_fail (buffer->priv->gvsp_payload_type == ARV_GVSP_PAYLOAD_TYPE_CHUNK_DATA, NULL);

	if (buffer->priv->status != ARV_BUFFER_STATUS_SUCCESS)
		return NULL;

	data = buffer->priv->data;
	offset = buffer->priv->size - sizeof (ArvChunkInfos);
	while (offset > 0) {
		infos = (ArvChunkInfos *) &data[offset];
		if (GUINT32_FROM_BE (infos->id) == chunk_id) {
			ptrdiff_t data_offset;

			data_offset = offset - GUINT32_FROM_BE (infos->size);
			if (data_offset >= 0) {
				if (size != NULL)
					*size = GUINT32_FROM_BE (infos->size);
				return &data[data_offset];
			} else
		       		return NULL;
		}
		if (GUINT32_FROM_BE (infos->size) > 0)
			offset = offset - GUINT32_FROM_BE (infos->size) - sizeof (ArvChunkInfos);
		else
			offset = 0;
	};

	return NULL;
}

/**
 * arv_buffer_get_user_data:
 * @buffer: a #ArvBuffer
 *
 * Gets a pointer to user data set by arv_buffer_new_full.
 *
 * Returns: user data, or NULL if not set.
 *
 * Since: 0.4.0
 */

const void *
arv_buffer_get_user_data (ArvBuffer *buffer)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), NULL);
	
	return buffer->priv->user_data;
}

/**
 * arv_buffer_get_status:
 * @buffer: a #ArvBuffer
 *
 * Gets the buffer acquisition status.
 *
 * Returns: buffer acquisition status.
 *
 * Since: 0.4.0
 */

ArvBufferStatus
arv_buffer_get_status (ArvBuffer *buffer)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), ARV_BUFFER_STATUS_UNKNOWN);
	
	return buffer->priv->status;
}

/**
 * arv_buffer_get_payload_type:
 * @buffer: a #ArvBuffer
 *
 * Gets the buffer payload type.
 *
 * Returns: payload type.
 *
 * Since: 0.4.0
 */

ArvBufferPayloadType
arv_buffer_get_payload_type (ArvBuffer *buffer)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), -1);
	
	switch (buffer->priv->gvsp_payload_type) {
		case ARV_GVSP_PAYLOAD_TYPE_IMAGE:
			return ARV_BUFFER_PAYLOAD_TYPE_IMAGE;
		case ARV_GVSP_PAYLOAD_TYPE_RAWDATA:
			return ARV_BUFFER_PAYLOAD_TYPE_RAWDATA;
		case ARV_GVSP_PAYLOAD_TYPE_FILE:
			return ARV_BUFFER_PAYLOAD_TYPE_FILE;
		case ARV_GVSP_PAYLOAD_TYPE_CHUNK_DATA:
			return ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA;
		case ARV_GVSP_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA:
			return ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA;
		case ARV_GVSP_PAYLOAD_TYPE_JPEG:
			return ARV_BUFFER_PAYLOAD_TYPE_JPEG;
		case ARV_GVSP_PAYLOAD_TYPE_JPEG2000:
			return ARV_BUFFER_PAYLOAD_TYPE_JPEG2000;
		case ARV_GVSP_PAYLOAD_TYPE_H264:
			return ARV_BUFFER_PAYLOAD_TYPE_H264;
		case ARV_GVSP_PAYLOAD_TYPE_MULTIZONE_IMAGE:
			return ARV_BUFFER_PAYLOAD_TYPE_MULTIZONE_IMAGE;
		default:
			return ARV_BUFFER_PAYLOAD_TYPE_UNKNOWN;
	}
}

/**
 * arv_buffer_get_timestamp:
 * @buffer: a #ArvBuffer
 *
 * Gets the buffer camera timestamp, expressed as nanoseconds. Not all devices
 * provide reliable timestamp, which means sometimes its better to rely on the
 * buffer completion host local time (given by @g_get_realtime for example).
 *
 * Returns: buffer timestamp, in nanoseconds.
 *
 * Since: 0.4.0
 */

guint64
arv_buffer_get_timestamp (ArvBuffer *buffer)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0);

	return buffer->priv->timestamp_ns;
}

/**
 * arv_buffer_set_timestamp:
 * @buffer: a #ArvBuffer
 * @timestamp_ns: a timestamp, expressed as nanoseconds
 *
 * Sets the buffer timestamp, which allows to override the timpestamp set by
 * the camera, which in some case is incorrect.
 *
 * Since: 0.4.0
 */

void
arv_buffer_set_timestamp (ArvBuffer *buffer, guint64 timestamp_ns)
{
	g_return_if_fail (ARV_IS_BUFFER (buffer));

	buffer->priv->timestamp_ns = timestamp_ns;
}

/**
 * arv_buffer_get_frame_id:
 * @buffer: a #ArvBuffer
 *
 * Gets the buffer frame id. For GigEVision devices, valid values are in the
 * 1..65535 range.
 *
 * Returns: frame id, 0 on error.
 *
 * Since: 0.4.0
 */

guint32
arv_buffer_get_frame_id (ArvBuffer *buffer)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0);

	return buffer->priv->frame_id;
}

/**
 * arv_buffer_get_image_region:
 * @buffer: a #ArvBuffer
 * @x: (allow-none): image x offset placeholder
 * @y: (allow-none): image y offset placeholder
 * @width: (allow-none): image width placholder
 * @height: (allow-none): image height placeholder
 *
 * Gets the image region. This function must only be called on buffer containing a @ARV_BUFFER_PAYLOAD_TYPE_IMAGE payload.
 *
 * Since: 0.4.0
 */

void
arv_buffer_get_image_region (ArvBuffer *buffer, gint *x, gint *y, gint *width, gint *height)
{
	g_return_if_fail (ARV_IS_BUFFER (buffer));
	g_return_if_fail (buffer->priv->gvsp_payload_type == ARV_GVSP_PAYLOAD_TYPE_IMAGE);

	if (x != NULL)
		*x = buffer->priv->x_offset;
	if (y != NULL)
		*y = buffer->priv->y_offset;
	if (width != NULL)
		*width = buffer->priv->width;
	if (height != NULL)
		*height = buffer->priv->height;
}

/**
 * arv_buffer_get_image_width:
 * @buffer: a #ArvBuffer
 *
 * Gets the image width. This function must only be called on buffer containing a @ARV_BUFFER_PAYLOAD_TYPE_IMAGE payload.
 *
 * Returns: image width, in pixels.
 *
 * Since: 0.4.0
 */

gint
arv_buffer_get_image_width (ArvBuffer *buffer)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0);
	g_return_val_if_fail (buffer->priv->gvsp_payload_type == ARV_GVSP_PAYLOAD_TYPE_IMAGE, 0);

	return buffer->priv->width;
}

/**
 * arv_buffer_get_image_height:
 * @buffer: a #ArvBuffer
 *
 * Gets the image width. This function must only be called on buffer containing a @ARV_BUFFER_PAYLOAD_TYPE_IMAGE payload.
 *
 * Returns: image height, in pixels.
 *
 * Since: 0.4.0
 */

gint
arv_buffer_get_image_height (ArvBuffer *buffer)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0);
	g_return_val_if_fail (buffer->priv->gvsp_payload_type == ARV_GVSP_PAYLOAD_TYPE_IMAGE, 0);

	return buffer->priv->height;
}

/**
 * arv_buffer_get_image_x:
 * @buffer: a #ArvBuffer
 *
 * Gets the image x offset. This function must only be called on buffer containing a @ARV_BUFFER_PAYLOAD_TYPE_IMAGE payload.
 *
 * Returns: image x offset, in pixels.
 *
 * Since: 0.4.0
 */

gint
arv_buffer_get_image_x (ArvBuffer *buffer)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0);
	g_return_val_if_fail (buffer->priv->gvsp_payload_type == ARV_GVSP_PAYLOAD_TYPE_IMAGE, 0);

	return buffer->priv->x_offset;
}

/**
 * arv_buffer_get_image_y:
 * @buffer: a #ArvBuffer
 *
 * Gets the image y offset. This function must only be called on buffer containing a @ARV_BUFFER_PAYLOAD_TYPE_IMAGE payload.
 *
 * Returns: image y offset, in pixels.
 *
 * Since: 0.4.0
 */

gint
arv_buffer_get_image_y (ArvBuffer *buffer)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0);
	g_return_val_if_fail (buffer->priv->gvsp_payload_type == ARV_GVSP_PAYLOAD_TYPE_IMAGE, 0);

	return buffer->priv->y_offset;
}

/**
 * arv_buffer_get_image_pixel_format:
 * @buffer: a #ArvBuffer
 *
 * Gets the image pixel format. This function must only be called on buffer containing a @ARV_BUFFER_PAYLOAD_TYPE_IMAGE payload.
 *
 * Returns: image pixel format.
 *
 * Since: 0.4.0
 */

ArvPixelFormat
arv_buffer_get_image_pixel_format (ArvBuffer *buffer)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0);
	g_return_val_if_fail (buffer->priv->gvsp_payload_type == ARV_GVSP_PAYLOAD_TYPE_IMAGE, 0);

	return buffer->priv->pixel_format;
}

static void
arv_buffer_init (ArvBuffer *buffer)
{
	buffer->priv = G_TYPE_INSTANCE_GET_PRIVATE (buffer, ARV_TYPE_BUFFER, ArvBufferPrivate);
	buffer->priv->status = ARV_BUFFER_STATUS_CLEARED;
}

static void
arv_buffer_finalize (GObject *object)
{
	ArvBuffer *buffer = ARV_BUFFER (object);

	if (!buffer->priv->is_preallocated) {
		g_free (buffer->priv->data);
		buffer->priv->data = NULL;
		buffer->priv->size = 0;
	}

	if (buffer->priv->user_data && buffer->priv->user_data_destroy_func)
		buffer->priv->user_data_destroy_func (buffer->priv->user_data);

	parent_class->finalize (object);
}

static void
arv_buffer_class_init (ArvBufferClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);

	g_type_class_add_private (this_class, sizeof (ArvBufferPrivate));

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_buffer_finalize;
}

G_DEFINE_TYPE (ArvBuffer, arv_buffer, G_TYPE_OBJECT)
