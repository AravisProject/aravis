#include <arv.h>
#include <arvgvinterface.h>
#include <arvgvstream.h>

#define ARV_GC1380_ACQUISITION_CONTROL		0x000130f4
#define ARV_GC1380_ACQUISITION_STOP		0
#define ARV_GC1380_ACQUISITION_START		1
#define ARV_GC1380_ACQUISITION_ABORT		2

int
main (int argc, char **argv)
{
	ArvInterface *interface;
	ArvDevice *device;
	ArvStream *stream;
	ArvBuffer *buffer;
	char memory_buffer[100000];
	char name[ARV_GVBS_USER_DEFINED_NAME_SIZE] = "lapp-vicam02";
	guint32 stream_port;

	g_thread_init (NULL);
	g_type_init ();

	interface = arv_gv_interface_get_instance ();

	device = arv_interface_get_first_device (interface);

	stream = arv_gv_stream_new (0);
	arv_stream_push_buffer (stream, arv_buffer_new (200, NULL));
	arv_stream_push_buffer (stream, arv_buffer_new (200, NULL));
	arv_stream_push_buffer (stream, arv_buffer_new (200, NULL));
	arv_stream_push_buffer (stream, arv_buffer_new (200, NULL));

	if (device != NULL) {
		arv_device_read_memory (device, 0x00014150, 8, memory_buffer);
		arv_device_read_memory (device, 0x000000e8, 16, memory_buffer);
		arv_device_read_memory (device,
					ARV_GVBS_USER_DEFINED_NAME,
					ARV_GVBS_USER_DEFINED_NAME_SIZE, memory_buffer);

		arv_device_write_register (device, ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE, 2);
		arv_device_write_memory (device,
					 ARV_GVBS_USER_DEFINED_NAME,
					 ARV_GVBS_USER_DEFINED_NAME_SIZE, name);

		stream_port = arv_gv_stream_get_port (ARV_GV_STREAM (stream));
		g_message ("stream port = %d", stream_port);

		arv_device_write_register (device, ARV_GVBS_FIRST_STREAM_CHANNEL_PORT, stream_port);
		g_message ("written stream port = %d", arv_device_read_register (device,
										 ARV_GVBS_FIRST_STREAM_CHANNEL_PORT));

		arv_device_write_register (device, ARV_GVBS_FIRST_STREAM_CHANNEL_IP_ADDRESS, 0x0a2a2b01);

		g_usleep (100000);

		arv_device_write_register (device, ARV_GVBS_FIRST_STREAM_CHANNEL_PACKET_SIZE, 0xf0000200);

		arv_device_write_register (device, ARV_GC1380_ACQUISITION_CONTROL, ARV_GC1380_ACQUISITION_START);

		g_usleep (3000000);

		arv_device_read_register (device, ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE);

		buffer = arv_stream_pop_buffer (stream);
		if (buffer != NULL) {
			g_message ("Image %dx%d (id: %d - status: %d)",
				   buffer->width, buffer->height, buffer->frame_id, buffer->status);
			arv_stream_push_buffer (stream, buffer);
		}

		buffer = arv_stream_pop_buffer (stream);
		if (buffer != NULL) {
			g_message ("Image %dx%d (id: %d - status: %d)",
				   buffer->width, buffer->height, buffer->frame_id, buffer->status);
			arv_stream_push_buffer (stream, buffer);
		}

		g_usleep (3000000);

		arv_device_read_register (device, ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE);

		g_usleep (3000000);

		arv_device_read_register (device, ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE);

		g_usleep (3000000);

		arv_device_write_register (device, ARV_GC1380_ACQUISITION_CONTROL, ARV_GC1380_ACQUISITION_STOP);

		arv_device_write_register (device, ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE, 0);

		g_object_unref (device);
	} else
		g_message ("No device found");

	g_object_unref (stream);

	g_object_unref (interface);

	return 0;
}
