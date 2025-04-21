#include <arv.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
	unsigned n_cameras;
        ArvCamera **cameras;
        GError *error = NULL;
        int i;

	printf("Aravis %u.%u.%u\n", arv_get_major_version(), arv_get_minor_version(), arv_get_micro_version());

        arv_select_interface ("USB3Vision");

	arv_update_device_list ();
	n_cameras = arv_get_n_devices();

        if (n_cameras < 1) {
                printf("No camera found\n");
                return EXIT_FAILURE;
        }

        cameras = g_new0 (ArvCamera *, n_cameras);

	for (i = 0; i < n_cameras; i++) {
                printf ("id of #%u: %s\n", i, arv_get_device_id(i));
                printf ("vendor of #%u: %s\n", i, arv_get_device_vendor(i));
                printf ("model of #%u: %s\n", i, arv_get_device_model(i));
                cameras[i] = arv_camera_new (arv_get_device_id(i), &error);
                if (error != NULL)
                        goto new_error;
	}

	for (i = 0; i < n_cameras; i++) {
                ArvBuffer *buffer;

                buffer = arv_camera_acquisition(cameras[i], 0, &error);
                if (ARV_IS_BUFFER (buffer) && arv_buffer_get_status(buffer) == ARV_BUFFER_STATUS_SUCCESS)
                        printf ("Image successfully acquired for camera #%d\n", i);
                else
                        printf ("Failed to acquire image for camera #%d\n", i);
                g_clear_object(&buffer);

                if (error != NULL)
                        goto new_error;
        }

new_error:
	for (i = 0; i < n_cameras; i++) {
                g_clear_object (&cameras[i]);
        }

        g_free (cameras);

        if (error != NULL) {
                printf ("Test failed: %s\n", error->message);
                g_clear_error (&error);

                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}
