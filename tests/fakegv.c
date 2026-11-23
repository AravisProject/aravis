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
		      "frame-retention", 4,
		      "packet-rate", 0,
		      "drop-every-nth-packet", 0,
		      "drop-burst-length", 0,
		      "drop-burst-frame-interval", 0,
		      "drop-head-length", 0,
		      "drop-head-frame-interval", 0,
		      "resend-lost-ratio", 0.0,
		      "frame-id-step", 1,
		      "new-source-port-per-stream", FALSE,
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

static guint64
stream_fast_fail (ArvStream *stream)
{
	return arv_stream_get_info_uint64_by_name (stream, "fast_fail");
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

	/* Every 97 frames (195 * 97 packets) the dropped packet is a trailer. Without fast fail the receiver only
	 * notices a lost trailer by timeout: 20 ms of silence for that frame, then the 1 ms initial timeout,
	 * evaluated on the next packet or poll, so the request comes up to about three frame periods (~60 ms at
	 * 50 fps) late. 16 retained frames keep that request answerable; with the default 4 (80 ms) it is
	 * borderline and the camera sometimes answers PACKET_UNAVAILABLE. With fast fail the trailer is requested
	 * once the next frame's packets arrive, and the simulator's default retention of 4 frames is enough. */
	g_object_set (simulator,
		      "packet-resend", TRUE,
		      "static-image", TRUE,
		      "drop-every-nth-packet", 97,
		      NULL);

	stream = create_loss_stream (8);
	g_object_set (simulator, "frame-retention", stream_fast_fail (stream) ? 4 : 16, NULL);

	arv_camera_start_acquisition (camera, &error);
	g_assert_no_error (error);

	/* About 2 s at 50 fps, with about two lost packets in each 195 packet frame */
	while (n_frames < 100) {
		ArvBuffer *buffer;

		buffer = arv_stream_timeout_pop_buffer (stream, 1000000);
		g_assert (ARV_IS_BUFFER (buffer));

		if (arv_buffer_get_status (buffer) != ARV_BUFFER_STATUS_SUCCESS)
			g_test_message ("resend: frame %u status %d; stream: %" G_GUINT64_FORMAT " missing packets, %"
					G_GUINT64_FORMAT " resend requests, %" G_GUINT64_FORMAT " resent, %"
					G_GUINT64_FORMAT " ratio reached, %" G_GUINT64_FORMAT " timeouts; simulator: %"
					G_GUINT64_FORMAT " dropped, %" G_GUINT64_FORMAT " resent, %" G_GUINT64_FORMAT
					" send errors, %" G_GUINT64_FORMAT " unavailable; stream error packets %" G_GUINT64_FORMAT,
					n_frames, arv_buffer_get_status (buffer),
					arv_stream_get_info_uint64_by_name (stream, "n_missing_packets"),
					arv_stream_get_info_uint64_by_name (stream, "n_resend_requests"),
					arv_stream_get_info_uint64_by_name (stream, "n_resent_packets"),
					arv_stream_get_info_uint64_by_name (stream, "n_resend_ratio_reached"),
					arv_stream_get_info_uint64_by_name (stream, "n_timeouts"),
					simulator_counter ("n-dropped-packets") - n_dropped_before,
					simulator_counter ("n-resent-packets") - n_resent_before,
					simulator_counter ("n-send-errors"),
					simulator_counter ("n-resend-unavailable"),
					arv_stream_get_info_uint64_by_name (stream, "n_error_packets"));
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

#define TRAILER_FRAME_RATE	100.0
#define TRAILER_PACKET_RATE	40000
#define TRAILER_N_FRAMES	100

static void
resend_trailer_test (void)
{
	ArvStream *stream;
	GError *error = NULL;
	guint64 n_unavailable_before = simulator_counter ("n-resend-unavailable");
	guint n_frames = 0;
	guint n_success = 0;
	guint last_frame_id = 0;
	gboolean fast_fail;

	/* Every trailer is lost, and the simulator keeps only 2 frames: frame N can be resent while frame N + 1 is
	 * being sent, not once N + 2 has started. At 100 fps, with the packets of a frame paced over ~5 ms, fast fail
	 * requests the trailer of N 1 ms after the first packet of N + 1, in time. Without fast fail the request
	 * waits for 20 ms of silence for N, by which time N + 2 has replaced it, and it is answered
	 * PACKET_UNAVAILABLE. */
	g_object_set (simulator,
		      "packet-resend", TRUE,
		      "static-image", TRUE,
		      "frame-retention", 2,
		      "packet-rate", TRAILER_PACKET_RATE,
		      "drop-every-nth-packet", LOSS_N_BLOCKS,
		      NULL);

	stream = create_loss_stream (16);
	fast_fail = stream_fast_fail (stream) != 0;
	arv_camera_set_frame_rate (camera, TRAILER_FRAME_RATE, &error);
	g_assert_no_error (error);

	arv_camera_start_acquisition (camera, &error);
	g_assert_no_error (error);

	while (n_frames < TRAILER_N_FRAMES) {
		ArvBuffer *buffer;

		buffer = arv_stream_timeout_pop_buffer (stream, 1000000);
		g_assert (ARV_IS_BUFFER (buffer));

		if (arv_buffer_get_status (buffer) == ARV_BUFFER_STATUS_SUCCESS) {
			g_assert (buffer_has_expected_content (buffer));
			if (fast_fail && n_frames > 0)
				g_assert_cmpuint (arv_buffer_get_frame_id (buffer), ==, next_frame_id (last_frame_id));
			last_frame_id = arv_buffer_get_frame_id (buffer);
			n_success++;
		}

		n_frames++;
		arv_stream_push_buffer (stream, buffer);
	}

	g_test_message ("resend trailer (fast-fail %d): %u frames, %u complete; %" G_GUINT64_FORMAT " resent packets, %"
			G_GUINT64_FORMAT " unavailable, %" G_GUINT64_FORMAT " failures",
			fast_fail, n_frames, n_success,
			arv_stream_get_info_uint64_by_name (stream, "n_resent_packets"),
			simulator_counter ("n-resend-unavailable") - n_unavailable_before,
			arv_stream_get_info_uint64_by_name (stream, "n_failures"));

	if (fast_fail) {
		g_assert_cmpuint (n_success, ==, n_frames);
		g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "n_resent_packets"), >=, n_frames);
	}

	finish_loss_stream (stream);
}

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

	finish_loss_stream (stream);
}

