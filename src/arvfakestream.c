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
 * SECTION: arvfakestream
 * @short_description: Fake stream
 */

#include <arvfakestreamprivate.h>
#include <arvfakecamera.h>
#include <arvfakedevice.h>
#include <arvstreamprivate.h>
#include <arvbufferprivate.h>
#include <arvdebug.h>
#include <arvmisc.h>

typedef struct {
	ArvStream *stream;

	ArvFakeCamera *fake_camera;
	ArvStreamCallback callback;
	void *callback_data;

	gboolean cancel;

	/* Statistics */

	guint64 n_completed_buffers;
	guint64 n_failures;
	guint64 n_underruns;

        guint64 n_transferred_bytes;
        guint64 n_ignored_bytes;
} ArvFakeStreamThreadData;

typedef struct {
	GThread *thread;

	ArvFakeStreamThreadData *thread_data;
} ArvFakeStreamPrivate;

struct _ArvFakeStream {
	ArvStream	stream;
};

struct _ArvFakeStreamClass {
	ArvStreamClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvFakeStream, arv_fake_stream, ARV_TYPE_STREAM, G_ADD_PRIVATE (ArvFakeStream))

/* Acquisition thread */

static void *
arv_fake_stream_thread (void *data)
{
	ArvFakeStreamThreadData *thread_data = data;
	ArvBuffer *buffer;

	arv_debug_stream_thread ("[FakeStream::thread] Start");

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

	while (!g_atomic_int_get (&thread_data->cancel)) {
		arv_fake_camera_wait_for_next_frame (thread_data->fake_camera);
		buffer = arv_stream_pop_input_buffer (thread_data->stream);
		if (buffer != NULL) {
                        buffer->priv->received_size = 0;
			if (thread_data->callback != NULL)
				thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_START_BUFFER,
						       NULL);

			arv_fake_camera_fill_buffer (thread_data->fake_camera, buffer, NULL);

                        thread_data->n_transferred_bytes += buffer->priv->allocated_size;

			if (buffer->priv->status == ARV_BUFFER_STATUS_SUCCESS)
				thread_data->n_completed_buffers++;
			else
				thread_data->n_failures++;
			arv_stream_push_output_buffer (thread_data->stream, buffer);

			if (thread_data->callback != NULL)
				thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE,
						       buffer);
		} else
			thread_data->n_underruns++;
	}

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_EXIT, NULL);

	arv_debug_stream_thread ("[FakeStream::thread] Stop");

	return NULL;
}

/* ArvFakeStream implemenation */

static void
arv_fake_stream_start_thread (ArvStream *stream)
{
	ArvFakeStream *fake_stream = ARV_FAKE_STREAM (stream);
	ArvFakeStreamPrivate *priv = arv_fake_stream_get_instance_private (fake_stream);
	ArvFakeStreamThreadData *thread_data;

	g_return_if_fail (priv->thread == NULL);
	g_return_if_fail (priv->thread_data != NULL);

	thread_data = priv->thread_data;
	thread_data->cancel = FALSE;

	priv->thread = g_thread_new ("arv_fake_stream", arv_fake_stream_thread, priv->thread_data);
}

static void
arv_fake_stream_stop_thread (ArvStream *stream)
{
	ArvFakeStream *fake_stream = ARV_FAKE_STREAM (stream);
	ArvFakeStreamPrivate *priv = arv_fake_stream_get_instance_private (fake_stream);
	ArvFakeStreamThreadData *thread_data;

	g_return_if_fail (priv->thread != NULL);
	g_return_if_fail (priv->thread_data != NULL);

	thread_data = priv->thread_data;

	g_atomic_int_set (&thread_data->cancel, TRUE);
	g_thread_join (priv->thread);

	priv->thread = NULL;
}

/**
 * arv_fake_stream_new: (skip)
 * @camera: a #ArvFakeDevice
 * @callback: (scope call): image processing callback
 * @callback_data: (closure): user data for @callback
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Return Value: (transfer full): a new #ArvStream.
 */

ArvStream *
arv_fake_stream_new (ArvFakeDevice *device, ArvStreamCallback callback, void *callback_data, GDestroyNotify destroy, GError **error)
{
	return g_initable_new (ARV_TYPE_FAKE_STREAM, NULL, error,
			       "device", device,
			       "callback", callback,
			       "callback-data", callback_data,
						 "destroy-notify", destroy,
			       NULL);
}

static void
arv_fake_stream_constructed (GObject *object)
{
	ArvStream *stream = ARV_STREAM (object);
	ArvFakeStream *fake_stream = ARV_FAKE_STREAM (object);
	ArvFakeStreamPrivate *priv = arv_fake_stream_get_instance_private (fake_stream);
	ArvFakeStreamThreadData *thread_data;
	ArvFakeDevice *fake_device = NULL;

	thread_data = g_new0 (ArvFakeStreamThreadData, 1);
	thread_data->stream = stream;

	g_object_get (object,
		      "device", &fake_device,
		      "callback", &thread_data->callback,
		      "callback-data", &thread_data->callback_data,
		      NULL);

	thread_data->fake_camera = arv_fake_device_get_fake_camera (fake_device);

	thread_data->cancel = FALSE;

        arv_stream_declare_info (ARV_STREAM (fake_stream), "n_completed_buffers",
                                 G_TYPE_UINT64, &thread_data->n_completed_buffers);
        arv_stream_declare_info (ARV_STREAM (fake_stream), "n_failures",
                                 G_TYPE_UINT64, &thread_data->n_failures);
        arv_stream_declare_info (ARV_STREAM (fake_stream), "n_underruns",
                                 G_TYPE_UINT64, &thread_data->n_underruns);
        arv_stream_declare_info (ARV_STREAM (fake_stream), "n_transferred_bytes",
                                 G_TYPE_UINT64, &thread_data->n_transferred_bytes);
        arv_stream_declare_info (ARV_STREAM (fake_stream), "n_ignored_bytes",
                                 G_TYPE_UINT64, &thread_data->n_ignored_bytes);

	priv->thread_data = thread_data;

	arv_fake_stream_start_thread (ARV_STREAM (fake_stream));

        G_OBJECT_CLASS (arv_fake_stream_parent_class)->constructed (object);

        g_clear_object (&fake_device);
}

/* ArvStream implementation */

static void
arv_fake_stream_init (ArvFakeStream *fake_stream)
{
}

static void
arv_fake_stream_finalize (GObject *object)
{
	ArvFakeStream *fake_stream = ARV_FAKE_STREAM (object);
	ArvFakeStreamPrivate *priv = arv_fake_stream_get_instance_private (fake_stream);

	arv_fake_stream_stop_thread (ARV_STREAM (fake_stream));

	if (priv->thread_data != NULL) {
		g_clear_pointer (&priv->thread_data, g_free);
	}

	G_OBJECT_CLASS (arv_fake_stream_parent_class)->finalize (object);
}

static void
arv_fake_stream_class_init (ArvFakeStreamClass *fake_stream_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (fake_stream_class);
	ArvStreamClass *stream_class = ARV_STREAM_CLASS (fake_stream_class);

	object_class->constructed = arv_fake_stream_constructed;
	object_class->finalize = arv_fake_stream_finalize;

	stream_class->start_thread = arv_fake_stream_start_thread;
	stream_class->stop_thread = arv_fake_stream_stop_thread;
}
