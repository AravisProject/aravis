#include <arv.h>
#include <arvgvinterface.h>

int
main (int argc, char **argv)
{
	ArvInterface *interface;

	g_type_init ();

	interface = arv_gv_interface_get_instance ();

	arv_interface_get_devices (interface);

	g_object_unref (interface);

	return 0;
}
