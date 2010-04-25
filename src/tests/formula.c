#include <glib.h>
#include <arv.h>

static void
test_int (void)
{
	ArvCamera *camera;

	camera = arv_camera_new (NULL);
	if (camera != NULL)
		g_object_unref (camera);

	g_assert (TRUE);
}

int
main (int   argc,
      char *argv[])
{
	g_test_init (&argc, &argv, NULL);

	g_type_init ();

	g_test_add_func ("/formula/test_int", test_int);

	return g_test_run();
}
