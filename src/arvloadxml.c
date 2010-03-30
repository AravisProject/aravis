#include <arv.h>
#include <stdlib.h>

static char *arv_option_camera_name = NULL;
static int arv_option_debug_level;

static const GOptionEntry arv_option_entries[] =
{
	{ "name",		'n', 0, G_OPTION_ARG_STRING,
		&arv_option_camera_name,"Camera name", NULL},
	{ "debug", 		'd', 0, G_OPTION_ARG_INT,
		&arv_option_debug_level, 	"Debug mode", NULL },
	{ NULL }
};

int
main (int argc, char **argv)
{
	ArvDevice *device;
	const char *genicam_data;
	size_t genicam_size;
	GOptionContext *context;
	GError *error = NULL;

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

	device = arv_new_device (arv_option_camera_name);
	if (device != NULL) {
		genicam_data = arv_device_get_genicam_data (device, &genicam_size);
		g_file_set_contents ("genicam.xml", genicam_data, genicam_size, NULL);
	} else
		g_message ("No device found");

	return 0;
}

