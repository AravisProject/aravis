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
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#include "libxml/parser.h"
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/videooverlay.h>
#include <gst/video/video.h>
#include <arv.h>
#include <arvdebugprivate.h>
#include <arvviewer.h>
#include <math.h>
#include <memory.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>  // for GDK_WINDOW_XID
#endif
#ifdef GDK_WINDOWING_WIN32
#include <gdk/gdkwin32.h>  // for GDK_WINDOW_HWND
#endif

#define ARV_VIEWER_NOTIFICATION_TIMEOUT 10
#define ARV_VIEWER_N_BUFFERS 10

static gboolean has_autovideo_sink = FALSE;
static gboolean has_gtksink = FALSE;
static gboolean has_gtkglsink = FALSE;
static gboolean has_bayer2rgb = FALSE;

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
			"videoflip"
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

		feature = gst_registry_lookup_feature (registry, "bayer2rgb");
		if (GST_IS_PLUGIN_FEATURE (feature)) {
			has_bayer2rgb = TRUE;
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

struct  _ArvViewer {
	GtkApplication parent_instance;

	ArvCamera *camera;
	char *camera_name;
	ArvStream *stream;
	ArvBuffer *last_buffer;
        guint component_id;

	GstElement *pipeline;
	GstElement *appsrc;
	GstElement *transform;
        GstElement *videosink;

	guint rotation;
	gboolean flip_vertical;
	gboolean flip_horizontal;

	double gain_min, gain_max;
	double exposure_min, exposure_max;

	ArvGcRepresentation gain_representation;
	ArvGcRepresentation exposure_time_representation;

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
	GtkWidget *component_label;
	GtkWidget *component_combo;
	GtkWidget *component_check;
	GtkWidget *pixel_format_combo;
	GtkWidget *camera_x;
	GtkWidget *camera_y;
        GtkWidget *camera_position_label;
        GtkWidget *camera_position_units;
	GtkWidget *camera_binning_x;
	GtkWidget *camera_binning_y;
        GtkWidget *camera_binning_label;
        GtkWidget *camera_binning_units;
	GtkWidget *camera_width;
	GtkWidget *camera_height;
	GtkWidget *video_box;
	GtkWidget *video_frame;
	GtkWidget *fps_label;
	GtkWidget *image_label;
	GtkWidget *trigger_combo_box;
	GtkWidget *frame_rate_entry;
	GtkWidget *exposure_spin_button;
	GtkWidget *exposure_hscale;
	GtkWidget *auto_exposure_toggle;
	GtkWidget *gain_spin_button;
	GtkWidget *gain_hscale;
	GtkWidget *auto_gain_toggle;
	GtkWidget *black_level_spin_button;
	GtkWidget *black_level_hscale;
        GtkWidget *auto_black_level_toggle;
	GtkWidget *acquisition_button;

        GtkWidget *notification_revealer;
        GtkWidget *notification_label;
        GtkWidget *notification_details;
        GtkWidget *notification_dismiss;

        guint notification_timeout;

	gulong camera_selected;
	gulong exposure_spin_changed;
	gulong exposure_hscale_changed;
	gulong auto_exposure_clicked;
	gulong gain_spin_changed;
	gulong gain_hscale_changed;
	gulong auto_gain_clicked;
	gulong black_level_spin_changed;
	gulong black_level_hscale_changed;
	gulong auto_black_level_clicked;
	gulong camera_x_changed;
	gulong camera_y_changed;
	gulong camera_binning_x_changed;
	gulong camera_binning_y_changed;
	gulong camera_width_changed;
	gulong camera_height_changed;
	gulong component_changed;
	gulong component_toggled;
	gulong pixel_format_changed;
        gulong rotate_cw_clicked;
        gulong flip_vertical_clicked;
        gulong flip_horizontal_clicked;

	guint gain_update_event;
	guint black_level_update_event;
	guint exposure_update_event;

	guint status_bar_update_event;
	gint64 last_status_bar_update_time_ms;
	guint64 last_n_images;
	guint64 last_n_bytes;

	gboolean auto_socket_buffer;
	gboolean packet_resend;
	guint initial_packet_timeout;
	guint packet_timeout;
	guint frame_retention;
	ArvRegisterCachePolicy register_cache_policy;
	ArvRangeCheckPolicy range_check_policy;
        ArvUvUsbMode usb_mode;

	gulong video_window_xid;
};

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
			guint initial_packet_timeout,
			guint packet_timeout,
			guint frame_retention,
			ArvRegisterCachePolicy register_cache_policy,
			ArvRangeCheckPolicy range_check_policy,
                        ArvUvUsbMode usb_mode)
{
	g_return_if_fail (viewer != NULL);

	viewer->auto_socket_buffer = auto_socket_buffer;
	viewer->packet_resend = packet_resend;
	viewer->initial_packet_timeout = initial_packet_timeout;
	viewer->packet_timeout = packet_timeout;
	viewer->frame_retention = frame_retention;
	viewer->register_cache_policy = register_cache_policy;
	viewer->range_check_policy = range_check_policy;
        viewer->usb_mode = usb_mode;
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

static void
notification_dismiss_clicked_cb (GtkButton *dismiss, ArvViewer *viewer)
{
        if (viewer->notification_timeout > 0)
                g_source_remove (viewer->notification_timeout);

        gtk_revealer_set_reveal_child (GTK_REVEALER (viewer->notification_revealer), FALSE);

        viewer->notification_timeout = 0;
}

static gboolean
hide_notification (gpointer user_data)
{
        ArvViewer *viewer = user_data;

        gtk_revealer_set_reveal_child (GTK_REVEALER (viewer->notification_revealer), FALSE);
        viewer->notification_timeout = 0;

        return G_SOURCE_REMOVE;
}

static void
arv_viewer_show_notification (ArvViewer *viewer, const char *message, const char *details)
{
        g_return_if_fail (ARV_IS_VIEWER (viewer));
        g_return_if_fail (message != NULL);

        if (viewer->notification_timeout > 0)
                g_source_remove (viewer->notification_timeout);

        gtk_revealer_set_reveal_child (GTK_REVEALER (viewer->notification_revealer), FALSE);
        gtk_label_set_text (GTK_LABEL (viewer->notification_label), message);
        if (details != NULL) {
                g_autofree char *text = g_strdup_printf ("<small>%s</small>", details);
                gtk_widget_show (viewer->notification_details);
                gtk_label_set_markup (GTK_LABEL (viewer->notification_details), text);
        } else {
                gtk_widget_hide (viewer->notification_details);
        }
        gtk_revealer_set_reveal_child (GTK_REVEALER (viewer->notification_revealer), TRUE);

        viewer->notification_timeout = g_timeout_add_seconds (ARV_VIEWER_NOTIFICATION_TIMEOUT, hide_notification, viewer);
}

typedef struct {
	GWeakRef stream;
	ArvBuffer* arv_buffer;
	void *data;
} ArvGstBufferReleaseData;

static void
gst_buffer_release_cb (void *user_data)
{
	ArvGstBufferReleaseData* release_data = user_data;

	ArvStream* stream = g_weak_ref_get (&release_data->stream);

	g_free (release_data->data);

	if (stream) {
		gint n_input_buffers, n_output_buffers;

		arv_stream_get_n_buffers (stream, &n_input_buffers, &n_output_buffers);
		arv_debug_viewer ("push buffer (%d,%d)", n_input_buffers, n_output_buffers);

		arv_stream_push_buffer (stream, release_data->arv_buffer);
		g_object_unref (stream);
	} else {
		arv_info_viewer ("invalid stream object");
		g_object_unref (release_data->arv_buffer);
	}

	g_weak_ref_clear (&release_data->stream);
	g_free (release_data);
}

static GstBuffer *
arv_to_gst_buffer (ArvBuffer *arv_buffer, guint part_id, ArvStream *stream)
{
	ArvGstBufferReleaseData* release_data;
	int arv_row_stride;
	int width, height;
	char *buffer_data;
	size_t buffer_size;
	size_t size;
	void *data;

	buffer_data = (char *) arv_buffer_get_part_data (arv_buffer, part_id, &buffer_size);
	arv_buffer_get_part_region (arv_buffer, part_id, NULL, NULL, &width, &height);
	arv_row_stride = width * ARV_PIXEL_FORMAT_BIT_PER_PIXEL (arv_buffer_get_part_pixel_format (arv_buffer, part_id)) / 8;

	release_data = g_new0 (ArvGstBufferReleaseData, 1);

	g_weak_ref_init (&release_data->stream, stream);
	release_data->arv_buffer = arv_buffer;

	/* Gstreamer requires row stride to be a multiple of 4 */
	if ((arv_row_stride & 0x3) != 0) {
		int gst_row_stride;
		int i;

		gst_row_stride = (arv_row_stride & ~(0x3)) + 4;

		size = height * gst_row_stride;
		data = g_malloc (size);

		for (i = 0; i < height; i++)
			memcpy (((char *) data) + i * gst_row_stride, buffer_data + i * arv_row_stride, arv_row_stride);

		release_data->data = data;

	} else {
		data = buffer_data;
		size = buffer_size;
	}

	return gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
					    data, size, 0, size,
					    release_data, gst_buffer_release_cb);
}

