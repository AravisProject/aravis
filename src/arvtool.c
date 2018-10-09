#include <arv.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void
arv_tool_list_features (ArvGc *genicam, const char *feature, gboolean show_description, int level)
{
	ArvGcNode *node;

	node = arv_gc_get_node (genicam, feature);
	if (ARV_IS_GC_FEATURE_NODE (node) &&
	    arv_gc_feature_node_is_implemented (ARV_GC_FEATURE_NODE (node), NULL)) {
		int i;

		for (i = 0; i < level; i++)
			printf ("    ");

		printf ("%s: '%s'%s\n",
			arv_dom_node_get_node_name (ARV_DOM_NODE (node)),
			feature,
			arv_gc_feature_node_is_available (ARV_GC_FEATURE_NODE (node), NULL) ? "" : " (Not available)");

		if (show_description) {
			const char *description;

			description = arv_gc_feature_node_get_description (ARV_GC_FEATURE_NODE (node), NULL);
			if (description)
				printf ("%s\n", description);
		}

		if (ARV_IS_GC_CATEGORY (node)) {
			const GSList *features;
			const GSList *iter;

			features = arv_gc_category_get_features (ARV_GC_CATEGORY (node));

			for (iter = features; iter != NULL; iter = iter->next)
				arv_tool_list_features (genicam, iter->data, show_description, level + 1);
		} else if (ARV_IS_GC_ENUMERATION (node)) {
			const GSList *childs;
			const GSList *iter;

			childs = arv_gc_enumeration_get_entries (ARV_GC_ENUMERATION (node));
			for (iter = childs; iter != NULL; iter = iter->next) {
				if (arv_gc_feature_node_is_implemented (iter->data, NULL)) {
					for (i = 0; i < level + 1; i++)
						printf ("    ");

					printf ("%s: '%s'%s\n",
						arv_dom_node_get_node_name (iter->data),
						arv_gc_feature_node_get_name (iter->data),
						arv_gc_feature_node_is_available (iter->data, NULL) ? "" : " (Not available)");
				}
			}
		}
	}
}

