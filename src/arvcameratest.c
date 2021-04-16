#include <arvdebugprivate.h>
#include <arv.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

static char *arv_option_camera_name = NULL;
static char *arv_option_debug_domains = NULL;
static char *arv_option_trigger = NULL;
static double arv_option_software_trigger = -1;
static double arv_option_frequency = -1.0;
static int arv_option_width = -1;
static int arv_option_height = -1;
static int arv_option_horizontal_binning = -1;
static int arv_option_vertical_binning = -1;
static double arv_option_exposure_time_us = -1;
static int arv_option_gain = -1;
static gboolean arv_option_auto_socket_buffer = FALSE;
static char *arv_option_packet_size_adjustment = NULL;
static gboolean arv_option_no_packet_resend = FALSE;
static double arv_option_packet_request_ratio = -1.0;
static unsigned int arv_option_packet_timeout = 20;
static unsigned int arv_option_frame_retention = 100;
static int arv_option_gv_stream_channel = -1;
static int arv_option_gv_packet_delay = -1;
static int arv_option_gv_packet_size = -1;
static gboolean arv_option_realtime = FALSE;
static gboolean arv_option_high_priority = FALSE;
static gboolean arv_option_no_packet_socket = FALSE;
static char *arv_option_chunks = NULL;
static unsigned int arv_option_bandwidth_limit = -1;
static char *arv_option_register_cache = NULL;
static char *arv_option_range_check = NULL;

/* clang-format off */
static const GOptionEntry arv_option_entries[] =
{
	{
		"name",					'n', 0, G_OPTION_ARG_STRING,
		&arv_option_camera_name,		"Camera name",
		NULL
	},
	{
		"frequency", 				'f', 0, G_OPTION_ARG_DOUBLE,
		&arv_option_frequency,			"Acquisition frequency",
		NULL
	},
	{
		"trigger",				't', 0, G_OPTION_ARG_STRING,
		&arv_option_trigger,			"External trigger",
		NULL
	},
	{
		"software-trigger",			'o', 0, G_OPTION_ARG_DOUBLE,
		&arv_option_software_trigger,		"Emit software trigger",
		NULL
	},
	{
		"width", 				'w', 0, G_OPTION_ARG_INT,
		&arv_option_width,			"Width",
		NULL
	},
	{
		"height", 				'h', 0, G_OPTION_ARG_INT,
		&arv_option_height, 			"Height",
		NULL
	},
	{
	       "h-binning", 				'\0', 0, G_OPTION_ARG_INT,
		&arv_option_horizontal_binning,		"Horizontal binning",
		NULL
	},
	{
		"v-binning", 				'\0', 0, G_OPTION_ARG_INT,
		&arv_option_vertical_binning, 		"Vertical binning",
		NULL
	},
	{
		"exposure", 				'e', 0, G_OPTION_ARG_DOUBLE,
		&arv_option_exposure_time_us, 		"Exposure time (us)",
		NULL
	},
	{
		"gain", 				'g', 0, G_OPTION_ARG_INT,
		&arv_option_gain,	 		"Gain (dB)",
		NULL
	},
	{
		"auto",					'a', 0, G_OPTION_ARG_NONE,
		&arv_option_auto_socket_buffer,		"Auto socket buffer size",
		NULL
	},
	{
		"packet-size-adjustment",		'j', 0, G_OPTION_ARG_STRING,
		&arv_option_packet_size_adjustment,	"Packet size adjustment",
		"{never|always|once|on-failure|on-failure-once}"
	},
	{
		"no-packet-resend",			'r', 0, G_OPTION_ARG_NONE,
		&arv_option_no_packet_resend,		"No packet resend",
		NULL
	},
	{
		"packet-request-ratio",			'q', 0, G_OPTION_ARG_DOUBLE,
		&arv_option_packet_request_ratio,
		"Packet resend request limit as a frame packet number ratio [0..2.0]",
		NULL
	},
	{
		"packet-timeout", 			'p', 0, G_OPTION_ARG_INT,
		&arv_option_packet_timeout, 		"Packet timeout (ms)",
		NULL
	},
	{
		"frame-retention", 			'm', 0, G_OPTION_ARG_INT,
		&arv_option_frame_retention, 		"Frame retention (ms)",
		NULL
	},
	{
		"gv-stream-channel",			'c', 0, G_OPTION_ARG_INT,
		&arv_option_gv_stream_channel,		"GigEVision stream channel id",
		NULL
	},
	{
		"gv-packet-delay",			'y', 0, G_OPTION_ARG_INT,
		&arv_option_gv_packet_delay,		"GigEVision packet delay (ns)",
		NULL
	},
	{
		"gv-packet-size",			'i', 0, G_OPTION_ARG_INT,
		&arv_option_gv_packet_size,		"GigEVision packet size (bytes)",
		NULL
	},
	{
		"chunks", 				'u', 0, G_OPTION_ARG_STRING,
		&arv_option_chunks,	 		"Chunks",
		NULL
	},
	{
		"realtime",				'\0', 0, G_OPTION_ARG_NONE,
		&arv_option_realtime,			"Make stream thread realtime",
		NULL
	},
	{
		"high-priority",			'\0', 0, G_OPTION_ARG_NONE,
		&arv_option_high_priority,		"Make stream thread high priority",
		NULL
	},
	{
		"no-packet-socket",			'\0', 0, G_OPTION_ARG_NONE,
		&arv_option_no_packet_socket,		"Disable use of packet socket",
		NULL
	},
	{
		"register-cache",			'\0', 0, G_OPTION_ARG_STRING,
		&arv_option_register_cache,		"Register cache policy",
		"{disable|enable|debug}"
	},
	{
		"range-check",				'\0', 0, G_OPTION_ARG_STRING,
		&arv_option_range_check,		"Range check policy",
		"{disable|enable}"
	},
	{
		"bandwidth-limit",			'b', 0, G_OPTION_ARG_INT,
		&arv_option_bandwidth_limit,		"Desired USB3 Vision device bandwidth limit",
		NULL
	},
	{
		"debug", 				'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 		NULL,
		"{<category>[:<level>][,...]|help}"
	},
	{ NULL }
};
/* clang-format on */

