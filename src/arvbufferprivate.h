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

#ifndef ARV_BUFFER_PRIVATE_H
#define ARV_BUFFER_PRIVATE_H

#include <arvbuffer.h>
#include <arvgvsp.h>

G_BEGIN_DECLS

struct _ArvBufferPrivate {
	size_t size;
	gboolean is_preallocated;
	void *data;

	void *user_data;
	GDestroyNotify user_data_destroy_func;

	ArvBufferStatus status;

	ArvGvspPayloadType gvsp_payload_type;

	guint32 frame_id;
	guint64 timestamp_ns;

	guint32 x_offset;
	guint32 y_offset;
	guint32 width;
	guint32 height;

	ArvPixelFormat pixel_format;
};

G_END_DECLS

#endif
