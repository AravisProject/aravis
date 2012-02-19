/* Lasem
 *
 * Copyright © 2009 Emmanuel Pacaud
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
 * Author:
 * 	Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <arvstr.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* http://www.ietf.org/rfc/rfc2396.txt - Implementation comes from librsvg (rsvg-base.c). */

gboolean
arv_str_is_uri  (const char *str)
{
	char const *p;

	if (str == NULL)
		return FALSE;

	if (strlen (str) < 4)
		return FALSE;

	if (   (str[0] < 'a' || str[0] > 'z')
	       && (str[0] < 'A' || str[0] > 'Z'))
		return FALSE;

	for (p = &str[1];
	     (*p >= 'a' && *p <= 'z')
	     || (*p >= 'A' && *p <= 'Z')
	     || (*p >= '0' && *p <= '9')
	     || *p == '+'
	     || *p == '-'
	     || *p == '.';
	     p++);

	if (strlen (p) < 3)
		return FALSE;

	return (p[0] == ':' && p[1] == '/' && p[2] == '/');
}

char *
arv_str_to_uri (const char *str)
{
	gchar *current_dir;
	gchar *absolute_filename;
	gchar *uri;

	if (str == NULL)
		return NULL;

	if (arv_str_is_uri (str))
		return g_strdup (str);

	if (g_path_is_absolute (str))
		return g_filename_to_uri (str, NULL, NULL);

	current_dir = g_get_current_dir ();
	absolute_filename = g_build_filename (current_dir, str, NULL);
	uri = g_filename_to_uri (absolute_filename, NULL, NULL);
	g_free (absolute_filename);
	g_free (current_dir);

	return uri;
}

gboolean
arv_str_parse_double (char **str, double *x)
{
	char *end, *c;
	gboolean integer_part = FALSE;
	gboolean fractional_part = FALSE;
	gboolean exponent_part = FALSE;
	double mantissa = 0.0;
	double exponent =0.0;
	double divisor;
	gboolean mantissa_sign = 1.0;
	gboolean exponent_sign = 1.0;

	c = *str;

	if (*c == '-') {
		mantissa_sign = -1.0;
		c++;
	} else if (*c == '+')
		c++;

	if (*c >= '0' && *c <= '9') {
		integer_part = TRUE;
		mantissa = *c - '0';
		c++;

		while (*c >= '0' && *c <= '9') {
			mantissa = mantissa * 10.0 + *c - '0';
			c++;
		}
	}


	if (*c == '.')
		c++;
	else if (!integer_part)
		return FALSE;

	if (*c >= '0' && *c <= '9') {
		fractional_part = TRUE;
		mantissa += (*c - '0') * 0.1;
		divisor = 0.01;
		c++;

		while (*c >= '0' && *c <= '9') {
			mantissa += (*c - '0') * divisor;
			divisor *= 0.1;
			c++;
		}
	}

	if (!fractional_part && !integer_part)
		return FALSE;

	end = c;

	if (*c == 'E' || *c == 'e') {
		c++;

		if (*c == '-') {
			exponent_sign = -1.0;
			c++;
		} else if (*c == '+')
			c++;

		if (*c >= '0' && *c <= '9') {
			exponent_part = TRUE;
			exponent = *c - '0';
			c++;

			while (*c >= '0' && *c <= '9') {
				exponent = exponent * 10.0 + *c - '0';
				c++;
			}
		}
	}

	if (exponent_part) {
		end = c;
		*x = mantissa_sign * mantissa * pow (10.0, exponent_sign * exponent);
	} else
		*x = mantissa_sign * mantissa;

	*str = end;

	return TRUE;
}

unsigned int
arv_str_parse_double_list (char **str, unsigned int n_values, double *values)
{
	char *ptr = *str;
	unsigned int i;

	arv_str_skip_comma_and_spaces (str);

	for (i = 0; i < n_values; i++) {
		if (!arv_str_parse_double (str, &values[i])) {
			*str = ptr;
			return i;
		}
		arv_str_skip_comma_and_spaces (str);
	}

	return i;
}

