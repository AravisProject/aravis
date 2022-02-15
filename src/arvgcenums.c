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
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <arvgcenums.h>

static unsigned int
_from_string (const char *string, const char **strings, unsigned int n_strings)
{
	unsigned int i;

	if (string == NULL)
		return 0;

	for (i = 0; i < n_strings; i++)
		if (g_strcmp0 (string, strings[i]) == 0)
			return i;

	return 0;
}

static const char *arv_gc_access_mode_strings[] = {
	"RO",
	"WO",
	"RW"
};

const char *
arv_gc_access_mode_to_string (ArvGcAccessMode value)
{
	return arv_gc_access_mode_strings[CLAMP (value, 0, ARV_GC_ACCESS_MODE_RW)];
}

ArvGcAccessMode
arv_gc_access_mode_from_string (const char *string)
{
	return _from_string (string, arv_gc_access_mode_strings,
			     G_N_ELEMENTS (arv_gc_access_mode_strings));
}
