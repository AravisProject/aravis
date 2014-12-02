/* Aravis - Digital camera library
 *
 * Copyright © 2009-2012 Emmanuel Pacaud
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

#include <arvconfig.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/videooverlay.h>
#include <gdk/gdkx.h>
#include <arv.h>
#include <stdlib.h>
#include <math.h>
#include <glib/gi18n-lib.h>
#include <libnotify/notify.h>

static char *arv_viewer_option_debug_domains = NULL;
static gboolean arv_viewer_option_auto_socket_buffer = FALSE;
static gboolean arv_viewer_option_no_packet_resend = FALSE;
static unsigned int arv_viewer_option_packet_timeout = 20;
static unsigned int arv_viewer_option_frame_retention = 100;

typedef struct {
	ArvCamera *camera;
	ArvDevice *device;
	ArvStream *stream;
	ArvBuffer *last_buffer;

	const char *pixel_format_string;

	GstElement *pipeline;
	GstElement *appsrc;
	GstElement *transform;

	guint rotation;
	gboolean flip_vertical;
	gboolean flip_horizontal;

	guint64 timestamp_offset;
	guint64 last_timestamp;

	GtkWidget *main_window;
	GtkWidget *snapshot_button;
	GtkWidget *rotate_cw_button;
	GtkWidget *flip_vertical_toggle;
	GtkWidget *flip_horizontal_toggle;
	GtkWidget *drawing_area;
	GtkWidget *camera_combo_box;
	GtkWidget *trigger_combo_box;
	GtkWidget *frame_rate_entry;
	GtkWidget *exposure_spin_button;
	GtkWidget *gain_spin_button;
	GtkWidget *exposure_hscale;
	GtkWidget *gain_hscale;
	GtkWidget *auto_exposure_toggle;
	GtkWidget *auto_gain_toggle;

	gulong exposure_spin_changed;
	gulong gain_spin_changed;
	gulong exposure_hscale_changed;
	gulong gain_hscale_changed;
	gulong auto_gain_clicked;
	gulong auto_exposure_clicked;

	double exposure_min, exposure_max;

	guint gain_update_event;
	guint exposure_update_event;

	gulong video_window_xid;

	NotifyNotification *notification;
} ArvViewer;

double
arv_viewer_value_to_log (double value, double min, double max)
{
	if (min >= max)
		return 1.0;

	if (value < min)
		return 0.0;

	return (log10 (value) - log10 (min)) / (log10 (max) - log10 (min));
}

double
arv_viewer_value_from_log (double value, double min, double max)
{
	if (min <= 0.0 || max <= 0)
		return 0.0;

	if (value > 1.0)
		return max;
	if (value < 0.0)
		return min;

	return pow (10.0, (value * (log10 (max) - log10 (min)) + log10 (min)));
}

void
arv_viewer_update_device_list_cb (ArvViewer *viewer)
{
	GtkListStore *list_store;
	GtkTreeIter iter;
	unsigned int n_devices;
	unsigned int i;

	list_store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_combo_box_set_model (GTK_COMBO_BOX (viewer->camera_combo_box), GTK_TREE_MODEL (list_store));
	arv_update_device_list ();
	n_devices = arv_get_n_devices ();
	for (i = 0; i < n_devices; i++) {
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter, 0, arv_get_device_id (i), -1);
	}
	if (n_devices > 0)
		gtk_combo_box_set_active (GTK_COMBO_BOX (viewer->camera_combo_box), 0);
	if (n_devices <= 1)
		gtk_widget_set_sensitive (viewer->camera_combo_box, FALSE);
}

static GstBuffer *
_arv_to_gst_buffer (ArvBuffer *arv_buffer) 
{
	GstBuffer *buffer;
	int arv_row_stride;
	int width, height;
	char *buffer_data;
	size_t buffer_size;

	buffer_data = (char *) arv_buffer_get_data (arv_buffer, &buffer_size);
	arv_buffer_get_image_region (arv_buffer, NULL, NULL, &width, &height);
	arv_row_stride = width * ARV_PIXEL_FORMAT_BIT_PER_PIXEL (arv_buffer_get_image_pixel_format (arv_buffer)) / 8;

	/* Gstreamer requires row stride to be a multiple of 4 */
	if ((arv_row_stride & 0x3) != 0) {
		int gst_row_stride;
		size_t size;
		void *data;
		int i;

		gst_row_stride = (arv_row_stride & ~(0x3)) + 4;

		size = height * gst_row_stride;
		data = g_malloc (size);	

		for (i = 0; i < height; i++)
			memcpy (((char *) data) + i * gst_row_stride, buffer_data + i * arv_row_stride, arv_row_stride);

		buffer = gst_buffer_new_wrapped (data, size);
	} else {
		buffer = gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
						      buffer_data, buffer_size,
						      0, buffer_size, NULL, NULL);
	}

	GST_BUFFER_DTS (buffer) = 0;
	GST_BUFFER_DURATION (buffer) = 0;

	return buffer;
}

