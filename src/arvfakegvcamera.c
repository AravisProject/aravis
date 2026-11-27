/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#include <arvdebugprivate.h>
#include <arv.h>
#include <stdlib.h>
#include <stdio.h>

static volatile gboolean cancel = FALSE;

static void
set_cancel (int signal)
{
       cancel = TRUE;
}

static char *arv_option_interface_name = NULL;
static char *arv_option_serial_number = NULL;
static char *arv_option_genicam_file = NULL;
static double arv_option_gvsp_lost_ratio = 0.0;
static char *arv_option_debug_domains = NULL;
static gboolean arv_option_packet_resend = FALSE;
static int arv_option_frame_retention = 4;
static gboolean arv_option_static_image = FALSE;
static int arv_option_packet_rate = 0;
static int arv_option_gvcp_poll_interval = 64;
static int arv_option_drop_every_nth = 0;
static char *arv_option_drop_burst = NULL;
static char *arv_option_drop_head = NULL;
static double arv_option_resend_lost_ratio = 0.0;
static char *arv_option_sensor_size = NULL;
static gboolean arv_option_print_counters = FALSE;

static const GOptionEntry arv_option_entries[] =
{
	{ "interface",		'i', 0, G_OPTION_ARG_STRING,
		&arv_option_interface_name,	"Listening interface name or address", "interface"},
	{ "serial",             's', 0, G_OPTION_ARG_STRING,
	        &arv_option_serial_number, 	"Fake camera serial number", "serial_nbr"},
	{ "genicam",            'g', 0, G_OPTION_ARG_STRING,
	        &arv_option_genicam_file, 	"XML Genicam file to use", "genicam_filename"},
	{ "gvsp-lost-ratio",    'r', 0, G_OPTION_ARG_DOUBLE,
	        &arv_option_gvsp_lost_ratio,	"GVSP lost packet ratio", "packet_per_thousand"},
	{ "resend",		0, 0, G_OPTION_ARG_NONE,
		&arv_option_packet_resend,	"Advertise and serve packet resend requests", NULL},
	{ "frame-retention",	0, 0, G_OPTION_ARG_INT,
		&arv_option_frame_retention,	"Frames kept for resends, current one included (default 4)", "n_frames"},
	{ "static-image",	0, 0, G_OPTION_ARG_NONE,
		&arv_option_static_image,	"Fill the image once per stream, then only advance frame id and timestamp", NULL},
	{ "packet-rate",	0, 0, G_OPTION_ARG_INT,
		&arv_option_packet_rate,	"Pace packets within a frame (0 = burst, default)", "packets_per_second"},
	{ "gvcp-poll-interval",	0, 0, G_OPTION_ARG_INT,
		&arv_option_gvcp_poll_interval,	"Packets between GVCP polls during a frame (default 64)", "n_packets"},
	{ "drop-every-nth",	0, 0, G_OPTION_ARG_INT,
		&arv_option_drop_every_nth,	"Drop every Nth packet", "N"},
	{ "drop-burst",		0, 0, G_OPTION_ARG_STRING,
		&arv_option_drop_burst,		"Drop K consecutive packets every N frames", "K:N"},
	{ "drop-head",		0, 0, G_OPTION_ARG_STRING,
		&arv_option_drop_head,		"Drop the leader and the first K data packets of every Nth frame", "K:N"},
	{ "resend-lost-ratio",	0, 0, G_OPTION_ARG_DOUBLE,
		&arv_option_resend_lost_ratio,	"Probability of dropping a resent packet", "ratio"},
	{ "sensor-size",	0, 0, G_OPTION_ARG_STRING,
		&arv_option_sensor_size,	"Sensor size (default 2048x2048)", "<width>x<height>"},
	{ "counters",		0, 0, G_OPTION_ARG_NONE,
		&arv_option_print_counters,	"Print sender counters once a second", NULL},
	{
		"debug", 			'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	NULL,
		"{<category>[:<level>][,...]|help}"
	},
	{ NULL }
};

static const char
description_content[] =
"The genicam parameter is for debug purpose only. It is not possible to load\n"
"any arbitrary genicam data, as the declared features must match the registers\n"
"of the fake device.\n"
"\n"
"Examples:\n"
"\n"
"arv-fake-gv-camera-" ARAVIS_API_VERSION " -i eth0\n"
"arv-fake-gv-camera-" ARAVIS_API_VERSION " -i 127.0.0.1\n"
"arv-fake-gv-camera-" ARAVIS_API_VERSION " -s GV02 -d all\n"
"arv-fake-gv-camera-" ARAVIS_API_VERSION " -i 127.0.0.1 --sensor-size 2448x2048 --static-image --resend --drop-every-nth 1000\n"
"\n"
"Sender counters are printed on exit as one line starting with 'sender-counters', tab separated\n"
"key=value pairs.\n";

static gboolean
parse_pair (const char *option, const char *text, guint *a, guint *b, char separator)
{
	char *end = NULL;

	if (text == NULL)
		return TRUE;

	*a = g_ascii_strtoull (text, &end, 10);
	if (end == NULL || *end != separator) {
		printf ("Invalid %s value '%s'\n", option, text);
		return FALSE;
	}
	*b = g_ascii_strtoull (end + 1, &end, 10);
	if (end == NULL || *end != '\0') {
		printf ("Invalid %s value '%s'\n", option, text);
		return FALSE;
	}

	return TRUE;
}

