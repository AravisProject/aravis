#include <arv.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
	ArvCamera *camera;
	ArvStream *stream;
	ArvChunkParser *parser;
	GError *error =NULL;

	/* Instantiation of the first available camera */
	camera = arv_camera_new (NULL, &error);

	if (ARV_IS_CAMERA (camera)) {
		gint payload;

		/* Instantiation of a chunk parser */
		parser = arv_camera_create_chunk_parser (camera);

		/* Enable chunk data */
		arv_camera_set_chunks (camera, "Width,Height", NULL);

		/* retrieve image payload (number of bytes per image) */
		payload = arv_camera_get_payload (camera, NULL);

		/* Create a new stream object */
		stream = arv_camera_create_stream (camera, NULL, NULL, &error);

		if (ARV_IS_STREAM (stream)) {
			ArvBuffer *buffer;

			/* Push 1 buffer in the stream input buffer queue */
			arv_stream_push_buffer (stream, arv_buffer_new (payload, NULL));

			/* Start the video stream */
			arv_camera_start_acquisition (camera, NULL);

			/* Retrieve the acquired buffer */
			buffer = arv_stream_pop_buffer (stream);

			printf ("ChunkWidth = %d\n", (int) arv_chunk_parser_get_integer_value (parser, buffer, "ChunkWidth", NULL));
			printf ("ChunkHeight = %d\n", (int) arv_chunk_parser_get_integer_value (parser, buffer, "ChunkHeight", NULL));

			g_object_unref (buffer);

			/* Stop the video stream */
			arv_camera_stop_acquisition (camera, NULL);

			g_object_unref (stream);
		} else {
			printf ("Can't create stream thread%s%s\n",
				error != NULL ? ": " : "",
				error != NULL ? error->message: "");
			g_clear_error (&error);
		}

		g_object_unref (parser);
		g_object_unref (camera);
	} else {
		printf ("No camera found%s%s\n",
			error != NULL ? ": " : "",
			error != NULL ? error->message: "");
		g_clear_error (&error);
	}

	return 0;
}