void
arv_viewer_new_buffer_cb (ArvStream *stream, ArvViewer *viewer)
{
	ArvBuffer *arv_buffer;

	arv_buffer = arv_stream_pop_buffer (stream);
	if (arv_buffer == NULL)
		return;

	if (arv_buffer_get_status (arv_buffer) == ARV_BUFFER_STATUS_SUCCESS) {
		GstBuffer *buffer;
		guint64 timestamp_ns;

		buffer = _arv_to_gst_buffer (arv_buffer);

		timestamp_ns =  g_get_real_time () * 1000LL;

		if (viewer->timestamp_offset == 0) {
			viewer->timestamp_offset = timestamp_ns;
			viewer->last_timestamp = timestamp_ns;
		}

		GST_BUFFER_DTS (buffer) = timestamp_ns - viewer->timestamp_offset;
		GST_BUFFER_DURATION (buffer) = timestamp_ns - viewer->last_timestamp;

		gst_app_src_push_buffer (GST_APP_SRC (viewer->appsrc), buffer);
	}

	if (viewer->last_buffer != NULL)
		arv_stream_push_buffer (stream, viewer->last_buffer);
	viewer->last_buffer = arv_buffer;
}

void
arv_viewer_frame_rate_entry_cb (GtkEntry *entry, ArvViewer *viewer)
{
	char *text;
	double frame_rate;

	text = (char *) gtk_entry_get_text (entry);

	arv_camera_set_frame_rate (viewer->camera, g_strtod (text, NULL));

	frame_rate = arv_camera_get_frame_rate (viewer->camera);
	text = g_strdup_printf ("%g", frame_rate);
	gtk_entry_set_text (entry, text);
	g_free (text);
}

static gboolean
arv_viewer_frame_rate_entry_focus_cb (GtkEntry *entry, GdkEventFocus *event,
		    ArvViewer *viewer)
{
	arv_viewer_frame_rate_entry_cb (entry, viewer);

	return FALSE;
}

void
arv_viewer_exposure_spin_cb (GtkSpinButton *spin_button, ArvViewer *viewer)
{
	double exposure = gtk_spin_button_get_value (spin_button);
	double log_exposure = arv_viewer_value_to_log (exposure, viewer->exposure_min, viewer->exposure_max);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_exposure_toggle), FALSE);

	arv_camera_set_exposure_time (viewer->camera, exposure);

	g_signal_handler_block (viewer->exposure_hscale, viewer->exposure_hscale_changed);
	gtk_range_set_value (GTK_RANGE (viewer->exposure_hscale), log_exposure);
	g_signal_handler_unblock (viewer->exposure_hscale, viewer->exposure_hscale_changed);
}

void
arv_viewer_gain_spin_cb (GtkSpinButton *spin_button, ArvViewer *viewer)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_gain_toggle), FALSE);

	arv_camera_set_gain (viewer->camera, gtk_spin_button_get_value (spin_button));

	g_signal_handler_block (viewer->gain_hscale, viewer->gain_hscale_changed);
	gtk_range_set_value (GTK_RANGE (viewer->gain_hscale), gtk_spin_button_get_value (spin_button));
	g_signal_handler_unblock (viewer->gain_hscale, viewer->gain_hscale_changed);
}

void
arv_viewer_exposure_scale_cb (GtkRange *range, ArvViewer *viewer)
{
	double log_exposure = gtk_range_get_value (range);
	double exposure = arv_viewer_value_from_log (log_exposure, viewer->exposure_min, viewer->exposure_max);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_exposure_toggle), FALSE);

	arv_camera_set_exposure_time (viewer->camera, exposure);

	g_signal_handler_block (viewer->exposure_spin_button, viewer->exposure_spin_changed);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->exposure_spin_button), exposure);
	g_signal_handler_unblock (viewer->exposure_spin_button, viewer->exposure_spin_changed);
}

