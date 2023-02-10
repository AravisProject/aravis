/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

/**
 * ArvBuffer:
 *
 * [class@ArvBuffer] provides a class for the instantiation of buffers used for the storage of the separate images of
 * the video stream.
 *
 * The actual data space may either be allocated by [class@ArvBuffer] during an object instantiation, of pre-allocated.
 * [class@ArvBuffer] also allows the transmission of image metadata, such as offsets and size of the transmitted region of
 * interest, pixel format and time stamp.
 */

#include <arvbufferprivate.h>

static gboolean
arv_buffer_part_is_image (ArvBuffer *buffer, guint part_id)
{
	return (ARV_IS_BUFFER (buffer) &&
                buffer->priv->status == ARV_BUFFER_STATUS_SUCCESS &&
                buffer->priv->n_parts > 0 &&
                part_id < buffer->priv->n_parts &&
                (buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE ||
                 buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA ||
                 buffer->priv->payload_type == ARV_BUFFER_PAYLOAD_TYPE_MULTIPART) &&
                (buffer->priv->parts[part_id].data_type == ARV_BUFFER_PART_DATA_TYPE_2D_IMAGE ||
                 buffer->priv->parts[part_id].data_type == ARV_BUFFER_PART_DATA_TYPE_2D_PLANE_BIPLANAR ||
                 buffer->priv->parts[part_id].data_type == ARV_BUFFER_PART_DATA_TYPE_2D_PLANE_TRIPLANAR ||
                 buffer->priv->parts[part_id].data_type == ARV_BUFFER_PART_DATA_TYPE_2D_PLANE_QUADPLANAR ||
                 buffer->priv->parts[part_id].data_type == ARV_BUFFER_PART_DATA_TYPE_3D_IMAGE ||
                 buffer->priv->parts[part_id].data_type == ARV_BUFFER_PART_DATA_TYPE_3D_PLANE_BIPLANAR ||
                 buffer->priv->parts[part_id].data_type == ARV_BUFFER_PART_DATA_TYPE_3D_PLANE_TRIPLANAR ||
                 buffer->priv->parts[part_id].data_type == ARV_BUFFER_PART_DATA_TYPE_3D_PLANE_QUADPLANAR ||
                 buffer->priv->parts[part_id].data_type == ARV_BUFFER_PART_DATA_TYPE_CONFIDENCE_MAP));
}

/**
 * arv_buffer_new_full:
 * @size: payload size
 * @preallocated: (transfer none): preallocated memory buffer
 * @user_data: (transfer none): a pointer to user data associated to this buffer
 * @user_data_destroy_func: (nullable): an optional user data destroy callback
 *
 * Creates a new buffer for the storage of the video stream images.
 * The data space can be either preallocated, and the caller is responsible
 * for it's deallocation, or allocated by this function. If it is the case,
 * data memory will be freed when the buffer is destroyed.
 *
 * If @user_data_destroy_func is non NULL, it will be called in order to destroy
 * user_data when the buffer is destroyed.
 *
 * Returns: a new [class@ArvBuffer] object
 *
 * Since: 0.2.0
 */

ArvBuffer *
arv_buffer_new_full (size_t size, void *preallocated, void *user_data, GDestroyNotify user_data_destroy_func)
{
	ArvBuffer *buffer;

	buffer = g_object_new (ARV_TYPE_BUFFER, NULL);
	buffer->priv->allocated_size = size;
	buffer->priv->user_data = user_data;
	buffer->priv->user_data_destroy_func = user_data_destroy_func;
	buffer->priv->chunk_endianness = G_BIG_ENDIAN;
	buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_UNKNOWN;
        buffer->priv->parts = g_new0 (ArvBufferPartInfos, 1);
        buffer->priv->n_parts = 1;

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
 * arv_buffer_get_data:
 * @buffer: a #ArvBuffer
 * @size: (out) (optional): location to store data size, or %NULL
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
		*size = buffer->priv->received_size;

	return buffer->priv->data;
}

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint32 id;
	guint32 size;
} ArvChunkInfos;