static void
new_buffer_cb (ArvStream *stream, ArvViewer *viewer)
{
	ArvBuffer *arv_buffer;
	gint n_input_buffers, n_output_buffers;

	arv_buffer = arv_stream_pop_buffer (stream);
	if (arv_buffer == NULL)
		return;

	arv_stream_get_n_buffers (stream, &n_input_buffers, &n_output_buffers);
	arv_debug_viewer ("pop buffer (%d,%d)", n_input_buffers, n_output_buffers);

	if (arv_buffer_get_status (arv_buffer) == ARV_BUFFER_STATUS_SUCCESS &&
            /* Ensure there is still available buffers for the stream thread */
            n_input_buffers + n_output_buffers > 0) {
		size_t size;
                gint part_id;

                part_id = arv_buffer_find_component(arv_buffer, viewer->component_id);
                if (part_id < 0)
                        part_id = 0;

                arv_buffer_get_part_data (arv_buffer, part_id, &size);

		g_clear_object( &viewer->last_buffer );
		viewer->last_buffer = g_object_ref( arv_buffer );

		gst_app_src_push_buffer (GST_APP_SRC (viewer->appsrc), arv_to_gst_buffer (arv_buffer, part_id, stream));
	} else {
		arv_debug_viewer ("push discarded buffer");
		arv_stream_push_buffer (stream, arv_buffer);
	}
}

static void
_apply_frame_rate (GtkEntry *entry, ArvViewer *viewer, gboolean grab_focus)
{
	char *text;
	double frame_rate;

	text = (char *) gtk_entry_get_text (entry);

	arv_camera_set_frame_rate (viewer->camera, g_strtod (text, NULL), NULL);

	frame_rate = arv_camera_get_frame_rate (viewer->camera, NULL);
	text = g_strdup_printf ("%g", frame_rate);
	gtk_entry_set_text (entry, text);
	if (grab_focus)
		gtk_widget_grab_focus (GTK_WIDGET(entry));
	g_free (text);
}

static void
frame_rate_entry_cb (GtkEntry *entry, ArvViewer *viewer)
{
	_apply_frame_rate (entry, viewer, TRUE);
}

static gboolean
frame_rate_entry_focus_cb (GtkEntry *entry, GdkEventFocus *event,
		    ArvViewer *viewer)
{
	_apply_frame_rate (entry, viewer, FALSE);

	return FALSE;
}

static void
exposure_spin_cb (GtkSpinButton *spin_button, ArvViewer *viewer)
{
	double exposure = gtk_spin_button_get_value (spin_button);
	double scaled_exposure = viewer->exposure_time_representation == ARV_GC_REPRESENTATION_LOGARITHMIC ? 
		arv_viewer_value_to_log (exposure, viewer->exposure_min, viewer->exposure_max) : exposure;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_exposure_toggle), FALSE);

	arv_camera_set_exposure_time (viewer->camera, exposure, NULL);

	g_signal_handler_block (viewer->exposure_hscale, viewer->exposure_hscale_changed);
	gtk_range_set_value (GTK_RANGE (viewer->exposure_hscale), scaled_exposure);
	gtk_widget_grab_focus (GTK_WIDGET (spin_button));
	g_signal_handler_unblock (viewer->exposure_hscale, viewer->exposure_hscale_changed);
}

static void
gain_spin_cb (GtkSpinButton *spin_button, ArvViewer *viewer)
{
	double gain = gtk_spin_button_get_value (spin_button);
	double scaled_gain = viewer->gain_representation == ARV_GC_REPRESENTATION_LOGARITHMIC ? 
		arv_viewer_value_to_log (gain, viewer->gain_min, viewer->gain_max) : gain;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_gain_toggle), FALSE);

	arv_camera_set_gain (viewer->camera, gtk_spin_button_get_value (spin_button), NULL);

	g_signal_handler_block (viewer->gain_hscale, viewer->gain_hscale_changed);
	gtk_range_set_value (GTK_RANGE (viewer->gain_hscale), scaled_gain);
	gtk_widget_grab_focus (GTK_WIDGET (spin_button));
	g_signal_handler_unblock (viewer->gain_hscale, viewer->gain_hscale_changed);
}

static void
black_level_spin_cb (GtkSpinButton *spin_button, ArvViewer *viewer)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_black_level_toggle), FALSE);

	arv_camera_set_black_level (viewer->camera, gtk_spin_button_get_value (spin_button), NULL);

	g_signal_handler_block (viewer->black_level_hscale, viewer->black_level_hscale_changed);
	gtk_range_set_value (GTK_RANGE (viewer->black_level_hscale), gtk_spin_button_get_value (spin_button));
	gtk_widget_grab_focus (GTK_WIDGET (spin_button));
	g_signal_handler_unblock (viewer->black_level_hscale, viewer->black_level_hscale_changed);
}

static void
exposure_scale_cb (GtkRange *range, ArvViewer *viewer)
{
	double scaled_exposure = gtk_range_get_value (range);
	double exposure = viewer->exposure_time_representation == ARV_GC_REPRESENTATION_LOGARITHMIC ?
		arv_viewer_value_from_log (scaled_exposure, viewer->exposure_min, viewer->exposure_max) : scaled_exposure;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_exposure_toggle), FALSE);

	arv_camera_set_exposure_time (viewer->camera, exposure, NULL);

	g_signal_handler_block (viewer->exposure_spin_button, viewer->exposure_spin_changed);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->exposure_spin_button), exposure);
	g_signal_handler_unblock (viewer->exposure_spin_button, viewer->exposure_spin_changed);
}

static void
gain_scale_cb (GtkRange *range, ArvViewer *viewer)
{
	double scaled_gain = gtk_range_get_value (range);
	double gain = viewer->gain_representation == ARV_GC_REPRESENTATION_LOGARITHMIC ?
		arv_viewer_value_from_log (scaled_gain, viewer->gain_min, viewer->gain_max) : scaled_gain;
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_gain_toggle), FALSE);

	arv_camera_set_gain (viewer->camera, gain, NULL);

	g_signal_handler_block (viewer->gain_spin_button, viewer->gain_spin_changed);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->gain_spin_button), gain);
	g_signal_handler_unblock (viewer->gain_spin_button, viewer->gain_spin_changed);
}

static void
black_level_scale_cb (GtkRange *range, ArvViewer *viewer)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_black_level_toggle), FALSE);

	arv_camera_set_black_level (viewer->camera, gtk_range_get_value (range), NULL);

	g_signal_handler_block (viewer->black_level_spin_button, viewer->black_level_spin_changed);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->black_level_spin_button), gtk_range_get_value (range));
	g_signal_handler_unblock (viewer->black_level_spin_button, viewer->black_level_spin_changed);
}

static gboolean
update_exposure_cb (void *data)
{
	ArvViewer *viewer = data;
	double exposure;
	double scaled_exposure;

	exposure = arv_camera_get_exposure_time (viewer->camera, NULL);
	scaled_exposure = viewer->exposure_time_representation == ARV_GC_REPRESENTATION_LOGARITHMIC ? 
		arv_viewer_value_to_log (exposure, viewer->exposure_min, viewer->exposure_max) : exposure;

	g_signal_handler_block (viewer->exposure_hscale, viewer->exposure_hscale_changed);
	g_signal_handler_block (viewer->exposure_spin_button, viewer->exposure_spin_changed);
	gtk_range_set_value (GTK_RANGE (viewer->exposure_hscale), scaled_exposure);
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

	arv_camera_set_exposure_time_auto (viewer->camera, is_auto ? ARV_AUTO_CONTINUOUS : ARV_AUTO_OFF, NULL);
	update_exposure_ui (viewer, is_auto);
}

