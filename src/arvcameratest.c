#include <arvdebugprivate.h>
#include <arvgvstreamprivate.h>
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
static char *arv_option_features = NULL;
static gboolean arv_option_auto_socket_buffer = FALSE;
static char *arv_option_packet_size_adjustment = NULL;
static gboolean arv_option_no_packet_resend = FALSE;
static double arv_option_packet_request_ratio = -1.0;
static unsigned int arv_option_initial_packet_timeout = ARV_GV_STREAM_INITIAL_PACKET_TIMEOUT_US_DEFAULT / 1000;
static unsigned int arv_option_packet_timeout = ARV_GV_STREAM_PACKET_TIMEOUT_US_DEFAULT / 1000;
static unsigned int arv_option_frame_retention = ARV_GV_STREAM_FRAME_RETENTION_US_DEFAULT / 1000;
static int arv_option_gv_stream_channel = -1;
static int arv_option_gv_packet_delay = -1;
static int arv_option_gv_packet_size = -1;
static gboolean arv_option_realtime = FALSE;
static gboolean arv_option_high_priority = FALSE;
static gboolean arv_option_no_packet_socket = FALSE;
static gboolean arv_option_multipart = FALSE;
static char *arv_option_chunks = NULL;
static int arv_option_bandwidth_limit = -1;
static char *arv_option_register_cache = NULL;
static char *arv_option_range_check = NULL;
static char *arv_option_access_check = NULL;
static int arv_option_duration_s = -1;
static char *arv_option_uv_usb_mode = NULL;
static gboolean arv_option_show_version = FALSE;
static gboolean arv_option_gv_allow_broadcast_discovery_ack = FALSE;

/* clang-format off */
static const GOptionEntry arv_option_entries[] =
{
	{
		"name",					'n', 0, G_OPTION_ARG_STRING,
		&arv_option_camera_name,		"Camera name",
		"<camera_id>"
	},
	{
		"frequency", 				'f', 0, G_OPTION_ARG_DOUBLE,
		&arv_option_frequency,			"Acquisition frequency",
		"<Hz>"
	},
	{
		"trigger",				't', 0, G_OPTION_ARG_STRING,
		&arv_option_trigger,			"External trigger",
		"<trigger_id>"
	},
	{
		"software-trigger",			'o', 0, G_OPTION_ARG_DOUBLE,
		&arv_option_software_trigger,		"Emit software trigger",
		"<Hz>"
	},
	{
		"width", 				'w', 0, G_OPTION_ARG_INT,
		&arv_option_width,			"Width",
		"<n_pixels>"
	},
	{
		"height", 				'h', 0, G_OPTION_ARG_INT,
		&arv_option_height, 			"Height",
		"<n_pixels>"
	},
	{
	       "h-binning", 				'\0', 0, G_OPTION_ARG_INT,
		&arv_option_horizontal_binning,		"Horizontal binning",
		"<n_pixels>"
	},
	{
		"v-binning", 				'\0', 0, G_OPTION_ARG_INT,
		&arv_option_vertical_binning, 		"Vertical binning",
		"<n_pixels>"
	},
	{
		"exposure", 				'e', 0, G_OPTION_ARG_DOUBLE,
		&arv_option_exposure_time_us, 		"Exposure time",
		"<time_us>"
	},
	{
		"gain", 				'g', 0, G_OPTION_ARG_INT,
		&arv_option_gain,	 		"Gain (dB)",
		"<dB>"
	},
	{
		"auto",					'a', 0, G_OPTION_ARG_NONE,
		&arv_option_auto_socket_buffer,		"Auto socket buffer size",
		NULL
	},
	{
		"features",				'\0', 0, G_OPTION_ARG_STRING,
		&arv_option_features,		        "Additional configuration as a space separated list of features",
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
		"Packet resend request limit as a frame packet ratio",
		"[0..2.0]"
	},
	{
		"initial-packet-timeout", 		'l', 0, G_OPTION_ARG_INT,
		&arv_option_initial_packet_timeout, 	"Initial packet timeout",
		"<ms>"
	},
	{
		"packet-timeout", 			'p', 0, G_OPTION_ARG_INT,
		&arv_option_packet_timeout, 		"Packet timeout",
		"<ms>"
	},
	{
		"frame-retention", 			'm', 0, G_OPTION_ARG_INT,
		&arv_option_frame_retention, 		"Frame retention",
	        "<ms>"
	},
	{
		"gv-stream-channel",			'c', 0, G_OPTION_ARG_INT,
		&arv_option_gv_stream_channel,		"GigEVision stream channel id",
		"<id>"
	},
	{
		"gv-packet-delay",			'y', 0, G_OPTION_ARG_INT,
		&arv_option_gv_packet_delay,		"GigEVision packet delay",
		"<ns>"
	},
	{
		"gv-packet-size",			'i', 0, G_OPTION_ARG_INT,
		&arv_option_gv_packet_size,		"GigEVision packet size",
		"<n_bytes>"
	},
	{
		"usb-mode",				's', 0, G_OPTION_ARG_STRING,
		&arv_option_uv_usb_mode,		"USB device I/O mode",
		"{sync|async}"
	},
	{
		"chunks", 				'u', 0, G_OPTION_ARG_STRING,
		&arv_option_chunks,	 		"Chunks",
		"<chunk_id>[,<chunk_id>[...]]"
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
		"multipart",    			'\0', 0, G_OPTION_ARG_NONE,
		&arv_option_multipart,		"Enable multipart payload",
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
		"{disable|enable|debug}"
	},
	{
		"access-check",			        '\0', 0, G_OPTION_ARG_STRING,
		&arv_option_access_check,	        "Feature access check policy",
		"{disable|enable}"
	},
	{
		"bandwidth-limit",			'b', 0, G_OPTION_ARG_INT,
		&arv_option_bandwidth_limit,		"Desired USB3 Vision device bandwidth limit",
		"<limit>"
	},
	{
		"duration",	        		'\0', 0, G_OPTION_ARG_INT,
		&arv_option_duration_s,		        "Test duration (s)",
		"<s>"
	},
	{
		"gv-allow-broadcast-discovery-ack",     '\0', 0, G_OPTION_ARG_NONE,
		&arv_option_gv_allow_broadcast_discovery_ack,
                "Allow broadcast discovery ack",
		NULL
	},
	{
		"debug", 				'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 		"Debug output selection",
		"{<category>[:<level>][,...]|help}"
	},
	{
		"version", 			        'v', 0, G_OPTION_ARG_NONE,
		&arv_option_show_version,     	        "Show version",
                NULL
	},
	{ NULL }
};
/* clang-format on */

