#include <glib.h>
#include <arv.h>
#include <string.h>

static void
unaligned_from_le_ptr_test (void)
{
	char test[]= {'\x11', '\x22', '\x33', '\x44','\x55', '\x66', '\x77', '\x88', '\x99', '\xaa', '\xbb', '\xcc'};
	guint32 v_uint32;
	guint16 v_uint16;

	v_uint32 = ARV_GUINT32_FROM_LE_PTR (&test[0], 0);
	g_assert_cmpuint (v_uint32, ==, 0x44332211);

	v_uint32 = ARV_GUINT32_FROM_LE_PTR (&test[1], 0);
	g_assert_cmpuint (v_uint32, ==, 0x55443322);
	
	v_uint32 = ARV_GUINT32_FROM_LE_PTR (&test[2], 0);
	g_assert_cmpuint (v_uint32, ==, 0x66554433);
	
	v_uint32 = ARV_GUINT32_FROM_LE_PTR (&test[3], 0);
	g_assert_cmpuint (v_uint32, ==, 0x77665544);

	v_uint16 = ARV_GUINT16_FROM_LE_PTR (&test[0], 0);
	g_assert_cmpuint (v_uint16, ==, 0x2211);
	
	v_uint16 = ARV_GUINT16_FROM_LE_PTR (&test[1], 0);
	g_assert_cmpuint (v_uint16, ==, 0x3322);
	
	v_uint16 = ARV_GUINT16_FROM_LE_PTR (&test[2], 0);
	g_assert_cmpuint (v_uint16, ==, 0x4433);
	
	v_uint16 = ARV_GUINT16_FROM_LE_PTR (&test[3], 0);
	g_assert_cmpuint (v_uint16, ==, 0x5544);
}

int
main (int argc, char *argv[])
{
	int result;
	int i;

	g_test_init (&argc, &argv, NULL);

	arv_g_type_init ();

	g_test_add_func ("/buffer/unaligned-from-le", unaligned_from_le_ptr_test);

	result = g_test_run();

	arv_shutdown ();

	return result;
}