static gboolean
update_gain_cb (void *data)
{
	ArvViewer *viewer = data;
	double gain;
	double scaled_gain;

	gain = arv_camera_get_gain (viewer->camera, NULL);
	scaled_gain = viewer->gain_representation == ARV_GC_REPRESENTATION_LOGARITHMIC ? 
		arv_viewer_value_to_log (gain, viewer->gain_min, viewer->gain_max) : gain;

	g_signal_handler_block (viewer->gain_hscale, viewer->gain_hscale_changed);
	g_signal_handler_block (viewer->gain_spin_button, viewer->gain_spin_changed);
	gtk_range_set_value (GTK_RANGE (viewer->gain_hscale), scaled_gain);
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


static void
auto_gain_cb (GtkToggleButton *toggle, ArvViewer *viewer)
{
	gboolean is_auto;

	is_auto = gtk_toggle_button_get_active (toggle);

	arv_camera_set_gain_auto (viewer->camera, is_auto ? ARV_AUTO_CONTINUOUS : ARV_AUTO_OFF, NULL);
	update_gain_ui (viewer, is_auto);
}

static gboolean
update_black_level_cb (void *data)
{
	ArvViewer *viewer = data;
	double black_level;

	black_level = arv_camera_get_black_level (viewer->camera, NULL);

	g_signal_handler_block (viewer->black_level_hscale, viewer->black_level_hscale_changed);
	g_signal_handler_block (viewer->black_level_spin_button, viewer->black_level_spin_changed);
	gtk_range_set_value (GTK_RANGE (viewer->black_level_hscale), black_level);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->black_level_spin_button), black_level);
	g_signal_handler_unblock (viewer->black_level_spin_button, viewer->black_level_spin_changed);
	g_signal_handler_unblock (viewer->black_level_hscale, viewer->black_level_hscale_changed);

	return TRUE;
}

static void
update_black_level_ui (ArvViewer *viewer, gboolean is_auto)
{
	update_black_level_cb (viewer);

	if (viewer->black_level_update_event > 0) {
		g_source_remove (viewer->black_level_update_event);
		viewer->black_level_update_event = 0;
	}

	if (is_auto)
		viewer->black_level_update_event = g_timeout_add_seconds (1, update_black_level_cb, viewer);

}


static void
auto_black_level_cb (GtkToggleButton *toggle, ArvViewer *viewer)
{
	gboolean is_auto;

	is_auto = gtk_toggle_button_get_active (toggle);

	arv_camera_set_black_level_auto (viewer->camera, is_auto ? ARV_AUTO_CONTINUOUS : ARV_AUTO_OFF, NULL);
	update_black_level_ui (viewer, is_auto);
}

static void
set_camera_widgets(ArvViewer *viewer)
{
	g_autofree char *string = NULL;
	gboolean is_frame_rate_available;
	gboolean is_gain_available;
	gboolean auto_gain;
	double black_level_min, black_level_max;
	gboolean is_black_level_available;
	gboolean auto_black_level;
	gboolean is_exposure_available;
	gboolean auto_exposure;

	g_signal_handler_block (viewer->gain_hscale, viewer->gain_hscale_changed);
	g_signal_handler_block (viewer->gain_spin_button, viewer->gain_spin_changed);
	g_signal_handler_block (viewer->black_level_hscale, viewer->black_level_hscale_changed);
	g_signal_handler_block (viewer->black_level_spin_button, viewer->black_level_spin_changed);
	g_signal_handler_block (viewer->exposure_hscale, viewer->exposure_hscale_changed);
	g_signal_handler_block (viewer->exposure_spin_button, viewer->exposure_spin_changed);

	arv_camera_get_exposure_time_bounds (viewer->camera, &viewer->exposure_min, &viewer->exposure_max, NULL);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->exposure_spin_button),
				   viewer->exposure_min, viewer->exposure_max);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->exposure_spin_button), 200.0, 1000.0);

	arv_camera_get_gain_bounds (viewer->camera, &viewer->gain_min, &viewer->gain_max, NULL);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->gain_spin_button), viewer->gain_min, viewer->gain_max);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->gain_spin_button), 1, 10);

	arv_camera_get_black_level_bounds (viewer->camera, &black_level_min, &black_level_max, NULL);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->black_level_spin_button), black_level_min, black_level_max);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->black_level_spin_button), 1, 10);

	gtk_range_set_range (GTK_RANGE (viewer->black_level_hscale), black_level_min, black_level_max);

	is_frame_rate_available = arv_camera_is_frame_rate_available (viewer->camera, NULL);
	gtk_widget_set_sensitive (viewer->frame_rate_entry, is_frame_rate_available);

	string = g_strdup_printf ("%g", arv_camera_get_frame_rate (viewer->camera, NULL));
	gtk_entry_set_text (GTK_ENTRY (viewer->frame_rate_entry), string);

	is_gain_available = arv_camera_is_gain_available (viewer->camera, NULL);
	if (is_gain_available){
		viewer->gain_representation = arv_camera_get_gain_representation(viewer->camera);
		switch(viewer->gain_representation){
			case ARV_GC_REPRESENTATION_UNDEFINED:
			case ARV_GC_REPRESENTATION_LINEAR:
				gtk_range_set_range (GTK_RANGE (viewer->exposure_hscale), viewer->exposure_min, viewer->exposure_max);
				gtk_widget_set_sensitive (viewer->gain_hscale, is_gain_available);
				gtk_widget_set_sensitive (viewer->gain_spin_button, is_gain_available);
				break;
			case ARV_GC_REPRESENTATION_LOGARITHMIC:
				gtk_range_set_range (GTK_RANGE (viewer->exposure_hscale), 0.0, 1.0);
				gtk_widget_set_sensitive (viewer->gain_hscale, is_gain_available);
				gtk_widget_set_sensitive (viewer->gain_spin_button, is_gain_available);
				break;
			case ARV_GC_REPRESENTATION_PURE_NUMBER:
				gtk_widget_set_sensitive (viewer->gain_hscale, FALSE);
				gtk_widget_set_sensitive (viewer->gain_spin_button, is_gain_available);
				break;
			default:
				gtk_widget_set_sensitive (viewer->gain_hscale, FALSE);
				gtk_widget_set_sensitive (viewer->gain_spin_button, FALSE);
		}
	}else{
		gtk_widget_set_sensitive (viewer->gain_hscale, FALSE);
		gtk_widget_set_sensitive (viewer->gain_spin_button, FALSE);
	}

	is_black_level_available = arv_camera_is_black_level_available (viewer->camera, NULL);
	gtk_widget_set_sensitive (viewer->black_level_hscale, is_black_level_available);
	gtk_widget_set_sensitive (viewer->black_level_spin_button, is_black_level_available);

	is_exposure_available = arv_camera_is_exposure_time_available (viewer->camera, NULL);
	if (is_exposure_available){
		viewer->exposure_time_representation = arv_camera_get_exposure_time_representation(viewer->camera);
		switch(viewer->exposure_time_representation){
			case ARV_GC_REPRESENTATION_UNDEFINED:
			case ARV_GC_REPRESENTATION_LINEAR:
				gtk_range_set_range (GTK_RANGE (viewer->gain_hscale), viewer->gain_min, viewer->gain_max);
				gtk_widget_set_sensitive (viewer->exposure_hscale, is_gain_available);
				gtk_widget_set_sensitive (viewer->exposure_spin_button, is_gain_available);
				break;
			case ARV_GC_REPRESENTATION_LOGARITHMIC:
				gtk_range_set_range (GTK_RANGE (viewer->gain_hscale), 0.0, 1.0);
				gtk_widget_set_sensitive (viewer->exposure_hscale, is_gain_available);
				gtk_widget_set_sensitive (viewer->exposure_spin_button, is_gain_available);
				break;
			case ARV_GC_REPRESENTATION_PURE_NUMBER:
				gtk_widget_set_sensitive (viewer->exposure_hscale, FALSE);
				gtk_widget_set_sensitive (viewer->exposure_spin_button, is_gain_available);
				break;
			default:
				gtk_widget_set_sensitive (viewer->exposure_hscale, FALSE);
				gtk_widget_set_sensitive (viewer->exposure_spin_button, FALSE);
		}
	}else{
		gtk_widget_set_sensitive (viewer->exposure_hscale, FALSE);
		gtk_widget_set_sensitive (viewer->exposure_spin_button, FALSE);
	}

	g_signal_handler_unblock (viewer->gain_hscale, viewer->gain_hscale_changed);
	g_signal_handler_unblock (viewer->gain_spin_button, viewer->gain_spin_changed);
	g_signal_handler_unblock (viewer->black_level_hscale, viewer->black_level_hscale_changed);
	g_signal_handler_unblock (viewer->black_level_spin_button, viewer->black_level_spin_changed);
	g_signal_handler_unblock (viewer->exposure_hscale, viewer->exposure_hscale_changed);
	g_signal_handler_unblock (viewer->exposure_spin_button, viewer->exposure_spin_changed);

	auto_gain = arv_camera_get_gain_auto (viewer->camera, NULL) != ARV_AUTO_OFF;
	auto_black_level = arv_camera_get_black_level_auto (viewer->camera, NULL) != ARV_AUTO_OFF;
	auto_exposure = arv_camera_get_exposure_time_auto (viewer->camera, NULL) != ARV_AUTO_OFF;

	update_gain_ui (viewer, auto_gain);
	update_black_level_ui (viewer, auto_black_level);
	update_exposure_ui (viewer, auto_exposure);

	g_signal_handler_block (viewer->auto_gain_toggle, viewer->auto_gain_clicked);
	g_signal_handler_block (viewer->auto_black_level_toggle, viewer->auto_black_level_clicked);
	g_signal_handler_block (viewer->auto_exposure_toggle, viewer->auto_exposure_clicked);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_gain_toggle), auto_gain);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_black_level_toggle), auto_black_level);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->auto_exposure_toggle), auto_exposure);

	gtk_widget_set_sensitive (viewer->auto_gain_toggle,
		arv_camera_is_gain_auto_available (viewer->camera, NULL));
	gtk_widget_set_sensitive (viewer->auto_black_level_toggle,
		arv_camera_is_black_level_auto_available (viewer->camera, NULL));
	gtk_widget_set_sensitive (viewer->auto_exposure_toggle,
		arv_camera_is_exposure_auto_available (viewer->camera, NULL));

	g_signal_handler_unblock (viewer->auto_gain_toggle, viewer->auto_gain_clicked);
	g_signal_handler_unblock (viewer->auto_black_level_toggle, viewer->auto_black_level_clicked);
	g_signal_handler_unblock (viewer->auto_exposure_toggle, viewer->auto_exposure_clicked);
}

