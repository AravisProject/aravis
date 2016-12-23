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

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/videooverlay.h>
#include <gdk/gdkx.h>
#include <arv.h>
#include <math.h>
#include <memory.h>
#include <libnotify/notify.h>

static gboolean has_autovideo_sink = FALSE;
static gboolean has_gtksink = FALSE;
static gboolean has_gtkglsink = FALSE;

static gboolean
gstreamer_plugin_check (void)
{
	static gsize check_done = 0;
	static gboolean check_success = FALSE;

	if (g_once_init_enter (&check_done)) {
		GstRegistry *registry;
		GstPluginFeature *feature;
		unsigned int i;
		gboolean success = TRUE;

		static char *plugins[] = {
			"appsrc",
			"videoconvert",
			"videoflip",
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

		feature = gst_registry_lookup_feature (registry, "autovideosink");
		if (GST_IS_PLUGIN_FEATURE (feature)) {
			has_autovideo_sink = TRUE;
			g_object_unref (feature);
		}

		feature = gst_registry_lookup_feature (registry, "gtksink");
		if (GST_IS_PLUGIN_FEATURE (feature)) {
			has_gtksink = TRUE;
			g_object_unref (feature);
		}

		feature = gst_registry_lookup_feature (registry, "gtkglsink");
		if (GST_IS_PLUGIN_FEATURE (feature)) {
			has_gtkglsink = TRUE;
			g_object_unref (feature);
		}

		if (!has_autovideo_sink && !has_gtkglsink && !has_gtksink) {
			g_print ("Missing GStreamer video output plugin (autovideosink, gtksink or gtkglsink)\n");
			success = FALSE;
		}

		if (!success)
			g_print ("Check your gstreamer installation.\n");

		/* Kludge, prevent autoloading of coglsink, which doesn't seem to work for us */
		feature = gst_registry_lookup_feature (registry, "coglsink");
		if (GST_IS_PLUGIN_FEATURE (feature)) {
			gst_plugin_feature_set_rank (feature, GST_RANK_NONE);
			g_object_unref (feature);
		}

		check_success = success;

		g_once_init_leave (&check_done, 1);
	}

	return check_success;
}

typedef struct {
	GtkApplication parent_instance;

	ArvCamera *camera;
	char *camera_name;
	ArvStream *stream;
	ArvBuffer *last_buffer;

	GstElement *pipeline;
	GstElement *appsrc;
	GstElement *transform;

	guint64 timestamp_offset;
	guint64 last_timestamp;

	guint rotation;
	gboolean flip_vertical;
	gboolean flip_horizontal;

	double exposure_min, exposure_max;

	NotifyNotification *notification;

	GtkWidget *main_window;
	GtkWidget *main_stack;
	GtkWidget *main_headerbar;
	GtkWidget *camera_box;
	GtkWidget *refresh_button;
	GtkWidget *video_mode_button;
	GtkWidget *camera_tree;
	GtkWidget *back_button;
	GtkWidget *snapshot_button;
	GtkWidget *rotate_cw_button;
	GtkWidget *flip_vertical_toggle;
	GtkWidget *flip_horizontal_toggle;
	GtkWidget *camera_parameters;
	GtkWidget *pixel_format_combo;
	GtkWidget *camera_x;
	GtkWidget *camera_y;
	GtkWidget *camera_binning_x;
	GtkWidget *camera_binning_y;
	GtkWidget *camera_width;
	GtkWidget *camera_height;
	GtkWidget *video_frame;
	GtkWidget *trigger_combo_box;
	GtkWidget *frame_rate_entry;
	GtkWidget *exposure_spin_button;
	GtkWidget *gain_spin_button;
	GtkWidget *exposure_hscale;
	GtkWidget *gain_hscale;
	GtkWidget *auto_exposure_toggle;
	GtkWidget *auto_gain_toggle;
	GtkWidget *acquisition_button;

	gulong camera_selected;
	gulong exposure_spin_changed;
	gulong gain_spin_changed;
	gulong exposure_hscale_changed;
	gulong gain_hscale_changed;
	gulong auto_gain_clicked;
	gulong auto_exposure_clicked;
	gulong camera_x_changed;
	gulong camera_y_changed;
	gulong camera_binning_x_changed;
	gulong camera_binning_y_changed;
	gulong camera_width_changed;
	gulong camera_height_changed;
	gulong pixel_format_changed;

	guint gain_update_event;
	guint exposure_update_event;

	gboolean auto_socket_buffer;
	gboolean packet_resend;
	guint packet_timeout;
	guint frame_retention;

	gulong video_window_xid;
} ArvViewer;

typedef GtkApplicationClass ArvViewerClass;

G_DEFINE_TYPE (ArvViewer, arv_viewer, GTK_TYPE_APPLICATION)

typedef enum {
	ARV_VIEWER_MODE_CAMERA_LIST,
	ARV_VIEWER_MODE_VIDEO
} ArvViewerMode;

static void 	select_mode 	(ArvViewer *viewer, ArvViewerMode mode);

void
arv_viewer_set_options (ArvViewer *viewer,
			gboolean auto_socket_buffer,
			gboolean packet_resend,
			guint packet_timeout,
			guint frame_retention)
{
	g_return_if_fail (viewer != NULL);

	viewer->auto_socket_buffer = auto_socket_buffer;
	viewer->packet_resend = packet_resend;
	viewer->packet_timeout = packet_timeout;
	viewer->frame_retention = frame_retention;
}

static double
arv_viewer_value_to_log (double value, double min, double max)
{
	if (min >= max)
		return 1.0;

	if (value < min)
		return 0.0;

	return (log10 (value) - log10 (min)) / (log10 (max) - log10 (min));
}

static double
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

static GstBuffer *
arv_to_gst_buffer (ArvBuffer *arv_buffer)
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

static void
new_buffer_cb (ArvStream *stream, ArvViewer *viewer)
{
	ArvBuffer *arv_buffer;

	arv_buffer = arv_stream_pop_buffer (stream);
	if (arv_buffer == NULL)
		return;

	if (arv_buffer_get_status (arv_buffer) == ARV_BUFFER_STATUS_SUCCESS) {
		GstBuffer *buffer;
		guint64 timestamp_ns;

		buffer = arv_to_gst_buffer (arv_buffer);

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

static void
frame_rate_entry_cb (GtkEntry *entry, ArvViewer *viewer)
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
frame_rate_entry_focus_cb (GtkEntry *entry, GdkEventFocus *event,
		    ArvViewer *viewer)
{
	frame_rate_entry_cb (entry, viewer);

	return FALSE;
}

static void
exposure_spin_cb (GtkSpinButton *spin_button, ArvViewer *viewer)
{
	double exposure = gtk_spin_button_get_value (spin_button);
	double log_exposure = arv_viewer_value_to_log (exposure, viewer->exposure_min, viewer->exposure_max);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_exposure_toggle), FALSE);

	arv_camera_set_exposure_time (viewer->camera, exposure);

	g_signal_handler_block (viewer->exposure_hscale, viewer->exposure_hscale_changed);
	gtk_range_set_value (GTK_RANGE (viewer->exposure_hscale), log_exposure);
	g_signal_handler_unblock (viewer->exposure_hscale, viewer->exposure_hscale_changed);
}

static void
gain_spin_cb (GtkSpinButton *spin_button, ArvViewer *viewer)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_gain_toggle), FALSE);

	arv_camera_set_gain (viewer->camera, gtk_spin_button_get_value (spin_button));

	g_signal_handler_block (viewer->gain_hscale, viewer->gain_hscale_changed);
	gtk_range_set_value (GTK_RANGE (viewer->gain_hscale), gtk_spin_button_get_value (spin_button));
	g_signal_handler_unblock (viewer->gain_hscale, viewer->gain_hscale_changed);
}

