/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* Video renderer for the viewer.
 *
 * The display path is chosen at runtime:
 *
 *  - gtk4paintablesink (gst-plugins-rs), when available. The sink exposes a
 *    GdkPaintable that a GtkPicture renders on the GPU. GTK repaints on its
 *    frame clock, so if frames arrive faster than the display refreshes only
 *    the newest one is seen.
 *
 *  - appsink, as a fallback. Decoded frames are copied into a single slot by
 *    the streaming thread and painted from a tick callback on the main
 *    thread. This decouples the decode rate from the paint rate: a slow UI
 *    skips frames instead of queueing them.
 *
 * Both paths present through a GtkPicture, so callers only deal with
 * arv_video_sink_get_element() and arv_video_sink_get_last_sample().
 */

#include "arvvideosink.h"

#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

/* The formats memory_format_from_info() can draw, in the order videoconvert
 * should prefer them. Anything else (packed YUV, for instance) is converted
 * by videoconvert before it reaches the sink. */
#define ARV_VIDEO_SINK_CPU_CAPS \
	"video/x-raw, format=(string){ RGB, BGR, RGBA, BGRA, GRAY8, GRAY16_LE }"

struct _ArvVideoSink {
	GtkWidget *frame;   /* borrowed: the GtkFrame we were attached to */
	GtkWidget *picture;
	GstElement *element;

	gboolean is_gpu;

	/* ---- CPU path: single decoded-frame slot ---- */
	GdkMemoryFormat format;
	GBytes *pending;          /* newest frame, owned by the streaming thread */
	guint64 pending_sequence; /* bumped on every new frame */
	guint64 painted_sequence; /* last sequence painted by the tick callback */
	GMutex mutex;

	int width;
	int height;

	guint tick_id;
};

static gboolean
sink_supports_paintable (void)
{
	static gsize checked = 0;
	static gboolean available = FALSE;

	if (g_once_init_enter (&checked)) {
		GstElementFactory *factory;

		factory = gst_element_factory_find ("gtk4paintablesink");
		available = factory != NULL;
		if (factory != NULL)
			gst_object_unref (factory);

		g_once_init_leave (&checked, 1);
	}

	return available;
}

/* ---- CPU path ---------------------------------------------------------- */

static GdkMemoryFormat
memory_format_from_info (const GstVideoInfo *info)
{
	switch (GST_VIDEO_INFO_FORMAT (info)) {
		case GST_VIDEO_FORMAT_RGB:
			return GDK_MEMORY_R8G8B8;
		case GST_VIDEO_FORMAT_BGR:
			return GDK_MEMORY_B8G8R8;
		case GST_VIDEO_FORMAT_RGBA:
			return GDK_MEMORY_R8G8B8A8;
		case GST_VIDEO_FORMAT_BGRA:
			return GDK_MEMORY_B8G8R8A8;
		case GST_VIDEO_FORMAT_GRAY8:
			return GDK_MEMORY_G8;
		case GST_VIDEO_FORMAT_GRAY16_LE:
			return GDK_MEMORY_G16;
		/* anything else would silently produce garbage, since the caller
		 * derives the stride from the pixel size */
		default:
			g_warning ("unsupported video format %s for CPU rendering",
				   GST_VIDEO_INFO_NAME (info));
			return GDK_MEMORY_R8G8B8;
	}
}

