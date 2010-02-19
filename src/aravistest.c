#include <arv.h>
#include <arvgvinterface.h>

int
main (int argc, char **argv)
{
	ArvInterface *interface;
	ArvDevice *device;

	g_type_init ();

	interface = arv_gv_interface_get_instance ();

	device = arv_interface_get_first_device (interface);

	g_object_unref (device);
	g_object_unref (interface);

	return 0;
}