/**
 * arv_buffer_has_chunks:
 * @buffer: a #ArvBuffer
 *
 * Returns: %TRUE if @buffer has a payload type that contains chunk data.
 *
 * Since: 0.8.0
 */

gboolean
arv_buffer_has_chunks (ArvBuffer *buffer)
{
	return ARV_IS_BUFFER (buffer) &&
		buffer->priv->status == ARV_BUFFER_STATUS_SUCCESS &&
                buffer->priv->has_chunks;
}

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
	unsigned char *data;
	ptrdiff_t offset;

	if (size != NULL)
		*size = 0;

	g_return_val_if_fail (arv_buffer_has_chunks (buffer), NULL);
	g_return_val_if_fail (buffer->priv->data != NULL, NULL);

	data = buffer->priv->data;
	offset = buffer->priv->received_size - sizeof (ArvChunkInfos);
	while (offset > 0) {
		guint32 id;
		guint32 chunk_size;

		infos = (ArvChunkInfos *) &data[offset];

		if (buffer->priv->chunk_endianness == G_BIG_ENDIAN) {
			id = GUINT32_FROM_BE (infos->id);
			chunk_size = GUINT32_FROM_BE (infos->size);
		} else {
			id = GUINT32_FROM_LE (infos->id);
			chunk_size = GUINT32_FROM_LE (infos->size);
		}

		if (id == chunk_id) {
			ptrdiff_t data_offset;

			data_offset = offset - chunk_size;
			if (data_offset >= 0) {
				if (size != NULL)
					*size = chunk_size;
				return &data[data_offset];
			} else
		       		return NULL;
		}
		if (chunk_size > 0)
			offset = offset - chunk_size - sizeof (ArvChunkInfos);
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

	return buffer->priv->payload_type;
}

/**
 * arv_buffer_get_timestamp:
 * @buffer: a #ArvBuffer
 *
 * Gets the buffer camera timestamp, expressed as nanoseconds. Not all devices
 * provide reliable timestamp, which means sometimes its better to rely on the
 * buffer completion host local time, or to use
 * arv_buffer_get_system_timestamp().
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
 * arv_buffer_get_system_timestamp:
 * @buffer: a #ArvBuffer
 *
 * Gets the system timestamp for when the frame was received. Expressed in
 * nanoseconds.
 *
 * Returns: buffer system timestamp, in nanoseconds.
 *
 * Since: 0.6.0
 */

guint64
arv_buffer_get_system_timestamp (ArvBuffer *buffer)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0);

	return buffer->priv->system_timestamp_ns;
}

/**
 * arv_buffer_set_system_timestamp:
 * @buffer: a #ArvBuffer
 * @timestamp_ns: a timestamp, expressed as nanoseconds
 *
 * Sets the system timestamp for when the frame was received. Expressed in
 * nanoseconds.
 *
 * Since: 0.6.0
 */

void
arv_buffer_set_system_timestamp (ArvBuffer *buffer, guint64 timestamp_ns)
{
	g_return_if_fail (ARV_IS_BUFFER (buffer));

	buffer->priv->system_timestamp_ns = timestamp_ns;
}


/**
 * arv_buffer_get_frame_id:
 * @buffer: a #ArvBuffer
 *
 * Gets the buffer frame id. For GigEVision devices, 0 is an invalid value.
 *
 * Returns: frame id, 0 on error.
 *
 * Since: 0.8.0
 */

guint64
arv_buffer_get_frame_id (ArvBuffer *buffer)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0);

	return buffer->priv->frame_id;
}

/**
 * arv_buffer_set_frame_id:
 * @buffer: a #ArvBuffer
 * @frame_id: a #guint64
 *
 * Sets the buffer frame id.  For GigEVision devices, 0 is an invalid value.
 *
 * Since: 0.8.3
 */

void
arv_buffer_set_frame_id (ArvBuffer *buffer, guint64 frame_id)
{
	g_return_if_fail (ARV_IS_BUFFER (buffer));

	buffer->priv->frame_id = frame_id;
}