#define UNRECOVERABLE_FRAME_INTERVAL	5
#define UNRECOVERABLE_HEAD_LENGTH	30
#define UNRECOVERABLE_N_FRAMES		60
#define UNRECOVERABLE_FRAME_RETENTION_US	200000
/* ArvGvStream:packet-request-ratio default */
#define LOSS_REQUEST_RATIO		0.25

typedef struct {
	guint n_frames;
	guint n_success;
	guint n_failed;
	guint n_gaps;
	gint64 max_delay_after_gap_us;	/* pop time - leader time, complete frames right after a lost one */
	gint64 max_delay_clean_us;	/* the same, for the other complete frames */
	guint64 n_requested_packets;	/* resend requested packets, counted by the simulator */
	guint64 n_requests;
	guint64 n_resend_dropped;
	guint64 n_skipped_frames;
	guint64 n_failures;
	guint64 n_timeouts;
	guint64 n_ratio_reached;
} UnrecoverableResult;

/* Leader and the first head_length data packets of every 5th frame are lost, and so is every resend: those frames
 * cannot be completed, every other one must be. */
static void
run_unrecoverable (guint head_length, guint frame_retention_us, ArvStream **stream_out, UnrecoverableResult *result)
{
	ArvStream *stream;
	GError *error = NULL;
	guint last_success_id = 0;
	guint last_missing_id = 0;
	gboolean has_missing_id = FALSE;
	gboolean after_gap = FALSE;
	guint n_success_after_last_failure = 0;
	guint64 n_requests_before = simulator_counter ("n-resend-requests");
	guint64 n_requested_packets_before = simulator_counter ("n-resend-requested-packets");
	guint64 n_resend_dropped_before = simulator_counter ("n-resend-dropped-packets");

	*result = (UnrecoverableResult) { 0 };

	g_object_set (simulator,
		      "packet-resend", TRUE,
		      "static-image", TRUE,
		      "drop-head-frame-interval", UNRECOVERABLE_FRAME_INTERVAL,
		      "drop-head-length", head_length,
		      "resend-lost-ratio", 1.0,
		      NULL);

	stream = create_loss_stream (16);
	if (frame_retention_us > 0)
		g_object_set (stream, "frame-retention", frame_retention_us, NULL);

	arv_camera_start_acquisition (camera, &error);
	g_assert_no_error (error);

	/* Stop on complete frames, to tell "still delivering" from "the last frame was a dropped one" */
	while ((result->n_frames < UNRECOVERABLE_N_FRAMES || n_success_after_last_failure < 2) &&
	       result->n_frames < UNRECOVERABLE_N_FRAMES + 2 * UNRECOVERABLE_FRAME_INTERVAL) {
		ArvBuffer *buffer;

		buffer = arv_stream_timeout_pop_buffer (stream, 1000000);
		g_assert (ARV_IS_BUFFER (buffer));

		if (arv_buffer_get_status (buffer) == ARV_BUFFER_STATUS_SUCCESS) {
			guint frame_id = arv_buffer_get_frame_id (buffer);
			gint64 delay_us = g_get_monotonic_time () -
				(gint64) (arv_buffer_get_system_timestamp (buffer) / 1000);

			g_assert (buffer_has_expected_content (buffer));

			if (result->n_success > 0 && frame_id != next_frame_id (last_success_id)) {
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
				result->n_gaps++;
				after_gap = TRUE;
			}

			if (after_gap)
				result->max_delay_after_gap_us = MAX (result->max_delay_after_gap_us, delay_us);
			else if (result->n_success > 0)
				result->max_delay_clean_us = MAX (result->max_delay_clean_us, delay_us);
			after_gap = FALSE;

			last_success_id = frame_id;
			result->n_success++;
			n_success_after_last_failure++;
		} else {
			result->n_failed++;
			n_success_after_last_failure = 0;
		}

		result->n_frames++;
		arv_stream_push_buffer (stream, buffer);
	}

	result->n_requests = simulator_counter ("n-resend-requests") - n_requests_before;
	result->n_requested_packets = simulator_counter ("n-resend-requested-packets") - n_requested_packets_before;
	result->n_resend_dropped = simulator_counter ("n-resend-dropped-packets") - n_resend_dropped_before;
	result->n_skipped_frames = arv_stream_get_info_uint64_by_name (stream, "n_skipped_frames");
	result->n_failures = arv_stream_get_info_uint64_by_name (stream, "n_failures");
	result->n_timeouts = arv_stream_get_info_uint64_by_name (stream, "n_timeouts");
	result->n_ratio_reached = arv_stream_get_info_uint64_by_name (stream, "n_resend_ratio_reached");

	g_test_message ("unrecoverable (head %u, fast-fail %" G_GUINT64_FORMAT "): %u frames, %u complete, %u failed,"
			" %u gaps, %" G_GUINT64_FORMAT " skipped, %" G_GUINT64_FORMAT " failures, %" G_GUINT64_FORMAT
			" timeouts, %" G_GUINT64_FORMAT " requests for %" G_GUINT64_FORMAT " packets, %" G_GUINT64_FORMAT
			" ratio reached; delivery delay: %" G_GINT64_FORMAT " us max after a lost frame, %" G_GINT64_FORMAT
			" us max otherwise",
			head_length, stream_fast_fail (stream),
			result->n_frames, result->n_success, result->n_failed, result->n_gaps,
			result->n_skipped_frames, result->n_failures, result->n_timeouts,
			result->n_requests, result->n_requested_packets, result->n_ratio_reached,
			result->max_delay_after_gap_us, result->max_delay_clean_us);

	/* Every lost frame is one of the gaps; the first loss may precede the first complete frame */
	g_assert_cmpuint (result->n_gaps, >=, result->n_frames / UNRECOVERABLE_FRAME_INTERVAL - 2);
	/* The stream keeps delivering after the losses */
	g_assert_cmpuint (n_success_after_last_failure, >=, 2);

	*stream_out = stream;
}

