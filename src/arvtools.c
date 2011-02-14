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
#include <arvdebug.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <zlib.h>

/**
 * SECTION: arvstatistic
 * @short_description: An histogram tool
 */

typedef struct _ArvHistogram ArvHistogram;

struct _ArvHistogram {
	char *		name;

	guint64		and_more;
	guint64	 	and_less;
	guint64	 	last_seen_worst;
	int 	      	worst;
	int 	        best;

	guint64 *	bins;
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
		statistic->histograms[i].bins = g_new (guint64, statistic->n_bins);
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
			g_string_append_printf (string, ";%8Lu", (unsigned long long) statistic->histograms[j].bins[i]);
		}
		g_string_append (string, "\n");
	}

	g_string_append (string, "-------------\n");

	for (j = 0; j < statistic->n_histograms; j++) {
		if (j == 0)
			g_string_append_printf (string, ">=%6d", i * statistic->bin_step + statistic->offset);
		g_string_append_printf (string, ";%8Lu", (unsigned long long) statistic->histograms[j].and_more);
	}
	g_string_append (string, "\n");

	for (j = 0; j < statistic->n_histograms; j++) {
		if (j == 0)
			g_string_append_printf (string, "< %6d", statistic->offset);
		g_string_append_printf (string, ";%8Lu", (unsigned long long) statistic->histograms[j].and_less);
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
		g_string_append_printf (string, ";%8Lu", (unsigned long long) statistic->histograms[j].last_seen_worst);
	}
	g_string_append (string, "\n");

	g_string_append_printf (string, "Counter = %8Lu", (unsigned long long) statistic->counter);

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

/**
 * SECTION: arvvalue
 * @short_description: An int64/double value storage
 */

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
arv_force_g_value_to_double (GValue *value, double v_double)
{
	if (!G_VALUE_HOLDS_DOUBLE (value)) {
		g_value_unset (value);
		g_value_init (value, G_TYPE_DOUBLE);
	}
	g_value_set_double (value, v_double);
}

void
arv_force_g_value_to_string (GValue *value, const char * v_string)
{
	if (!G_VALUE_HOLDS_STRING (value)) {
		g_value_unset (value);
		g_value_init (value, G_TYPE_STRING);
	}
	g_value_set_string (value, v_string);
}

void
arv_copy_memory_with_endianess (void *to, size_t to_size, guint to_endianess,
				void *from, size_t from_size, guint from_endianess)
{
	char *to_ptr;
	char *from_ptr;
	int i;

	g_return_if_fail (to != NULL);
	g_return_if_fail (from != NULL);

	if (to_endianess == G_LITTLE_ENDIAN &&
	    from_endianess == G_BIG_ENDIAN) {
		to_ptr = to;
		from_ptr = from + from_size - 1;
		if (to_size <= from_size) {
			for (i = 0; i < to_size; i++, to_ptr++, from_ptr--)
				*to_ptr = *from_ptr;
		} else {
			for (i = 0; i < from_size; i++, to_ptr++, from_ptr--)
				*to_ptr = *from_ptr;
			memset (to + from_size, 0, to_size - from_size);
		}
	} else if (to_endianess == G_BIG_ENDIAN &&
		   from_endianess == G_LITTLE_ENDIAN) {
		to_ptr = to + to_size - 1;
		from_ptr = from;
		if (to_size <= from_size) {
			for (i = 0; i < to_size; i++, to_ptr--, from_ptr++)
				*to_ptr = *from_ptr;
		} else {
			for (i = 0; i < from_size; i++, to_ptr--, from_ptr++)
				*to_ptr = *from_ptr;
			memset (to, 0, to_size - from_size);
		}
	} else if (to_endianess == G_LITTLE_ENDIAN &&
		   from_endianess == G_LITTLE_ENDIAN) {
		if (to_size <= from_size)
			memcpy (to, from, to_size);
		else {
			memcpy (to, from, from_size);
			memset (to + from_size, 0, to_size - from_size);
		}
	} else if (to_endianess == G_BIG_ENDIAN &&
		   from_endianess == G_BIG_ENDIAN) {
		if (to_size <= from_size)
			memcpy (to, from + from_size - to_size, to_size);
		else {
			memcpy (to + to_size - from_size, from, from_size);
			memset (to, 0, to_size - from_size);
		}
	} else
		g_assert_not_reached ();
}

#define ARV_DECOMPRESS_CHUNK 16384

