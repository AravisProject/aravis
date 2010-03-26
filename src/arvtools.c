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

#include <arvtools.h>
#include <string.h>
#include <math.h>

typedef struct _ArvHistogram ArvHistogram;

struct _ArvHistogram {
	char *		name;

	gint64 		and_more;
	gint64	 	and_less;
	gint64	 	last_seen_worst;
	int 	      	worst;
	int 	        best;

	gint64 *	bins;
};

struct _ArvStatistic {
	guint n_histograms;
	guint n_bins;
	guint bin_step;
	int offset;

	guint64 counter;

	ArvHistogram *histograms;
};

static void
_arv_statistic_free (ArvStatistic *statistic)
{
	guint j;

	if (statistic == NULL)
		return;

	if (statistic->histograms != NULL) {
		for (j = 0; j < statistic->n_histograms && statistic->histograms[j].bins != NULL; j++) {
			if (statistic->histograms[j].name != NULL)
				g_free (statistic->histograms[j].name);
			g_free (statistic->histograms[j].bins);
		}
		g_free (statistic->histograms);
	}

	g_free (statistic);
}

ArvStatistic *
arv_statistic_new (unsigned int n_histograms, unsigned n_bins, unsigned int bin_step, int offset)
{
	ArvStatistic *statistic;
	unsigned int i;

	g_return_val_if_fail (n_histograms > 0, NULL);
	g_return_val_if_fail (n_bins > 0, NULL);
	g_return_val_if_fail (bin_step > 0, NULL);

	statistic = g_new (ArvStatistic, 1);
	g_return_val_if_fail (statistic != NULL, NULL);

	statistic->n_histograms = n_histograms;
	statistic->n_bins = n_bins;
	statistic->bin_step = bin_step;
	statistic->offset = offset;

	statistic->histograms = g_new (ArvHistogram, n_histograms);
	if (statistic->histograms == NULL) {
		_arv_statistic_free (statistic);
		g_warning ("[ArvStatistic::new] failed to allocate histogram memory");
		return NULL;
	}

	for (i = 0; i < statistic->n_histograms; i++) {
		statistic->histograms[i].name = NULL;
		statistic->histograms[i].bins = g_new (gint64, statistic->n_bins);
		if (statistic->histograms[i].bins == NULL) {
			arv_statistic_free (statistic);
			g_warning ("[TolmStatistic::new] failed to allocate bin memory");
			return NULL;
		}
	}

	arv_statistic_reset (statistic);

	return statistic;
}

void
arv_statistic_free (ArvStatistic *statistic)
{
	g_return_if_fail (statistic != NULL);

	_arv_statistic_free (statistic);
}

void
arv_statistic_reset (ArvStatistic *statistic)
{
	ArvHistogram *histogram;
	int i, j;

	g_return_if_fail (statistic != NULL);

	statistic->counter = 0;

	for (j = 0; j < statistic->n_histograms; j++) {
		histogram = &statistic->histograms[j];

		histogram->last_seen_worst = 0;
		histogram->best = 0x7fffffff;
		histogram->worst = 0x80000000;
		histogram->and_more = histogram->and_less = 0;
		for (i = 0; i < statistic->n_bins; i++)
			histogram->bins[i] = 0;
	}
}

void
arv_statistic_set_name (ArvStatistic *statistic, unsigned int histogram_id, char const *name)
{
	ArvHistogram *histogram;
	size_t length;

	g_return_if_fail (statistic != NULL);
	g_return_if_fail (histogram_id < statistic->n_histograms);

	histogram = &statistic->histograms[histogram_id];

	if (histogram->name != NULL) {
		g_free (histogram->name);
		histogram->name = NULL;
	}

	if (name == NULL)
		return;

	length = strlen (name);
	if (length < 1)
		return;

	histogram->name = g_malloc (length + 1);
	if (histogram->name == NULL)
		return;

	memcpy (histogram->name, name, length + 1);
}

gboolean
arv_statistic_fill (ArvStatistic *statistic, guint histogram_id, int value, guint64 counter)
{
	ArvHistogram *histogram;
	unsigned int class;

	if (statistic == NULL)
		return FALSE;
	if (histogram_id >= statistic->n_histograms)
		return FALSE;

	statistic->counter = counter;

	histogram = &statistic->histograms[histogram_id];

	if (histogram->best > value)
		histogram->best = value;

	if (histogram->worst < value) {
		histogram->worst = value;
		histogram->last_seen_worst = counter;
	}

	class = (value - statistic->offset) / statistic->bin_step;

	if (value < statistic->offset)
		histogram->and_less++;
	else if (class >= statistic->n_bins)
		histogram->and_more++;
	else
		histogram->bins[class]++;

	return TRUE;
}

