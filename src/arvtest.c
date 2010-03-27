#include <arv.h>
#include <arvgvinterface.h>
#include <arvgvstream.h>
#include <arvgvdevice.h>
#include <arvgcinteger.h>
#include <arvdebug.h>
#ifdef ARAVIS_WITH_CAIRO
#include <cairo.h>
#endif
#include <stdlib.h>

static gboolean cancel = FALSE;

static void
set_cancel (int signal)
{
	cancel = TRUE;
}

typedef struct {
	guint32 payload_size;
	guint32 acquisition_control;
	guint32 acquisition_stop;
	guint32 acquisition_start;
} ArvCameraRegisters;

static ArvCameraRegisters arv_cameras[] = {
	{
		.payload_size = 		0xf1f0003c,
		.acquisition_control = 		0xf0f00614,
		.acquisition_stop =		0x00000000,
		.acquisition_start =		0x80000000
	},
	{
		.payload_size = 		0x00012200,
		.acquisition_control = 		0x000130f4,
		.acquisition_stop =		0x00000000,
		.acquisition_start =		0x00000001
	}
};

static char *arv_option_camera_type = "prosilica";
static int arv_option_debug_level;
static gboolean arv_option_snaphot = FALSE;
static gboolean arv_option_auto_buffer = FALSE;

static const GOptionEntry arv_option_entries[] =
{
	{ "type",		't', 0, G_OPTION_ARG_STRING,
		&arv_option_camera_type,	"Camera type", NULL},
	{ "snapshot",		's', 0, G_OPTION_ARG_NONE,
		&arv_option_snaphot,	"Snapshot", NULL},
	{ "auto",		'a', 0, G_OPTION_ARG_NONE,
		&arv_option_auto_buffer,	"AutoBufferSize", NULL},
	{ "debug", 		'd', 0, G_OPTION_ARG_INT,
		&arv_option_debug_level, 	"Debug mode", NULL },
	{ NULL }
};

typedef enum {
	ARV_CAMERA_TYPE_BASLER,
	ARV_CAMERA_TYPE_PROSILICA
} ArvCameraType;

int
main (int argc, char **argv)
{
	ArvInterface *interface;
	ArvDevice *device;
	ArvStream *stream;
	ArvBuffer *buffer;
	ArvCameraType camera_type;
	GOptionContext *context;
	GError *error = NULL;
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

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, arv_option_entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	arv_debug_enable (arv_option_debug_level);

	if (g_strcmp0 (arv_option_camera_type, "basler") == 0)
		camera_type = ARV_CAMERA_TYPE_BASLER;
	else if (g_strcmp0 (arv_option_camera_type, "prosilica") == 0)
		camera_type = ARV_CAMERA_TYPE_PROSILICA;
	else {
		g_print ("Unknow camera type: %s\n", arv_option_camera_type);
		return EXIT_FAILURE;
	}

	g_print ("Testing %s camera\n", arv_option_camera_type);

	interface = arv_gv_interface_get_instance ();

	device = arv_interface_get_first_device (interface);
	if (device != NULL) {
		ArvGc *genicam;
		ArvGcNode *node;
		guint32 value;

		stream = arv_device_get_stream (device);
		if (arv_option_auto_buffer)
			arv_gv_stream_set_option (ARV_GV_STREAM (stream),
						  ARV_GV_STREAM_OPTION_SOCKET_BUFFER_AUTO,
						  0);

		genicam = arv_device_get_genicam (device);
		node = arv_gc_get_node (genicam, "PayloadSize");
		value = arv_gc_integer_get_value (ARV_GC_INTEGER (node));
		g_print ("payload size = %d (0x%x)\n", value, value);

		arv_device_read_register (device, arv_cameras[camera_type].payload_size, &value);
		g_print ("payload size = %d (0x%x)\n", value, value);

		for (i = 0; i < 30; i++)
			arv_stream_push_buffer (stream, arv_buffer_new (value, NULL));

		arv_device_read_register (device, ARV_GVBS_FIRST_STREAM_CHANNEL_PORT, &value);
		g_print ("stream port = %d (%d)\n", value, arv_gv_stream_get_port (ARV_GV_STREAM (stream)));

		arv_device_read_memory (device, 0x00014150, 8, memory_buffer);
		arv_device_read_memory (device, 0x000000e8, 16, memory_buffer);
		arv_device_read_memory (device,
					ARV_GVBS_USER_DEFINED_NAME,
					ARV_GVBS_USER_DEFINED_NAME_SIZE, memory_buffer);

		arv_device_write_memory (device,
					 ARV_GVBS_USER_DEFINED_NAME,
					 ARV_GVBS_USER_DEFINED_NAME_SIZE, name);

		arv_device_write_register (device,
					  arv_cameras[camera_type].acquisition_control,
					  arv_cameras[camera_type].acquisition_start);

		signal (SIGINT, set_cancel);

		do {
			g_usleep (100000);

			do  {
				buffer = arv_stream_pop_buffer (stream);
				if (buffer != NULL) {
#ifdef ARAVIS_WITH_CAIRO
					if (arv_option_snaphot &&
					    !snapshot_done &&
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
					arv_stream_push_buffer (stream, buffer);
				}
			} while (buffer != NULL);
		} while (!cancel);

		arv_device_read_register (device, ARV_GVBS_FIRST_STREAM_CHANNEL_PORT, &value);
		g_print ("stream port = %d (%d)\n", value, arv_gv_stream_get_port (ARV_GV_STREAM (stream)));

		arv_device_write_register (device,
					  arv_cameras[camera_type].acquisition_control,
					  arv_cameras[camera_type].acquisition_stop);

		g_object_unref (stream);
		g_object_unref (device);
	} else
		g_print ("No device found\n");

	g_object_unref (interface);

	return 0;
}
