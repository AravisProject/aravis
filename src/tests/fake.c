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
fake_device_test (void)
{
	ArvDevice *device;
	int int_value;
	double dbl_value;

	device = arv_fake_device_new ("TEST0");
	g_assert (ARV_IS_FAKE_DEVICE (device));

	/* Check default */
	int_value = arv_device_get_integer_feature_value (device, "Width");
	g_assert_cmpint (int_value, ==, 512);

	arv_device_set_integer_feature_value (device, "Width", 1024);
	int_value = arv_device_get_integer_feature_value (device, "Width");
	g_assert_cmpint (int_value, ==, 1024);

	/* Check default */
	int_value = arv_device_get_integer_feature_value (device, "Height");
	g_assert_cmpint (int_value, ==, 512);

	arv_device_set_integer_feature_value (device, "Height", 1024);
	int_value = arv_device_get_integer_feature_value (device, "Height");
	g_assert_cmpint (int_value, ==, 1024);

	int_value = arv_device_get_integer_feature_value (device, "BinningHorizontal");
	g_assert_cmpint (int_value, ==, 1);
	int_value = arv_device_get_integer_feature_value (device, "BinningVertical");
	g_assert_cmpint (int_value, ==, 1);
	dbl_value = arv_device_get_float_feature_value (device,  "ExposureTimeAbs");
	g_assert_cmpfloat (dbl_value, ==, 40000.0);
	int_value = arv_device_get_integer_feature_value (device, "PixelFormat");
	g_assert_cmpint (int_value, ==, ARV_PIXEL_FORMAT_MONO_8);

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
	g_test_add_func ("/fake/fake-device", fake_device_test);

	return g_test_run();
}

