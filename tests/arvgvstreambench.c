/* SPDX-License-Identifier:Unlicense */

/* GigE Vision stream receiver bench, on loopback.
 *
 * Launches arv-fake-gv-camera as a child process (so that sender and receiver CPU are accounted separately), opens it
 * by address through the normal Aravis client API, streams for a fixed duration with a consumer that pops and pushes
 * buffers (optionally stalling once), and prints a header and one TSV row.
 *
 * Loopback has no NIC: the numbers measure userspace and kernel socket cost and the receiver logic, not a network.
 */

#include <arv.h>
#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#endif

#ifndef ARV_FAKE_GV_CAMERA_PATH
#define ARV_FAKE_GV_CAMERA_PATH "arv-fake-gv-camera-" ARAVIS_API_VERSION
#endif

static char *option_fake_camera = NULL;
static char *option_label = NULL;
static int option_width = 2448;
static int option_height = 2048;
static int option_packet_size = 8996;
static double option_frame_rate = 100.0;
static int option_packet_rate = 0;
static double option_duration = 10.0;
static int option_n_buffers = 16;
static double option_loss_ratio = 0.0;
static int option_drop_every_nth = 0;
static char *option_drop_burst = NULL;
static char *option_drop_head = NULL;
static double option_resend_lost_ratio = 0.0;
static gboolean option_no_resend = FALSE;
static gboolean option_dynamic_image = FALSE;
static int option_frame_retention = 4;
static int option_socket_buffer_size = 0;
static double option_stall_at = 1.0;
static int option_stall_ms = 0;
static gboolean option_no_header = FALSE;
static char *option_debug = NULL;

static const GOptionEntry option_entries[] =
{
	{ "fake-camera",	0, 0, G_OPTION_ARG_FILENAME, &option_fake_camera,
		"arv-fake-gv-camera executable (default: the one in the build tree)", "path" },
	{ "label",		0, 0, G_OPTION_ARG_STRING, &option_label, "Label for the row", "text" },
	{ "width",		0, 0, G_OPTION_ARG_INT, &option_width, "Width (default 2448)", "pixels" },
	{ "height",		0, 0, G_OPTION_ARG_INT, &option_height, "Height (default 2048)", "pixels" },
	{ "packet-size",	0, 0, G_OPTION_ARG_INT, &option_packet_size, "GevSCPSPacketSize (default 8996)", "bytes" },
	{ "frame-rate",		0, 0, G_OPTION_ARG_DOUBLE, &option_frame_rate, "Frame rate (default 100)", "fps" },
	{ "packet-rate",	0, 0, G_OPTION_ARG_INT, &option_packet_rate,
		"Sender in-frame pacing, 0 for a burst (default 0)", "packets_per_s" },
	{ "duration",		0, 0, G_OPTION_ARG_DOUBLE, &option_duration, "Duration (default 10)", "s" },
	{ "buffers",		0, 0, G_OPTION_ARG_INT, &option_n_buffers, "Stream buffers (default 16)", "n" },
	{ "loss",		0, 0, G_OPTION_ARG_DOUBLE, &option_loss_ratio,
		"Random injected loss ratio (0.001 = 0.1 %)", "ratio" },
	{ "drop-every-nth",	0, 0, G_OPTION_ARG_INT, &option_drop_every_nth, "Drop every Nth packet", "N" },
	{ "drop-burst",		0, 0, G_OPTION_ARG_STRING, &option_drop_burst,
		"Drop K consecutive packets every N frames", "K:N" },
	{ "drop-head",		0, 0, G_OPTION_ARG_STRING, &option_drop_head,
		"Drop the leader and K packets of every Nth frame", "K:N" },
	{ "resend-lost-ratio",	0, 0, G_OPTION_ARG_DOUBLE, &option_resend_lost_ratio,
		"Probability of dropping a resent packet", "ratio" },
	{ "no-resend",		0, 0, G_OPTION_ARG_NONE, &option_no_resend,
		"Sender does not advertise or serve resends", NULL },
	{ "dynamic-image",	0, 0, G_OPTION_ARG_NONE, &option_dynamic_image,
		"Sender recomputes the test pattern every frame (default: static image)", NULL },
	{ "frame-retention",	0, 0, G_OPTION_ARG_INT, &option_frame_retention,
		"Frames the sender keeps for resends (default 4)", "n" },
	{ "socket-buffer-size",	0, 0, G_OPTION_ARG_INT, &option_socket_buffer_size,
		"Fixed receive socket buffer size, 0 for the stream's automatic size", "bytes" },
	{ "stall-at",		0, 0, G_OPTION_ARG_DOUBLE, &option_stall_at,
		"When the consumer stalls (default 1)", "s" },
	{ "stall-ms",		0, 0, G_OPTION_ARG_INT, &option_stall_ms,
		"Stall the consumer once for this long (default 0, no stall)", "ms" },
	{ "no-header",		0, 0, G_OPTION_ARG_NONE, &option_no_header, "Only print the row", NULL },
	{ "debug",		'd', 0, G_OPTION_ARG_STRING, &option_debug, "Aravis debug domains", "domains" },
	{ NULL }
};

