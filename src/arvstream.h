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

#ifndef ARV_STREAM_H
#define ARV_STREAM_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvbuffer.h>

G_BEGIN_DECLS

/**
 * ArvStreamCallbackType:
 * @ARV_STREAM_CALLBACK_TYPE_INIT: thread initialization, happens once
 * @ARV_STREAM_CALLBACK_TYPE_EXIT: thread end, happens once
 * @ARV_STREAM_CALLBACK_TYPE_START_BUFFER: buffer filling start, happens at each frame
 * @ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE: buffer filled, happens at each frame
 *
 * Describes when the reason the stream callback is called. You are probably more interested in
 * @ARV_STREAM_CALLBACK_TYPE_INIT and @ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE.
 */

typedef enum {
	ARV_STREAM_CALLBACK_TYPE_INIT,
	ARV_STREAM_CALLBACK_TYPE_EXIT,
	ARV_STREAM_CALLBACK_TYPE_START_BUFFER,
	ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE
} ArvStreamCallbackType;

#define ARV_TYPE_STREAM             (arv_stream_get_type ())
ARV_API G_DECLARE_DERIVABLE_TYPE (ArvStream, arv_stream, ARV, STREAM, GObject)

struct _ArvStreamClass {
	GObjectClass parent_class;

	void		(*start_thread)		(ArvStream *stream);
	void		(*stop_thread)		(ArvStream *stream);

	/* signals */
	void        	(*new_buffer)   	(ArvStream *stream);
};

/**
 * ArvStreamCallback:
 * @user_data: a pointer to user data associated to this callback
 * @type: reason of the callback call
 * @buffer: a [class@ArvBuffer] object
 *
 * This is the signature of the callback passed on an #ArvStream instantiation, which will be called on the stream
 * receiving thread initialization and finalization, and on every received buffer, once when the buffer is pulled from
 * the buffer queue, and one more when the buffer is done (successfully or not).
 *
 * @buffer is assured to be a valid #ArvBuffer object only when type is @ARV_STREAM_CALLBACK_TYPE_START_BUFFER or
 * @ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE.
 *
 * The callback is awaken from the stream receiving thread, which means it is forbidden to access to the camera
 * instance, except if you take care to protect the instance access from concurrent access. It also means all the time
 * spent in the callback is less time available for the incoming data handling. CPU intensive image processing should
 * happen elsewhere.
 */

typedef void (*ArvStreamCallback)	(void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer);

ARV_API void		arv_stream_push_buffer			(ArvStream *stream, ArvBuffer *buffer);
ARV_API ArvBuffer *	arv_stream_pop_buffer			(ArvStream *stream);
ARV_API ArvBuffer *	arv_stream_try_pop_buffer		(ArvStream *stream);
ARV_API ArvBuffer *	arv_stream_timeout_pop_buffer		(ArvStream *stream, guint64 timeout);
ARV_API void		arv_stream_get_n_buffers		(ArvStream *stream,
								 gint *n_input_buffers,
								 gint *n_output_buffers);
ARV_API void		arv_stream_start_thread			(ArvStream *stream);
ARV_API unsigned int	arv_stream_stop_thread			(ArvStream *stream, gboolean delete_buffers);

ARV_API void		arv_stream_get_statistics		(ArvStream *stream,
								 guint64 *n_completed_buffers,
								 guint64 *n_failures,
								 guint64 *n_underruns);

ARV_API guint		arv_stream_get_n_infos			(ArvStream *stream);
ARV_API const char *	arv_stream_get_info_name		(ArvStream *stream, guint id);
ARV_API GType		arv_stream_get_info_type		(ArvStream *stream, guint id);
ARV_API guint64		arv_stream_get_info_uint64		(ArvStream *stream, guint id);
ARV_API double		arv_stream_get_info_double		(ArvStream *stream, guint id);
ARV_API guint64		arv_stream_get_info_uint64_by_name	(ArvStream *stream, const char *name);
ARV_API double		arv_stream_get_info_double_by_name	(ArvStream *stream, const char *name);

ARV_API void		arv_stream_set_emit_signals		(ArvStream *stream, gboolean emit_signals);
ARV_API gboolean	arv_stream_get_emit_signals		(ArvStream *stream);

G_END_DECLS

#endif
