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
static char *arv_option_access_check = NULL;
static gboolean arv_option_gv_allow_broadcast_discovery_ack = FALSE;
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
		"{disable|enable|debug}"
	},
	{
		"access-check",			'\0', 0, G_OPTION_ARG_STRING,
		&arv_option_access_check,	"Feature access check policy",
		"{disable|enable}"
	},
	{
		"gv-allow-broadcast-discovery-ack",
                '\0', 0, G_OPTION_ARG_NONE,
		&arv_option_gv_allow_broadcast_discovery_ack,
                "Allow broadcast discovery ack",
		NULL
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
		&arv_option_show_version,     	"Show version",
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
"  network <setting>[=<value>]...:   read/write network settings\n"
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
"arv-tool-" ARAVIS_API_VERSION " network mode=PersistentIP\n"
"arv-tool-" ARAVIS_API_VERSION " network ip=192.168.0.1 mask=255.255.255.0 gateway=192.168.0.254\n"
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
                                } else if (ARV_IS_GC_REGISTER (node)) {
                                        void* buffer;
                                        guint64 length = arv_gc_register_get_length(ARV_GC_REGISTER (node), &error);
                                        arv_gc_register_get (ARV_GC_REGISTER (node), buffer, length, &error);

                                        value = g_strdup_printf ("%" G_GUINT64_FORMAT, (char *)buffer ?  length : 0);
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
                                        } else if (ARV_IS_GC_REGISTER (feature)) {
                                                unsigned char* buffer;

                                                guint64 length = arv_gc_register_get_length(ARV_GC_REGISTER (feature), &error);    
                                                if (error == NULL)
                                                        printf ("Length of %s = %"G_GUINT64_FORMAT"\n", tokens[0], length);        

                                                buffer = (unsigned char*) malloc(length);
                                                arv_gc_register_get (ARV_GC_REGISTER (feature), (void*)buffer, length, &error);    

                                                if (error == NULL){
                                                        printf("Content of %s =", tokens[0]);
                                                        for( int i = 0; i < length; i++){
                                                                if ( i%8 == 0){
                                                                        printf("\n\t");
                                                                }
                                                                printf("0x%02x ", *((buffer)+i));
                                                        }
                                                }
                                                printf("\n");
                                                free(buffer);
                                        }else {
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
                                GError *error = NULL;

                                address = g_ascii_strtoll(&tokens[0][2], NULL, 0);

                                if (tokens[1] != NULL) {
                                        arv_device_write_register (device,
                                                                   address,
                                                                   g_ascii_strtoll (tokens[1],
                                                                                    NULL, 0), &error);
                                        if (error != NULL)
                                                printf ("R[0x%08x] write error: %s\n", address, error->message);
                                }

                                if (error == NULL) {
                                        arv_device_read_register (device, address, &value, &error);
                                        if (error == NULL) {
                                                printf ("R[0x%08x] = 0x%08x\n",
                                                        address, value);
                                        } else {
                                                printf ("R[0x%08x] read error: %s\n", address, error->message);
                                        }
                                }

                                g_clear_error(&error);
                        } else
                                printf ("Feature '%s' not found\n", tokens[0]);
                }
                g_strfreev (tokens);
        }
}

static void
arv_tool_show_network_mode (ArvGvDevice* gv_device, GError** error)
{
        GError *local_error = NULL;
        ArvGvIpConfigurationMode mode;

        mode = arv_gv_device_get_ip_configuration_mode (gv_device, &local_error);
        if (local_error != NULL) {
                g_propagate_error (error, local_error);
                return;
        }

        switch (mode) {
                case ARV_GV_IP_CONFIGURATION_MODE_NONE:
                        printf ("Mode: None\n");
                        break;
                case ARV_GV_IP_CONFIGURATION_MODE_PERSISTENT_IP:
                        printf ("Mode: PersistentIP\n");
                        break;
                case ARV_GV_IP_CONFIGURATION_MODE_DHCP:
                        printf ("Mode: DHCP\n");
                        break;
                case ARV_GV_IP_CONFIGURATION_MODE_LLA:
                        printf ("Mode: LLA\n");
                        break;
                case ARV_GV_IP_CONFIGURATION_MODE_FORCE_IP:
                        printf ("Mode: ForceIP\n");
                        break;
        }
}

