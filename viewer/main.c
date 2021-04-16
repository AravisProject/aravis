/* Aravis - Digital camera library
 *
 * Copyright © 2009-2019 Emmanuel Pacaud
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

#include <arvviewer.h>
#include <arvdebugprivate.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <arv.h>
#include <stdlib.h>
#include <libnotify/notify.h>
#include <libintl.h>

#if GST_GL_HAVE_WINDOW_X11 && defined (GDK_WINDOWING_X11)
#include <gst/gl/x11/gstgldisplay_x11.h>
#endif

static char *arv_viewer_option_debug_domains = NULL;
static char *arv_option_register_cache = NULL;
static char *arv_option_range_check = NULL;
static gboolean arv_viewer_option_auto_socket_buffer = FALSE;
static gboolean arv_viewer_option_no_packet_resend = FALSE;
static unsigned int arv_viewer_option_packet_timeout = 20;
static unsigned int arv_viewer_option_frame_retention = 100;

static const GOptionEntry arv_viewer_option_entries[] =
{
	{
		"auto-buffer-size",			'a', 0, G_OPTION_ARG_NONE,
		&arv_viewer_option_auto_socket_buffer,	"Auto socket buffer size", NULL
	},
	{
		"no-packet-resend",			'r', 0, G_OPTION_ARG_NONE,
		&arv_viewer_option_no_packet_resend,	"No packet resend", NULL
	},
	{
		"packet-timeout", 			'p', 0, G_OPTION_ARG_INT,
		&arv_viewer_option_packet_timeout, 	"Packet timeout (ms)", NULL
	},
	{
		"frame-retention", 			'm', 0, G_OPTION_ARG_INT,
		&arv_viewer_option_frame_retention, 	"Frame retention (ms)", NULL
	},
	{
		"register-cache",			'\0', 0, G_OPTION_ARG_STRING,
		&arv_option_register_cache, 		"Register cache policy",
		"{disable|enable|debug}"
	},
	{
		"range-check",				'\0', 0, G_OPTION_ARG_STRING,
		&arv_option_range_check,		"Range check policy",
		"{disable|enable}"
	},
	{
		"debug", 				'd', 0, G_OPTION_ARG_STRING,
		&arv_viewer_option_debug_domains, 	NULL,
		"{<category>[:<level>][,...]|help}"
	},
	{ NULL }
};

int
main (int argc, char **argv)
{
	ArvViewer *viewer;
	GtkIconTheme *icon_theme;
	int status;
	GOptionContext *context;
	GError *error = NULL;
	ArvRegisterCachePolicy register_cache_policy;
	ArvRangeCheckPolicy range_check_policy;

#if GST_GL_HAVE_WINDOW_X11 && defined(GDK_WINDOWING_X11)
	XInitThreads ();
#endif

	bindtextdomain (ARAVIS_GETTEXT, ARAVIS_LOCALE_DIR);
	bind_textdomain_codeset (ARAVIS_GETTEXT, "UTF-8");
	textdomain (ARAVIS_GETTEXT);

	gtk_init (&argc, &argv);
	gst_init (&argc, &argv);

	icon_theme = gtk_icon_theme_get_default ();
	gtk_icon_theme_add_resource_path (icon_theme, "/org/aravis/viewer/icons/gnome/");

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, arv_viewer_option_entries, NULL);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_add_group (context, gst_init_get_option_group ());
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	if (arv_option_register_cache == NULL)
		register_cache_policy = ARV_REGISTER_CACHE_POLICY_DEFAULT;
	else if (g_strcmp0 (arv_option_register_cache, "disable") == 0)
		register_cache_policy = ARV_REGISTER_CACHE_POLICY_DISABLE;
	else if (g_strcmp0 (arv_option_register_cache, "enable") == 0)
		register_cache_policy = ARV_REGISTER_CACHE_POLICY_ENABLE;
	else if (g_strcmp0 (arv_option_register_cache, "debug") == 0)
		register_cache_policy = ARV_REGISTER_CACHE_POLICY_DEBUG;
	else {
		printf ("Invalid register cache policy\n");
		return EXIT_FAILURE;
	}

	if (arv_option_range_check == NULL)
		range_check_policy = ARV_RANGE_CHECK_POLICY_DEFAULT;
	else if (g_strcmp0 (arv_option_range_check, "disable") == 0)
		range_check_policy = ARV_RANGE_CHECK_POLICY_DISABLE;
	else if (g_strcmp0 (arv_option_range_check, "enable") == 0)
		range_check_policy = ARV_RANGE_CHECK_POLICY_ENABLE;
	else if (g_strcmp0 (arv_option_range_check, "debug") == 0)
		range_check_policy = ARV_RANGE_CHECK_POLICY_DEBUG;
	else {
		printf ("Invalid range check policy\n");
		return EXIT_FAILURE;
	}

	if (!arv_debug_enable (arv_viewer_option_debug_domains)) {
		if (g_strcmp0 (arv_viewer_option_debug_domains, "help") != 0)
			printf ("Invalid debug selection\n");
		else
			arv_debug_print ();
		return EXIT_FAILURE;
	}

	viewer = arv_viewer_new ();
	if (!ARV_IS_VIEWER (viewer))
		return EXIT_FAILURE;

	arv_viewer_set_options (viewer,
				arv_viewer_option_auto_socket_buffer,
				!arv_viewer_option_no_packet_resend,
				arv_viewer_option_packet_timeout,
				arv_viewer_option_frame_retention,
				register_cache_policy,
				range_check_policy);

	notify_init ("Aravis Viewer");

	status = g_application_run (G_APPLICATION (viewer), argc, argv);

	g_object_unref (viewer);

	notify_uninit ();

	return status;
}
