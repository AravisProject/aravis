#include <arv.h>
#include <arvgvinterface.h>

int
main (int argc, char **argv)
{
	ArvInterface *interface;
	ArvDevice *device;
	char buffer[1024];

	g_type_init ();

	interface = arv_gv_interface_get_instance ();

	device = arv_interface_get_first_device (interface);

	arv_device_read_register (device, 0x00014150, 8, buffer);
	arv_device_read_register (device, 0x000000e8, 16, buffer);
	arv_device_read_register (device,
				  ARV_GVCP_GENICAM_FILENAME_ADDRESS_1,
				  ARV_GVCP_GENICAM_FILENAME_SIZE, buffer);
	arv_device_read_register (device,
				  ARV_GVCP_GENICAM_FILENAME_ADDRESS_2,
				  ARV_GVCP_GENICAM_FILENAME_SIZE, buffer);
	arv_device_read_register (device,
				  0x00100000, 0x00015904, buffer);

	g_object_unref (device);
	g_object_unref (interface);

	return 0;
}
