/* The goal of this example is to create a pipeline with an aravis source, and while keeping the pipeline alive, remove
 * the source and replace it with a newly created  aravis source. */

#include <gst/gst.h>
#include <unistd.h>
#include <arv.h>

#define USE_FAKE_SOURCE 0

static void
drain_bus (GstElement *pipeline)
{
        GstBus *bus = gst_element_get_bus (pipeline);
        GstMessage *msg;

        while ((msg = gst_bus_pop (bus)) != NULL) {
                if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
                        GError *err = NULL;
                        gchar *dbg = NULL;
                        gst_message_parse_error (msg, &err, &dbg);
                        g_printerr ("Error from %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
                        g_clear_error (&err);
                        g_free (dbg);
                }
                gst_message_unref (msg);   /* releases the reference to the source */
        }
        gst_object_unref (bus);
}

static void
on_source_finalized (gpointer user_data, GObject *where_the_object_was)
{
        gchar *name = user_data;
        g_print ("*** Element '%s' (%p) has been finalized\n",
                 name, (void *) where_the_object_was);
        g_free (name);
}

static GstElement *
make_source (const gchar *name)
#if USE_FAKE_SOURCE
{
        GstElement *source;

       source = gst_element_factory_make ("fakesrc", name);
        if (!source) {
                g_printerr ("Failed to create the fakesrc element\n");
                return NULL;
        }
        g_object_weak_ref (G_OBJECT (source), on_source_finalized, g_strdup (name));

        g_object_set (source,
                      "sync", TRUE,
                      "datarate", 10240,
                      "sizemax", 1024,
                      NULL);
        gst_util_set_object_arg (G_OBJECT (source), "sizetype", "fixed");

        return source;
}
#else
{
        GstElement *source;

        source = gst_element_factory_make ("aravissrc", name);
        if (!source) {
                g_printerr ("Failed to create the aravissrc element\n");
                return NULL;
        }
        g_object_weak_ref (G_OBJECT (source), on_source_finalized, g_strdup (name));

        g_object_set(G_OBJECT(source),
                     "camera-name", "Fake_1",
                     "num-arv-buffers", 5,
                     "packet-resend", FALSE,
                     "packet-size", 9000,
                     "auto-packet-size", FALSE,
                     NULL);

        return source;
}
#endif

static void
on_handoff (GstElement *sink, GstBuffer *buffer, GstPad *pad, gpointer user_data)
{
        static unsigned buffer_count = 0;

        buffer_count++;
        g_print ("  fakesink got buffer #%u (%" G_GSIZE_FORMAT " bytes)\n",
                 buffer_count, gst_buffer_get_size (buffer));
}

static gboolean
wait_for_state (GstElement *pipeline, GstState target)
{
        GstState state;
        GstStateChangeReturn ret;

        ret= gst_element_get_state (pipeline, &state, NULL, 5 * GST_SECOND);

        return ret != GST_STATE_CHANGE_FAILURE && state == target;
}

