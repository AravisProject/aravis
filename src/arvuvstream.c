/* Aravis - Digital camera library
 *
 * Copyright © 2009-2016 Emmanuel Pacaud
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
 * SECTION: arvuvstream
 * @short_description: USB3Vision camera stream
 */

#include <arvuvstream.h>
#include <arvstreamprivate.h>
#include <arvbufferprivate.h>
#include <arvdebug.h>
#include <arvmisc.h>

static GObjectClass *parent_class = NULL;

struct _ArvUvStreamPrivate {
	GThread *thread;
	void *thread_data;
};

/* Acquisition thread */

typedef struct {
	ArvStream *stream;

	ArvStreamCallback callback;
	void *user_data;

	gboolean cancel;

	/* Statistics */

	guint n_completed_buffers;
	guint n_failures;
	guint n_underruns;
} ArvUvStreamThreadData;

static void *
arv_uv_stream_thread (void *data)
{
	ArvUvStreamThreadData *thread_data = data;

	arv_log_stream_thread ("[UvStream::thread] Start");

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->user_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

	while (!thread_data->cancel) {
		g_usleep (1000000);
	}

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->user_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

	arv_log_stream_thread ("[UvStream::thread] Stop");

	return NULL;
}

/* ArvUvStream implemenation */


/**
 * arv_uv_stream_new: (skip)
 * @callback: (scope call): image processing callback
 * @user_data: (closure): user data for @callback
 *
 * Return Value: (transfer full): a new #ArvStream.
 */

ArvStream *
arv_uv_stream_new (ArvStreamCallback callback, void *user_data)
{
	ArvUvStream *uv_stream;
	ArvUvStreamThreadData *thread_data;
	ArvStream *stream;

	uv_stream = g_object_new (ARV_TYPE_UV_STREAM, NULL);

	stream = ARV_STREAM (uv_stream);

	thread_data = g_new (ArvUvStreamThreadData, 1);
	thread_data->stream = stream;
	thread_data->callback = callback;
	thread_data->user_data = user_data;
	thread_data->cancel = FALSE;

	thread_data->n_completed_buffers = 0;
	thread_data->n_failures = 0;
	thread_data->n_underruns = 0;

	uv_stream->priv->thread_data = thread_data;
	uv_stream->priv->thread = arv_g_thread_new ("arv_uv_stream", arv_uv_stream_thread, uv_stream->priv->thread_data);

	return ARV_STREAM (uv_stream);
}

/* ArvStream implementation */

static void
arv_uv_stream_get_statistics (ArvStream *stream,
				guint64 *n_completed_buffers,
				guint64 *n_failures,
				guint64 *n_underruns)
{
	ArvUvStream *uv_stream = ARV_UV_STREAM (stream);
	ArvUvStreamThreadData *thread_data;

	thread_data = uv_stream->priv->thread_data;

	*n_completed_buffers = thread_data->n_completed_buffers;
	*n_failures = thread_data->n_failures;
	*n_underruns = thread_data->n_underruns;
}

static void
arv_uv_stream_init (ArvUvStream *uv_stream)
{
	uv_stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (uv_stream, ARV_TYPE_UV_STREAM, ArvUvStreamPrivate);
}

static void
arv_uv_stream_finalize (GObject *object)
{
	ArvUvStream *uv_stream = ARV_UV_STREAM (object);

	if (uv_stream->priv->thread != NULL) {
		ArvUvStreamThreadData *thread_data;

		thread_data = uv_stream->priv->thread_data;

		thread_data->cancel = TRUE;
		g_thread_join (uv_stream->priv->thread);
		g_free (thread_data);

		uv_stream->priv->thread_data = NULL;
		uv_stream->priv->thread = NULL;
	}

	parent_class->finalize (object);
}

static void
arv_uv_stream_class_init (ArvUvStreamClass *uv_stream_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (uv_stream_class);
	ArvStreamClass *stream_class = ARV_STREAM_CLASS (uv_stream_class);

	g_type_class_add_private (uv_stream_class, sizeof (ArvUvStreamPrivate));

	parent_class = g_type_class_peek_parent (uv_stream_class);

	object_class->finalize = arv_uv_stream_finalize;

	stream_class->get_statistics = arv_uv_stream_get_statistics;
}

G_DEFINE_TYPE (ArvUvStream, arv_uv_stream, ARV_TYPE_STREAM)