static const char
description_content[] =
"This tool configures a camera and starts video streaming, infinitely unless a duration is given.";

typedef struct {
	GMainLoop *main_loop;

	int buffer_count;
	int error_count;
	size_t transferred;

	ArvChunkParser *chunk_parser;
	char **chunks;

        gint64 start_time;
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

				integer_value = arv_chunk_parser_get_integer_value (data->chunk_parser,
                                                                                    buffer, data->chunks[i], &error);
				if (error == NULL)
					g_print ("%s = %" G_GINT64_FORMAT "\n", data->chunks[i], integer_value);
				else {
					double float_value;

					g_clear_error (&error);
					float_value = arv_chunk_parser_get_float_value (data->chunk_parser,
                                                                                        buffer, data->chunks[i], &error);
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

	if (cancel ||
            (arv_option_duration_s > 0 &&
             (g_get_monotonic_time() - data->start_time) > 1000000 * arv_option_duration_s)) {
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
        ArvAccessCheckPolicy access_check_policy;
	ArvGvPacketSizeAdjustment adjustment;
	ArvUvUsbMode usb_mode;
	GOptionContext *context;
	GError *error = NULL;
	int i;

	data.buffer_count = 0;
	data.error_count = 0;
	data.transferred = 0;
	data.chunks = NULL;
	data.chunk_parser = NULL;

	context = g_option_context_new (NULL);
	g_option_context_set_summary (context, "Small utility for basic device checks.");
	g_option_context_set_description (context, description_content);
	g_option_context_add_main_entries (context, arv_option_entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

        if (arv_option_show_version) {
                printf ("%u.%u.%u\n",
                        arv_get_major_version (),
                        arv_get_minor_version (),
                        arv_get_micro_version ());
                return EXIT_SUCCESS;
        }

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

	if (arv_option_access_check == NULL)
		access_check_policy = ARV_ACCESS_CHECK_POLICY_DEFAULT;
	else if (g_strcmp0 (arv_option_access_check, "disable") == 0)
		access_check_policy = ARV_ACCESS_CHECK_POLICY_DISABLE;
	else if (g_strcmp0 (arv_option_access_check, "enable") == 0)
		access_check_policy = ARV_ACCESS_CHECK_POLICY_ENABLE;
	else {
		printf ("Invalid access check policy\n");
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

	if (arv_option_uv_usb_mode == NULL)
		usb_mode = ARV_UV_USB_MODE_DEFAULT;
	else if (g_strcmp0 (arv_option_uv_usb_mode, "sync") == 0)
		usb_mode = ARV_UV_USB_MODE_SYNC;
	else if (g_strcmp0 (arv_option_uv_usb_mode, "async") == 0)
		usb_mode = ARV_UV_USB_MODE_ASYNC;
	else {
		printf ("Invalid USB device I/O mode\n");
		return EXIT_FAILURE;
	}

	if (!arv_debug_enable (arv_option_debug_domains)) {
		if (g_strcmp0 (arv_option_debug_domains, "help") != 0)
			printf ("Invalid debug selection\n");
		else
			arv_debug_print_infos ();
		return EXIT_FAILURE;
	}

        if (arv_option_gv_allow_broadcast_discovery_ack)
                arv_set_interface_flags ("GigEVision", ARV_GV_INTERFACE_FLAGS_ALLOW_BROADCAST_DISCOVERY_ACK);

	arv_enable_interface ("Fake");

	arv_debug_enable (arv_option_debug_domains);

    #ifdef G_OS_WIN32
        setbuf(stderr,NULL);
        setbuf(stdout,NULL);
    #endif

	if (arv_option_camera_name == NULL)
		g_print ("Looking for the first available camera\n");
	else
		g_print ("Looking for camera '%s'\n", arv_option_camera_name);

	camera = arv_camera_new (arv_option_camera_name, &error);
	if (camera != NULL) {
		const char *vendor_name =NULL;
		const char *model_name = NULL;
		const char *serial_number = NULL;
		void (*old_sigint_handler)(int);
		gint payload = 0;
		gint width, height;
		gint dx, dy;
		double exposure = 0;
		guint64 n_completed_buffers;
		guint64 n_failures;
		guint64 n_underruns;
		guint uv_bandwidth = 0;
		guint min, max;
		guint gv_n_channels = 0;
                guint gv_channel_id = 0;
                guint gv_packet_delay = 0;
		int gain = 0;
		guint software_trigger_source = 0;
		gboolean success = TRUE;

		arv_camera_set_register_cache_policy (camera, register_cache_policy);
		arv_camera_set_range_check_policy (camera, range_check_policy);
                arv_camera_set_access_check_policy (camera, access_check_policy);

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

		error = NULL;

		if (error == NULL && arv_camera_are_chunks_available (camera, NULL))
			arv_camera_set_chunks (camera, arv_option_chunks, &error);
		if (error == NULL) arv_camera_set_region (camera, -1, -1, arv_option_width, arv_option_height, &error);
		if (error == NULL) arv_camera_set_binning (camera, arv_option_horizontal_binning,
							   arv_option_vertical_binning, &error);
		if (error == NULL) arv_camera_set_exposure_time (camera, arv_option_exposure_time_us, &error);
		if (error == NULL) arv_camera_set_gain (camera, arv_option_gain, &error);

		if (arv_camera_is_uv_device (camera)) {
			if (error == NULL)
                                arv_camera_uv_set_usb_mode (camera, usb_mode);
			if (error == NULL && arv_option_bandwidth_limit >= 0)
                                        arv_camera_uv_set_bandwidth (camera, arv_option_bandwidth_limit, &error);
		}

		if (arv_camera_is_gv_device (camera)) {
			if (error == NULL) arv_camera_gv_select_stream_channel (camera, arv_option_gv_stream_channel, &error);
			if (error == NULL) arv_camera_gv_set_packet_delay (camera, arv_option_gv_packet_delay, &error);
			if (error == NULL) arv_camera_gv_set_packet_size (camera, arv_option_gv_packet_size, &error);
                        arv_camera_gv_set_stream_options (camera, arv_option_no_packet_socket ?
                                                          ARV_GV_STREAM_OPTION_PACKET_SOCKET_DISABLED :
                                                          ARV_GV_STREAM_OPTION_NONE);
                        if (arv_option_packet_size_adjustment != NULL)
                                arv_camera_gv_set_packet_size_adjustment (camera, adjustment);
                        if (error == NULL) arv_camera_gv_set_multipart (camera, TRUE,
                                                                        arv_option_multipart ? &error : NULL);
                }

                if (error == NULL && arv_option_features != NULL)
                        arv_device_set_features_from_string (arv_camera_get_device (camera), arv_option_features, &error);

		if (error != NULL) {
			printf ("Failed to configure the device: %s\n", error->message);
			g_clear_error (&error);
			success = FALSE;
		}

		if (success) {
			if (error == NULL) vendor_name = arv_camera_get_vendor_name (camera, &error);
			if (error == NULL) model_name = arv_camera_get_model_name (camera, &error);
			if (error == NULL) serial_number = arv_camera_get_device_serial_number (camera, &error);

			if (error == NULL) arv_camera_get_region (camera, NULL, NULL, &width, &height, &error);
			if (error == NULL && arv_camera_is_binning_available (camera, NULL))
				arv_camera_get_binning (camera, &dx, &dy, &error);
			if (error == NULL && arv_camera_is_exposure_time_available (camera, NULL))
				exposure = arv_camera_get_exposure_time (camera, &error);
			if (error == NULL && arv_camera_is_gain_available (camera, NULL))
				gain = arv_camera_get_gain (camera, &error);
			if (error == NULL) payload = arv_camera_get_payload (camera, &error);

			if (arv_camera_is_uv_device (camera)) {
				if (arv_camera_uv_is_bandwidth_control_available (camera, NULL)) {
					if (error == NULL) arv_camera_uv_get_bandwidth_bounds (camera, &min, &max, &error);
					if (error == NULL) uv_bandwidth = arv_camera_uv_get_bandwidth (camera, &error);
				}
			}

			if (arv_camera_is_gv_device (camera)) {
				if (error == NULL) gv_n_channels = arv_camera_gv_get_n_stream_channels (camera, &error);
				if (error == NULL) gv_channel_id = arv_camera_gv_get_current_stream_channel (camera, &error);
				if (error == NULL) gv_packet_delay = arv_camera_gv_get_packet_delay (camera, &error);
			}

			if (error != NULL) {
				printf ("Failed to read the current device configuration: %s\n", error->message);
				g_clear_error (&error);
				success = FALSE;
			}

		}

		if (success) {
			printf ("vendor name            = %s\n", vendor_name);
			printf ("model name             = %s\n", model_name);
			printf ("device serial number   = %s\n", serial_number);
			printf ("image width            = %d\n", width);
			printf ("image height           = %d\n", height);
			if (arv_camera_is_binning_available (camera, NULL)) {
				printf ("horizontal binning     = %d\n", dx);
				printf ("vertical binning       = %d\n", dy);
			}
			if (arv_camera_is_exposure_time_available (camera, NULL))
				printf ("exposure               = %g µs\n", exposure);
			if (arv_camera_is_gain_available (camera, NULL))
				printf ("gain                   = %d dB\n", gain);
			printf ("payload                = %d bytes\n", payload);
			if (arv_camera_is_uv_device (camera)) {
				if (arv_camera_uv_is_bandwidth_control_available (camera, NULL)) {
					printf ("uv bandwidth limit     = %d [%d..%d]\n", uv_bandwidth, min, max);
				}
			}
			if (arv_camera_is_gv_device (camera)) {
				printf ("gv n_stream channels   = %d\n", gv_n_channels);
				printf ("gv current channel     = %d\n", gv_channel_id);
				printf ("gv packet delay        = %d ns\n", gv_packet_delay);
			}

		}

		if (success) {
		    stream = arv_camera_create_stream (camera, stream_cb, NULL, &error);

                    if (arv_camera_is_gv_device (camera)) {
                            guint gv_packet_size;

                            gv_packet_size = arv_camera_gv_get_packet_size (camera, &error);
                            printf ("gv packet size         = %d bytes\n", gv_packet_size);
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
						  "initial-packet-timeout", (unsigned) arv_option_initial_packet_timeout * 1000,
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

                            data.start_time = g_get_monotonic_time();

			    g_timeout_add (1000, periodic_task_cb, &data);

			    data.main_loop = g_main_loop_new (NULL, FALSE);

			    old_sigint_handler = signal (SIGINT, set_cancel);

			    g_main_loop_run (data.main_loop);

			    if (software_trigger_source > 0)
				    g_source_remove (software_trigger_source);

			    signal (SIGINT, old_sigint_handler);

			    g_main_loop_unref (data.main_loop);

			    arv_stream_get_statistics (stream, &n_completed_buffers, &n_failures, &n_underruns);

                            for (i = 0; i < arv_stream_get_n_infos (stream); i++) {
                                    if (arv_stream_get_info_type (stream, i) == G_TYPE_UINT64) {
                                            g_print ("%-22s = %" G_GUINT64_FORMAT "\n",
                                                     arv_stream_get_info_name (stream, i),
                                                     arv_stream_get_info_uint64 (stream, i));
                                    }
                            }

			    arv_camera_stop_acquisition (camera, NULL);

			    arv_stream_set_emit_signals (stream, FALSE);

			    g_object_unref (stream);
		    } else {
			    printf ("Can't create stream thread%s%s\n",
				    error != NULL ? ": " : "",
				    error != NULL ? error->message : "");

			    g_clear_error (&error);
		    }
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
