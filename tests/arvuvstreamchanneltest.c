#include <arv.h>
#include <stdio.h>
#include <stdlib.h>

#define N_BUFFERS	4
#define TIMEOUT_US	2000000

int
main (int argc, char **argv)
{
	ArvCamera *camera = NULL;
	ArvDevice *device;
	ArvStream **streams = NULL;
	GError *error = NULL;
	unsigned n_stream_channels = 0;
	unsigned payload;
	unsigned i;
	int status = EXIT_SUCCESS;

	arv_select_interface ("USB3Vision");

	camera = arv_camera_new (argc > 1 ? argv[1] : NULL, &error);
	if (error != NULL)
		goto camera_error;

	device = arv_camera_get_device (camera);
	payload = arv_camera_get_payload (camera, &error);
	if (error != NULL)
		goto camera_error;

	n_stream_channels = arv_device_get_integer_feature_value (device, "ArvUvStreamChannelCount", &error);
	if (error != NULL)
		goto camera_error;

	printf ("Number of stream channels: %u\n", n_stream_channels);

	streams = g_new0 (ArvStream *, n_stream_channels);

	for (i = 0; i < n_stream_channels; i++) {
		unsigned j;

		arv_device_set_integer_feature_value (device, "ArvUvStreamChannelSelector", i, &error);
		if (error != NULL)
			goto camera_error;

		streams[i] = arv_device_create_stream (device, NULL, NULL, NULL, &error);
		if (error != NULL)
			goto camera_error;

		for (j = 0; j < N_BUFFERS; j++)
			arv_stream_push_buffer (streams[i], arv_buffer_new (payload, NULL));
	}

	arv_camera_start_acquisition (camera, &error);
	if (error != NULL)
		goto camera_error;

	for (i = 0; i < n_stream_channels; i++) {
		ArvBuffer *buffer;

		buffer = arv_stream_timeout_pop_buffer (streams[i], TIMEOUT_US);
		if (ARV_IS_BUFFER (buffer) && arv_buffer_get_status (buffer) == ARV_BUFFER_STATUS_SUCCESS) {
			size_t size;

			arv_buffer_get_data (buffer, &size);
			printf ("Stream channel %u: image successfully acquired (%" G_GSIZE_FORMAT " bytes)\n",
				i, size);
		} else {
			printf ("Stream channel %u: failed to acquire image\n", i);
			status = EXIT_FAILURE;
		}
		g_clear_object (&buffer);
	}

	arv_camera_stop_acquisition (camera, &error);

camera_error:

	if (streams != NULL) {
		for (i = 0; i < n_stream_channels; i++)
			g_clear_object (&streams[i]);
		g_free (streams);
	}
	g_clear_object (&camera);

	if (error != NULL) {
		printf ("Test failed: %s\n", error->message);
		g_clear_error (&error);

		return EXIT_FAILURE;
	}

	return status;
}
