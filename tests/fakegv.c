/* SPDX-License-Identifier:Unlicense */

#include <glib.h>
#include <arv.h>

static ArvCamera *camera = NULL;
static ArvGvFakeCamera *simulator = NULL;

static void
discovery_test (void)
{
        ArvCamera *test_camera;
        char *discovery_iface;
        int n_devices;
        int i;

        arv_gv_interface_set_discovery_interface_name("not-an-iface");

        arv_update_device_list();

        discovery_iface = arv_gv_interface_dup_discovery_interface_name();
        g_assert (discovery_iface != NULL);
        g_assert_cmpstr("not-an-iface", ==, discovery_iface);
        g_free (discovery_iface);

        test_camera = arv_camera_new ("Aravis-GVTest", NULL);
        g_assert (test_camera == NULL);

        arv_gv_interface_set_discovery_interface_name(NULL);

        arv_update_device_list();

        discovery_iface = arv_gv_interface_dup_discovery_interface_name();
        g_assert (discovery_iface == NULL);

        test_camera = arv_camera_new ("Aravis-GVTest", NULL);
        g_assert (ARV_IS_CAMERA(test_camera));
        g_clear_object (&test_camera);

        n_devices = arv_get_n_devices ();

        for (i = 0; i < n_devices; i++) {
                if (g_strcmp0 (arv_get_device_vendor (i), "Aravis") == 0 &&
                    g_strcmp0 (arv_get_device_model (i), "Fake") == 0 &&
                    g_strcmp0 (arv_get_device_serial_nbr (i), "GVTest") == 0 &&
                    g_strcmp0 (arv_get_device_manufacturer_info (i), "none") == 0 &&
                    g_strcmp0 (arv_get_device_id (i), "Aravis-Fake-GVTest") == 0 &&
                    g_strcmp0 (arv_get_device_physical_id (i), "00:00:00:00:00:00") == 0)
                        return;
        }

        g_assert_not_reached ();
}

static void
register_test (void)
{
	ArvDevice *device;
	int int_value;
	double dbl_value;
	double boolean_value;

	device = arv_camera_get_device (camera);
	g_assert (ARV_IS_GV_DEVICE (device));

	/* Check default */
	int_value = arv_device_get_integer_feature_value (device, "Width", NULL);
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_WIDTH_DEFAULT);

	arv_device_set_integer_feature_value (device, "Width", 1024, NULL);
	int_value = arv_device_get_integer_feature_value (device, "Width", NULL);
	g_assert_cmpint (int_value, ==, 1024);

	/* Check default */
	int_value = arv_device_get_integer_feature_value (device, "Height", NULL);
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_HEIGHT_DEFAULT);

	arv_device_set_integer_feature_value (device, "Height", 1024, NULL);
	int_value = arv_device_get_integer_feature_value (device, "Height", NULL);
	g_assert_cmpint (int_value, ==, 1024);

	int_value = arv_device_get_integer_feature_value (device, "BinningHorizontal", NULL);
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_BINNING_HORIZONTAL_DEFAULT);
	int_value = arv_device_get_integer_feature_value (device, "BinningVertical", NULL);
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_BINNING_VERTICAL_DEFAULT);
	int_value = arv_device_get_integer_feature_value (device, "PixelFormat", NULL);
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_PIXEL_FORMAT_DEFAULT);

	dbl_value = arv_device_get_float_feature_value (device, "AcquisitionFrameRate", NULL);
	g_assert_cmpfloat (dbl_value, ==, ARV_FAKE_CAMERA_ACQUISITION_FRAME_RATE_DEFAULT);
	dbl_value = arv_device_get_float_feature_value (device,  "ExposureTimeAbs", NULL);
	g_assert_cmpfloat (dbl_value, ==, ARV_FAKE_CAMERA_EXPOSURE_TIME_US_DEFAULT);

	int_value = arv_device_get_integer_feature_value (device, "GainRaw", NULL);
	g_assert_cmpint (int_value, ==, 0);
	int_value = arv_device_get_integer_feature_value (device, "GainAuto", NULL);
	g_assert_cmpint (int_value, ==, 1);

	int_value = arv_device_get_integer_feature_value (device, "PayloadSize", NULL);
	g_assert_cmpint (int_value, ==, 1024 * 1024);

	arv_device_set_boolean_feature_value (device, "TestBoolean", FALSE, NULL);
	boolean_value = arv_device_get_boolean_feature_value (device, "TestBoolean", NULL);
	g_assert_cmpint (boolean_value, ==, FALSE);
	int_value = arv_device_get_integer_feature_value (device, "TestRegister", NULL);
	g_assert_cmpint (int_value, ==, 123);

	arv_device_set_boolean_feature_value (device, "TestBoolean", TRUE, NULL);
	boolean_value = arv_device_get_boolean_feature_value (device, "TestBoolean", NULL);
	g_assert_cmpint (boolean_value, ==, TRUE);
	int_value = arv_device_get_integer_feature_value (device, "TestRegister", NULL);
	g_assert_cmpint (int_value, ==, 321);
}

