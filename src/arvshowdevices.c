#include <arv.h>
#include <stdlib.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
	unsigned int n_devices;

	g_type_init ();

	arv_update_device_list ();
	n_devices = arv_get_n_devices ();
	if (n_devices < 1)
		printf ("No device found\n");
	else {
		unsigned int i;

		for (i = 0; i < n_devices; i++)
			printf ("%s\n", (char *) arv_get_device_id (i));
	}

	return EXIT_SUCCESS;
}
