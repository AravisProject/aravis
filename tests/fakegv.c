#include <glib.h>
#include <arv.h>

static void
register_test (void)
{
	ArvCamera *camera;
	ArvDevice *device;
	int int_value;
	double dbl_value;
	double boolean_value;

	camera = arv_camera_new ("Aravis-GV01");
	g_assert (ARV_IS_CAMERA (camera));
	device = arv_camera_get_device (camera);
	g_assert (ARV_IS_GV_DEVICE (device));

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
	int_value = arv_device_get_integer_feature_value (device, "PixelFormat");
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_PIXEL_FORMAT_DEFAULT);

	dbl_value = arv_device_get_float_feature_value (device, "AcquisitionFrameRate");
	g_assert_cmpfloat (dbl_value, ==, ARV_FAKE_CAMERA_ACQUISITION_FRAME_RATE_DEFAULT);
	dbl_value = arv_device_get_float_feature_value (device,  "ExposureTimeAbs");
	g_assert_cmpfloat (dbl_value, ==, ARV_FAKE_CAMERA_EXPOSURE_TIME_US_DEFAULT);

	int_value = arv_device_get_integer_feature_value (device, "GainRaw");
	g_assert_cmpint (int_value, ==, 0);
	int_value = arv_device_get_integer_feature_value (device, "GainAuto");
	g_assert_cmpint (int_value, ==, 1);

	int_value = arv_device_get_integer_feature_value (device, "PayloadSize");
	g_assert_cmpint (int_value, ==, 1024 * 1024);

	arv_device_set_boolean_feature_value (device, "TestBoolean", FALSE);
	boolean_value = arv_device_get_boolean_feature_value (device, "TestBoolean");
	g_assert_cmpint (boolean_value, ==, FALSE);
	int_value = arv_device_get_integer_feature_value (device, "TestRegister");
	g_assert_cmpint (int_value, ==, 123);

	arv_device_set_boolean_feature_value (device, "TestBoolean", TRUE);
	boolean_value = arv_device_get_boolean_feature_value (device, "TestBoolean");
	g_assert_cmpint (boolean_value, ==, TRUE);
	int_value = arv_device_get_integer_feature_value (device, "TestRegister");
	g_assert_cmpint (int_value, ==, 321);

	g_clear_object (&camera);
}

static void
acquisition_test (void)
{
	ArvCamera *camera;
	ArvBuffer *buffer;

	camera = arv_camera_new ("Aravis-GV01");
	g_assert (ARV_IS_CAMERA (camera));

	buffer = arv_camera_acquisition (camera, 0);
	g_assert (ARV_IS_BUFFER (buffer));

	g_clear_object (&camera);
	g_clear_object (&buffer);
}

int
main (int argc, char *argv[])
{
	ArvGvFakeCamera *simulator;
	int result;

	g_test_init (&argc, &argv, NULL);

	arv_g_type_init ();

	simulator = arv_gv_fake_camera_new ("lo");

	arv_gv_fake_camera_start (simulator);

	g_test_add_func ("/fakegv/device_registers", register_test);
	g_test_add_func ("/fakegv/acquisition", acquisition_test);

	result = g_test_run();

	arv_gv_fake_camera_stop (simulator);

	g_object_unref (simulator);

	arv_shutdown ();

	return result;
}

