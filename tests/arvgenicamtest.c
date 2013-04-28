#include <arv.h>
#include <stdlib.h>

static char **arv_option_filenames = NULL;
static char *arv_option_debug_domains = NULL;

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
	ArvGc *genicam;
	char *xml;
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
		g_file_get_contents (arv_option_filenames[i], &xml, &size, NULL);

		if (xml != NULL) {
			ArvGcNode *node;

			g_print ("Loading '%s'.\n", arv_option_filenames[i]);

			genicam = arv_gc_new (NULL, xml, size);

			node = arv_gc_get_node (genicam, "RegAcquisitionCommand");
			if (node != NULL) {
				g_print ("RegAcquisitionCommand address = 0x%Lx - length = 0x%Lx\n",
					 (unsigned long long) arv_gc_register_get_address (ARV_GC_REGISTER (node), NULL),
					 (unsigned long long) arv_gc_register_get_length (ARV_GC_REGISTER (node), NULL));
			}

			node = arv_gc_get_node (genicam, "IntWidthIncrement");
			if (node != NULL) {
				g_print ("IntWidthIncrement value = %Ld\n",
					 (long long) arv_gc_integer_get_value (ARV_GC_INTEGER (node), NULL));
			}

			node = arv_gc_get_node (genicam, "WhitebalValueMin");
			if (node != NULL) {
				g_print ("WhitebalValueMin value = %g\n",
					 arv_gc_float_get_value (ARV_GC_FLOAT (node), NULL));
			}

			g_free (xml);

			g_object_unref (genicam);
		} else
			g_print ("File '%s' not found.\n", arv_option_filenames[i]);
	}

	return EXIT_SUCCESS;
}
