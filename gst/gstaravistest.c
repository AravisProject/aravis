#include <arv.h>
#include <gst/gst.h>

int
main (int argc, char **argv)
{
	GstElement *pipeline, *source, *filter, *sink;
	GstElement *theora_encoder, *theora_decoder;
	GMainLoop *loop;

	gst_init (&argc, &argv);

	gst_plugin_load_file (GST_ARAVIS_PLUGIN, NULL);

	pipeline = gst_pipeline_new ("pipeline");
	source = gst_element_factory_make ("aravissrc", "source");
	filter = gst_element_factory_make ("ffmpegcolorspace", "filter");
	theora_encoder = gst_element_factory_make ("theoraenc", "theoraenc");
	theora_decoder = gst_element_factory_make ("theoradec", "theoradec");
	sink = gst_element_factory_make ("autovideosink", "sink");

	g_object_set (source, "width", 256, "height", 256, "h-binning", 4, "v-binning", 4, NULL);

	gst_bin_add_many (GST_BIN (pipeline), source, filter, theora_encoder, theora_decoder, sink, NULL);
	gst_element_link_many (source, filter, theora_encoder, theora_decoder, sink, NULL);

	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);

	loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (loop);

	gst_object_unref (pipeline);
	g_main_loop_unref (loop);

	return 0;
}