void
arv_viewer_gain_scale_cb (GtkRange *range, ArvViewer *viewer)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_gain_toggle), FALSE);

	arv_camera_set_gain (viewer->camera, gtk_range_get_value (range));

	g_signal_handler_block (viewer->gain_spin_button, viewer->gain_spin_changed);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->gain_spin_button), gtk_range_get_value (range));
	g_signal_handler_unblock (viewer->gain_spin_button, viewer->gain_spin_changed);
}

gboolean
arv_viewer_update_exposure_cb (void *data)
{
	ArvViewer *viewer = data;
	double exposure;
	double log_exposure;

	exposure = arv_camera_get_exposure_time (viewer->camera);
	log_exposure = arv_viewer_value_to_log (exposure, viewer->exposure_min, viewer->exposure_max);

	g_signal_handler_block (viewer->exposure_hscale, viewer->exposure_hscale_changed);
	g_signal_handler_block (viewer->exposure_spin_button, viewer->exposure_spin_changed);
	gtk_range_set_value (GTK_RANGE (viewer->exposure_hscale), log_exposure);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->exposure_spin_button), exposure);
	g_signal_handler_unblock (viewer->exposure_spin_button, viewer->exposure_spin_changed);
	g_signal_handler_unblock (viewer->exposure_hscale, viewer->exposure_hscale_changed);

	return TRUE;
}

void
arv_viewer_update_exposure_ui (ArvViewer *viewer, gboolean is_auto)
{
	arv_viewer_update_exposure_cb (viewer);

	if (viewer->exposure_update_event > 0) {
		g_source_remove (viewer->exposure_update_event);
		viewer->exposure_update_event = 0;
	}

	if (is_auto)
		viewer->exposure_update_event = g_timeout_add_seconds (1, arv_viewer_update_exposure_cb, viewer);
}

void
arv_viewer_auto_exposure_cb (GtkToggleButton *toggle, ArvViewer *viewer)
{
	gboolean is_auto;

	is_auto = gtk_toggle_button_get_active (toggle);

	arv_camera_set_exposure_time_auto (viewer->camera, is_auto ? ARV_AUTO_CONTINUOUS : ARV_AUTO_OFF);
	arv_viewer_update_exposure_ui (viewer, is_auto);
}

gboolean
arv_viewer_update_gain_cb (void *data)
{
	ArvViewer *viewer = data;
	double gain;

	gain = arv_camera_get_gain (viewer->camera);

	g_signal_handler_block (viewer->gain_hscale, viewer->gain_hscale_changed);
	g_signal_handler_block (viewer->gain_spin_button, viewer->gain_spin_changed);
	gtk_range_set_value (GTK_RANGE (viewer->gain_hscale), gain);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->gain_spin_button), gain);
	g_signal_handler_unblock (viewer->gain_spin_button, viewer->gain_spin_changed);
	g_signal_handler_unblock (viewer->gain_hscale, viewer->gain_hscale_changed);

	return TRUE;
}

void
arv_viewer_update_gain_ui (ArvViewer *viewer, gboolean is_auto)
{
	arv_viewer_update_gain_cb (viewer);

	if (viewer->gain_update_event > 0) {
		g_source_remove (viewer->gain_update_event);
		viewer->gain_update_event = 0;
	}

	if (is_auto)
		viewer->gain_update_event = g_timeout_add_seconds (1, arv_viewer_update_gain_cb, viewer);

}

void
arv_viewer_auto_gain_cb (GtkToggleButton *toggle, ArvViewer *viewer)
{
	gboolean is_auto;

	is_auto = gtk_toggle_button_get_active (toggle);

	arv_camera_set_gain_auto (viewer->camera, is_auto ? ARV_AUTO_CONTINUOUS : ARV_AUTO_OFF);
	arv_viewer_update_gain_ui (viewer, is_auto);
}

void
arv_viewer_release_camera (ArvViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	if (viewer->pipeline != NULL)
		gst_element_set_state (viewer->pipeline, GST_STATE_NULL);

	if (viewer->stream != NULL) {
		g_object_unref (viewer->stream);
		viewer->stream = NULL;
	}

	if (viewer->camera != NULL) {
		g_object_unref (viewer->camera);
		viewer->camera = NULL;
		viewer->device = NULL;
	}

	if (viewer->pipeline != NULL) {
		g_object_unref (viewer->pipeline);
		viewer->pipeline = NULL;
		viewer->appsrc = NULL;
	}

	if (viewer->last_buffer != NULL) {
		g_object_unref (viewer->last_buffer);
		viewer->last_buffer = NULL;
	}

	viewer->timestamp_offset = 0;
	viewer->last_timestamp = 0;
}