static void
acquisition_test (void)
{
	GError *error = NULL;
	ArvBuffer *buffer;
	gint x, y, width, height;
        const char *ignore_buffer;

        ignore_buffer = g_getenv("ARV_TEST_IGNORE_BUFFER");

	buffer = arv_camera_acquisition (camera, 0, &error);
	g_assert (error == NULL);
	g_assert (ARV_IS_BUFFER (buffer));

        if (ignore_buffer == NULL && arv_buffer_get_status(buffer) == ARV_BUFFER_STATUS_SUCCESS) {
                arv_buffer_get_image_region (buffer, &x, &y, &width, &height);

                g_assert_cmpint (x, ==, 0);
                g_assert_cmpint (y, ==, 0);
                g_assert_cmpint (width, ==, 1024);
                g_assert_cmpint (height, ==, 1024);

                g_assert_cmpint (arv_buffer_get_image_x (buffer), ==, 0);
                g_assert_cmpint (arv_buffer_get_image_y (buffer), ==, 0);
                g_assert_cmpint (arv_buffer_get_image_width (buffer), ==, 1024);
                g_assert_cmpint (arv_buffer_get_image_height (buffer), ==, 1024);

                g_assert (arv_buffer_get_image_pixel_format (buffer) == ARV_PIXEL_FORMAT_MONO_8);
        }

	g_clear_object (&buffer);
}

static void
new_buffer_cb (ArvStream *stream, unsigned *buffer_count)
{
	ArvBuffer *buffer;

	buffer = arv_stream_try_pop_buffer (stream);
	if (buffer != NULL) {
		(*buffer_count)++;
		if (*buffer_count == 10) {
			/* Sleep after the last buffer was received, in order
			 * to keep a reference to stream while the main loop
			 * ends. If the main is able to unref stream while
			 * this signal callback is still waiting, stream will
			 * be finalized in its stream thread contex (because
			 * g_signal_emit holds a reference to stream), leading
			 * to a deadlock. */
			g_usleep (1000000);
		}
		arv_stream_push_buffer (stream, buffer);
	}
}

static void
stream_test (void)
{
	ArvStream *stream;
	GError *error = NULL;
	size_t payload;
	unsigned buffer_count = 0;
	unsigned i;

	stream = arv_camera_create_stream (camera, NULL, NULL, NULL, &error);
	g_assert (ARV_IS_STREAM (stream));
	g_assert (error == NULL);

	payload = arv_camera_get_payload (camera, NULL);

	for (i = 0; i < 5; i++)
		arv_stream_push_buffer (stream, arv_buffer_new (payload, NULL));

	g_signal_connect (stream, "new-buffer", G_CALLBACK (new_buffer_cb), &buffer_count);
	arv_stream_set_emit_signals (stream, TRUE);

	arv_camera_start_acquisition (camera, NULL);

	while (buffer_count < 10)
		g_usleep (1000);

	arv_camera_stop_acquisition (camera, NULL);
	/* The following will block until the signal callback returns
	 * which avoids a race and possible deadlock.
	 */
	arv_stream_set_emit_signals (stream, FALSE);

	g_clear_object (&stream);

	/* For actually testing the deadlock condition (see comment in
	 * new_buffer_cb), one must wait a bit before leaving this test,
	 * because otherwise the stream thread will be killed while sleeping. */
	g_usleep (2000000);
}

