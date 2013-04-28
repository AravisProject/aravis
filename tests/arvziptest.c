#include <arv.h>
#include <stdlib.h>

static char **arv_option_filenames = NULL;
static char *arv_option_debug_domains;

static const GOptionEntry arv_option_entries[] =
{
	{ G_OPTION_REMAINING,	' ', 0, G_OPTION_ARG_FILENAME_ARRAY,
		&arv_option_filenames,		NULL, NULL},
	{ "debug", 		'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	"Debug mode", NULL },
	{ NULL }
};

int
main (int argc, char **argv)
{
	ArvZip *zip;
	char *buffer;
	size_t size;
	GOptionContext *context;
	GError *error = NULL;
	int i;

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

	if (arv_option_filenames == NULL) {
		g_print ("Missing input filename.\n");
		return EXIT_FAILURE;
	}

	for (i = 0; arv_option_filenames[i] != NULL; i++) {
		g_file_get_contents (arv_option_filenames[i], &buffer, &size, NULL);

		if (buffer != NULL) {
			zip = arv_zip_new (buffer, size);
			arv_zip_free (zip);
		}
	}

	return EXIT_SUCCESS;
}

