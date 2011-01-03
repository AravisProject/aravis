#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/interfaces/xoverlay.h>
#include <gdk/gdkx.h>
#include <arv.h>
#include <stdlib.h>

typedef struct {
	GstElement *appsrc;

	guint64 timestamp_offset;
	guint64 last_timestamp;
} CallbackData;

void
new_buffer_cb (ArvStream *stream, CallbackData *data)
{
	ArvBuffer *arv_buffer;
	GstBuffer *buffer;

	arv_buffer = arv_stream_pop_buffer (stream);
	if (arv_buffer == NULL)
		return;

	buffer = gst_buffer_new ();

	GST_BUFFER_DATA (buffer) = arv_buffer->data;
	GST_BUFFER_MALLOCDATA (buffer) = NULL;
	GST_BUFFER_SIZE (buffer) = arv_buffer->size;

	if (data->timestamp_offset == 0) {
		data->timestamp_offset = arv_buffer->timestamp_ns;
		data->last_timestamp = arv_buffer->timestamp_ns;
	}

	GST_BUFFER_TIMESTAMP (buffer) = arv_buffer->timestamp_ns - data->timestamp_offset;
	GST_BUFFER_DURATION (buffer) = arv_buffer->timestamp_ns - data->last_timestamp;

	gst_app_src_push_buffer (GST_APP_SRC (data->appsrc), buffer);

	arv_stream_push_buffer (stream, arv_buffer);
}

int
main (int argc,char *argv[])
{
	GtkBuilder *builder;
	GtkWidget *widget;
	GtkListStore *list_store;
	GtkCellRenderer *cell;
	GtkTreeIter iter;
	ArvCamera *camera;
	ArvStream *stream;
	GstElement *pipeline;
	GstElement *appsrc;
	GstElement *ffmpegcolorspace;
	GstElement *ximagesink;
	GstCaps *caps;
	char *camera_id;
	char *ui_filename;
	unsigned int n_devices;
	unsigned int i;
	unsigned int payload;
	int width;
	int height;
	unsigned int frame_rate;
	CallbackData data;
	gulong window_xid;

	gtk_init (&argc, &argv);
	gst_init (&argc, &argv);

	builder = gtk_builder_new ();

	ui_filename = g_build_filename (ARAVIS_DATA_DIR, "arv-viewer.ui", NULL);
	gtk_builder_add_from_file (builder, ui_filename, NULL);
	g_free (ui_filename);

	widget = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));

	g_signal_connect (widget, "destroy", G_CALLBACK (gtk_main_quit), NULL);

	gtk_widget_show_all (widget);

	widget = GTK_WIDGET (gtk_builder_get_object (builder, "camera_combobox"));
	list_store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_combo_box_set_model (GTK_COMBO_BOX (widget), GTK_TREE_MODEL (list_store));
	arv_update_device_list ();
	n_devices = arv_get_n_devices ();
	for (i = 0; i < n_devices; i++) {
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter, 0, arv_get_device_id (i), -1);
	}
	if (n_devices > 0)
		gtk_combo_box_set_active (GTK_COMBO_BOX (widget), 0);
	if (n_devices <= 1)
		gtk_widget_set_sensitive (widget, FALSE);

	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), cell, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), cell, "text", 0, NULL);


	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 0, &camera_id, -1);
	camera = arv_camera_new (camera_id);
	g_free (camera_id);

	stream = arv_camera_create_stream (camera, NULL, NULL);
	arv_stream_set_emit_signals (stream, TRUE);
	payload = arv_camera_get_payload (camera);
	for (i = 0; i < 50; i++)
		arv_stream_push_buffer (stream, arv_buffer_new (payload, NULL));

	arv_camera_get_region (camera, NULL, NULL, &width, &height);
	frame_rate = (unsigned int) (double) (0.5 + arv_camera_get_frame_rate (camera));

	arv_camera_start_acquisition (camera);

	pipeline = gst_pipeline_new ("pipeline");

	appsrc = gst_element_factory_make ("appsrc", "appsrc");
	ffmpegcolorspace = gst_element_factory_make ("ffmpegcolorspace", "ffmpegcolorspace");
	ximagesink = gst_element_factory_make ("xvimagesink", "xvimagesink");
	g_object_set (ximagesink, "force-aspect-ratio", TRUE, NULL);
	gst_bin_add_many (GST_BIN (pipeline), appsrc, ffmpegcolorspace, ximagesink, NULL);
	gst_element_link_many (appsrc, ffmpegcolorspace, ximagesink, NULL);
	caps = gst_caps_new_simple ("video/x-raw-gray",
				    "bpp", G_TYPE_INT, 8,
				    "depth", G_TYPE_INT, 8,
				    "endianness", G_TYPE_INT, G_BIG_ENDIAN,
				    "width", G_TYPE_INT, width,
				    "height", G_TYPE_INT, height,
				    "framerate", GST_TYPE_FRACTION, frame_rate, 1,
				    "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
				    NULL);
	gst_app_src_set_caps (GST_APP_SRC (appsrc), caps);
	gst_caps_unref (caps);
	gst_element_set_state (pipeline, GST_STATE_PLAYING);

	widget = GTK_WIDGET (gtk_builder_get_object (builder, "video_drawingarea"));
	window_xid = GDK_WINDOW_XID (widget->window);
	gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (ximagesink), window_xid);

	data.appsrc = appsrc;
	data.timestamp_offset = 0;
	data.last_timestamp = 0;
	g_signal_connect (stream, "new-buffer", G_CALLBACK (new_buffer_cb), &data);

	g_object_unref (builder);

	gtk_main ();

	arv_camera_stop_acquisition (camera);
	g_object_unref (stream);

	g_object_unref (camera);

	return EXIT_SUCCESS;
}