#define N_BUFFERS	5

static struct {
	int width;
	int height;
} rois[] = { {100, 100}, {200, 200}, {300,300} };

static void
dynamic_roi_test (void)
{
	ArvStream *stream;
	GError *error = NULL;
	size_t payload;
	unsigned buffer_count = 0;
	unsigned i, j;

	stream = arv_camera_create_stream (camera, NULL, NULL, NULL, &error);
	g_assert (ARV_IS_STREAM (stream));
	g_assert (error == NULL);

	payload = arv_camera_get_payload (camera, NULL);

	g_signal_connect (stream, "new-buffer", G_CALLBACK (new_buffer_cb), &buffer_count);
	arv_stream_set_emit_signals (stream, TRUE);

	for (j = 0; j < G_N_ELEMENTS (rois); j++) {
		int height, width;

                if (j > 0)
                        g_assert (arv_stream_delete_buffers (stream) == N_BUFFERS);

		buffer_count = 0;

		arv_camera_set_region (camera, 0, 0, rois[j].width , rois[j].height, NULL);
		arv_camera_get_region (camera, NULL, NULL, &width, &height, NULL);

		g_assert (width == rois[j].width);
		g_assert (height == rois[j].height);

		payload = arv_camera_get_payload (camera, NULL);

		for (i = 0; i < N_BUFFERS; i++)
			arv_stream_push_buffer (stream, arv_buffer_new (payload, NULL));

		arv_camera_start_acquisition (camera, NULL);

		while (buffer_count < 10) {
			g_usleep (10000);
		}

		arv_camera_stop_acquisition (camera, NULL);
	}

	arv_stream_set_emit_signals (stream, FALSE);

	g_clear_object (&stream);
}


/* Loss and resend tests
 *
 * The simulator sends a static image (filled once per stream by test_pattern_fill) and drops packets with a
 * deterministic pattern, so that every delivered frame can be checked byte for byte and the frames that are
 * expected to fail are known in advance. */

#define LOSS_WIDTH		512
#define LOSS_HEIGHT		512
#define LOSS_FRAME_RATE		50.0
#define LOSS_SOCKET_BUFFER_SIZE	(4 * 1024 * 1024)

static gint n_pattern_fills = 0;

static guint8
expected_pixel (size_t i)
{
	return (guint8) ((i * 31) ^ (i >> 9));
}

static void
test_pattern_fill (ArvBuffer *buffer, void *fill_pattern_data,
		   guint32 exposure_time_us, guint32 gain, ArvPixelFormat pixel_format)
{
	guint8 *data = (guint8 *) arv_buffer_get_data (buffer, NULL);
	size_t size = (size_t) arv_buffer_get_image_width (buffer) * arv_buffer_get_image_height (buffer);
	size_t i;

	for (i = 0; i < size; i++)
		data[i] = expected_pixel (i);

	g_atomic_int_inc (&n_pattern_fills);
}

static gboolean
buffer_has_expected_content (ArvBuffer *buffer)
{
	const guint8 *data;
	size_t size;
	size_t i;

	data = arv_buffer_get_data (buffer, &size);
	if (size != LOSS_WIDTH * LOSS_HEIGHT)
		return FALSE;

	for (i = 0; i < size; i++)
		if (data[i] != expected_pixel (i))
			return FALSE;

	return TRUE;
}

static guint
next_frame_id (guint frame_id)
{
	/* 16 bit GVSP frame ids, 0 is skipped */
	return frame_id >= 65535 ? 1 : frame_id + 1;
}

