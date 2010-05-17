#include <glib.h>
#include <arv.h>

static void
load_fake_camera_genicam_test (void)
{
	const char *genicam_data;
	size_t size;

	genicam_data = arv_get_fake_camera_genicam_data (&size);
	g_assert (genicam_data != NULL);
	g_assert (size != 0);

	genicam_data = arv_get_fake_camera_genicam_data (NULL);
	g_assert (genicam_data != NULL);
}

static void
trigger_registers_test (void)
{
	ArvDevice *device;
	ArvGc *genicam;
	ArvGcNode *node;
	gint64 address;

	device = arv_fake_device_new ("TEST0");
	g_assert (ARV_IS_FAKE_DEVICE (device));

	genicam = arv_device_get_genicam (device);
	g_assert (ARV_IS_GC (genicam));

	node = arv_gc_get_node (genicam, "TriggerModeRegister");
	g_assert (ARV_IS_GC_NODE (node));

	address = arv_gc_register_get_address (ARV_GC_REGISTER (node));
	g_assert_cmpint (address, ==, ARV_FAKE_CAMERA_REGISTER_TRIGGER_MODE);

	address = arv_gc_register_get_address (ARV_GC_REGISTER (arv_gc_get_node (genicam,
										 "TriggerSourceRegister")));
	g_assert_cmpint (address, ==, ARV_FAKE_CAMERA_REGISTER_TRIGGER_SOURCE);

	address = arv_gc_register_get_address (ARV_GC_REGISTER (arv_gc_get_node (genicam,
										 "TriggerActivationRegister")));
	g_assert_cmpint (address, ==, ARV_FAKE_CAMERA_REGISTER_TRIGGER_ACTIVATION);

	arv_device_set_string_feature_value (device, "TriggerSelector", "AcquisitionStart");

	address = arv_gc_register_get_address (ARV_GC_REGISTER (node));
	g_assert_cmpint (address, ==, ARV_FAKE_CAMERA_REGISTER_TRIGGER_MODE +
			 ARV_FAKE_CAMERA_REGISTER_ACQUISITION_START_OFFSET);

	g_object_unref (device);
}

static void
fake_device_test (void)
{
	ArvDevice *device;
	int int_value;
	double dbl_value;

	device = arv_fake_device_new ("TEST0");
	g_assert (ARV_IS_FAKE_DEVICE (device));

	/* Check default */
	int_value = arv_device_get_integer_feature_value (device, "Width");
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_WIDTH_DEFAULT);

	arv_device_set_integer_feature_value (device, "Width", 1024);
	int_value = arv_device_get_integer_feature_value (device, "Width");
	g_assert_cmpint (int_value, ==, 1024);

	/* Check default */
	int_value = arv_device_get_integer_feature_value (device, "Height");
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_HEIGHT_DEFAULT);

	arv_device_set_integer_feature_value (device, "Height", 1024);
	int_value = arv_device_get_integer_feature_value (device, "Height");
	g_assert_cmpint (int_value, ==, 1024);

	int_value = arv_device_get_integer_feature_value (device, "BinningHorizontal");
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_BINNING_HORIZONTAL_DEFAULT);
	int_value = arv_device_get_integer_feature_value (device, "BinningVertical");
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_BINNING_VERTICAL_DEFAULT);
	dbl_value = arv_device_get_float_feature_value (device,  "ExposureTimeAbs");
	g_assert_cmpfloat (dbl_value, ==, ARV_FAKE_CAMERA_EXPOSURE_TIME_US_DEFAULT);
	int_value = arv_device_get_integer_feature_value (device, "PixelFormat");
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_PIXEL_FORMAT_DEFAULT);

	int_value = arv_device_get_integer_feature_value (device, "GainRaw");
	g_assert_cmpint (int_value, ==, 0);
	int_value = arv_device_get_integer_feature_value (device, "GainAuto");
	g_assert_cmpint (int_value, ==, 0);

	int_value = arv_device_get_integer_feature_value (device, "PayloadSize");
	g_assert_cmpint (int_value, ==, 1024 * 1024);

	g_object_unref (device);
}

int
main (int argc, char *argv[])
{
	int i;

	g_test_init (&argc, &argv, NULL);

	g_type_init ();

	arv_set_fake_camera_genicam_filename (GENICAM_FILENAME);

	g_test_add_func ("/fake/load-fake-camera-genicam", load_fake_camera_genicam_test);
	g_test_add_func ("/fake/trigger-registers", trigger_registers_test);
	g_test_add_func ("/fake/fake-device", fake_device_test);

	return g_test_run();
}

