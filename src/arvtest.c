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

#include <arv.h>
#include <stdlib.h>
#include <stdio.h>
#include <arvdebugprivate.h>

typedef struct {
        GKeyFile *key_file;

        ArvXmlSchema *schema_1_1;
        ArvXmlSchema *schema_1_0;
} ArvTest;

typedef struct {
	int dummy;
} TestData;

typedef struct {
	gboolean genicam_success;
	const char *genicam_error;

	gboolean read_sensor_success;
	gboolean sensor_size_success;
	gint sensor_width;
	gint sensor_height;

        gboolean acquisition_success;
} TestResult;

static void
print_result (TestData *data, TestResult *result)
{
	printf ("Genicam: %s %s\n",
		result->genicam_success ? "SUCCESS" : "FAILED",
		result->genicam_error != NULL ? result->genicam_error : "");
	if (result->read_sensor_success)
		printf ("Sensor size read: %d %d\n", result->sensor_width, result->sensor_height);
	else
		printf ("Sensor size read: FAILED\n");
        printf ("Sensor size: %s\n", result->sensor_size_success ? "SUCCESS" : "FAILED");
        printf ("Acquisition: %s\n", result->acquisition_success ? "SUCCESS" : "FAILED");
}

static char *
arv_test_get_key_file_string (ArvTest *test, ArvCamera *camera, const char *key)
{
        g_autofree char *group = g_strdup_printf ("%s:%s",
                                                  arv_camera_get_vendor_name (camera, NULL),
                                                  arv_camera_get_model_name (camera, NULL));

        return g_key_file_get_string (test->key_file, group, key, NULL);
}

static gint *
arv_test_get_key_file_integer_list (ArvTest *test, ArvCamera *camera, const char *key, gsize *size)
{
        g_autofree char *group = g_strdup_printf ("%s:%s",
                                                  arv_camera_get_vendor_name (camera, NULL),
                                                  arv_camera_get_model_name (camera, NULL));

        return g_key_file_get_integer_list (test->key_file, group, key, size, NULL);
}

static gboolean
arv_test_genicam (ArvTest *test, ArvCamera *camera, TestData *data, TestResult *result)
{
	ArvDevice *device;
        g_autofree char *version;
	const char *genicam;
	size_t size;

        version = arv_test_get_key_file_string (test, camera, "Schema");

	device = arv_camera_get_device (camera);
	genicam = arv_device_get_genicam_xml (device, &size);

	if (genicam == NULL) {
		result->genicam_error = "Failed to retrieve Genicam data";
		result->genicam_success = FALSE;

		return FALSE;
	}

        if ((g_strcmp0 (version, "1.1") == 0 &&
             !arv_xml_schema_validate (test->schema_1_1, genicam, size, NULL, NULL, NULL)) ||
            (g_strcmp0 (version, "1.0") == 0 &&
             !arv_xml_schema_validate (test->schema_1_0, genicam, size, NULL, NULL, NULL))) {
		result->genicam_error = "Invalid Genicam XML data";
		result->genicam_success = FALSE;

		return FALSE;
	}

	result->genicam_error = NULL;
	result->genicam_success = TRUE;

	return TRUE;
}

static gboolean
arv_test_device_properties (ArvTest *test, ArvCamera *camera, TestData *data, TestResult *result)
{
        g_autoptr (GError) error = NULL;
        g_autofree gint *sensor_size = NULL;
        gsize size = 0;

        sensor_size = arv_test_get_key_file_integer_list (test, camera, "SensorSize", &size);

	arv_camera_get_sensor_size (camera, &result->sensor_width, &result->sensor_height, &error);

	result->read_sensor_success = error == NULL;

        result->sensor_size_success = error == NULL;
        if (error == NULL && size == 2) {
                if (sensor_size[0] != result->sensor_width ||
                    sensor_size[1] != result->sensor_height) {
                        result->sensor_size_success = FALSE;
                }
        }

        return result->read_sensor_success;
}

static gboolean
arv_test_acquisition (ArvTest *test, ArvCamera *camera, TestData *data, TestResult *result)
{
        g_autoptr (GError) error = NULL;
        g_autoptr (ArvBuffer) buffer = NULL;

        buffer = arv_camera_acquisition (camera, 1000000, &error);

        result->acquisition_success = error == NULL && ARV_IS_BUFFER (buffer);

        return result->acquisition_success;
}

static char *arv_option_debug_domains = NULL;

static const GOptionEntry arv_option_entries[] =
{
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
        ArvTest *test;
        g_autoptr (GBytes) bytes = NULL;
	unsigned n_devices, i;

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

        test = g_new0 (ArvTest, 1);

        bytes = g_resources_lookup_data("/org/aravis/GenApiSchema_Version_1_0.xsd", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
        test->schema_1_0 = arv_xml_schema_new_from_memory (g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes));
        g_clear_pointer (&bytes, g_bytes_unref);

        bytes = g_resources_lookup_data("/org/aravis/GenApiSchema_Version_1_0.xsd", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
        test->schema_1_1 = arv_xml_schema_new_from_memory (g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes));
        g_clear_pointer (&bytes, g_bytes_unref);

        bytes = g_resources_lookup_data ("/org/aravis/arv-test.ini", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
        test->key_file = g_key_file_new ();
        g_key_file_load_from_data (test->key_file, g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes),
                                   G_KEY_FILE_NONE, NULL);

	arv_update_device_list ();
	n_devices = arv_get_n_devices ();
	for (i = 0; i < n_devices; i++) {
	        g_autoptr (ArvCamera) camera = NULL;
                g_autoptr (GError) error = NULL;
                g_autofree TestData *data = NULL;
		g_autofree TestResult *result = NULL;

		printf ("Testing device '%s' from '%s'\n", arv_get_device_model (i), arv_get_device_vendor (i));

		data = g_new0 (TestData, 1);
		result = g_new0 (TestResult, 1);

		camera = arv_camera_new (arv_get_device_id (i), &error);

		arv_test_genicam (test, camera, data, result);
		arv_test_device_properties (test, camera, data, result);
                arv_test_acquisition (test, camera, data, result);

		print_result (data, result);
	}

	g_object_unref (test->schema_1_1);
	g_object_unref (test->schema_1_0);

        g_key_file_unref (test->key_file);

	arv_shutdown ();

	return EXIT_SUCCESS;
}
