#include <arv.h>
#include <stdlib.h>
#include <stdio.h>

static char *arv_option_device_name = NULL;
static char *arv_option_debug_domains = NULL;

static const GOptionEntry arv_option_entries[] =
{
	{ "name",		'n', 0, G_OPTION_ARG_STRING,
		&arv_option_device_name,"Camera name", NULL},
	{ "debug", 		'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	"Debug domains", NULL },
	{ NULL }
};

int
main (int argc, char **argv)
{
	ArvDevice *device;
	GOptionContext *context;
	GError *error = NULL;
	int i;

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

	arv_debug_enable (arv_option_debug_domains);

	device = arv_open_device (arv_option_device_name);
	if (!ARV_IS_DEVICE (device)) {
		printf ("Device '%s' not found\n", arv_option_device_name);
		return EXIT_FAILURE;
	}

	if (argc < 2) {
	} else
		for (i = 1; i < argc; i++) {
			ArvGcNode *feature;
			char **tokens;

			tokens = g_strsplit (argv[i], "=", 2);
			feature = arv_device_get_feature (device, tokens[0]);
			if (!ARV_IS_GC_NODE (feature))
				printf ("Feature '%s' not found\n", tokens[0]);
			else {
				if (tokens[1] != NULL)
					arv_gc_node_set_value_from_string (feature, tokens[1]);

				printf ("%s = %s\n", tokens[0], arv_gc_node_get_value_as_string (feature));
			}
			g_strfreev (tokens);
		}

	/* For debug purpose only */
	arv_shutdown ();

	return EXIT_SUCCESS;
}