static void
unrecoverable_test (void)
{
	ArvStream *stream;
	UnrecoverableResult result;

	/* 31 packets is under the stream's default request ratio (0.25 * 195), so the receiver does request them, and
	 * the resend loss is exercised. */
	run_unrecoverable (UNRECOVERABLE_HEAD_LENGTH, UNRECOVERABLE_FRAME_RETENTION_US, &stream, &result);

	/* One frame in 5 fails, the first failure may come before the first complete frame */
	g_assert_cmpuint (result.n_failed, >=, result.n_frames / UNRECOVERABLE_FRAME_INTERVAL - 1);
	g_assert_cmpuint (result.n_failed, <=, result.n_frames / UNRECOVERABLE_FRAME_INTERVAL + 1);
	/* Every failed frame is one of the gaps; the first failure may precede the first complete frame */
	g_assert_cmpuint (result.n_gaps, <=, result.n_failed);
	g_assert_cmpuint (result.n_gaps + 1, >=, result.n_failed);
	g_assert_cmpuint (result.n_requests, >, 0);
	g_assert_cmpuint (result.n_resend_dropped, >, 0);

	/* The lost frame is requested once; the second request, a packet-timeout (20 ms) later, exceeds the budget.
	 * With fast fail the frame is closed then, because newer frames exist, and the complete frames behind it are
	 * delivered without waiting out the frame-retention, here 200 ms so that the bound has room on a loaded
	 * machine. Without, the lost frame holds its buffer and the four frames behind it until frame-retention has
	 * passed since its last packet: ~200 ms for the next frame, measured from its leader to its delivery. */
	if (stream_fast_fail (stream)) {
		g_assert_cmpint (result.max_delay_after_gap_us, <, UNRECOVERABLE_FRAME_RETENTION_US / 2);
		g_assert_cmpint (result.max_delay_clean_us, <, UNRECOVERABLE_FRAME_RETENTION_US / 2);
	}

	finish_loss_stream (stream);
}

