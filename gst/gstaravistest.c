#include <arv.h>
#include <gst/gst.h>

static int arv_option_width = 256;
static int arv_option_height = 256;
static int arv_option_horizontal_binning = 4;
static int arv_option_vertical_binning = 4;
static char *arv_option_sink_type = NULL;
static char *arv_option_debug_domains = NULL;
static char *arv_option_host = "127.0.0.1";

static const GOptionEntry arv_option_entries[] =
{
	{ "width", 		'w', 0, G_OPTION_ARG_INT,
		&arv_option_width,		"Width", NULL },
	{ "height", 		'g', 0, G_OPTION_ARG_INT,
		&arv_option_height, 		"Height", NULL },
	{ "h-binning", 		'\0', 0, G_OPTION_ARG_INT,
		&arv_option_horizontal_binning,"Horizontal binning", NULL },
	{ "v-binning", 		'\0', 0, G_OPTION_ARG_INT,
		&arv_option_vertical_binning, 	"Vertical binning", NULL },
	{ "sink",		's', 0, G_OPTION_ARG_STRING,
		&arv_option_sink_type,		"Sink type", NULL},
	{ "host",		't', 0, G_OPTION_ARG_STRING,
		&arv_option_host,		"Host IP address", NULL},
	{ "debug", 		'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	"Debug mode", NULL },
	{ NULL }
};

int
main (int argc, char **argv)
{
	GstElement *pipeline, *source, *filter;
	GMainLoop *loop;
	GOptionContext *context;
	GError *error = NULL;

	gst_init (&argc, &argv);

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, arv_option_entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	arv_debug_enable (arv_option_debug_domains);

	gst_plugin_load_file (GST_ARAVIS_PLUGIN, NULL);

	pipeline = gst_pipeline_new ("pipeline");
	source = gst_element_factory_make ("aravissrc", "source");
	filter = gst_element_factory_make ("ffmpegcolorspace", "filter");

	g_object_set (source,
		      "width", arv_option_width,
		      "height", arv_option_height,
		      "h-binning", arv_option_horizontal_binning,
		      "v-binning", arv_option_vertical_binning, NULL);

	if (g_strcmp0 (arv_option_sink_type, "theora_udp") == 0) {
		GstElement *theora_encoder;
		GstElement *udp_sink;

		theora_encoder = gst_element_factory_make ("theoraenc", "theoraenc");
		udp_sink = gst_element_factory_make ("udpsink", "udpsink");

		g_object_set (theora_encoder, "bitrate", 150, "speed-level", 0, NULL);
		g_object_set (udp_sink, "host", arv_option_host, "port", 1234, NULL);

		gst_bin_add_many (GST_BIN (pipeline), source, filter, theora_encoder, udp_sink, NULL);
		gst_element_link_many (source, filter, theora_encoder, udp_sink, NULL);
	} else if (g_strcmp0 (arv_option_sink_type, "mjpeg_tcp") == 0) {
		GstElement *encoder;
		GstElement *muxer;
		GstElement *tcp_sink;

		encoder = gst_element_factory_make ("jpegenc", "encoder");
		muxer = gst_element_factory_make ("multipartmux", "muxer");
		tcp_sink = gst_element_factory_make ("tcpserversink", "tcpsink");

		g_object_set (encoder, "quality", 25, NULL);
		g_object_set (tcp_sink, "port", 1234, NULL);

		gst_bin_add_many (GST_BIN (pipeline), source, filter, encoder, muxer, tcp_sink, NULL);
		gst_element_link_many (source, filter, encoder, muxer, tcp_sink, NULL);
	} else if (g_strcmp0 (arv_option_sink_type, "smoke_tcp") == 0) {
		GstElement *encoder;
		GstElement *muxer;
		GstElement *tcp_sink;

		encoder = gst_element_factory_make ("smokeenc", "encoder");
		muxer = gst_element_factory_make ("multipartmux", "muxer");
		tcp_sink = gst_element_factory_make ("tcpserversink", "tcpsink");

		g_object_set (tcp_sink, "port", 1234, NULL);

		gst_bin_add_many (GST_BIN (pipeline), source, filter, encoder, muxer, tcp_sink, NULL);
		gst_element_link_many (source, filter, encoder, muxer, tcp_sink, NULL);
	} else {
		GstElement *sink;

		sink = gst_element_factory_make ("xvimagesink", "sink");

		gst_bin_add_many (GST_BIN (pipeline), source, filter, sink, NULL);
		gst_element_link_many (source, filter, sink, NULL);
	}

	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);

	loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (loop);

	gst_object_unref (pipeline);
	g_main_loop_unref (loop);

	return 0;
}