static void
exposure_scale_cb (GtkRange *range, ArvViewer *viewer)
{
	double log_exposure = gtk_range_get_value (range);
	double exposure = arv_viewer_value_from_log (log_exposure, viewer->exposure_min, viewer->exposure_max);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_exposure_toggle), FALSE);

	arv_camera_set_exposure_time (viewer->camera, exposure);

	g_signal_handler_block (viewer->exposure_spin_button, viewer->exposure_spin_changed);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->exposure_spin_button), exposure);
	g_signal_handler_unblock (viewer->exposure_spin_button, viewer->exposure_spin_changed);
}

static void
gain_scale_cb (GtkRange *range, ArvViewer *viewer)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_gain_toggle), FALSE);

	arv_camera_set_gain (viewer->camera, gtk_range_get_value (range));

	g_signal_handler_block (viewer->gain_spin_button, viewer->gain_spin_changed);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->gain_spin_button), gtk_range_get_value (range));
	g_signal_handler_unblock (viewer->gain_spin_button, viewer->gain_spin_changed);
}

gboolean
update_exposure_cb (void *data)
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

static void
update_exposure_ui (ArvViewer *viewer, gboolean is_auto)
{
	update_exposure_cb (viewer);

	if (viewer->exposure_update_event > 0) {
		g_source_remove (viewer->exposure_update_event);
		viewer->exposure_update_event = 0;
	}

	if (is_auto)
		viewer->exposure_update_event = g_timeout_add_seconds (1, update_exposure_cb, viewer);
}