void
arv_viewer_snapshot_cb (GtkButton *button, ArvViewer *viewer)
{
	GFile *file;
	char *path;
	char *filename;
	GDateTime *date;
	char *date_string;
	int width, height;
	const char *data;
	size_t size;

	g_return_if_fail (ARV_IS_CAMERA (viewer->camera));
	g_return_if_fail (ARV_IS_BUFFER (viewer->last_buffer));

	arv_buffer_get_image_region (viewer->last_buffer, NULL, NULL, &width, &height);
	data = arv_buffer_get_data (viewer->last_buffer, &size);

	path = g_build_filename (g_get_user_special_dir (G_USER_DIRECTORY_PICTURES),
					 "Aravis", NULL);
	file = g_file_new_for_path (path);
	g_free (path);
	g_file_make_directory (file, NULL, NULL);
	g_object_unref (file);

	date = g_date_time_new_now_local ();
	date_string = g_date_time_format (date, "%Y-%m-%d-%H:%M:%S");
	filename = g_strdup_printf ("%s-%s-%d-%d-%s-%s.raw",
				    arv_camera_get_vendor_name (viewer->camera),
				    arv_camera_get_device_id (viewer->camera),
				    width,
				    height,
				    viewer->pixel_format_string != NULL ? viewer->pixel_format_string : "Unknown",
				    date_string);
	path = g_build_filename (g_get_user_special_dir (G_USER_DIRECTORY_PICTURES),
				 "Aravis", filename, NULL);
	g_file_set_contents (path, data, size, NULL);

	if (viewer->notification) {
		notify_notification_update (viewer->notification,
					    "Snapshot saved to Image folder",
					    path,
					    "gtk-save");
		notify_notification_show (viewer->notification, NULL);
	}

	g_free (path);
	g_free (filename);
	g_free (date_string);
	g_date_time_unref (date);
}

static void
_update_transform (ArvViewer *viewer)
{
	static const gint methods[4][4] = {
		{0, 1, 2, 3},
		{4, 6, 5, 7},
		{5, 7, 4, 6},
		{2, 3, 0, 1}
	};
	int index = (viewer->flip_horizontal ? 1 : 0) + (viewer->flip_vertical ? 2 : 0);

	g_object_set (viewer->transform, "method", methods[index][viewer->rotation % 4], NULL);
}

void
arv_viewer_rotate_cw_cb (GtkButton *button, ArvViewer *viewer)
{
	viewer->rotation = (viewer->rotation + 1) % 4;

	_update_transform (viewer);
}

void
arv_viewer_flip_horizontal_cb (GtkToggleButton *toggle, ArvViewer *viewer)
{
	viewer->flip_horizontal = gtk_toggle_button_get_active (toggle);

	_update_transform (viewer);
}

void
arv_viewer_flip_vertical_cb (GtkToggleButton *toggle, ArvViewer *viewer)
{
	viewer->flip_vertical = gtk_toggle_button_get_active (toggle);

	_update_transform (viewer);
}

static GstBusSyncReply
bus_sync_handler (GstBus *bus, GstMessage *message, gpointer user_data)
{
	ArvViewer *viewer = user_data;

	if (!gst_is_video_overlay_prepare_window_handle_message(message))
		return GST_BUS_PASS;

	if (viewer->video_window_xid != 0) {
		GstVideoOverlay *videooverlay;

		videooverlay = GST_VIDEO_OVERLAY (GST_MESSAGE_SRC (message));
		gst_video_overlay_set_window_handle (videooverlay, viewer->video_window_xid);
	} else {
		g_warning ("Should have obtained video_window_xid by now!");
	}

	gst_message_unref (message);

	return GST_BUS_DROP;
}

static void
stream_cb (void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer)
{
	if (type == ARV_STREAM_CALLBACK_TYPE_INIT) {
		if (!arv_make_thread_realtime (10) &&
		    !arv_make_thread_high_priority (-10))
			g_warning ("Failed to make stream thread high priority");
	}
}