typedef struct {
	GString *header;
	GString *row;
} Table;

static void
table_add (Table *table, const char *name, const char *format, ...) G_GNUC_PRINTF (3, 4);

static void
table_add (Table *table, const char *name, const char *format, ...)
{
	va_list args;

	if (table->header->len > 0)
		g_string_append_c (table->header, '\t');
	g_string_append (table->header, name);

	if (table->row->len > 0)
		g_string_append_c (table->row, '\t');
	va_start (args, format);
	g_string_append_vprintf (table->row, format, args);
	va_end (args);
}

#ifdef __APPLE__
/* Datagrams the kernel dropped on a full UDP socket buffer, system wide: Apple only */
static guint64
udp_fullsock (void)
{
	struct udpstat stats;
	size_t size = sizeof (stats);

	if (sysctlbyname ("net.inet.udp.stats", &stats, &size, NULL, 0) == 0)
		return stats.udps_fullsock;
	return 0;
}
#endif

static double
rusage_cpu_s (int who)
{
	struct rusage usage;

	if (getrusage (who, &usage) != 0)
		return 0.0;

	return usage.ru_utime.tv_sec + usage.ru_utime.tv_usec * 1e-6 +
		usage.ru_stime.tv_sec + usage.ru_stime.tv_usec * 1e-6;
}

static double
load_average_1 (void)
{
	double load[3] = {0.0, 0.0, 0.0};

	if (getloadavg (load, 3) < 1)
		return -1.0;

	return load[0];
}

/* Number of frame ids strictly between two 16 bit GVSP frame ids, 0 being skipped */
static guint
missing_ids_between (guint previous, guint current)
{
	guint n = 0;
	guint id = previous;

	while (n < 65536) {
		id = id >= 65535 ? 1 : id + 1;
		if (id == current)
			return n;
		n++;
	}

	return n;
}