static void
auto_exposure_cb (GtkToggleButton *toggle, ArvViewer *viewer)
{
	gboolean is_auto;

	is_auto = gtk_toggle_button_get_active (toggle);

	arv_camera_set_exposure_time_auto (viewer->camera, is_auto ? ARV_AUTO_CONTINUOUS : ARV_AUTO_OFF);
	update_exposure_ui (viewer, is_auto);
}

static gboolean
update_gain_cb (void *data)
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

static void
update_gain_ui (ArvViewer *viewer, gboolean is_auto)
{
	update_gain_cb (viewer);

	if (viewer->gain_update_event > 0) {
		g_source_remove (viewer->gain_update_event);
		viewer->gain_update_event = 0;
	}

	if (is_auto)
		viewer->gain_update_event = g_timeout_add_seconds (1, update_gain_cb, viewer);

}


void
auto_gain_cb (GtkToggleButton *toggle, ArvViewer *viewer)
{
	gboolean is_auto;

	is_auto = gtk_toggle_button_get_active (toggle);

	arv_camera_set_gain_auto (viewer->camera, is_auto ? ARV_AUTO_CONTINUOUS : ARV_AUTO_OFF);
	update_gain_ui (viewer, is_auto);
}

void
snapshot_cb (GtkButton *button, ArvViewer *viewer)
{
	GFile *file;
	char *path;
	char *filename;
	GDateTime *date;
	char *date_string;
	int width, height;
	const char *data;
	const char *pixel_format;
	size_t size;

	g_return_if_fail (ARV_IS_CAMERA (viewer->camera));
	g_return_if_fail (ARV_IS_BUFFER (viewer->last_buffer));

	pixel_format = arv_camera_get_pixel_format_as_string (viewer->camera);
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
				    pixel_format != NULL ? pixel_format : "Unknown",
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
update_transform (ArvViewer *viewer)
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

static void
rotate_cw_cb (GtkButton *button, ArvViewer *viewer)
{
	viewer->rotation = (viewer->rotation + 1) % 4;

	update_transform (viewer);
}

static void
flip_horizontal_cb (GtkToggleButton *toggle, ArvViewer *viewer)
{
	viewer->flip_horizontal = gtk_toggle_button_get_active (toggle);

	update_transform (viewer);
}

static void
flip_vertical_cb (GtkToggleButton *toggle, ArvViewer *viewer)
{
	viewer->flip_vertical = gtk_toggle_button_get_active (toggle);

	update_transform (viewer);
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

static void
update_camera_region (ArvViewer *viewer)
{
	gint x, y, width, height;
	gint dx, dy;
	gint min, max;

	g_signal_handler_block (viewer->camera_x, viewer->camera_x_changed);
	g_signal_handler_block (viewer->camera_y, viewer->camera_y_changed);
	g_signal_handler_block (viewer->camera_width, viewer->camera_width_changed);
	g_signal_handler_block (viewer->camera_height, viewer->camera_height_changed);
	g_signal_handler_block (viewer->camera_binning_x, viewer->camera_binning_x_changed);
	g_signal_handler_block (viewer->camera_binning_y, viewer->camera_binning_y_changed);

	arv_camera_get_region (viewer->camera, &x, &y, &width, &height);
	arv_camera_get_binning (viewer->camera, &dx, &dy);

	arv_camera_get_x_binning_bounds (viewer->camera, &min, &max);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->camera_binning_x), min, max);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->camera_binning_x), 1, 10);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->camera_binning_x), dx);
	arv_camera_get_y_binning_bounds (viewer->camera, &min, &max);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->camera_binning_y), min, max);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->camera_binning_y), 1, 10);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->camera_binning_y), dy);
	arv_camera_get_x_offset_bounds (viewer->camera, &min, &max);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->camera_x), min, max);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->camera_x), 1, 10);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->camera_x), x);
	arv_camera_get_y_offset_bounds (viewer->camera, &min, &max);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->camera_y), min, max);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->camera_y), 1, 10);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->camera_y), y);
	arv_camera_get_width_bounds (viewer->camera, &min, &max);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->camera_width), min, max);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->camera_width), 1, 10);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->camera_width), width);
	arv_camera_get_height_bounds (viewer->camera, &min, &max);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->camera_height), min, max);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->camera_height), 1, 10);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->camera_height), height);

	g_signal_handler_unblock (viewer->camera_x, viewer->camera_x_changed);
	g_signal_handler_unblock (viewer->camera_y, viewer->camera_y_changed);
	g_signal_handler_unblock (viewer->camera_width, viewer->camera_width_changed);
	g_signal_handler_unblock (viewer->camera_height, viewer->camera_height_changed);
	g_signal_handler_unblock (viewer->camera_binning_x, viewer->camera_binning_x_changed);
	g_signal_handler_unblock (viewer->camera_binning_y, viewer->camera_binning_y_changed);
}

