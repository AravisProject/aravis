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
	g_assert_cmpint (value, ==, -4182);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_b), NULL);
	g_assert_cmpint (value, ==, 0x1010);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_c), NULL);
	g_assert_cmpint (value, ==, 0x0);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_c), 0xff, NULL);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_c), NULL);
	g_assert_cmpint (value, ==, 1);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_b), NULL);
	g_assert_cmpint (value, ==, 0x1011);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_b), 0xff, NULL);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_b), NULL);
	g_assert_cmpint (value, ==, 0xff);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_c), 0x0, NULL);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_c), NULL);
	g_assert_cmpint (value, ==, 0);

	g_object_unref (device);
}

static void
fake_device_test (void)
{
	ArvDevice *device;
	ArvDeviceStatus status;
	int int_value;
	double dbl_value;
	double boolean_value;
	gint64 minimum, maximum;
	gint64 *values;
	const char **string_values;
	const char *string_value;
	guint n_values;
	double float_minimum, float_maximum;
	const char *genicam;
	gsize size;

	device = arv_fake_device_new ("TEST0");
	g_assert (ARV_IS_FAKE_DEVICE (device));

	genicam = arv_device_get_genicam_xml (device, &size);
	g_assert (genicam != NULL);

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

	arv_device_get_integer_feature_bounds (device, "Width", &minimum, &maximum);
	g_assert_cmpint (minimum, ==, 1);
	g_assert_cmpint (maximum, ==, 2048);

	arv_device_get_float_feature_bounds (device, "ExposureTimeAbs", &float_minimum, &float_maximum);
	g_assert_cmpfloat (float_minimum, ==, 10.0);
	g_assert_cmpfloat (float_maximum, ==, 10000000.0);

	arv_device_set_float_feature_value (device,  "ExposureTimeAbs", 20.0);
	dbl_value = arv_device_get_float_feature_value (device,  "ExposureTimeAbs");
	g_assert_cmpfloat (dbl_value, ==, 20.0);

	values = arv_device_get_available_enumeration_feature_values (device, "GainAuto", &n_values);
	g_assert (values != NULL);
	g_assert_cmpint (n_values, ==, 3);
	g_free (values);

	string_values = arv_device_get_available_enumeration_feature_values_as_strings (device, "GainAuto", &n_values);
	g_assert (string_values != NULL);
	g_assert_cmpint (n_values, ==, 3);
	g_free (string_values);

	arv_device_set_string_feature_value (device, "TestStringReg", "String");
	string_value = arv_device_get_string_feature_value (device, "TestStringReg");
	g_assert_cmpstr (string_value, ==, "String");

	status = arv_device_get_status (device);
	g_assert_cmpint (status, ==, ARV_DEVICE_STATUS_SUCCESS);

	g_object_unref (device);
}

static void
fake_device_error_test (void)
{
	ArvDevice *device;
	ArvDeviceStatus status;
	int int_value;
	double boolean_value;

	device = arv_fake_device_new ("TEST0");
	g_assert (ARV_IS_FAKE_DEVICE (device));

	int_value = arv_device_get_integer_feature_value (device, "Unknown");
	g_assert_cmpint (int_value, ==, 0);

	boolean_value = arv_device_get_boolean_feature_value (device, "Unknown");
	g_assert_cmpint (boolean_value, ==, 0);

	status = arv_device_get_status (device);
	g_assert_cmpint (status, ==, ARV_DEVICE_STATUS_SUCCESS);

	g_object_unref (device);
}

static void
fill_pattern_cb (ArvBuffer *buffer, void *fill_pattern_data, guint32 exposure_time_us, guint32 gain, ArvPixelFormat pixel_format)
{
	gint *counter = fill_pattern_data;

	(*counter)++;
}

static void
fake_stream_test (void)
{
	ArvCamera *camera;
	ArvDevice *device;
	ArvFakeCamera *fake_camera;
	ArvStream *stream;
	ArvBuffer *buffer;
	guint64 n_completed_buffers;
	guint64 n_failures;
	guint64 n_underruns;
	gint n_input_buffers;
	gint n_output_buffers;
	gint payload;
	gint counter = 0;

	camera = arv_camera_new ("Fake_1");
	g_assert (ARV_IS_CAMERA (camera));

	device = arv_camera_get_device (camera);
	g_assert (ARV_IS_DEVICE (device));

	fake_camera = arv_fake_device_get_fake_camera (ARV_FAKE_DEVICE (device));
	g_assert (ARV_IS_FAKE_CAMERA (fake_camera));

	stream = arv_camera_create_stream (camera, NULL, NULL);
	g_assert (ARV_IS_STREAM (stream));

	arv_fake_camera_set_fill_pattern (fake_camera, fill_pattern_cb, &counter);

	payload = arv_camera_get_payload (camera);
	arv_stream_push_buffer (stream,  arv_buffer_new (payload, NULL));
	arv_camera_set_acquisition_mode (camera, ARV_ACQUISITION_MODE_SINGLE_FRAME);
	arv_camera_start_acquisition (camera);
	buffer = arv_stream_pop_buffer (stream);
	arv_camera_stop_acquisition (camera);

	arv_fake_camera_set_fill_pattern (fake_camera, NULL, NULL);

	g_assert_cmpint (counter, ==, 1);

	g_assert (ARV_IS_BUFFER (buffer));

	arv_stream_get_statistics (stream, &n_completed_buffers, &n_failures, &n_underruns);
	g_assert_cmpint (n_completed_buffers, == ,1);
	g_assert_cmpint (n_failures, ==, 0);
	g_assert_cmpint (n_underruns, ==, 0);

	arv_stream_get_n_buffers (stream, &n_input_buffers, &n_output_buffers);
	g_assert_cmpint (n_input_buffers, ==, 0);
	g_assert_cmpint (n_output_buffers, ==, 0);

	g_clear_object (&buffer);
	g_clear_object (&stream);
	g_clear_object (&camera);
}

int
main (int argc, char *argv[])
{
	int result;

	g_test_init (&argc, &argv, NULL);

	arv_enable_interface ("Fake");

	arv_update_device_list ();

	arv_set_fake_camera_genicam_filename (GENICAM_FILENAME);

	g_test_add_func ("/fake/load-fake-camera-genicam", load_fake_camera_genicam_test);
	g_test_add_func ("/fake/trigger-registers", trigger_registers_test);
	g_test_add_func ("/fake/registers", registers_test);
	g_test_add_func ("/fake/fake-device", fake_device_test);
	g_test_add_func ("/fake/fake-device-error", fake_device_error_test);
	g_test_add_func ("/fake/fake-stream", fake_stream_test);

	result = g_test_run();

	arv_shutdown ();

	return result;
}

