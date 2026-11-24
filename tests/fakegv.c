/* SPDX-License-Identifier:Unlicense */

#include <glib.h>
#include <arv.h>
#include "../src/arvgvcpprivate.h"

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

#if ARAVIS_HAS_EVENT

#define EVENT_TEST_N_EVENTS	5
#define EVENT_TEST_TIMEOUT_US	10000000

typedef struct {
	GThread *thread;
	guint n_events;
	gint64 last_frame_id;
	gboolean is_duplicated;
	gboolean is_wrong_thread;
	gboolean is_wrong_data;
} EventTestData;

static void
event_test_callback (void *user_data, ArvEvent *event)
{
	EventTestData *data = user_data;
	GError *error = NULL;
	gint64 frame_id;
	gint64 timestamp;

	if (g_thread_self () != data->thread)
		data->is_wrong_thread = TRUE;

	g_assert_cmpuint (arv_event_get_id (event), ==, ARV_FAKE_CAMERA_EVENT_EXPOSURE_END);

	frame_id = arv_camera_get_integer (camera, "EventExposureEndFrameID", &error);
	g_assert_no_error (error);
	timestamp = arv_camera_get_integer (camera, "EventExposureEndTimestamp", &error);
	g_assert_no_error (error);

	if (frame_id != (gint64) (arv_event_get_block_id (event) & 0xffff) ||
	    timestamp != (gint64) arv_event_get_timestamp (event))
		data->is_wrong_data = TRUE;

	if (data->n_events > 0 && frame_id == data->last_frame_id)
		data->is_duplicated = TRUE;

	data->last_frame_id = frame_id;
	data->n_events++;
}

static void
event_test_run (double lost_ack_ratio)
{
	EventTestData data = {0};
	ArvFakeCamera *fake_camera;
	ArvDevice *device;
	ArvStream *stream;
	GError *error = NULL;
	gint64 start_time;
	guint32 port = 0;
	size_t payload;
	int i;

	device = arv_camera_get_device (camera);
	fake_camera = arv_gv_fake_camera_get_fake_camera (simulator);

	/* The message channel is configured at device creation */
	arv_fake_camera_read_register (fake_camera, ARV_GVBS_MESSAGE_CHANNEL_0_PORT_OFFSET, &port);
	g_assert_cmpuint (port, !=, 0);

	g_object_set (simulator, "gvcp-event-lost-ack-ratio", lost_ack_ratio, NULL);

	data.thread = g_thread_self ();

	arv_camera_set_event_notification (camera, "ExposureEnd", TRUE, &error);
	g_assert_no_error (error);
	arv_device_set_event_callback (device, event_test_callback, &data, NULL);

	stream = arv_camera_create_stream (camera, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	payload = arv_camera_get_payload (camera, &error);
	g_assert_no_error (error);
	for (i = 0; i < 5; i++)
		arv_stream_push_buffer (stream, arv_buffer_new (payload, NULL));

	arv_camera_set_acquisition_mode (camera, ARV_ACQUISITION_MODE_CONTINUOUS, &error);
	g_assert_no_error (error);
	arv_camera_start_acquisition (camera, &error);
	g_assert_no_error (error);

	start_time = g_get_monotonic_time ();
	while (data.n_events < EVENT_TEST_N_EVENTS &&
	       g_get_monotonic_time () - start_time < EVENT_TEST_TIMEOUT_US) {
		ArvBuffer *buffer;

		buffer = arv_stream_timeout_pop_buffer (stream, 100000);
		if (buffer != NULL)
			arv_stream_push_buffer (stream, buffer);

		while (g_main_context_iteration (NULL, FALSE));
	}

	arv_camera_stop_acquisition (camera, &error);
	g_assert_no_error (error);

	arv_camera_set_event_notification (camera, "ExposureEnd", FALSE, &error);
	g_assert_no_error (error);

	g_object_unref (stream);

	/* Late events */
	g_usleep (100000);
	while (g_main_context_iteration (NULL, FALSE));

	arv_device_set_event_callback (device, NULL, NULL, NULL);
	g_object_set (simulator, "gvcp-event-lost-ack-ratio", 0.0, NULL);

	g_assert_cmpuint (data.n_events, >=, EVENT_TEST_N_EVENTS);
	g_assert_false (data.is_wrong_thread);
	g_assert_false (data.is_wrong_data);
	g_assert_false (data.is_duplicated);
	g_assert_cmpuint (arv_device_get_n_dropped_events (device), ==, 0);
}

static void
event_test (void)
{
	event_test_run (0.0);
}

static void
event_retransmission_test (void)
{
	/* All acknowledges are ignored by the fake camera, which retransmits each event */
	event_test_run (1.0);
}

#endif

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
#if ARAVIS_HAS_EVENT
	g_test_add_func ("/fakegv/event", event_test);
	g_test_add_func ("/fakegv/event-retransmission", event_retransmission_test);
#endif

	result = g_test_run();

	g_object_unref (camera);

#if ARAVIS_HAS_EVENT
	{
		guint32 port = 0xffff;

		/* The message channel is closed at device destruction */
		arv_fake_camera_read_register (arv_gv_fake_camera_get_fake_camera (simulator),
					       ARV_GVBS_MESSAGE_CHANNEL_0_PORT_OFFSET, &port);
		g_assert_cmpuint (port, ==, 0);
	}
#endif

	g_object_unref (simulator);

	arv_shutdown ();

	return result;
}