void
camera_region_cb (GtkSpinButton *spin_button, ArvViewer *viewer)
{
	int x = gtk_spin_button_get_value (GTK_SPIN_BUTTON (viewer->camera_x));
	int y = gtk_spin_button_get_value (GTK_SPIN_BUTTON (viewer->camera_y));
	int width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (viewer->camera_width));
	int height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (viewer->camera_height));

	arv_camera_set_region (viewer->camera, x, y, width, height);

	update_camera_region (viewer);
}

void
camera_binning_cb (GtkSpinButton *spin_button, ArvViewer *viewer)
{
	int dx = gtk_spin_button_get_value (GTK_SPIN_BUTTON (viewer->camera_binning_x));
	int dy = gtk_spin_button_get_value (GTK_SPIN_BUTTON (viewer->camera_binning_y));

	arv_camera_set_binning (viewer->camera, dx, dy);

	update_camera_region (viewer);
}

void
pixel_format_combo_cb (GtkComboBoxText *combo, ArvViewer *viewer)
{
	char *pixel_format;

	pixel_format = gtk_combo_box_text_get_active_text (combo);
	arv_camera_set_pixel_format_from_string (viewer->camera, pixel_format);
	g_free (pixel_format);
}

void
update_device_list_cb (GtkToolButton *button, ArvViewer *viewer)
{
	GtkListStore *list_store;
	GtkTreeIter iter;
	unsigned int n_devices;
	unsigned int i;

	gtk_widget_set_sensitive (viewer->video_mode_button, FALSE);
	gtk_widget_set_sensitive (viewer->camera_parameters, FALSE);

	g_signal_handler_block (gtk_tree_view_get_selection (GTK_TREE_VIEW (viewer->camera_tree)), viewer->camera_selected);
	list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (viewer->camera_tree)));
	gtk_list_store_clear (list_store);
	arv_update_device_list ();
	n_devices = arv_get_n_devices ();
	for (i = 0; i < n_devices; i++) {
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    0, arv_get_device_id (i),
				    1, arv_get_device_vendor (i),
				    2, arv_get_device_model (i),
				    3, arv_get_device_serial_nbr (i),
				    -1);
	}
	g_signal_handler_unblock (gtk_tree_view_get_selection (GTK_TREE_VIEW (viewer->camera_tree)), viewer->camera_selected);
}

static void
remove_widget (GtkWidget *widget, gpointer data)
{
	gtk_container_remove (data, widget);
	g_object_unref (widget);
}

