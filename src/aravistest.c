#include <arv.h>
#include <arvgvinterface.h>
#include <arvgvstream.h>
#include <arvgvdevice.h>

#define ARV_GC1380_PAYLOAD_SIZE			0x00012200
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
	int i;

	g_thread_init (NULL);
	g_type_init ();

	interface = arv_gv_interface_get_instance ();

	device = arv_interface_get_first_device (interface);
	if (device != NULL) {
		guint32 value;

		stream = arv_device_get_stream (device);

		arv_device_read_register (device, ARV_GC1380_PAYLOAD_SIZE, &value);
		g_message ("payload size = %d", value);

		for (i = 0; i < 30; i++)
			arv_stream_push_buffer (stream, arv_buffer_new (value, NULL));

		arv_device_read_register (device, ARV_GVBS_FIRST_STREAM_CHANNEL_PORT, &value);
		g_message ("stream port = %d (%d)", value, arv_gv_stream_get_port (ARV_GV_STREAM (stream)));

		arv_device_read_memory (device, 0x00014150, 8, memory_buffer);
		arv_device_read_memory (device, 0x000000e8, 16, memory_buffer);
		arv_device_read_memory (device,
					ARV_GVBS_USER_DEFINED_NAME,
					ARV_GVBS_USER_DEFINED_NAME_SIZE, memory_buffer);

		arv_device_write_memory (device,
					 ARV_GVBS_USER_DEFINED_NAME,
					 ARV_GVBS_USER_DEFINED_NAME_SIZE, name);

		arv_device_write_register (device, ARV_GC1380_ACQUISITION_CONTROL, ARV_GC1380_ACQUISITION_START);

		g_usleep (3000000);

		do  {
			buffer = arv_stream_pop_buffer (stream);
			if (buffer != NULL) {
				g_message ("Image %dx%d (id: %d - status: %d)",
					   buffer->width, buffer->height, buffer->frame_id, buffer->status);
				arv_stream_push_buffer (stream, buffer);
			}
		} while (buffer != NULL);

		g_usleep (10000000);

		arv_device_write_register (device, ARV_GC1380_ACQUISITION_CONTROL, ARV_GC1380_ACQUISITION_STOP);

		g_object_unref (stream);
		g_object_unref (device);
	} else
		g_message ("No device found");

	g_object_unref (interface);

	return 0;
}
