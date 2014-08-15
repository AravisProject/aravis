#include <glib.h>
#include <arv.h>

static void
load_fake_camera_genicam_test (void)
{
	const char *genicam_xml;
	size_t size;

	genicam_xml = arv_get_fake_camera_genicam_xml (&size);
	g_assert (genicam_xml != NULL);
	g_assert (size != 0);

	genicam_xml = arv_get_fake_camera_genicam_xml (NULL);
	g_assert (genicam_xml != NULL);
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

	address = arv_gc_register_get_address (ARV_GC_REGISTER (node), NULL);
	g_assert_cmpint (address, ==, ARV_FAKE_CAMERA_REGISTER_TRIGGER_MODE);

	address = arv_gc_register_get_address (ARV_GC_REGISTER (arv_gc_get_node (genicam,
										 "TriggerSourceRegister")), NULL);
	g_assert_cmpint (address, ==, ARV_FAKE_CAMERA_REGISTER_TRIGGER_SOURCE);

	address = arv_gc_register_get_address (ARV_GC_REGISTER (arv_gc_get_node (genicam,
										 "TriggerActivationRegister")), NULL);
	g_assert_cmpint (address, ==, ARV_FAKE_CAMERA_REGISTER_TRIGGER_ACTIVATION);

	arv_device_set_string_feature_value (device, "TriggerSelector", "AcquisitionStart");

	address = arv_gc_register_get_address (ARV_GC_REGISTER (node), NULL);
	g_assert_cmpint (address, ==, ARV_FAKE_CAMERA_REGISTER_TRIGGER_MODE +
			 ARV_FAKE_CAMERA_REGISTER_ACQUISITION_START_OFFSET);

	g_object_unref (device);
}

static void
registers_test (void)
{
	ArvDevice *device;
	ArvGc *genicam;
	ArvGcNode *node;
	ArvGcNode *node_a;
	ArvGcNode *node_b;
	ArvGcNode *node_c;
	gint64 value;
	gint64 address;

	device = arv_fake_device_new ("TEST0");
	g_assert (ARV_IS_FAKE_DEVICE (device));

	genicam = arv_device_get_genicam (device);
	g_assert (ARV_IS_GC (genicam));

	node = arv_gc_get_node (genicam, "TestRegister");
	g_assert (ARV_IS_GC_NODE (node));

	address = arv_gc_register_get_address (ARV_GC_REGISTER (node), NULL);
	g_assert_cmpint (address, ==, 0x1f0);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node), 0x12345678, NULL);
	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), NULL);
	g_assert_cmpint (value, ==, 0x12345678);

	node_a = arv_gc_get_node (genicam, "StructEntry_0_15");
	g_assert (ARV_IS_GC_NODE (node_a));
	node_b = arv_gc_get_node (genicam, "StructEntry_16_31");
	g_assert (ARV_IS_GC_NODE (node_b));
	node_c = arv_gc_get_node (genicam, "StructEntry_16");
	g_assert (ARV_IS_GC_NODE (node_c));

	value = arv_gc_register_get_address (ARV_GC_REGISTER (node_a), NULL);
	g_assert_cmpint (value, ==, address);
	value = arv_gc_register_get_address (ARV_GC_REGISTER (node_b), NULL);
	g_assert_cmpint (value, ==, address);
	value = arv_gc_register_get_address (ARV_GC_REGISTER (node_c), NULL);
	g_assert_cmpint (value, ==, address);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_a), NULL);
	g_assert_cmpint (value, ==, 0x5678);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_b), NULL);
	g_assert_cmpint (value, ==, 0x1234);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_b), 0x10101010, NULL);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_a), NULL);
	g_assert_cmpint (value, ==, 0x5678);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_a), 0xabcdefaa, NULL);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_a), NULL);
	g_assert_cmpint (value, ==, 0xefaa);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_b), NULL);
	g_assert_cmpint (value, ==, 0x1010);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_c), NULL);
	g_assert_cmpint (value, ==, 0x0);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_c), 0xff, NULL);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_c), NULL);
	g_assert_cmpint (value, ==, 0x1);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_b), NULL);
	g_assert_cmpint (value, ==, 0x1011);

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

	g_object_unref (device);
}

int
main (int argc, char *argv[])
{
	int result;

	g_test_init (&argc, &argv, NULL);

	arv_g_type_init ();

	arv_set_fake_camera_genicam_filename (GENICAM_FILENAME);

	g_test_add_func ("/fake/load-fake-camera-genicam", load_fake_camera_genicam_test);
	g_test_add_func ("/fake/trigger-registers", trigger_registers_test);
	g_test_add_func ("/fake/registers", registers_test);
	g_test_add_func ("/fake/fake-device", fake_device_test);

	result = g_test_run();

	arv_shutdown ();

	return result;
}