static void
stop_video (ArvViewer *viewer)
{
	if (GST_IS_PIPELINE (viewer->pipeline))
		gst_element_set_state (viewer->pipeline, GST_STATE_NULL);

	g_clear_object (&viewer->stream);
	g_clear_object (&viewer->pipeline);

	viewer->appsrc = NULL;

	g_clear_object (&viewer->last_buffer);

	gtk_container_foreach (GTK_CONTAINER (viewer->video_frame), remove_widget, viewer->video_frame);
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

static gboolean
start_video (ArvViewer *viewer)
{
	GstElement *videoconvert;
	GstElement *videosink;
	GstCaps *caps;
	ArvPixelFormat pixel_format;
	double frame_rate;
	double gain_min, gain_max;
	unsigned payload;
	unsigned i;
	gint width, height;
	char *string;
	const char *caps_string;
	gboolean auto_gain, auto_exposure;
	gboolean is_frame_rate_available;
	gboolean is_exposure_available;
	gboolean is_exposure_auto_available;
	gboolean is_gain_available;
	gboolean is_gain_auto_available;

	if (!ARV_IS_CAMERA (viewer->camera))
		return FALSE;

	stop_video (viewer);

	viewer->rotation = 0;
	viewer->stream = arv_camera_create_stream (viewer->camera, stream_cb, NULL);
	if (viewer->stream == NULL) {
		g_object_unref (viewer->camera);
		viewer->camera = NULL;
		return FALSE;
	}

	if (ARV_IS_GV_STREAM (viewer->stream)) {
		if (viewer->auto_socket_buffer)
			g_object_set (viewer->stream,
				      "socket-buffer", ARV_GV_STREAM_SOCKET_BUFFER_AUTO,
				      "socket-buffer-size", 0,
				      NULL);
		if (!viewer->packet_resend)
			g_object_set (viewer->stream,
				      "packet-resend", ARV_GV_STREAM_PACKET_RESEND_NEVER,
				      NULL);
		g_object_set (viewer->stream,
			      "packet-timeout", (unsigned) viewer->packet_timeout * 1000,
			      "frame-retention", (unsigned) viewer->frame_retention * 1000,
			      NULL);
	}

	viewer->timestamp_offset = 0;
	viewer->last_timestamp = 0;

	arv_stream_set_emit_signals (viewer->stream, TRUE);
	payload = arv_camera_get_payload (viewer->camera);
	for (i = 0; i < 50; i++)
		arv_stream_push_buffer (viewer->stream, arv_buffer_new (payload, NULL));

	arv_camera_get_region (viewer->camera, NULL, NULL, &width, &height);
	pixel_format = arv_camera_get_pixel_format (viewer->camera);
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

	update_gain_ui (viewer, auto_gain);
	update_exposure_ui (viewer, auto_exposure);

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
		stop_video (viewer);
		return FALSE;
	}

	arv_camera_start_acquisition (viewer->camera);

	viewer->pipeline = gst_pipeline_new ("pipeline");

	viewer->appsrc = gst_element_factory_make ("appsrc", NULL);
	videoconvert = gst_element_factory_make ("videoconvert", NULL);
	viewer->transform = gst_element_factory_make ("videoflip", NULL);

	gst_bin_add_many (GST_BIN (viewer->pipeline), viewer->appsrc, videoconvert, viewer->transform, NULL);

	if (g_str_has_prefix (caps_string, "video/x-bayer")) {
		GstElement *bayer2rgb;

		bayer2rgb = gst_element_factory_make ("bayer2rgb", NULL);
		gst_bin_add (GST_BIN (viewer->pipeline), bayer2rgb);
		gst_element_link_many (viewer->appsrc, bayer2rgb, videoconvert, viewer->transform, NULL);
	} else {
		gst_element_link_many (viewer->appsrc, videoconvert, viewer->transform, NULL);
	}

	if (has_gtksink || has_gtkglsink) {
		GtkWidget *video_widget;

		videosink = gst_element_factory_make ("gtkglsink", NULL);

		if (GST_IS_ELEMENT (videosink)) {
			GstElement *glupload;

			glupload = gst_element_factory_make ("glupload", NULL);
			gst_bin_add_many (GST_BIN (viewer->pipeline), glupload, videosink, NULL);
			gst_element_link_many (viewer->transform, glupload, videosink, NULL);
		} else {
			videosink = gst_element_factory_make ("gtksink", NULL);
			gst_element_link_many (viewer->transform, videosink, NULL);
		}

		g_object_get (videosink, "widget", &video_widget, NULL);
		gtk_container_add (GTK_CONTAINER (viewer->video_frame), video_widget);
		gtk_widget_show (video_widget);
		g_object_set(G_OBJECT (video_widget), "force-aspect-ratio", TRUE, NULL);
		gtk_widget_set_size_request (video_widget, 640, 480);
	} else {
		videosink = gst_element_factory_make ("autovideosink", NULL);
		gst_bin_add (GST_BIN (viewer->pipeline), videosink);
		gst_element_link_many (viewer->transform, videosink, NULL);
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

	if (!has_gtkglsink && !has_gtksink) {
		GstBus *bus;

		bus = gst_pipeline_get_bus (GST_PIPELINE (viewer->pipeline));
		gst_bus_set_sync_handler (bus, (GstBusSyncHandler) bus_sync_handler, viewer, NULL);
		gst_object_unref (bus);
	}

	gst_element_set_state (viewer->pipeline, GST_STATE_PLAYING);

	g_signal_connect (viewer->stream, "new-buffer", G_CALLBACK (new_buffer_cb), viewer);

	return TRUE;
}

static gboolean
select_camera_list_mode (gpointer user_data)
{
	ArvViewer *viewer = user_data;

	select_mode (viewer, ARV_VIEWER_MODE_CAMERA_LIST);
	update_device_list_cb (GTK_TOOL_BUTTON (viewer->refresh_button), viewer);

	return FALSE;
}

static void
control_lost_cb (ArvCamera *camera, ArvViewer *viewer)
{
	g_main_context_invoke (NULL, select_camera_list_mode, viewer);
}

static void
stop_camera (ArvViewer *viewer)
{
	gtk_widget_set_sensitive (viewer->camera_parameters, FALSE);
	gtk_widget_set_sensitive (viewer->video_mode_button, FALSE);
	stop_video (viewer);
	g_clear_object (&viewer->camera);
	g_clear_pointer (&viewer->camera_name, g_free);
}

static gboolean
start_camera (ArvViewer *viewer, const char *camera_id)
{
	GtkTreeIter iter;
	GtkListStore *list_store;
	gint64 *pixel_formats;
	const char *pixel_format_string;
	const char **pixel_format_strings;
	guint i, n_pixel_formats, n_pixel_format_strings, n_valid_formats;
	gboolean binning_available;

	stop_camera (viewer);

	viewer->camera = arv_camera_new (camera_id);

	if (!ARV_IS_CAMERA (viewer->camera))
		return FALSE;

	viewer->camera_name = g_strdup (camera_id);

	gtk_widget_set_sensitive (viewer->camera_parameters, TRUE);

	arv_camera_set_chunk_mode (viewer->camera, FALSE);

	update_camera_region (viewer);

	g_signal_handler_block (viewer->pixel_format_combo, viewer->pixel_format_changed);

	list_store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (viewer->pixel_format_combo)));
	gtk_list_store_clear (list_store);
	n_valid_formats = 0;
	pixel_format_strings = arv_camera_get_available_pixel_formats_as_strings (viewer->camera, &n_pixel_format_strings);
	pixel_formats = arv_camera_get_available_pixel_formats (viewer->camera, &n_pixel_formats);
	g_assert (n_pixel_formats == n_pixel_format_strings);
	pixel_format_string = arv_camera_get_pixel_format_as_string (viewer->camera);
	for (i = 0; i < n_pixel_formats; i++) {
		if (arv_pixel_format_to_gst_caps_string (pixel_formats[i]) != NULL) {
			gtk_list_store_append (list_store, &iter);
			gtk_list_store_set (list_store, &iter, 0, pixel_format_strings[i], -1);
			if (g_strcmp0 (pixel_format_strings[i], pixel_format_string) == 0)
				gtk_combo_box_set_active (GTK_COMBO_BOX (viewer->pixel_format_combo), n_valid_formats);
			n_valid_formats++;
		}
	}
	g_free (pixel_formats);
	g_free (pixel_format_strings);
	gtk_widget_set_sensitive (viewer->pixel_format_combo, n_valid_formats > 1);
	gtk_widget_set_sensitive (viewer->video_mode_button, TRUE);

	binning_available = arv_camera_is_binning_available (viewer->camera);
	gtk_widget_set_sensitive (viewer->camera_binning_x, binning_available);
	gtk_widget_set_sensitive (viewer->camera_binning_y, binning_available);

	g_signal_handler_unblock (viewer->pixel_format_combo, viewer->pixel_format_changed);

	g_signal_connect (arv_camera_get_device (viewer->camera), "control-lost", G_CALLBACK (control_lost_cb), viewer);

	return TRUE;
}