char *
arv_statistic_to_string (const ArvStatistic *statistic)
{
	int i, j, bin_max;
	gboolean max_found = FALSE;
	GString *string;
	char *str;

	g_return_val_if_fail (statistic != NULL, NULL);

	string = g_string_new ("");

	bin_max = 0;
	for (i = statistic->n_bins - 1; i > 0 && !max_found; i--) {
		for (j = 0; j < statistic->n_histograms && !max_found; j++) {
			if (statistic->histograms[j].bins[i] != 0) {
				bin_max = i;
				max_found = TRUE;
			}
		}
	}

	if (bin_max >= statistic->n_bins)
		bin_max = statistic->n_bins - 1;

	for (j = 0; j < statistic->n_histograms; j++) {
		if (j == 0)
			g_string_append (string, "  bins  ");
		g_string_append_printf (string, ";%8.8s",
					statistic->histograms[j].name != NULL ?
					statistic->histograms[j].name :
					"  ----  ");
	}
	g_string_append (string, "\n");

	for (i = 0; i <= bin_max; i++) {
		for (j = 0; j < statistic->n_histograms; j++) {
			if (j == 0)
				g_string_append_printf (string, "%8d", i * statistic->bin_step + statistic->offset);
			g_string_append_printf (string, ";%8Lu", statistic->histograms[j].bins[i]);
		}
		g_string_append (string, "\n");
	}

	g_string_append (string, "-------------\n");

	for (j = 0; j < statistic->n_histograms; j++) {
		if (j == 0)
			g_string_append_printf (string, ">=%6d", i * statistic->bin_step + statistic->offset);
		g_string_append_printf (string, ";%8Ld", statistic->histograms[j].and_more);
	}
	g_string_append (string, "\n");

	for (j = 0; j < statistic->n_histograms; j++) {
		if (j == 0)
			g_string_append_printf (string, "< %6d", statistic->offset);
		g_string_append_printf (string, ";%8Ld", statistic->histograms[j].and_less);
	}
	g_string_append (string, "\n");

	for (j = 0; j < statistic->n_histograms; j++) {
		if (j == 0)
			g_string_append (string, "min     ");
		if (statistic->histograms[j].best != 0x7fffffff)
			g_string_append_printf (string, ";%8d", statistic->histograms[j].best);
		else
			g_string_append_printf (string, ";%8s", "n/a");
	}
	g_string_append (string, "\n");

	for (j = 0; j < statistic->n_histograms; j++) {
		if (j == 0)
			g_string_append (string, "max     ");
		if (statistic->histograms[j].worst != 0x80000000)
			g_string_append_printf (string, ";%8d", statistic->histograms[j].worst);
		else
			g_string_append_printf (string, ";%8s", "n/a");
	}
	g_string_append (string, "\n");

	for (j = 0; j < statistic->n_histograms; j++) {
		if (j == 0)
			g_string_append (string, "last max\nat:     ");
		g_string_append_printf (string, ";%8Ld", statistic->histograms[j].last_seen_worst);
	}
	g_string_append (string, "\n");

	g_string_append_printf (string, "Counter = %8Ld", statistic->counter);

	str = string->str;
	g_string_free (string, FALSE);

	return str;
}

/* ArvStr implementation */

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

/* ArvValue implementation */

ArvValue *
arv_value_new_double (double v_double)
{
	ArvValue *value = g_new (ArvValue, 1);
	value->type = G_TYPE_DOUBLE;
	value->data.v_double = v_double;

	return value;
}

ArvValue *
arv_value_new_int64 (double v_int64)
{
	ArvValue *value = g_new (ArvValue, 1);
	value->type = G_TYPE_INT64;
	value->data.v_int64 = v_int64;

	return value;
}

void
arv_value_free (ArvValue *value)
{
	g_free (value);
}

void
arv_value_copy (ArvValue *to, const ArvValue *from)
{
	*to = *from;
}

void
arv_value_set_int64 (ArvValue *value, gint64 v_int64)
{
	value->type = G_TYPE_INT64;
	value->data.v_int64 = v_int64;
}

void
arv_value_set_double (ArvValue *value, double v_double)
{
	value->type = G_TYPE_DOUBLE;
	value->data.v_double = v_double;
}

gint64
arv_value_get_int64 (ArvValue *value)
{
	if (value->type == G_TYPE_INT64)
		return value->data.v_int64;
	else
		return (gint64) value->data.v_double;
}

double
arv_value_get_double (ArvValue *value)
{
	if (value->type == G_TYPE_INT64)
		return (double) value->data.v_int64;
	else
		return value->data.v_double;
}

gboolean
arv_value_holds_int64 (ArvValue *value)
{
	return value->type == G_TYPE_INT64;
}

double
arv_value_holds_double (ArvValue *value)
{
	return value->type == G_TYPE_DOUBLE;
}

/* GValue utilities */

GValue *
arv_new_g_value_int64 (gint64 v_int64)
{
	GValue *value = g_new0 (GValue, 1);

	g_value_init (value, G_TYPE_INT64);
	g_value_set_int64 (value, v_int64);

	return value;
}

GValue *
arv_new_g_value_string (const char *v_string)
{
	GValue *value = g_new0 (GValue, 1);

	g_value_init (value, G_TYPE_STRING);
	g_value_set_string (value, v_string);

	return value;
}

void
arv_free_g_value (GValue *value)
{
	g_return_if_fail (G_IS_VALUE (value));

	g_value_unset (value);
	g_free (value);
}

void
arv_force_g_value_to_int64 (GValue *value, gint64 v_int64)
{
	if (!G_VALUE_HOLDS_INT64 (value)) {
		g_value_unset (value);
		g_value_init (value, G_TYPE_INT64);
	}
	g_value_set_int64 (value, v_int64);
}

void
arv_force_g_value_to_string (GValue *value, const char * v_string)
{
	if (!G_VALUE_HOLDS_INT64 (value)) {
		g_value_unset (value);
		g_value_init (value, G_TYPE_STRING);
	}
	g_value_set_string (value, v_string);
}