void
arv_viewer_select_camera_cb (GtkComboBox *combo_box, ArvViewer *viewer)
{
	GtkTreeIter iter;
	GtkTreeModel *list_store;
	GstCaps *caps;
	GstElement *videoconvert;
	GstElement *videosink;
	GstBus *bus;
	ArvPixelFormat pixel_format;
	char *camera_id;
	char *string;
	unsigned int payload;
	int width;
	int height;
	unsigned int i;
	double frame_rate;
	double gain_min, gain_max;
	gboolean auto_gain, auto_exposure;
	const char *caps_string;
	gboolean is_frame_rate_available;
	gboolean is_exposure_available;
	gboolean is_exposure_auto_available;
	gboolean is_gain_available;
	gboolean is_gain_auto_available;

	g_return_if_fail (viewer != NULL);

	arv_viewer_release_camera (viewer);

	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (viewer->camera_combo_box), &iter);
	list_store = gtk_combo_box_get_model (GTK_COMBO_BOX (viewer->camera_combo_box));
	gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 0, &camera_id, -1);
	viewer->camera = arv_camera_new (camera_id);
	g_free (camera_id);
	if (viewer->camera == NULL)
		return;

	arv_camera_set_chunk_mode (viewer->camera, FALSE);

	viewer->rotation = 0;
	viewer->stream = arv_camera_create_stream (viewer->camera, stream_cb, NULL);
	if (viewer->stream == NULL) {
		g_object_unref (viewer->camera);
		viewer->camera = NULL;
		return;
	}

	if (ARV_IS_GV_STREAM (viewer->stream)) {
		if (arv_viewer_option_auto_socket_buffer)
			g_object_set (viewer->stream,
				      "socket-buffer", ARV_GV_STREAM_SOCKET_BUFFER_AUTO,
				      "socket-buffer-size", 0,
				      NULL);
		if (arv_viewer_option_no_packet_resend)
			g_object_set (viewer->stream,
				      "packet-resend", ARV_GV_STREAM_PACKET_RESEND_NEVER,
				      NULL);
		g_object_set (viewer->stream,
			      "packet-timeout", (unsigned) arv_viewer_option_packet_timeout * 1000,
			      "frame-retention", (unsigned) arv_viewer_option_frame_retention * 1000,
			      NULL);
	}
	arv_stream_set_emit_signals (viewer->stream, TRUE);
	payload = arv_camera_get_payload (viewer->camera);
	for (i = 0; i < 50; i++)
		arv_stream_push_buffer (viewer->stream, arv_buffer_new (payload, NULL));

	arv_camera_get_region (viewer->camera, NULL, NULL, &width, &height);
	pixel_format = arv_camera_get_pixel_format (viewer->camera);
	viewer->pixel_format_string = arv_camera_get_pixel_format_as_string (viewer->camera);
	arv_camera_get_exposure_time_bounds (viewer->camera, &viewer->exposure_min, &viewer->exposure_max);
	arv_camera_get_gain_bounds (viewer->camera, &gain_min, &gain_max);
	frame_rate = arv_camera_get_frame_rate (viewer->camera);
	auto_gain = arv_camera_get_gain_auto (viewer->camera) != ARV_AUTO_OFF;
	auto_exposure = arv_camera_get_gain_auto (viewer->camera) != ARV_AUTO_OFF;

	is_frame_rate_available = arv_camera_is_frame_rate_available (viewer->camera);
	is_exposure_available = arv_camera_is_exposure_time_available (viewer->camera);
	is_exposure_auto_available = arv_camera_is_exposure_auto_available (viewer->camera);
	is_gain_available = arv_camera_is_gain_available (viewer->camera);
	is_gain_auto_available = arv_camera_is_gain_auto_available (viewer->camera);

	g_signal_handler_block (viewer->gain_hscale, viewer->gain_hscale_changed);
	g_signal_handler_block (viewer->gain_spin_button, viewer->gain_spin_changed);
	g_signal_handler_block (viewer->exposure_hscale, viewer->exposure_hscale_changed);
	g_signal_handler_block (viewer->exposure_spin_button, viewer->exposure_spin_changed);

	gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->exposure_spin_button),
				   viewer->exposure_min, viewer->exposure_max);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->exposure_spin_button), 200.0, 1000.0);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->gain_spin_button), gain_min, gain_max);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->gain_spin_button), 1, 10);

	gtk_range_set_range (GTK_RANGE (viewer->exposure_hscale), 0.0, 1.0);
	gtk_range_set_range (GTK_RANGE (viewer->gain_hscale), gain_min, gain_max);

	gtk_widget_set_sensitive (viewer->frame_rate_entry, is_frame_rate_available);

	string = g_strdup_printf ("%g", frame_rate);
	gtk_entry_set_text (GTK_ENTRY (viewer->frame_rate_entry), string);
	g_free (string);

	gtk_widget_set_sensitive (viewer->gain_hscale, is_gain_available);
	gtk_widget_set_sensitive (viewer->gain_spin_button, is_gain_available);

	gtk_widget_set_sensitive (viewer->exposure_hscale, is_exposure_available);
	gtk_widget_set_sensitive (viewer->exposure_spin_button, is_exposure_available);

	g_signal_handler_unblock (viewer->gain_hscale, viewer->gain_hscale_changed);
	g_signal_handler_unblock (viewer->gain_spin_button, viewer->gain_spin_changed);
	g_signal_handler_unblock (viewer->exposure_hscale, viewer->exposure_hscale_changed);
	g_signal_handler_unblock (viewer->exposure_spin_button, viewer->exposure_spin_changed);

	auto_gain = arv_camera_get_gain_auto (viewer->camera) != ARV_AUTO_OFF;
	auto_exposure = arv_camera_get_exposure_time_auto (viewer->camera) != ARV_AUTO_OFF;

	arv_viewer_update_gain_ui (viewer, auto_gain);
	arv_viewer_update_exposure_ui (viewer, auto_exposure);

	g_signal_handler_block (viewer->auto_gain_toggle, viewer->auto_gain_clicked);
	g_signal_handler_block (viewer->auto_exposure_toggle, viewer->auto_exposure_clicked);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_gain_toggle), auto_gain);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_exposure_toggle), auto_exposure);

	gtk_widget_set_sensitive (viewer->auto_gain_toggle, is_gain_auto_available);
	gtk_widget_set_sensitive (viewer->auto_exposure_toggle, is_exposure_auto_available);

	g_signal_handler_unblock (viewer->auto_gain_toggle, viewer->auto_gain_clicked);
	g_signal_handler_unblock (viewer->auto_exposure_toggle, viewer->auto_exposure_clicked);

	caps_string = arv_pixel_format_to_gst_caps_string (pixel_format);
	if (caps_string == NULL) {
		g_message ("GStreamer cannot understand the camera pixel format: 0x%x!\n", (int) pixel_format);
		return;
	}

	arv_camera_start_acquisition (viewer->camera);

	viewer->pipeline = gst_pipeline_new ("pipeline");

	viewer->appsrc = gst_element_factory_make ("appsrc", NULL);
	videoconvert = gst_element_factory_make ("videoconvert", NULL);
	viewer->transform = gst_element_factory_make ("videoflip", NULL);
	videosink = gst_element_factory_make ("autovideosink", NULL);

	if (g_str_has_prefix (caps_string, "video/x-bayer")) {
		GstElement *bayer2rgb;

		bayer2rgb = gst_element_factory_make ("bayer2rgb", NULL);

		gst_bin_add_many (GST_BIN (viewer->pipeline), viewer->appsrc, bayer2rgb,
				  videoconvert, viewer->transform, videosink, NULL);
		gst_element_link_many (viewer->appsrc, bayer2rgb,
				       videoconvert, viewer->transform, videosink, NULL);
	} else {
		gst_bin_add_many (GST_BIN (viewer->pipeline), viewer->appsrc,
				  videoconvert, viewer->transform, videosink, NULL);
		gst_element_link_many (viewer->appsrc, videoconvert,
				       viewer->transform, videosink, NULL);
	}

	caps = gst_caps_from_string (caps_string);
	gst_caps_set_simple (caps,
			     "width", G_TYPE_INT, width,
			     "height", G_TYPE_INT, height,
			     "framerate", GST_TYPE_FRACTION, (unsigned int ) (double) (0.5 + frame_rate), 1,
			     NULL);
	gst_app_src_set_caps (GST_APP_SRC (viewer->appsrc), caps);
	gst_caps_unref (caps);

	g_object_set(G_OBJECT (viewer->appsrc), "format", GST_FORMAT_TIME, NULL);

	bus = gst_pipeline_get_bus (GST_PIPELINE (viewer->pipeline));
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) bus_sync_handler, viewer, NULL);
	gst_object_unref (bus);

	gst_element_set_state (viewer->pipeline, GST_STATE_PLAYING);

	g_signal_connect (viewer->stream, "new-buffer", G_CALLBACK (arv_viewer_new_buffer_cb), viewer);
}

