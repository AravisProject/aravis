/* Aravis - Digital camera library
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* Regression test for the aravissrc teardown deadlock: a GigE Vision stream
 * with packet loss delivers a steady flow of incomplete (non-SUCCESS) buffers,
 * which used to wedge gst_aravis_create() with no way to interrupt it, so
 * set_state(NULL) never returned. Assert teardown completes within a bound. */

#include <arv.h>
#include <gst/gst.h>

/* High enough that multi-packet frames essentially never complete. */
#define TEARDOWN_TEST_LOST_RATIO	0.2

static ArvGvFakeCamera *simulator = NULL;

typedef struct {
	GstElement *pipeline;
	GMutex mutex;
	GCond cond;
	gboolean done;
} TeardownData;

static gpointer
set_state_null_thread (gpointer user_data)
{
	TeardownData *data = user_data;

	/* The call that used to deadlock. */
	gst_element_set_state (data->pipeline, GST_STATE_NULL);

	g_mutex_lock (&data->mutex);
	data->done = TRUE;
	g_cond_signal (&data->cond);
	g_mutex_unlock (&data->mutex);

	return NULL;
}

static void
teardown_under_packet_loss_test (void)
{
	GstElement *pipeline;
	GError *error = NULL;
	TeardownData data = {0};
	GThread *thread;
	gint64 deadline;
	gboolean torn_down;

	pipeline = gst_parse_launch ("aravissrc camera-name=Aravis-GVTest ! fakesink sync=false",
				     &error);
	g_assert_no_error (error);
	g_assert_nonnull (pipeline);

	gst_element_set_state (pipeline, GST_STATE_PLAYING);
	/* Let the streaming task settle into create()'s buffer wait. */
	g_usleep (G_USEC_PER_SEC);

	data.pipeline = pipeline;
	g_mutex_init (&data.mutex);
	g_cond_init (&data.cond);

	thread = g_thread_new ("set-state-null", set_state_null_thread, &data);

	deadline = g_get_monotonic_time () + 5 * G_USEC_PER_SEC;
	g_mutex_lock (&data.mutex);
	while (!data.done) {
		if (!g_cond_wait_until (&data.cond, &data.mutex, deadline))
			break;
	}
	torn_down = data.done;
	g_mutex_unlock (&data.mutex);

	/* Aborts here pre-fix (deadlock) instead of hanging the whole binary. */
	g_assert_true (torn_down);

	g_thread_join (thread);
	gst_object_unref (pipeline);
	g_mutex_clear (&data.mutex);
	g_cond_clear (&data.cond);
}

int
main (int argc, char **argv)
{
	int result;

	gst_init (&argc, &argv);
	g_test_init (&argc, &argv, NULL);

	arv_set_fake_camera_genicam_filename (GENICAM_FILENAME);

	simulator = arv_gv_fake_camera_new ("127.0.0.1", "GVTest");
	g_assert (ARV_IS_GV_FAKE_CAMERA (simulator));
	g_object_set (simulator, "gvsp-lost-ratio", TEARDOWN_TEST_LOST_RATIO, NULL);

	arv_update_device_list ();

	g_test_add_func ("/gstreamer/teardown-under-packet-loss", teardown_under_packet_loss_test);

	result = g_test_run ();

	g_object_unref (simulator);
	arv_shutdown ();

	return result;
}