static void
print_counters (ArvGvFakeCamera *gv_camera, double elapsed_s)
{
	guint64 n_frames, n_packets, n_errors, n_no_space, n_dropped, n_requests, n_requested, n_resent;
	guint64 n_resend_dropped, n_unavailable, sending_us, streaming_us;

	g_object_get (gv_camera,
		      "n-sent-frames", &n_frames,
		      "n-sent-packets", &n_packets,
		      "n-send-errors", &n_errors,
		      "n-send-no-buffer-space", &n_no_space,
		      "n-dropped-packets", &n_dropped,
		      "n-resend-requests", &n_requests,
		      "n-resend-requested-packets", &n_requested,
		      "n-resent-packets", &n_resent,
		      "n-resend-dropped-packets", &n_resend_dropped,
		      "n-resend-unavailable", &n_unavailable,
		      "sending-time-us", &sending_us,
		      "streaming-time-us", &streaming_us,
		      NULL);

	printf ("sender-counters"
		"\tsent_frames=%" G_GUINT64_FORMAT
		"\tsent_packets=%" G_GUINT64_FORMAT
		"\tsend_errors=%" G_GUINT64_FORMAT
		"\tsend_no_buffer_space=%" G_GUINT64_FORMAT
		"\tdropped_packets=%" G_GUINT64_FORMAT
		"\tresend_requests=%" G_GUINT64_FORMAT
		"\tresend_requested_packets=%" G_GUINT64_FORMAT
		"\tresent_packets=%" G_GUINT64_FORMAT
		"\tresend_dropped_packets=%" G_GUINT64_FORMAT
		"\tresend_unavailable=%" G_GUINT64_FORMAT
		"\tsending_time_us=%" G_GUINT64_FORMAT
		"\tstreaming_time_us=%" G_GUINT64_FORMAT
		"\tin_frame_packets_per_s=%.0f"
		"\tstreaming_packets_per_s=%.0f"
		"\telapsed_s=%.3f\n",
		n_frames, n_packets, n_errors, n_no_space, n_dropped, n_requests, n_requested, n_resent,
		n_resend_dropped, n_unavailable, sending_us, streaming_us,
		sending_us > 0 ? n_packets * 1e6 / sending_us : 0.0,
		streaming_us > 0 ? n_packets * 1e6 / streaming_us : 0.0,
		elapsed_s);
	fflush (stdout);
}

int
main (int argc, char **argv)
{
	guint drop_burst_length = 0, drop_burst_interval = 0;
	guint drop_head_length = 0, drop_head_interval = 0;
	guint sensor_width = 0, sensor_height = 0;
	gint64 start_us;
	ArvGvFakeCamera *gv_camera;
	GOptionContext *context;
	GError *error = NULL;

	context = g_option_context_new (NULL);
	g_option_context_set_summary (context, "Fake GigEVision camera.");
	g_option_context_set_description (context, description_content);
	g_option_context_add_main_entries (context, arv_option_entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	if (!arv_debug_enable (arv_option_debug_domains)) {
		if (g_strcmp0 (arv_option_debug_domains, "help") != 0)
			printf ("Invalid debug selection\n");
		else
			arv_debug_print_infos ();
		return EXIT_FAILURE;
	}

	if (!parse_pair ("--drop-burst", arv_option_drop_burst, &drop_burst_length, &drop_burst_interval, ':') ||
	    !parse_pair ("--drop-head", arv_option_drop_head, &drop_head_length, &drop_head_interval, ':') ||
	    !parse_pair ("--sensor-size", arv_option_sensor_size, &sensor_width, &sensor_height, 'x'))
		return EXIT_FAILURE;

	gv_camera = arv_gv_fake_camera_new_full (arv_option_interface_name, arv_option_serial_number, arv_option_genicam_file);

	g_object_set (gv_camera,
		      "gvsp-lost-ratio", arv_option_gvsp_lost_ratio / 1000.0,
		      "packet-resend", arv_option_packet_resend,
		      "frame-retention", (guint) MAX (1, arv_option_frame_retention),
		      "static-image", arv_option_static_image,
		      "packet-rate", (guint) MAX (0, arv_option_packet_rate),
		      "gvcp-poll-interval", (guint) MAX (0, arv_option_gvcp_poll_interval),
		      "drop-every-nth-packet", (guint) MAX (0, arv_option_drop_every_nth),
		      "drop-burst-length", drop_burst_length,
		      "drop-burst-frame-interval", drop_burst_interval,
		      "drop-head-length", drop_head_length,
		      "drop-head-frame-interval", drop_head_interval,
		      "resend-lost-ratio", arv_option_resend_lost_ratio,
		      "sensor-width", sensor_width,
		      "sensor-height", sensor_height,
		      NULL);

	signal (SIGINT, set_cancel);
#ifdef SIGTERM
	signal (SIGTERM, set_cancel);
#endif

	start_us = g_get_monotonic_time ();

	if (arv_gv_fake_camera_is_running (gv_camera)) {
		gint64 last_print_us = start_us;

		printf ("ready\n");
		fflush (stdout);

		while (!cancel) {
			g_usleep (50000);
			if (arv_option_print_counters && g_get_monotonic_time () - last_print_us >= 1000000) {
				last_print_us = g_get_monotonic_time ();
				print_counters (gv_camera, (last_print_us - start_us) / 1e6);
			}
		}

		print_counters (gv_camera, (g_get_monotonic_time () - start_us) / 1e6);
	} else
		printf ("Failed to start camera\n");

	g_object_unref (gv_camera);

	return EXIT_SUCCESS;
}
