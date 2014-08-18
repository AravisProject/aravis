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

/**
 * SECTION: arvdebug
 * @short_description: Debugging tools
 */

#include <arvdebug.h>
#include <glib/gprintf.h>
#include <stdlib.h>

ArvDebugCategory arv_debug_category_interface =
{
	.name = "interface",
	.level = -1
};

ArvDebugCategory arv_debug_category_device =
{
	.name = "device",
	.level = -1
};

ArvDebugCategory arv_debug_category_chunk =
{
	.name = "chunk",
	.level = -1
};

ArvDebugCategory arv_debug_category_stream =
{
	.name = "stream",
	.level = -1
};

ArvDebugCategory arv_debug_category_stream_thread =
{
	.name = "stream-thread",
	.level = -1
};

ArvDebugCategory arv_debug_category_gvcp =
{
	.name = "gvcp",
	.level = -1
};

ArvDebugCategory arv_debug_category_gvsp =
{
	.name = "gvsp",
	.level = -1
};

ArvDebugCategory arv_debug_category_genicam =
{
	.name = "genicam",
	.level = -1
};

ArvDebugCategory arv_debug_category_evaluator =
{
	.name = "evaluator",
	.level = -1
};

ArvDebugCategory arv_debug_category_misc =
{
	.name = "misc",
	.level = -1
};

static GHashTable *arv_debug_categories = NULL;

static void
arv_debug_category_free (ArvDebugCategory *category)
{
	if (category != NULL) {
		g_free (category->name);
		g_free (category);
	}
}

static void
arv_debug_initialize (const char *debug_var)
{
	if (arv_debug_categories != NULL)
		return;

	arv_debug_categories = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
						      (GDestroyNotify) arv_debug_category_free);

	if (debug_var != NULL) {
		char **categories;
		int i;

		categories = g_strsplit (debug_var, ",", -1);
		for (i = 0; categories[i] != NULL; i++) {
			ArvDebugCategory *category;
			char **infos;

			category = g_new0 (ArvDebugCategory, 1);

			infos = g_strsplit (categories[i], ":", -1);
			if (infos[0] != NULL) {
				category->name = g_strdup (infos[0]);
				if (infos[1] != NULL)
					category->level = atoi (infos[1]);
				else
					category->level = ARV_DEBUG_LEVEL_DEBUG;

				g_hash_table_insert (arv_debug_categories, category->name, category);
			} else
				g_free (category);

			g_strfreev (infos);
		}
		g_strfreev (categories);
	}
}

gboolean
arv_debug_check	(ArvDebugCategory *category, ArvDebugLevel level)
{
	ArvDebugCategory *configured_category;

	if (category == NULL)
		return FALSE;

	if ((int) level <= (int) category->level)
		return TRUE;

	if ((int) category->level >= 0)
		return FALSE;

	arv_debug_initialize (g_getenv ("ARV_DEBUG"));

	configured_category = g_hash_table_lookup (arv_debug_categories, category->name);
	if (configured_category == NULL)
		configured_category = g_hash_table_lookup (arv_debug_categories, "all");
	if (configured_category != NULL)
		category->level = configured_category->level;
	else
		category->level = 0;


	return (int) level <= (int) category->level;
}

static void
arv_debug_with_level (ArvDebugCategory *category, ArvDebugLevel level, const char *format, va_list args)
{
	if (!arv_debug_check (category, level))
		return;

	g_vprintf (format, args);
	g_printf ("\n");
}

void
arv_warning (ArvDebugCategory *category, const char *format, ...)
{
	va_list args;

	va_start (args, format);
	arv_debug_with_level (category, ARV_DEBUG_LEVEL_WARNING, format, args);
	va_end (args);
}

void
arv_debug (ArvDebugCategory *category, const char *format, ...)
{
	va_list args;

	va_start (args, format);
	arv_debug_with_level (category, ARV_DEBUG_LEVEL_DEBUG, format, args);
	va_end (args);
}

void
arv_log (ArvDebugCategory *category, const char *format, ...)
{
	va_list args;

	va_start (args, format);
	arv_debug_with_level (category, ARV_DEBUG_LEVEL_LOG, format, args);
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
 */

void
arv_debug_enable (const char *category_selection)
{
	arv_debug_initialize (category_selection);
}

void
arv_debug_shutdown (void)
{
	GHashTable *debug_categories = arv_debug_categories;

	arv_debug_categories = NULL;

	if (debug_categories != NULL)
		g_hash_table_unref (debug_categories);
}
