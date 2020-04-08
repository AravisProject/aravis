#include <glib.h>
#include <arv.h>

static void
trigger_registers_test (void)
{
	ArvDevice *device;
	ArvGc *genicam;
	ArvGcNode *node;
	GError *error = NULL;
	gint64 address;

	device = arv_fake_device_new ("TEST0", &error);
	g_assert (ARV_IS_FAKE_DEVICE (device));
	g_assert (error == NULL);

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

	arv_device_set_string_feature_value (device, "TriggerSelector", "AcquisitionStart", NULL);

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
	GError *error = NULL;
	gint64 value;
	gint64 address;

	device = arv_fake_device_new ("TEST0", &error);
	g_assert (ARV_IS_FAKE_DEVICE (device));
	g_assert (error == NULL);

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
	GError *error = NULL;
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

	device = arv_fake_device_new ("TEST0", &error);
	g_assert (ARV_IS_FAKE_DEVICE (device));
	g_assert (error == NULL);

	genicam = arv_device_get_genicam_xml (device, &size);
	g_assert (genicam != NULL);

	/* Check default */
	int_value = arv_device_get_integer_feature_value (device, "Width", NULL);
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_WIDTH_DEFAULT);

	arv_device_set_integer_feature_value (device, "Width", 1024, NULL);
	int_value = arv_device_get_integer_feature_value (device, "Width", NULL);
	g_assert_cmpint (int_value, ==, 1024);

	/* Check default */
	int_value = arv_device_get_integer_feature_value (device, "Height", NULL);
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_HEIGHT_DEFAULT);

	arv_device_set_integer_feature_value (device, "Height", 1024, NULL);
	int_value = arv_device_get_integer_feature_value (device, "Height", NULL);
	g_assert_cmpint (int_value, ==, 1024);

	int_value = arv_device_get_integer_feature_value (device, "BinningHorizontal", NULL);
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_BINNING_HORIZONTAL_DEFAULT);
	int_value = arv_device_get_integer_feature_value (device, "BinningVertical", NULL);
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_BINNING_VERTICAL_DEFAULT);
	int_value = arv_device_get_integer_feature_value (device, "PixelFormat", NULL);
	g_assert_cmpint (int_value, ==, ARV_FAKE_CAMERA_PIXEL_FORMAT_DEFAULT);

	dbl_value = arv_device_get_float_feature_value (device, "AcquisitionFrameRate", NULL);
	g_assert_cmpfloat (dbl_value, ==, ARV_FAKE_CAMERA_ACQUISITION_FRAME_RATE_DEFAULT);
	dbl_value = arv_device_get_float_feature_value (device,  "ExposureTimeAbs", NULL);
	g_assert_cmpfloat (dbl_value, ==, ARV_FAKE_CAMERA_EXPOSURE_TIME_US_DEFAULT);

	int_value = arv_device_get_integer_feature_value (device, "GainRaw", NULL);
	g_assert_cmpint (int_value, ==, 0);
	int_value = arv_device_get_integer_feature_value (device, "GainAuto", NULL);
	g_assert_cmpint (int_value, ==, 1);

	int_value = arv_device_get_integer_feature_value (device, "PayloadSize", NULL);
	g_assert_cmpint (int_value, ==, 1024 * 1024);

	arv_device_set_boolean_feature_value (device, "TestBoolean", FALSE, NULL);
	boolean_value = arv_device_get_boolean_feature_value (device, "TestBoolean", NULL);
	g_assert_cmpint (boolean_value, ==, FALSE);
	int_value = arv_device_get_integer_feature_value (device, "TestRegister", NULL);
	g_assert_cmpint (int_value, ==, 123);

	arv_device_set_boolean_feature_value (device, "TestBoolean", TRUE, NULL);
	boolean_value = arv_device_get_boolean_feature_value (device, "TestBoolean", NULL);
	g_assert_cmpint (boolean_value, ==, TRUE);
	int_value = arv_device_get_integer_feature_value (device, "TestRegister", NULL);
	g_assert_cmpint (int_value, ==, 321);

	arv_device_get_integer_feature_bounds (device, "Width", &minimum, &maximum, NULL);
	g_assert_cmpint (minimum, ==, 1);
	g_assert_cmpint (maximum, ==, 2048);

	arv_device_get_float_feature_bounds (device, "ExposureTimeAbs", &float_minimum, &float_maximum, NULL);
	g_assert_cmpfloat (float_minimum, ==, 10.0);
	g_assert_cmpfloat (float_maximum, ==, 10000000.0);

	arv_device_set_float_feature_value (device,  "ExposureTimeAbs", 20.0, NULL);
	dbl_value = arv_device_get_float_feature_value (device,  "ExposureTimeAbs", NULL);
	g_assert_cmpfloat (dbl_value, ==, 20.0);

	values = arv_device_dup_available_enumeration_feature_values (device, "GainAuto", &n_values, NULL);
	g_assert (values != NULL);
	g_assert_cmpint (n_values, ==, 3);
	g_free (values);

	string_values = arv_device_dup_available_enumeration_feature_values_as_strings (device, "GainAuto", &n_values, NULL);
	g_assert (string_values != NULL);
	g_assert_cmpint (n_values, ==, 3);
	g_free (string_values);

	arv_device_set_string_feature_value (device, "TestStringReg", "String", NULL);
	string_value = arv_device_get_string_feature_value (device, "TestStringReg", NULL);
	g_assert_cmpstr (string_value, ==, "String");

	g_object_unref (device);
}

