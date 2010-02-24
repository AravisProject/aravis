#include <arv.h>
#include <arvgvinterface.h>
#include <arvgvstream.h>

int
main (int argc, char **argv)
{
	ArvInterface *interface;
	ArvDevice *device;
	ArvStream *stream;
	char buffer[100000];
	char name[ARV_GVCP_GENICAM_USER_DEFINED_NAME_SIZE] = "lapp-vicam02";
	guint32 value;

	g_type_init ();

	interface = arv_gv_interface_get_instance ();

	device = arv_interface_get_first_device (interface);

	stream = arv_gv_stream_new (0);

	if (device != NULL) {
		arv_device_read (device, 0x00014150, 8, buffer);
		arv_device_read (device, 0x000000e8, 16, buffer);
		arv_device_read (device,
				 ARV_GVCP_GENICAM_FILENAME_ADDRESS_1,
				 ARV_GVCP_GENICAM_FILENAME_SIZE, buffer);
		arv_device_read (device,
				 ARV_GVCP_GENICAM_FILENAME_ADDRESS_2,
				 ARV_GVCP_GENICAM_FILENAME_SIZE, buffer);
		arv_device_read (device,
				 0x00100000, 0x00015904, buffer);

		value = g_htonl (2);
		arv_device_write (device,
				  ARV_GVCP_GENICAM_CONTROL_CHANNEL_PRIVILEGE_ADDRESS,
				  sizeof (value), &value);
		arv_device_write (device,
				  ARV_GVCP_GENICAM_USER_DEFINED_NAME_ADDRESS,
				  ARV_GVCP_GENICAM_USER_DEFINED_NAME_SIZE, name);

		value = g_htonl (arv_gv_stream_get_port (ARV_GV_STREAM (stream)));
		g_message ("port = %d", arv_gv_stream_get_port (ARV_GV_STREAM (stream)));

		arv_device_write (device,
				  ARV_GVCP_GENICAM_FIRST_STREAM_CHANNEL_PORT_ADDRESS,
				  sizeof (value), &value);
		arv_device_read (device,
				 ARV_GVCP_GENICAM_FIRST_STREAM_CHANNEL_PORT_ADDRESS,
				 sizeof (value), &value);

		g_usleep (10000000);

		value = g_htonl (0);
		arv_device_write (device,
				  ARV_GVCP_GENICAM_CONTROL_CHANNEL_PRIVILEGE_ADDRESS,
				  sizeof (value), &value);

		g_file_set_contents ("/tmp/genicam.xml", buffer, 0x00015904, NULL);

		g_object_unref (device);
	} else
		g_message ("No device found");

	g_object_unref (stream);

	g_object_unref (interface);

	return 0;
}