static void
arv_tool_show_current_ip (ArvGvDevice* gv_device, GError** error)
{
        GError *local_error = NULL;
        GInetAddress* ip;
        GInetAddressMask* mask;
        GInetAddress* gateway;
        gchar* ip_str;
        gchar* mask_str;
        gchar* gateway_str;

        arv_gv_device_get_current_ip (gv_device, &ip, &mask, &gateway, &local_error);
        if (local_error != NULL) {
                g_propagate_error (error, local_error);
                return;
        }

        ip_str = g_inet_address_to_string (ip);
        mask_str = g_inet_address_mask_to_string (mask);
        gateway_str = g_inet_address_to_string (gateway);

        g_object_unref(ip);
        g_object_unref(mask);
        g_object_unref(gateway);

        printf ("Current IP: %s\nCurrent Mask: %s\nCurrent Gateway: %s\n", ip_str, mask_str, gateway_str);

        g_free (ip_str);
        g_free (mask_str);
        g_free (gateway_str);
}

static void
arv_tool_show_persistent_ip (ArvGvDevice* gv_device, gboolean show_ip, gboolean show_mask, gboolean show_gateway, GError** error)
{
        GError *local_error = NULL;
        GInetAddress* ip;
        GInetAddressMask* mask;
        GInetAddress* gateway;
        gchar* ip_str;
        gchar* mask_str;
        gchar* gateway_str;

        arv_gv_device_get_persistent_ip (gv_device, &ip, &mask, &gateway, &local_error);
        if (local_error != NULL) {
                g_propagate_error (error, local_error);
                return;
        }

        ip_str = g_inet_address_to_string (ip);
        mask_str = g_inet_address_mask_to_string (mask);
        gateway_str = g_inet_address_to_string (gateway);

        g_object_unref(ip);
        g_object_unref(mask);
        g_object_unref(gateway);

        if (show_ip)
                printf ("Persistent IP: %s\n", ip_str);
        if (show_mask)
                printf ("Persistent Mask: %s\n", mask_str);
        if (show_gateway)
                printf ("Persistent Gateway: %s\n", gateway_str);

        g_free (ip_str);
        g_free (mask_str);
        g_free (gateway_str);
}


