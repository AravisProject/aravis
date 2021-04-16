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

/**
 * SECTION: arvdebug
 * @short_description: Debugging tools
 */

#include <glib/gprintf.h>
#include <stdlib.h>
#include <arvdebugprivate.h>
#include <arvenumtypesprivate.h>

ArvDebugCategoryInfos arv_debug_category_infos[] = {
	{ .name = "interface", 		.description = "Device lookup for each supported protocol" },
	{ .name = "device", 		.description = "Device control" },
	{ .name = "stream", 		.description = "Video stream management" },
	{ .name = "stream-thread", 	.description = "Video stream thread (likely high volume output)" },
	{ .name = "cp", 		.description = "Control protocol packets" },
	{ .name = "sp", 		.description = "Stream protocol packets (likely high volume output)" },
	{ .name = "genicam", 		.description = "Genicam specialized DOM elements" },
	{ .name = "chunk", 		.description = "Chunk data code" },
	{ .name = "dom", 		.description = "Genicam DOM document" },
	{ .name = "evaluator", 		.description = "Expression evaluator" },
	{ .name = "viewer", 		.description = "Simple viewer application" },
	{ .name = "misc", 		.description = "Miscellaneous code" }
};

static gboolean
arv_debug_initialize (const char *debug_var)
{
	gboolean success = TRUE;
	char **categories;
	int i;

	if (debug_var == NULL)
		return TRUE;

	categories = g_strsplit (debug_var, ",", -1);
	for (i = 0; categories[i] != NULL; i++) {
		char **infos;
		unsigned int j;

		infos = g_strsplit (categories[i], ":", -1);
		if (infos[0] != NULL) {
			for (j = 0; j < G_N_ELEMENTS (arv_debug_category_infos); j++) {
				if (g_strcmp0 (arv_debug_category_infos[j].name, infos[0]) == 0 ||
				    g_strcmp0 ("all", infos[0]) == 0) {
					if (infos[1] != NULL)
						arv_debug_category_infos[j].level = atoi (infos[1]);
					else
						arv_debug_category_infos[j].level = ARV_DEBUG_LEVEL_DEBUG;
				} else {
					success = FALSE;
				}
			}
		}

		g_strfreev (infos);
	}
	g_strfreev (categories);

	return success;
}

gboolean
arv_debug_check	(ArvDebugCategory category, ArvDebugLevel level)
{
	if (category < 0 || category >= G_N_ELEMENTS (arv_debug_category_infos))
		return FALSE;

	if ((int) level <= (int) arv_debug_category_infos[category].level)
		return TRUE;

	return FALSE;
}

static void arv_debug_with_level (ArvDebugCategory category,
				  ArvDebugLevel level,
				  const char *format,
				  va_list args) G_GNUC_PRINTF(3,0);

static void
arv_debug_with_level (ArvDebugCategory category, ArvDebugLevel level, const char *format, va_list args)
{
	if (!arv_debug_check (category, level))
		return;

	g_vprintf (format, args);
	g_printf ("\n");
}

void
arv_warning (ArvDebugCategory category, const char *format, ...)
{
	va_list args;

	va_start (args, format);
	arv_debug_with_level (category, ARV_DEBUG_LEVEL_WARNING, format, args);
	va_end (args);
}

void
arv_debug (ArvDebugCategory category, const char *format, ...)
{
	va_list args;

	va_start (args, format);
	arv_debug_with_level (category, ARV_DEBUG_LEVEL_DEBUG, format, args);
	va_end (args);
}

void
arv_log (ArvDebugCategory category, const char *format, ...)
{
	va_list args;

	va_start (args, format);
	arv_debug_with_level (category, ARV_DEBUG_LEVEL_LOG, format, args);
	va_end (args);
}

void
arv_verbosely_log (ArvDebugCategory category, const char *format, ...)
{
	va_list args;

	va_start (args, format);
	arv_debug_with_level (category, ARV_DEBUG_LEVEL_VERBOSE_LOG, format, args);
	va_end (args);
}

void
arv_message (const char *format, ...)
{
	va_list args;

	va_start (args, format);
	g_vprintf (format, args);
	g_printf ("\n");
	va_end (args);
}

/**
 * arv_debug_enable:
 * @category_selection: debug category configuration string
 *
 * Configures the debug output using a configuration string consisting of a comma separated list of debug categories or category/debug level pair. This function overwrites the configuration done by ARV_DEBUG environment variable.
 * For example, enabling debug level 3 of the gvcp category and default debug level of category genicam is done using:
 *
 * <informalexample><programlisting>
 * arv_debug_enable ("gvcp:3,genicam");
 * </programlisting></informalexample>
 *
 * Returns: %TRUE on success
 */

gboolean
arv_debug_enable (const char *category_selection)
{
	return arv_debug_initialize (category_selection);
}

static char *
arv_debug_dup_infos_as_string (void)
{
	GEnumClass *debug_level_class = g_type_class_ref (ARV_TYPE_DEBUG_LEVEL);
	GString *string = g_string_new ("");
	unsigned int i;
	char *str;

	g_string_append (string, "Debug categories:\n");
	for (i = 0; i < ARV_DEBUG_CATEGORY_N_ELEMENTS; i++) {
		g_string_append_printf (string, "%-15s: %s\n",
					arv_debug_category_infos[i].name,
					arv_debug_category_infos[i].description);
	}
	g_string_append (string, "all            : Everything\n");

	g_string_append (string, "\nDebug levels:\n");
	for (i = 0; i < ARV_DEBUG_LEVEL_N_ELEMENTS; i++) {
		GEnumValue *enum_value;

		enum_value = g_enum_get_value (g_type_class_ref (ARV_TYPE_DEBUG_LEVEL), i);
		if (enum_value != NULL)
			g_string_append_printf (string, "%d: %s\n", i, enum_value->value_nick);
	}

	g_type_class_unref (debug_level_class);

	str = string->str;
	g_string_free (string, FALSE);

	return str;
}

void
arv_debug_print_infos (void)
{
	char *str;

	str = arv_debug_dup_infos_as_string ();
	printf ("%s", str);
	g_free (str);
}

__attribute__((constructor)) static void
arv_initialize_debug (void) {
	unsigned int i;

	for (i = 0; i < ARV_DEBUG_CATEGORY_N_ELEMENTS; i++)
		arv_debug_category_infos[i].level = 0;

	arv_debug_initialize (g_getenv ("ARV_DEBUG"));
}

