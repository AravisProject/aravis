/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#include <arvdebugprivate.h>
#include <arv.h>
#include <stdlib.h>
#include <stdio.h>

static gboolean cancel = FALSE;

static void
set_cancel (int signal)
{
       cancel = TRUE;
}

static char *arv_option_interface_name = NULL;
static char *arv_option_serial_number = NULL;
static char *arv_option_genicam_file = NULL;
static double arv_option_gvsp_lost_ratio = 0.0;
static char *arv_option_debug_domains = NULL;

static const GOptionEntry arv_option_entries[] =
{
	{ "interface",		'i', 0, G_OPTION_ARG_STRING,
		&arv_option_interface_name,	"Listening interface name or address", "interface"},
	{ "serial",             's', 0, G_OPTION_ARG_STRING,
	        &arv_option_serial_number, 	"Fake camera serial number", "serial_nbr"},
	{ "genicam",            'g', 0, G_OPTION_ARG_STRING,
	        &arv_option_genicam_file, 	"XML Genicam file to use", "genicam_filename"},
	{ "gvsp-lost-ratio",    'r', 0, G_OPTION_ARG_DOUBLE,
	        &arv_option_gvsp_lost_ratio,	"GVSP lost packet ratio", "packet_per_thousand"},
	{
		"debug", 			'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	NULL,
		"{<category>[:<level>][,...]|help}"
	},
	{ NULL }
};

static const char
description_content[] =
"The genicam parameter is for debug purpose only. It is not possible to load\n"
"any arbitrary genicam data, as the declared features must match the registers\n"
"of the fake device.\n"
"\n"
"Examples:\n"
"\n"
"arv-fake-gv-camera-" ARAVIS_API_VERSION " -i eth0\n"
"arv-fake-gv-camera-" ARAVIS_API_VERSION " -i 127.0.0.1\n"
"arv-fake-gv-camera-" ARAVIS_API_VERSION " -s GV02 -d all\n";

int
main (int argc, char **argv)
{
	ArvGvFakeCamera *gv_camera;
	GOptionContext *context;
	GError *error = NULL;

	context = g_option_context_new (NULL);
	g_option_context_set_summary (context, "Fake GigEVision camera.");
	g_option_context_set_description (context, description_content);
	g_option_context_add_main_entries (context, arv_option_entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	if (!arv_debug_enable (arv_option_debug_domains)) {
		if (g_strcmp0 (arv_option_debug_domains, "help") != 0)
			printf ("Invalid debug selection\n");
		else
			arv_debug_print_infos ();
		return EXIT_FAILURE;
	}

	gv_camera = arv_gv_fake_camera_new_full (arv_option_interface_name, arv_option_serial_number, arv_option_genicam_file);

	g_object_set (gv_camera, "gvsp-lost-ratio", arv_option_gvsp_lost_ratio / 1000.0, NULL);

	signal (SIGINT, set_cancel);

	if (arv_gv_fake_camera_is_running (gv_camera))
		while (!cancel)
			g_usleep (1000000);
	else
		printf ("Failed to start camera\n");

	g_object_unref (gv_camera);

	return EXIT_SUCCESS;
}