static void
camera_selection_changed_cb (GtkTreeSelection *selection, ArvViewer *viewer)
{
	GtkTreeIter iter;
	GtkTreeModel *tree_model;
	char *camera_id = NULL;

	if (gtk_tree_selection_get_selected (selection, &tree_model, &iter)) {
		gtk_tree_model_get (tree_model, &iter, 0, &camera_id, -1);
		start_camera (viewer, camera_id);
		g_free (camera_id);
	} else
		stop_camera (viewer);
}

static void
select_mode (ArvViewer *viewer, ArvViewerMode mode)
{
	gboolean video_visibility;
	char *subtitle;
	gint width, height, x, y;

	if (!ARV_IS_CAMERA (viewer->camera))
		mode = ARV_VIEWER_MODE_CAMERA_LIST;

	switch (mode) {
		case ARV_VIEWER_MODE_CAMERA_LIST:
			video_visibility = FALSE;
			gtk_stack_set_visible_child (GTK_STACK (viewer->main_stack), viewer->camera_box);
			gtk_header_bar_set_title (GTK_HEADER_BAR (viewer->main_headerbar), "Aravis Viewer");
			gtk_header_bar_set_subtitle (GTK_HEADER_BAR (viewer->main_headerbar), NULL);
			stop_video (viewer);
			break;
		case ARV_VIEWER_MODE_VIDEO:
			video_visibility = TRUE;
			arv_camera_get_region (viewer->camera, &x, &y, &width, &height);
			subtitle = g_strdup_printf ("%s %dx%d@%d,%d %s",
						    arv_camera_get_model_name (viewer->camera),
						    width, height,
						    x, y,
						    arv_camera_get_pixel_format_as_string (viewer->camera));
			gtk_stack_set_visible_child (GTK_STACK (viewer->main_stack), viewer->video_frame);
			gtk_header_bar_set_title (GTK_HEADER_BAR (viewer->main_headerbar), viewer->camera_name);
			gtk_header_bar_set_subtitle (GTK_HEADER_BAR (viewer->main_headerbar), subtitle);
			g_free (subtitle);
			start_video (viewer);
			break;
		default:
			g_assert_not_reached ();
			break;
	}

	gtk_widget_set_visible (viewer->back_button, video_visibility);
	gtk_widget_set_visible (viewer->rotate_cw_button, video_visibility);
	gtk_widget_set_visible (viewer->flip_vertical_toggle, video_visibility);
	gtk_widget_set_visible (viewer->flip_horizontal_toggle, video_visibility);
	gtk_widget_set_visible (viewer->snapshot_button, video_visibility);
	gtk_widget_set_visible (viewer->acquisition_button, video_visibility);

}

