#include <arv.h>
#include <stdlib.h>
#include <stdio.h>

static char *arv_option_device_name = NULL;
static gboolean arv_option_show_xml = FALSE;
static char *arv_option_debug_domains = NULL;

static const GOptionEntry arv_option_entries[] =
{
	{ "name",		'n', 0, G_OPTION_ARG_STRING,
		&arv_option_device_name,	NULL, "device_name"},
	{ "xml",		'x', 0, G_OPTION_ARG_NONE,
		&arv_option_show_xml,		"Show Genicam data", NULL},
	{ "debug", 		'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	NULL, "category[:level][,...]" },
	{ NULL }
};

int
main (int argc, char **argv)
{
	unsigned int n_devices;
	GOptionContext *context;
	GError *error = NULL;

	g_thread_init (NULL);
	g_type_init ();

	context = g_option_context_new (NULL);
	g_option_context_set_summary (context, "Utility that gives the list of the connected devices.");
	g_option_context_add_main_entries (context, arv_option_entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	arv_debug_enable (arv_option_debug_domains);

	arv_update_device_list ();
	n_devices = arv_get_n_devices ();
	if (n_devices < 1)
		printf ("No device found\n");
	else {
		unsigned int i;

		for (i = 0; i < n_devices; i++) {
			const char *device_id;

			device_id = arv_get_device_id (i);
			if (arv_option_device_name == NULL ||
			    g_strcmp0 (device_id, arv_option_device_name) == 0) {
				if (device_id != NULL)
					printf ("%s\n",  device_id);

				if (arv_option_show_xml) {
					ArvDevice *device;

					device = arv_open_device (device_id);
					if (ARV_IS_DEVICE (device)) {
						const char *xml;
						size_t size;

						xml = arv_device_get_genicam_xml (device, &size);
						if (xml != NULL)
							printf ("%*s\n", (int) size, xml);
					}

					g_object_unref (device);
				}
			}
		}
	}

	/* For debug purpose only */
	arv_shutdown ();

	return EXIT_SUCCESS;
}
