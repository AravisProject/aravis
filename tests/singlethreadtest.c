#include <arv.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFERS_COUNT	4

int main(int argc, char** argv) {
	arv_g_type_init();
	
	if(argc != 2) {
		printf("Usage\n");
		printf("=====\n\n");
		printf("%s 0: Multi-threading mode\n", argv[0]);
		printf("%s 1: Single-threading mode\n", argv[0]);
		return 1;
	}

	int singlethread = atoi(argv[1]);
	
	// Open camera
	printf("Open\n");
	ArvCamera* camera = arv_camera_new(arv_get_device_id(0));
	if(camera == NULL) {
		printf("No cameras found!\n");
		return 1;
	}

	// Set some basic settings
	printf("Configure\n");
	arv_camera_set_pixel_format(camera, ARV_PIXEL_FORMAT_MONO_8);
	arv_camera_set_acquisition_mode(camera, ARV_ACQUISITION_MODE_CONTINUOUS);


	// Set to singlethreaded mode
	if(singlethread) {
		printf("Setting single thread mode\n");
		arv_camera_uv_set_stream_options(camera, ARV_UV_STREAM_OPTION_THREADING_DISABLED);
		arv_camera_gv_set_stream_options(camera, ARV_GV_STREAM_OPTION_THREADING_DISABLED);
	}
	
	// Create stream
	printf("Create stream\n");
	ArvStream* stream = arv_camera_create_stream(camera, NULL, NULL);
	if(stream == NULL) {
		printf("Error creating camera stream\n");
		return 1;
	}

	// Create buffers
	printf("Create buffers\n");
	int payloadSize = arv_camera_get_payload (camera);
	for(int i=0; i < BUFFERS_COUNT; i++) {
		arv_stream_push_buffer(stream, arv_buffer_new (payloadSize, NULL));
	}

	// Start image acquisition
	printf("Starting acquisition\n");
	arv_camera_start_acquisition(camera);

	printf("Capturing\n");
	for(;;) {
		ArvBuffer* buffer = NULL;
		
		if(singlethread) {
			// Manually schedule the "thread" of our patched aravis version
			arv_stream_schedule_thread(stream);
		}

		buffer = arv_stream_try_pop_buffer(stream);
		
		if(buffer == NULL) {
			continue;
		}

		printf("Captured %d x %d pixels!\n", arv_buffer_get_image_width(buffer),
			arv_buffer_get_image_height(buffer));

		arv_stream_push_buffer(stream, buffer);
	}
	

	// Clean up
	printf("Clean up\n");
	g_object_unref(stream);
	arv_camera_stop_acquisition(camera);
	g_object_unref(camera);

	return 0;
}
