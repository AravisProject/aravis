/* SPDX-License-Identifier:Unlicense */

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

static void
write_lock_test (void)
{
	ArvDevice *device;
	ArvGc *genicam;
	GError *error = NULL;
	ArvGcNode *node;
	ArvGcNode *property;
	ArvDomNode *result;
	unsigned int length;

	device = arv_fake_device_new ("TEST0", &error);
	g_assert_no_error (error);

	genicam = arv_device_get_genicam (device);

	/* The device Genicam data are write locked once the device is created */
	g_assert_true (arv_dom_document_get_write_lock (ARV_DOM_DOCUMENT (genicam)));

	node = arv_gc_get_node (genicam, "Root");
	length = arv_dom_node_list_get_length (arv_dom_node_get_child_nodes (ARV_DOM_NODE (node)));

	g_test_expect_message (NULL, G_LOG_LEVEL_CRITICAL, "*write_lock*");
	arv_dom_element_set_attribute (ARV_DOM_ELEMENT (node), "Name", "NotRoot");
	g_test_assert_expected_messages ();
	g_assert_cmpstr (arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (node)), ==, "Root");

	property = arv_gc_property_node_new_p_feature ();
	arv_dom_node_append_child (ARV_DOM_NODE (property),
				   ARV_DOM_NODE (arv_dom_document_create_text_node (ARV_DOM_DOCUMENT (genicam), "Width")));

	g_test_expect_message (NULL, G_LOG_LEVEL_CRITICAL, "*write locked*");
	result = arv_dom_node_append_child (ARV_DOM_NODE (node), ARV_DOM_NODE (g_object_ref (property)));
	g_test_assert_expected_messages ();
	g_assert_null (result);
	g_assert_cmpuint (arv_dom_node_list_get_length (arv_dom_node_get_child_nodes (ARV_DOM_NODE (node))), ==, length);

	g_test_expect_message (NULL, G_LOG_LEVEL_CRITICAL, "*write_lock*");
	arv_dom_document_append_from_memory (ARV_DOM_DOCUMENT (genicam), NULL, "<Integer Name=\"Foo\"/>", -1, NULL);
	g_test_assert_expected_messages ();
	g_assert_null (arv_gc_get_node (genicam, "Foo"));

	/* Back to a regular, not thread safe, DOM tree */
	arv_dom_document_set_write_lock (ARV_DOM_DOCUMENT (genicam), FALSE);
	g_assert_false (arv_dom_document_get_write_lock (ARV_DOM_DOCUMENT (genicam)));

	result = arv_dom_node_append_child (ARV_DOM_NODE (node), ARV_DOM_NODE (property));
	g_assert_true (result == ARV_DOM_NODE (property));
	g_assert_cmpuint (arv_dom_node_list_get_length (arv_dom_node_get_child_nodes (ARV_DOM_NODE (node))), ==,
			  length + 1);

	arv_dom_document_set_write_lock (ARV_DOM_DOCUMENT (genicam), TRUE);

	g_object_unref (device);
}

int
main (int argc, char *argv[])
{
	int result;

	g_test_init (&argc, &argv, NULL);

	arv_set_fake_camera_genicam_filename (GENICAM_FILENAME);

	g_test_add_func ("/dom/child-list", child_list_test);
	g_test_add_func ("/dom/write-lock", write_lock_test);

	result = g_test_run();

	arv_shutdown ();

	return result;
}