static void
reset_simulator (void)
{
	g_object_set (simulator,
		      "packet-resend", FALSE,
		      "static-image", FALSE,
		      "drop-every-nth-packet", 0,
		      "drop-burst-length", 0,
		      "drop-burst-frame-interval", 0,
		      "drop-head-length", 0,
		      "drop-head-frame-interval", 0,
		      "resend-lost-ratio", 0.0,
		      NULL);
	arv_fake_camera_set_fill_pattern (arv_gv_fake_camera_get_fake_camera (simulator), NULL, NULL, NULL);
}

static ArvStream *
create_loss_stream (unsigned n_buffers)
{
	ArvStream *stream;
	GError *error = NULL;
	size_t payload;
	unsigned i;

	arv_fake_camera_set_fill_pattern (arv_gv_fake_camera_get_fake_camera (simulator), test_pattern_fill, NULL, NULL);
	g_atomic_int_set (&n_pattern_fills, 0);

	arv_camera_set_region (camera, 0, 0, LOSS_WIDTH, LOSS_HEIGHT, &error);
	g_assert_no_error (error);
	arv_camera_set_frame_rate (camera, LOSS_FRAME_RATE, &error);
	g_assert_no_error (error);

	stream = arv_camera_create_stream (camera, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (ARV_IS_STREAM (stream));

	/* The device was opened before the simulator advertised packet resend, which made the stream default to
	 * NEVER. A large socket buffer keeps a loaded test machine from adding loss of its own. */
	g_object_set (stream,
		      "packet-resend", ARV_GV_STREAM_PACKET_RESEND_ALWAYS,
		      "socket-buffer", ARV_GV_STREAM_SOCKET_BUFFER_FIXED,
		      "socket-buffer-size", LOSS_SOCKET_BUFFER_SIZE,
		      NULL);

	payload = arv_camera_get_payload (camera, &error);
	g_assert_no_error (error);
	g_assert_cmpint (payload, ==, LOSS_WIDTH * LOSS_HEIGHT);

	for (i = 0; i < n_buffers; i++)
		arv_stream_push_buffer (stream, arv_buffer_new (payload, NULL));

	return stream;
}

static void
finish_loss_stream (ArvStream *stream)
{
	GError *error = NULL;

	arv_camera_stop_acquisition (camera, &error);
	g_assert_no_error (error);
	g_object_unref (stream);

	reset_simulator ();
}

static guint64
simulator_counter (const char *name)
{
	guint64 value = 0;

	g_object_get (simulator, name, &value, NULL);

	return value;
}

static void
resend_test (void)
{
	ArvStream *stream;
	GError *error = NULL;
	guint64 n_resent_before = simulator_counter ("n-resent-packets");
	guint64 n_dropped_before = simulator_counter ("n-dropped-packets");
	guint n_frames = 0;
	guint last_frame_id = 0;

	g_object_set (simulator,
		      "packet-resend", TRUE,
		      "static-image", TRUE,
		      "drop-every-nth-packet", 97,
		      NULL);

	stream = create_loss_stream (8);

	arv_camera_start_acquisition (camera, &error);
	g_assert_no_error (error);

	/* About 2 s at 50 fps, with about two lost packets in each 195 packet frame */
	while (n_frames < 100) {
		ArvBuffer *buffer;

		buffer = arv_stream_timeout_pop_buffer (stream, 1000000);
		g_assert (ARV_IS_BUFFER (buffer));

		g_assert_cmpint (arv_buffer_get_status (buffer), ==, ARV_BUFFER_STATUS_SUCCESS);
		g_assert (buffer_has_expected_content (buffer));
		if (n_frames > 0)
			g_assert_cmpuint (arv_buffer_get_frame_id (buffer), ==, next_frame_id (last_frame_id));
		last_frame_id = arv_buffer_get_frame_id (buffer);

		n_frames++;
		arv_stream_push_buffer (stream, buffer);
	}

	g_assert_cmpint (g_atomic_int_get (&n_pattern_fills), ==, 1);
	g_assert_cmpuint (simulator_counter ("n-dropped-packets") - n_dropped_before, >, 0);
	g_assert_cmpuint (simulator_counter ("n-resent-packets") - n_resent_before, >, 0);
	g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "n_resent_packets"), >, 0);
	g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "n_failures"), ==, 0);

	finish_loss_stream (stream);
}