static GSubprocess *
launch_sender (GDataInputStream **output, GError **error)
{
	GPtrArray *args = g_ptr_array_new_with_free_func (g_free);
	GSubprocess *subprocess = NULL;
	int attempt;

	g_ptr_array_add (args, g_strdup (option_fake_camera != NULL ? option_fake_camera : ARV_FAKE_GV_CAMERA_PATH));
	g_ptr_array_add (args, g_strdup ("-i"));
	g_ptr_array_add (args, g_strdup ("127.0.0.1"));
	g_ptr_array_add (args, g_strdup_printf ("--sensor-size=%dx%d", MAX (option_width, 2048), MAX (option_height, 2048)));
	if (!option_dynamic_image)
		g_ptr_array_add (args, g_strdup ("--static-image"));
	if (!option_no_resend)
		g_ptr_array_add (args, g_strdup ("--resend"));
	g_ptr_array_add (args, g_strdup_printf ("--frame-retention=%d", option_frame_retention));
	g_ptr_array_add (args, g_strdup_printf ("--packet-rate=%d", option_packet_rate));
	g_ptr_array_add (args, g_strdup_printf ("--gvsp-lost-ratio=%g", option_loss_ratio * 1000.0));
	g_ptr_array_add (args, g_strdup_printf ("--drop-every-nth=%d", option_drop_every_nth));
	if (option_drop_burst != NULL)
		g_ptr_array_add (args, g_strdup_printf ("--drop-burst=%s", option_drop_burst));
	if (option_drop_head != NULL)
		g_ptr_array_add (args, g_strdup_printf ("--drop-head=%s", option_drop_head));
	g_ptr_array_add (args, g_strdup_printf ("--resend-lost-ratio=%g", option_resend_lost_ratio));
	g_ptr_array_add (args, NULL);

	/* The GVCP port is fixed: another fake camera on 127.0.0.1 makes ours fail to start. Wait for it. */
	for (attempt = 0; attempt < 60; attempt++) {
		GInputStream *stdout_pipe;
		char *line;

		g_clear_error (error);
		subprocess = g_subprocess_newv ((const char * const *) args->pdata, G_SUBPROCESS_FLAGS_STDOUT_PIPE, error);
		if (subprocess == NULL)
			break;

		stdout_pipe = g_subprocess_get_stdout_pipe (subprocess);
		*output = g_data_input_stream_new (stdout_pipe);

		line = g_data_input_stream_read_line (*output, NULL, NULL, error);
		if (line != NULL && g_strcmp0 (line, "ready") == 0) {
			g_free (line);
			break;
		}

		g_subprocess_force_exit (subprocess);
		g_subprocess_wait (subprocess, NULL, NULL);
		g_clear_object (output);
		g_clear_object (&subprocess);

		if (g_strcmp0 (line, "Failed to start camera") != 0) {
			g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Fake camera did not start: %s",
				     line != NULL ? line : "no output");
			g_free (line);
			break;
		}

		g_printerr ("Fake camera did not start (GVCP port busy?), retrying in 2 s\n");
		g_free (line);
		g_usleep (2000000);
	}

	g_ptr_array_unref (args);

	if (subprocess == NULL && error != NULL && *error == NULL)
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Fake camera did not start");

	return subprocess;
}

static void
parse_sender_counters (const char *line, Table *table)
{
	char **fields;
	int i;

	fields = g_strsplit (line, "\t", -1);
	for (i = 1; fields[i] != NULL; i++) {
		char **pair = g_strsplit (fields[i], "=", 2);

		if (pair[0] != NULL && pair[1] != NULL) {
			char *name = g_strdup_printf ("tx_%s", pair[0]);

			table_add (table, name, "%s", pair[1]);
			g_free (name);
		}
		g_strfreev (pair);
	}
	g_strfreev (fields);
}

int
main (int argc, char **argv)
{
	GOptionContext *context;
	GError *error = NULL;
	GSubprocess *sender;
	GDataInputStream *sender_output = NULL;
	ArvCamera *camera = NULL;
	ArvStream *stream = NULL;
	Table table;
	size_t payload;
	char *sender_counters = NULL;
	char *line;
#ifdef __APPLE__
	guint64 fullsock_start, fullsock_end;
#endif
	double cpu_self_start, cpu_self_end;
	double cpu_children_start, cpu_children_end;
	double load_start, load_end;
	gint64 start_us, end_us, stall_us;
	gboolean stalled = FALSE;
	guint64 n_complete = 0, n_failed = 0, n_gap_events = 0, n_missing_ids = 0;
	guint64 n_status[ARV_BUFFER_STATUS_FILLING + 2] = {0};
	guint last_frame_id = 0;
	gboolean has_last_frame_id = FALSE;
	guint i, n_infos;
	int attempt;

	context = g_option_context_new (NULL);
	g_option_context_set_summary (context, "GigE Vision stream receiver bench on loopback, against arv-fake-gv-camera.");
	g_option_context_add_main_entries (context, option_entries, NULL);
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_printerr ("Option parsing failed: %s\n", error->message);
		return EXIT_FAILURE;
	}
	g_option_context_free (context);

	if (option_debug != NULL)
		arv_debug_enable (option_debug);

	/* Never scan or open anything but the loopback fake camera */
	arv_gv_interface_set_discovery_interface_name ("lo0");

	table.header = g_string_new ("");
	table.row = g_string_new ("");

	load_start = load_average_1 ();
	cpu_children_start = rusage_cpu_s (RUSAGE_CHILDREN);

	sender = launch_sender (&sender_output, &error);
	if (sender == NULL) {
		g_printerr ("Failed to launch fake camera: %s\n", error != NULL ? error->message : "unknown error");
		return EXIT_FAILURE;
	}

	for (attempt = 0; attempt < 10 && camera == NULL; attempt++) {
		g_clear_error (&error);
		camera = arv_camera_new ("127.0.0.1", &error);
		if (camera == NULL)
			g_usleep (200000);
	}
	if (camera == NULL) {
		g_printerr ("Failed to open the fake camera: %s\n", error != NULL ? error->message : "unknown error");
		g_subprocess_force_exit (sender);
		return EXIT_FAILURE;
	}

	arv_camera_set_region (camera, 0, 0, option_width, option_height, &error);
	if (error == NULL)
		arv_camera_set_pixel_format (camera, ARV_PIXEL_FORMAT_MONO_8, &error);
	if (error == NULL)
		arv_camera_gv_set_packet_size (camera, option_packet_size, &error);
	if (error == NULL)
		arv_camera_set_frame_rate (camera, option_frame_rate, &error);
	if (error == NULL)
		stream = arv_camera_create_stream (camera, NULL, NULL, NULL, &error);
	if (error != NULL) {
		g_printerr ("Failed to configure the fake camera: %s\n", error->message);
		g_subprocess_force_exit (sender);
		return EXIT_FAILURE;
	}

	if (option_socket_buffer_size > 0)
		g_object_set (stream,
			      "socket-buffer", ARV_GV_STREAM_SOCKET_BUFFER_FIXED,
			      "socket-buffer-size", option_socket_buffer_size,
			      NULL);

	payload = arv_camera_get_payload (camera, NULL);
	for (i = 0; i < (guint) option_n_buffers; i++)
		arv_stream_push_buffer (stream, arv_buffer_new (payload, NULL));

