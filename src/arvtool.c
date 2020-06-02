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

#include <arv.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char *arv_option_device_name = NULL;
static char *arv_option_device_address = NULL;
static char *arv_option_debug_domains = NULL;
static char *arv_option_cache_policy = NULL;
static gboolean arv_option_show_time = FALSE;

static const GOptionEntry arv_option_entries[] =
{
	{ "name",		'n', 0, G_OPTION_ARG_STRING,
		&arv_option_device_name,	NULL, "<device_name>"},
	{ "address",		'a', 0, G_OPTION_ARG_STRING,
		&arv_option_device_address,	NULL, "<device_address>"},
	{ "cache-policy",	'c', 0, G_OPTION_ARG_STRING,
		&arv_option_cache_policy, 	"Register cache policy", "[disable|enable|debug]" },
	{ "time",		't', 0, G_OPTION_ARG_NONE,
		&arv_option_show_time, 		"Show execution time", NULL},
	{ "debug", 		'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	NULL, "<category>[:<level>][,...]" },
	{ NULL }
};

static const char
description_content[] =
"Command may be one of the following possibilities:\n"
"\n"
"  genicam:                          dump the content of the Genicam xml data\n"
"  features:                         list all features\n"
"  values:                           list all available feature values\n"
"  description [<feature>] ...:      show the full feature description\n"
"  control <feature>[=<value>] ...:  read/write device features\n"
"\n"
"If no command is given, this utility will list all the available devices.\n"
"For the control command, direct access to device registers is provided using a R[address] syntax"
" in place of a feature name.\n"
"\n"
"Examples:\n"
"\n"
"arv-tool-" ARAVIS_API_VERSION " control Width=128 Height=128 Gain R[0x10000]=0x10\n"
"arv-tool-" ARAVIS_API_VERSION " features\n"
"arv-tool-" ARAVIS_API_VERSION " description Width Height\n"
"arv-tool-" ARAVIS_API_VERSION " -n Basler-210ab4 genicam\n";

typedef enum {
	ARV_TOOL_LIST_MODE_FEATURES,
	ARV_TOOL_LIST_MODE_DESCRIPTIONS,
	ARV_TOOL_LIST_MODE_VALUES
} ArvToolListMode;

static void
arv_tool_list_features (ArvGc *genicam, const char *feature, ArvToolListMode list_mode, int level)
{
	ArvGcNode *node;

	node = arv_gc_get_node (genicam, feature);
	if (ARV_IS_GC_FEATURE_NODE (node) &&
	    arv_gc_feature_node_is_implemented (ARV_GC_FEATURE_NODE (node), NULL)) {

		if (ARV_IS_GC_CATEGORY (node)) {
			printf ("%*s%-12s: '%s'\n", 4 * level, "",
				arv_dom_node_get_node_name (ARV_DOM_NODE (node)),
				feature);
		} else {
			if (arv_gc_feature_node_is_available (ARV_GC_FEATURE_NODE (node), NULL)) {
				char *value = NULL;
				GError *error = NULL;
				gboolean is_selector;

				if (list_mode == ARV_TOOL_LIST_MODE_VALUES) {
					const char *unit;

					if (ARV_IS_GC_STRING (node) ||
					    ARV_IS_GC_ENUMERATION (node)) {
						value = g_strdup_printf ("'%s'", arv_gc_string_get_value (ARV_GC_STRING (node), &error));
					} else if (ARV_IS_GC_INTEGER (node)) {
						if (ARV_IS_GC_ENUMERATION (node)) {
							value = g_strdup_printf ("'%s'",
										 arv_gc_string_get_value (ARV_GC_STRING (node), &error));
						} else {
							unit = arv_gc_integer_get_unit (ARV_GC_INTEGER (node), NULL);

							value = g_strdup_printf ("%" G_GINT64_FORMAT "%s%s",
										 arv_gc_integer_get_value (ARV_GC_INTEGER (node), &error),
										 unit != NULL ? " " : "",
										 unit != NULL ? unit : "");
						}
					} else if (ARV_IS_GC_FLOAT (node)) {
						unit = arv_gc_float_get_unit (ARV_GC_FLOAT (node), NULL);

						value = g_strdup_printf ("%g%s%s",
									 arv_gc_float_get_value (ARV_GC_FLOAT (node), &error),
									 unit != NULL ? " " : "",
									 unit != NULL ? unit : "");
					} else if (ARV_IS_GC_BOOLEAN (node)) {
						value = g_strdup_printf ("%s",
									 arv_gc_boolean_get_value (ARV_GC_BOOLEAN (node), &error) ?
									 "true" : "false");
					}
				}

				is_selector = ARV_IS_GC_SELECTOR (node) && arv_gc_selector_is_selector (ARV_GC_SELECTOR (node));

				if (error != NULL) {
					g_clear_error (&error);
				} else {
					if (value != NULL && value[0] != '\0')
						printf ("%*s%-12s: '%s' = %s\n", 4 * level, "",
							arv_dom_node_get_node_name (ARV_DOM_NODE (node)), feature, value);
					else
						printf ("%*s%-12s: '%s'\n", 4 * level, "",
							arv_dom_node_get_node_name (ARV_DOM_NODE (node)), feature);

					if (is_selector) {
						const GSList *iter;

						for (iter = arv_gc_selector_get_selected_features (ARV_GC_SELECTOR (node));
						     iter != NULL;
						     iter = iter->next) {
							printf (" %*s     * %s\n", 4 * level, " ",
								arv_gc_feature_node_get_name (iter->data));
						}

					}
				}

				g_clear_pointer (&value, g_free);
			} else {
				if (list_mode == ARV_TOOL_LIST_MODE_FEATURES)
					printf ("%*s%-12s: '%s' (Not available)\n", 4 * level, "",
						arv_dom_node_get_node_name (ARV_DOM_NODE (node)), feature);
			}
		}

		if (list_mode == ARV_TOOL_LIST_MODE_DESCRIPTIONS) {
			const char *description;

			description = arv_gc_feature_node_get_description (ARV_GC_FEATURE_NODE (node));
			if (description)
				printf ("%s\n", description);
		}

		if (ARV_IS_GC_CATEGORY (node)) {
			const GSList *features;
			const GSList *iter;

			features = arv_gc_category_get_features (ARV_GC_CATEGORY (node));

			for (iter = features; iter != NULL; iter = iter->next)
				arv_tool_list_features (genicam, iter->data, list_mode, level + 1);
		} else if (ARV_IS_GC_ENUMERATION (node) && list_mode == ARV_TOOL_LIST_MODE_FEATURES) {
			const GSList *childs;
			const GSList *iter;

			childs = arv_gc_enumeration_get_entries (ARV_GC_ENUMERATION (node));
			for (iter = childs; iter != NULL; iter = iter->next) {
				if (arv_gc_feature_node_is_implemented (iter->data, NULL)) {
					printf ("%*s%-12s: '%s'%s\n", 4 * (level + 1), "",
						arv_dom_node_get_node_name (iter->data),
						arv_gc_feature_node_get_name (iter->data),
						arv_gc_feature_node_is_available (iter->data, NULL) ? "" : " (Not available)");
				}
			}
		}
	}
}

static void
arv_tool_execute_command (int argc, char **argv, ArvDevice *device, ArvRegisterCachePolicy cache_policy)
{
	ArvGc *genicam;
	const char *command = argv[1];
	gint64 start;

	if (device == NULL || argc < 2)
		return;

	arv_device_set_register_cache_policy (device, cache_policy);

	genicam = arv_device_get_genicam (device);

	start = g_get_monotonic_time ();

	if (g_strcmp0 (command, "genicam") == 0) {
		const char *xml;
		size_t size;

		xml = arv_device_get_genicam_xml (device, &size);
		if (xml != NULL)
			printf ("%*s\n", (int) size, xml);
	} else if (g_strcmp0 (command, "features") == 0) {
		arv_tool_list_features (genicam, "Root", ARV_TOOL_LIST_MODE_FEATURES, 0);
	} else if (g_strcmp0 (command, "values") == 0) {
		arv_tool_list_features (genicam, "Root", ARV_TOOL_LIST_MODE_VALUES, 0);
	} else if (g_strcmp0 (command, "description") == 0) {
		if (argc < 3)
			arv_tool_list_features (genicam, "Root", ARV_TOOL_LIST_MODE_DESCRIPTIONS, 0);
		else {
			int i;

			for (i = 2; i < argc; i++) {
				ArvGcNode *node;

				node = arv_gc_get_node (genicam, argv[i]);
				if (ARV_IS_GC_NODE (node)) {
					const char *description;

					printf ("%s: '%s'\n", arv_dom_node_get_node_name (ARV_DOM_NODE (node)), argv[i]);

					description = arv_gc_feature_node_get_description (ARV_GC_FEATURE_NODE (node));
					if (description)
						printf ("%s\n", description);
				}
			}
		}
	} else if (g_strcmp0 (command, "control") == 0) {
		int i;

		for (i = 2; i < argc; i++) {
			ArvGcNode *feature;
			char **tokens;

			tokens = g_strsplit (argv[i], "=", 2);
			feature = arv_device_get_feature (device, tokens[0]);
			if (ARV_IS_GC_FEATURE_NODE (feature)) {
				if (ARV_IS_GC_COMMAND (feature)) {
					arv_gc_command_execute (ARV_GC_COMMAND (feature), NULL);
					printf ("%s executed\n", tokens[0]);
				} else {
					const char *unit;
					GError *error = NULL;

					if (tokens[1] != NULL)
						arv_gc_feature_node_set_value_from_string (ARV_GC_FEATURE_NODE (feature),
											   tokens[1], &error);

					if (error == NULL) {
						if (ARV_IS_GC_STRING (feature) ||
						    ARV_IS_GC_ENUMERATION (feature)) {
							const char *value = arv_gc_string_get_value (ARV_GC_STRING (feature), &error);

							if (error == NULL)
								printf ("%s = %s\n", tokens[0], value);
						} else if (ARV_IS_GC_INTEGER (feature)) {
							gint64 max_int64, min_int64, inc_int64;
							gint64 value;

							min_int64 = arv_gc_integer_get_min (ARV_GC_INTEGER (feature), NULL);
							max_int64 = arv_gc_integer_get_max (ARV_GC_INTEGER (feature), NULL);
							inc_int64 = arv_gc_integer_get_inc (ARV_GC_INTEGER (feature), NULL);
							unit = arv_gc_integer_get_unit (ARV_GC_INTEGER (feature), NULL);

							value = arv_gc_integer_get_value (ARV_GC_INTEGER (feature), &error);

							if (error == NULL) {
								GString *string = g_string_new ("");

								g_string_append_printf (string, "%s = %" G_GINT64_FORMAT, tokens[0], value);

								if (unit != NULL)
									g_string_append_printf (string, " %s", unit);
								if (min_int64 != G_MININT64)
									g_string_append_printf (string, " min:%" G_GINT64_FORMAT, min_int64);
								if (max_int64 != G_MAXINT64)
									g_string_append_printf (string, " max:%" G_GINT64_FORMAT, max_int64);
								if (inc_int64 != 1)
									g_string_append_printf (string, " inc:%" G_GINT64_FORMAT, inc_int64);

								printf ("%s\n", string->str);
								g_string_free (string, TRUE);
							}
						} else if (ARV_IS_GC_FLOAT (feature)) {
							double max_double, min_double, inc_double;
							GString *string = g_string_new ("");
							double value;

							min_double = arv_gc_float_get_min (ARV_GC_FLOAT (feature), NULL);
							max_double = arv_gc_float_get_max (ARV_GC_FLOAT (feature), NULL);
							inc_double = arv_gc_float_get_inc (ARV_GC_FLOAT (feature), NULL);
							unit = arv_gc_float_get_unit (ARV_GC_FLOAT (feature), NULL);

							value = arv_gc_float_get_value (ARV_GC_FLOAT (feature), &error);

							if (error == NULL) {
								g_string_append_printf (string, "%s = %g", tokens[0], value);

								if (unit != NULL)
									g_string_append_printf (string, " %s", unit);
								if (min_double != -G_MAXDOUBLE)
									g_string_append_printf (string, " min:%g", min_double);
								if (max_double != G_MAXDOUBLE)
									g_string_append_printf (string, " max:%g", max_double);
								if (inc_double != 1)
									g_string_append_printf (string, " inc:%g", inc_double);

								printf ("%s\n", string->str);
								g_string_free (string, TRUE);
							}
						} else if (ARV_IS_GC_BOOLEAN (feature)) {
							gboolean value = arv_gc_boolean_get_value (ARV_GC_BOOLEAN (feature), &error);

							if (error == NULL)
								printf ("%s = %s\n", tokens[0], value ?  "true" : "false");
						} else {
							const char *value =  arv_gc_feature_node_get_value_as_string
								(ARV_GC_FEATURE_NODE (feature), &error);

							if (error == NULL)
								printf ("%s = %s\n", tokens[0], value);
						}
					}

					if (error != NULL) {
							printf ("%s read error: %s\n", tokens[0], error->message);
							g_clear_error (&error);
					}
				}
			} else {
				if (g_strrstr (tokens[0], "R[") == tokens[0]) {
					guint32 value;
					guint32 address;

					address = g_ascii_strtoll(&tokens[0][2], NULL, 0);

					if (tokens[1] != NULL) {
						arv_device_write_register (device,
									   address,
									   g_ascii_strtoll (tokens[1],
											    NULL, 0), NULL); /* TODO error handling */
					}

					arv_device_read_register (device, address, &value, NULL); /* TODO error handling */

					printf ("R[0x%08x] = 0x%08x\n",
						address, value);
				} else
					printf ("Feature '%s' not found\n", tokens[0]);
			}
			g_strfreev (tokens);
		}
	} else {
		printf ("Unknown command\n");
	}

	if (arv_option_show_time)
		printf ("Executed in %g s\n", (g_get_monotonic_time () - start) / 1000000.0);
}

int
main (int argc, char **argv)
{
	ArvDevice *device;
	ArvRegisterCachePolicy cache_policy = ARV_REGISTER_CACHE_POLICY_DEFAULT;
	const char *device_id;
	GOptionContext *context;
	GError *error = NULL;
	unsigned int n_devices;
	unsigned int i;

	context = g_option_context_new (" command <parameters>");
	g_option_context_set_summary (context, "Small utility for basic control of a Genicam device.");
	g_option_context_set_description (context, description_content);
	g_option_context_add_main_entries (context, arv_option_entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	if (arv_option_cache_policy == NULL ||
	    g_strcmp0 (arv_option_cache_policy, "disable") == 0)
		cache_policy = ARV_REGISTER_CACHE_POLICY_DISABLE;
	else if (g_strcmp0 (arv_option_cache_policy, "enable") == 0)
		cache_policy = ARV_REGISTER_CACHE_POLICY_ENABLE;
	else if (g_strcmp0 (arv_option_cache_policy, "debug") == 0)
		cache_policy = ARV_REGISTER_CACHE_POLICY_DEBUG;
	else {
		printf ("Invalid cache policy\n");
		return EXIT_FAILURE;
	}

	arv_debug_enable (arv_option_debug_domains);

	device_id = arv_option_device_address != NULL ? arv_option_device_address : arv_option_device_name;
	if (device_id != NULL) {
		GError *error = NULL;

		device = arv_open_device (device_id, &error);

		if (ARV_IS_DEVICE (device)) {
			if (argc < 2)
				printf ("%s\n", device_id);
			else
				arv_tool_execute_command (argc, argv, device, cache_policy);
			g_object_unref (device);
		} else {
			fprintf (stderr, "Device '%s' not found%s%s\n", device_id,
				 error != NULL ? ": " : "",
				 error != NULL ? error->message : "");
			g_clear_error (&error);
		}
	} else {
		arv_update_device_list ();
		n_devices = arv_get_n_devices ();

		if (n_devices > 0) {
			for (i = 0; i < n_devices; i++) {
				GError *error = NULL;

				device_id = arv_get_device_id (i);
				device = arv_open_device (device_id, &error);

				if (ARV_IS_DEVICE (device)) {
					printf ("%s (%s)\n", device_id, arv_get_device_address (i));
					arv_tool_execute_command (argc, argv, device, cache_policy);

					g_object_unref (device);
				} else {
					fprintf (stderr, "Failed to open device '%s'%s%s\n", device_id,
						 error != NULL ? ": " : "",
						 error != NULL ? error->message : "");
					g_clear_error (&error);
				}
			}
		} else {
			fprintf (stderr, "No device found\n");
		}
	}

	arv_shutdown ();

	return EXIT_SUCCESS;
}