/* More than the resend budget (0.25 * 195 = 48.75 packets) */
#define UNRECOVERABLE_LONG_HEAD_LENGTH	60

static void
unrecoverable_head_test (void)
{
	ArvStream *stream;
	UnrecoverableResult result;

	/* Leader and 60 data packets of every 5th frame are lost: the first packet the receiver sees of such a frame
	 * is packet 61, and requesting the head would exceed the budget. With fast fail the frame is skipped without
	 * taking a buffer (only the very first frame of the stream, before any frame size is known, may be started);
	 * without, it takes a buffer, requests nothing and times out. */
	run_unrecoverable (UNRECOVERABLE_LONG_HEAD_LENGTH, 0, &stream, &result);

	if (stream_fast_fail (stream)) {
		g_assert_cmpuint (result.n_skipped_frames, >=, result.n_gaps);
		g_assert_cmpuint (result.n_failed, <=, 1);
		g_assert_cmpuint (result.n_failures, <=, 1);
		g_assert_cmpuint (result.n_requests, ==, 0);
	} else {
		g_assert_cmpuint (result.n_skipped_frames, ==, 0);
		g_assert_cmpuint (result.n_failed, >=, result.n_frames / UNRECOVERABLE_FRAME_INTERVAL - 1);
	}

	finish_loss_stream (stream);
}

static void
resend_budget_test (void)
{
	ArvStream *stream;
	UnrecoverableResult result;
	double budget = LOSS_REQUEST_RATIO * LOSS_N_BLOCKS;

	/* Each lost frame misses 31 packets and no resend ever arrives. The budget (0.25 * 195 = 48.75 packets) allows
	 * one request for the 31, not a second one; before the budget was cumulative every request was checked alone
	 * and each lost frame cost ~3 x 31 = 93 requested packets. Up to two lost frames may still be open when the
	 * count is read. */
	run_unrecoverable (UNRECOVERABLE_HEAD_LENGTH, 0, &stream, &result);

	g_test_message ("resend budget: %" G_GUINT64_FORMAT " requested packets for %u failed frames (%.1f per frame,"
			" budget %.2f)", result.n_requested_packets, result.n_failed,
			result.n_failed > 0 ? (double) result.n_requested_packets / result.n_failed : 0.0, budget);

	g_assert_cmpuint (result.n_failed, >, 5);
	g_assert_cmpuint (result.n_requested_packets, >, 0);
	g_assert_cmpfloat ((double) result.n_requested_packets, <=, budget * (result.n_failed + 2));

	finish_loss_stream (stream);
}

/* Datagrams sent to the stream port from a socket of the test, as any host on the link could */

static GSocket *
create_injection_socket (ArvStream *stream, GSocketAddress **destination)
{
	GInetAddress *loopback;
	GSocketAddress *local;
	GSocket *socket;
	GError *error = NULL;

	loopback = g_inet_address_new_from_string ("127.0.0.1");
	socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, &error);
	g_assert_no_error (error);
	local = g_inet_socket_address_new (loopback, 0);
	g_socket_bind (socket, local, FALSE, &error);
	g_assert_no_error (error);
	*destination = g_inet_socket_address_new (loopback, arv_gv_stream_get_port (ARV_GV_STREAM (stream)));
	g_object_unref (local);
	g_object_unref (loopback);

	return socket;
}

static void
inject (GSocket *socket, GSocketAddress *destination, const guint8 *data, gsize size)
{
	GError *error = NULL;

	g_assert_cmpint (g_socket_send_to (socket, destination, (const char *) data, size, NULL, &error), ==, size);
	g_assert_no_error (error);
}

/* A GVSP packet with the standard header: status, 16 bit frame id, content type and 24 bit packet id */
static gsize
gvsp_header (guint8 *data, guint16 status, guint16 frame_id, guint8 content_type, guint32 packet_id)
{
	data[0] = status >> 8;
	data[1] = status & 0xff;
	data[2] = frame_id >> 8;
	data[3] = frame_id & 0xff;
	data[4] = content_type;
	data[5] = (packet_id >> 16) & 0xff;
	data[6] = (packet_id >> 8) & 0xff;
	data[7] = packet_id & 0xff;

	return 8;
}

