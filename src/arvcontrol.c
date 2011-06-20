#include <arv.h>
#include <stdlib.h>
#include <stdio.h>

static char *arv_option_device_name = NULL;
static gboolean arv_option_list = FALSE;
static gboolean arv_option_show_description = FALSE;
static char *arv_option_debug_domains = NULL;

static const GOptionEntry arv_option_entries[] =
{
	{ "name",		'n', 0, G_OPTION_ARG_STRING,
		&arv_option_device_name,	NULL, "device_name"},
	{ "list",		'l', 0, G_OPTION_ARG_NONE,
		&arv_option_list,		"List available features", NULL},
	{ "description",	'i', 0, G_OPTION_ARG_NONE,
		&arv_option_show_description,	"Show feature description", NULL},
	{ "debug", 		'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	NULL, "category[:level][,...]" },
	{ NULL }
};

static void
arv_control_list_features (ArvGc *genicam, const char *feature, gboolean show_description, int level)
{
	ArvGcNode *node;

	node = arv_gc_get_node (genicam, feature);
	if (ARV_IS_GC_NODE (node)) {
		int i;

		for (i = 0; i < level; i++)
			printf ("    ");

		printf ("%s: '%s'\n", arv_gc_node_get_node_name (node), feature);

		if (show_description) {
			const char *description;

			description = arv_gc_node_get_description (node);
			if (description)
				printf ("%s\n", description);
		}

		if (ARV_IS_GC_CATEGORY (node)) {
			const GSList *features;
			const GSList *iter;

			features = arv_gc_category_get_features (ARV_GC_CATEGORY (node));

			for (iter = features; iter != NULL; iter = iter->next)
				arv_control_list_features (genicam, iter->data, show_description, level + 1);
		} else if (ARV_IS_GC_ENUMERATION (node)) {
			const GSList *childs;
			const GSList *iter;

			childs = arv_gc_node_get_childs (node);
			for (iter = childs; iter != NULL; iter = iter->next) {
				for (i = 0; i < level + 1; i++)
					printf ("    ");

				printf ("%s: '%s'\n",
					arv_gc_node_get_node_name (iter->data),
					arv_gc_node_get_name (iter->data));
			}
		}
	}
}

int
main (int argc, char **argv)
{
	ArvDevice *device;
	GOptionContext *context;
	GError *error = NULL;
	int i;

	g_thread_init (NULL);
	g_type_init ();

	context = g_option_context_new ("feature[=value] ...");
	g_option_context_set_summary (context, "Small utility for read/write of Genicam device features.");
	g_option_context_set_description (context,
					  "For example the setting of the Width and Height features, followed by"
					  " the read of the Gain, is done with this command: 'arv-control-"
					  ARAVIS_API_VERSION " Width=128 Height=128 Gain'.");
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

	if (arv_option_list) {
		ArvGc *genicam;

		genicam = arv_device_get_genicam (device);
		arv_control_list_features (genicam, "Root", arv_option_show_description, 0);
	} else if (argc < 2) {
	} else
		for (i = 1; i < argc; i++) {
			ArvGcNode *feature;
			char **tokens;

			tokens = g_strsplit (argv[i], "=", 2);
			feature = arv_device_get_feature (device, tokens[0]);
			if (!ARV_IS_GC_NODE (feature))
				printf ("Feature '%s' not found\n", tokens[0]);
			else {
				if (ARV_IS_GC_COMMAND (feature)) {
					arv_gc_command_execute (ARV_GC_COMMAND (feature));
					printf ("%s executed\n", tokens[0]);
				} else {
					if (tokens[1] != NULL)
						arv_gc_node_set_value_from_string (feature, tokens[1]);

					printf ("%s = %s\n", tokens[0], arv_gc_node_get_value_as_string (feature));
				}
			}
			g_strfreev (tokens);
		}

	/* For debug purpose only */
	arv_shutdown ();

	return EXIT_SUCCESS;
}
