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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author:
 * 	Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <arvstr.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/**
 * SECTION: arvstr
 * @short_description: String utilities
 */

/**
 * arv_str_strip:
 * @str: (allow-none): a string
 * @illegal_chars: illegal characters
 * @replacement_char: replacement character
 *
 * Remove any @illegal_chars from @str, and replace them by @replacement_char if they are not at the end or at the beginning of @str.
 * Several consecutive @illegal_chars are replaced by only one @replacement_char. @illegal_chars at the beginnig or at the end of @str
 * are simply removed.
 *
 * If @replacement_char is '\0', all @illegal_chars are simply removed.
 *
 * Returns: @str
 *
 * Since: 0.4.0
 */

char *
arv_str_strip (char *str, const char *illegal_chars, char replacement_char)
{
	char *last_char = NULL;
	char *ptr = str;
	char *out = str;
	unsigned int n_illegal_chars;
	unsigned int i;
	
	if (str == NULL || illegal_chars == NULL)
		return str;

	n_illegal_chars = strlen (illegal_chars);
	if (n_illegal_chars == 0)
		return str;

	while (*ptr != '\0') {
		gboolean found = FALSE;
		for (i = 0; i < n_illegal_chars && !found; i++)
			found = illegal_chars[i] == *ptr;
		
		if (found) {
			if (last_char == out && replacement_char != '\0') {
				*out = replacement_char;
				out++;
			}
		} else {
			*out = *ptr;
			out++;
			last_char = out;
		}
		ptr++;
	}

	if (last_char != NULL)
		*last_char = '\0';
	else
		*str = '\0';

	return str;
}

/* http://www.ietf.org/rfc/rfc2396.txt - Implementation comes from librsvg (rsvg-base.c). */

gboolean
arv_str_is_uri (const char *str)
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

/**
 * arv_g_string_append_hex_dump:
 * @string: a #GString
 * @data: binary data
 * @size: size of binary data
 *
 * Adds an hexadecimal dump of @data to @string, which consists in lines displaying the data adress, 16 8 bit values in hexadecimal representation, followed by their corresponding ASCII character (replaced by a dot for control ones).
 *
 * Here is an example of the output:
 *
 * 01e0 c8 b7 89 b0 45 fa 3d 9d 8c e9 a7 33 46 85 1f 2c  ....E.=....3F..,
 * 01f0 3f 4c ba 8d 99 f3 ff d0 40 78 73 37 32 e5 4f 9f  ?L......@xs72.O.
 * 0200 d0 d2 f2 ef 5a 2f fc 61 e3 64 36 21              ....Z/.a.d6!    
 */

void
arv_g_string_append_hex_dump (GString *string, const void *data, size_t size)
{
	guint64 i, j, index;

	for (i = 0; i < (size + 15) / 16; i++) {
		for (j = 0; j < 16; j++) {
			index = i * 16 + j;
			if (j == 0)
				g_string_append_printf (string, "%08" G_GINT64_MODIFIER "x", i * 16);
			if (index < size)
				g_string_append_printf (string, " %02x", *((guint8 *) data + index));
			else
				g_string_append (string, "   ");
		}
		for (j = 0; j < 16; j++) {
			index = i * 16 + j;
			if (j == 0)
				g_string_append (string, "  ");
			if (index < size)
				if (*((char *) data + index) >= ' ' &&
				    *((char *) data + index) <  '\x7f')
					g_string_append_c (string, *((char *) data + index));
				else g_string_append_c (string, '.');
			else
				g_string_append_c (string, ' ');
		}
		if (index < size)
			g_string_append (string, "\n");
	}
}
