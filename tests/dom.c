#include <arv.h>

static void
child_list_test (void)
{
	ArvDevice *device;
	ArvGc *genicam;
	GError *error = NULL;
        ArvGcNode *node;
        ArvDomNodeList *children;
        ArvDomNode *item;
        unsigned int length, i;

	device = arv_fake_device_new ("TEST0", &error);
	g_assert (ARV_IS_FAKE_DEVICE (device));
	g_assert (error == NULL);

	genicam = arv_device_get_genicam (device);
	g_assert (ARV_IS_GC (genicam));

        node = arv_gc_get_node (genicam, "Root");

        children = arv_dom_node_get_child_nodes(ARV_DOM_NODE(node));
        length = arv_dom_node_list_get_length(children);
        g_assert (length > 0);

        for (i = 0; i < length; ++i) {
                item = arv_dom_node_list_get_item(children, i);
                g_assert (ARV_IS_DOM_NODE (item));
        }

        g_object_unref (device);
}

int
main (int argc, char *argv[])
{
	int result;

	g_test_init (&argc, &argv, NULL);

	arv_set_fake_camera_genicam_filename (GENICAM_FILENAME);

	g_test_add_func ("/dom/child-list", child_list_test);

	result = g_test_run();

	arv_shutdown ();

	return result;
}