void
switch_to_camera_list_cb (GtkToolButton *button, ArvViewer *viewer)
{
	select_mode (viewer, ARV_VIEWER_MODE_CAMERA_LIST);
}

void
switch_to_video_mode_cb (GtkToolButton *button, ArvViewer *viewer)
{
	select_mode (viewer, ARV_VIEWER_MODE_VIDEO);
}

void
arv_viewer_quit_cb (GtkApplicationWindow *window, ArvViewer *viewer)
{
	g_application_quit (G_APPLICATION (viewer));
}

static void
video_frame_realize_cb (GtkWidget * widget, ArvViewer *viewer)
{
	viewer->video_window_xid = GDK_WINDOW_XID (gtk_widget_get_window (widget));
}

static void
activate (GApplication *application)
{
	ArvViewer *viewer = (ArvViewer *) application;
	GtkBuilder *builder;
	char *ui_filename;
	GError *err = NULL;

	builder = gtk_builder_new ();

	ui_filename = g_build_filename (ARAVIS_DATA_DIR, "arv-viewer.ui", NULL);

	if (!gtk_builder_add_from_file (builder, ui_filename, &err)) {
		g_error ("Cant't load user interface file: %s", err->message);
		g_error_free (err);
	}

	g_free (ui_filename);

	viewer->main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	viewer->main_stack = GTK_WIDGET (gtk_builder_get_object (builder, "main_stack"));
	viewer->main_headerbar = GTK_WIDGET (gtk_builder_get_object (builder, "main_headerbar"));
	viewer->camera_box = GTK_WIDGET (gtk_builder_get_object (builder, "camera_box"));
	viewer->refresh_button = GTK_WIDGET (gtk_builder_get_object (builder, "refresh_button"));
	viewer->video_mode_button = GTK_WIDGET (gtk_builder_get_object (builder, "video_mode_button"));
	viewer->back_button = GTK_WIDGET (gtk_builder_get_object (builder, "back_button"));
	viewer->snapshot_button = GTK_WIDGET (gtk_builder_get_object (builder, "snapshot_button"));
	viewer->camera_tree = GTK_WIDGET (gtk_builder_get_object (builder, "camera_tree"));
	viewer->camera_parameters = GTK_WIDGET (gtk_builder_get_object (builder, "camera_parameters"));
	viewer->pixel_format_combo = GTK_WIDGET (gtk_builder_get_object (builder, "pixel_format_combo"));
	viewer->camera_x = GTK_WIDGET (gtk_builder_get_object (builder, "camera_x"));
	viewer->camera_y = GTK_WIDGET (gtk_builder_get_object (builder, "camera_y"));
	viewer->camera_binning_x = GTK_WIDGET (gtk_builder_get_object (builder, "camera_binning_x"));
	viewer->camera_binning_y = GTK_WIDGET (gtk_builder_get_object (builder, "camera_binning_y"));
	viewer->camera_width = GTK_WIDGET (gtk_builder_get_object (builder, "camera_width"));
	viewer->camera_height = GTK_WIDGET (gtk_builder_get_object (builder, "camera_height"));
	viewer->video_frame = GTK_WIDGET (gtk_builder_get_object (builder, "video_frame"));
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
	viewer->acquisition_button = GTK_WIDGET (gtk_builder_get_object (builder, "acquisition_button"));

	g_object_unref (builder);

	gtk_widget_set_no_show_all (viewer->trigger_combo_box, TRUE);

	gtk_widget_show_all (viewer->main_window);

	gtk_application_add_window (GTK_APPLICATION (application), GTK_WINDOW (viewer->main_window));

	g_signal_connect (viewer->refresh_button, "clicked", G_CALLBACK (update_device_list_cb), viewer);
	g_signal_connect (viewer->video_mode_button, "clicked", G_CALLBACK (switch_to_video_mode_cb), viewer);
	g_signal_connect (viewer->back_button, "clicked", G_CALLBACK (switch_to_camera_list_cb), viewer);
	g_signal_connect (viewer->main_window, "destroy", G_CALLBACK (arv_viewer_quit_cb), viewer);
	g_signal_connect (viewer->snapshot_button, "clicked", G_CALLBACK (snapshot_cb), viewer);
	g_signal_connect (viewer->rotate_cw_button, "clicked", G_CALLBACK (rotate_cw_cb), viewer);
	g_signal_connect (viewer->flip_horizontal_toggle, "clicked", G_CALLBACK (flip_horizontal_cb), viewer);
	g_signal_connect (viewer->flip_vertical_toggle, "clicked", G_CALLBACK (flip_vertical_cb), viewer);
	g_signal_connect (viewer->frame_rate_entry, "activate", G_CALLBACK (frame_rate_entry_cb), viewer);
	g_signal_connect (viewer->frame_rate_entry, "focus-out-event", G_CALLBACK (frame_rate_entry_focus_cb), viewer);

	if (!has_gtksink && !has_gtkglsink) {
		g_signal_connect (viewer->video_frame, "realize", G_CALLBACK (video_frame_realize_cb), viewer);
	}

	viewer->camera_selected = g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (viewer->camera_tree)), "changed",
						    G_CALLBACK (camera_selection_changed_cb), viewer);
	viewer->exposure_spin_changed = g_signal_connect (viewer->exposure_spin_button, "value-changed",
							  G_CALLBACK (exposure_spin_cb), viewer);
	viewer->gain_spin_changed = g_signal_connect (viewer->gain_spin_button, "value-changed",
						      G_CALLBACK (gain_spin_cb), viewer);
	viewer->exposure_hscale_changed = g_signal_connect (viewer->exposure_hscale, "value-changed",
							    G_CALLBACK (exposure_scale_cb), viewer);
	viewer->gain_hscale_changed = g_signal_connect (viewer->gain_hscale, "value-changed",
							G_CALLBACK (gain_scale_cb), viewer);
	viewer->auto_exposure_clicked = g_signal_connect (viewer->auto_exposure_toggle, "clicked",
							  G_CALLBACK (auto_exposure_cb), viewer);
	viewer->auto_gain_clicked = g_signal_connect (viewer->auto_gain_toggle, "clicked",
						      G_CALLBACK (auto_gain_cb), viewer);
	viewer->pixel_format_changed = g_signal_connect (viewer->pixel_format_combo, "changed",
							 G_CALLBACK (pixel_format_combo_cb), viewer);
	viewer->camera_x_changed = g_signal_connect (viewer->camera_x, "value-changed",
						     G_CALLBACK (camera_region_cb), viewer);
	viewer->camera_y_changed = g_signal_connect (viewer->camera_y, "value-changed",
						     G_CALLBACK (camera_region_cb), viewer);
	viewer->camera_width_changed = g_signal_connect (viewer->camera_width, "value-changed",
							 G_CALLBACK (camera_region_cb), viewer);
	viewer->camera_height_changed = g_signal_connect (viewer->camera_height, "value-changed",
							  G_CALLBACK (camera_region_cb), viewer);
	viewer->camera_binning_x_changed = g_signal_connect (viewer->camera_binning_x, "value-changed",
							     G_CALLBACK (camera_binning_cb), viewer);
	viewer->camera_binning_y_changed = g_signal_connect (viewer->camera_binning_y, "value-changed",
							     G_CALLBACK (camera_binning_cb), viewer);

	gtk_widget_set_sensitive (viewer->camera_parameters, FALSE);
	select_mode (viewer, ARV_VIEWER_MODE_CAMERA_LIST);
	update_device_list_cb (GTK_TOOL_BUTTON (viewer->refresh_button), viewer);
}