#ifdef __APPLE__
	fullsock_start = udp_fullsock ();
#endif
	cpu_self_start = rusage_cpu_s (RUSAGE_SELF);

	arv_camera_start_acquisition (camera, &error);
	if (error != NULL) {
		g_printerr ("Failed to start acquisition: %s\n", error->message);
		g_subprocess_force_exit (sender);
		return EXIT_FAILURE;
	}

	start_us = g_get_monotonic_time ();
	end_us = start_us + (gint64) (option_duration * 1e6);
	stall_us = start_us + (gint64) (option_stall_at * 1e6);

	while (g_get_monotonic_time () < end_us) {
		ArvBuffer *buffer;

		if (option_stall_ms > 0 && !stalled && g_get_monotonic_time () >= stall_us) {
			g_usleep (option_stall_ms * 1000);
			stalled = TRUE;
		}

		buffer = arv_stream_timeout_pop_buffer (stream, 100000);
		if (buffer == NULL)
			continue;

		if (arv_buffer_get_status (buffer) == ARV_BUFFER_STATUS_SUCCESS) {
			guint frame_id = arv_buffer_get_frame_id (buffer);

			if (has_last_frame_id) {
				guint n_missing = missing_ids_between (last_frame_id, frame_id);

				if (n_missing > 0) {
					n_gap_events++;
					n_missing_ids += n_missing;
				}
			}
			last_frame_id = frame_id;
			has_last_frame_id = TRUE;
			n_complete++;
		} else
			n_failed++;

		if ((guint) arv_buffer_get_status (buffer) <= ARV_BUFFER_STATUS_FILLING)
			n_status[arv_buffer_get_status (buffer)]++;

		arv_stream_push_buffer (stream, buffer);
	}

	end_us = g_get_monotonic_time ();
	arv_camera_stop_acquisition (camera, NULL);

	cpu_self_end = rusage_cpu_s (RUSAGE_SELF);
#ifdef __APPLE__
	fullsock_end = udp_fullsock ();