void *
arv_decompress (void *input_buffer, size_t input_size, size_t *output_size)
{
	z_stream stream;
	GByteArray *output;
	guchar z_stream_output[ARV_DECOMPRESS_CHUNK];
	unsigned have;
	int result;

	g_return_val_if_fail (input_buffer != NULL, NULL);
	g_return_val_if_fail (input_size > 0, NULL);

	/* allocate inflate state */
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	stream.avail_in = 0;
	stream.next_in = Z_NULL;
	stream.data_type = Z_UNKNOWN;

	g_return_val_if_fail (inflateInit2(&stream, -MAX_WBITS) == Z_OK, NULL);

	output = g_byte_array_new ();

	/* decompress until deflate stream ends or end of file */
	do {
		stream.avail_in = MIN (input_size, ARV_DECOMPRESS_CHUNK);
		stream.next_in = input_buffer;

		arv_debug ("decompress", "[Decompress] Input ptr = 0x%x - Chunk size = %d - %c",
			   stream.next_in, stream.avail_in, *stream.next_in);

		input_size -= stream.avail_in;
		input_buffer += stream.avail_in;

		/* run inflate() on input until output buffer not full */
		do {
			stream.avail_out = ARV_DECOMPRESS_CHUNK;
			stream.next_out = z_stream_output;
			result = inflate(&stream, Z_NO_FLUSH);
			if (result == Z_STREAM_ERROR) {
				arv_debug ("decompress", "[Decompress] Z_STREAM_ERROR");
				goto CLEANUP;
			}

			switch (result) {
				case Z_NEED_DICT:
					arv_debug ("decompress", "[Decompress] Z_NEED_DICT");
					goto CLEANUP;
				case Z_DATA_ERROR:
					arv_debug ("decompress", "[Decompress] Z_DATA_ERROR");
					goto CLEANUP;
				case Z_MEM_ERROR:
					arv_debug ("decompress", "[Decompress] Z_MEM_ERROR");
					goto CLEANUP;
			}

			have = ARV_DECOMPRESS_CHUNK - stream.avail_out;
			g_byte_array_append (output, z_stream_output, have);
		} while (stream.avail_out == 0);

		/* done when inflate() says it's done */
	} while (input_size > 0 && result != Z_STREAM_END);

	/* clean up and return */
	inflateEnd(&stream);

	if (result != Z_STREAM_END) {
		arv_debug ("decompress", "[Decompress] !Z_STREAM_END");
		g_byte_array_free (output, TRUE);
		if (output_size != NULL)
			*output_size = 0;
		return NULL;
	}

	if (output_size != NULL)
		*output_size = output->len;

	return g_byte_array_free (output, FALSE);

CLEANUP:

	if (output_size != NULL)
		*output_size = 0;

	g_byte_array_free (output, TRUE);
	inflateEnd(&stream);

	return NULL;
}

/**
 * SECTION: arvgst
 * @short_description: Gstreamer utilities
 */

typedef struct {
	ArvPixelFormat pixel_format;
	const char *gst_caps_string;
} ArvGstCapsInfos;

ArvGstCapsInfos arv_gst_caps_infos[] = {
	{ ARV_PIXEL_FORMAT_MONO_8,		"video/x-raw-yuv, format=(fourcc)Y8  "},
	{ ARV_PIXEL_FORMAT_MONO_10,		"video/x-raw-gray, bpp=(int)16, depth=(int)16"},
	{ ARV_PIXEL_FORMAT_MONO_12,		"video/x-raw-gray, bpp=(int)16, depth=(int)16"},
	{ ARV_PIXEL_FORMAT_MONO_16,		"video/x-raw-gray, bpp=(int)16, depth=(int)16"},
	{ ARV_PIXEL_FORMAT_YUV_422_PACKED,	"video/x-raw-yuv, format=(fourcc)UYVY"},
	{ ARV_PIXEL_FORMAT_YUV_422_YUYV_PACKED,	"video/x-raw-yuv, format=(fourcc)YUY2"},
	{ ARV_PIXEL_FORMAT_RGB_8_PACKED,	"video/x-raw-rgb, bpp=(int)24, depth=(int)24"},
};

/**
 * arv_pixel_format_to_gst_caps_string:
 * @pixel_format: a pixel format
 * Return value: a gstreamer caps string describing the given @pixel_format.
 */

const char *
arv_pixel_format_to_gst_caps_string (ArvPixelFormat pixel_format)
{
	int i;

	for (i = 0; i < G_N_ELEMENTS (arv_gst_caps_infos); i++)
		if (arv_gst_caps_infos[i].pixel_format == pixel_format)
			break;

	if (i == G_N_ELEMENTS (arv_gst_caps_infos))
		return NULL;

	return arv_gst_caps_infos[i].gst_caps_string;
}
