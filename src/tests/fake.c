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

int
main (int argc, char *argv[])
{
	int i;

	g_test_init (&argc, &argv, NULL);

	g_type_init ();

	g_test_add_func ("/fake/load-fake-camera-genicam", load_fake_camera_genicam_test);

	return g_test_run();
}

