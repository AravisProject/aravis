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

#include <arvfakestream.h>
#include <arvbuffer.h>
#include <arvdebug.h>

static GObjectClass *parent_class = NULL;

/* Acquisition thread */

typedef struct {
	ArvStreamCallback callback;
	void *user_data;

	gboolean cancel;
	GAsyncQueue *input_queue;
	GAsyncQueue *output_queue;

	/* Statistics */

	guint n_processed_buffers;
	guint n_failures;
	guint n_underruns;
} ArvFakeStreamThreadData;

static void *
arv_fake_stream_thread (void *data)
{
	ArvFakeStreamThreadData *thread_data = data;

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->user_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->user_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

	return NULL;
}

/* ArvFakeStream implemenation */

ArvStream *
arv_fake_stream_new (ArvStreamCallback callback, void *user_data)
{
	ArvFakeStream *fake_stream;
	ArvFakeStreamThreadData *thread_data;
	ArvStream *stream;

	fake_stream = g_object_new (ARV_TYPE_FAKE_STREAM, NULL);

	stream = ARV_STREAM (fake_stream);

	thread_data = g_new (ArvFakeStreamThreadData, 1);
	thread_data->callback = callback;
	thread_data->user_data = user_data;
	thread_data->cancel = FALSE;
	thread_data->input_queue = stream->input_queue;
	thread_data->output_queue = stream->output_queue;

	thread_data->n_processed_buffers = 0;
	thread_data->n_failures = 0;
	thread_data->n_underruns = 0;

	fake_stream->thread_data = thread_data;

	fake_stream->thread = g_thread_create (arv_fake_stream_thread, fake_stream->thread_data, TRUE, NULL);

	return ARV_STREAM (fake_stream);
}

/* ArvStream implementation */

static void
arv_fake_stream_get_statistics (ArvStream *stream,
				guint64 *n_processed_buffers,
				guint64 *n_failures,
				guint64 *n_underruns)
{
	ArvFakeStream *fake_stream = ARV_FAKE_STREAM (stream);
	ArvFakeStreamThreadData *thread_data;

	thread_data = fake_stream->thread_data;

	*n_processed_buffers = thread_data->n_processed_buffers;
	*n_failures = thread_data->n_failures;
	*n_underruns = thread_data->n_underruns;
}

static void
arv_fake_stream_init (ArvFakeStream *fake_stream)
{
}

static void
arv_fake_stream_finalize (GObject *object)
{
	ArvFakeStream *fake_stream = ARV_FAKE_STREAM (object);

	if (fake_stream->thread != NULL) {
		ArvFakeStreamThreadData *thread_data;

		thread_data = fake_stream->thread_data;

		thread_data->cancel = TRUE;
		g_thread_join (fake_stream->thread);
		g_free (thread_data);

		fake_stream->thread_data = NULL;
		fake_stream->thread = NULL;
	}

	parent_class->finalize (object);
}

static void
arv_fake_stream_class_init (ArvFakeStreamClass *fake_stream_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (fake_stream_class);
	ArvStreamClass *stream_class = ARV_STREAM_CLASS (fake_stream_class);

	parent_class = g_type_class_peek_parent (fake_stream_class);

	object_class->finalize = arv_fake_stream_finalize;

	stream_class->get_statistics = arv_fake_stream_get_statistics;
}

G_DEFINE_TYPE (ArvFakeStream, arv_fake_stream, ARV_TYPE_STREAM)