void
arv_viewer_free (ArvViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	if (viewer->notification)
		g_object_unref (viewer->notification);

	if (viewer->exposure_update_event > 0)
		g_source_remove (viewer->exposure_update_event);
	if (viewer->gain_update_event > 0)
		g_source_remove (viewer->gain_update_event);

	arv_viewer_release_camera (viewer);
}

void
arv_viewer_quit_cb (GtkWidget *widget, ArvViewer *viewer)
{
	arv_viewer_free (viewer);

	gtk_main_quit ();
}

static void
drawing_area_realize_cb (GtkWidget * widget, ArvViewer *viewer)
{
	viewer->video_window_xid = GDK_WINDOW_XID (gtk_widget_get_window (widget));
}

ArvViewer *
arv_viewer_new (void)
{
	GtkBuilder *builder;
	GtkCellRenderer *cell;
	ArvViewer *viewer;
	char *ui_filename;

	viewer = g_new0 (ArvViewer, 1);

	builder = gtk_builder_new ();

	ui_filename = g_build_filename (ARAVIS_DATA_DIR, "arv-viewer.ui", NULL);

	if (!gtk_builder_add_from_file (builder, ui_filename, NULL))
		g_error ("The user interface file is missing ('%s')", ui_filename);

	g_free (ui_filename);

	viewer->camera_combo_box = GTK_WIDGET (gtk_builder_get_object (builder, "camera_combobox"));
	viewer->snapshot_button = GTK_WIDGET (gtk_builder_get_object (builder, "snapshot_button"));
	viewer->main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	viewer->drawing_area = GTK_WIDGET (gtk_builder_get_object (builder, "video_drawingarea"));
	viewer->trigger_combo_box = GTK_WIDGET (gtk_builder_get_object (builder, "trigger_combobox"));
	viewer->frame_rate_entry = GTK_WIDGET (gtk_builder_get_object (builder, "frame_rate_entry"));
	viewer->exposure_spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "exposure_spinbutton"));
	viewer->gain_spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "gain_spinbutton"));
	viewer->exposure_hscale = GTK_WIDGET (gtk_builder_get_object (builder, "exposure_hscale"));
	viewer->gain_hscale = GTK_WIDGET (gtk_builder_get_object (builder, "gain_hscale"));
	viewer->auto_exposure_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "auto_exposure_togglebutton"));
	viewer->auto_gain_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "auto_gain_togglebutton"));
	viewer->rotate_cw_button = GTK_WIDGET (gtk_builder_get_object (builder, "rotate_cw_button"));
	viewer->flip_vertical_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "flip_vertical_togglebutton"));
	viewer->flip_horizontal_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "flip_horizontal_togglebutton"));

	g_object_unref (builder);

	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (viewer->camera_combo_box), cell, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (viewer->camera_combo_box), cell, "text", 0, NULL);

	gtk_widget_set_no_show_all (viewer->trigger_combo_box, TRUE);

	g_signal_connect (viewer->drawing_area, "realize", G_CALLBACK (drawing_area_realize_cb), viewer);

