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
	char name[ARV_GVBS_USER_DEFINED_NAME_SIZE] = "lapp-vicam02";
	guint16 stream_port;

	g_type_init ();

	interface = arv_gv_interface_get_instance ();

	device = arv_interface_get_first_device (interface);

	stream = arv_gv_stream_new (0);

	if (device != NULL) {
		arv_device_read_memory (device, 0x00014150, 8, buffer);
		arv_device_read_memory (device, 0x000000e8, 16, buffer);
		arv_device_read_memory (device,
					ARV_GVBS_USER_DEFINED_NAME,
					ARV_GVBS_USER_DEFINED_NAME_SIZE, buffer);
		arv_device_read_memory (device,
					ARV_GVBS_SECOND_XML_URL,
					ARV_GVBS_XML_URL_SIZE, buffer);
		arv_device_read_memory (device,
					0x00100000, 0x00015904, buffer);

		arv_device_write_register (device, ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE, 2);
		arv_device_write_memory (device,
					 ARV_GVBS_USER_DEFINED_NAME,
					 ARV_GVBS_USER_DEFINED_NAME_SIZE, name);

		stream_port = arv_gv_stream_get_port (ARV_GV_STREAM (stream));
		g_message ("stream port = %d", stream_port);

		arv_device_write_register (device, ARV_GVBS_FIRST_STREAM_CHANNEL_PORT, stream_port);
		arv_device_read_register (device, ARV_GVBS_FIRST_STREAM_CHANNEL_PORT);

		g_usleep (10000000);

		arv_device_write_register (device, ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE, 0);

		g_file_set_contents ("/tmp/genicam.xml", buffer, 0x00015904, NULL);

		g_object_unref (device);
	} else
		g_message ("No device found");

	g_object_unref (stream);

	g_object_unref (interface);

	return 0;
}