static guint64
wait_for_info (ArvStream *stream, const char *name, guint64 value, gint64 timeout_us)
{
	gint64 end_us = g_get_monotonic_time () + timeout_us;
	guint64 current;

	while ((current = arv_stream_get_info_uint64_by_name (stream, name)) < value &&
	       g_get_monotonic_time () < end_us)
		g_usleep (1000);

	return current;
}

#define N_SHORT_DATAGRAMS	5

static void
send_short_datagrams (GSocket *socket, GSocketAddress *destination)
{
	guint8 data[64] = { 0 };

	/* 0, 1 and 7 bytes: less than the 8 byte standard header */
	inject (socket, destination, data, 0);
	inject (socket, destination, data, 1);
	gvsp_header (data, 0, 1, 0x03, 5);
	inject (socket, destination, data, 7);
	/* A leader header with no leader after it */
	inject (socket, destination, data, gvsp_header (data, 0, 1, 0x01, 0));
	/* The extended id flag set, in 12 bytes of a 20 byte header */
	gvsp_header (data, 0, 0, 0x81, 0);
	inject (socket, destination, data, 12);
}

static void
short_datagrams_test (void)
{
	ArvStream *stream;
	GSocket *socket;
	GSocketAddress *destination;
	GError *error = NULL;
	guint n_frames = 0;
	guint64 n_expected;

	g_object_set (simulator,
		      "packet-resend", TRUE,
		      "static-image", TRUE,
		      NULL);

	stream = create_loss_stream (8);
	socket = create_injection_socket (stream, &destination);

	/* The stream thread runs from the stream's acquisition start: start it alone, the camera later */
	arv_stream_start_acquisition (stream, &error);
	g_assert_no_error (error);

	/* Before the acquisition: the stream socket is not connected to the camera yet (and none of these may
	 * become the sender it connects to), so all of them reach the receiver */
	send_short_datagrams (socket, destination);
	/* recvmsg_x hands the receiver every datagram; GLib's g_socket_receive_messages (the standard loop) consumes
	 * a 0 byte datagram without returning it, so that one never reaches the receiver there */
	n_expected = arv_stream_get_info_uint64_by_name (stream, "rx_mode") == 1 ?
		N_SHORT_DATAGRAMS : N_SHORT_DATAGRAMS - 1;
	g_assert_cmpuint (wait_for_info (stream, "n_short_packets", n_expected, 1000000), ==, n_expected);
	g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "n_received_packets"), ==, 0);
	g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "socket_connected"), ==, 0);

	/* The stream is already started: only the camera */
	arv_camera_execute_command (camera, "AcquisitionStart", &error);
	g_assert_no_error (error);

	while (n_frames < 20) {
		ArvBuffer *buffer;

		/* During the stream: they reach the receiver unless its socket is connected to the camera */
		if (n_frames % 5 == 0)
			send_short_datagrams (socket, destination);

		buffer = arv_stream_timeout_pop_buffer (stream, 1000000);
		g_assert (ARV_IS_BUFFER (buffer));
		g_assert_cmpint (arv_buffer_get_status (buffer), ==, ARV_BUFFER_STATUS_SUCCESS);
		g_assert (buffer_has_expected_content (buffer));
		n_frames++;
		arv_stream_push_buffer (stream, buffer);
	}

	g_test_message ("short datagrams: %" G_GUINT64_FORMAT " rejected, socket connected %" G_GUINT64_FORMAT,
			arv_stream_get_info_uint64_by_name (stream, "n_short_packets"),
			arv_stream_get_info_uint64_by_name (stream, "socket_connected"));

	g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "n_short_packets"), >=, n_expected);
	g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "n_failures"), ==, 0);

	g_object_unref (destination);
	g_object_unref (socket);
	finish_loss_stream (stream);
}

static void
inject_leader (GSocket *socket, GSocketAddress *destination, guint16 frame_id)
{
	guint8 data[64] = { 0 };
	gsize size = gvsp_header (data, 0, frame_id, 0x01, 0);

	/* flags, payload type (image), 64 bit timestamp, pixel format, width, height, x, y, x and y padding */
	data[size + 3] = 0x01;
	size += 12;
	data[size + 0] = 0x01; data[size + 1] = 0x08; data[size + 2] = 0x00; data[size + 3] = 0x01; /* Mono8 */
	data[size + 6] = LOSS_WIDTH >> 8;
	data[size + 10] = LOSS_HEIGHT >> 8;
	size += 24;

	inject (socket, destination, data, size);
}

static void
inject_trailer (GSocket *socket, GSocketAddress *destination, guint16 frame_id, guint32 packet_id)
{
	guint8 data[64] = { 0 };
	gsize size = gvsp_header (data, 0, frame_id, 0x02, packet_id);

	/* payload type (image), size y */
	data[size + 3] = 0x01;
	size += 8;

	inject (socket, destination, data, size);
}