#if !GTK_CHECK_VERSION (3,14,0)
	gtk_widget_set_double_buffered (viewer->drawing_area, FALSE);
#endif

	gtk_widget_show_all (viewer->main_window);

	g_signal_connect (viewer->main_window, "destroy", G_CALLBACK (arv_viewer_quit_cb), viewer);
	g_signal_connect (viewer->snapshot_button, "clicked", G_CALLBACK (arv_viewer_snapshot_cb), viewer);
	g_signal_connect (viewer->rotate_cw_button, "clicked", G_CALLBACK (arv_viewer_rotate_cw_cb), viewer);
	g_signal_connect (viewer->flip_horizontal_toggle, "clicked", G_CALLBACK (arv_viewer_flip_horizontal_cb), viewer);
	g_signal_connect (viewer->flip_vertical_toggle, "clicked", G_CALLBACK (arv_viewer_flip_vertical_cb), viewer);
	g_signal_connect (viewer->camera_combo_box, "changed", G_CALLBACK (arv_viewer_select_camera_cb), viewer);

	g_signal_connect (viewer->frame_rate_entry, "activate", G_CALLBACK (arv_viewer_frame_rate_entry_cb), viewer);
	g_signal_connect (viewer->frame_rate_entry, "focus-out-event", G_CALLBACK (arv_viewer_frame_rate_entry_focus_cb), viewer);

	viewer->exposure_spin_changed = g_signal_connect (viewer->exposure_spin_button, "value-changed",
							  G_CALLBACK (arv_viewer_exposure_spin_cb), viewer);
	viewer->gain_spin_changed = g_signal_connect (viewer->gain_spin_button, "value-changed",
						      G_CALLBACK (arv_viewer_gain_spin_cb), viewer);
	viewer->exposure_hscale_changed = g_signal_connect (viewer->exposure_hscale, "value-changed",
							    G_CALLBACK (arv_viewer_exposure_scale_cb), viewer);
	viewer->gain_hscale_changed = g_signal_connect (viewer->gain_hscale, "value-changed",
							G_CALLBACK (arv_viewer_gain_scale_cb), viewer);
	viewer->auto_exposure_clicked = g_signal_connect (viewer->auto_exposure_toggle, "clicked",
							  G_CALLBACK (arv_viewer_auto_exposure_cb), viewer);
	viewer->auto_gain_clicked = g_signal_connect (viewer->auto_gain_toggle, "clicked",
						      G_CALLBACK (arv_viewer_auto_gain_cb), viewer);

	viewer->notification = notify_notification_new (NULL, NULL, NULL);

	return viewer;
}

