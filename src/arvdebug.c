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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <arvdebug.h>
#include <glib/gprintf.h>
#include <stdlib.h>

static gboolean arv_debug_checked = FALSE;
static gboolean arv_debug_level = ARV_DEBUG_LEVEL_NONE;

static gboolean
arv_debug_get_level (void)
{
	const char *debug_var;

	if (arv_debug_checked)
		return arv_debug_level;

	debug_var = g_getenv ("ARV_DEBUG");

	arv_debug_level = debug_var != NULL ? atoi (debug_var) : ARV_DEBUG_LEVEL_NONE;

	arv_debug_checked = TRUE;

	return arv_debug_level;
}

void
arv_debug (ArvDebugLevel level, char const *format, ...)
{
	va_list args;

	if (level > arv_debug_get_level ())
		return;

	va_start (args, format);
	g_vprintf (format, args);
	g_printf ("\n");
	va_end (args);
}

gboolean
arv_debug_check (ArvDebugLevel level)
{
	return arv_debug_get_level () >= level;
}

void
arv_debug_enable (ArvDebugLevel level)
{
	arv_debug_level = level;
	arv_debug_checked = TRUE;
}