static void
arv_tool_network (int argc, char **argv, ArvDevice *device)
{
        ArvGvDevice* gv_device = NULL;

        if (!ARV_IS_GV_DEVICE (device)) {
                printf ("This is not a GV device\n");
                return;
        }

        gv_device = ARV_GV_DEVICE (device);
        if (argv[2] == NULL) {
                GError *error = NULL;

                arv_tool_show_network_mode (gv_device, &error);
                if (error == NULL) {
                        arv_tool_show_current_ip (gv_device, &error);
                }
                if (error == NULL) {
                        arv_tool_show_persistent_ip (gv_device, TRUE, TRUE, TRUE, &error);
                }
                if (error != NULL) {
                        printf ("%s error: %s\n", argv[2], error->message);
                        g_clear_error (&error);
                }
        } else {
                int i;

                for (i = 2; i < argc; i++) {
                        GError *error = NULL;
                        char **tokens;

                        tokens = g_strsplit (argv[i], "=", 2);
                        if (g_strcmp0 (tokens[0], "mode") == 0) {
                                if (tokens[1] != NULL) {
                                        ArvGvIpConfigurationMode mode;

                                        if (g_ascii_strcasecmp (tokens[1], "PersistentIP") == 0)
                                                mode = ARV_GV_IP_CONFIGURATION_MODE_PERSISTENT_IP;
                                        else if (g_ascii_strcasecmp (tokens[1], "DHCP") == 0)
                                                mode = ARV_GV_IP_CONFIGURATION_MODE_DHCP;
                                        else if (g_ascii_strcasecmp (tokens[1], "LLA") == 0)
                                                mode = ARV_GV_IP_CONFIGURATION_MODE_LLA;
                                        else {
                                                printf ("Unknown mode \"%s\". Avalaible modes: PersistentIP, DHCP and LLA\n",
                                                        tokens[1]);
                                                return;
                                        }
                                        arv_gv_device_set_ip_configuration_mode (gv_device, mode, &error);
                                } else {
                                        arv_tool_show_network_mode (gv_device, &error);
                                }
                        } else if (g_strcmp0 (tokens[0], "ip") == 0) {
                                if (tokens[1] != NULL) {
                                        arv_gv_device_set_persistent_ip_from_string (gv_device, tokens[1], NULL, NULL, &error);
                                } else {
                                        arv_tool_show_persistent_ip (gv_device, TRUE, FALSE, FALSE, &error);
                                }
                        } else if (g_strcmp0 (tokens[0], "mask") == 0) {
                                if (tokens[1] != NULL) {
                                        arv_gv_device_set_persistent_ip_from_string (gv_device, NULL, tokens[1], NULL, &error);
                                } else {
                                        arv_tool_show_persistent_ip (gv_device, FALSE, TRUE, FALSE, &error);
                                }
                        } else if (g_strcmp0 (tokens[0], "gateway") == 0) {
                                if (tokens[1] != NULL) {
                                        arv_gv_device_set_persistent_ip_from_string (gv_device, NULL, NULL, tokens[1], &error);
                                } else {
                                        arv_tool_show_persistent_ip (gv_device, FALSE, FALSE, TRUE, &error);
                                }
                        }
                        if (error != NULL) {
                                printf ("%s error: %s\n", argv[i], error->message);
                                g_clear_error (&error);
                                return;
                        }
                        g_strfreev (tokens);
                }
        }
}

static void
arv_tool_execute_command (int argc, char **argv, ArvDevice *device,
			  ArvRegisterCachePolicy register_cache_policy,
			  ArvRangeCheckPolicy range_check_policy,
                          ArvAccessCheckPolicy access_check_policy)
{
	ArvGc *genicam;
	const char *command = argv[1];
	gint64 start;

	if (device == NULL || argc < 2)
		return;

	arv_device_set_register_cache_policy (device, register_cache_policy);
	arv_device_set_range_check_policy (device, range_check_policy);
	arv_device_set_access_check_policy (device, access_check_policy);

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
        } else if (g_strcmp0 (command, "network") == 0) {
                arv_tool_network (argc, argv, device);
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
        ArvAccessCheckPolicy access_check_policy;
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

	if (arv_option_access_check == NULL)
		access_check_policy = ARV_ACCESS_CHECK_POLICY_DEFAULT;
	else if (g_strcmp0 (arv_option_access_check, "disable") == 0)
		access_check_policy = ARV_ACCESS_CHECK_POLICY_DISABLE;
	else if (g_strcmp0 (arv_option_access_check, "enable") == 0)
		access_check_policy = ARV_ACCESS_CHECK_POLICY_ENABLE;
	else {
		printf ("Invalid access check policy\n");
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

        if (arv_option_gv_allow_broadcast_discovery_ack)
                arv_set_interface_flags ("GigEVision", ARV_GV_INTERFACE_FLAGS_ALLOW_BROADCAST_DISCOVERY_ACK);

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
							  register_cache_policy,
                                                          range_check_policy,
                                                          access_check_policy);
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
                                                                          register_cache_policy,
                                                                          range_check_policy,
                                                                          access_check_policy);

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