/* 512x512 Mono8 in 1400 byte packets: leader, 193 data packets, trailer */
#define LOSS_N_BLOCKS	195

static void
resend_unavailable_test (void)
{
	ArvStream *stream;
	GError *error = NULL;
	guint64 n_unavailable_before = simulator_counter ("n-resend-unavailable");
	gint64 end_us;

	/* Every trailer is lost and only the frame being sent is retained. The receiver asks for a trailer when
	 * nothing came for that frame for packet-timeout (20 ms); at 100 fps the frame has rotated out by then, so the
	 * simulator answers with a PACKET_UNAVAILABLE status packet, which the receiver must recognise. */
	g_object_set (simulator,
		      "packet-resend", TRUE,
		      "static-image", TRUE,
		      "frame-retention", 1,
		      "drop-every-nth-packet", LOSS_N_BLOCKS,
		      NULL);

	stream = create_loss_stream (16);
	arv_camera_set_frame_rate (camera, 100.0, &error);
	g_assert_no_error (error);

	arv_camera_start_acquisition (camera, &error);
	g_assert_no_error (error);

	end_us = g_get_monotonic_time () + 1000000;
	while (g_get_monotonic_time () < end_us) {
		ArvBuffer *buffer;

		buffer = arv_stream_timeout_pop_buffer (stream, 200000);
		if (buffer != NULL)
			arv_stream_push_buffer (stream, buffer);
	}

	g_test_message ("resend unavailable: %" G_GUINT64_FORMAT " unavailable sent, %" G_GUINT64_FORMAT
			" error packets, %" G_GUINT64_FORMAT " resend disabled, %" G_GUINT64_FORMAT " completed",
			simulator_counter ("n-resend-unavailable") - n_unavailable_before,
			arv_stream_get_info_uint64_by_name (stream, "n_error_packets"),
			arv_stream_get_info_uint64_by_name (stream, "n_resend_disabled"),
			arv_stream_get_info_uint64_by_name (stream, "n_completed_buffers"));

	g_assert_cmpuint (simulator_counter ("n-resend-unavailable") - n_unavailable_before, >, 0);
	g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "n_error_packets"), >, 0);
	g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "n_resend_disabled"), >, 0);

	g_object_set (simulator, "frame-retention", 4, NULL);
	finish_loss_stream (stream);
}

#define UNRECOVERABLE_FRAME_INTERVAL	5
#define UNRECOVERABLE_HEAD_LENGTH	30
#define UNRECOVERABLE_N_FRAMES		60

