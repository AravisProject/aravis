#include <glib.h>
#include <arv.h>

static void
integer_test (void)
{
	ArvDevice *device;
	ArvGc *genicam;
	ArvGcNode *node;
	gint64 v_int64;

	device = arv_fake_device_new ("TEST0");
	g_assert (ARV_IS_FAKE_DEVICE (device));

	genicam = arv_device_get_genicam (device);
	g_assert (ARV_IS_GC (genicam));

	node = arv_gc_get_node (genicam, "RWInteger");
	g_assert (ARV_IS_GC_INTEGER_NODE (node));

	v_int64 = arv_gc_integer_get_value (ARV_GC_INTEGER (node));
	g_assert_cmpint (v_int64, ==, 1);

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

int
main (int argc, char *argv[])
{
	int result;
	int i;

	g_test_init (&argc, &argv, NULL);

	g_type_init ();

	arv_set_fake_camera_genicam_filename (GENICAM_FILENAME);

	g_test_add_func ("/genicam/integer", integer_test);
	g_test_add_func ("/genicam/enumeration", enumeration_test);

	result = g_test_run();

	arv_shutdown ();

	return result;
}

