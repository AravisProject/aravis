/* Simple script which will start an aravis pipeline, then restart ONLY the camera. */
#include <gst/gst.h>
#include <stdio.h>
#include <unistd.h>

int
main (int argc, char *argv[])
{
        GstElement *pipeline;
        GstElement *source;
        GstElement *sink;
        GstElement *new_source;
        GstState current_state, pending_state;

        gst_init (&argc, &argv);

        pipeline = gst_pipeline_new ("aravis_pipeline");
        source = gst_element_factory_make ("aravissrc", "source");
        if (!source) {
                printf ("Failed to create the source element\n");
                return EXIT_FAILURE;
        }

        g_object_set(G_OBJECT(source),
                     "camera-name", "Aravis-Fake-GV01",
                     "num-arv-buffers", 5,
                     "packet-resend", FALSE,
                     "packet-size", 9000,
                     "auto-packet-size", FALSE,
                     NULL);

        sink = gst_element_factory_make ("fakesink", "fake_sink");

        if (!pipeline || !source || !sink) {
                printf ("Not all elements could be created.\n");
                return EXIT_FAILURE;
        }

        gst_bin_add_many (GST_BIN (pipeline), source, sink, NULL);

        if (!gst_element_link (source, sink)) {
                printf ("Elements could not be linked.\n");
                gst_object_unref (pipeline);
                return EXIT_FAILURE;
        }

        printf ("Start the pipeline\n");
        g_assert_cmpint (gst_element_set_state(pipeline, GST_STATE_PLAYING), ==, GST_STATE_CHANGE_ASYNC);

        printf ("Wait for a few frames\n");
        sleep (1);

        printf ("Stop the source\n");
        g_assert_cmpint (gst_element_set_state(source, GST_STATE_NULL), ==, GST_STATE_CHANGE_SUCCESS);

        gst_element_get_state (source, &current_state, &pending_state, 0);
        printf ("source current state:%d - pending state:%d\n", current_state, pending_state);

        sleep (1);

        gst_element_unlink(source, sink);
        g_assert (gst_bin_remove(GST_BIN(pipeline), source));

        printf ("Create a new source\n");
        new_source = gst_element_factory_make ("aravissrc", "new_source");
        if (!new_source) {
                printf ("Failed to create the source element\n");
                return EXIT_FAILURE;
        }
        g_object_set(G_OBJECT(new_source),
                     "camera-name", "Aravis-Fake-GV01",
                     "num-arv-buffers", 5,
                     "packet-resend", FALSE,
                     "packet-size", 9000,
                     "auto-packet-size", FALSE,
                     NULL);

        g_assert (gst_bin_add(GST_BIN(pipeline), new_source));
        g_assert (gst_element_link(new_source, sink));
        g_assert (gst_element_sync_state_with_parent(new_source));

        gst_element_get_state (source, &current_state, &pending_state, 0);
        printf ("new source current state:%d - pending state:%d\n", current_state, pending_state);

        printf ("Start the new source\n");
        g_assert_cmpint (gst_element_set_state(source, GST_STATE_PLAYING), ==, GST_STATE_CHANGE_SUCCESS);

        gst_element_get_state (source, &current_state, &pending_state, 0);
        printf ("new source current state:%d - pending state:%d\n", current_state, pending_state);

        printf ("Wait for a few frames\n");

        printf ("Stop the pipeline\n");
        g_assert_cmpint (gst_element_set_state (pipeline, GST_STATE_NULL), ==, GST_STATE_CHANGE_SUCCESS);

        printf ("Free the pipeline\n");
        gst_object_unref (pipeline);

        return EXIT_SUCCESS;
}