/**
 * arv_buffer_get_n_parts:
 * @buffer: a #ArvBuffer
 *
 * Returns: the number of part in the buffer.
 *
 * Since: 0.8.23
 */

guint
arv_buffer_get_n_parts (ArvBuffer *buffer)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0);

        return buffer->priv->n_parts;
}

/**
 * arv_buffer_find_component:
 * @buffer: a #ArvBuffer
 * @component_id: the component id to find
 *
 * Search for the part corresponding to @component_id
 *
 * Return: the corresponding part id, -1 if not found.
 *
 * Since: 0.8.25
 */

gint
arv_buffer_find_component (ArvBuffer *buffer, guint component_id)
{
        guint i;

	g_return_val_if_fail (ARV_IS_BUFFER (buffer), -1);

        for (i = 0; i < buffer->priv->n_parts; i++)
                if (buffer->priv->parts[i].component_id == component_id)
                        return i;

        return -1;
}

/**
 * arv_buffer_get_part_data:
 * @buffer: a #ArvBuffer
 * @part_id: part id
 * @size: (out) (optional): data size placeholder
 *
 * Returns: (array length=size) (element-type guint8): a pointer to the part data.
 *
 * Since: 0.8.23
 */

const void *
arv_buffer_get_part_data (ArvBuffer *buffer, guint part_id, size_t *size)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), NULL);
        g_return_val_if_fail (part_id < buffer->priv->n_parts, NULL);

        if (size != NULL)
                *size = buffer->priv->parts[part_id].size;

        return buffer->priv->data + buffer->priv->parts[part_id].data_offset;
}

/**
 * arv_buffer_get_part_component_id:
 * @buffer: a #ArvBuffer
 * @part_id: part id
 *
 * Returns: the part component id value
 *
 * Since: 0.8.25
 */

guint
arv_buffer_get_part_component_id (ArvBuffer *buffer, guint part_id)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), 0);
        g_return_val_if_fail (part_id < buffer->priv->n_parts, 0);

        return buffer->priv->parts[part_id].component_id;
}

/**
 * arv_buffer_get_part_data_type:
 * @buffer: a #ArvBuffer
 * @part_id: part id
 *
 * Returns: the part #ArvBufferPartDataType
 *
 * Since: 0.8.23
 */

ArvBufferPartDataType
arv_buffer_get_part_data_type (ArvBuffer *buffer, guint part_id)
{
	g_return_val_if_fail (ARV_IS_BUFFER (buffer), ARV_BUFFER_PART_DATA_TYPE_UNKNOWN);
        g_return_val_if_fail (part_id < buffer->priv->n_parts, ARV_BUFFER_PART_DATA_TYPE_UNKNOWN);

        return buffer->priv->parts[part_id].data_type;
}

/**
 * arv_buffer_get_part_pixel_format:
 * @buffer: a #ArvBuffer
 * @part_id: a part id
 *
 * Gets the part pixel format.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Returns: part pixel format.
 *
 * Since: 0.8.23
 */

ArvPixelFormat
arv_buffer_get_part_pixel_format (ArvBuffer *buffer, guint part_id)
{
	g_return_val_if_fail (arv_buffer_part_is_image (buffer, part_id), 0);

	return buffer->priv->parts[part_id].pixel_format;
}

/**
 * arv_buffer_get_part_region:
 * @buffer: a #ArvBuffer
 * @part_id: a part id
 * @x: (out) (optional): x offset placeholder
 * @y: (out) (optional): y offset placeholder
 * @width: (out) (optional): width placeholder
 * @height: (out) (optional): height placeholder
 *
 * Gets the part region.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Since: 0.8.23
 */

void
arv_buffer_get_part_region (ArvBuffer *buffer, guint part_id, gint *x, gint *y, gint *width, gint *height)
{
	g_return_if_fail (arv_buffer_part_is_image (buffer, part_id));

	if (x != NULL)
		*x = buffer->priv->parts[part_id].x_offset;
	if (y != NULL)
		*y = buffer->priv->parts[part_id].y_offset;
	if (width != NULL)
		*width = buffer->priv->parts[part_id].width;
	if (height != NULL)
		*height = buffer->priv->parts[part_id].height;
}