typedef struct {
	GMainLoop *main_loop;

	int buffer_count;
	int error_count;
	size_t transferred;

	ArvChunkParser *chunk_parser;
	char **chunks;
} ApplicationData;

static gboolean cancel = FALSE;

static void
set_cancel (int signal)
{
	cancel = TRUE;
}

static void
new_buffer_cb (ArvStream *stream, ApplicationData *data)
{
	ArvBuffer *buffer;

	buffer = arv_stream_try_pop_buffer (stream);
	if (buffer != NULL) {
		if (arv_buffer_get_status (buffer) == ARV_BUFFER_STATUS_SUCCESS) {
			size_t size = 0;
			data->buffer_count++;
			arv_buffer_get_data (buffer, &size);
			data->transferred += size;
		} else {
			data->error_count++;
		}

		if (arv_buffer_has_chunks (buffer) && data->chunks != NULL) {
			int i;

			for (i = 0; data->chunks[i] != NULL; i++) {
				gint64 integer_value;
				GError *error = NULL;

				integer_value = arv_chunk_parser_get_integer_value (data->chunk_parser, buffer, data->chunks[i], &error);
				if (error == NULL)
					g_print ("%s = %" G_GINT64_FORMAT "\n", data->chunks[i], integer_value);
				else {
					double float_value;

					g_clear_error (&error);
					float_value = arv_chunk_parser_get_float_value (data->chunk_parser, buffer, data->chunks[i], &error);
					if (error == NULL)
						g_print ("%s = %g\n", data->chunks[i], float_value);
					else
						g_clear_error (&error);
				}
			}
		}

		/* Image processing here */

		arv_stream_push_buffer (stream, buffer);
	}
}

static void
stream_cb (void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer)
{
	if (type == ARV_STREAM_CALLBACK_TYPE_INIT) {
		if (arv_option_realtime) {
			if (!arv_make_thread_realtime (10))
				printf ("Failed to make stream thread realtime\n");
		} else if (arv_option_high_priority) {
			if (!arv_make_thread_high_priority (-10))
				printf ("Failed to make stream thread high priority\n");
		}
	}
}