static void
arv_tool_execute_command (int argc, char **argv, ArvDevice *device)
{
	ArvGc *genicam;
	const char *command = argv[1];

	if (device == NULL || argc < 2)
		return;

	genicam = arv_device_get_genicam (device);

	if (g_strcmp0 (command, "genicam") == 0) {
		const char *xml;
		size_t size;

		xml = arv_device_get_genicam_xml (device, &size);
		if (xml != NULL)
			printf ("%*s\n", (int) size, xml);
	} else if (g_strcmp0 (command, "features") == 0) {
		arv_tool_list_features (genicam, "Root", FALSE, 0);
	} else if (g_strcmp0 (command, "description") == 0) {
		if (argc < 3)
			arv_tool_list_features (genicam, "Root", TRUE, 0);
		else {
			int i;

			for (i = 2; i < argc; i++) {
				ArvGcNode *node;

				node = arv_gc_get_node (genicam, argv[i]);
				if (ARV_IS_GC_NODE (node)) {
					const char *description;

					printf ("%s: '%s'\n", arv_dom_node_get_node_name (ARV_DOM_NODE (node)), argv[i]);

					description = arv_gc_feature_node_get_description (ARV_GC_FEATURE_NODE (node), NULL);
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
					GType value_type;
					gint64 max_int64, min_int64;
					double max_double, min_double;
					const char *unit;

					if (tokens[1] != NULL)
						arv_gc_feature_node_set_value_from_string (ARV_GC_FEATURE_NODE (feature),
											   tokens[1], NULL);

					value_type = arv_gc_feature_node_get_value_type (ARV_GC_FEATURE_NODE (feature));

					switch (value_type) {
						case G_TYPE_INT64:
							if (ARV_IS_GC_ENUMERATION (feature)) {
								printf ("%s = %s\n", tokens[0],
									arv_gc_string_get_value (ARV_GC_STRING (feature), NULL));
							} else {
								min_int64 = arv_gc_integer_get_min (ARV_GC_INTEGER (feature), NULL);
								max_int64 = arv_gc_integer_get_max (ARV_GC_INTEGER (feature), NULL);
								unit = arv_gc_integer_get_unit (ARV_GC_INTEGER (feature), NULL);

								if (min_int64 != -G_MAXINT64 && max_int64 != G_MAXINT64)
									printf ("%s = %" G_GINT64_FORMAT
										"%s%s (min:%" G_GINT64_FORMAT
										";max:%" G_GINT64_FORMAT
										")\n", tokens[0],
										arv_gc_integer_get_value (ARV_GC_INTEGER (feature), NULL),
										unit != NULL ? " ": "",
										unit != NULL ? unit : "",
										min_int64, max_int64);
								else
									printf ("%s = %" G_GINT64_FORMAT "%s%s\n",
										tokens[0],
										arv_gc_integer_get_value (ARV_GC_INTEGER (feature), NULL),
										unit != NULL ? " ": "",
										unit != NULL ? unit : "");
							}
							break;
						case G_TYPE_DOUBLE:
							min_double = arv_gc_float_get_min (ARV_GC_FLOAT (feature), NULL);
							max_double = arv_gc_float_get_max (ARV_GC_FLOAT (feature), NULL);
							unit = arv_gc_float_get_unit (ARV_GC_FLOAT (feature), NULL);

							if (min_double != -G_MAXDOUBLE && max_double != G_MAXDOUBLE)
								printf ("%s = %g%s%s (min:%g;max:%g)\n", tokens[0],
									arv_gc_float_get_value (ARV_GC_FLOAT (feature), NULL),
									unit != NULL ? " ": "",
									unit != NULL ? unit : "",
									min_double, max_double);
							else
								printf ("%s = %g%s%s\n", tokens[0],
									arv_gc_float_get_value (ARV_GC_FLOAT (feature), NULL),
									unit != NULL ? " ": "",
									unit != NULL ? unit : "");
							break;
						case G_TYPE_STRING:
							printf ("%s = %s\n", tokens[0],
								arv_gc_string_get_value (ARV_GC_STRING (feature), NULL));
							break;
						case G_TYPE_BOOLEAN:
							printf ("%s = %s\n", tokens[0],
								arv_gc_integer_get_value (ARV_GC_INTEGER (feature), NULL) != 0 ?
								"true" : "false");
							break;
						default:
							printf ("%s = %s\n", tokens[0],
								arv_gc_feature_node_get_value_as_string
								(ARV_GC_FEATURE_NODE (feature),
								 NULL));
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
		printf ("Unkown command\n");
	}
}

static char *arv_option_device_name = NULL;
static char *arv_option_device_address = NULL;
static char *arv_option_debug_domains = NULL;

static const GOptionEntry arv_option_entries[] =
{
	{ "name",		'n', 0, G_OPTION_ARG_STRING,
		&arv_option_device_name,	NULL, "<device_name>"},
	{ "address",		'a', 0, G_OPTION_ARG_STRING,
		&arv_option_device_address,	NULL, "<device_address>"},
	{ "debug", 		'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	NULL, "<category>[:<level>][,...]" },
	{ NULL }
};

static const char
description_content[] =
"Command may be one of the following possibilities:\n"
"\n"
"  genicam:                          dump the content of the Genicam xml data\n"
"  features:                         list all availale features\n"
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

int
main (int argc, char **argv)
{
	ArvDevice *device;
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

	arv_debug_enable (arv_option_debug_domains);

	device_id = arv_option_device_address != NULL ? arv_option_device_address : arv_option_device_name;
	if (device_id != NULL) {
		device = arv_open_device (device_id);

		if (ARV_IS_DEVICE (device)) {
			if (argc < 2)
				printf ("%s\n", device_id);
			else
				arv_tool_execute_command (argc, argv, device);
			g_object_unref (device);
		} else {
			fprintf (stderr, "Device '%s' not found\n", device_id);
		}
	} else {
		arv_update_device_list ();
		n_devices = arv_get_n_devices ();

		if (n_devices > 0) {
			for (i = 0; i < n_devices; i++) {
				device_id = arv_get_device_id (i);
				device = arv_open_device (device_id);

				if (ARV_IS_DEVICE (device)) {
					printf ("%s (%s)\n", device_id, arv_get_device_address (i));
					arv_tool_execute_command (argc, argv, device);

					g_object_unref (device);
				}
			}
		} else {
			fprintf (stderr, "No device found\n");
		}
	}

	arv_shutdown ();

	return EXIT_SUCCESS;
}
