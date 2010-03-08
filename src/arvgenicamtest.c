#include <arv.h>
#include <arvdebug.h>
#include <arvgc.h>
#include <stdlib.h>

static char **arv_option_filenames = NULL;
static int arv_option_debug_level;

static const GOptionEntry arv_option_entries[] =
{
	{ G_OPTION_REMAINING,	' ', 0, G_OPTION_ARG_FILENAME_ARRAY,
		&arv_option_filenames,		NULL, NULL},
	{ "debug", 		'd', 0, G_OPTION_ARG_INT,
		&arv_option_debug_level, 	"Debug mode", NULL },
	{ NULL }
};

int
main (int argc, char **argv)
{
	ArvGc *genicam;
	char *xml;
	size_t size;
	GOptionContext *context;
	GError *error = NULL;
	int i;

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

	if (arv_option_filenames == NULL) {
		g_print ("Missing input filename.\n");
		return EXIT_FAILURE;
	}

	for (i = 0; arv_option_filenames[i] != NULL; i++) {
		g_file_get_contents (arv_option_filenames[i], &xml, &size, NULL);

		if (xml != NULL) {
			g_print ("Loading '%s'.\n", arv_option_filenames[i]);

			genicam = arv_gc_new (xml, size);

			g_free (xml);

			g_object_unref (genicam);
		} else
			g_print ("File '%s' not found.\n", arv_option_filenames[i]);
	}

	return EXIT_SUCCESS;
}
