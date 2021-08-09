/* Aravis - Digital camera library
 *
 * Copyright © 2009-2021 Emmanuel Pacaud
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <glib/gprintf.h>
#include <arv.h>
#include <stdlib.h>
#include <stdio.h>
#include <arvdebugprivate.h>
#include <math.h>

typedef enum {
        ARV_TEST_STATUS_SUCCESS,
        ARV_TEST_STATUS_FAILURE,
        ARV_TEST_STATUS_IGNORED
} ArvTestStatus;

typedef struct {
        char *test_name;
        char *camera_name;
        ArvTestStatus status;
        char *comment;
} ArvTestResult;

#define ARV_TYPE_TEST_RESULT (arv_test_result_get_type())
GType arv_test_result_get_type(void);

static ArvTestResult *
arv_test_result_new (const char *test_name, const char *camera_name, ArvTestStatus status, const char *comment)
{
        ArvTestResult *result = g_new0 (ArvTestResult, 1);

        result->test_name = g_strdup (test_name);
        result->camera_name = g_strdup (camera_name);
        result->status = status;
        result->comment = g_strdup (comment);

        return result;
}

static ArvTestResult *
arv_test_result_copy (ArvTestResult *result)
{
        return arv_test_result_new (result->test_name, result->camera_name, result->status, result->comment);
}

static void
arv_test_result_free (ArvTestResult *result)
{
        if (result != NULL) {
                g_free (result->test_name);
                g_free (result->camera_name);
                g_free (result->comment);
                g_free (result);
        }
}

G_DEFINE_BOXED_TYPE (ArvTestResult, arv_test_result, arv_test_result_copy, arv_test_result_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (ArvTestResult, arv_test_result_free)

typedef struct {
        char *id;
        ArvCamera *camera;
        char *vendor_model;
        GSList *results;
} ArvTestCamera;

#define ARV_TYPE_TEST_CAMERA (arv_test_camera_get_type())
GType arv_test_camera_get_type(void);

static ArvTestCamera *
arv_test_camera_new (const char *camera_id)
{
        ArvTestCamera *camera = g_new0 (ArvTestCamera, 1);

        camera->id = g_strdup (camera_id);
        camera->camera = arv_camera_new (camera_id, NULL);
        camera->vendor_model = g_strdup_printf ("%s:%s",
                                                arv_camera_get_vendor_name (camera->camera, NULL),
                                                arv_camera_get_model_name (camera->camera, NULL));

        return camera;
}

static ArvTestCamera *
arv_test_camera_copy (ArvTestCamera *camera)
{
        return arv_test_camera_new (camera->id);
}

static void
arv_test_camera_free (ArvTestCamera *camera)
{
        if (camera != NULL) {
                g_slist_free_full (camera->results, (GDestroyNotify) arv_test_result_free);
                g_free (camera->id);
                g_object_unref (camera->camera);
                g_free (camera->vendor_model);
                g_free (camera);
        }
}

static gboolean
stdout_has_color_support (void)
{
#if GLIB_CHECK_VERSION(2,50,0)
	static int has_color_support = -1;

	if (has_color_support >= 0)
		return has_color_support > 0;

	has_color_support = g_log_writer_supports_color (STDOUT_FILENO) ? 1 : 0;

	return has_color_support;
#else
	return FALSE;
#endif
}

static void
arv_test_camera_add_result (ArvTestCamera *test_camera,
                            const char *test_name, const char *step_name,
                            ArvTestStatus status, const char *comment)
{
        char *title;
        const char *status_str;

        title = g_strdup_printf ("%s:%s", test_name, step_name);

        if (stdout_has_color_support ())
                switch (status) {
                        case ARV_TEST_STATUS_SUCCESS: status_str = "\033[1;32mSUCCESS\033[0m"; break;
                        case ARV_TEST_STATUS_FAILURE: status_str = "\033[1;31mFAILURE\033[0m"; break;
                        case ARV_TEST_STATUS_IGNORED: status_str = "IGNORED"; break;
                        default: status_str = "";
                }
        else
                switch (status) {
                        case ARV_TEST_STATUS_SUCCESS: status_str = "SUCCESS"; break;
                        case ARV_TEST_STATUS_FAILURE: status_str = "FAILURE"; break;
                        case ARV_TEST_STATUS_IGNORED: status_str = "IGNORED"; break;
                        default: status_str = "";
                }

        g_fprintf (stdout, "%-35s %s %s\n", title, status_str, comment != NULL ? comment : "");

        test_camera->results = g_slist_append (test_camera->results,
                                               arv_test_result_new (title, test_camera->vendor_model,
                                                                    status, comment));
}

G_DEFINE_BOXED_TYPE (ArvTestCamera, arv_test_camera, arv_test_camera_copy, arv_test_camera_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (ArvTestCamera, arv_test_camera_free)

#define ARV_TYPE_TEST arv_test_get_type()
G_DECLARE_FINAL_TYPE (ArvTest, arv_test, ARV, TEST, GObject)

struct _ArvTest {
        GObject parent;

        GPatternSpec *camera_selection;
        GPatternSpec *test_selection;

        GKeyFile *key_file;

        ArvXmlSchema *schema_1_1;
        ArvXmlSchema *schema_1_0;
};

G_DEFINE_TYPE (ArvTest, arv_test, G_TYPE_OBJECT)

static void
arv_test_finalize (GObject *gobject)
{
        ArvTest *self = ARV_TEST (gobject);

	g_clear_object (&self->schema_1_1);
	g_clear_object (&self->schema_1_0);

        g_clear_pointer (&self->key_file, g_key_file_unref);

        g_clear_pointer (&self->camera_selection, g_pattern_spec_free);
        g_clear_pointer (&self->test_selection, g_pattern_spec_free);

        G_OBJECT_CLASS (arv_test_parent_class)->finalize (gobject);
}

static void
arv_test_class_init (ArvTestClass *test_class)
{
        GObjectClass *object_class = G_OBJECT_CLASS (test_class);

        object_class->finalize = arv_test_finalize;
}

static void
arv_test_init (ArvTest *self)
{
        g_autoptr (GBytes) bytes = NULL;

        bytes = g_resources_lookup_data("/org/aravis/GenApiSchema_Version_1_0.xsd", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
        self->schema_1_0 = arv_xml_schema_new_from_memory (g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes));
        g_clear_pointer (&bytes, g_bytes_unref);

        bytes = g_resources_lookup_data("/org/aravis/GenApiSchema_Version_1_0.xsd", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
        self->schema_1_1 = arv_xml_schema_new_from_memory (g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes));
        g_clear_pointer (&bytes, g_bytes_unref);

        bytes = g_resources_lookup_data ("/org/aravis/arv-test.cfg", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
        self->key_file = g_key_file_new ();
        g_key_file_load_from_data (self->key_file, g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes),
                                   G_KEY_FILE_NONE, NULL);

}

static ArvTest *
arv_test_new (const char *camera_selection, const char *test_selection)
{
        ArvTest *test;

        test = g_object_new (ARV_TYPE_TEST, NULL);

        test->camera_selection = g_pattern_spec_new (camera_selection != NULL ? camera_selection : "*");
        test->test_selection = g_pattern_spec_new (test_selection != NULL ? test_selection : "*");

        return test;
}

static double
arv_test_camera_get_key_file_double (ArvTestCamera *test_camera, ArvTest *test, const char *key)
{
        g_return_val_if_fail (test_camera != NULL, 0);
        g_return_val_if_fail (ARV_IS_TEST (test), 0);

        return g_key_file_get_double (test->key_file, test_camera->vendor_model, key, NULL);
}

#if 0
static gint64
arv_test_camera_get_key_file_int64 (ArvTestCamera *test_camera, ArvTest *test, const char *key)
{
        g_return_val_if_fail (test_camera != NULL, 0);
        g_return_val_if_fail (ARV_IS_TEST (test), 0);

        return g_key_file_get_int64 (test->key_file, test_camera->vendor_model, key, NULL);
}
#endif

static char *
arv_test_camera_get_key_file_string (ArvTestCamera *test_camera, ArvTest *test, const char *key)
{
        g_return_val_if_fail (test_camera != NULL, NULL);
        g_return_val_if_fail (ARV_IS_TEST (test), NULL);

        return g_key_file_get_string (test->key_file, test_camera->vendor_model, key, NULL);
}

static gint *
arv_test_camera_get_key_file_integer_list (ArvTestCamera *test_camera, ArvTest *test, const char *key, gsize *size)
{
        g_return_val_if_fail (test_camera != NULL, NULL);
        g_return_val_if_fail (ARV_IS_TEST (test), NULL);

        return g_key_file_get_integer_list (test->key_file, test_camera->vendor_model, key, size, NULL);
}

static void
arv_test_genicam (ArvTest *test, const char *test_name, ArvTestCamera *test_camera)
{
	ArvDevice *device;
        ArvTestStatus status;
        g_autofree char *version = NULL;
        g_autofree char *comment = NULL;
	const char *genicam;
	size_t size;

        g_return_if_fail (ARV_IS_TEST (test));

        version = arv_test_camera_get_key_file_string (test_camera, test, "Schema");

	device = arv_camera_get_device (test_camera->camera);
	genicam = arv_device_get_genicam_xml (device, &size);

        arv_test_camera_add_result (test_camera, test_name, "Load",
                                    genicam != NULL ? ARV_TEST_STATUS_SUCCESS : ARV_TEST_STATUS_FAILURE,
                                    genicam != NULL ? "" : "Failed to retrieve Genicam data");

	if (genicam == NULL)
                return;

        status = ARV_TEST_STATUS_IGNORED;

        if (g_strcmp0 (version, "1.1") == 0) {
                if (arv_xml_schema_validate (test->schema_1_1, genicam, size, NULL, NULL, NULL)) {
                        status = ARV_TEST_STATUS_SUCCESS;
                        comment = g_strdup_printf ("%s", version);
                } else {
                        status = ARV_TEST_STATUS_FAILURE;
                }
        } else if (g_strcmp0 (version, "1.0") == 0) {
                if (arv_xml_schema_validate (test->schema_1_0, genicam, size, NULL, NULL, NULL)) {
                        status = ARV_TEST_STATUS_SUCCESS;
                        comment = g_strdup_printf ("%s", version);
                } else {
                        status = ARV_TEST_STATUS_FAILURE;
                }
        }

        arv_test_camera_add_result (test_camera, test_name, "Schema", status, comment);
}

static void
arv_test_device_properties (ArvTest *test, const char *test_name, ArvTestCamera *test_camera)
{
        g_autoptr (GError) error = NULL;
        g_autofree gint *sensor_size = NULL;
        g_autofree char *comment = NULL;
        ArvTestStatus status;
        gint sensor_width, sensor_height;
        gsize size = 0;

        g_return_if_fail (ARV_IS_TEST (test));

        sensor_size = arv_test_camera_get_key_file_integer_list (test_camera, test, "SensorSize", &size);

	arv_camera_get_sensor_size (test_camera->camera, &sensor_width, &sensor_height, &error);
        arv_test_camera_add_result (test_camera, test_name, "SensorSizeReadout",
                                    error == NULL ? ARV_TEST_STATUS_SUCCESS : ARV_TEST_STATUS_FAILURE,
                                    error != NULL ? error->message : NULL);

        if (error == NULL && size == 2) {
                if (sensor_size[0] != sensor_width ||
                    sensor_size[1] != sensor_height) {
                        status = ARV_TEST_STATUS_FAILURE;
                        comment = g_strdup_printf ("Found %dx%d instead of %dx%d",
                                                   sensor_width, sensor_height,
                                                   sensor_size[0], sensor_size[1]);
                } else {
                        status = ARV_TEST_STATUS_SUCCESS;
                        comment = g_strdup_printf ("%dx%d", sensor_size[0], sensor_size[1]);
                }
        }  else {
                status = ARV_TEST_STATUS_IGNORED;
        }

        arv_test_camera_add_result (test_camera, test_name, "SensorSizeCheck", status, comment);
}

static void
arv_test_acquisition (ArvTest *test, const char *test_name, ArvTestCamera *test_camera)
{
        g_autoptr (GError) error = NULL;
        g_autoptr (ArvBuffer) buffer = NULL;

        g_return_if_fail (ARV_IS_TEST (test));

        buffer = arv_camera_acquisition (test_camera->camera, 1000000, &error);

        arv_test_camera_add_result (test_camera, test_name, "BufferCheck",
                                    ARV_IS_BUFFER (buffer) &&
                                    arv_buffer_get_status (buffer) == ARV_BUFFER_STATUS_SUCCESS &&
                                    error == NULL ?
                                    ARV_TEST_STATUS_SUCCESS :
                                    ARV_TEST_STATUS_FAILURE,
                                    NULL);
}

static void
arv_test_multiple_acquisition (ArvTest *test, const char *test_name, ArvTestCamera *test_camera)
{
        g_autoptr (GError) error = NULL;
        g_autofree char *message = NULL;
        ArvStream *stream;
        double frame_rate;
        unsigned int i;
        size_t payload_size;
        gboolean success = TRUE;
        gint64 start_time = -1;
        gint64 end_time = -1;
        gboolean frame_rate_success;

        g_return_if_fail (ARV_IS_TEST (test));

        frame_rate = arv_test_camera_get_key_file_double (test_camera, test, "FrameRate");
        if (frame_rate <= 0.0)
                frame_rate = 10.0;

        arv_camera_set_acquisition_mode (test_camera->camera, ARV_ACQUISITION_MODE_CONTINUOUS, &error);
        if (error == NULL)
                arv_camera_set_frame_rate (test_camera->camera, frame_rate, &error);

        if (error != NULL) {
                arv_test_camera_add_result (test_camera, test_name, "MultipleAcquisition",
                                            ARV_TEST_STATUS_FAILURE, error->message);
                return;
        }

        stream = arv_camera_create_stream (test_camera->camera, NULL, NULL, &error);
        if (error == NULL)
                payload_size = arv_camera_get_payload (test_camera->camera, &error);
        if (error == NULL) {
                for (i = 0 ; i < 10; i++)
                        arv_stream_push_buffer (stream, arv_buffer_new (payload_size, FALSE));
        }
        if (error == NULL)
                arv_camera_start_acquisition (test_camera->camera, &error);
        if (error == NULL) {
                for (i = 0 ; i < 10; i++) {
                        ArvBuffer *buffer;

                        buffer = arv_stream_timeout_pop_buffer (stream, 500000);
                        if (buffer == NULL)
                                success = FALSE;
                        else {
                                if (arv_buffer_get_status (buffer) == ARV_BUFFER_STATUS_SUCCESS) {
                                        if (start_time < 0)
                                                start_time = arv_buffer_get_timestamp (buffer);
                                        else
                                                end_time = arv_buffer_get_timestamp (buffer);
                                } else {
                                        success = FALSE;
                                }
                                arv_stream_push_buffer (stream, buffer);
                        }
                }
        }
        if (error == NULL)
                arv_camera_stop_acquisition (test_camera->camera, &error);

        g_object_unref (stream);

        arv_test_camera_add_result (test_camera, test_name, "BufferCheck",
                                    success && error == NULL ? ARV_TEST_STATUS_SUCCESS : ARV_TEST_STATUS_FAILURE,
                                    error != NULL ? error->message : NULL);

        frame_rate_success = FALSE;

        if (end_time > 0 && start_time > 0 && start_time != end_time) {
                double actual_frame_rate = 9 * 1e9 / (end_time - start_time);
                double frame_rate_error = fabs (actual_frame_rate - frame_rate) / frame_rate;

                if (frame_rate_error < 0.05) {
                        frame_rate_success = TRUE;
                        message = g_strdup_printf ("%.2f", actual_frame_rate);
                } else
                        message = g_strdup_printf ("%.2f (expected:%.2f)", actual_frame_rate, frame_rate);
        }

        arv_test_camera_add_result (test_camera, test_name, "FrameRate",
                                    frame_rate_success ? ARV_TEST_STATUS_SUCCESS : ARV_TEST_STATUS_FAILURE,
                                    message);
}

const struct {
        const char *name;
        void (*run) (ArvTest *test, const char *test_name, ArvTestCamera *test_camera);
        gboolean is_slow;
} tests[] = {
        {"Genicam",                     arv_test_genicam,               FALSE},
        {"Properties",                  arv_test_device_properties,     FALSE},
        {"MultipleAcquisition",         arv_test_multiple_acquisition,  FALSE},
        {"Acquisition",                 arv_test_acquisition,           FALSE}
};

static gboolean
arv_test_run (ArvTest *test)
{
	unsigned n_devices, i;
        gboolean success = TRUE;

        g_return_val_if_fail (ARV_IS_TEST (test), FALSE);

	arv_update_device_list ();
	n_devices = arv_get_n_devices ();
	for (i = 0; i < n_devices; i++) {
                const char *camera_id = arv_get_device_id (i);

                if (g_pattern_match_string (test->camera_selection, camera_id)) {
                        g_autoptr (ArvTestCamera) test_camera = NULL;
                        unsigned int j;

                        printf ("Testing device '%s' from '%s'\n", arv_get_device_model (i), arv_get_device_vendor (i));

                        test_camera = arv_test_camera_new (camera_id);

                        for (j = 0; j < G_N_ELEMENTS (tests); j++) {
                                if (g_pattern_match_string (test->test_selection, tests[j].name))
                                        tests[j].run (test, tests[j].name, test_camera);
                        }
                }
	}

        return success;
}

static char *arv_option_camera_selection = NULL;
static char *arv_option_test_selection = NULL;
static char *arv_option_debug_domains = NULL;

static const GOptionEntry arv_option_entries[] =
{
	{
		"name", 				'n', 0, G_OPTION_ARG_STRING,
		&arv_option_camera_selection, 		NULL,
		"<pattern>"
	},
	{
		"test", 				't', 0, G_OPTION_ARG_STRING,
		&arv_option_test_selection, 		NULL,
		"<pattern>"
	},
	{
		"debug", 				'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 		NULL,
		"{<category>[:<level>][,...]|help}"
	},
	{ NULL }
};

int
main (int argc, char **argv)
{
	GOptionContext *context;
	GError *error = NULL;
        ArvTest *test = NULL;
        gboolean success;

	context = g_option_context_new (NULL);
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

        test = arv_test_new (arv_option_camera_selection, arv_option_test_selection);

        success = arv_test_run (test);

        g_clear_object (&test);

	arv_shutdown ();

	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
