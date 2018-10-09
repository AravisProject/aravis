/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <arv.h>
#include <stdlib.h>
#include <stdio.h>

static gboolean cancel = FALSE;

static void
set_cancel (int signal)
{
       cancel = TRUE;
}

static char *arv_option_interface_name = "lo";
static char *arv_option_debug_domains = NULL;
static char *arv_option_genicam_file = NULL;

static const GOptionEntry arv_option_entries[] =
{
	{ "interface",		'i', 0, G_OPTION_ARG_STRING,
		&arv_option_interface_name,	"Listening interface name", "interface_id"},
	{ "genicam",            'g', 0, G_OPTION_ARG_STRING,
	        &arv_option_genicam_file, "XML Genicam file to use", "genicam_filename"},
	{ "debug", 		'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	NULL, "category[:level][,...]" },
	{ NULL }
};

int
main (int argc, char **argv)
{
	ArvGvFakeCamera *gv_camera;
	GOptionContext *context;
	GError *error = NULL;

	context = g_option_context_new (NULL);
	g_option_context_set_summary (context, "Fake GigEVision camera.");
	g_option_context_set_description (context, "Example: 'arv-fake-gv-camera-" ARAVIS_API_VERSION " -i eth0'");
	g_option_context_add_main_entries (context, arv_option_entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	arv_debug_enable (arv_option_debug_domains);

	arv_set_fake_camera_genicam_filename (arv_option_genicam_file);

	gv_camera = arv_gv_fake_camera_new (arv_option_interface_name);

	signal (SIGINT, set_cancel);

	if (arv_gv_fake_camera_start (gv_camera)) {

		while (!cancel)
			sleep (1);

		arv_gv_fake_camera_stop (gv_camera);
	} else
		printf ("Failed to start camera\n");

	g_object_unref (gv_camera);

	return EXIT_SUCCESS;
}