static gboolean
_save_gst_sample_to_file (GstSample *sample, const char *path, const char *mime_type, GError **error)
{
        GstSample *converted;
        GstCaps *caps;
        GstBuffer *gst_buffer;
        gboolean success = FALSE;

        g_return_val_if_fail (GST_IS_SAMPLE (sample), FALSE);

        caps = gst_caps_from_string (mime_type);
        converted = gst_video_convert_sample (sample, caps, GST_CLOCK_TIME_NONE, NULL);
        gst_caps_unref (caps);

        gst_buffer = gst_sample_get_buffer (converted);
        if (gst_buffer) {
                GstMapInfo map;

                gst_buffer_map (gst_buffer, &map, GST_MAP_READ);
                success = g_file_set_contents (path, (void *) map.data, map.size, error);
                gst_buffer_unmap (gst_buffer, &map);
        } else
        gst_sample_unref (converted);

        return success;
}

static void
snapshot_cb (GtkButton *button, ArvViewer *viewer)
{
        GtkFileFilter *filter;
        GtkFileFilter *filter_all;
        GtkWidget *dialog;
        GstSample *sample = NULL;
        ArvBuffer *buffer = NULL;
	char *path;
	char *filename;
	GDateTime *date;
	char *date_string;
	int width, height;
	const char *data;
	const char *pixel_format;
	size_t size;
        gint result;

        if (GST_IS_ELEMENT (viewer->videosink))
                sample = gst_base_sink_get_last_sample (GST_BASE_SINK (viewer->videosink));
        if (ARV_IS_BUFFER (viewer->last_buffer))
                buffer = g_object_ref (viewer->last_buffer);

        if (!ARV_IS_BUFFER (buffer) && !GST_IS_SAMPLE (sample)) {
                arv_viewer_show_notification (viewer, "No buffer available", NULL);
                return;
        }

	g_return_if_fail (ARV_IS_CAMERA (viewer->camera));

	pixel_format = arv_camera_get_pixel_format_as_string (viewer->camera, NULL);
	arv_buffer_get_image_region (buffer, NULL, NULL, &width, &height);
	data = arv_buffer_get_image_data (buffer, &size);

	date = g_date_time_new_now_local ();
	date_string = g_date_time_format (date, "%Y-%m-%d-%H:%M:%S");
	filename = g_strdup_printf ("%s-%s-%d-%d-%s-%s.raw",
				    arv_camera_get_vendor_name (viewer->camera, NULL),
				    arv_camera_get_device_serial_number (viewer->camera, NULL),
				    width,
				    height,
				    pixel_format != NULL ? pixel_format : "Unknown",
				    date_string);
	g_free (date_string);
	g_date_time_unref (date);

	path = g_build_filename (g_get_user_special_dir (G_USER_DIRECTORY_PICTURES), filename, NULL);

        dialog = gtk_file_chooser_dialog_new ("Save Snapshot", GTK_WINDOW (viewer->main_window),
                                              GTK_FILE_CHOOSER_ACTION_SAVE,
                                              "_Cancel",
                                              GTK_RESPONSE_CANCEL,
                                              "_Save",
                                              GTK_RESPONSE_ACCEPT,
                                              NULL);
        gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), path);
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), filename);

        filter_all = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter_all, "Supported image formats");

        if (GST_IS_SAMPLE (sample)) {
                filter = gtk_file_filter_new ();
                gtk_file_filter_add_mime_type (filter, "image/png");
                gtk_file_filter_add_mime_type (filter_all, "image/png");
                gtk_file_filter_set_name (filter, "PNG (*.png)");
                gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

                filter = gtk_file_filter_new ();
                gtk_file_filter_add_mime_type (filter, "image/jpeg");
                gtk_file_filter_add_mime_type (filter_all, "image/jpeg");
                gtk_file_filter_set_name (filter, "JPEG (*.jpeg)");
                gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
        }

        if (ARV_IS_BUFFER (buffer)) {
                filter = gtk_file_filter_new ();
                gtk_file_filter_add_pattern (filter, "*.raw");
                gtk_file_filter_add_pattern (filter_all, "*.raw");
                gtk_file_filter_set_name (filter, "Raw images (*.raw)");
                gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
        }

        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter_all);
        gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter_all);

	g_free (path);
	g_free (filename);

        result = gtk_dialog_run (GTK_DIALOG (dialog));
        if (result == GTK_RESPONSE_ACCEPT) {
                g_autoptr (GError) error = NULL;
                g_autofree char * content_type = NULL;
                gboolean success = FALSE;

                filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

                content_type = g_content_type_guess (filename, NULL, 0, NULL);

                if (GST_IS_SAMPLE (sample) && g_content_type_is_mime_type (content_type, "image/png")) {
                        success = _save_gst_sample_to_file (sample, filename, "image/png", NULL);
                } else if (GST_IS_SAMPLE (sample) && g_content_type_is_mime_type (content_type, "image/jpeg")) {
                        success = _save_gst_sample_to_file (sample, filename, "image/jpeg", NULL);
                } else if (ARV_IS_BUFFER (buffer)) {
                        success = g_file_set_contents (filename, data, size, &error);
                        g_free (filename);
                }

                if (!success)
                        arv_viewer_show_notification (viewer, "Failed to save image to file",
                                                      error != NULL ? error->message : NULL);
        }

        gtk_widget_destroy (dialog);
        g_clear_pointer (&sample, gst_sample_unref);
        g_clear_object (&buffer);
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
			arv_warning_viewer ("Failed to make stream thread high priority");
	}
}

static gboolean
update_status_bar_cb (void *data)
{
	ArvViewer *viewer = data;
	char *text;
	gint64 time_ms = g_get_real_time () / 1000;
	gint64 elapsed_time_ms = time_ms - viewer->last_status_bar_update_time_ms;
	guint64 n_images = arv_stream_get_info_uint64_by_name (viewer->stream, "n_completed_buffers");
	guint64 n_bytes = arv_stream_get_info_uint64_by_name (viewer->stream, "n_transferred_bytes");
	guint64 n_errors = arv_stream_get_info_uint64_by_name (viewer->stream, "n_failures");

	if (elapsed_time_ms == 0)
		return TRUE;

	text = g_strdup_printf ("%.1f fps (%.1f MB/s)",
				1000.0 * (n_images - viewer->last_n_images) / elapsed_time_ms,
				((n_bytes - viewer->last_n_bytes) / 1000.0) / elapsed_time_ms);
	gtk_label_set_label (GTK_LABEL (viewer->fps_label), text);
	g_free (text);

	text = g_strdup_printf ("%" G_GUINT64_FORMAT " image%s / %" G_GUINT64_FORMAT " error%s",
				n_images, n_images > 0 ? "s" : "",
				n_errors, n_errors > 0 ? "s" : "");
	gtk_label_set_label (GTK_LABEL (viewer->image_label), text);
	g_free (text);

	viewer->last_status_bar_update_time_ms = time_ms;
	viewer->last_n_images = n_images;
	viewer->last_n_bytes = n_bytes;

	return TRUE;
}