static void
fake_device_error_test (void)
{
	ArvDevice *device;
	GError *error = NULL;
	int int_value;
	double boolean_value;

	device = arv_fake_device_new ("TEST0", &error);
	g_assert (ARV_IS_FAKE_DEVICE (device));
	g_assert (error == NULL);

	int_value = arv_device_get_integer_feature_value (device, "Unknown", &error);
	g_assert_cmpint (int_value, ==, 0);
	g_assert (error != NULL);

	g_clear_error (&error);

	boolean_value = arv_device_get_boolean_feature_value (device, "Unknown", &error);
	g_assert_cmpint (boolean_value, ==, 0);
	g_assert (error != NULL);

	g_clear_error (&error);

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
	GError *error = NULL;
	guint64 n_completed_buffers;
	guint64 n_failures;
	guint64 n_underruns;
	gint n_input_buffers;
	gint n_output_buffers;
	gint payload;
	gint counter = 0;

	camera = arv_camera_new ("Fake_1", &error);
	g_assert (ARV_IS_CAMERA (camera));
	g_assert (error == NULL);

	device = arv_camera_get_device (camera);
	g_assert (ARV_IS_DEVICE (device));

	fake_camera = arv_fake_device_get_fake_camera (ARV_FAKE_DEVICE (device));
	g_assert (ARV_IS_FAKE_CAMERA (fake_camera));

	stream = arv_camera_create_stream (camera, NULL, NULL);
	g_assert (ARV_IS_STREAM (stream));

	arv_fake_camera_set_fill_pattern (fake_camera, fill_pattern_cb, &counter);

	payload = arv_camera_get_payload (camera, NULL);
	arv_stream_push_buffer (stream,  arv_buffer_new (payload, NULL));
	arv_camera_set_acquisition_mode (camera, ARV_ACQUISITION_MODE_SINGLE_FRAME, NULL);
	arv_camera_start_acquisition (camera, NULL);
	buffer = arv_stream_pop_buffer (stream);
	arv_camera_stop_acquisition (camera, NULL);

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

static void
camera_api_test (void)
{
	ArvCamera *camera;
	ArvPixelFormat pixel_format;
	GError *error = NULL;
	int x, y, w, h;
	const char *string;
	void *ptr;
	unsigned int n;
	gboolean b;
	double d;

	camera = arv_camera_new ("Fake_1", &error);
	g_assert (ARV_IS_CAMERA (camera));
	g_assert (error == NULL);

	string = arv_camera_get_vendor_name (camera, &error);
	g_assert (error == NULL);
	g_assert (string != NULL);

	string = arv_camera_get_model_name (camera, &error);
	g_assert (error == NULL);
	g_assert (string != NULL);

	string = arv_camera_get_device_id (camera, &error);
	g_assert (error == NULL);
	g_assert (string != NULL);

	arv_camera_get_region (camera, &x, &y, &w, &h, &error);
	g_assert (error == NULL);
	g_assert_cmpint (x, ==, 0);
	g_assert_cmpint (y, ==, 0);
	g_assert_cmpint (w, ==, ARV_FAKE_CAMERA_WIDTH_DEFAULT);
	g_assert_cmpint (h, ==, ARV_FAKE_CAMERA_HEIGHT_DEFAULT);

	arv_camera_get_sensor_size (camera, &w, &h, &error);
	g_assert (error == NULL);
	g_assert_cmpint (w, ==, ARV_FAKE_CAMERA_SENSOR_WIDTH);
	g_assert_cmpint (h, ==, ARV_FAKE_CAMERA_SENSOR_HEIGHT);

	arv_camera_set_region (camera, 10, 20, 30, 40, &error);
	g_assert (error == NULL);
	arv_camera_get_region (camera, &x, &y, &w, &h, &error);
	g_assert (error == NULL);
	g_assert_cmpint (x, ==, 10);
	g_assert_cmpint (y, ==, 20);
	g_assert_cmpint (w, ==, 30);
	g_assert_cmpint (h, ==, 40);

	arv_camera_get_binning (camera, &x, &y, &error);
	g_assert (error == NULL);
	g_assert_cmpint (x, ==, ARV_FAKE_CAMERA_BINNING_HORIZONTAL_DEFAULT);
	g_assert_cmpint (y, ==, ARV_FAKE_CAMERA_BINNING_VERTICAL_DEFAULT);

	arv_camera_set_binning (camera, 2, 4, &error);
	g_assert (error == NULL);
	arv_camera_get_binning (camera, &x, &y, &error);
	g_assert (error == NULL);
	g_assert_cmpint (x, ==, 2);
	g_assert_cmpint (y, ==, 4);

	b = arv_camera_is_binning_available (camera, &error);
	g_assert (error == NULL);
	g_assert (b);

	pixel_format = arv_camera_get_pixel_format (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpint (pixel_format, ==, ARV_PIXEL_FORMAT_MONO_8);

	arv_camera_set_pixel_format (camera, ARV_PIXEL_FORMAT_RGB_8_PACKED, &error);
	g_assert (error == NULL);
	pixel_format = arv_camera_get_pixel_format (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpint (pixel_format, ==, ARV_PIXEL_FORMAT_RGB_8_PACKED);

	string = arv_camera_get_pixel_format_as_string (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpstr (string, ==, "RGB8");

	arv_camera_set_pixel_format_from_string (camera, "Mono8", &error);
	g_assert (error == NULL);
	pixel_format = arv_camera_get_pixel_format (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpint (pixel_format, ==, ARV_PIXEL_FORMAT_MONO_8);

	ptr = arv_camera_dup_available_pixel_formats (camera, &n, &error);
	g_assert (error == NULL);
	g_assert (ptr != NULL);
	g_assert_cmpint (n, ==, 2);
	g_clear_pointer (&ptr, g_free);

	ptr = arv_camera_dup_available_pixel_formats_as_strings (camera, &n, &error);
	g_assert (error == NULL);
	g_assert (ptr != NULL);
	g_assert_cmpint (n, ==, 2);
	g_clear_pointer (&ptr, g_free);

	ptr = arv_camera_dup_available_pixel_formats_as_display_names (camera, &n, &error);
	g_assert (error == NULL);
	g_assert (ptr != NULL);
	g_assert_cmpint (n, ==, 2);
	g_clear_pointer (&ptr, g_free);

	b = arv_camera_is_frame_rate_available (camera, &error);
	g_assert (error == NULL);
	g_assert (b);

	d = arv_camera_get_frame_rate (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpfloat (d, ==, ARV_FAKE_CAMERA_ACQUISITION_FRAME_RATE_DEFAULT);

	arv_camera_set_frame_rate (camera, 10.0, &error);
	g_assert (error == NULL);
	d = arv_camera_get_frame_rate (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpfloat (d, ==, 10.0);

	b = arv_camera_is_exposure_auto_available (camera, &error);
	g_assert (error == NULL);
	g_assert (!b);

	b = arv_camera_is_exposure_time_available (camera, &error);
	g_assert (error == NULL);
	g_assert (b);

	arv_camera_set_gain (camera, 1.0, &error);
	g_assert (error == NULL);
	d = arv_camera_get_gain (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpfloat (d, ==, 1.0);

	arv_camera_set_integer (camera, "Unknown", 0, &error);
	g_assert (error != NULL);
	g_clear_error (&error);

	g_object_unref (camera);
}

static void
set_features_from_string_test (void)
{
	ArvDevice *device;
	GError *error = NULL;
	gboolean success;
	gint64 int_value;

	device = arv_fake_device_new ("TEST0", &error);
	g_assert (ARV_IS_FAKE_DEVICE (device));
	g_assert (error == NULL);

	success = arv_device_set_features_from_string (device, "PixelFormat=RGB8 Height=1048 AcquisitionStart", &error);
	g_assert (success);
	g_assert (error == NULL);

	int_value = arv_device_get_integer_feature_value (device, "Height", NULL);
	g_assert_cmpint (int_value, ==, 1048);

	success = arv_device_set_features_from_string (device, "NotAFeature=NotAValue", &error);
	g_assert (!success);
	g_assert (error != NULL);
	g_clear_error (&error);

	success = arv_device_set_features_from_string (device, "NotAFeature", &error);
	g_assert (!success);
	g_assert (error != NULL);
	g_clear_error (&error);

	success = arv_device_set_features_from_string (device, "PixelFormat", &error);
	g_assert (!success);
	g_assert (error != NULL);
	g_clear_error (&error);

	success = arv_device_set_features_from_string (device, "PixelFormat=NotAValidEntry", &error);
	g_assert (!success);
	g_assert (error != NULL);
	g_clear_error (&error);

	success = arv_device_set_features_from_string (device, "StartAcquitision=Value", &error);
	g_assert (!success);
	g_assert (error != NULL);
	g_clear_error (&error);

	success = arv_device_set_features_from_string (device, "PixelFormat=RGB8    AcquisitionStart", &error);
	g_assert (success);
	g_assert (error == NULL);

	success = arv_device_set_features_from_string (device, NULL, NULL);
	g_assert (success);

	g_object_unref (device);
}

int
main (int argc, char *argv[])
{
	int result;

	g_test_init (&argc, &argv, NULL);

	arv_set_fake_camera_genicam_filename (GENICAM_FILENAME);

	arv_enable_interface ("Fake");

	arv_update_device_list ();

	g_test_add_func ("/fake/trigger-registers", trigger_registers_test);
	g_test_add_func ("/fake/registers", registers_test);
	g_test_add_func ("/fake/fake-device", fake_device_test);
	g_test_add_func ("/fake/fake-device-error", fake_device_error_test);
	g_test_add_func ("/fake/fake-stream", fake_stream_test);
	g_test_add_func ("/fake/camera-api", camera_api_test);
	g_test_add_func ("/fake/set-features-from-string", set_features_from_string_test);

	result = g_test_run();

	arv_shutdown ();

	return result;
}

