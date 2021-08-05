#include <glib.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>
#include <arv.h>

#include <arvmiscprivate.h>

int
main (int argc, char **argv)
{
	GFile *file;
	GFileInputStream *stream;
	g_autofree char *scheme = NULL;
	g_autofree char *path = NULL;
	g_autofree char *genicam = NULL;
	const char *filename;
	gsize len = 0;

	if (argc != 2) {
		printf ("Usage: load-http-test <URL>\n");
		return EXIT_FAILURE;
	}

	filename = argv[1];

	if (arv_parse_genicam_url (filename, -1, &scheme, NULL, &path, NULL, NULL, NULL, NULL)) {
		if (g_ascii_strcasecmp (scheme, "http:") == 0) {
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

	g_print ("size = %" G_GSIZE_FORMAT "\n", len);
	g_print ("%s\n", genicam != NULL ? genicam : "NULL");

	return EXIT_SUCCESS;
}
