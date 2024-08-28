/* Aravis - Digital camera library
 *
 * Copyright © 2009-2021 Emmanuel Pacaud
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
 * SECTION: arvv4l2stream
 * @short_description: V4l2 stream
 */

#include <arvv4l2streamprivate.h>
#include <arvv4l2device.h>
#include <arvstreamprivate.h>
#include <arvbufferprivate.h>
#include <arvdebug.h>
#include <arvmisc.h>

typedef struct {
	ArvStream *stream;

	ArvV4l2Device *v4l2_device;
	ArvStreamCallback callback;
	void *callback_data;

	gboolean cancel;

	/* Statistics */

	guint n_completed_buffers;
	guint n_failures;
	guint n_underruns;
} ArvV4l2StreamThreadData;

typedef struct {
	GThread *thread;

	ArvV4l2StreamThreadData *thread_data;
} ArvV4l2StreamPrivate;

struct _ArvV4l2Stream {
	ArvStream	stream;
};

struct _ArvV4l2StreamClass {
	ArvStreamClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvV4l2Stream, arv_v4l2_stream, ARV_TYPE_STREAM, G_ADD_PRIVATE (ArvV4l2Stream))

/* Acquisition thread */

static void *
arv_v4l2_stream_thread (void *data)
{
	ArvV4l2StreamThreadData *thread_data = data;
	ArvBuffer *buffer;

	arv_info_stream_thread ("[V4l2Stream::thread] Start");

	if (thread_data->callback != NULL)
		thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_INIT, NULL);

	while (!g_atomic_int_get (&thread_data->cancel)) {
		sleep(1);
		buffer = arv_stream_pop_input_buffer (thread_data->stream);
		if (buffer != NULL) {
			if (thread_data->callback != NULL)
				thread_data->callback (thread_data->callback_data, ARV_STREAM_CALLBACK_TYPE_START_BUFFER,
						       NULL);

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

	arv_info_stream_thread ("[V4l2Stream::thread] Stop");

	return NULL;
}

/* ArvV4l2Stream implemenation */

static gboolean
arv_v4l2_stream_start_acquisition (ArvStream *stream, GError **error)
{
	ArvV4l2Stream *v4l2_stream = ARV_V4L2_STREAM (stream);
	ArvV4l2StreamPrivate *priv = arv_v4l2_stream_get_instance_private (v4l2_stream);
	ArvV4l2StreamThreadData *thread_data;

	g_return_val_if_fail (priv->thread == NULL, FALSE);
	g_return_val_if_fail (priv->thread_data != NULL, FALSE);

	thread_data = priv->thread_data;
	thread_data->cancel = FALSE;

	priv->thread = g_thread_new ("arv_v4l2_stream", arv_v4l2_stream_thread, priv->thread_data);

        return TRUE;
}

static gboolean
arv_v4l2_stream_stop_acquisition (ArvStream *stream, GError **error)
{
	ArvV4l2Stream *v4l2_stream = ARV_V4L2_STREAM (stream);
	ArvV4l2StreamPrivate *priv = arv_v4l2_stream_get_instance_private (v4l2_stream);
	ArvV4l2StreamThreadData *thread_data;

	g_return_val_if_fail (priv->thread != NULL, FALSE);
	g_return_val_if_fail (priv->thread_data != NULL, FALSE);

	thread_data = priv->thread_data;

	g_atomic_int_set (&thread_data->cancel, TRUE);
	g_thread_join (priv->thread);

	priv->thread = NULL;

        return TRUE;
}

/**
 * arv_v4l2_stream_new: (skip)
 * @camera: a #ArvV4l2Device
 * @callback: (scope call): image processing callback
 * @callback_data: (closure): user data for @callback
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Return Value: (transfer full): a new #ArvStream.
 */

ArvStream *
arv_v4l2_stream_new (ArvV4l2Device *device,
                     ArvStreamCallback callback, void *callback_data, GDestroyNotify destroy,
                     GError **error)
{
	return g_initable_new (ARV_TYPE_V4L2_STREAM, NULL, error,
			       "device", device,
			       "callback", callback,
			       "callback-data", callback_data,
                               "destroy-notify", destroy,
			       NULL);
}

static void
arv_v4l2_stream_constructed (GObject *object)
{
	ArvStream *stream = ARV_STREAM (object);
	ArvV4l2Stream *v4l2_stream = ARV_V4L2_STREAM (object);
	ArvV4l2StreamPrivate *priv = arv_v4l2_stream_get_instance_private (v4l2_stream);
	ArvV4l2StreamThreadData *thread_data;
	g_autoptr (ArvV4l2Device) v4l2_device = NULL;

	thread_data = g_new (ArvV4l2StreamThreadData, 1);
	thread_data->stream = stream;

	g_object_get (object,
		      "device", &thread_data->v4l2_device,
		      "callback", &thread_data->callback,
		      "callback-data", &thread_data->callback_data,
		      NULL);

	thread_data->cancel = FALSE;

	thread_data->n_completed_buffers = 0;
	thread_data->n_failures = 0;
	thread_data->n_underruns = 0;

	priv->thread_data = thread_data;

        arv_stream_declare_info (ARV_STREAM (v4l2_stream), "n_completed_buffers",
                                 G_TYPE_UINT64, &priv->thread_data->n_completed_buffers);
        arv_stream_declare_info (ARV_STREAM (v4l2_stream), "n_failures",
                                 G_TYPE_UINT64, &priv->thread_data->n_failures);
        arv_stream_declare_info (ARV_STREAM (v4l2_stream), "n_underruns",
                                 G_TYPE_UINT64, &priv->thread_data->n_underruns);
}

/* ArvStream implementation */

static void
arv_v4l2_stream_init (ArvV4l2Stream *v4l2_stream)
{
}

static void
arv_v4l2_stream_finalize (GObject *object)
{
	ArvV4l2Stream *v4l2_stream = ARV_V4L2_STREAM (object);
	ArvV4l2StreamPrivate *priv = arv_v4l2_stream_get_instance_private (v4l2_stream);

        if (priv->thread != NULL)
                arv_v4l2_stream_stop_acquisition (ARV_STREAM (v4l2_stream), NULL);

	if (priv->thread_data != NULL) {
		g_clear_object (&priv->thread_data->v4l2_device);
		g_clear_pointer (&priv->thread_data, g_free);
	}

	G_OBJECT_CLASS (arv_v4l2_stream_parent_class)->finalize (object);
}

static void
arv_v4l2_stream_class_init (ArvV4l2StreamClass *v4l2_stream_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (v4l2_stream_class);
	ArvStreamClass *stream_class = ARV_STREAM_CLASS (v4l2_stream_class);

	object_class->constructed = arv_v4l2_stream_constructed;
	object_class->finalize = arv_v4l2_stream_finalize;

	stream_class->start_acquisition = arv_v4l2_stream_start_acquisition;
	stream_class->stop_acquisition = arv_v4l2_stream_stop_acquisition;
}
