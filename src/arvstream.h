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

#ifndef ARV_STREAM_H
#define ARV_STREAM_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvbuffer.h>

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

#define ARV_TYPE_STREAM             (arv_stream_get_type ())
G_DECLARE_DERIVABLE_TYPE (ArvStream, arv_stream, ARV, STREAM, GObject)

struct _ArvStreamClass {
	GObjectClass parent_class;

	void		(*start_thread)		(ArvStream *stream);
	void		(*stop_thread)		(ArvStream *stream);
	void		(*get_statistics)	(ArvStream *stream, guint64 *n_completed_buffers,
						 guint64 *n_failures, guint64 *n_underruns);

	/* signals */
	void        	(*new_buffer)   	(ArvStream *stream);
};

typedef void (*ArvStreamCallback)	(void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer);

void		arv_stream_push_buffer 			(ArvStream *stream, ArvBuffer *buffer);
ArvBuffer *	arv_stream_pop_buffer			(ArvStream *stream);
ArvBuffer *	arv_stream_try_pop_buffer		(ArvStream *stream);
ArvBuffer * 	arv_stream_timeout_pop_buffer 		(ArvStream *stream, guint64 timeout);
void 		arv_stream_get_n_buffers 		(ArvStream *stream,
							 gint *n_input_buffers,
							 gint *n_output_buffers);
void		arv_stream_start_thread			(ArvStream *stream);
unsigned int	arv_stream_stop_thread			(ArvStream *stream, gboolean delete_buffers);

void		arv_stream_get_statistics		(ArvStream *stream,
							 guint64 *n_completed_buffers,
							 guint64 *n_failures,
							 guint64 *n_underruns);

void 		arv_stream_set_emit_signals 		(ArvStream *stream, gboolean emit_signals);
gboolean 	arv_stream_get_emit_signals 		(ArvStream *stream);

G_END_DECLS

#endif