static void
inject_payload (GSocket *socket, GSocketAddress *destination, guint16 frame_id, guint32 packet_id)
{
	guint8 data[8 + 100] = { 0 };

	gvsp_header (data, 0, frame_id, 0x03, packet_id);
	inject (socket, destination, data, sizeof (data));
}

static void
small_packet_count_test (void)
{
	ArvStream *stream;
	GSocket *socket;
	GSocketAddress *destination;
	GError *error = NULL;
	guint n_buffers = 0;
	guint i;

	g_object_set (simulator, "packet-resend", TRUE, NULL);
	stream = create_loss_stream (8);
	socket = create_injection_socket (stream, &destination);

	/* The stream thread runs from the stream's acquisition start; the camera is not started */
	arv_stream_start_acquisition (stream, &error);
	g_assert_no_error (error);

	/* No acquisition: every packet comes from the test. Frame 100 is a leader alone; it holds the frames behind
	 * it open until it times out (frame-retention, 100 ms). */
	inject_leader (socket, destination, 100);

	/* Frame 101 starts with a trailer at packet id 0, which makes it a one packet frame. Its n_packets - 2 used
	 * to wrap, so that a data packet at any id passed the range check, and packet 1000 was copied 1.4 MB into a
	 * 256 kB buffer with a negative length. */
	inject_trailer (socket, destination, 101, 0);
	inject_payload (socket, destination, 101, 1000);

	/* Frame 102: packets 0, 1, 2, 4 and 5, then a trailer at 3. The contiguous block ends at 5, above the
	 * trailer's n_packets of 4: the frame cannot complete, and when it fails its missing packet count,
	 * n_packets - (last_valid_packet + 1) = -2, used to wrap n_missing_packets. */
	inject_leader (socket, destination, 102);
	inject_payload (socket, destination, 102, 1);
	inject_payload (socket, destination, 102, 2);
	inject_payload (socket, destination, 102, 4);
	inject_payload (socket, destination, 102, 5);
	inject_trailer (socket, destination, 102, 3);

	for (i = 0; i < 3; i++) {
		ArvBuffer *buffer = arv_stream_timeout_pop_buffer (stream, 1000000);

		if (buffer == NULL)
			break;
		g_test_message ("small packet count: frame %" G_GUINT64_FORMAT " status %d",
				arv_buffer_get_frame_id (buffer), arv_buffer_get_status (buffer));
		n_buffers++;
		arv_stream_push_buffer (stream, buffer);
	}

	g_test_message ("small packet count: %u buffers, %" G_GUINT64_FORMAT " received, %" G_GUINT64_FORMAT
			" missing packets, %" G_GUINT64_FORMAT " failures",
			n_buffers, arv_stream_get_info_uint64_by_name (stream, "n_received_packets"),
			arv_stream_get_info_uint64_by_name (stream, "n_missing_packets"),
			arv_stream_get_info_uint64_by_name (stream, "n_failures"));

	g_assert_cmpuint (n_buffers, ==, 3);
	g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "n_received_packets"), ==, 9);
	/* Frame 100 misses 194 of its 195 packets, frame 102 none below its trailer */
	g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "n_missing_packets"), ==, LOSS_N_BLOCKS - 1);

	arv_stream_stop_acquisition (stream, &error);
	g_assert_no_error (error);
	g_object_unref (destination);
	g_object_unref (socket);
	g_object_unref (stream);
	reset_simulator ();
}

static void
pop_complete_frames (ArvStream *stream, guint n_frames, guint64 timeout_us, const char *phase)
{
	guint i;

	for (i = 0; i < n_frames; i++) {
		ArvBuffer *buffer = arv_stream_timeout_pop_buffer (stream, timeout_us);

		if (buffer == NULL)
			g_test_message ("source relearn: no frame %u %s; %" G_GUINT64_FORMAT " received packets, %"
					G_GUINT64_FORMAT " reconnects, connected %" G_GUINT64_FORMAT, i, phase,
					arv_stream_get_info_uint64_by_name (stream, "n_received_packets"),
					arv_stream_get_info_uint64_by_name (stream, "n_socket_reconnects"),
					arv_stream_get_info_uint64_by_name (stream, "socket_connected"));
		g_assert (ARV_IS_BUFFER (buffer));
		g_assert_cmpint (arv_buffer_get_status (buffer), ==, ARV_BUFFER_STATUS_SUCCESS);
		g_assert (buffer_has_expected_content (buffer));
		arv_stream_push_buffer (stream, buffer);
	}
}