static void
update_camera_region (ArvViewer *viewer)
{
	gint x, y, width, height;
	gint dx, dy;
	gint min, max;
	gint inc;

	g_signal_handler_block (viewer->camera_x, viewer->camera_x_changed);
	g_signal_handler_block (viewer->camera_y, viewer->camera_y_changed);
	g_signal_handler_block (viewer->camera_width, viewer->camera_width_changed);
	g_signal_handler_block (viewer->camera_height, viewer->camera_height_changed);
	g_signal_handler_block (viewer->camera_binning_x, viewer->camera_binning_x_changed);
	g_signal_handler_block (viewer->camera_binning_y, viewer->camera_binning_y_changed);

	arv_camera_get_region (viewer->camera, &x, &y, &width, &height, NULL);
	arv_camera_get_binning (viewer->camera, &dx, &dy, NULL);

        if (gtk_widget_get_visible(viewer->camera_binning_x)) {
                arv_camera_get_x_binning_bounds (viewer->camera, &min, &max, NULL);
                inc = arv_camera_get_x_binning_increment (viewer->camera, NULL);
                gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->camera_binning_x), min, max);
                gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->camera_binning_x), inc, inc * 10);
                gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (viewer->camera_binning_x), TRUE);
                gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->camera_binning_x), dx);
        }
        if (gtk_widget_get_visible(viewer->camera_binning_y)) {
                arv_camera_get_y_binning_bounds (viewer->camera, &min, &max, NULL);
                inc = arv_camera_get_y_binning_increment (viewer->camera,  NULL);
                gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->camera_binning_y), min, max);
                gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->camera_binning_y), inc, inc * 10);
                gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (viewer->camera_binning_y), TRUE);
                gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->camera_binning_y), dy);
        }
        if (gtk_widget_get_visible(viewer->camera_x)) {
                arv_camera_get_x_offset_bounds (viewer->camera, &min, &max, NULL);
                inc = arv_camera_get_x_offset_increment (viewer->camera, NULL);
                gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->camera_x), min, max);
                gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->camera_x), inc, inc * 10);
                gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (viewer->camera_x), TRUE);
                gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->camera_x), x);
        }
        if (gtk_widget_get_visible(viewer->camera_y)) {
                arv_camera_get_y_offset_bounds (viewer->camera, &min, &max, NULL);
                inc = arv_camera_get_y_offset_increment (viewer->camera, NULL);
                gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->camera_y), min, max);
                gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->camera_y), inc, inc * 10);
                gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (viewer->camera_y), TRUE);
                gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->camera_y), y);
        }
        if (gtk_widget_get_visible(viewer->camera_width)) {
                arv_camera_get_width_bounds (viewer->camera, &min, &max, NULL);
                inc = arv_camera_get_width_increment (viewer->camera, NULL);
                gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->camera_width), min, max);
                gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->camera_width), inc, inc * 10);
                gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (viewer->camera_width), TRUE);
                gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->camera_width), width);
        }
        if (gtk_widget_get_visible(viewer->camera_height)) {
                arv_camera_get_height_bounds (viewer->camera, &min, &max, NULL);
                inc = arv_camera_get_height_increment (viewer->camera, NULL);
                gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->camera_height), min, max);
                gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->camera_height), inc, inc * 10);
                gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (viewer->camera_height), TRUE);
                gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->camera_height), height);
        }

	g_signal_handler_unblock (viewer->camera_x, viewer->camera_x_changed);
	g_signal_handler_unblock (viewer->camera_y, viewer->camera_y_changed);
	g_signal_handler_unblock (viewer->camera_width, viewer->camera_width_changed);
	g_signal_handler_unblock (viewer->camera_height, viewer->camera_height_changed);
	g_signal_handler_unblock (viewer->camera_binning_x, viewer->camera_binning_x_changed);
	g_signal_handler_unblock (viewer->camera_binning_y, viewer->camera_binning_y_changed);
}

static void
update_pixel_format (ArvViewer *viewer)
{
	GtkListStore *list_store;
	GtkTreeIter iter;
	gint64 *pixel_formats;
	const char *pixel_format_string;
	const char **pixel_format_strings;
	guint i, n_pixel_formats, n_pixel_format_strings, n_valid_formats;
        gboolean bayer_tooltip = FALSE;
        gint current_format = -1;

	g_signal_handler_block (viewer->pixel_format_combo, viewer->pixel_format_changed);

	list_store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (viewer->pixel_format_combo)));
	gtk_list_store_clear (list_store);
	n_valid_formats = 0;
	pixel_format_strings = arv_camera_dup_available_pixel_formats_as_strings (viewer->camera, &n_pixel_format_strings, NULL);
	pixel_formats = arv_camera_dup_available_pixel_formats (viewer->camera, &n_pixel_formats, NULL);
	g_assert (n_pixel_formats == n_pixel_format_strings);
	pixel_format_string = arv_camera_get_pixel_format_as_string (viewer->camera, NULL);
	for (i = 0; i < n_pixel_formats; i++) {
		const char *caps_string = arv_pixel_format_to_gst_caps_string (pixel_formats[i]);
                gboolean valid = FALSE;

                gtk_list_store_append (list_store, &iter);

                if (caps_string != NULL && g_str_has_prefix (caps_string, "video/x-bayer") && !has_bayer2rgb) {
                        bayer_tooltip = TRUE;
                } else if (caps_string != NULL) {
			if (current_format < 0 ||
                            g_strcmp0 (pixel_format_strings[i], pixel_format_string) == 0)
                                current_format = i;
			n_valid_formats++;
                        valid = TRUE;
		}

                gtk_list_store_set (list_store, &iter,
                                    0, pixel_format_strings[i],
                                    1, valid,
                                    -1);
	}
	g_free (pixel_formats);
	g_free (pixel_format_strings);
	gtk_widget_set_sensitive (viewer->pixel_format_combo, n_valid_formats > 0);
	gtk_widget_set_sensitive (viewer->video_mode_button,
                                  n_valid_formats > 0 &&
                                  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(viewer->component_check)));

	gtk_combo_box_set_active (GTK_COMBO_BOX (viewer->pixel_format_combo), current_format >= 0 ? current_format : 0);

        gtk_widget_set_tooltip_text (GTK_WIDGET (viewer->pixel_format_combo),
                                     bayer_tooltip ?
                                     "Found bayer pixel formats, but the GStreamer bayer plugin "
                                     "is not installed." :
                                     NULL);

	g_signal_handler_unblock (viewer->pixel_format_combo, viewer->pixel_format_changed);
}

static void
camera_region_cb (GtkSpinButton *spin_button, ArvViewer *viewer)
{
	int x = gtk_spin_button_get_value (GTK_SPIN_BUTTON (viewer->camera_x));
	int y = gtk_spin_button_get_value (GTK_SPIN_BUTTON (viewer->camera_y));
	int width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (viewer->camera_width));
	int height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (viewer->camera_height));

	arv_camera_set_region (viewer->camera, x, y, width, height, NULL);

	update_camera_region (viewer);
}

static void
camera_binning_cb (GtkSpinButton *spin_button, ArvViewer *viewer)
{
	int dx = gtk_spin_button_get_value (GTK_SPIN_BUTTON (viewer->camera_binning_x));
	int dy = gtk_spin_button_get_value (GTK_SPIN_BUTTON (viewer->camera_binning_y));

	arv_camera_set_binning (viewer->camera, dx, dy, NULL);

	update_camera_region (viewer);
}

static void
component_combo_cb (GtkComboBoxText *combo, ArvViewer *viewer)
{
	char *component;
        gboolean enabled;

	component = gtk_combo_box_text_get_active_text (combo);
        enabled = arv_camera_select_component (viewer->camera, component, ARV_COMPONENT_SELECTION_FLAGS_NONE, NULL,
                                               NULL);
        g_free (component);

	g_signal_handler_block (viewer->component_check, viewer->component_toggled);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewer->component_check), enabled);
	g_signal_handler_unblock (viewer->component_check, viewer->component_toggled);

	update_camera_region (viewer);
        update_pixel_format(viewer);
}

static void
component_toggled_cb (GtkToggleButton *button, ArvViewer *viewer)
{
        char *component;
        gboolean enable;

        component = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(viewer->component_combo));
        enable = gtk_toggle_button_get_active(button);

	gtk_widget_set_sensitive (viewer->video_mode_button, enable);

        arv_camera_select_component(viewer->camera, component,
                                    enable ? ARV_COMPONENT_SELECTION_FLAGS_ENABLE :
                                    ARV_COMPONENT_SELECTION_FLAGS_DISABLE,
                                    NULL, NULL);

        g_free (component);
}