static void
startup (GApplication *application)
{
	arv_enable_interface ("Fake");

	G_APPLICATION_CLASS (arv_viewer_parent_class)->startup (application);
}

static void
shutdown (GApplication *application)
{
	G_APPLICATION_CLASS (arv_viewer_parent_class)->shutdown (application);

	arv_shutdown ();
}

static void
finalize (GObject *object)
{
	ArvViewer *viewer = (ArvViewer *) object;

	G_OBJECT_CLASS (arv_viewer_parent_class)->finalize (object);

	g_clear_object (&viewer->notification);
}

ArvViewer *
arv_viewer_new (void)
{
  ArvViewer *arv_viewer;

  if (!gstreamer_plugin_check ())
	  return NULL;

  g_set_application_name ("ArvViewer");

  arv_viewer = g_object_new (arv_viewer_get_type (),
			     "application-id", "org.aravis.ArvViewer",
			     "flags", G_APPLICATION_NON_UNIQUE,
			     "inactivity-timeout", 30000,
			     NULL);

  return arv_viewer;
}


static void
arv_viewer_init (ArvViewer *viewer)
{
	viewer->notification = notify_notification_new (NULL, NULL, NULL);
	viewer->auto_socket_buffer = FALSE;
	viewer->packet_resend = TRUE;
	viewer->packet_timeout = 20;
	viewer->frame_retention = 100;
}

static void
arv_viewer_class_init (ArvViewerClass *class)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = finalize;

  application_class->startup = startup;
  application_class->shutdown = shutdown;
  application_class->activate = activate;
}
