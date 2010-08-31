#include <arv.h>
#include <stdlib.h>

static gboolean cancel = FALSE;

static void
set_cancel (int signal)
{
	cancel = TRUE;
}

static char *arv_option_camera_name = NULL;
static char *arv_option_debug_domains = NULL;
static gboolean arv_option_snaphot = FALSE;
static gboolean arv_option_auto_buffer = FALSE;
static char *arv_option_trigger = NULL;
static double arv_option_frequency = -1.0;
static int arv_option_width = -1;
static int arv_option_height = -1;
static int arv_option_horizontal_binning = -1;
static int arv_option_vertical_binning = -1;
static double arv_option_exposure_time_us = -1;
static int arv_option_gain = -1;

static const GOptionEntry arv_option_entries[] =
{
	{ "name",		'n', 0, G_OPTION_ARG_STRING,
		&arv_option_camera_name,"Camera name", NULL},
	{ "snapshot",		's', 0, G_OPTION_ARG_NONE,
		&arv_option_snaphot,	"Snapshot", NULL},
	{ "auto",		'a', 0, G_OPTION_ARG_NONE,
		&arv_option_auto_buffer,	"Auto buffer size", NULL},
	{ "frequency", 		'f', 0, G_OPTION_ARG_DOUBLE,
		&arv_option_frequency,	"Acquisition frequency", NULL },
	{ "trigger",		't', 0, G_OPTION_ARG_STRING,
		&arv_option_trigger,	"External trigger", NULL},
	{ "width", 		'\0', 0, G_OPTION_ARG_INT,
		&arv_option_width,		"Width", NULL },
	{ "height", 		'\0', 0, G_OPTION_ARG_INT,
		&arv_option_height, 		"Height", NULL },
	{ "h-binning", 		'\0', 0, G_OPTION_ARG_INT,
		&arv_option_horizontal_binning,"Horizontal binning", NULL },
	{ "v-binning", 		'\0', 0, G_OPTION_ARG_INT,
		&arv_option_vertical_binning, 	"Vertical binning", NULL },
	{ "exposure", 		'e', 0, G_OPTION_ARG_DOUBLE,
		&arv_option_exposure_time_us, 	"Exposure time (µs)", NULL },
	{ "gain", 		'g', 0, G_OPTION_ARG_INT,
		&arv_option_gain,	 	"Gain (dB)", NULL },
	{ "debug", 		'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	"Debug domains", NULL },
	{ NULL }
};

int
main (int argc, char **argv)
{
	ArvCamera *camera;
	ArvStream *stream;
	ArvBuffer *buffer;
	GOptionContext *context;
	GError *error = NULL;
	int i;

	g_thread_init (NULL);
	g_type_init ();

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

	if (arv_option_camera_name == NULL)
		g_print ("Looking for the first available camera\n");
	else
		g_print ("Looking for camera '%s'\n", arv_option_camera_name);

	camera = arv_camera_new (arv_option_camera_name);
	if (camera != NULL) {
		gint payload;
		gint x, y, width, height;
		gint dx, dy;
		double exposure;
		guint64 n_completed_buffers;
		guint64 n_failures;
		guint64 n_underruns;
		int gain;

		arv_camera_set_region (camera, 0, 0, arv_option_width, arv_option_height);
		arv_camera_set_binning (camera, arv_option_horizontal_binning, arv_option_vertical_binning);
		arv_camera_set_exposure_time (camera, arv_option_exposure_time_us);
		arv_camera_set_gain (camera, arv_option_gain);

		arv_camera_get_region (camera, &x, &y, &width, &height);
		arv_camera_get_binning (camera, &dx, &dy);
		exposure = arv_camera_get_exposure_time (camera);
		payload = arv_camera_get_payload (camera);
		gain = arv_camera_get_gain (camera);

		g_printf ("vendor name         = %s\n", arv_camera_get_vendor_name (camera));
		g_printf ("model name          = %s\n", arv_camera_get_model_name (camera));
		g_printf ("device id           = %s\n", arv_camera_get_device_id (camera));
		g_printf ("image width         = %d\n", width);
		g_printf ("image height        = %d\n", height);
		g_printf ("horizontal binning  = %d\n", dx);
		g_printf ("vertical binning    = %d\n", dy);
		g_printf ("exposure            = %g µs\n", exposure);
		g_printf ("gain                = %d dB\n", gain);

		stream = arv_camera_create_stream (camera, NULL, NULL);
		if (arv_option_auto_buffer)
			arv_gv_stream_set_option (ARV_GV_STREAM (stream),
						  ARV_GV_STREAM_OPTION_SOCKET_BUFFER_AUTO,
						  0);

		for (i = 0; i < 200; i++)
			arv_stream_push_buffer (stream, arv_buffer_new (payload, NULL));

		arv_camera_set_acquisition_mode (camera, ARV_ACQUISITION_MODE_CONTINUOUS);

		if (arv_option_frequency > 0.0)
			arv_camera_set_frame_rate (camera, arv_option_frequency);

		if (arv_option_trigger != NULL)
			arv_camera_set_trigger (camera, arv_option_trigger);

		arv_camera_start_acquisition (camera);

		signal (SIGINT, set_cancel);

		do {
			int buffer_count;

			g_usleep (1000000);

			buffer_count = 0;
			do  {
				buffer = arv_stream_pop_buffer (stream);
				if (buffer != NULL) {
					if (buffer->status == ARV_BUFFER_STATUS_SUCCESS)
						buffer_count++;
					/* Image processing here */
					arv_stream_push_buffer (stream, buffer);
				}
			} while (buffer != NULL);

			g_printf ("Frame rate = %d\n", buffer_count);

		} while (!cancel);

		arv_stream_get_statistics (stream, &n_completed_buffers, &n_failures, &n_underruns);

		g_printf ("Completed buffers = %Lu\n", (unsigned long long) n_completed_buffers);
		g_printf ("Failures          = %Lu\n", (unsigned long long) n_failures);
		g_printf ("Underruns         = %Lu\n", (unsigned long long) n_underruns);

		arv_camera_stop_acquisition (camera);

		g_object_unref (stream);
		g_object_unref (camera);
	} else
		g_printf ("No camera found\n");

	return 0;
}
