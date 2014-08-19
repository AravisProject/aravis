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
 * SECTION: arvfakestream
 * @short_description: Fake camera stream
 */

#include <arvfakestream.h>
#include <arvstreamprivate.h>
#include <arvbufferprivate.h>
#include <arvdebug.h>
#include <arvmisc.h>

static GObjectClass *parent_class = NULL;

struct _ArvFakeStreamPrivate {
	GThread *thread;
	void *thread_data;

	ArvFakeCamera *camera;
};

/* Acquisition thread */

typedef struct {
	ArvFakeCamera *camera;
	ArvStream *stream;

	ArvStreamCallback callback;
	void *user_data;

	gboolean cancel;

	/* Statistics */

	guint n_completed_buffers;
	guint n_failures;
	guint n_underruns;
} ArvFakeStreamThreadData;

static void *
arv_fake_stream_thread (void *data)
{
	ArvFakeStreamThreadData *thread_data = data;
	ArvBuffer *buffer;

	arv_log_stream_thread ("[FakeStream::thread] Start");

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->user_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

	while (!thread_data->cancel) {
		arv_fake_camera_wait_for_next_frame (thread_data->camera);
		buffer = arv_stream_pop_input_buffer (thread_data->stream);
		if (buffer != NULL) {
			arv_fake_camera_fill_buffer (thread_data->camera, buffer, NULL);
			if (buffer->priv->status == ARV_BUFFER_STATUS_SUCCESS)
				thread_data->n_completed_buffers++;
			else
				thread_data->n_failures++;
			if (thread_data->callback != NULL)
				thread_data->callback (thread_data->user_data, ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
						       buffer);
			arv_stream_push_output_buffer (thread_data->stream, buffer);
		} else
			thread_data->n_underruns++;
	}

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->user_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

	arv_log_stream_thread ("[FakeStream::thread] Stop");

	return NULL;
}

/* ArvFakeStream implemenation */


/**
 * arv_fake_stream_new: (skip)
 * @camera: a #ArvFakeCamera
 * @callback: (scope call): image processing callback
 * @user_data: (closure): user data for @callback
 *
 * Return Value: (transfer full): a new #ArvStream.
 */

ArvStream *
arv_fake_stream_new (ArvFakeCamera *camera, ArvStreamCallback callback, void *user_data)
{
	ArvFakeStream *fake_stream;
	ArvFakeStreamThreadData *thread_data;
	ArvStream *stream;

	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), NULL);

	g_object_ref (camera);

	fake_stream = g_object_new (ARV_TYPE_FAKE_STREAM, NULL);

	stream = ARV_STREAM (fake_stream);

	thread_data = g_new (ArvFakeStreamThreadData, 1);
	thread_data->stream = stream;
	thread_data->camera = camera;
	thread_data->callback = callback;
	thread_data->user_data = user_data;
	thread_data->cancel = FALSE;

	thread_data->n_completed_buffers = 0;
	thread_data->n_failures = 0;
	thread_data->n_underruns = 0;

	fake_stream->priv->camera = camera;
	fake_stream->priv->thread_data = thread_data;
	fake_stream->priv->thread = arv_g_thread_new ("arv_fake_stream", arv_fake_stream_thread, fake_stream->priv->thread_data);

	return ARV_STREAM (fake_stream);
}

/* ArvStream implementation */

static void
arv_fake_stream_get_statistics (ArvStream *stream,
				guint64 *n_completed_buffers,
				guint64 *n_failures,
				guint64 *n_underruns)
{
	ArvFakeStream *fake_stream = ARV_FAKE_STREAM (stream);
	ArvFakeStreamThreadData *thread_data;

	thread_data = fake_stream->priv->thread_data;

	*n_completed_buffers = thread_data->n_completed_buffers;
	*n_failures = thread_data->n_failures;
	*n_underruns = thread_data->n_underruns;
}

static void
arv_fake_stream_init (ArvFakeStream *fake_stream)
{
	fake_stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (fake_stream, ARV_TYPE_FAKE_STREAM, ArvFakeStreamPrivate);
}

static void
arv_fake_stream_finalize (GObject *object)
{
	ArvFakeStream *fake_stream = ARV_FAKE_STREAM (object);

	if (fake_stream->priv->thread != NULL) {
		ArvFakeStreamThreadData *thread_data;

		thread_data = fake_stream->priv->thread_data;

		thread_data->cancel = TRUE;
		g_thread_join (fake_stream->priv->thread);
		g_free (thread_data);

		fake_stream->priv->thread_data = NULL;
		fake_stream->priv->thread = NULL;
	}

	g_object_unref (fake_stream->priv->camera);

	parent_class->finalize (object);
}

static void
arv_fake_stream_class_init (ArvFakeStreamClass *fake_stream_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (fake_stream_class);
	ArvStreamClass *stream_class = ARV_STREAM_CLASS (fake_stream_class);

	g_type_class_add_private (fake_stream_class, sizeof (ArvFakeStreamPrivate));

	parent_class = g_type_class_peek_parent (fake_stream_class);

	object_class->finalize = arv_fake_stream_finalize;

	stream_class->get_statistics = arv_fake_stream_get_statistics;
}

G_DEFINE_TYPE (ArvFakeStream, arv_fake_stream, ARV_TYPE_STREAM)
