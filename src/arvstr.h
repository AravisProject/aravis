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

#ifndef ARV_STR_H
#define ARV_STR_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>

G_BEGIN_DECLS

ARV_API char *		arv_str_strip			(char *str, const char *illegal_chars, char replacement_char);

ARV_API gboolean	arv_str_is_uri			(const char *str);
ARV_API char *		arv_str_to_uri			(const char *str);

ARV_API gboolean	arv_str_parse_double		(char **str, double *x);
ARV_API unsigned int	arv_str_parse_double_list	(char **str, unsigned int n_values, double *values);

static inline void
arv_str_skip_spaces (char **str)
{
	while (g_ascii_isspace (**str))
		(*str)++;
}

static inline void
arv_str_skip_char (char **str, char c)
{
	while (**str == c)
		(*str)++;
}

static inline void
arv_str_skip_comma_and_spaces (char **str)
{
	while (g_ascii_isspace (**str) || **str == ',')
		(*str)++;
}

static inline void
arv_str_skip_semicolon_and_spaces (char **str)
{
	while (g_ascii_isspace (**str) || **str == ';')
		(*str)++;
}

static inline void
arv_str_skip_colon_and_spaces (char **str)
{
	while (g_ascii_isspace (**str) || **str == ':')
		(*str)++;
}

ARV_API void		arv_g_string_append_hex_dump	(GString *string, const void *data, size_t size);

G_END_DECLS

#endif