#endif

	table_add (&table, "label", "%s", option_label != NULL ? option_label : "-");
	table_add (&table, "width", "%d", option_width);
	table_add (&table, "height", "%d", option_height);
	table_add (&table, "packet_size", "%d", option_packet_size);
	table_add (&table, "frame_rate", "%g", option_frame_rate);
	table_add (&table, "packet_rate", "%d", option_packet_rate);
	table_add (&table, "buffers", "%d", option_n_buffers);
	table_add (&table, "duration_s", "%.3f", (end_us - start_us) / 1e6);
	table_add (&table, "loss", "%g", option_loss_ratio);
	table_add (&table, "drop_every_nth", "%d", option_drop_every_nth);
	table_add (&table, "drop_burst", "%s", option_drop_burst != NULL ? option_drop_burst : "-");
	table_add (&table, "drop_head", "%s", option_drop_head != NULL ? option_drop_head : "-");
	table_add (&table, "resend_lost_ratio", "%g", option_resend_lost_ratio);
	table_add (&table, "resend", "%s", option_no_resend ? "no" : "yes");
	table_add (&table, "stall_ms", "%d", option_stall_ms);
	table_add (&table, "socket_buffer", "%d", option_socket_buffer_size);

	table_add (&table, "frames_expected", "%.0f", (end_us - start_us) / 1e6 * option_frame_rate);
	table_add (&table, "frames_complete", "%" G_GUINT64_FORMAT, n_complete);
	table_add (&table, "frames_failed", "%" G_GUINT64_FORMAT, n_failed);
	table_add (&table, "complete_fps", "%.1f", n_complete / ((end_us - start_us) / 1e6));
	table_add (&table, "status_timeout", "%" G_GUINT64_FORMAT, n_status[ARV_BUFFER_STATUS_TIMEOUT]);
	table_add (&table, "status_missing_packets", "%" G_GUINT64_FORMAT, n_status[ARV_BUFFER_STATUS_MISSING_PACKETS]);
	table_add (&table, "frame_id_gap_events", "%" G_GUINT64_FORMAT, n_gap_events);
	table_add (&table, "frame_ids_missing", "%" G_GUINT64_FORMAT, n_missing_ids);

	n_infos = arv_stream_get_n_infos (stream);
	for (i = 0; i < n_infos; i++) {
		if (arv_stream_get_info_type (stream, i) == G_TYPE_UINT64) {
			char *name = g_strdup_printf ("rx_%s", arv_stream_get_info_name (stream, i));

			table_add (&table, name, "%" G_GUINT64_FORMAT, arv_stream_get_info_uint64 (stream, i));
			g_free (name);
		} else if (arv_stream_get_info_type (stream, i) == G_TYPE_DOUBLE) {
			char *name = g_strdup_printf ("rx_%s", arv_stream_get_info_name (stream, i));

			table_add (&table, name, "%g", arv_stream_get_info_double (stream, i));
			g_free (name);
		}
	}

	g_clear_object (&stream);
	g_clear_object (&camera);

	/* Ask the sender to quit: it prints its counters on the way out */
	g_subprocess_send_signal (sender, SIGINT);
	while ((line = g_data_input_stream_read_line (sender_output, NULL, NULL, NULL)) != NULL) {
		if (g_str_has_prefix (line, "sender-counters")) {
			g_free (sender_counters);
			sender_counters = line;
		} else
			g_free (line);
	}
	g_subprocess_wait (sender, NULL, NULL);
	cpu_children_end = rusage_cpu_s (RUSAGE_CHILDREN);

	if (sender_counters != NULL)
		parse_sender_counters (sender_counters, &table);
	else
		table_add (&table, "tx_counters", "missing");

#ifdef __APPLE__
	table_add (&table, "udp_fullsock_delta", "%" G_GUINT64_FORMAT, fullsock_end - fullsock_start);
#else
	table_add (&table, "udp_fullsock_delta", "n/a");
#endif
	table_add (&table, "cpu_receiver_s", "%.2f", cpu_self_end - cpu_self_start);
	table_add (&table, "cpu_sender_s", "%.2f", cpu_children_end - cpu_children_start);
	load_end = load_average_1 ();
	table_add (&table, "load1_start", "%.2f", load_start);
	table_add (&table, "load1_end", "%.2f", load_end);

	if (!option_no_header)
		printf ("%s\n", table.header->str);
	printf ("%s\n", table.row->str);

	g_free (sender_counters);
	g_object_unref (sender_output);
	g_object_unref (sender);
	g_string_free (table.header, TRUE);
	g_string_free (table.row, TRUE);

	arv_shutdown ();

	return EXIT_SUCCESS;
}
