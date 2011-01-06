#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/interfaces/xoverlay.h>
#include <gdk/gdkx.h>
#include <arv.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
	ArvCamera *camera;
	ArvDevice *device;
	ArvStream *stream;

	GstElement *pipeline;
	GstElement *appsrc;

	guint64 timestamp_offset;
	guint64 last_timestamp;

	GtkWidget *main_window;
	GtkWidget *drawing_area;
	GtkWidget *camera_combo_box;
	GtkWidget *frame_rate_entry;
	GtkWidget *exposure_spin_button;
	GtkWidget *gain_spin_button;
	GtkWidget *exposure_hscale;
	GtkWidget *gain_hscale;

	gulong exposure_spin_changed;
	gulong gain_spin_changed;
	gulong exposure_hscale_changed;
	gulong gain_hscale_changed;

	double exposure_min, exposure_max;
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

void
arv_viewer_new_buffer_cb (ArvStream *stream, ArvViewer *viewer)
{
	ArvBuffer *arv_buffer;
	GstBuffer *buffer;

	arv_buffer = arv_stream_pop_buffer (stream);
	if (arv_buffer == NULL)
		return;

	if (arv_buffer->status == ARV_BUFFER_STATUS_SUCCESS) {
		buffer = gst_buffer_new ();

		GST_BUFFER_DATA (buffer) = arv_buffer->data;
		GST_BUFFER_MALLOCDATA (buffer) = NULL;
		GST_BUFFER_SIZE (buffer) = arv_buffer->size;

		if (viewer->timestamp_offset == 0) {
			viewer->timestamp_offset = arv_buffer->timestamp_ns;
			viewer->last_timestamp = arv_buffer->timestamp_ns;
		}

		GST_BUFFER_TIMESTAMP (buffer) = arv_buffer->timestamp_ns - viewer->timestamp_offset;
		GST_BUFFER_DURATION (buffer) = arv_buffer->timestamp_ns - viewer->last_timestamp;

		gst_app_src_push_buffer (GST_APP_SRC (viewer->appsrc), buffer);
	}

	arv_stream_push_buffer (stream, arv_buffer);
}

void
arv_viewer_frame_rate_entry_cb (GtkEntry *entry, ArvViewer *viewer)
{
	const char *text;

	text = gtk_entry_get_text (entry);

	arv_camera_set_frame_rate (viewer->camera, g_strtod (text, NULL));
}

void
arv_viewer_exposure_spin_cb (GtkSpinButton *spin_button, ArvViewer *viewer)
{
	double exposure = gtk_spin_button_get_value (spin_button);
	double log_exposure = arv_viewer_value_to_log (exposure, viewer->exposure_min, viewer->exposure_max);

	arv_camera_set_exposure_time (viewer->camera, exposure); 

	g_signal_handler_block (viewer->exposure_hscale, viewer->exposure_hscale_changed);
	gtk_range_set_value (GTK_RANGE (viewer->exposure_hscale), log_exposure);
	g_signal_handler_unblock (viewer->exposure_hscale, viewer->exposure_hscale_changed);
}

void
arv_viewer_gain_spin_cb (GtkSpinButton *spin_button, ArvViewer *viewer)
{
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

	arv_camera_set_exposure_time (viewer->camera, exposure);

	g_signal_handler_block (viewer->exposure_spin_button, viewer->exposure_spin_changed);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->exposure_spin_button), exposure);
	g_signal_handler_unblock (viewer->exposure_spin_button, viewer->exposure_spin_changed);
}

void
arv_viewer_gain_scale_cb (GtkRange *range, ArvViewer *viewer)
{
	arv_camera_set_gain (viewer->camera, gtk_range_get_value (range));

	g_signal_handler_block (viewer->gain_spin_button, viewer->gain_spin_changed);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->gain_spin_button), gtk_range_get_value (range));
	g_signal_handler_unblock (viewer->gain_spin_button, viewer->gain_spin_changed);
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

	viewer->timestamp_offset = 0;
	viewer->last_timestamp = 0;
}

