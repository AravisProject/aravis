#include <arv.h>
#include <stdlib.h>
#include "../src/arvgvsp.h"
#include "../src/arvgvcp.h"

static gboolean cancel = FALSE;

static void
set_cancel (int signal)
{
	cancel = TRUE;
}

static char *arv_option_camera_name = NULL;
static char *arv_option_debug_domains = NULL;
static gboolean arv_option_auto_buffer = FALSE;
static int arv_option_width = -1;
static int arv_option_height = -1;
static int arv_option_horizontal_binning = -1;
static int arv_option_vertical_binning = -1;

static const GOptionEntry arv_option_entries[] =
{
	{ "name",		'n', 0, G_OPTION_ARG_STRING,
		&arv_option_camera_name,"Camera name", NULL},
	{ "auto",		'a', 0, G_OPTION_ARG_NONE,
		&arv_option_auto_buffer,	"AutoBufferSize", NULL},
	{ "width", 		'w', 0, G_OPTION_ARG_INT,
		&arv_option_width,		"Width", NULL },
	{ "height", 		'h', 0, G_OPTION_ARG_INT,
		&arv_option_height, 		"Height", NULL },
	{ "h-binning", 		'\0', 0, G_OPTION_ARG_INT,
		&arv_option_horizontal_binning,"Horizontal binning", NULL },
	{ "v-binning", 		'\0', 0, G_OPTION_ARG_INT,
		&arv_option_vertical_binning, 	"Vertical binning", NULL },
	{ "debug", 		'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	"Debug mode", NULL },
	{ NULL }
};

typedef enum {
	ARV_CAMERA_TYPE_BASLER,
	ARV_CAMERA_TYPE_PROSILICA
} ArvCameraType;

int
main (int argc, char **argv)
{
	ArvDevice *device;
	ArvStream *stream;
	ArvBuffer *buffer;
	GOptionContext *context;
	GError *error = NULL;
	char memory_buffer[100000];
	int i;

	arv_g_thread_init (NULL);
	arv_g_type_init ();

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, arv_option_entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	arv_debug_enable (arv_option_debug_domains);

	if (arv_option_camera_name == NULL)
		g_print ("Looking for the first available camera\n");
	else
		g_print ("Looking for camera '%s'\n", arv_option_camera_name);

	device = arv_open_device (arv_option_camera_name);
	if (device != NULL) {
		ArvGc *genicam;
		ArvGcNode *node;
		guint32 value;
		guint32 maximum;
		guint32 minimum;
		guint64 n_processed_buffers;
		guint64 n_failures;
		guint64 n_underruns;
		double v_double;
		double v_double_min;
		double v_double_max;
		const char *v_string;
		gboolean v_boolean;

		genicam = arv_device_get_genicam (device);

		if (arv_option_width > 0) {
			node = arv_gc_get_node (genicam, "Width");
			arv_gc_integer_set_value (ARV_GC_INTEGER (node), arv_option_width, NULL);
		}
		if (arv_option_height > 0) {
			node = arv_gc_get_node (genicam, "Height");
			arv_gc_integer_set_value (ARV_GC_INTEGER (node), arv_option_height, NULL);
		}
		if (arv_option_horizontal_binning > 0) {
			node = arv_gc_get_node (genicam, "BinningHorizontal");
			arv_gc_integer_set_value (ARV_GC_INTEGER (node), arv_option_horizontal_binning, NULL);
		}
		if (arv_option_vertical_binning > 0) {
			node = arv_gc_get_node (genicam, "BinningVertical");
			arv_gc_integer_set_value (ARV_GC_INTEGER (node), arv_option_vertical_binning, NULL);
		}

		node = arv_gc_get_node (genicam, "DeviceVendorName");
		v_string = arv_gc_string_get_value (ARV_GC_STRING (node), NULL);
		g_print ("vendor        = %s\n", v_string);
		node = arv_gc_get_node (genicam, "DeviceModelName");
		v_string = arv_gc_string_get_value (ARV_GC_STRING (node), NULL);
		g_print ("model         = %s\n", v_string);
		node = arv_gc_get_node (genicam, "DeviceID");
		v_string = arv_gc_string_get_value (ARV_GC_STRING (node), NULL);
		g_print ("device id     = %s\n", v_string);
		node = arv_gc_get_node (genicam, "SensorWidth");
		value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), NULL);
		g_print ("sensor width  = %d\n", value);
		node = arv_gc_get_node (genicam, "SensorHeight");
		value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), NULL);
		g_print ("sensor height = %d\n", value);
		node = arv_gc_get_node (genicam, "Width");
		value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), NULL);
		maximum = arv_gc_integer_get_max (ARV_GC_INTEGER (node), NULL);
		g_print ("image width   = %d (max:%d)\n", value, maximum);
		node = arv_gc_get_node (genicam, "Height");
		value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), NULL);
		maximum = arv_gc_integer_get_max (ARV_GC_INTEGER (node), NULL);
		g_print ("image height  = %d (max:%d)\n", value, maximum);
		node = arv_gc_get_node (genicam, "BinningHorizontal");
		value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), NULL);
		maximum = arv_gc_integer_get_max (ARV_GC_INTEGER (node), NULL);
		minimum = arv_gc_integer_get_min (ARV_GC_INTEGER (node), NULL);
		g_print ("horizontal binning  = %d (min:%d - max:%d)\n", value, minimum, maximum);
		node = arv_gc_get_node (genicam, "BinningVertical");
		value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), NULL);
		maximum = arv_gc_integer_get_max (ARV_GC_INTEGER (node), NULL);
		minimum = arv_gc_integer_get_min (ARV_GC_INTEGER (node), NULL);
		g_print ("vertical binning    = %d (min:%d - max:%d)\n", value, minimum, maximum);
		node = arv_gc_get_node (genicam, "ExposureTimeAbs");
		v_double = arv_gc_float_get_value (ARV_GC_FLOAT (node), NULL);
		v_double_min = arv_gc_float_get_min (ARV_GC_FLOAT (node), NULL);
		v_double_max = arv_gc_float_get_max (ARV_GC_FLOAT (node), NULL);
		g_print ("exposure            = %g (min:%g - max:%g)\n", v_double, v_double_min, v_double_max);
		node = arv_gc_get_node (genicam, "ExposureAuto");
		v_string = arv_gc_enumeration_get_string_value (ARV_GC_ENUMERATION (node), NULL);
		g_print ("exposure auto mode  = %s\n", v_string);
		node = arv_gc_get_node (genicam, "GainRaw");
		value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), NULL);
		maximum = arv_gc_integer_get_max (ARV_GC_INTEGER (node), NULL);
		minimum = arv_gc_integer_get_min (ARV_GC_INTEGER (node), NULL);
		g_print ("gain                = %d (min:%d - max:%d)\n", value, minimum, maximum);
		node = arv_gc_get_node (genicam, "GainAuto");
		v_string = arv_gc_enumeration_get_string_value (ARV_GC_ENUMERATION (node), NULL);
		g_print ("gain auto mode      = %s\n", v_string);
		node = arv_gc_get_node (genicam, "TriggerSelector");
		v_string = arv_gc_enumeration_get_string_value (ARV_GC_ENUMERATION (node), NULL);
		g_print ("trigger selector    = %s\n", v_string);
		node = arv_gc_get_node (genicam, "ReverseX");
		if (node != NULL) {
			v_boolean = arv_gc_boolean_get_value (ARV_GC_BOOLEAN (node), NULL);
			g_print ("reverse x          = %s\n", v_boolean ? "TRUE" : "FALSE");
		}

		stream = arv_device_create_stream (device, NULL, NULL);
		if (arv_option_auto_buffer)
			g_object_set (stream,
				      "socket-buffer", ARV_GV_STREAM_SOCKET_BUFFER_AUTO,
				      "socket-buffer-size", 0,
				      NULL);

		node = arv_gc_get_node (genicam, "PayloadSize");
		value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), NULL);
		g_print ("payload size  = %d (0x%x)\n", value, value);

		for (i = 0; i < 30; i++)
			arv_stream_push_buffer (stream, arv_buffer_new (value, NULL));

		arv_device_read_register (device, ARV_GVBS_STREAM_CHANNEL_0_PORT_OFFSET, &value, NULL);
		g_print ("stream port = %d (%d)\n", value, arv_gv_stream_get_port (ARV_GV_STREAM (stream)));

		arv_device_read_memory (device, 0x00014150, 8, memory_buffer, NULL);
		arv_device_read_memory (device, 0x000000e8, 16, memory_buffer, NULL);
		arv_device_read_memory (device,
					ARV_GVBS_USER_DEFINED_NAME_OFFSET,
					ARV_GVBS_USER_DEFINED_NAME_SIZE, memory_buffer, NULL);

		node = arv_gc_get_node (genicam, "AcquisitionStart");
		arv_gc_command_execute (ARV_GC_COMMAND (node), NULL);

		signal (SIGINT, set_cancel);

		do {
			g_usleep (100000);

			do  {
				buffer = arv_stream_try_pop_buffer (stream);
				if (buffer != NULL)
					arv_stream_push_buffer (stream, buffer);
			} while (buffer != NULL);
		} while (!cancel);

		arv_device_read_register (device, ARV_GVBS_STREAM_CHANNEL_0_PORT_OFFSET, &value, NULL);
		g_print ("stream port = %d (%d)\n", value, arv_gv_stream_get_port (ARV_GV_STREAM (stream)));

		arv_stream_get_statistics (stream, &n_processed_buffers, &n_failures, &n_underruns);

		g_print ("Processed buffers = %Lu\n", (unsigned long long) n_processed_buffers);
		g_print ("Failures          = %Lu\n", (unsigned long long) n_failures);
		g_print ("Underruns         = %Lu\n", (unsigned long long) n_underruns);

		node = arv_gc_get_node (genicam, "AcquisitionStop");
		arv_gc_command_execute (ARV_GC_COMMAND (node), NULL);

		g_object_unref (stream);
		g_object_unref (device);
	} else
		g_print ("No device found\n");

	return 0;
}
