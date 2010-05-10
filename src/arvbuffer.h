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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_BUFFER_H
#define ARV_BUFFER_H

#include <arvtypes.h>

G_BEGIN_DECLS

typedef void (*ArvFrameCallback)	(ArvBuffer *buffer);

typedef enum {
	ARV_BUFFER_STATUS_SUCCESS,
	ARV_BUFFER_STATUS_CLEARED,
	ARV_BUFFER_STATUS_FILLING,
	ARV_BUFFER_STATUS_MISSING_BLOCK,
	ARV_BUFFER_STATUS_SIZE_MISMATCH,
	ARV_BUFFER_STATUS_ABORTED
} ArvBufferStatus;

#define ARV_TYPE_BUFFER             (arv_buffer_get_type ())
#define ARV_BUFFER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_BUFFER, ArvBuffer))
#define ARV_BUFFER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_BUFFER, ArvBufferClass))
#define ARV_IS_BUFFER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_BUFFER))
#define ARV_IS_BUFFER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_BUFFER))
#define ARV_BUFFER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_BUFFER, ArvBufferClass))

typedef struct _ArvBufferClass ArvBufferClass;

struct _ArvBuffer {
	GObject	object;

	size_t size;
	void *data;
	gboolean is_preallocated;

	ArvBufferStatus status;

	guint32 frame_id;
	guint32 x;
	guint32 y;
	guint32 width;
	guint32 height;
	ArvPixelFormat pixel_format;
};

struct _ArvBufferClass {
	GObjectClass parent_class;
};

GType arv_buffer_get_type (void);

ArvBuffer * 		arv_buffer_new 			(size_t size, void *preallocated);

G_END_DECLS

#endif