static void
source_relearn_test (void)
{
	ArvStream *stream;
	GError *error = NULL;
	gboolean connected;
	guint64 n_reconnects;

	/* Every acquisition comes from a new source port, as from a device that rebooted. A stream socket connected
	 * to the first one would see nothing of the second. */
	g_object_set (simulator,
		      "packet-resend", TRUE,
		      "static-image", TRUE,
		      "new-source-port-per-stream", TRUE,
		      NULL);

	stream = create_loss_stream (8);

	arv_camera_start_acquisition (camera, &error);
	g_assert_no_error (error);
	pop_complete_frames (stream, 5, 1000000, "at the start");
	connected = arv_stream_get_info_uint64_by_name (stream, "socket_connected") != 0;
	/* The camera only: arv_camera_stop_acquisition would also end the stream thread */
	arv_camera_execute_command (camera, "AcquisitionStop", &error);
	g_assert_no_error (error);

	/* An idle connected socket learns the sender again after 1 s */
	if (connected)
		g_assert_cmpuint (wait_for_info (stream, "n_socket_reconnects", 1, 3000000), ==, 1);
	else
		g_usleep (1200000);

	arv_camera_execute_command (camera, "AcquisitionStart", &error);
	g_assert_no_error (error);
	pop_complete_frames (stream, 5, 1000000, "after the idle gap");

	/* A stream thread restart (arv_camera_stop_acquisition ends it, arv_camera_start_acquisition starts a new
	 * one) learns it again at once: 0.5 s is less than the idle threshold */
	arv_camera_stop_acquisition (camera, &error);
	g_assert_no_error (error);
	arv_camera_start_acquisition (camera, &error);
	g_assert_no_error (error);
	pop_complete_frames (stream, 5, 500000, "after the thread restart");

	n_reconnects = arv_stream_get_info_uint64_by_name (stream, "n_socket_reconnects");
	g_test_message ("source relearn: connected %d, %" G_GUINT64_FORMAT " reconnects, %" G_GUINT64_FORMAT
			" foreign sources", connected, n_reconnects,
			arv_stream_get_info_uint64_by_name (stream, "n_rx_foreign_source"));

	if (connected) {
		g_assert_cmpuint (n_reconnects, ==, 2);
		g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "socket_connected"), ==, 1);
	} else
		g_assert_cmpuint (n_reconnects, ==, 0);

	finish_loss_stream (stream);
}

/* 65535 / 3: the simulator's frame ids repeat every third frame */
#define REPEATED_ID_STEP		21845
#define REPEATED_ID_DROP_INTERVAL	4
#define REPEATED_ID_N_FRAMES		60

