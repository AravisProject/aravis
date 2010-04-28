#include <glib.h>
#include <arv.h>

static void
load_genicam_test (void)
{
	ArvGc *genicam;
	ArvGcNode *node;
	char *genicam_data;
	size_t genicam_data_size;

	g_file_get_contents (GENICAM_FILENAME, &genicam_data, &genicam_data_size, NULL);
	g_assert (genicam_data != NULL);

	genicam = arv_gc_new (NULL, genicam_data, genicam_data_size);
	g_assert (ARV_IS_GC (genicam));

	node = arv_gc_get_node (genicam, "AcquisitionStart");
	g_assert (ARV_IS_GC_COMMAND (node));

	node = arv_gc_get_node (genicam, "Unknown");
	g_assert (!ARV_IS_GC_COMMAND (node));

	g_object_unref (genicam);

	g_free (genicam_data);
}

int
main (int argc, char *argv[])
{
	int i;

	g_test_init (&argc, &argv, NULL);

	g_type_init ();

	g_test_add_func ("/genicam/load-genicam", load_genicam_test);

	return g_test_run();
}
