#include <arv.h>
#include <arvgvinterface.h>
#include <arvgvdevice.h>

int
main (int argc, char **argv)
{
	ArvInterface *interface;
	ArvDevice *device;
	const char *genicam_data;
	size_t genicam_size;

	g_thread_init (NULL);
	g_type_init ();

	interface = arv_gv_interface_get_instance ();

	device = arv_interface_get_first_device (interface);
	if (device != NULL) {
		genicam_data = arv_device_get_genicam (device, &genicam_size);
		g_file_set_contents ("genicam.xml", genicam_data, genicam_size, NULL);
	} else
		g_message ("No device found");

	g_object_unref (interface);

	return 0;
}

