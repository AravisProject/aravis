#include <arv.h>
#include <stdlib.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
	ArvCamera *camera;

	camera = arv_camera_new (argc > 1 ? argv[1] : NULL);
	if (!ARV_IS_CAMERA (camera)) {
		printf ("Camera not found\n");
		return EXIT_FAILURE;
	}

	if (arv_camera_is_gv_device (camera)) {
		unsigned packet_size;

		packet_size = arv_camera_gv_auto_packet_size (camera, NULL);
		printf ("Packet size set to %d bytes on camera %s-%s\n", packet_size,
			arv_camera_get_vendor_name (camera, NULL), arv_camera_get_device_id (camera, NULL));
	} else {
		printf ("%s-%s is not a GigEVision camera\n",
			arv_camera_get_vendor_name (camera, NULL), arv_camera_get_device_id (camera, NULL));
	}

	g_clear_object (&camera);

	return EXIT_SUCCESS;
}
