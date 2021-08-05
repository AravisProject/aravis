#include <arv.h>
#include <stdio.h>
#include <stdlib.h>

static gboolean cancel = FALSE;

static void
set_cancel (int signal)
{
	cancel = TRUE;
}

static char *arv_option_camera_name = NULL;
static char *arv_option_feature_name = "Gain";
static int arv_option_min = 0;
static int arv_option_max = 10;
static char *arv_option_debug_domains = "device";

static const GOptionEntry arv_option_entries[] =
{
	{
		"name",					'n', 0, G_OPTION_ARG_STRING,
		&arv_option_camera_name,		"Camera name", NULL
	},
	{
		"feature",				'f', 0, G_OPTION_ARG_STRING,
		&arv_option_feature_name,		"Feature name", NULL
	},
	{
		"max", 					'a', 0, G_OPTION_ARG_INT,
		&arv_option_max,			"Max", NULL
	},
	{
		"min", 					'i', 0, G_OPTION_ARG_INT,
		&arv_option_min, 			"Min", NULL
	},
	{
		"debug", 				'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 		"Debug domains", NULL
	},
	{ NULL }
};

int main(int argc, char *argv[])
{
    ArvDevice *device;
    ArvStream *stream;
    ArvCamera *camera;
    ArvGcFeatureNode *feature;
    guint64 n_completed_buffers;
    guint64 n_failures;
    guint64 n_underruns;
    GOptionContext *context;
    GError *error = NULL;
    void (*old_sigint_handler)(int);
    int i, payload;

    context = g_option_context_new (NULL);
    g_option_context_set_summary (context, "Test of heartbeat robustness while continuously changing a feature.");
    g_option_context_add_main_entries (context, arv_option_entries, NULL);

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
	    g_option_context_free (context);
	    g_print ("Option parsing failed: %s\n", error->message);
	    g_error_free (error);
	    return EXIT_FAILURE;
    }

    g_option_context_free (context);

    arv_debug_enable (arv_option_debug_domains);

    camera = arv_camera_new (arv_option_camera_name, &error);
    if (!ARV_IS_CAMERA (camera)) {
	    printf ("Device not found%s%s\n",
		    error != NULL ? ": " : "",
		    error != NULL ? error->message : "");
	    g_clear_error (&error);

	    return EXIT_FAILURE;
    }

    device = arv_camera_get_device (camera);

    stream = arv_camera_create_stream (camera, NULL, NULL, &error);
    if (!ARV_IS_STREAM (stream)) {
	    printf ("Invalid device%s%s\n",
		    error != NULL ? ": " : "",
		    error != NULL ? error->message : "");
	    g_clear_error (&error);
	    g_clear_object (&camera);

	    return EXIT_FAILURE;
    } else {
	    payload = arv_camera_get_payload (camera, NULL);

	    if (ARV_IS_GV_STREAM (stream)) {
		    g_object_set (stream,
				  //"socket-buffer", ARV_GV_STREAM_SOCKET_BUFFER_AUTO,
				  "socket-buffer", ARV_GV_STREAM_SOCKET_BUFFER_FIXED,
				  "socket-buffer-size", payload*6,
				  "packet-timeout", 1000 * 1000,
				  "frame-retention", 100 * 1000,
				  "packet-resend", ARV_GV_STREAM_PACKET_RESEND_ALWAYS,
				  NULL);
	    }

	    for (i = 0; i < 100; i++)
		    arv_stream_push_buffer(stream, arv_buffer_new(payload, NULL));

	    arv_camera_set_acquisition_mode(camera, ARV_ACQUISITION_MODE_CONTINUOUS, NULL);

	    feature = ARV_GC_FEATURE_NODE (arv_device_get_feature (device, arv_option_feature_name));

	    arv_camera_start_acquisition (camera, NULL);

	    old_sigint_handler = signal (SIGINT, set_cancel);

	    while (!cancel) {
		    ArvBuffer *buffer = arv_stream_timeout_pop_buffer(stream, 2000000);
		    if (buffer) {
			    g_usleep(10);
			    arv_stream_push_buffer (stream, buffer);
		    }

		    if (!(++i%5)) {
			    char *value;

			    if ((i/100) % 2 == 0)
				    value = g_strdup_printf ("%d", arv_option_min);
			    else
				    value = g_strdup_printf ("%d", arv_option_max);

			    fprintf (stderr, "Setting %s from %s to %s\n",
				     arv_option_feature_name,
				     arv_gc_feature_node_get_value_as_string (feature, NULL),
				     value);
			    arv_gc_feature_node_set_value_from_string (feature, value, NULL);

			    g_free (value);
		    }
	    }

	    signal (SIGINT, old_sigint_handler);

	    arv_stream_get_statistics (stream, &n_completed_buffers, &n_failures, &n_underruns);

	    g_print ("\nCompleted buffers = %" G_GUINT64_FORMAT "\n", n_completed_buffers);
	    g_print ("Failures          = %" G_GUINT64_FORMAT "\n", n_failures);
	    g_print ("Underruns         = %" G_GUINT64_FORMAT "\n", n_underruns);

	    arv_camera_stop_acquisition (camera, NULL);
    }

    g_object_unref (camera);

    return 0;
}