void
arv_viewer_select_camera_cb (GtkComboBox *combo_box, ArvViewer *viewer)
{
	GtkTreeIter iter;
	GtkTreeModel *list_store;
	GstCaps *caps;
	GstElement *ffmpegcolorspace;
	GstElement *ximagesink;
	char *camera_id;
	char *text;
	unsigned int payload;
	int width;
	int height;
	unsigned int i;
	gulong window_xid;
	double exposure;
	double log_exposure;
	double frame_rate;
	gint64 gain, gain_min, gain_max;

	g_return_if_fail (viewer != NULL);

	arv_viewer_release_camera (viewer);

	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (viewer->camera_combo_box), &iter);
	list_store = gtk_combo_box_get_model (GTK_COMBO_BOX (viewer->camera_combo_box));
	gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 0, &camera_id, -1);
	viewer->camera = arv_camera_new (camera_id);
	g_free (camera_id);

	viewer->stream = arv_camera_create_stream (viewer->camera, NULL, NULL);
	arv_stream_set_emit_signals (viewer->stream, TRUE);
	payload = arv_camera_get_payload (viewer->camera);
	for (i = 0; i < 5; i++)
		arv_stream_push_buffer (viewer->stream, arv_buffer_new (payload, NULL));

	arv_camera_get_region (viewer->camera, NULL, NULL, &width, &height);
	exposure = arv_camera_get_exposure_time (viewer->camera);
	gain = arv_camera_get_gain (viewer->camera);
	arv_camera_get_exposure_time_bounds (viewer->camera, &viewer->exposure_min, &viewer->exposure_max);
	arv_camera_get_gain_bounds (viewer->camera, &gain_min, &gain_max);
	frame_rate = arv_camera_get_frame_rate (viewer->camera);

	gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->exposure_spin_button), viewer->exposure_min, viewer->exposure_max);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->exposure_spin_button), 200.0, 1000.0); 
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->exposure_spin_button), exposure);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (viewer->gain_spin_button), gain_min, gain_max);
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON (viewer->gain_spin_button), 1, 10);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (viewer->gain_spin_button), gain);

	log_exposure = arv_viewer_value_to_log (exposure, viewer->exposure_min, viewer->exposure_max);

	gtk_range_set_range (GTK_RANGE (viewer->exposure_hscale), 0.0, 1.0);
	gtk_range_set_value (GTK_RANGE (viewer->exposure_hscale), log_exposure);
	gtk_range_set_range (GTK_RANGE (viewer->gain_hscale), gain_min, gain_max);
	gtk_range_set_value (GTK_RANGE (viewer->gain_hscale), gain);

	text = g_strdup_printf ("%g", frame_rate);
	gtk_entry_set_text (GTK_ENTRY (viewer->frame_rate_entry), text);
	g_free (text);

	arv_camera_start_acquisition (viewer->camera);

	viewer->pipeline = gst_pipeline_new ("pipeline");

	viewer->appsrc = gst_element_factory_make ("appsrc", "appsrc");
	ffmpegcolorspace = gst_element_factory_make ("ffmpegcolorspace", "ffmpegcolorspace");
	ximagesink = gst_element_factory_make ("xvimagesink", "xvimagesink");
	g_object_set (ximagesink, "force-aspect-ratio", TRUE, NULL);
	gst_bin_add_many (GST_BIN (viewer->pipeline), viewer->appsrc, ffmpegcolorspace, ximagesink, NULL);
	gst_element_link_many (viewer->appsrc, ffmpegcolorspace, ximagesink, NULL);
	caps = gst_caps_new_simple ("video/x-raw-gray",
				    "bpp", G_TYPE_INT, 8,
				    "depth", G_TYPE_INT, 8,
				    "endianness", G_TYPE_INT, G_BIG_ENDIAN,
				    "width", G_TYPE_INT, width,
				    "height", G_TYPE_INT, height,
				    "framerate", GST_TYPE_FRACTION, (unsigned int ) (double) (0.5 + frame_rate), 1,
				    "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
				    NULL);
	gst_app_src_set_caps (GST_APP_SRC (viewer->appsrc), caps);
	gst_caps_unref (caps);
	gst_element_set_state (viewer->pipeline, GST_STATE_PLAYING);

	window_xid = GDK_WINDOW_XID (viewer->drawing_area->window);
	gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (ximagesink), window_xid);

	g_signal_connect (viewer->stream, "new-buffer", G_CALLBACK (arv_viewer_new_buffer_cb), viewer);
}

void
arv_viewer_free (ArvViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	arv_viewer_release_camera (viewer);
}

void
arv_viewer_quit_cb (GtkWidget *widget, ArvViewer *viewer)
{
	arv_viewer_free (viewer);

	gtk_main_quit ();
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
	gtk_builder_add_from_file (builder, ui_filename, NULL);
	g_free (ui_filename);

	viewer->camera_combo_box = GTK_WIDGET (gtk_builder_get_object (builder, "camera_combobox"));
	viewer->main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	viewer->drawing_area = GTK_WIDGET (gtk_builder_get_object (builder, "video_drawingarea"));
	viewer->frame_rate_entry = GTK_WIDGET (gtk_builder_get_object (builder, "frame_rate_entry"));
	viewer->exposure_spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "exposure_spinbutton"));
	viewer->gain_spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "gain_spinbutton"));
	viewer->exposure_hscale = GTK_WIDGET (gtk_builder_get_object (builder, "exposure_hscale"));
	viewer->gain_hscale = GTK_WIDGET (gtk_builder_get_object (builder, "gain_hscale"));

	g_object_unref (builder);

	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (viewer->camera_combo_box), cell, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (viewer->camera_combo_box), cell, "text", 0, NULL);

	gtk_widget_show_all (viewer->main_window);

	g_signal_connect (viewer->main_window, "destroy", G_CALLBACK (arv_viewer_quit_cb), viewer);
	g_signal_connect (viewer->camera_combo_box, "changed", G_CALLBACK (arv_viewer_select_camera_cb), viewer);

	g_signal_connect (viewer->frame_rate_entry, "changed", G_CALLBACK (arv_viewer_frame_rate_entry_cb), viewer);

	viewer->exposure_spin_changed = g_signal_connect (viewer->exposure_spin_button, "value-changed",
							  G_CALLBACK (arv_viewer_exposure_spin_cb), viewer);
	viewer->gain_spin_changed = g_signal_connect (viewer->gain_spin_button, "value-changed",
						      G_CALLBACK (arv_viewer_gain_spin_cb), viewer);
	viewer->exposure_hscale_changed = g_signal_connect (viewer->exposure_hscale, "value-changed",
							    G_CALLBACK (arv_viewer_exposure_scale_cb), viewer);
	viewer->gain_hscale_changed = g_signal_connect (viewer->gain_hscale, "value-changed",
							G_CALLBACK (arv_viewer_gain_scale_cb), viewer);

	return viewer;
}

int
main (int argc,char *argv[])
{
	ArvViewer *viewer;

	gtk_init (&argc, &argv);
	gst_init (&argc, &argv);

	viewer = arv_viewer_new ();

	arv_viewer_update_device_list_cb (viewer);
	arv_viewer_select_camera_cb (NULL, viewer);

	gtk_main ();

	return EXIT_SUCCESS;
}
