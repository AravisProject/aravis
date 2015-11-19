#include <glib.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>
#include <arv.h>

int
main (int argc, char **argv)
{
	GFile *file;
	GFileInputStream *stream;
	const char *filename;
	char *genicam = NULL;
	char **tokens;
	gsize len = 0;

	if (argc != 2) {
		printf ("Usage: load-http-test <URL>\n");
		return EXIT_FAILURE;
	}

	filename = argv[1];

	tokens = g_regex_split (arv_gv_device_get_url_regex (), filename, 0);

	if (tokens[0] != NULL && tokens[1] != NULL) {
		if (g_ascii_strcasecmp (tokens[1], "http:") == 0) {
			file = g_file_new_for_uri (filename);
			stream = g_file_read (file, NULL, NULL);
			if(stream) {
				GDataInputStream *data_stream;

				data_stream = g_data_input_stream_new (G_INPUT_STREAM (stream));
				genicam = g_data_input_stream_read_upto (data_stream, "", 0, &len, NULL, NULL);

				g_object_unref (data_stream);
				g_object_unref (stream);
			}
			g_object_unref (file);
		}
	}

	g_strfreev (tokens);

	printf ("size = %lu\n", len);
	printf ("%s\n", genicam != NULL ? genicam : "NULL");

	g_free (genicam);

	return EXIT_SUCCESS;
}
