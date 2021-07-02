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

#ifndef ARV_MISC_PRIVATE_H
#define ARV_MISC_PRIVATE_H

#include <arvmisc.h>

typedef struct _ArvStatistic ArvStatistic;

ArvStatistic *		arv_statistic_new 		(guint n_histograms, guint n_bins, guint bin_step, int offset);
void			arv_statistic_free		(ArvStatistic *statistic);
void 			arv_statistic_reset 		(ArvStatistic *statistic);
gboolean 		arv_statistic_fill 		(ArvStatistic *statistic, guint histogram_id, int value,
							 guint64 counter);
void 			arv_statistic_set_name 		(ArvStatistic *statistic, guint histogram_id, char const *name);

char *			arv_statistic_to_string 	(const ArvStatistic *statistic);

struct _ArvValue {
	GType type;
	union {
		gint64 v_int64;
		double v_double;
	} data;
};

gboolean	arv_parse_genicam_url		(const char *url, gssize url_length,
						 char **scheme, char **authority, char **path,
						 char **query, char **fragment,
						 guint64 *address, guint64 *size);

#if GLIB_CHECK_VERSION(2,68,0)
#define arv_memdup(p,s) g_memdup2(p,s)
#else
#define arv_memdup(p,s) g_memdup(p,(size_t) s)
#endif

#endif
