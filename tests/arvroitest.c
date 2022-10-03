#include <arv.h>
#include <arvstr.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

#define N_BUFFERS	20

#define WIDTH_MIN	100
#define HEIGHT_MIN	100
#define WIDTH_MAX	600
#define HEIGHT_MAX	600
#define SIZE_INC	20

typedef struct {
	ArvCamera *camera;
	ArvStream *stream;
	GMainLoop *main_loop;
	int buffer_count;
	int width, height;
} ApplicationData;

static gboolean cancel = FALSE;

static void
set_cancel (int signal)
{
	cancel = TRUE;
}

static void
new_buffer_cb (ArvStream *stream, ApplicationData *data)
{
	ArvBuffer *buffer;

	buffer = arv_stream_try_pop_buffer (stream);
	if (buffer != NULL) {

		if (arv_buffer_get_status (buffer) == ARV_BUFFER_STATUS_SUCCESS)
			data->buffer_count++;

		if (arv_buffer_get_image_width (buffer) != data->width ||
		    arv_buffer_get_image_height (buffer) != data->height)
			printf ("Size error! %dx%d (expected: %dx%d)\n",
				arv_buffer_get_image_width (buffer),
				arv_buffer_get_image_height (buffer),
				data->width,
				data->height);

		/* Image processing here */

		arv_stream_push_buffer (stream, buffer);
	}
}

static gboolean
framerate_task_cb (void *abstract_data)
{
	ApplicationData *data = abstract_data;

	printf ("Frame rate = %g Hz\n", data->buffer_count / 10.0);
	data->buffer_count = 0;

	return TRUE;
}

static gboolean
check_cancel_task_cb (void *abstract_data)
{
	ApplicationData *data = abstract_data;

	if (cancel) {
		g_main_loop_quit (data->main_loop);
		return FALSE;
	}

	return TRUE;
}

static gboolean
switch_roi (gpointer user_data)
{
	ApplicationData *data = user_data;
	gint width;
	gint height;
        guint n_deleted;

	arv_camera_stop_acquisition (data->camera, NULL);

        n_deleted = arv_stream_stop_thread (data->stream, FALSE);

        g_assert (n_deleted == 0);

	data->width += SIZE_INC;
	if (data->width > WIDTH_MAX)
		data->width = WIDTH_MIN;
	data->height += SIZE_INC;
	if (data->height > HEIGHT_MAX)
		data->height = HEIGHT_MIN;

	arv_camera_set_region (data->camera, 0, 0, data->width, data->height, NULL);
	arv_camera_get_region (data->camera, NULL, NULL, &width, &height, NULL);

	g_assert (width == data->width);
	g_assert (height == data->height);

	printf ("image size set to %dx%d\n", width, height);

        arv_stream_start_thread (data->stream);

	arv_camera_start_acquisition (data->camera, NULL);

	return TRUE;
}

int
main (int argc, char **argv)
{
	ApplicationData data;
	GError *error = NULL;
	const char *camera_name = NULL;
	int i;

	data.buffer_count = 0;
	data.width = WIDTH_MIN;
	data.height = HEIGHT_MIN;

	if (argc > 2) {
		printf ("Usage: arv-roi-test <camera-name>\n");

		return EXIT_FAILURE;
	}

	if (argc == 1) {
		g_print ("Looking for the first available camera\n");
	} else {
		camera_name = argv[1];
		g_print ("Looking for camera '%s'\n", camera_name);
	}

	data.camera = arv_camera_new (camera_name, &error);

	if (ARV_IS_CAMERA (data.camera)) {
		void (*old_sigint_handler)(int);
		gint max_payload;
		gint x, y, width, height;
		guint64 n_completed_buffers;
		guint64 n_failures;
		guint64 n_underruns;

		arv_camera_set_region (data.camera, 0, 0, WIDTH_MAX, HEIGHT_MAX, NULL);

		max_payload = arv_camera_get_payload (data.camera, NULL);

		arv_camera_set_frame_rate (data.camera, 20.0, NULL);
		arv_camera_get_region (data.camera, &x, &y, &width, &height, NULL);
		arv_camera_set_region (data.camera, 0, 0, data.width, data.height, NULL);

		printf ("vendor name           = %s\n", arv_camera_get_vendor_name (data.camera, NULL));
		printf ("model name            = %s\n", arv_camera_get_model_name (data.camera, NULL));
		printf ("device id             = %s\n", arv_camera_get_device_id (data.camera, NULL));
		printf ("image x               = %d\n", x);
		printf ("image y               = %d\n", y);
		printf ("image width           = %d\n", width);
		printf ("image height          = %d\n", height);

		data.stream = arv_camera_create_stream (data.camera, NULL, NULL, &error);

		if (ARV_IS_STREAM (data.stream)) {
			g_signal_connect (data.stream, "new-buffer", G_CALLBACK (new_buffer_cb), &data);
			arv_stream_set_emit_signals (data.stream, TRUE);

			for (i = 0; i < N_BUFFERS; i++)
				arv_stream_push_buffer (data.stream, arv_buffer_new (max_payload, NULL));

			arv_camera_set_acquisition_mode (data.camera, ARV_ACQUISITION_MODE_CONTINUOUS, NULL);
			arv_camera_start_acquisition (data.camera, NULL);

			data.main_loop = g_main_loop_new (NULL, FALSE);

			old_sigint_handler = signal (SIGINT, set_cancel);

			g_timeout_add_seconds (10, framerate_task_cb, &data);
			g_timeout_add_seconds (1, check_cancel_task_cb, &data);
			g_timeout_add (697, switch_roi, &data);

			g_main_loop_run (data.main_loop);

			signal (SIGINT, old_sigint_handler);

			g_main_loop_unref (data.main_loop);

			arv_stream_get_statistics (data.stream, &n_completed_buffers, &n_failures, &n_underruns);

			g_print ("Completed buffers = %" G_GUINT64_FORMAT "\n", n_completed_buffers);
			g_print ("Failures          = %" G_GUINT64_FORMAT "\n", n_failures);
			g_print ("Underruns         = %" G_GUINT64_FORMAT "\n", n_underruns);

			arv_camera_stop_acquisition (data.camera, NULL);

			arv_stream_set_emit_signals (data.stream, FALSE);

			g_object_unref (data.stream);
		} else {
			printf ("Can't create stream thread%s%s\n",
				error != NULL ? ": " : "",
				error != NULL ? error->message : "");

			g_clear_error (&error);
		}

		g_object_unref (data.camera);
	} else {
		printf ("No camera found%s%s\n",
			error != NULL ? ": " : "",
			error != NULL ? error->message : "");
		g_clear_error (&error);
	}

	return 0;
}
