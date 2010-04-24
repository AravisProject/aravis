#include <arv.h>
#include <gst/gst.h>

int
main (int argc, char **argv)
{
	GstElement *pipeline, *source, *sink;
	GMainLoop *loop;

	gst_init (&argc, &argv);

	pipeline = gst_pipeline_new ("pipeline");
	source = gst_element_factory_make ("videotestsrc", "source");
	sink = gst_element_factory_make ("autovideosink", "sink");

	gst_bin_add_many (GST_BIN (pipeline), source, sink, NULL);
	gst_element_link_many (source, sink, NULL);

	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);

	loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (loop);

	gst_object_unref (pipeline);
	g_main_loop_unref (loop);

	return 0;
}
