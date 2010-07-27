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

#ifndef ARV_STREAM_H
#define ARV_STREAM_H

#include <arvtypes.h>

G_BEGIN_DECLS

/**
 * ArvStreamCallbackType:
 * @ARV_STREAM_CALLBACK_TYPE_INIT: thread initialization, happens once
 * @ARV_STREAM_CALLBACK_TYPE_EXIT: thread end, happens once
 * @ARV_STREAM_CALLBACK_TYPE_START_BUFFER: buffer filling start, happens at each frame
 * @ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE: buffer filled, happens at each frame
 *
 * Describes when the stream callback is called.
 */

typedef enum {
	ARV_STREAM_CALLBACK_TYPE_INIT,
	ARV_STREAM_CALLBACK_TYPE_EXIT,
	ARV_STREAM_CALLBACK_TYPE_START_BUFFER,
	ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE
} ArvStreamCallbackType;

typedef void (*ArvStreamCallback)	(void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer);

#define ARV_TYPE_STREAM             (arv_stream_get_type ())
#define ARV_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_STREAM, ArvStream))
#define ARV_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_STREAM, ArvStreamClass))
#define ARV_IS_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_STREAM))
#define ARV_IS_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_STREAM))
#define ARV_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_STREAM, ArvStreamClass))

typedef struct _ArvStreamClass ArvStreamClass;

struct _ArvStream {
	GObject	object;

	GAsyncQueue *input_queue;
	GAsyncQueue *output_queue;
};

struct _ArvStreamClass {
	GObjectClass parent_class;

	void		(*get_statistics)	(ArvStream *stream, guint64 *n_completed_buffers,
						 guint64 *n_failures, guint64 *n_underruns);
};

GType arv_stream_get_type (void);

void			arv_stream_push_buffer 			(ArvStream *stream, ArvBuffer *buffer);
ArvBuffer *		arv_stream_pop_buffer			(ArvStream *stream);
void 			arv_stream_get_n_buffers 		(ArvStream *stream,
								 gint *n_input_buffers,
								 gint *n_output_buffers);
void			arv_stream_get_statistics		(ArvStream *stream,
								 guint64 *n_completed_buffers,
								 guint64 *n_failures,
								 guint64 *n_underruns);

G_END_DECLS

#endif