static void
repeated_frame_id_test (void)
{
	ArvStream *stream;
	GError *error = NULL;
	guint64 n_sent_before;
	guint64 n_sent;
	guint64 n_skipped;
	guint n_success = 0;
	guint n_failed = 0;
	gint64 end_us;

	/* Every 4th frame loses its leader and 60 data packets, more than the resend budget, so fast fail skips it,
	 * and the frame ids repeat every third frame. A skipped frame's id therefore comes round again three frames
	 * later, in a frame that is sent whole: the same thing happens on a real stream once its 16 bit ids wrap
	 * (every 65535 frames, 298 s at 220 fps) after a skip, or when a device restarts its ids. That frame must be
	 * received, not taken for the rest of the skipped one. */
	g_object_set (simulator,
		      "packet-resend", TRUE,
		      "static-image", TRUE,
		      "frame-id-step", REPEATED_ID_STEP,
		      "drop-head-frame-interval", REPEATED_ID_DROP_INTERVAL,
		      "drop-head-length", UNRECOVERABLE_LONG_HEAD_LENGTH,
		      "resend-lost-ratio", 1.0,
		      NULL);

	stream = create_loss_stream (16);
	if (!stream_fast_fail (stream)) {
		/* Without fast fail no frame is skipped, and an open frame with a lost head would collect the packets
		 * of the frame that reuses its id three frames later: the scenario only exists with fast fail. */
		g_object_unref (stream);
		reset_simulator ();
		g_test_skip ("fast fail is off");
		return;
	}

	n_sent_before = simulator_counter ("n-sent-frames");

	arv_camera_start_acquisition (camera, &error);
	g_assert_no_error (error);

	end_us = g_get_monotonic_time () + 5000000;
	while (n_success + n_failed < REPEATED_ID_N_FRAMES && g_get_monotonic_time () < end_us) {
		ArvBuffer *buffer;

		buffer = arv_stream_timeout_pop_buffer (stream, 200000);
		if (buffer == NULL)
			continue;

		if (arv_buffer_get_status (buffer) == ARV_BUFFER_STATUS_SUCCESS) {
			g_assert (buffer_has_expected_content (buffer));
			n_success++;
		} else
			n_failed++;

		arv_stream_push_buffer (stream, buffer);
	}

	arv_camera_stop_acquisition (camera, &error);
	g_assert_no_error (error);
	/* Let the last frame in flight be processed */
	g_usleep (100000);

	n_sent = simulator_counter ("n-sent-frames") - n_sent_before;
	n_skipped = arv_stream_get_info_uint64_by_name (stream, "n_skipped_frames");

	g_test_message ("repeated frame id: %" G_GUINT64_FORMAT " frames sent, %u complete, %u failed, %"
			G_GUINT64_FORMAT " skipped, %" G_GUINT64_FORMAT " completed, %" G_GUINT64_FORMAT
			" ignored packets",
			n_sent, n_success, n_failed, n_skipped,
			arv_stream_get_info_uint64_by_name (stream, "n_completed_buffers"),
			arv_stream_get_info_uint64_by_name (stream, "n_ignored_packets"));

	g_assert_cmpuint (n_sent, >=, 2 * REPEATED_ID_DROP_INTERVAL);
	g_assert_cmpuint (n_skipped, >, 0);
	/* Only the frames that lost their head are missing: one in four, give or take the frames in flight when the
	 * acquisition stopped. Swallowing the frames that reuse a skipped id would lose one more in four. */
	g_assert_cmpuint (n_skipped, <=, n_sent / REPEATED_ID_DROP_INTERVAL + 1);
	g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "n_completed_buffers") + n_skipped + 2, >=,
			  n_sent);
	g_assert_cmpuint (n_failed, ==, 0);

	g_object_unref (stream);
	reset_simulator ();
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
	guint n_after_stall = 0;
	guint n_failed_after_stall = 0;
	guint64 n_skipped_frames;

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

		n_after_stall++;
		if (arv_buffer_get_status (buffer) != ARV_BUFFER_STATUS_SUCCESS)
			n_failed_after_stall++;

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

	n_skipped_frames = arv_stream_get_info_uint64_by_name (stream, "n_skipped_frames");

	g_test_message ("slow consumer (fast-fail %" G_GUINT64_FORMAT "): recovered %" G_GINT64_FORMAT " us after the"
			" stall, %u frames after recovery, %u failed; %u frames after the stall, %u failed; %"
			G_GUINT64_FORMAT " underruns in %" G_GUINT64_FORMAT " frames, %" G_GUINT64_FORMAT " skipped, %"
			G_GUINT64_FORMAT " failures",
			stream_fast_fail (stream),
			recovered_us > 0 ? recovered_us - stall_end_us : -1,
			n_after_recovery, n_failed_after_recovery,
			n_after_stall, n_failed_after_stall,
			arv_stream_get_info_uint64_by_name (stream, "n_underruns"),
			arv_stream_get_info_uint64_by_name (stream, "n_underrun_frames"),
			n_skipped_frames,
			arv_stream_get_info_uint64_by_name (stream, "n_failures"));

	if (stream_fast_fail (stream)) {
		/* The frames that found no buffer during the stall are dropped whole: none is started mid-way when
		 * the consumer returns buffers, so every frame delivered after the stall, the first ones included, is
		 * complete. */
		g_assert_cmpuint (n_skipped_frames, >, 0);
		g_assert_cmpuint (n_skipped_frames, ==,
				  arv_stream_get_info_uint64_by_name (stream, "n_underrun_frames"));
		g_assert_cmpuint (n_failed_after_stall, ==, 0);
		g_assert_cmpuint (arv_stream_get_info_uint64_by_name (stream, "n_failures"), ==, 0);
	} else
		g_assert_cmpuint (n_skipped_frames, ==, 0);

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
	g_test_add_func ("/fakegv/resend-trailer", resend_trailer_test);
	g_test_add_func ("/fakegv/resend-unavailable", resend_unavailable_test);
	g_test_add_func ("/fakegv/unrecoverable", unrecoverable_test);
	g_test_add_func ("/fakegv/resend-budget", resend_budget_test);
	g_test_add_func ("/fakegv/unrecoverable-head", unrecoverable_head_test);
	g_test_add_func ("/fakegv/slow-consumer", slow_consumer_test);
	g_test_add_func ("/fakegv/repeated-frame-id", repeated_frame_id_test);
	g_test_add_func ("/fakegv/short-datagrams", short_datagrams_test);
	g_test_add_func ("/fakegv/small-packet-count", small_packet_count_test);
	g_test_add_func ("/fakegv/source-relearn", source_relearn_test);

	result = g_test_run();

	g_object_unref (camera);

	g_object_unref (simulator);

	arv_shutdown ();

	return result;
}

