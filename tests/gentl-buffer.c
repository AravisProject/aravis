/* Aravis - Digital camera library
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "private.h"

GENTL_THREAD_LOCAL_STORAGE GError *gentl_err = NULL;

GQuark
gentl_error_quark (void)
{
	return g_quark_from_static_string ("gentl-test-error-quark");
}

static void
test_null_string_list (void)
{
	char destination = 1;
	size_t size = 0;

	g_assert_cmpint (gentl_to_buf (INFO_DATATYPE_STRINGLIST, NULL, NULL, &size, NULL), ==,
			 GC_ERR_SUCCESS);
	g_assert_cmpuint (size, ==, 1);

	g_assert_cmpint (gentl_to_buf (INFO_DATATYPE_STRINGLIST, &destination, NULL, &size, NULL), ==,
			 GC_ERR_SUCCESS);
	g_assert_cmpuint (size, ==, 1);
	g_assert_cmpint (destination, ==, '\0');
}

static void
test_string_list (void)
{
	const char source[] = "one\0two\0";
	char destination[sizeof source] = { 0 };
	size_t size = sizeof destination;

	g_assert_cmpint (gentl_to_buf (INFO_DATATYPE_STRINGLIST, destination, source, &size, NULL), ==,
			 GC_ERR_SUCCESS);
	g_assert_cmpuint (size, ==, sizeof source);
	g_assert_cmpmem (destination, size, source, sizeof source);
}

static void
test_buffer_too_small (void)
{
	const char source[] = "Aravis";
	char destination[2];
	size_t size = sizeof destination;

	g_assert_cmpint (gentl_to_buf (INFO_DATATYPE_STRING, destination, source, &size, NULL), ==,
			 GC_ERR_BUFFER_TOO_SMALL);
	g_assert_cmpuint (size, ==, sizeof source);
}

static void
test_unaligned_integer (void)
{
	guint32 source = 0x12345678;
	guint8 destination[sizeof source + 1] = { 0 };
	size_t size = sizeof source;

	g_assert_cmpint (gentl_to_buf (INFO_DATATYPE_UINT32, destination + 1, &source, &size, NULL), ==,
			 GC_ERR_SUCCESS);
	g_assert_cmpmem (destination + 1, sizeof source, &source, sizeof source);
}

int
main (int argc, char **argv)
{
	int result;

	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/gentl/buffer/null-string-list", test_null_string_list);
	g_test_add_func ("/gentl/buffer/string-list", test_string_list);
	g_test_add_func ("/gentl/buffer/too-small", test_buffer_too_small);
	g_test_add_func ("/gentl/buffer/unaligned-integer", test_unaligned_integer);

	result = g_test_run ();
	g_clear_error (&gentl_err);

	return result;
}