static const GOptionEntry arv_viewer_option_entries[] =
{
	{
		"auto-buffer-size",			'a', 0, G_OPTION_ARG_NONE,
		&arv_viewer_option_auto_socket_buffer,	"Auto socket buffer size", NULL
	},
	{
		"no-packet-resend",			'r', 0, G_OPTION_ARG_NONE,
		&arv_viewer_option_no_packet_resend,	"No packet resend", NULL
	},
	{
		"packet-timeout", 			'p', 0, G_OPTION_ARG_INT,
		&arv_viewer_option_packet_timeout, 	"Packet timeout (ms)", NULL
	},
	{
		"frame-retention", 			'm', 0, G_OPTION_ARG_INT,
		&arv_viewer_option_frame_retention, 	"Frame retention (ms)", NULL
	},
	{
		"debug", 				'd', 0, G_OPTION_ARG_STRING,
		&arv_viewer_option_debug_domains, 	"Debug domains", NULL
	},
	{ NULL }
};

static gboolean
_gstreamer_plugin_check (void)
{
	GstRegistry *registry;
	GstPluginFeature *feature;
	unsigned int i;
	gboolean success = TRUE;

	static char *plugins[] = {
		"appsrc",
		"videoconvert",
		"videoflip",
		"autovideosink",
		"bayer2rgb"
	};

	registry = gst_registry_get ();

	for (i = 0; i < G_N_ELEMENTS (plugins); i++) {
		feature = gst_registry_lookup_feature (registry, plugins[i]);
		if (!GST_IS_PLUGIN_FEATURE (feature)) {
			g_print ("Gstreamer plugin '%s' is missing.\n", plugins[i]);
			success = FALSE;
		}
		else

		g_object_unref (feature);
	}

	if (!success)
		g_print ("Check your gstreamer installation.\n");

	/* Kludge, prevent autoloading of coglsink, which doesn't seem to work for us */
	feature = gst_registry_lookup_feature (registry, "coglsink");
	if (GST_IS_PLUGIN_FEATURE (feature)) {
		gst_plugin_feature_set_rank (feature, GST_RANK_NONE);
		g_object_unref (feature);
	}

	return success;
}

int
main (int argc,char *argv[])
{
	ArvViewer *viewer;
	GOptionContext *context;
	GError *error = NULL;

	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	arv_g_thread_init (NULL);

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, arv_viewer_option_entries, NULL);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_add_group (context, gst_init_get_option_group ());
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	gtk_init (&argc, &argv);
	gst_init (&argc, &argv);

	if (!_gstreamer_plugin_check ()) {
		arv_shutdown ();

		return EXIT_FAILURE;
	}

	arv_debug_enable (arv_viewer_option_debug_domains);

	arv_enable_interface ("Fake");

	notify_init ("Aravis Viewer");

	viewer = arv_viewer_new ();

	arv_viewer_update_device_list_cb (viewer);
	arv_viewer_select_camera_cb (NULL, viewer);

	gtk_main ();

	notify_uninit ();

	/* For debug purpose only */
	arv_shutdown ();

	return EXIT_SUCCESS;
}