static void
swap_source_test (void)
{
        GstElement *pipeline;
        GstElement *source;
        GstElement *sink;
        int i;

        /* Create the initial pipeline */

        g_print ("Pipeline creation\n");

        pipeline = gst_pipeline_new ("aravis-pipeline");
        source   = make_source ("source-0");
        sink = gst_element_factory_make ("fakesink", "fake_sink");
        g_object_set (sink,
                      "sync", TRUE,
                      "signal-handoffs", TRUE,
                      NULL);
        g_signal_connect (sink, "handoff", G_CALLBACK (on_handoff), NULL);

        if (!pipeline || !source || !sink) {
                g_printerr ("Not all elements could be created.\n");
                g_assert_not_reached();
        }

        gst_bin_add_many (GST_BIN (pipeline), source, sink, NULL);
        if (!gst_element_link (source, sink)) {
                g_printerr ("Elements could not be linked.\n");
                gst_object_unref (pipeline);
                g_assert_not_reached();
        }

        /* Start it */

        g_print ("Start the pipeline\n");
        if (gst_element_set_state (pipeline, GST_STATE_PLAYING)
            == GST_STATE_CHANGE_FAILURE || !wait_for_state (pipeline, GST_STATE_PLAYING)) {
                g_printerr ("Failed to start pipeline\n");
                gst_object_unref (pipeline);
                g_assert_not_reached();
        }

        g_print ("Wait for a few frames\n");
        sleep (1);

        for (i = 1; i <= 5; i++) {
                char *name;

                /* Now we are trying to swap the source of the pipeline. First, we stop the source */

                g_print ("Remove the source\n");
                g_assert_cmpint (gst_element_set_state (source, GST_STATE_NULL), !=, GST_STATE_CHANGE_FAILURE);
                gst_element_unlink (source, sink);
                g_assert (gst_bin_remove (GST_BIN (pipeline), source));
                source = NULL;

                /* Allow the destuction of the removed source */

                drain_bus (pipeline);

                /* Then we create a new source */

                g_print ("Create a new source\n");

                name = g_strdup_printf ("source-%i", i);
                source   = make_source (name);
                g_free (name);

                /* Add it to the pipeline */

                g_assert (gst_bin_add(GST_BIN(pipeline), source));
                if (!gst_element_link (source, sink)) {
                        g_printerr ("Could not link new source to sink\n");
                        gst_element_set_state (pipeline, GST_STATE_NULL);
                        gst_object_unref (pipeline);
                        g_assert_not_reached();
                }

                if (!gst_element_sync_state_with_parent (source)) {
                        g_printerr ("Could not sync new source state with pipeline\n");
                        gst_element_set_state (pipeline, GST_STATE_NULL);
                        gst_object_unref (pipeline);
                        g_assert_not_reached();
                }

                g_print ("Wait for a few frames\n");
                sleep (1);
        }

        /* Free everything now */

        g_print ("Stop the pipeline\n");
        g_assert_cmpint (gst_element_set_state (pipeline, GST_STATE_NULL), ==, GST_STATE_CHANGE_SUCCESS);

        g_print ("Free the pipeline\n");
        gst_object_unref (pipeline);
}

static void
signals_test (void)
{
        GstElement *pipeline, *src;
        GError *error = NULL;
        gboolean success = TRUE;

        pipeline = gst_parse_launch ("aravissrc name=src camera-name=Fake_1 ! fakesink",
                                     &error);
        g_assert (GST_IS_PIPELINE (pipeline));

        src = gst_bin_get_by_name (GST_BIN (pipeline), "src");

        g_assert_cmpint (gst_element_set_state (pipeline, GST_STATE_PLAYING), !=, GST_STATE_CHANGE_FAILURE);

        sleep (1);

        g_signal_emit_by_name (src, "software-trigger");

        g_signal_emit_by_name (src, "set-float-feature", "ExposureTimeAbs", 5000.0, &success);
        g_assert (success);

        g_signal_emit_by_name (src, "set-integer-feature", "NonExistentFeature", 5000, &success);
        g_assert (!success);

        gst_element_set_state (pipeline, GST_STATE_NULL);
        gst_object_unref (src);
        gst_object_unref (pipeline);
}

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
main (int argc, char *argv[])
{
	int result;

        g_setenv ("GST_PLUGIN_PATH", GST_PLUGIN_PATH, TRUE);
	g_test_init (&argc, &argv, NULL);

        gst_init (&argc, &argv);

	arv_set_fake_camera_genicam_filename (GENICAM_FILENAME);
	arv_enable_interface ("Fake");

	simulator = arv_gv_fake_camera_new ("127.0.0.1", "GVTest");
	g_assert (ARV_IS_GV_FAKE_CAMERA (simulator));
	g_object_set (simulator, "gvsp-lost-ratio", TEARDOWN_TEST_LOST_RATIO, NULL);
	arv_update_device_list ();

	g_test_add_func ("/gst/swap-source-test", swap_source_test);
	g_test_add_func ("/gst/signals-test", signals_test);
	g_test_add_func ("/gst/teardown-under-packet-loss-test", teardown_under_packet_loss_test);

	result = g_test_run();

        g_object_unref (simulator);
	arv_shutdown ();
        gst_deinit();

	return result;
}

