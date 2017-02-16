#include <arv.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
	int dummy;
} TestData;

typedef struct {
	gboolean genicam_success;
	const char *genicam_error;

	gboolean read_sensor_success;
	gint sensor_width;
	gint sensor_height;
} TestResult;

ArvXmlSchema *schema_1_1;
ArvXmlSchema *schema_1_0;

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
}

static gboolean
test_genicam (ArvCamera *camera, TestData *data, TestResult *result)
{
	ArvDevice *device;
	const char *genicam;
	size_t size;

	device = arv_camera_get_device (camera);
	genicam = arv_device_get_genicam_xml (device, &size);

	if (genicam == NULL) {
		result->genicam_error = "Failed to retrieve Genicam data";
		result->genicam_success = FALSE;

		return FALSE;
	}

	if (!arv_xml_schema_validate (schema_1_1, genicam, size, NULL, NULL, NULL) &&
	    !arv_xml_schema_validate (schema_1_0, genicam, size, NULL, NULL, NULL)) {
		result->genicam_error = "Invalid Genicam XML data";
		result->genicam_success = FALSE;

		return FALSE;
	}

	result->genicam_error = NULL;
	result->genicam_success = TRUE;

	return TRUE;
}

static gboolean
test_device_properties (ArvCamera *camera, TestData *data, TestResult *result)
{
	arv_camera_get_sensor_size (camera, &result->sensor_width, &result->sensor_height);

	result->read_sensor_success = arv_device_get_status (arv_camera_get_device (camera)) == ARV_DEVICE_STATUS_SUCCESS;

	return TRUE;
}

int
main (int argc, char **argv)
{
	unsigned n_devices, i;
	ArvCamera *camera;

	schema_1_0 = arv_xml_schema_new_from_path ("GenApiSchema_Version_1_0.xsd");
	schema_1_1 = arv_xml_schema_new_from_path ("GenApiSchema_Version_1_1.xsd");

	arv_update_device_list ();
	n_devices = arv_get_n_devices ();
	for (i = 0; i < n_devices; i++) {
		TestData *data;
		TestResult *result;

		printf ("Testing device '%s' from '%s'\n", arv_get_device_model (i), arv_get_device_vendor (i));

		data = g_new0 (TestData, 1);
		result = g_new0 (TestResult, 1);

		camera = arv_camera_new (arv_get_device_id (i));

		test_genicam (camera, data, result);
		test_device_properties (camera, data, result);

		print_result (data, result);

		g_object_unref (camera);

		g_free (data);
		g_free (result);
	}

	g_object_unref (schema_1_1);
	g_object_unref (schema_1_0);

	arv_shutdown ();

	return EXIT_SUCCESS;
}
