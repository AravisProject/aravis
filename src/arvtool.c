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

#include <arvdebugprivate.h>
#include <arvmiscprivate.h>
#include <arv.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char *arv_option_device_selection = NULL;
static char *arv_option_device_address = NULL;
static char *arv_option_debug_domains = NULL;
static char *arv_option_register_cache = NULL;
static char *arv_option_range_check = NULL;
static gboolean arv_option_show_time = FALSE;
static gboolean arv_option_show_version = FALSE;

static const GOptionEntry arv_option_entries[] =
{
	{
		"name",				'n', 0, G_OPTION_ARG_STRING,
		&arv_option_device_selection,	NULL,
		"<pattern>"
	},
	{
		"address",			'a', 0, G_OPTION_ARG_STRING,
		&arv_option_device_address,	NULL,
		"<device_address>"
	},
	{
		"register-cache",		'\0', 0, G_OPTION_ARG_STRING,
		&arv_option_register_cache, 	"Register cache policy",
		"{disable|enable|debug}"
	},
	{
		"range-check",			'\0', 0, G_OPTION_ARG_STRING,
		&arv_option_range_check,	"Range check policy",
		"{disable|enable}"
	},
	{
		"time",				't', 0, G_OPTION_ARG_NONE,
		&arv_option_show_time, 		"Show execution time",
		NULL
	},
	{
		"debug", 			'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	NULL,
		"{<category>[:<level>][,...]|help}"
	},
	{
		"version", 			'v', 0, G_OPTION_ARG_NONE,
		&arv_option_show_version,     	"Show library version",
                NULL
	},
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
"arv-tool-" ARAVIS_API_VERSION " -n Basler-210ab4 genicam";

typedef enum {
	ARV_TOOL_LIST_MODE_FEATURES,
	ARV_TOOL_LIST_MODE_DESCRIPTIONS,
	ARV_TOOL_LIST_MODE_VALUES
} ArvToolListMode;

static void
arv_tool_show_feature (ArvGcFeatureNode *node, ArvToolListMode list_mode, int level)
{
        if (ARV_IS_GC_CATEGORY (node)) {
                printf ("%*s%-12s: '%s'\n", 4 * level, "",
                        arv_dom_node_get_node_name (ARV_DOM_NODE (node)),
                        arv_gc_feature_node_get_name (node));
        } else {
                if (arv_gc_feature_node_is_available (node, NULL)) {
                        char *value = NULL;
                        GError *error = NULL;
                        gboolean is_selector;
                        const char *access_mode;

                        access_mode = arv_gc_access_mode_to_string (arv_gc_feature_node_get_actual_access_mode (node));

                        if (list_mode == ARV_TOOL_LIST_MODE_VALUES) {
                                const char *unit;

                                if (ARV_IS_GC_STRING (node) ||
                                    ARV_IS_GC_ENUMERATION (node)) {
                                        value = g_strdup_printf ("'%s'", arv_gc_string_get_value (ARV_GC_STRING (node),
                                                                                                  &error));
                                } else if (ARV_IS_GC_INTEGER (node)) {
                                        if (ARV_IS_GC_ENUMERATION (node)) {
                                                value = g_strdup_printf ("'%s'",
                                                                         arv_gc_string_get_value (ARV_GC_STRING (node),
                                                                                                  &error));
                                        } else {
                                                unit = arv_gc_integer_get_unit (ARV_GC_INTEGER (node));

                                                value = g_strdup_printf ("%" G_GINT64_FORMAT "%s%s",
                                                                         arv_gc_integer_get_value (ARV_GC_INTEGER (node),
                                                                                                   &error),
                                                                         unit != NULL ? " " : "",
                                                                         unit != NULL ? unit : "");
                                        }
                                } else if (ARV_IS_GC_FLOAT (node)) {
                                        unit = arv_gc_float_get_unit (ARV_GC_FLOAT (node));

                                        value = g_strdup_printf ("%g%s%s",
                                                                 arv_gc_float_get_value (ARV_GC_FLOAT (node),
                                                                                         &error),
                                                                 unit != NULL ? " " : "",
                                                                 unit != NULL ? unit : "");
                                } else if (ARV_IS_GC_BOOLEAN (node)) {
                                        value = g_strdup_printf ("%s",
                                                                 arv_gc_boolean_get_value (ARV_GC_BOOLEAN (node),
                                                                                           &error) ?  "true" : "false");
                                }
                        }

                        is_selector = ARV_IS_GC_SELECTOR (node) && arv_gc_selector_is_selector (ARV_GC_SELECTOR (node));

                        if (error != NULL) {
                                g_clear_error (&error);
                        } else {
                                if (value != NULL && value[0] != '\0')
                                        printf ("%*s%-13s: [%s] '%s' = %s\n", 4 * level, "",
                                                arv_dom_node_get_node_name (ARV_DOM_NODE (node)),
                                                access_mode, arv_gc_feature_node_get_name (node), value);
                                else
                                        printf ("%*s%-13s: [%s] '%s'\n", 4 * level, "",
                                                arv_dom_node_get_node_name (ARV_DOM_NODE (node)),
                                                access_mode, arv_gc_feature_node_get_name (node));

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
                                        arv_dom_node_get_node_name (ARV_DOM_NODE (node)),
                                        arv_gc_feature_node_get_name (node));
                }
        }

        if (list_mode == ARV_TOOL_LIST_MODE_DESCRIPTIONS) {
                const char *description;

                description = arv_gc_feature_node_get_description (node);
                if (description)
                        printf ("%s\n", description);
        }

        if (ARV_IS_GC_ENUMERATION (node) && list_mode == ARV_TOOL_LIST_MODE_FEATURES) {
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

static void
arv_tool_list_features (ArvGc *genicam, const char *feature, ArvToolListMode list_mode, GRegex *regex, int level)
{
	ArvGcNode *node;

	node = arv_gc_get_node (genicam, feature);
	if (ARV_IS_GC_FEATURE_NODE (node) &&
	    arv_gc_feature_node_is_implemented (ARV_GC_FEATURE_NODE (node), NULL)) {
                gboolean match;

                match = regex == NULL || g_regex_match (regex,
                                                        arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (node)),
                                                        0, NULL);
                if (match)
                        arv_tool_show_feature (ARV_GC_FEATURE_NODE (node), list_mode, level);

		if (ARV_IS_GC_CATEGORY (node)) {
			const GSList *features;
			const GSList *iter;

			features = arv_gc_category_get_features (ARV_GC_CATEGORY (node));

			for (iter = features; iter != NULL; iter = iter->next)
				arv_tool_list_features (genicam, iter->data, list_mode, match ? NULL : regex, level + 1);
                }
	}
}

static void
arv_tool_control (int argc, char **argv, ArvDevice *device)
{
        int i;

        for (i = 2; i < argc; i++) {
                ArvGcNode *feature;
                char **tokens;

                tokens = g_strsplit (argv[i], "=", 2);
                feature = arv_device_get_feature (device, tokens[0]);
                if (ARV_IS_GC_FEATURE_NODE (feature)) {
                        if (ARV_IS_GC_COMMAND (feature)) {
                                GError *error = NULL;

                                arv_gc_command_execute (ARV_GC_COMMAND (feature), &error);
                                if (error != NULL) {
                                        printf ("%s execute error: %s\n",
                                                tokens[0],
                                                error->message);
                                        g_clear_error (&error);
                                } else
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
                                                const char *value = arv_gc_string_get_value (ARV_GC_STRING (feature),
                                                                                             &error);

                                                if (error == NULL)
                                                        printf ("%s = %s\n", tokens[0], value);
                                        } else if (ARV_IS_GC_INTEGER (feature)) {
                                                gint64 max_int64, min_int64, inc_int64;
                                                gint64 value;

                                                min_int64 = arv_gc_integer_get_min (ARV_GC_INTEGER (feature), NULL);
                                                max_int64 = arv_gc_integer_get_max (ARV_GC_INTEGER (feature), NULL);
                                                inc_int64 = arv_gc_integer_get_inc (ARV_GC_INTEGER (feature), NULL);
                                                unit = arv_gc_integer_get_unit (ARV_GC_INTEGER (feature));

                                                value = arv_gc_integer_get_value (ARV_GC_INTEGER (feature), &error);

                                                if (error == NULL) {
                                                        GString *string = g_string_new ("");

                                                        g_string_append_printf (string, "%s = %" G_GINT64_FORMAT,
                                                                                tokens[0], value);

                                                        if (unit != NULL)
                                                                g_string_append_printf (string, " %s", unit);
                                                        if (min_int64 != G_MININT64)
                                                                g_string_append_printf (string, " min:%" G_GINT64_FORMAT,
                                                                                        min_int64);
                                                        if (max_int64 != G_MAXINT64)
                                                                g_string_append_printf (string, " max:%" G_GINT64_FORMAT,
                                                                                        max_int64);
                                                        if (inc_int64 != 1)
                                                                g_string_append_printf (string, " inc:%" G_GINT64_FORMAT,
                                                                                        inc_int64);

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
                                                unit = arv_gc_float_get_unit (ARV_GC_FLOAT (feature));

                                                value = arv_gc_float_get_value (ARV_GC_FLOAT (feature), &error);

                                                if (error == NULL) {
                                                        g_string_append_printf (string, "%s = %g", tokens[0], value);

                                                        if (unit != NULL)
                                                                g_string_append_printf (string, " %s", unit);
                                                        if (min_double != -G_MAXDOUBLE)
                                                                g_string_append_printf (string, " min:%g", min_double);
                                                        if (max_double != G_MAXDOUBLE)
                                                                g_string_append_printf (string, " max:%g", max_double);
                                                        if (inc_double != G_MINDOUBLE)
                                                                g_string_append_printf (string, " inc:%g", inc_double);

                                                        printf ("%s\n", string->str);
                                                        g_string_free (string, TRUE);
                                                }
                                        } else if (ARV_IS_GC_BOOLEAN (feature)) {
                                                gboolean value = arv_gc_boolean_get_value (ARV_GC_BOOLEAN (feature),
                                                                                           &error);

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
                                        printf ("%s %s error: %s\n",
                                                tokens[0],
                                                tokens[1] != NULL ? "write" : "read",
                                                error->message);
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
}

static void
arv_tool_execute_command (int argc, char **argv, ArvDevice *device,
			  ArvRegisterCachePolicy register_cache_policy,
			  ArvRangeCheckPolicy range_check_policy)
{
	ArvGc *genicam;
	const char *command = argv[1];
	gint64 start;

	if (device == NULL || argc < 2)
		return;

	arv_device_set_register_cache_policy (device, register_cache_policy);
	arv_device_set_range_check_policy (device, range_check_policy);

	genicam = arv_device_get_genicam (device);

	start = g_get_monotonic_time ();

	if (g_strcmp0 (command, "genicam") == 0) {
		const char *xml;
		size_t size;

		xml = arv_device_get_genicam_xml (device, &size);
		if (xml != NULL)
			printf ("%*s\n", (int) size, xml);
	} else if (g_strcmp0 (command, "features") == 0) {
                if (argc > 3)
                        printf ("features command takes at most one feature selection parameter\n");
                else {
                        GRegex *regex;

                        regex = arv_regex_new_from_glob_pattern (argc == 3 ? argv[2] : "*", TRUE);
                        arv_tool_list_features (genicam, "Root", ARV_TOOL_LIST_MODE_FEATURES,  regex, 0);
                        g_regex_unref (regex);
                }
	} else if (g_strcmp0 (command, "values") == 0) {
                if (argc > 3)
                        printf ("features command takes at most one feature selection parameter\n");
                else {
                        GRegex *regex;

                        regex = arv_regex_new_from_glob_pattern (argc == 3 ? argv[2] : "*", TRUE);
                        arv_tool_list_features (genicam, "Root", ARV_TOOL_LIST_MODE_VALUES, regex, 0);
                        g_regex_unref (regex);
                }
        } else if (g_strcmp0 (command, "description") == 0) {
                if (argc > 3)
                        printf ("features command takes at most one feature selection parameter\n");
                else {
                        GRegex *regex;

                        regex = arv_regex_new_from_glob_pattern (argc == 3 ? argv[2] : "*", TRUE);
                        arv_tool_list_features (genicam, "Root", ARV_TOOL_LIST_MODE_DESCRIPTIONS, regex, 0);
                        g_regex_unref (regex);
                }
	} else if (g_strcmp0 (command, "control") == 0) {
                arv_tool_control (argc, argv, device);
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
	ArvRegisterCachePolicy register_cache_policy;
	ArvRangeCheckPolicy range_check_policy;
        GRegex *regex;
	const char *device_id;
	GOptionContext *context;
	GError *error = NULL;
	unsigned int n_devices;
        unsigned int n_found_devices = 0;
	unsigned int i;
        gboolean is_glob_pattern = FALSE;

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

        if (arv_option_show_version) {
                printf ("%u.%u.%u\n",
                        arv_get_major_version (),
                        arv_get_minor_version (),
                        arv_get_micro_version ());
                return EXIT_SUCCESS;
        }

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

	if (!arv_debug_enable (arv_option_debug_domains)) {
		if (g_strcmp0 (arv_option_debug_domains, "help") != 0)
			printf ("Invalid debug selection\n");
		else
			arv_debug_print_infos ();
		return EXIT_FAILURE;
	}

        for (i = 0; arv_option_device_selection != NULL && arv_option_device_selection[i] != '\0'; i++)
                if (arv_option_device_selection[i] == '*' ||
                    arv_option_device_selection[i] == '?' ||
                    arv_option_device_selection[i] == '|')
                        is_glob_pattern = TRUE;

	device_id = arv_option_device_address != NULL ?
                arv_option_device_address :
                (is_glob_pattern ? NULL : arv_option_device_selection);
	if (device_id != NULL) {
		GError *error = NULL;

		device = arv_open_device (device_id, &error);
		if (ARV_IS_DEVICE (device)) {
			if (argc < 2) {
				printf ("%s\n", device_id);
			} else {
				arv_tool_execute_command (argc, argv, device,
							  register_cache_policy, range_check_policy);
                        }
			g_object_unref (device);
                } else {
                        if (error != NULL) {
                                fprintf (stderr, "%s\n", error->message);
                                g_clear_error (&error);
                        } else {
                                fprintf (stderr, "Device '%s' not found", device_id);
                        }
                }

                arv_shutdown ();

                return EXIT_SUCCESS;
        }

        arv_update_device_list ();
        n_devices = arv_get_n_devices ();

        regex = arv_regex_new_from_glob_pattern (arv_option_device_selection != NULL ?
                                                 arv_option_device_selection : "*", TRUE);

        if (n_devices > 0) {
                for (i = 0; i < n_devices; i++) {
                        GError *error = NULL;

                        device_id = arv_get_device_id (i);

                        if (g_regex_match (regex, device_id, 0, NULL)) {
                                n_found_devices++;

                                printf ("%s (%s)\n", device_id, arv_get_device_address (i));

                                if (argc >= 2) {
                                        device = arv_open_device (device_id, &error);

                                        if (ARV_IS_DEVICE (device)) {
                                                arv_tool_execute_command (argc, argv, device,
                                                                          register_cache_policy, range_check_policy);

                                                g_object_unref (device);
                                        } else {
                                                fprintf (stderr, "Failed to open device '%s'%s%s\n", device_id,
                                                         error != NULL ? ": " : "",
                                                         error != NULL ? error->message : "");
                                                g_clear_error (&error);
                                        }
                                }
                        }
                }
        }

        if (n_found_devices < 1) {
                if (n_devices > 0)
                        fprintf (stderr, "No matching device found (%d filtered out)\n", n_devices);
                else
                        fprintf (stderr, "No device found\n");
        }

        g_regex_unref (regex);

	arv_shutdown ();

	return EXIT_SUCCESS;
}