static void
pixel_format_combo_cb (GtkComboBoxText *combo, ArvViewer *viewer)
{
	char *pixel_format;

	pixel_format = gtk_combo_box_text_get_active_text (combo);
	arv_camera_set_pixel_format_from_string (viewer->camera, pixel_format, NULL);
	g_free (pixel_format);
}

static void
update_device_list_cb (GtkToolButton *button, ArvViewer *viewer)
{
	GtkListStore *list_store;
	GtkTreeIter iter;
	unsigned int n_devices;
	unsigned int i;

	gtk_widget_set_sensitive (viewer->video_mode_button, FALSE);
	gtk_revealer_set_reveal_child (GTK_REVEALER(viewer->camera_parameters), FALSE);
	gtk_widget_set_sensitive (viewer->camera_parameters, FALSE);

	g_signal_handler_block (gtk_tree_view_get_selection (GTK_TREE_VIEW (viewer->camera_tree)), viewer->camera_selected);
	list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (viewer->camera_tree)));
	gtk_list_store_clear (list_store);
	arv_update_device_list ();
	n_devices = arv_get_n_devices ();
	for (i = 0; i < n_devices; i++) {
		GString *protocol;

		protocol = g_string_new (NULL);
		g_string_append_printf (protocol, "aravis-%s-symbolic", arv_get_device_protocol (i));
		g_string_ascii_down (protocol);

		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    0, arv_get_device_id (i),
				    1, protocol->str,
				    2, arv_get_device_vendor (i),
				    3, arv_get_device_model (i),
				    4, arv_get_device_serial_nbr (i),
				    -1);

		g_string_free (protocol, TRUE);
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

	if (ARV_IS_STREAM (viewer->stream))
		arv_stream_set_emit_signals (viewer->stream, FALSE);

	g_clear_object (&viewer->stream);
	g_clear_object (&viewer->pipeline);

	viewer->appsrc = NULL;

	g_clear_object (&viewer->last_buffer);

	if (ARV_IS_CAMERA (viewer->camera))
		arv_camera_stop_acquisition (viewer->camera, NULL);

	gtk_container_foreach (GTK_CONTAINER (viewer->video_frame), remove_widget, viewer->video_frame);

	if (viewer->status_bar_update_event > 0) {
		g_source_remove (viewer->status_bar_update_event);
		viewer->status_bar_update_event = 0;
	}

	if (viewer->exposure_update_event > 0) {
		g_source_remove (viewer->exposure_update_event);
		viewer->exposure_update_event = 0;
	}

	if (viewer->gain_update_event > 0) {
		g_source_remove (viewer->gain_update_event);
		viewer->gain_update_event = 0;
	}

	if (viewer->black_level_update_event > 0) {
		g_source_remove (viewer->black_level_update_event);
		viewer->black_level_update_event = 0;
	}
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
	GstCaps *caps;
	ArvPixelFormat pixel_format;
	unsigned payload;
	unsigned i;
	gint width, height;
	const char *caps_string;
        char *component;

	if (!ARV_IS_CAMERA (viewer->camera))
		return FALSE;

	stop_video (viewer);

        component = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(viewer->component_combo));
        if (component != NULL) {
                arv_camera_select_component (viewer->camera, component, ARV_COMPONENT_SELECTION_FLAGS_NONE,
                                             &viewer->component_id, NULL);
                g_free (component);
        }

	viewer->rotation = 0;
	viewer->stream = arv_camera_create_stream (viewer->camera, stream_cb, NULL, NULL);
	if (!ARV_IS_STREAM (viewer->stream)) {
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
			      "initial-packet-timeout", (unsigned) viewer->initial_packet_timeout * 1000,
			      "packet-timeout", (unsigned) viewer->packet_timeout * 1000,
			      "frame-retention", (unsigned) viewer->frame_retention * 1000,
			      NULL);
	}

	arv_stream_set_emit_signals (viewer->stream, TRUE);
	payload = arv_camera_get_payload (viewer->camera, NULL);
	for (i = 0; i < ARV_VIEWER_N_BUFFERS; i++)
		arv_stream_push_buffer (viewer->stream, arv_buffer_new (payload, NULL));

	set_camera_widgets(viewer);
	pixel_format = arv_camera_get_pixel_format (viewer->camera, NULL);

	caps_string = arv_pixel_format_to_gst_caps_string (pixel_format);
	if (caps_string == NULL) {
		g_message ("GStreamer cannot understand this camera pixel format: 0x%x!", (int) pixel_format);
		stop_video (viewer);
		return FALSE;
        } else if (g_str_has_prefix (caps_string, "video/x-bayer") && !has_bayer2rgb) {
		g_message ("GStreamer bayer plugin is required for pixel format: 0x%x!", (int) pixel_format);
		stop_video (viewer);
		return FALSE;
	}

        arv_camera_set_acquisition_mode (viewer->camera, ARV_ACQUISITION_MODE_CONTINUOUS, NULL);
	arv_camera_start_acquisition (viewer->camera, NULL);

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

#if 0 /* Disable glsink for now, it crashes when we come back to camera list with:
	(lt-arv-viewer:29151): Gdk-WARNING **: eglMakeCurrent failed
	(lt-arv-viewer:29151): Gdk-WARNING **: eglMakeCurrent failed
	(lt-arv-viewer:29151): Gdk-WARNING **: eglMakeCurrent failed
	Erreur de segmentation (core dumped)
	*/

		videosink = gst_element_factory_make ("gtkglsink", NULL);

		if (GST_IS_ELEMENT (videosink)) {
			GstElement *glupload;

			glupload = gst_element_factory_make ("glupload", NULL);
			gst_bin_add_many (GST_BIN (viewer->pipeline), glupload, videosink, NULL);
			gst_element_link_many (viewer->transform, glupload, videosink, NULL);
		} else {
#else
		{
#endif
			viewer->videosink = gst_element_factory_make ("gtksink", NULL);
			gst_bin_add_many (GST_BIN (viewer->pipeline), viewer->videosink, NULL);
			gst_element_link_many (viewer->transform, viewer->videosink, NULL);
		}

		g_object_get (viewer->videosink, "widget", &video_widget, NULL);
		gtk_container_add (GTK_CONTAINER (viewer->video_frame), video_widget);
		gtk_widget_show (video_widget);
		g_object_set(G_OBJECT (video_widget), "force-aspect-ratio", TRUE, NULL);
		gtk_widget_set_size_request (video_widget, 640, 480);
	} else {
		viewer->videosink = gst_element_factory_make ("autovideosink", NULL);
		gst_bin_add (GST_BIN (viewer->pipeline), viewer->videosink);
		gst_element_link_many (viewer->transform, viewer->videosink, NULL);
	}

	g_object_set(G_OBJECT (viewer->videosink), "sync", FALSE, NULL);

	caps = gst_caps_from_string (caps_string);
	arv_camera_get_region (viewer->camera, NULL, NULL, &width, &height, NULL);
	gst_caps_set_simple (caps,
			     "width", G_TYPE_INT, width,
			     "height", G_TYPE_INT, height,
			     "framerate", GST_TYPE_FRACTION, 0, 1,
			     NULL);
	gst_app_src_set_caps (GST_APP_SRC (viewer->appsrc), caps);
	gst_caps_unref (caps);

	g_object_set(G_OBJECT (viewer->appsrc), "format", GST_FORMAT_TIME, "is-live", TRUE, "do-timestamp", TRUE, NULL);

	if (!has_gtkglsink && !has_gtksink) {
		GstBus *bus;

		bus = gst_pipeline_get_bus (GST_PIPELINE (viewer->pipeline));
		gst_bus_set_sync_handler (bus, (GstBusSyncHandler) bus_sync_handler, viewer, NULL);
		gst_object_unref (bus);
	}

	gst_element_set_state (viewer->pipeline, GST_STATE_PLAYING);

	viewer->last_status_bar_update_time_ms = g_get_real_time () / 1000;
	viewer->last_n_images = 0;
	viewer->last_n_bytes = 0;
	viewer->status_bar_update_event = g_timeout_add_seconds (1, update_status_bar_cb, viewer);

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
        gtk_revealer_set_reveal_child (GTK_REVEALER(viewer->camera_parameters), FALSE);
	gtk_widget_set_sensitive (viewer->camera_parameters, FALSE);
	gtk_widget_set_sensitive (viewer->video_mode_button, FALSE);
	stop_video (viewer);
	g_clear_object (&viewer->camera);
	g_clear_pointer (&viewer->camera_name, g_free);
}

