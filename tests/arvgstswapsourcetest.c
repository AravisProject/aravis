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

int
main (int argc, char *argv[])
{
        GstElement *pipeline;
        GstElement *source;
        GstElement *sink;
        GstPlugin *plugin;
        GError *error = NULL;

        gst_init (&argc, &argv);

        plugin = gst_plugin_load_file (GST_ARAVIS_PLUGIN, &error);
        if (!plugin) {
                g_printerr ("Failed to load plugin: %s\n", error->message);
                g_clear_error (&error);
                return EXIT_FAILURE;
        }
        gst_object_unref (plugin);

	arv_set_fake_camera_genicam_filename (GENICAM_FILENAME);
	arv_enable_interface ("Fake");

        /* Create the initial pipeline */

        g_print ("Pipeline creation\n");

        pipeline = gst_pipeline_new ("aravis-pipeline");
        source   = make_source ("source-1");
        sink = gst_element_factory_make ("fakesink", "fake_sink");
        g_object_set (sink,
                      "sync", TRUE,
                      "signal-handoffs", TRUE,
                      NULL);
        g_signal_connect (sink, "handoff", G_CALLBACK (on_handoff), NULL);

        if (!pipeline || !source || !sink) {
                g_printerr ("Not all elements could be created.\n");
                return EXIT_FAILURE;
        }

        gst_bin_add_many (GST_BIN (pipeline), source, sink, NULL);
        if (!gst_element_link (source, sink)) {
                g_printerr ("Elements could not be linked.\n");
                gst_object_unref (pipeline);
                return EXIT_FAILURE;
        }

        /* Start it */

        g_print ("Start the pipeline\n");
        if (gst_element_set_state (pipeline, GST_STATE_PLAYING)
            == GST_STATE_CHANGE_FAILURE || !wait_for_state (pipeline, GST_STATE_PLAYING)) {
                g_printerr ("Failed to start pipeline\n");
                gst_object_unref (pipeline);
                return EXIT_FAILURE;
        }

        g_print ("Wait for a few frames\n");
        sleep (1);

        /* Now we are trying to swap the source of the pipeline. First, we stop the source */

        g_print ("Remove the source\n");
        g_assert_cmpint (gst_element_set_state (source, GST_STATE_NULL), !=, GST_STATE_CHANGE_FAILURE);
        gst_element_unlink (source, sink);
        g_assert (gst_bin_remove (GST_BIN (pipeline), source));
        source = NULL;

        drain_bus (pipeline);

        g_print ("Wait 1s while pipeline is running without data\n");
        sleep (1);

        /* Then we create a new source */

        g_print ("Create a new source\n");

        source   = make_source ("source-2");

        /* Add it to the pipeline */

        g_assert (gst_bin_add(GST_BIN(pipeline), source));
        if (!gst_element_link (source, sink)) {
                g_printerr ("Could not link new source to sink\n");
                gst_element_set_state (pipeline, GST_STATE_NULL);
                gst_object_unref (pipeline);
                return EXIT_FAILURE;
        }

        if (!gst_element_sync_state_with_parent (source)) {
                g_printerr ("Could not sync new source state with pipeline\n");
                gst_element_set_state (pipeline, GST_STATE_NULL);
                gst_object_unref (pipeline);
                return EXIT_FAILURE;
        }

        g_print ("Wait for a few frames\n");
        sleep (1);

        /* Free everything now */

        g_print ("Stop the pipeline\n");
        g_assert_cmpint (gst_element_set_state (pipeline, GST_STATE_NULL), ==, GST_STATE_CHANGE_SUCCESS);

        g_print ("Free the pipeline\n");
        gst_object_unref (pipeline);

        return EXIT_SUCCESS;
}
