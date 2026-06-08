/* SPDX-License-Identifier:Unlicense */

#include <glib.h>
#include <arv.h>

static void
discovery_test (void)
{
        int n_devices;
        int i;

        n_devices = arv_get_n_devices ();

        for (i = 0; i < n_devices; i++) {
                if (g_strcmp0 (arv_get_device_vendor (i), "Aravis") == 0 &&
                    g_strcmp0 (arv_get_device_model (i), "Fake") == 0 &&
                    g_strcmp0 (arv_get_device_serial_nbr (i), "1") == 0 &&
                    g_strcmp0 (arv_get_device_manufacturer_info (i), "none") == 0 &&
                    g_strcmp0 (arv_get_device_id (i), "Fake_1") == 0 &&
                    g_strcmp0 (arv_get_device_physical_id (i), "Fake_1") == 0)
                        return;
        }

        g_assert_not_reached ();
}

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

	address = arv_gc_register_get_address (ARV_GC_REGISTER (arv_gc_get_node (genicam,
										 "TriggerSoftwareCommandRegister")), NULL);
	g_assert_cmpint (address, ==, ARV_FAKE_CAMERA_REGISTER_TRIGGER_SOFTWARE);

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
	ArvGcNode *node_0_15;
	ArvGcNode *node_16_31;
	ArvGcNode *node_15;
	ArvGcNode *node_0_31;
	GError *error = NULL;
	gint64 value;
	gint64 address;

	device = arv_fake_device_new ("TEST0", &error);
	g_assert (ARV_IS_FAKE_DEVICE (device));
	g_assert (error == NULL);

	genicam = arv_device_get_genicam (device);
	g_assert (ARV_IS_GC (genicam));

	arv_gc_set_range_check_policy (genicam, ARV_RANGE_CHECK_POLICY_ENABLE);

	node = arv_gc_get_node (genicam, "TestRegister");
	g_assert (ARV_IS_GC_NODE (node));

	address = arv_gc_register_get_address (ARV_GC_REGISTER (node), &error);
	g_assert_cmpint (address, ==, 0x1f0);
	g_assert (error == NULL);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node), 0x12345678, &error);
	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), NULL);
	g_assert_cmpint (value, ==, 0x12345678);
	g_assert (error == NULL);

	node_0_15 = arv_gc_get_node (genicam, "StructEntry_0_15");
	g_assert (ARV_IS_GC_NODE (node_0_15));
	node_16_31 = arv_gc_get_node (genicam, "StructEntry_16_31");
	g_assert (ARV_IS_GC_NODE (node_16_31));
	node_15 = arv_gc_get_node (genicam, "StructEntry_15");
	g_assert (ARV_IS_GC_NODE (node_15));
	node_0_31 = arv_gc_get_node (genicam, "StructEntry_0_31");
	g_assert (ARV_IS_GC_NODE (node_0_31));

	value = arv_gc_register_get_address (ARV_GC_REGISTER (node_0_15), NULL);
	g_assert_cmpint (value, ==, address);
	value = arv_gc_register_get_address (ARV_GC_REGISTER (node_16_31), NULL);
	g_assert_cmpint (value, ==, address);
	value = arv_gc_register_get_address (ARV_GC_REGISTER (node_15), NULL);
	g_assert_cmpint (value, ==, address);
	value = arv_gc_register_get_address (ARV_GC_REGISTER (node_0_31), NULL);
	g_assert_cmpint (value, ==, address);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_0_15), NULL);
	g_assert_cmpint (value, ==, 0x1234);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_16_31), NULL);
	g_assert_cmpint (value, ==, 0x5678);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_16_31), 0x10101010, &error);
	g_assert (error != NULL);
	g_assert (error->code == ARV_GC_ERROR_OUT_OF_RANGE);
	g_clear_error (&error);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_0_31), 0x10101010, &error);
	g_assert (error == NULL);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_0_15), NULL);
	g_assert_cmpint (value, ==, 0x1010);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_0_15), 0xabcdefaa, &error);
	g_assert (error != NULL);
	g_assert (error->code == ARV_GC_ERROR_OUT_OF_RANGE);
	g_clear_error (&error);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_0_31), 0xabcdefaa, &error);
	g_assert (error == NULL);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_0_15), NULL);
	g_assert_cmpint (value, ==, 0xabcd);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_16_31), NULL);
	g_assert_cmpint (value, ==, -4182);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_15), NULL);
	g_assert_cmpint (value, ==, 0x1);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_15), 0xff, &error);
	g_assert (error != NULL);
	g_assert (error->code == ARV_GC_ERROR_OUT_OF_RANGE);
	g_clear_error (&error);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_0_31), 0xffffff, &error);
	g_assert (error == NULL);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_15), NULL);
	g_assert_cmpint (value, ==, 1);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_16_31), NULL);
	g_assert_cmpint (value, ==, -1);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_16_31), 0xff, NULL);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_16_31), NULL);
	g_assert_cmpint (value, ==, 0xff);

	arv_gc_integer_set_value (ARV_GC_INTEGER (node_15), 0x0, NULL);

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node_15), NULL);
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
        gint n_buffer_filling;
	gint payload;
	gint counter = 0;
        guint n_infos;
        const char *info_name;
        GType info_type;

	camera = arv_camera_new ("Fake_1", &error);
	g_assert (ARV_IS_CAMERA (camera));
	g_assert (error == NULL);

	device = arv_camera_get_device (camera);
	g_assert (ARV_IS_DEVICE (device));

	fake_camera = arv_fake_device_get_fake_camera (ARV_FAKE_DEVICE (device));
	g_assert (ARV_IS_FAKE_CAMERA (fake_camera));

	stream = arv_camera_create_stream (camera, NULL, NULL, NULL, &error);
	g_assert (ARV_IS_STREAM (stream));
	g_assert (error == NULL);

	arv_fake_camera_set_fill_pattern (fake_camera, fill_pattern_cb, &counter, NULL);

	payload = arv_camera_get_payload (camera, NULL);
	arv_stream_push_buffer (stream,  arv_buffer_new (payload, NULL));
	arv_camera_set_acquisition_mode (camera, ARV_ACQUISITION_MODE_SINGLE_FRAME, NULL);
	arv_camera_start_acquisition (camera, NULL);
	buffer = arv_stream_pop_buffer (stream);
	arv_camera_stop_acquisition (camera, NULL);

	arv_fake_camera_set_fill_pattern (fake_camera, NULL, NULL, NULL);

	g_assert_cmpint (counter, ==, 1);

	g_assert (ARV_IS_BUFFER (buffer));

	arv_stream_get_statistics (stream, &n_completed_buffers, &n_failures, &n_underruns);
	g_assert_cmpint (n_completed_buffers, == ,1);
	g_assert_cmpint (n_failures, ==, 0);
        /* FIXME n_underruns should be 0 */
	/* g_assert_cmpint (n_underruns, ==, 0); */

	arv_stream_get_n_owned_buffers (stream, &n_input_buffers, &n_output_buffers, &n_buffer_filling);
	g_assert_cmpint (n_input_buffers, ==, 0);
	g_assert_cmpint (n_output_buffers, ==, 0);
        g_assert_cmpint (n_buffer_filling, == , 0);

        n_infos = arv_stream_get_n_infos (stream);
        g_assert_cmpint (n_infos, ==, 5);

        info_name = arv_stream_get_info_name (stream, 0);
        g_assert_cmpstr (info_name, ==, "n_completed_buffers");

        info_name = arv_stream_get_info_name (stream, 1);
        g_assert_cmpstr (info_name, ==, "n_failures");

        info_name = arv_stream_get_info_name (stream, 2);
        g_assert_cmpstr (info_name, ==, "n_underruns");

        info_type = arv_stream_get_info_type (stream, 0);
        g_assert_cmpint (info_type, ==, G_TYPE_UINT64);

        info_type = arv_stream_get_info_type (stream, 1);
        g_assert_cmpint (info_type, ==, G_TYPE_UINT64);

        info_type = arv_stream_get_info_type (stream, 2);
        g_assert_cmpint (info_type, ==, G_TYPE_UINT64);

        g_assert_cmpint (n_completed_buffers, ==, arv_stream_get_info_uint64_by_name (stream, "n_completed_buffers"));
        g_assert_cmpint (n_completed_buffers, ==, arv_stream_get_info_uint64 (stream, 0));

        g_assert_cmpint (n_failures, ==, arv_stream_get_info_uint64_by_name (stream, "n_failures"));
        g_assert_cmpint (n_failures, ==, arv_stream_get_info_uint64 (stream, 1));

        g_assert_cmpint (n_underruns, ==, arv_stream_get_info_uint64_by_name (stream, "n_underruns"));
        g_assert_cmpint (n_underruns, ==, arv_stream_get_info_uint64 (stream, 2));

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

	arv_camera_set_pixel_format (camera, ARV_PIXEL_FORMAT_BAYER_BG_8, &error);
	g_assert (error == NULL);
	pixel_format = arv_camera_get_pixel_format (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpint (pixel_format, ==, ARV_PIXEL_FORMAT_BAYER_BG_8);

	string = arv_camera_get_pixel_format_as_string (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpstr (string, ==, "BayerBG8");

	arv_camera_set_pixel_format (camera, ARV_PIXEL_FORMAT_BAYER_GB_8, &error);
	g_assert (error == NULL);
	pixel_format = arv_camera_get_pixel_format (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpint (pixel_format, ==, ARV_PIXEL_FORMAT_BAYER_GB_8);

	string = arv_camera_get_pixel_format_as_string (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpstr (string, ==, "BayerGB8");

	arv_camera_set_pixel_format (camera, ARV_PIXEL_FORMAT_BAYER_GR_8, &error);
	g_assert (error == NULL);
	pixel_format = arv_camera_get_pixel_format (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpint (pixel_format, ==, ARV_PIXEL_FORMAT_BAYER_GR_8);

	string = arv_camera_get_pixel_format_as_string (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpstr (string, ==, "BayerGR8");

	arv_camera_set_pixel_format (camera, ARV_PIXEL_FORMAT_BAYER_RG_8, &error);
	g_assert (error == NULL);
	pixel_format = arv_camera_get_pixel_format (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpint (pixel_format, ==, ARV_PIXEL_FORMAT_BAYER_RG_8);

	string = arv_camera_get_pixel_format_as_string (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpstr (string, ==, "BayerRG8");

	arv_camera_set_pixel_format_from_string (camera, "Mono8", &error);
	g_assert (error == NULL);
	pixel_format = arv_camera_get_pixel_format (camera, &error);
	g_assert (error == NULL);
	g_assert_cmpint (pixel_format, ==, ARV_PIXEL_FORMAT_MONO_8);

	ptr = arv_camera_dup_available_pixel_formats (camera, &n, &error);
	g_assert (error == NULL);
	g_assert (ptr != NULL);
	g_assert_cmpint (n, ==, 7);
	g_clear_pointer (&ptr, g_free);

	ptr = arv_camera_dup_available_pixel_formats_as_strings (camera, &n, &error);
	g_assert (error == NULL);
	g_assert (ptr != NULL);
	g_assert_cmpint (n, ==, 7);
	g_clear_pointer (&ptr, g_free);

	ptr = arv_camera_dup_available_pixel_formats_as_display_names (camera, &n, &error);
	g_assert (error == NULL);
	g_assert (ptr != NULL);
	g_assert_cmpint (n, ==, 7);
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
set_flag (gpointer user_data, GObject *object)
{
	gboolean *flag = user_data;

	*flag = TRUE;
}

static void
camera_device_test (void)
{
	ArvDevice *device;
	ArvDevice *camera_device;
	ArvCamera *camera;
	GError *error = NULL;
	gboolean device_released = FALSE;

	device = arv_fake_device_new ("TEST0", &error);
	g_assert (ARV_IS_FAKE_DEVICE (device));
	g_assert (error == NULL);

	g_object_weak_ref (G_OBJECT (device), set_flag, &device_released);

	camera = arv_camera_new_with_device (device, &error);
	g_assert (ARV_IS_CAMERA (camera));
	g_assert (error == NULL);

	camera_device = arv_camera_get_device (camera);
	g_assert (ARV_IS_FAKE_DEVICE (camera_device));
	g_assert (device == camera_device);

	g_assert (!device_released);

	g_object_unref (device);
	g_assert (!device_released);

	g_object_unref (camera);
	g_assert (device_released);

	g_test_expect_message (G_LOG_DOMAIN,
			       G_LOG_LEVEL_CRITICAL,
			       "*assertion*");
	arv_camera_new_with_device (NULL, NULL);
	g_test_assert_expected_messages ();

}

static void
camera_trigger_selector_test (void)
{
	ArvCamera *camera;
	GError *error = NULL;
	const char *string;

	camera = arv_camera_new ("Fake_1", &error);
	g_assert (ARV_IS_CAMERA (camera));
	g_assert (error == NULL);

	/* Enable TriggerMode on FrameStart */
	arv_camera_set_string (camera, "TriggerSelector", "FrameStart", &error);
	g_assert (error == NULL);
	arv_camera_set_string (camera, "TriggerMode", "On", &error);
	g_assert (error == NULL);
	string = arv_camera_get_string(camera, "TriggerMode", &error);
	g_assert (error == NULL);
	g_assert_cmpstr (string, ==, "On");

	/* Disable TriggerMode on AcquisitionStart */
	arv_camera_set_string (camera, "TriggerSelector", "AcquisitionStart", &error);
	g_assert (error == NULL);
	arv_camera_set_string (camera, "TriggerMode", "Off", &error);
	g_assert (error == NULL);
	string = arv_camera_get_string(camera, "TriggerMode", &error);
	g_assert (error == NULL);
	g_assert_cmpstr (string, ==, "Off");

	/* Re-check TriggerMode value on FrameStart */
	arv_camera_set_string (camera, "TriggerSelector", "FrameStart", &error);
	g_assert (error == NULL);
	string = arv_camera_get_string(camera, "TriggerMode", &error);
	g_assert (error == NULL);
	g_assert_cmpstr (string, ==, "On");

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

	success = arv_device_set_features_from_string (device, "R[0x1f0]", &error);
	g_assert (!success);
	g_assert (error != NULL);
	g_clear_error (&error);

	success = arv_device_set_features_from_string (device, "R[0x1f0]=10", &error);
	g_assert (success);
	g_assert (error == NULL);
        int_value = arv_device_get_integer_feature_value (device, "TestRegister", &error);
	g_assert (error == NULL);
        g_assert_cmpint(int_value, ==, 10);

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

	g_test_add_func ("/fake/discovery-test", discovery_test);
	g_test_add_func ("/fake/trigger-registers", trigger_registers_test);
	g_test_add_func ("/fake/registers", registers_test);
	g_test_add_func ("/fake/fake-device", fake_device_test);
	g_test_add_func ("/fake/fake-device-error", fake_device_error_test);
	g_test_add_func ("/fake/fake-stream", fake_stream_test);
	g_test_add_func ("/fake/camera-api", camera_api_test);
	g_test_add_func ("/fake/camera-device", camera_device_test);
	g_test_add_func ("/fake/camera-trigger-selector", camera_trigger_selector_test);
	g_test_add_func ("/fake/set-features-from-string", set_features_from_string_test);

	result = g_test_run();

	arv_shutdown ();

	return result;
}

