#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <unistd.h>
#include <arv.h>

#define USE_FAKE_SOURCE 0

/* Small frames for the timing tests, which check neither the image content
 * nor drop behaviour: 4 kB per frame instead of 256 kB. The GV fake camera
 * sends a frame as one burst without packet resend, a large burst can
 * overflow the socket buffer on a loaded host (CI) and lose the frame, and
 * filling large frames costs the fake camera time on slow hosts. */
#define TIMING_CAPS "video/x-raw,format=GRAY8,width=64,height=64"

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

/* The goal of this example is to create a pipeline with an aravis source, and while keeping the pipeline alive, remove
 * the source and replace it with a newly created  aravis source. */

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
        g_usleep (G_USEC_PER_SEC);

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
                g_usleep (G_USEC_PER_SEC);
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

        g_usleep (G_USEC_PER_SEC);

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

	deadline = g_get_monotonic_time () + 10 * G_USEC_PER_SEC;
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

/* chunks on a camera without chunk support (the fake camera): a clean
 * element error naming the problem, no crash. */

static void
chunks_unsupported_test (void)
{
        GstElement *pipeline = gst_pipeline_new ("chunks");
        GstElement *source = make_source ("chunks-source");
        GstElement *sink = gst_element_factory_make ("fakesink", NULL);
        GstMessage *msg;
        GError *error = NULL;

        g_object_set (source, "chunks", "ExposureTime,FrameID", NULL);
        gst_bin_add_many (GST_BIN (pipeline), source, sink, NULL);
        g_assert_true (gst_element_link (source, sink));
        gst_element_set_state (pipeline, GST_STATE_PLAYING);

        msg = gst_bus_timed_pop_filtered (GST_ELEMENT_BUS (pipeline), 5 * GST_SECOND,
                                          GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
        g_assert_nonnull (msg);
        g_assert_cmpint (GST_MESSAGE_TYPE (msg), ==, GST_MESSAGE_ERROR);
        gst_message_parse_error (msg, &error, NULL);
        g_print ("expected error: %s\n", error->message);
        g_clear_error (&error);
        gst_message_unref (msg);

        g_assert_cmpint (gst_element_set_state (pipeline, GST_STATE_NULL), ==, GST_STATE_CHANGE_SUCCESS);
        gst_object_unref (pipeline);
}

/* chunks-configure=false: aravissrc leaves the camera's chunk configuration
 * alone and only reads the listed chunks. On a camera without chunks this is
 * no error; frames flow without a chunk meta. */
static void
chunks_read_only_test (void)
{
        GstElement *pipeline = gst_pipeline_new ("chunks-read");
        GstElement *source = make_source ("chunks-read-source");
        GstElement *sink = gst_element_factory_make ("appsink", "sink");
        int i;

        g_object_set (source, "chunks", "ExposureTime,FrameID", "chunks-configure", FALSE, NULL);
        g_object_set (sink, "sync", FALSE, NULL);
        gst_bin_add_many (GST_BIN (pipeline), source, sink, NULL);
        g_assert_true (gst_element_link (source, sink));
        g_assert_cmpint (gst_element_set_state (pipeline, GST_STATE_PLAYING), !=, GST_STATE_CHANGE_FAILURE);

        for (i = 0; i < 5; i++) {
                GstSample *sample = gst_app_sink_try_pull_sample (GST_APP_SINK (sink), 3 * GST_SECOND);

                g_assert_nonnull (sample);
#if GST_CHECK_VERSION(1, 20, 0)
                g_assert_null (gst_buffer_get_custom_meta (gst_sample_get_buffer (sample), "GstAravisChunkMeta"));
#endif
                gst_sample_unref (sample);
        }

        g_assert_cmpint (gst_element_set_state (pipeline, GST_STATE_NULL), ==, GST_STATE_CHANGE_SUCCESS);
        gst_object_unref (pipeline);
}

/* Sparse (triggered) stream: each software trigger gives exactly one frame, a
 * pause longer than the buffer timeout is not an error with
 * timeout-policy=wait, and the PTS spacing follows the spacing of the
 * captures. Uses the GV fake camera, which honours the software trigger. It
 * polls the trigger at 25 Hz, and on a loaded host (CI virtual machines) much
 * later, so the capture follows its trigger by a varying delay: the PTS steps
 * are compared with the capture steps (device timestamp, the host monotonic
 * time at capture for the fake camera), the trigger only bounds the age. The
 * tolerance covers the host side receive jitter. */

#define SPARSE_TRIGGER_TOLERANCE_MS	80
#define SPARSE_TRIGGER_MAX_AGE_MS	500
#define SPARSE_TRIGGER_MAX_LOST		3

static void
sparse_trigger_print_statistics (GstElement *source)
{
        ArvStream *stream = NULL;
        guint64 n_completed = 0, n_failures = 0, n_underruns = 0;

        g_object_get (source, "stream", &stream, NULL);
        if (stream == NULL)
                return;
        arv_stream_get_statistics (stream, &n_completed, &n_failures, &n_underruns);
        g_print ("sparse: stream completed %" G_GUINT64_FORMAT ", failures %" G_GUINT64_FORMAT
                 ", underruns %" G_GUINT64_FORMAT "\n", n_completed, n_failures, n_underruns);
        g_object_unref (stream);
}

/* Triggers one frame and pulls it. The GV fake camera has no packet resend,
 * so a single packet lost on a loaded host loses the whole frame: then the
 * trigger is repeated. Returns the sample and the host time of the trigger
 * that produced it; *lost tells whether a frame was lost on the way. */
static GstSample *
sparse_trigger_pull (GstElement *source, GstElement *sink, guint *n_lost, gint64 *trigger_us, gboolean *lost)
{
        *lost = FALSE;

        for (;;) {
                GstSample *sample;

                *trigger_us = g_get_monotonic_time ();
                g_signal_emit_by_name (source, "software-trigger");
                sample = gst_app_sink_try_pull_sample (GST_APP_SINK (sink), GST_SECOND);
                if (sample != NULL)
                        return sample;

                (*n_lost)++;
                *lost = TRUE;
                g_print ("sparse: no frame within 1 s of a trigger (%u lost)\n", *n_lost);
                sparse_trigger_print_statistics (source);
                g_assert_cmpuint (*n_lost, <=, SPARSE_TRIGGER_MAX_LOST);
        }
}

static void
sparse_trigger_test (void)
{
        static const guint intervals_ms[] = {300, 600, 2500, 400, 300};
        GstElement *pipeline = gst_pipeline_new ("sparse-trigger");
        GstElement *source = gst_element_factory_make ("aravissrc", "sparse-source");
        GstElement *sink = gst_element_factory_make ("appsink", "sink");
        GstCaps *reference = gst_caps_from_string ("timestamp/x-ptp");
        GstCaps *caps;
        GstClockTime last_pts = GST_CLOCK_TIME_NONE;
        gint64 last_trigger_us = 0;
        gint64 last_capture_ns = 0;
        GstSample *sample;
        gboolean lost;
        guint n_lost = 0;
        guint n_steps = 0;
        guint i;

        g_object_set (simulator, "gvsp-lost-ratio", 0.0, NULL);

        /* trigger first: it is applied when camera-name opens the camera */
        g_object_set (source, "trigger", "Software", NULL);
        g_object_set (source, "camera-name", "Aravis-GVTest", NULL);
        gst_util_set_object_arg (G_OBJECT (source), "timing-mode", "sparse");
        gst_util_set_object_arg (G_OBJECT (source), "timeout-policy", "wait");
        g_object_set (source, "add-reference-timestamp-meta", TRUE, NULL);
        g_object_set (sink, "sync", FALSE, NULL);
        gst_bin_add_many (GST_BIN (pipeline), source, sink, NULL);
        caps = gst_caps_from_string (TIMING_CAPS);
        g_assert_true (gst_element_link_filtered (source, sink, caps));
        gst_caps_unref (caps);

        g_assert_cmpint (gst_element_set_state (pipeline, GST_STATE_PLAYING), !=, GST_STATE_CHANGE_FAILURE);
        /* acquisition runs once the caps are set */
        g_usleep (G_USEC_PER_SEC);

        /* warm-up: the first triggered frame after the start is irregular (fake camera) */
        sample = sparse_trigger_pull (source, sink, &n_lost, &last_trigger_us, &lost);
        gst_sample_unref (sample);

        for (i = 0; i <= G_N_ELEMENTS (intervals_ms); i++) {
                GstBuffer *buffer;
                GstReferenceTimestampMeta *meta;
                gint64 trigger_us;
                gint64 capture_ns;
                gint64 age_ms;

                if (i > 0) {
                        gint64 wait_us = last_trigger_us + intervals_ms[i - 1] * 1000 - g_get_monotonic_time ();

                        if (wait_us > 0)
                                g_usleep (wait_us);
                }

                sample = sparse_trigger_pull (source, sink, &n_lost, &trigger_us, &lost);
                age_ms = (g_get_monotonic_time () - trigger_us) / 1000;
                buffer = gst_sample_get_buffer (sample);
                meta = gst_buffer_get_reference_timestamp_meta (buffer, reference);
                g_assert_nonnull (meta);
                /* the fake camera's device clock is the host monotonic time at capture */
                capture_ns = meta->timestamp;

                g_assert_cmpint (age_ms, <, SPARSE_TRIGGER_MAX_AGE_MS);
                g_assert_true (GST_BUFFER_PTS_IS_VALID (buffer));
                if (GST_CLOCK_TIME_IS_VALID (last_pts) && !lost) {
                        gint64 pts_step_ms;
                        gint64 trigger_step_ms;
                        gint64 capture_step_ms;

                        g_assert_cmpuint (GST_BUFFER_PTS (buffer), >=, last_pts);
                        pts_step_ms = (GST_BUFFER_PTS (buffer) - last_pts) / GST_MSECOND;
                        trigger_step_ms = (trigger_us - last_trigger_us) / 1000;
                        capture_step_ms = (capture_ns - last_capture_ns) / 1000000;
                        g_print ("sparse: trigger step %" G_GINT64_FORMAT " ms, capture step %" G_GINT64_FORMAT
                                 " ms, PTS step %" G_GINT64_FORMAT " ms, captured %" G_GINT64_FORMAT
                                 " ms after the trigger, frame after %" G_GINT64_FORMAT " ms\n",
                                 trigger_step_ms, capture_step_ms, pts_step_ms,
                                 (capture_ns / 1000 - trigger_us) / 1000, age_ms);
                        g_assert_cmpint (ABS (pts_step_ms - capture_step_ms), <, SPARSE_TRIGGER_TOLERANCE_MS);
                        n_steps++;
                }
                last_pts = GST_BUFFER_PTS (buffer);
                last_trigger_us = trigger_us;
                last_capture_ns = capture_ns;
                gst_sample_unref (sample);
        }

        if (n_lost > 0)
                sparse_trigger_print_statistics (source);
        /* a lost frame skips one step: enough steps must remain */
        g_assert_cmpuint (n_steps, >=, G_N_ELEMENTS (intervals_ms) - SPARSE_TRIGGER_MAX_LOST);

        /* nothing without a trigger */
        g_assert_null (gst_app_sink_try_pull_sample (GST_APP_SINK (sink), 300 * GST_MSECOND));

        g_assert_cmpint (gst_element_set_state (pipeline, GST_STATE_NULL), ==, GST_STATE_CHANGE_SUCCESS);
        gst_object_unref (pipeline);
        gst_caps_unref (reference);
        g_object_set (simulator, "gvsp-lost-ratio", TEARDOWN_TEST_LOST_RATIO, NULL);
}

/* Fixed frame rate: the camera is set to the negotiated frame rate, the PTS
 * spacing follows the spacing of the captures (device timestamp, the host
 * monotonic time at capture for the fake camera), the DURATION is the device
 * timestamp difference, and the source reports one frame period of latency.
 * The fake camera itself is not punctual on a loaded host (CI virtual
 * machines): it captures late and then skips frame slots, so its capture
 * period is only bounded from below and the skipped slots are printed. */

#define FIXED_RATE_FPS			25
#define FIXED_RATE_N_STEPS		25
#define FIXED_RATE_TOLERANCE_MS		80

static void
fixed_rate_test (void)
{
        GError *error = NULL;
        GstElement *pipeline;
        GstElement *source;
        GstElement *sink;
        GstQuery *query;
        GstCaps *reference = gst_caps_from_string ("timestamp/x-ptp");
        ArvCamera *camera = NULL;
        GstClockTime period = GST_SECOND / FIXED_RATE_FPS;
        GstClockTime last_pts = GST_CLOCK_TIME_NONE;
        gint64 first_capture_ns = 0;
        gint64 last_capture_ns = 0;
        GstClockTime min_latency;
        gboolean live;
        double frame_rate;
        guint n_skipped = 0;
        guint i;

        pipeline = gst_parse_launch ("aravissrc name=src camera-name=Fake_1 add-reference-timestamp-meta=true "
                                     "! " TIMING_CAPS ",framerate=25/1 ! appsink name=sink sync=false", &error);
        g_assert_no_error (error);
        source = gst_bin_get_by_name (GST_BIN (pipeline), "src");
        sink = gst_bin_get_by_name (GST_BIN (pipeline), "sink");

        g_assert_cmpint (gst_element_set_state (pipeline, GST_STATE_PLAYING), !=, GST_STATE_CHANGE_FAILURE);

        /* skip the start */
        for (i = 0; i < 5; i++) {
                GstSample *sample = gst_app_sink_try_pull_sample (GST_APP_SINK (sink), 3 * GST_SECOND);

                g_assert_nonnull (sample);
                gst_sample_unref (sample);
        }

        /* the negotiated frame rate reached the camera */
        g_object_get (source, "camera", &camera, NULL);
        g_assert_nonnull (camera);
        frame_rate = arv_camera_get_frame_rate (camera, &error);
        g_assert_no_error (error);
        g_assert_cmpfloat (ABS (frame_rate - FIXED_RATE_FPS), <, 0.1);
        g_object_unref (camera);

        for (i = 0; i <= FIXED_RATE_N_STEPS; i++) {
                GstSample *sample = gst_app_sink_try_pull_sample (GST_APP_SINK (sink), 3 * GST_SECOND);
                GstReferenceTimestampMeta *meta;
                GstBuffer *buffer;
                gint64 capture_ns;

                g_assert_nonnull (sample);
                buffer = gst_sample_get_buffer (sample);
                g_assert_true (GST_BUFFER_PTS_IS_VALID (buffer));
                meta = gst_buffer_get_reference_timestamp_meta (buffer, reference);
                g_assert_nonnull (meta);
                capture_ns = meta->timestamp;

                if (GST_CLOCK_TIME_IS_VALID (last_pts)) {
                        gint64 pts_step_ns;
                        gint64 capture_step_ns;

                        g_assert_cmpuint (GST_BUFFER_PTS (buffer), >, last_pts);
                        pts_step_ns = GST_BUFFER_PTS (buffer) - last_pts;
                        capture_step_ns = capture_ns - last_capture_ns;
                        g_assert_cmpint (ABS (pts_step_ns - capture_step_ns), <,
                                         FIXED_RATE_TOLERANCE_MS * (gint64) GST_MSECOND);
                        g_assert_true (GST_BUFFER_DURATION_IS_VALID (buffer));
                        g_assert_cmpint (ABS ((gint64) GST_BUFFER_DURATION (buffer) - capture_step_ns), <,
                                         (gint64) GST_MSECOND);
                        if (capture_step_ns > (gint64) period * 3 / 2)
                                n_skipped++;
                } else
                        first_capture_ns = capture_ns;
                last_pts = GST_BUFFER_PTS (buffer);
                last_capture_ns = capture_ns;
                gst_sample_unref (sample);
        }

        g_print ("fixed rate: camera at %.1f fps, mean capture period %" G_GINT64_FORMAT
                 " us, %u frame slot(s) skipped by the fake camera\n", frame_rate,
                 (last_capture_ns - first_capture_ns) / FIXED_RATE_N_STEPS / 1000, n_skipped);
        /* captures never come faster than the frame rate */
        g_assert_cmpint ((last_capture_ns - first_capture_ns) / FIXED_RATE_N_STEPS, >, (gint64) period * 9 / 10);

        query = gst_query_new_latency ();
        g_assert_true (gst_element_query (source, query));
        gst_query_parse_latency (query, &live, &min_latency, NULL);
        g_assert_true (live);
        g_assert_cmpuint (min_latency, ==, period);
        gst_query_unref (query);

        g_assert_cmpint (gst_element_set_state (pipeline, GST_STATE_NULL), ==, GST_STATE_CHANGE_SUCCESS);
        gst_caps_unref (reference);
        gst_object_unref (source);
        gst_object_unref (sink);
        gst_object_unref (pipeline);
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
#if GST_CHECK_VERSION(1, 20, 0)
	/* Before 1.20 "chunks" is ignored (no GstCustomMeta): no error to expect */
	g_test_add_func ("/gst/chunks-unsupported", chunks_unsupported_test);
#endif
	g_test_add_func ("/gst/chunks-read-only", chunks_read_only_test);
	g_test_add_func ("/gst/sparse-trigger", sparse_trigger_test);
	g_test_add_func ("/gst/fixed-rate", fixed_rate_test);

	result = g_test_run();

        g_object_unref (simulator);
	arv_shutdown ();
        gst_deinit();

	return result;
}