/**
 * arv_buffer_get_part_padding:
 * @buffer: a #ArvBuffer
 * @part_id: a part id
 * @x_padding: (out) (optional): x offset placeholder
 * @y_padding: (out) (optional): y offset placeholder
 *
 * Gets the part padding.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Since: 0.8.23
 */

void
arv_buffer_get_part_padding (ArvBuffer *buffer, guint part_id, gint *x_padding, gint *y_padding)
{
	g_return_if_fail (arv_buffer_part_is_image (buffer, part_id));

	if (x_padding != NULL)
		*x_padding = buffer->priv->parts[part_id].x_padding;
	if (y_padding != NULL)
		*y_padding = buffer->priv->parts[part_id].y_padding;
}

/**
 * arv_buffer_get_part_width:
 * @buffer: a #ArvBuffer
 * @part_id: a part id
 *
 * Gets the part width.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Returns: width, in pixels.
 *
 * Since: 0.8.23
 */

gint
arv_buffer_get_part_width (ArvBuffer *buffer, guint part_id)
{
	g_return_val_if_fail (arv_buffer_part_is_image (buffer, part_id), 0);

	return buffer->priv->parts[part_id].width;
}

/**
 * arv_buffer_get_part_height:
 * @buffer: a #ArvBuffer
 * @part_id: a part id
 *
 * Gets the part height.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Returns: height, in pixels.
 *
 * Since: 0.8.23
 */

gint
arv_buffer_get_part_height (ArvBuffer *buffer, guint part_id)
{
	g_return_val_if_fail (arv_buffer_part_is_image (buffer, part_id), 0);

	return buffer->priv->parts[part_id].height;
}

/**
 * arv_buffer_get_part_x:
 * @buffer: a #ArvBuffer
 * @part_id: a part id
 *
 * Gets the part x offset.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Returns: x offset, in pixels.
 *
 * Since: 0.8.23
 */

gint
arv_buffer_get_part_x (ArvBuffer *buffer, guint part_id)
{
	g_return_val_if_fail (arv_buffer_part_is_image (buffer, part_id), 0);

	return buffer->priv->parts[part_id].x_offset;
}

/**
 * arv_buffer_get_part_y:
 * @buffer: a #ArvBuffer
 * @part_id: a part id
 *
 * Gets the part y_offset.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Returns: y offset, in pixels.
 *
 * Since: 0.8.23
 */

gint
arv_buffer_get_part_y (ArvBuffer *buffer, guint part_id)
{
	g_return_val_if_fail (arv_buffer_part_is_image (buffer, part_id), 0);

	return buffer->priv->parts[part_id].y_offset;
}

/**
 * arv_buffer_get_image_data:
 * @buffer: a #ArvBuffer
 * @size: (out) (optional): data size placeholder
 *
 * Returns: (array length=size) (element-type guint8): a pointer to the image data.
 *
 * Since: 0.8.25
 */

const void *
arv_buffer_get_image_data (ArvBuffer *buffer, size_t *size)
{
        return arv_buffer_get_part_data (buffer, 0, size);
}

/**
 * arv_buffer_get_image_pixel_format:
 * @buffer: a #ArvBuffer
 *
 * Gets the image pixel format.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Returns: image pixel format.
 *
 * Since: 0.4.0
 */

ArvPixelFormat
arv_buffer_get_image_pixel_format (ArvBuffer *buffer)
{
        return arv_buffer_get_part_pixel_format (buffer, 0);
}

/**
 * arv_buffer_get_image_region:
 * @buffer: a #ArvBuffer
 * @x: (out) (optional): image x offset placeholder
 * @y: (out) (optional): image y offset placeholder
 * @width: (out) (optional): image width placholder
 * @height: (out) (optional): image height placeholder
 *
 * Gets the image region.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Since: 0.4.0
 */

void
arv_buffer_get_image_region (ArvBuffer *buffer, gint *x, gint *y, gint *width, gint *height)
{
        arv_buffer_get_part_region (buffer, 0, x, y, width, height);
}

