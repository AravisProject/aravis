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
	ArvGc *genicam;
	int value;

	device = arv_fake_device_new ("TEST0");
	g_assert (ARV_IS_FAKE_DEVICE (device));

	genicam = arv_device_get_genicam (device);
	g_assert (ARV_IS_GC (genicam));

	/* Check default */
	value = arv_gc_integer_get_value (ARV_GC_INTEGER (arv_gc_get_node (genicam, "Width")));
	g_assert_cmpint (value, ==, 512);

	arv_gc_integer_set_value (ARV_GC_INTEGER (arv_gc_get_node (genicam, "Width")), 1024);
	value = arv_gc_integer_get_value (ARV_GC_INTEGER (arv_gc_get_node (genicam, "Width")));
	g_assert_cmpint (value, ==, 1024);

	/* Check default */
	value = arv_gc_integer_get_value (ARV_GC_INTEGER (arv_gc_get_node (genicam, "Height")));
	g_assert_cmpint (value, ==, 512);

	arv_gc_integer_set_value (ARV_GC_INTEGER (arv_gc_get_node (genicam, "Height")), 1024);
	value = arv_gc_integer_get_value (ARV_GC_INTEGER (arv_gc_get_node (genicam, "Height")));
	g_assert_cmpint (value, ==, 1024);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (arv_gc_get_node (genicam, "BinningHorizontal")));
	g_assert_cmpint (value, ==, 1);
	value = arv_gc_integer_get_value (ARV_GC_INTEGER (arv_gc_get_node (genicam, "BinningVertical")));
	g_assert_cmpint (value, ==, 1);
	value = arv_gc_integer_get_value (ARV_GC_INTEGER (arv_gc_get_node (genicam, "GainRaw")));
	g_assert_cmpint (value, ==, 0);
	value = arv_gc_enumeration_get_int_value (ARV_GC_ENUMERATION (arv_gc_get_node (genicam, "GainAuto")));
	g_assert_cmpint (value, ==, 0);
	value = arv_gc_float_get_value (ARV_GC_FLOAT (arv_gc_get_node (genicam, "ExposureTimeAbs")));
	g_assert_cmpfloat (value, ==, 40000.0);

	g_object_unref (device);
}

int
main (int argc, char *argv[])
{
	int i;

	g_test_init (&argc, &argv, NULL);

	g_type_init ();

	g_test_add_func ("/fake/load-fake-camera-genicam", load_fake_camera_genicam_test);
	g_test_add_func ("/fake/fake-device", fake_device_test);

	return g_test_run();
}

