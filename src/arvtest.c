#include <arv.h>
#include <arvgvinterface.h>
#include <arvgvstream.h>
#include <arvgvdevice.h>
#ifdef ARAVIS_WITH_CAIRO
#include <cairo.h>
#endif

#ifdef PROSILICA		/* Prosilica */
#define ARV_PAYLOAD_SIZE		0x00012200
#define ARV_ACQUISITION_CONTROL		0x000130f4
#define ARV_ACQUISITION_STOP		0
#define ARV_ACQUISITION_START		1
#else				/* Basler */
#define ARV_PAYLOAD_SIZE		0xf1f0003c
#define ARV_ACQUISITION_CONTROL		0xf0f00614
#define ARV_ACQUISITION_STOP		0
#define ARV_ACQUISITION_START		0x80000000
#endif

int
main (int argc, char **argv)
{
	ArvInterface *interface;
	ArvDevice *device;
	ArvStream *stream;
	ArvBuffer *buffer;
	char memory_buffer[100000];
#ifdef PROSILICA
	char name[ARV_GVBS_USER_DEFINED_NAME_SIZE] = "lapp-vicam02";
#else
	char name[ARV_GVBS_USER_DEFINED_NAME_SIZE] = "lapp-vicam01";
#endif
	int i;
#ifdef ARAVIS_WITH_CAIRO
	gboolean snapshot_done = FALSE;
#endif

	g_thread_init (NULL);
	g_type_init ();

#ifdef PROSILICA
	g_message ("Testing Prosilica camera");
#else
	g_message ("Testing Basler camera");
#endif

	interface = arv_gv_interface_get_instance ();

	device = arv_interface_get_first_device (interface);
	if (device != NULL) {
		guint32 value;

		stream = arv_device_get_stream (device);

		arv_device_read_register (device, ARV_PAYLOAD_SIZE, &value);
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

		arv_device_write_register (device, ARV_ACQUISITION_CONTROL, ARV_ACQUISITION_START);

		g_usleep (3000000);

		do  {
			buffer = arv_stream_pop_buffer (stream);
			if (buffer != NULL) {
#ifdef ARAVIS_WITH_CAIRO
				if (!snapshot_done &&
				    buffer->status == ARV_BUFFER_STATUS_SUCCESS) {
					snapshot_done = TRUE;

					cairo_surface_t *surface;

					surface = cairo_image_surface_create_for_data (buffer->data,
										       CAIRO_FORMAT_A8,
										       buffer->width,
										       buffer->height,
										       buffer->width);
					cairo_surface_write_to_png (surface, "test.png");
					cairo_surface_destroy (surface);
				}
#endif
				g_message ("Image %dx%d (id: %d - status: %d)",
					   buffer->width, buffer->height, buffer->frame_id, buffer->status);
				arv_stream_push_buffer (stream, buffer);
			}
		} while (buffer != NULL);

		g_usleep (10000000);

		arv_device_read_register (device, ARV_GVBS_FIRST_STREAM_CHANNEL_PORT, &value);
		g_message ("stream port = %d (%d)", value, arv_gv_stream_get_port (ARV_GV_STREAM (stream)));

		arv_device_write_register (device, ARV_ACQUISITION_CONTROL, ARV_ACQUISITION_STOP);

		g_object_unref (stream);
		g_object_unref (device);
	} else
		g_message ("No device found");

	g_object_unref (interface);

	return 0;
}