static gboolean
periodic_task_cb (void *abstract_data)
{
	ApplicationData *data = abstract_data;

	printf ("%3d frame%s - %7.3g MiB/s",
		data->buffer_count,
		data->buffer_count > 1 ? "s/s" : "/s ",
		(double) data->transferred / 1e6);
	if (data->error_count > 0)
		printf (" - %d error%s\n", data->error_count, data->error_count > 1 ? "s" : "");
	else
		printf ("\n");
	data->buffer_count = 0;
	data->error_count = 0;
	data->transferred = 0;

	if (cancel) {
		g_main_loop_quit (data->main_loop);
		return FALSE;
	}

	return TRUE;
}

static gboolean
emit_software_trigger (void *abstract_data)
{
	ArvCamera *camera = abstract_data;

	arv_camera_software_trigger (camera, NULL);

	return TRUE;
}

static void
control_lost_cb (ArvGvDevice *gv_device)
{
	printf ("Control lost\n");

	cancel = TRUE;
}

int
main (int argc, char **argv)
{
	ApplicationData data;
	ArvCamera *camera;
	ArvStream *stream;
	ArvRegisterCachePolicy register_cache_policy;
	ArvRangeCheckPolicy range_check_policy;
	ArvGvPacketSizeAdjustment adjustment;
	GOptionContext *context;
	GError *error = NULL;
	int i;

	data.buffer_count = 0;
	data.error_count = 0;
	data.transferred = 0;
	data.chunks = NULL;
	data.chunk_parser = NULL;

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, arv_option_entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	if (arv_option_register_cache == NULL)
		register_cache_policy = ARV_REGISTER_CACHE_POLICY_DEFAULT;
	else if (g_strcmp0 (arv_option_register_cache, "disable") == 0)
		register_cache_policy = ARV_REGISTER_CACHE_POLICY_DISABLE;
	else if (g_strcmp0 (arv_option_register_cache, "enable") == 0)
		register_cache_policy = ARV_REGISTER_CACHE_POLICY_ENABLE;
	else if (g_strcmp0 (arv_option_register_cache, "debug") == 0)
		register_cache_policy = ARV_REGISTER_CACHE_POLICY_DEBUG;
	else {
		printf ("Invalid register cache policy\n");
		return EXIT_FAILURE;
	}

	if (arv_option_range_check == NULL)
		range_check_policy = ARV_RANGE_CHECK_POLICY_DEFAULT;
	else if (g_strcmp0 (arv_option_range_check, "disable") == 0)
		range_check_policy = ARV_RANGE_CHECK_POLICY_DISABLE;
	else if (g_strcmp0 (arv_option_range_check, "enable") == 0)
		range_check_policy = ARV_RANGE_CHECK_POLICY_ENABLE;
	else if (g_strcmp0 (arv_option_range_check, "debug") == 0)
		range_check_policy = ARV_RANGE_CHECK_POLICY_DEBUG;
	else {
		printf ("Invalid range check policy\n");
		return EXIT_FAILURE;
	}

	if (arv_option_packet_size_adjustment == NULL)
		adjustment = ARV_GV_PACKET_SIZE_ADJUSTMENT_DEFAULT;
	else if (g_strcmp0 (arv_option_packet_size_adjustment, "always") == 0)
		adjustment = ARV_GV_PACKET_SIZE_ADJUSTMENT_ALWAYS;
	else if (g_strcmp0 (arv_option_packet_size_adjustment, "never") == 0)
		adjustment = ARV_GV_PACKET_SIZE_ADJUSTMENT_NEVER;
	else if (g_strcmp0 (arv_option_packet_size_adjustment, "once") == 0)
		adjustment = ARV_GV_PACKET_SIZE_ADJUSTMENT_ONCE;
	else if (g_strcmp0 (arv_option_packet_size_adjustment, "on-failure") == 0)
		adjustment = ARV_GV_PACKET_SIZE_ADJUSTMENT_ON_FAILURE;
	else if (g_strcmp0 (arv_option_packet_size_adjustment, "on-failure-once") == 0)
		adjustment = ARV_GV_PACKET_SIZE_ADJUSTMENT_ON_FAILURE_ONCE;
	else {
		printf ("Invalid GigEVision packet size adjustment\n");
		return EXIT_FAILURE;
	}

	if (!arv_debug_enable (arv_option_debug_domains)) {
		if (g_strcmp0 (arv_option_debug_domains, "help") != 0)
			printf ("Invalid debug selection\n");
		else
			arv_debug_print_infos ();
		return EXIT_FAILURE;
	}

	arv_enable_interface ("Fake");

	arv_debug_enable (arv_option_debug_domains);

	if (arv_option_camera_name == NULL)
		g_print ("Looking for the first available camera\n");
	else
		g_print ("Looking for camera '%s'\n", arv_option_camera_name);

	camera = arv_camera_new (arv_option_camera_name, &error);
	if (camera != NULL) {
		void (*old_sigint_handler)(int);
		gint payload;
		gint x, y, width, height;
		gint dx, dy;
		double exposure;
		guint64 n_completed_buffers;
		guint64 n_failures;
		guint64 n_underruns;
		int gain;
		guint software_trigger_source = 0;

		arv_camera_set_register_cache_policy (camera, register_cache_policy);
		arv_camera_set_range_check_policy (camera, range_check_policy);

		if (arv_option_chunks != NULL) {
			char *striped_chunks;

			striped_chunks = g_strdup (arv_option_chunks);
			arv_str_strip (striped_chunks, " ,:;", ',');
			data.chunks = g_strsplit_set (striped_chunks, ",", -1);
			g_free (striped_chunks);

			data.chunk_parser = arv_camera_create_chunk_parser (camera);

			for (i = 0; data.chunks[i] != NULL; i++) {
				char *chunk = g_strdup_printf ("Chunk%s", data.chunks[i]);

				g_free (data.chunks[i]);
				data.chunks[i] = chunk;
			}
		}

		arv_camera_set_chunks (camera, arv_option_chunks, NULL);
		arv_camera_set_region (camera, 0, 0, arv_option_width, arv_option_height, NULL);
		arv_camera_set_binning (camera, arv_option_horizontal_binning, arv_option_vertical_binning, NULL);
		arv_camera_set_exposure_time (camera, arv_option_exposure_time_us, NULL);
		arv_camera_set_gain (camera, arv_option_gain, NULL);

		if (arv_camera_is_uv_device(camera)) {
			arv_camera_uv_set_bandwidth (camera, arv_option_bandwidth_limit, NULL);
		}

		if (arv_camera_is_gv_device (camera)) {
			arv_camera_gv_select_stream_channel (camera, arv_option_gv_stream_channel, NULL);
			arv_camera_gv_set_packet_delay (camera, arv_option_gv_packet_delay, NULL);
			arv_camera_gv_set_packet_size (camera, arv_option_gv_packet_size, NULL);
			arv_camera_gv_set_stream_options (camera, arv_option_no_packet_socket ?
							  ARV_GV_STREAM_OPTION_PACKET_SOCKET_DISABLED :
							  ARV_GV_STREAM_OPTION_NONE);
			if (arv_option_packet_size_adjustment != NULL)
				arv_camera_gv_set_packet_size_adjustment (camera, adjustment);
		}

		arv_camera_get_region (camera, &x, &y, &width, &height, NULL);
		arv_camera_get_binning (camera, &dx, &dy, NULL);
		exposure = arv_camera_get_exposure_time (camera, NULL);
		payload = arv_camera_get_payload (camera, NULL);
		gain = arv_camera_get_gain (camera, NULL);

		printf ("vendor name           = %s\n", arv_camera_get_vendor_name (camera, NULL));
		printf ("model name            = %s\n", arv_camera_get_model_name (camera, NULL));
		printf ("device id             = %s\n", arv_camera_get_device_id (camera, NULL));
		printf ("image width           = %d\n", width);
		printf ("image height          = %d\n", height);
		printf ("horizontal binning    = %d\n", dx);
		printf ("vertical binning      = %d\n", dy);
		printf ("payload               = %d bytes\n", payload);
		printf ("exposure              = %g µs\n", exposure);
		printf ("gain                  = %d dB\n", gain);

		if (arv_camera_is_uv_device (camera)) {
			guint min,max;

			arv_camera_uv_get_bandwidth_bounds (camera, &min, &max, NULL);
			printf ("uv bandwidth limit    = %d [%d..%d]\n", arv_camera_uv_get_bandwidth (camera, NULL), min, max);
		}

		stream = arv_camera_create_stream (camera, stream_cb, NULL, &error);

		if (arv_camera_is_gv_device (camera)) {
			printf ("gv n_stream channels  = %d\n", arv_camera_gv_get_n_stream_channels (camera, NULL));
			printf ("gv current channel    = %d\n",
				arv_camera_gv_get_current_stream_channel (camera, NULL));
			g_print ("gv packet delay       = %" G_GINT64_FORMAT " ns\n",
				 arv_camera_gv_get_packet_delay (camera, NULL));
			printf ("gv packet size        = %d bytes\n", arv_camera_gv_get_packet_size (camera, NULL));
		}

		if (ARV_IS_STREAM (stream)) {
			if (ARV_IS_GV_STREAM (stream)) {
				if (arv_option_auto_socket_buffer)
					g_object_set (stream,
						      "socket-buffer", ARV_GV_STREAM_SOCKET_BUFFER_AUTO,
						      "socket-buffer-size", 0,
						      NULL);
				if (arv_option_no_packet_resend)
					g_object_set (stream,
						      "packet-resend", ARV_GV_STREAM_PACKET_RESEND_NEVER,
						      NULL);
				if (arv_option_packet_request_ratio >= 0.0)
					g_object_set (stream,
						      "packet-request-ratio", arv_option_packet_request_ratio,
						      NULL);

				g_object_set (stream,
					      "packet-timeout", (unsigned) arv_option_packet_timeout * 1000,
					      "frame-retention", (unsigned) arv_option_frame_retention * 1000,
					      NULL);
			}

			for (i = 0; i < 50; i++)
				arv_stream_push_buffer (stream, arv_buffer_new (payload, NULL));

			arv_camera_set_acquisition_mode (camera, ARV_ACQUISITION_MODE_CONTINUOUS, NULL);

			if (arv_option_frequency > 0.0)
				arv_camera_set_frame_rate (camera, arv_option_frequency, NULL);

			if (arv_option_trigger != NULL)
				arv_camera_set_trigger (camera, arv_option_trigger, NULL);

			if (arv_option_software_trigger > 0.0) {
				arv_camera_set_trigger (camera, "Software", NULL);
				software_trigger_source = g_timeout_add ((double) (0.5 + 1000.0 /
										   arv_option_software_trigger),
									 emit_software_trigger, camera);
			}

			arv_camera_start_acquisition (camera, NULL);

			g_signal_connect (stream, "new-buffer", G_CALLBACK (new_buffer_cb), &data);
			arv_stream_set_emit_signals (stream, TRUE);

			g_signal_connect (arv_camera_get_device (camera), "control-lost",
					  G_CALLBACK (control_lost_cb), NULL);

			g_timeout_add (1000, periodic_task_cb, &data);

			data.main_loop = g_main_loop_new (NULL, FALSE);

			old_sigint_handler = signal (SIGINT, set_cancel);

			g_main_loop_run (data.main_loop);

			if (software_trigger_source > 0)
				g_source_remove (software_trigger_source);

			signal (SIGINT, old_sigint_handler);

			g_main_loop_unref (data.main_loop);

			arv_stream_get_statistics (stream, &n_completed_buffers, &n_failures, &n_underruns);

			g_print ("Completed buffers = %" G_GUINT64_FORMAT "\n", n_completed_buffers);
			g_print ("Failures          = %" G_GUINT64_FORMAT "\n", n_failures);
			g_print ("Underruns         = %" G_GUINT64_FORMAT "\n", n_underruns);

			arv_camera_stop_acquisition (camera, NULL);

			arv_stream_set_emit_signals (stream, FALSE);

			g_object_unref (stream);
		} else {
			printf ("Can't create stream thread%s%s\n",
				error != NULL ? ": " : "",
				error != NULL ? error->message : "");

			g_clear_error (&error);
		}

		g_object_unref (camera);
	} else {
		printf ("No camera found%s%s\n",
			error != NULL ? ": " : "",
			error != NULL ? error->message : "");
		g_clear_error (&error);
	}

	if (data.chunks != NULL)
		g_strfreev (data.chunks);

	g_clear_object (&data.chunk_parser);

	return 0;
}