static void
unrecoverable_test (void)
{
	ArvStream *stream;
	GError *error = NULL;
	guint n_frames = 0;
	guint n_success = 0;
	guint n_failed = 0;
	guint n_gaps = 0;
	guint last_success_id = 0;
	guint last_missing_id = 0;
	gboolean has_missing_id = FALSE;
	guint n_success_after_last_failure = 0;
	guint64 n_requests_before = simulator_counter ("n-resend-requests");
	guint64 n_resend_dropped_before = simulator_counter ("n-resend-dropped-packets");

	/* Leader and the first 30 of 193 data packets of every 5th frame are lost, and so is every resend: those
	 * frames cannot be completed, every other one must be. 31 packets is under the stream's default request
	 * ratio (0.25 * 195), so the receiver does request them, and the resend loss is exercised. */
	g_object_set (simulator,
		      "packet-resend", TRUE,
		      "static-image", TRUE,
		      "drop-head-frame-interval", UNRECOVERABLE_FRAME_INTERVAL,
		      "drop-head-length", UNRECOVERABLE_HEAD_LENGTH,
		      "resend-lost-ratio", 1.0,
		      NULL);

	stream = create_loss_stream (16);

	arv_camera_start_acquisition (camera, &error);
	g_assert_no_error (error);

	/* Stop on complete frames, to tell "still delivering" from "the last frame was a dropped one" */
	while ((n_frames < UNRECOVERABLE_N_FRAMES || n_success_after_last_failure < 2) &&
	       n_frames < UNRECOVERABLE_N_FRAMES + 2 * UNRECOVERABLE_FRAME_INTERVAL) {
		ArvBuffer *buffer;

		buffer = arv_stream_timeout_pop_buffer (stream, 1000000);
		g_assert (ARV_IS_BUFFER (buffer));

		if (arv_buffer_get_status (buffer) == ARV_BUFFER_STATUS_SUCCESS) {
			guint frame_id = arv_buffer_get_frame_id (buffer);

			g_assert (buffer_has_expected_content (buffer));

			if (n_success > 0 && frame_id != next_frame_id (last_success_id)) {
				guint missing_id = next_frame_id (last_success_id);
				guint k;

				/* Exactly one frame is missing between two complete ones ... */
				g_assert_cmpuint (frame_id, ==, next_frame_id (missing_id));

				/* ... and the missing frames are the dropped ones, one every 5 */
				if (has_missing_id) {
					guint id = last_missing_id;

					for (k = 0; k < UNRECOVERABLE_FRAME_INTERVAL; k++)
						id = next_frame_id (id);
					g_assert_cmpuint (missing_id, ==, id);
				}
				last_missing_id = missing_id;
				has_missing_id = TRUE;
				n_gaps++;
			}

			last_success_id = frame_id;
			n_success++;
			n_success_after_last_failure++;
		} else {
			n_failed++;
			n_success_after_last_failure = 0;
		}

		n_frames++;
		arv_stream_push_buffer (stream, buffer);
	}

	g_test_message ("unrecoverable: %u frames, %u complete, %u failed, %u gaps, "
			"%" G_GUINT64_FORMAT " failures, %" G_GUINT64_FORMAT " timeouts, %" G_GUINT64_FORMAT
			" resend requests, %" G_GUINT64_FORMAT " ratio reached, %" G_GUINT64_FORMAT " underruns",
			n_frames, n_success, n_failed, n_gaps,
			arv_stream_get_info_uint64_by_name (stream, "n_failures"),
			arv_stream_get_info_uint64_by_name (stream, "n_timeouts"),
			arv_stream_get_info_uint64_by_name (stream, "n_resend_requests"),
			arv_stream_get_info_uint64_by_name (stream, "n_resend_ratio_reached"),
			arv_stream_get_info_uint64_by_name (stream, "n_underruns"));

	/* One frame in 5 fails, the first failure may come before the first complete frame */
	g_assert_cmpuint (n_failed, >=, n_frames / UNRECOVERABLE_FRAME_INTERVAL - 1);
	g_assert_cmpuint (n_failed, <=, n_frames / UNRECOVERABLE_FRAME_INTERVAL + 1);
	/* Every failed frame is one of the gaps; the first failure may precede the first complete frame */
	g_assert_cmpuint (n_gaps, <=, n_failed);
	g_assert_cmpuint (n_gaps + 1, >=, n_failed);
	/* The stream keeps delivering after the failures */
	g_assert_cmpuint (n_success_after_last_failure, >=, 2);
	g_assert_cmpuint (simulator_counter ("n-resend-requests") - n_requests_before, >, 0);
	g_assert_cmpuint (simulator_counter ("n-resend-dropped-packets") - n_resend_dropped_before, >, 0);

	finish_loss_stream (stream);
}

#define SLOW_CONSUMER_N_BUFFERS		4
#define SLOW_CONSUMER_STALL_US		300000
#define SLOW_CONSUMER_RECOVERY_US	1000000