/* Streaming thread: keep only the newest frame. */
static GstFlowReturn
cpu_new_sample (GstElement *appsink, ArvVideoSink *sink)
{
	GstSample *sample;
	GstCaps *caps;
	GstBuffer *buffer;
	GstVideoInfo info;
	GstVideoFrame frame;
	GBytes *bytes = NULL;

	sample = gst_app_sink_pull_sample (GST_APP_SINK (appsink));
	if (sample == NULL)
		return GST_FLOW_ERROR;

	caps = gst_sample_get_caps (sample);
	buffer = gst_sample_get_buffer (sample);

	if (caps == NULL || buffer == NULL ||
	    !gst_video_info_from_caps (&info, caps) ||
	    !gst_video_frame_map (&frame, &info, buffer, GST_MAP_READ)) {
		gst_sample_unref (sample);
		return GST_FLOW_OK;
	}

	/* GdkMemoryTexture needs a tightly packed copy: the GstBuffer rows may
	 * be padded, and the buffer is recycled as soon as the sample is
	 * released. */
	{
		gint stride = (gint) GST_VIDEO_INFO_PLANE_STRIDE (&info, 0);
		gint packed = GST_VIDEO_INFO_WIDTH (&info) * GST_VIDEO_INFO_COMP_PSTRIDE (&info, 0);
		gint height = GST_VIDEO_INFO_HEIGHT (&info);
		guchar *src = GST_VIDEO_FRAME_PLANE_DATA (&frame, 0);
		guchar *data;
		gint y;

		data = g_malloc ((gsize) packed * height);
		for (y = 0; y < height; y++)
			memcpy (data + (gsize) y * packed, src + (gsize) y * stride, packed);

		bytes = g_bytes_new_take (data, (gsize) packed * height);

		g_mutex_lock (&sink->mutex);
		if (sink->pending != NULL)
			g_bytes_unref (sink->pending);
		sink->pending = bytes;
		sink->pending_sequence++;
		sink->format = memory_format_from_info (&info);
		sink->width = GST_VIDEO_INFO_WIDTH (&info);
		sink->height = GST_VIDEO_INFO_HEIGHT (&info);
		g_mutex_unlock (&sink->mutex);
	}

	gst_video_frame_unmap (&frame);
	gst_sample_unref (sample);

	return GST_FLOW_OK;
}

/* Main thread: paint the newest frame, if it changed. */
static gboolean
cpu_tick_cb (GtkWidget *widget, GdkFrameClock *clock, gpointer user_data)
{
	ArvVideoSink *sink = user_data;
	GBytes *bytes = NULL;
	GdkMemoryFormat format;
	gsize stride;
	int width, height;
	gboolean changed;
	guint64 sequence;

	g_mutex_lock (&sink->mutex);
	changed = sink->pending != NULL && sink->pending_sequence != sink->painted_sequence;
	if (changed) {
		bytes = g_bytes_ref (sink->pending);
		format = sink->format;
		width = sink->width;
		height = sink->height;
		sequence = sink->pending_sequence;
	}
	g_mutex_unlock (&sink->mutex);

	if (!changed)
		return G_SOURCE_CONTINUE;

	stride = g_bytes_get_size (bytes) / height;

	{
		GdkTexture *texture;

		texture = gdk_memory_texture_new (width, height, format, bytes, stride);
		if (texture != NULL) {
			gtk_picture_set_paintable (GTK_PICTURE (sink->picture),
						   GDK_PAINTABLE (texture));
			g_object_unref (texture);
		}
	}

	g_bytes_unref (bytes);

	g_mutex_lock (&sink->mutex);
	sink->painted_sequence = sequence;
	g_mutex_unlock (&sink->mutex);

	return G_SOURCE_CONTINUE;
}

/* ---- public API -------------------------------------------------------- */

