#include <arv.h>
#include <stdio.h>

gboolean cancel = FALSE;

static void set_cancel(int signal)
{
	cancel = TRUE;
}

static gboolean
periodic_task_cb (void *user_data)
{
	GMainLoop *main_loop = user_data;
	guint n_devices;
	int i;

	if (cancel) {
		g_main_loop_quit(main_loop);

		return FALSE;
	}

	printf ("Update Device List\n");

	arv_update_device_list();
	n_devices = arv_get_n_devices();

	printf ("Number of found Devices: %d\n", n_devices);

	for (i = 0; i < n_devices; i++)
		printf ("%s\n", arv_get_device_id (i));

	return TRUE;
}

int main(int argc, char** argv)
{
	GMainLoop *main_loop;
	void *sigint_handler;

	sigint_handler = signal(SIGINT, set_cancel);

	main_loop = g_main_loop_new (NULL, FALSE);

	g_timeout_add_seconds (10, periodic_task_cb, main_loop);

	g_main_loop_run (main_loop);

	signal(SIGINT, sigint_handler);

	g_main_loop_unref (main_loop);

	arv_shutdown();

	return 0;
}
