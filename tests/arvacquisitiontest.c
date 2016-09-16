#include <arv.h>
#include <stdlib.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
	ArvCamera *camera;
	ArvBuffer *buffer;

	camera = arv_camera_new (argc > 1 ? argv[1] : NULL);
	buffer = arv_camera_acquisition (camera, 0);

	if (ARV_IS_BUFFER (buffer))
		printf ("Image successfully acquired\n");
	else
		printf ("Failed to acquire a single image\n");

	g_clear_object (&camera);
	g_clear_object (&buffer);

	return EXIT_SUCCESS;
}
