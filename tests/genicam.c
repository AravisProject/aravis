#include <glib.h>
#include <arv.h>

static void
integer_test (void)
{
	ArvDevice *device;
	ArvGc *genicam;
	ArvGcNode *node;
	gint64 v_int64;
	const char *v_string;

	device = arv_fake_device_new ("TEST0");
	g_assert (ARV_IS_FAKE_DEVICE (device));

	genicam = arv_device_get_genicam (device);
	g_assert (ARV_IS_GC (genicam));

	node = arv_gc_get_node (genicam, "RWInteger");
	g_assert (ARV_IS_GC_INTEGER_NODE (node));

	v_int64 = arv_gc_integer_get_value (ARV_GC_INTEGER (node));
	g_assert_cmpint (v_int64, ==, 1);

	v_string = arv_gc_node_get_value_as_string (node);
	g_assert_cmpstr (v_string, ==, "1");

	g_object_unref (device);
}

static void
boolean_test (void)
{
	ArvDevice *device;
	ArvGc *genicam;
	ArvGcNode *node;
	gboolean v_boolean;
	const char *v_string;

	device = arv_fake_device_new ("TEST0");
	g_assert (ARV_IS_FAKE_DEVICE (device));

	genicam = arv_device_get_genicam (device);
	g_assert (ARV_IS_GC (genicam));

	node = arv_gc_get_node (genicam, "RWBoolean");
	g_assert (ARV_IS_GC_BOOLEAN (node));

	v_boolean = arv_gc_boolean_get_value (ARV_GC_BOOLEAN (node));
	g_assert_cmpint (v_boolean, ==, TRUE);

	v_string = arv_gc_node_get_value_as_string (node);
	g_assert_cmpstr (v_string, ==, "true");

	g_object_unref (device);
}

static void
float_test (void)
{
	ArvDevice *device;
	ArvGc *genicam;
	ArvGcNode *node;
	double v_double;
	const char *v_string;

	device = arv_fake_device_new ("TEST0");
	g_assert (ARV_IS_FAKE_DEVICE (device));

	genicam = arv_device_get_genicam (device);
	g_assert (ARV_IS_GC (genicam));

	node = arv_gc_get_node (genicam, "RWFloat");
	g_assert (ARV_IS_GC_FLOAT_NODE (node));

	v_double = arv_gc_float_get_value (ARV_GC_FLOAT (node));
	g_assert_cmpfloat (v_double, ==, 0.1);

	v_string = arv_gc_node_get_value_as_string (node);
	g_assert_cmpstr (v_string, ==, "0.1");

	g_object_unref (device);
}

static void
enumeration_test (void)
{
	ArvDevice *device;
	ArvGc *genicam;
	ArvGcNode *node;
	gint64 v_int64;
	gint64 *values;
	guint n_values;

	device = arv_fake_device_new ("TEST0");
	g_assert (ARV_IS_FAKE_DEVICE (device));

	genicam = arv_device_get_genicam (device);
	g_assert (ARV_IS_GC (genicam));

	node = arv_gc_get_node (genicam, "Enumeration");
	g_assert (ARV_IS_GC_ENUMERATION (node));

	v_int64 = arv_gc_enumeration_get_int_value (ARV_GC_ENUMERATION (node));
	g_assert_cmpint (v_int64, ==, 0);

	values = arv_gc_enumeration_get_available_int_values (ARV_GC_ENUMERATION (node), &n_values);
	g_assert_cmpint (n_values, ==, 2);
	g_assert (values != NULL);

	g_free (values);

	g_object_unref (device);
}

GRegex *arv_gv_device_get_url_regex (void);

static void
url_test (void)
{
	char **tokens;
	unsigned int i;

	tokens = g_regex_split (arv_gv_device_get_url_regex (), "Local:Basler_Ace_GigE_e7c9b87e_Version_3_3.zip;c0000000;10cca", 0);

	g_assert_cmpint (g_strv_length (tokens), ==, 6);

	g_assert_cmpstr (tokens[0], ==, "");
	g_assert_cmpstr (tokens[1], ==, "Local:");
	g_assert_cmpstr (tokens[2], ==, "Basler_Ace_GigE_e7c9b87e_Version_3_3.zip");
	g_assert_cmpstr (tokens[3], ==, "c0000000");
	g_assert_cmpstr (tokens[4], ==, "10cca");

	g_strfreev (tokens);
}

int
main (int argc, char *argv[])
{
	int result;
	int i;

	g_test_init (&argc, &argv, NULL);

	g_type_init ();

	arv_set_fake_camera_genicam_filename (GENICAM_FILENAME);

	g_test_add_func ("/genicam/integer", integer_test);
	g_test_add_func ("/genicam/boolean", boolean_test);
	g_test_add_func ("/genicam/float", float_test);
	g_test_add_func ("/genicam/enumeration", enumeration_test);
	g_test_add_func ("/genicam/url", url_test);

	result = g_test_run();

	arv_shutdown ();

	return result;
}