static void
set_sensitive (GtkCellLayout *cell_layout,
               GtkCellRenderer *cell,
               GtkTreeModel *tree_model,
               GtkTreeIter *iter,
               gpointer data)
{
        gboolean valid;

        gtk_tree_model_get (tree_model, iter, 1, &valid, -1);

        g_object_set(cell, "sensitive", valid, NULL);
}

static gboolean
start_camera (ArvViewer *viewer, const char *camera_id)
{
	GtkTreeIter iter;
	GtkListStore *list_store;
        const char **components;
        guint n_components;
	gboolean binning_available;
        gboolean region_offset_available;
        unsigned int i;

	stop_camera (viewer);

	viewer->camera = arv_camera_new (camera_id, NULL);

	if (!ARV_IS_CAMERA (viewer->camera)) {
                gtk_revealer_set_reveal_child (GTK_REVEALER(viewer->camera_parameters), FALSE);
                return FALSE;
        }

	arv_device_set_register_cache_policy (arv_camera_get_device (viewer->camera),
					      viewer->register_cache_policy);
	arv_device_set_range_check_policy (arv_camera_get_device (viewer->camera),
					   viewer->range_check_policy);

        if (arv_camera_is_uv_device (viewer->camera))
                arv_camera_uv_set_usb_mode (viewer->camera, viewer->usb_mode);

        if (arv_camera_is_gv_device(viewer->camera))
                arv_camera_gv_set_multipart(viewer->camera, TRUE, NULL);

	viewer->camera_name = g_strdup (camera_id);

        gtk_widget_set_sensitive (viewer->camera_parameters, TRUE);
	gtk_revealer_set_reveal_child (GTK_REVEALER(viewer->camera_parameters), TRUE);

	arv_camera_set_chunk_mode (viewer->camera, FALSE, NULL);

        if (arv_camera_is_component_available(viewer->camera, NULL)) {
                gint first_enabled_component = -1;

                gtk_widget_set_visible(viewer->component_label, TRUE);
                gtk_widget_set_visible(viewer->component_combo, TRUE);
                gtk_widget_set_visible(viewer->component_check, TRUE);

                g_signal_handler_block (viewer->component_check, viewer->component_toggled);
                g_signal_handler_block (viewer->component_combo, viewer->component_changed);

                list_store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (viewer->component_combo)));
                gtk_list_store_clear (list_store);
                components = arv_camera_dup_available_components (viewer->camera, &n_components, NULL);
                for (i = 0; i < n_components; i++) {
                        gtk_list_store_append (list_store, &iter);
                        gtk_list_store_set (list_store, &iter,
                                            0, components[i],
                                            -1);

                        if (first_enabled_component < 0 &&
                            arv_camera_select_component(viewer->camera, components[i],
                                                        ARV_COMPONENT_SELECTION_FLAGS_NONE, NULL, NULL))
                                first_enabled_component = i;
                }
                g_free (components);
                gtk_widget_set_sensitive (viewer->component_combo, n_components > 0);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewer->component_check), n_components > 0);
                if (n_components > 0)
                        gtk_combo_box_set_active (GTK_COMBO_BOX (viewer->component_combo), MAX(first_enabled_component, 0));
                g_signal_handler_unblock (viewer->component_check, viewer->component_toggled);
                g_signal_handler_unblock (viewer->component_combo, viewer->component_changed);
        } else {
                g_signal_handler_block (viewer->component_check, viewer->component_toggled);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewer->component_check), TRUE);
                g_signal_handler_unblock (viewer->component_check, viewer->component_toggled);
                gtk_widget_set_visible(viewer->component_label, FALSE);
                gtk_widget_set_visible(viewer->component_combo, FALSE);
                gtk_widget_set_visible(viewer->component_check, FALSE);
        }

        region_offset_available = arv_camera_is_region_offset_available (viewer->camera, NULL);
	gtk_widget_set_visible (viewer->camera_x, region_offset_available);
	gtk_widget_set_visible (viewer->camera_y, region_offset_available);
        gtk_widget_set_visible (viewer->camera_position_label, region_offset_available);
        gtk_widget_set_visible (viewer->camera_position_units, region_offset_available);

	binning_available = arv_camera_is_binning_available (viewer->camera, NULL);
	gtk_widget_set_visible (viewer->camera_binning_x, binning_available);
	gtk_widget_set_visible (viewer->camera_binning_y, binning_available);
        gtk_widget_set_visible (viewer->camera_binning_label, binning_available);
        gtk_widget_set_visible (viewer->camera_binning_units, binning_available);

        update_pixel_format(viewer);

	update_camera_region (viewer);

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
                        g_signal_handler_block (viewer->flip_vertical_toggle, viewer->flip_vertical_clicked);
                        g_signal_handler_block (viewer->flip_horizontal_toggle, viewer->flip_horizontal_clicked);
                        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(viewer->flip_vertical_toggle), FALSE);
                        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(viewer->flip_horizontal_toggle), FALSE);
                        g_signal_handler_unblock (viewer->flip_vertical_toggle, viewer->flip_vertical_clicked);
                        g_signal_handler_unblock (viewer->flip_horizontal_toggle, viewer->flip_horizontal_clicked);
			video_visibility = TRUE;
			arv_camera_get_region (viewer->camera, &x, &y, &width, &height, NULL);
			subtitle = g_strdup_printf ("%s %dx%d@%d,%d %s",
						    arv_camera_get_model_name (viewer->camera, NULL),
						    width, height,
						    x, y,
						    arv_camera_get_pixel_format_as_string (viewer->camera, NULL));
			gtk_stack_set_visible_child (GTK_STACK (viewer->main_stack), viewer->video_box);
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

static void
switch_to_camera_list_cb (GtkToolButton *button, ArvViewer *viewer)
{
	select_mode (viewer, ARV_VIEWER_MODE_CAMERA_LIST);
}

static void
switch_to_video_mode_cb (GtkToolButton *button, ArvViewer *viewer)
{
	select_mode (viewer, ARV_VIEWER_MODE_VIDEO);
}

static void
arv_viewer_quit_cb (GtkApplicationWindow *window, ArvViewer *viewer)
{
	stop_camera (viewer);
	g_application_quit (G_APPLICATION (viewer));
}

static void
video_frame_realize_cb (GtkWidget * widget, ArvViewer *viewer)
{
#ifdef GDK_WINDOWING_X11
	viewer->video_window_xid = GDK_WINDOW_XID (gtk_widget_get_window (widget));
#endif
#ifdef GDK_WINDOWING_WIN32
	viewer->video_window_xid = (guintptr) GDK_WINDOW_HWND (gtk_widget_get_window (widget));
#endif
}