/**
 * arv_buffer_get_image_padding:
 * @buffer: a #ArvBuffer
 * @x_padding: (out) (optional): image x offset placeholder
 * @y_padding: (out) (optional): image y offset placeholder
 *
 * Gets the image padding.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Since: 0.8.23
 */

void
arv_buffer_get_image_padding (ArvBuffer *buffer, gint *x_padding, gint *y_padding)
{
        return arv_buffer_get_part_padding (buffer, 0, x_padding, y_padding);
}

/**
 * arv_buffer_get_image_width:
 * @buffer: a #ArvBuffer
 *
 * Gets the image width.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Returns: image width, in pixels.
 *
 * Since: 0.4.0
 */

gint
arv_buffer_get_image_width (ArvBuffer *buffer)
{
        return arv_buffer_get_part_width (buffer, 0);
}

/**
 * arv_buffer_get_image_height:
 * @buffer: a #ArvBuffer
 *
 * Gets the image width.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Returns: image height, in pixels.
 *
 * Since: 0.4.0
 */

gint
arv_buffer_get_image_height (ArvBuffer *buffer)
{
        return arv_buffer_get_part_height (buffer, 0);
}

/**
 * arv_buffer_get_image_x:
 * @buffer: a #ArvBuffer
 *
 * Gets the image x offset.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Returns: image x offset, in pixels.
 *
 * Since: 0.4.0
 */

gint
arv_buffer_get_image_x (ArvBuffer *buffer)
{
        return arv_buffer_get_part_x (buffer, 0);
}

/**
 * arv_buffer_get_image_y:
 * @buffer: a #ArvBuffer
 *
 * Gets the image y offset.
 *
 * This function must only be called if buffer payload is either @ARV_BUFFER_PAYLOAD_TYPE_IMAGE,
 * @ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA or @ARV_BUFFER_PAYLOAD_TYPE_MULTIPART.
 *
 * Returns: image y offset, in pixels.
 *
 * Since: 0.4.0
 */

gint
arv_buffer_get_image_y (ArvBuffer *buffer)
{
        return arv_buffer_get_part_y (buffer, 0);
}

void
arv_buffer_set_n_parts (ArvBuffer* buffer, guint n_parts)
{
        g_return_if_fail (ARV_IS_BUFFER(buffer));

        if (G_UNLIKELY (n_parts == 0)) {
                buffer->priv->n_parts = 0;
                g_clear_pointer (&buffer->priv->parts, g_free);

                return;
        }

        if (buffer->priv->n_parts != n_parts)
                buffer->priv->parts = g_realloc_n (buffer->priv->parts, n_parts, sizeof (ArvBufferPartInfos));

        memset (buffer->priv->parts, 0, n_parts * sizeof (ArvBufferPartInfos));
        buffer->priv->n_parts = n_parts;
}

G_DEFINE_TYPE_WITH_CODE (ArvBuffer, arv_buffer, G_TYPE_OBJECT, G_ADD_PRIVATE (ArvBuffer))

static void
arv_buffer_init (ArvBuffer *buffer)
{
	buffer->priv = arv_buffer_get_instance_private (buffer);
	buffer->priv->status = ARV_BUFFER_STATUS_CLEARED;
}

static void
arv_buffer_finalize (GObject *object)
{
	ArvBuffer *buffer = ARV_BUFFER (object);

        buffer->priv->n_parts = 0;
        g_clear_pointer (&buffer->priv->parts, g_free);

	if (!buffer->priv->is_preallocated) {
		g_free (buffer->priv->data);
		buffer->priv->data = NULL;
		buffer->priv->allocated_size = 0;
	}

	if (buffer->priv->user_data && buffer->priv->user_data_destroy_func)
		buffer->priv->user_data_destroy_func (buffer->priv->user_data);

	G_OBJECT_CLASS (arv_buffer_parent_class)->finalize (object);
}

static void
arv_buffer_class_init (ArvBufferClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);

	object_class->finalize = arv_buffer_finalize;
}