ArvVideoSink *
arv_video_sink_new (GtkWidget *frame)
{
	ArvVideoSink *sink;
	GstElement *element;

	g_return_val_if_fail (GTK_IS_WIDGET (frame), NULL);

	sink = g_new0 (ArvVideoSink, 1);
	g_mutex_init (&sink->mutex);
	sink->is_gpu = sink_supports_paintable ();

	if (sink->is_gpu) {
		GdkPaintable *paintable = NULL;

		element = gst_element_factory_make ("gtk4paintablesink", NULL);
		g_object_get (element, "paintable", &paintable, NULL);

		sink->picture = gtk_picture_new_for_paintable (paintable);
		g_clear_object (&paintable);
	} else {
		GstCaps *accepted;

		element = gst_element_factory_make ("appsink", NULL);

		/* An appsink caps of ANY gives videoconvert nothing to negotiate
		 * toward, so a packed YUV format such as YUY2 reaches this sink
		 * unchanged and cannot be mapped to a GdkMemoryFormat. State the
		 * formats this renderer can draw, and videoconvert converts the rest
		 * (YCbCr422_8 included) before they arrive. */
		accepted = gst_caps_from_string (ARV_VIDEO_SINK_CPU_CAPS);
		g_object_set (element,
			      "emit-signals", TRUE,
			      "caps", accepted,
			      /* keep the queue shallow: the slot holds the newest frame,
			       * there is nothing to gain from buffering more */
			      "max-buffers", 2,
			      "drop", TRUE,
			      NULL);
		gst_caps_unref (accepted);

		sink->picture = gtk_picture_new ();
		g_signal_connect (element, "new-sample", G_CALLBACK (cpu_new_sample), sink);
		sink->tick_id = gtk_widget_add_tick_callback (sink->picture, cpu_tick_cb, sink, NULL);
	}

	/* match the previous gtksink behaviour: no clock synchronisation, so the
	 * sink renders everything it is given and never drops for lateness */
	g_object_set (element, "sync", FALSE, NULL);

	/* the pipeline owns the element; keep our own reference so it stays
	 * valid regardless of the order in which the pipeline and the sink are
	 * torn down */
	sink->element = gst_object_ref_sink (element);

	/* letterbox rather than stretch: GTK_CONTENT_FIT_CONTAIN is the
	 * replacement for the deprecated keep-aspect-ratio = TRUE */
	gtk_picture_set_content_fit (GTK_PICTURE (sink->picture), GTK_CONTENT_FIT_CONTAIN);
	gtk_widget_set_size_request (sink->picture, 640, 480);
	sink->frame = frame;
	gtk_frame_set_child (GTK_FRAME (frame), sink->picture);
	gtk_widget_set_visible (sink->picture, TRUE);

	return sink;
}

GstElement *
arv_video_sink_get_element (ArvVideoSink *sink)
{
	g_return_val_if_fail (sink != NULL, NULL);

	return sink->element;
}

GstSample *
arv_video_sink_get_last_sample (ArvVideoSink *sink)
{
	GstSample *sample = NULL;

	g_return_val_if_fail (sink != NULL, NULL);

	/* both sinks derive from GstBaseSink */
	if (GST_IS_BASE_SINK (sink->element))
		g_object_get (sink->element, "last-sample", &sample, NULL);

	return sample;
}

gboolean
arv_video_sink_is_gpu (ArvVideoSink *sink)
{
	g_return_val_if_fail (sink != NULL, FALSE);

	return sink->is_gpu;
}

void
arv_video_sink_free (ArvVideoSink *sink)
{
	GstElement *element;

	if (sink == NULL)
		return;

	/* Drop the tick callback while the picture is still alive: the caller
	 * may already have unparented it, which destroys the widget. */
	if (sink->tick_id > 0 && sink->picture != NULL)
		gtk_widget_remove_tick_callback (sink->picture, sink->tick_id);
	sink->tick_id = 0;

	/* Detach the sink element so no new-sample callback can reach a sink
	 * that is about to be freed. State is set to NULL before dropping our
	 * reference, which stops the streaming thread. */
	element = sink->element;
	sink->element = NULL;
	if (GST_IS_ELEMENT (element)) {
		g_signal_handlers_disconnect_by_data (element, sink);
		gst_element_set_state (element, GST_STATE_NULL);
		gst_object_unref (element);
	}

	/* Detach through the frame rather than unparenting directly: GtkFrame
	 * caches its child, and a bare gtk_widget_unparent() leaves the frame
	 * holding a dangling pointer. */
	if (sink->frame != NULL) {
		if (gtk_frame_get_child (GTK_FRAME (sink->frame)) == sink->picture)
			gtk_frame_set_child (GTK_FRAME (sink->frame), NULL);
		sink->frame = NULL;
	}
	sink->picture = NULL;

	if (sink->pending != NULL)
		g_bytes_unref (sink->pending);

	g_mutex_clear (&sink->mutex);
	g_free (sink);
}