static void
activate (GApplication *application)
{
	ArvViewer *viewer = (ArvViewer *) application;
	g_autoptr (GtkBuilder) builder = NULL;
        g_autoptr (GtkListStore) list_store = NULL;
        GtkCellRenderer *renderer;

	builder = gtk_builder_new_from_resource ("/org/aravis/viewer/arv-viewer.ui");

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
	viewer->component_label = GTK_WIDGET (gtk_builder_get_object (builder, "component_label"));
	viewer->component_combo = GTK_WIDGET (gtk_builder_get_object (builder, "component_combo"));
	viewer->component_check = GTK_WIDGET (gtk_builder_get_object (builder, "component_check"));
	viewer->pixel_format_combo = GTK_WIDGET (gtk_builder_get_object (builder, "pixel_format_combo"));
	viewer->camera_x = GTK_WIDGET (gtk_builder_get_object (builder, "camera_x"));
	viewer->camera_y = GTK_WIDGET (gtk_builder_get_object (builder, "camera_y"));
        viewer->camera_position_label = GTK_WIDGET(gtk_builder_get_object(builder, "camera_position_label"));
        viewer->camera_position_units = GTK_WIDGET(gtk_builder_get_object(builder, "camera_position_units"));
	viewer->camera_binning_x = GTK_WIDGET (gtk_builder_get_object (builder, "camera_binning_x"));
	viewer->camera_binning_y = GTK_WIDGET (gtk_builder_get_object (builder, "camera_binning_y"));
        viewer->camera_binning_label = GTK_WIDGET(gtk_builder_get_object(builder, "camera_binning_label"));
        viewer->camera_binning_units = GTK_WIDGET(gtk_builder_get_object(builder, "camera_binning_units"));
	viewer->camera_width = GTK_WIDGET (gtk_builder_get_object (builder, "camera_width"));
	viewer->camera_height = GTK_WIDGET (gtk_builder_get_object (builder, "camera_height"));
	viewer->video_box = GTK_WIDGET (gtk_builder_get_object (builder, "video_box"));
	viewer->video_frame = GTK_WIDGET (gtk_builder_get_object (builder, "video_frame"));
	viewer->fps_label = GTK_WIDGET (gtk_builder_get_object (builder, "fps_label"));
	viewer->image_label = GTK_WIDGET (gtk_builder_get_object (builder, "image_label"));
	viewer->trigger_combo_box = GTK_WIDGET (gtk_builder_get_object (builder, "trigger_combobox"));
	viewer->frame_rate_entry = GTK_WIDGET (gtk_builder_get_object (builder, "frame_rate_entry"));
	viewer->exposure_spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "exposure_spinbutton"));
	viewer->exposure_hscale = GTK_WIDGET (gtk_builder_get_object (builder, "exposure_hscale"));
	viewer->auto_exposure_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "auto_exposure_togglebutton"));
	viewer->gain_spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "gain_spinbutton"));
	viewer->gain_hscale = GTK_WIDGET (gtk_builder_get_object (builder, "gain_hscale"));
	viewer->auto_gain_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "auto_gain_togglebutton"));
	viewer->black_level_spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "black_level_spinbutton"));
	viewer->black_level_hscale = GTK_WIDGET (gtk_builder_get_object (builder, "black_level_hscale"));
	viewer->auto_black_level_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "auto_black_level_togglebutton"));
	viewer->rotate_cw_button = GTK_WIDGET (gtk_builder_get_object (builder, "rotate_cw_button"));
	viewer->flip_vertical_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "flip_vertical_togglebutton"));
	viewer->flip_horizontal_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "flip_horizontal_togglebutton"));
	viewer->acquisition_button = GTK_WIDGET (gtk_builder_get_object (builder, "acquisition_button"));

        viewer->notification_revealer = GTK_WIDGET (gtk_builder_get_object (builder, "notification_revealer"));
        viewer->notification_label = GTK_WIDGET (gtk_builder_get_object (builder, "notification_label"));
        viewer->notification_details = GTK_WIDGET (gtk_builder_get_object (builder, "notification_details"));
        viewer->notification_dismiss = GTK_WIDGET (gtk_builder_get_object (builder, "notification_dismiss"));

        g_signal_connect (viewer->notification_dismiss, "clicked",
                          G_CALLBACK (notification_dismiss_clicked_cb), viewer);

        list_store = gtk_list_store_new(1, G_TYPE_STRING);
        gtk_combo_box_set_model (GTK_COMBO_BOX (viewer->component_combo), GTK_TREE_MODEL (list_store));

        renderer = gtk_cell_renderer_text_new();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (viewer->component_combo));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (viewer->component_combo), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (viewer->component_combo), renderer, "text", 0, NULL);

        list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
        gtk_combo_box_set_model (GTK_COMBO_BOX (viewer->pixel_format_combo), GTK_TREE_MODEL (list_store));

        renderer = gtk_cell_renderer_text_new();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (viewer->pixel_format_combo));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (viewer->pixel_format_combo), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (viewer->pixel_format_combo), renderer, "text", 0, NULL);
        gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (viewer->pixel_format_combo), renderer, set_sensitive, NULL, NULL);

	gtk_widget_set_no_show_all (viewer->trigger_combo_box, TRUE);

	gtk_widget_show_all (viewer->main_window);

	gtk_application_add_window (GTK_APPLICATION (application), GTK_WINDOW (viewer->main_window));

	g_signal_connect (viewer->refresh_button, "clicked", G_CALLBACK (update_device_list_cb), viewer);
	g_signal_connect (viewer->video_mode_button, "clicked", G_CALLBACK (switch_to_video_mode_cb), viewer);
	g_signal_connect (viewer->back_button, "clicked", G_CALLBACK (switch_to_camera_list_cb), viewer);
	g_signal_connect (viewer->main_window, "destroy", G_CALLBACK (arv_viewer_quit_cb), viewer);
	g_signal_connect (viewer->snapshot_button, "clicked", G_CALLBACK (snapshot_cb), viewer);
	viewer->rotate_cw_clicked = g_signal_connect (viewer->rotate_cw_button, "clicked",
                                                      G_CALLBACK (rotate_cw_cb), viewer);
	viewer->flip_horizontal_clicked = g_signal_connect (viewer->flip_horizontal_toggle,
                                                            "clicked", G_CALLBACK (flip_horizontal_cb), viewer);
	viewer->flip_vertical_clicked = g_signal_connect (viewer->flip_vertical_toggle, "clicked",
                                                          G_CALLBACK (flip_vertical_cb), viewer);
	g_signal_connect (viewer->frame_rate_entry, "activate", G_CALLBACK (frame_rate_entry_cb), viewer);
	g_signal_connect (viewer->frame_rate_entry, "focus-out-event", G_CALLBACK (frame_rate_entry_focus_cb), viewer);

	if (!has_gtksink && !has_gtkglsink) {
		g_signal_connect (viewer->video_frame, "realize", G_CALLBACK (video_frame_realize_cb), viewer);
	}

	viewer->camera_selected = g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (viewer->camera_tree)), "changed",
						    G_CALLBACK (camera_selection_changed_cb), viewer);
	viewer->exposure_spin_changed = g_signal_connect (viewer->exposure_spin_button, "value-changed",
							  G_CALLBACK (exposure_spin_cb), viewer);
	viewer->exposure_hscale_changed = g_signal_connect (viewer->exposure_hscale, "value-changed",
							    G_CALLBACK (exposure_scale_cb), viewer);
	viewer->auto_exposure_clicked = g_signal_connect (viewer->auto_exposure_toggle, "clicked",
							  G_CALLBACK (auto_exposure_cb), viewer);
	viewer->gain_spin_changed = g_signal_connect (viewer->gain_spin_button, "value-changed",
						      G_CALLBACK (gain_spin_cb), viewer);
	viewer->gain_hscale_changed = g_signal_connect (viewer->gain_hscale, "value-changed",
							G_CALLBACK (gain_scale_cb), viewer);
	viewer->auto_gain_clicked = g_signal_connect (viewer->auto_gain_toggle, "clicked",
						      G_CALLBACK (auto_gain_cb), viewer);
	viewer->black_level_spin_changed = g_signal_connect (viewer->black_level_spin_button, "value-changed",
						      G_CALLBACK (black_level_spin_cb), viewer);
	viewer->black_level_hscale_changed = g_signal_connect (viewer->black_level_hscale, "value-changed",
							G_CALLBACK (black_level_scale_cb), viewer);
	viewer->auto_black_level_clicked = g_signal_connect (viewer->auto_black_level_toggle, "clicked",
						      G_CALLBACK (auto_black_level_cb), viewer);
	viewer->component_changed = g_signal_connect (viewer->component_combo, "changed",
							 G_CALLBACK (component_combo_cb), viewer);
        viewer->component_toggled = g_signal_connect (viewer->component_check, "toggled",
                                                      G_CALLBACK (component_toggled_cb), viewer);
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
	gtk_revealer_set_reveal_child (GTK_REVEALER(viewer->camera_parameters), FALSE);

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
viewer_shutdown (GApplication *application)
{
	G_APPLICATION_CLASS (arv_viewer_parent_class)->shutdown (application);

	arv_shutdown ();
}

static void
finalize (GObject *object)
{
	G_OBJECT_CLASS (arv_viewer_parent_class)->finalize (object);
}

ArvViewer *
arv_viewer_new (void)
{
  ArvViewer *arv_viewer;

  if (!gstreamer_plugin_check ())
	  return NULL;

  g_set_application_name ("Aravis Viewer");

  arv_viewer = g_object_new (arv_viewer_get_type (),
			     "application-id", "org.aravis.viewer",
			     "flags", G_APPLICATION_NON_UNIQUE,
			     "inactivity-timeout", 30000,
			     NULL);

  return arv_viewer;
}


static void
arv_viewer_init (ArvViewer *viewer)
{
	viewer->auto_socket_buffer = FALSE;
	viewer->packet_resend = TRUE;
	viewer->packet_timeout = 20;
	viewer->frame_retention = 100;
	viewer->register_cache_policy = ARV_REGISTER_CACHE_POLICY_DEFAULT;
	viewer->range_check_policy = ARV_RANGE_CHECK_POLICY_DEFAULT;
}

static void
arv_viewer_class_init (ArvViewerClass *class)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = finalize;

  application_class->startup = startup;
  application_class->shutdown = viewer_shutdown;
  application_class->activate = activate;
}