static void
slow_consumer_test (void)
{
	ArvStream *stream;
	GError *error = NULL;
	guint n_frames = 0;
	gint64 stall_end_us;
	gint64 recovered_us = 0;
	guint n_after_recovery = 0;
	guint n_failed_after_recovery = 0;

	g_object_set (simulator,
		      "packet-resend", TRUE,
		      "static-image", TRUE,
		      NULL);

	stream = create_loss_stream (SLOW_CONSUMER_N_BUFFERS);

	arv_camera_start_acquisition (camera, &error);
	g_assert_no_error (error);

	/* Steady state first */
	while (n_frames < 10) {
		ArvBuffer *buffer;

		buffer = arv_stream_timeout_pop_buffer (stream, 1000000);
		g_assert (ARV_IS_BUFFER (buffer));
		g_assert_cmpint (arv_buffer_get_status (buffer), ==, ARV_BUFFER_STATUS_SUCCESS);
		n_frames++;
		arv_stream_push_buffer (stream, buffer);
	}

	/* Stall once: every buffer ends up in the output queue and the receiver has nowhere to put the next frames */
	g_usleep (SLOW_CONSUMER_STALL_US);
	stall_end_us = g_get_monotonic_time ();

	while (g_get_monotonic_time () < stall_end_us + 2 * SLOW_CONSUMER_RECOVERY_US) {
		ArvBuffer *buffer;
		gboolean is_new;

		buffer = arv_stream_timeout_pop_buffer (stream, 1000000);
		g_assert (ARV_IS_BUFFER (buffer));

		/* A frame whose leader was processed after the stall ended */
		is_new = arv_buffer_get_status (buffer) == ARV_BUFFER_STATUS_SUCCESS &&
			arv_buffer_get_system_timestamp (buffer) / 1000 > (guint64) stall_end_us;

		if (recovered_us == 0) {
			if (is_new) {
				recovered_us = g_get_monotonic_time ();
				g_assert (buffer_has_expected_content (buffer));
			}
		} else {
			n_after_recovery++;
			if (arv_buffer_get_status (buffer) != ARV_BUFFER_STATUS_SUCCESS)
				n_failed_after_recovery++;
			else
				g_assert (buffer_has_expected_content (buffer));
		}

		arv_stream_push_buffer (stream, buffer);
	}

	g_test_message ("slow consumer: recovered %" G_GINT64_FORMAT " us after the stall, %u frames after recovery,"
			" %u failed, %" G_GUINT64_FORMAT " underruns, %" G_GUINT64_FORMAT " failures",
			recovered_us > 0 ? recovered_us - stall_end_us : -1,
			n_after_recovery, n_failed_after_recovery,
			arv_stream_get_info_uint64_by_name (stream, "n_underruns"),
			arv_stream_get_info_uint64_by_name (stream, "n_failures"));

	g_assert_cmpint (recovered_us, >, 0);
	g_assert_cmpint (recovered_us - stall_end_us, <=, SLOW_CONSUMER_RECOVERY_US);
	/* Once recovered, it stays recovered: about 50 frames in the remaining second, all complete */
	g_assert_cmpuint (n_after_recovery, >=, 25);
	g_assert_cmpuint (n_failed_after_recovery, ==, 0);

	finish_loss_stream (stream);
}

int
main (int argc, char *argv[])
{
	int result;

	g_test_init (&argc, &argv, NULL);

	arv_set_fake_camera_genicam_filename (GENICAM_FILENAME);

	simulator = arv_gv_fake_camera_new ("127.0.0.1", "GVTest");
	g_assert (ARV_IS_GV_FAKE_CAMERA (simulator));

	camera = arv_camera_new ("Aravis-GVTest", NULL);
	g_assert (ARV_IS_CAMERA (camera));

	arv_update_device_list ();

	g_test_add_func ("/fakegv/discovery", discovery_test);
	g_test_add_func ("/fakegv/device_registers", register_test);
	g_test_add_func ("/fakegv/acquisition", acquisition_test);
	g_test_add_func ("/fakegv/stream", stream_test);
	g_test_add_func ("/fakegv/dynamic_roi", dynamic_roi_test);
	g_test_add_func ("/fakegv/resend", resend_test);
	g_test_add_func ("/fakegv/resend-unavailable", resend_unavailable_test);
	g_test_add_func ("/fakegv/unrecoverable", unrecoverable_test);
	g_test_add_func ("/fakegv/slow-consumer", slow_consumer_test);

	result = g_test_run();

	g_object_unref (camera);

	g_object_unref (simulator);

	arv_shutdown ();

	return result;
}

