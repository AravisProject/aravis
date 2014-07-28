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

#include <arvmisc.h>
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

/**
 * arv_statistic_new: (skip)
 * @n_histograms: number of histograms
 * @n_bins: number of bins for each histogram
 * @bin_step: bin step
 * @offset: offset of the first bin
 * Return value: a new #ArvStatistic structure
 */

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

ArvValue *
arv_value_duplicate (const ArvValue *from)
{
	ArvValue *value = g_new (ArvValue, 1);

	if (from == NULL)
		return NULL;

	*value = *from;

	return value;
}

GType
arv_value_get_type (void)
{
	GType type_id = 0;

	if (type_id == 0)
		type_id = g_boxed_type_register_static ("ArvValue",
							(GBoxedCopyFunc) arv_value_duplicate,
							(GBoxedFreeFunc) arv_value_free);

	return type_id;
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
		from_ptr = ((char *) from) + from_size - 1;
		if (to_size <= from_size) {
			for (i = 0; i < to_size; i++, to_ptr++, from_ptr--)
				*to_ptr = *from_ptr;
		} else {
			for (i = 0; i < from_size; i++, to_ptr++, from_ptr--)
				*to_ptr = *from_ptr;
			memset (((char *) to) + from_size, 0, to_size - from_size);
		}
	} else if (to_endianess == G_BIG_ENDIAN &&
		   from_endianess == G_LITTLE_ENDIAN) {
		to_ptr = ((char *) to) + to_size - 1;
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
			memset (((char *) to) + from_size, 0, to_size - from_size);
		}
	} else if (to_endianess == G_BIG_ENDIAN &&
		   from_endianess == G_BIG_ENDIAN) {
		if (to_size <= from_size)
			memcpy (to, ((char *) from) + from_size - to_size, to_size);
		else {
			memcpy (((char *) to) + to_size - from_size, from, from_size);
			memset (to, 0, to_size - from_size);
		}
	} else
		g_assert_not_reached ();
}

#define ARV_DECOMPRESS_CHUNK 16384

/**
 * arv_decompress:
 * @input_buffer: compressed data
 * @input_buffer: size of compressed data
 * @output_size: (out): placeholder for inflated data
 * Return value: (transfer full): a newly allocated buffer
 **/

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

		arv_debug_misc ("[Decompress] Input ptr = 0x%x - Chunk size = %d - %c",
				stream.next_in, stream.avail_in, *stream.next_in);

		input_size -= stream.avail_in;
		input_buffer = ((char *) input_buffer) + stream.avail_in;

		/* run inflate() on input until output buffer not full */
		do {
			stream.avail_out = ARV_DECOMPRESS_CHUNK;
			stream.next_out = z_stream_output;
			result = inflate(&stream, Z_NO_FLUSH);
			if (result == Z_STREAM_ERROR) {
				arv_warning_misc ("[Decompress] Z_STREAM_ERROR");
				goto CLEANUP;
			}

			switch (result) {
				case Z_NEED_DICT:
					arv_warning_misc ("[Decompress] Z_NEED_DICT");
					goto CLEANUP;
				case Z_DATA_ERROR:
					arv_warning_misc ("[Decompress] Z_DATA_ERROR");
					goto CLEANUP;
				case Z_MEM_ERROR:
					arv_warning_misc ("[Decompress] Z_MEM_ERROR");
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
		arv_warning_misc ("[Decompress] !Z_STREAM_END");
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

#define ARV_MAKE_FOURCC(a,b,c,d)        ((guint32)((a)|(b)<<8|(c)<<16|(d)<<24))

typedef struct {
	ArvPixelFormat pixel_format;
	const char *gst_caps_string;
	const char *name;
	const char *format;
	const char *gst_0_10_caps_string;
	const char *name_0_10;
	int bpp;
	int depth;
	guint32 fourcc;
} ArvGstCapsInfos;

ArvGstCapsInfos arv_gst_caps_infos[] = {
	{
		ARV_PIXEL_FORMAT_MONO_8,
		"video/x-raw, format=(string)GRAY8",
		"video/x-raw", "GRAY8",
		"video/x-raw-gray, bpp=(int)8, depth=(int)8",
		"video/x-raw-gray",	8,	8,	0
	},
	{
		ARV_PIXEL_FORMAT_MONO_10,
		"video/x-raw, format=(string)GRAY16_LE",
		"video/x-raw", 	"GRAY16_LE",
		"video/x-raw-gray, bpp=(int)16, depth=(int)10",
		"video/x-raw-gray",	16,	10,	0
	},
	{
		ARV_PIXEL_FORMAT_MONO_12,
		"video/x-raw, format=(string)GRAY16_LE",
		"video/x-raw",	"GRAY16_LE",
		"video/x-raw-gray, bpp=(int)16, depth=(int)12",
		"video/x-raw-gray",	16,	12,	0
	},
	{
		ARV_PIXEL_FORMAT_MONO_16,
		"video/x-raw, format=(string)GRAY16_LE",
		"video/x-raw",	"GRAY16_LE",
		"video/x-raw-gray, bpp=(int)16, depth=(int)16",
		"video/x-raw-gray",	16,	16,	0
	},
	{
		ARV_PIXEL_FORMAT_BAYER_GR_8,
		"video/x-bayer, format=(string)grbg",
		"video/x-bayer",	"grbg",
		"video/x-raw-bayer, format=(string)grbg, bpp=(int)8, depth=(int)8",
		"video/x-raw-bayer",	8,	8,	ARV_MAKE_FOURCC ('g','r','b','g')
	},
	{
		ARV_PIXEL_FORMAT_BAYER_RG_8,
		"video/x-bayer, format=(string)rggb",
		"video/x-bayer",	"rggb",
		"video/x-raw-bayer, format=(string)rggb, bpp=(int)8, depth=(int)8",
		"video/x-raw-bayer",	8,	8,	ARV_MAKE_FOURCC ('r','g','g','b')
	},
	{
		ARV_PIXEL_FORMAT_BAYER_GB_8,
		"video/x-bayer, format=(string)gbrg",
		"video/x-bayer",	"gbrg",
		"video/x-raw-bayer, format=(string)gbrg, bpp=(int)8, depth=(int)8",
		"video/x-raw-bayer",	8,	8,	ARV_MAKE_FOURCC ('g','b','r','g')
	},
	{
		ARV_PIXEL_FORMAT_BAYER_BG_8,
		"video/x-bayer, format=(string)bggr",
		"video/x-bayer",	"bggr",
		"video/x-raw-bayer, format=(string)bggr, bpp=(int)8, depth=(int)8",
		"video/x-raw-bayer",	8,	8,	ARV_MAKE_FOURCC ('b','g','g','r')
	},

/* Non 8bit bayer formats are not supported by gstreamer bayer plugin.
 * This feature is discussed in bug https://bugzilla.gnome.org/show_bug.cgi?id=693666 .*/

	{
		ARV_PIXEL_FORMAT_YUV_422_PACKED,
		"video/x-raw, format=(string)UYVY",
		"video/x-raw",	"UYVY",
		"video/x-raw-yuv, format=(fourcc)UYVY",
		"video/x-raw-yuv",	0,	0,	ARV_MAKE_FOURCC ('U','Y','V','Y')
	},
	{
		ARV_PIXEL_FORMAT_YUV_422_YUYV_PACKED,
		"video/x-raw, format=(string)YUY2",
		"video/x-raw-yuv, format=(fourcc)YUYU2",
		"video/x-raw-yuv",	0,	0,	ARV_MAKE_FOURCC ('Y','U','Y','2')
	},
	{
		ARV_PIXEL_FORMAT_RGB_8_PACKED,
		"video/x-raw, format=(string)RGB",
		"video/x-raw",	"RGB",
		"video/x-raw-rgb, format=(string)RGB, bpp=(int)24, depth=(int)24",
		"video/x-raw-rgb",	24,	24,	0
	},
	{
		ARV_PIXEL_FORMAT_CUSTOM_YUV_422_YUYV_PACKED,
		"video/x-raw, format=(string)YUY2",
		"video/x-raw",	"YUY2",
		"video/x-raw-yuv, format=(fourcc)YUYU2",
		"video/x-raw-yuv",	0,	0,	ARV_MAKE_FOURCC ('Y','U','Y','2')
	}
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

	if (i == G_N_ELEMENTS (arv_gst_caps_infos)) {
		arv_warning_misc ("[PixelFormat::to_gst_caps_string] 0x%08x not found", pixel_format);
		return NULL;
	}

	arv_log_misc ("[PixelFormat::to_gst_caps_string] 0x%08x -> %s",
		      pixel_format, arv_gst_caps_infos[i].gst_caps_string);

	return arv_gst_caps_infos[i].gst_caps_string;
}

ArvPixelFormat
arv_pixel_format_from_gst_caps (const char *name, const char *format)
{
	unsigned int i;

	g_return_val_if_fail (name != NULL, 0);

	for (i = 0; i < G_N_ELEMENTS (arv_gst_caps_infos); i++) {
		if (strcmp (name, arv_gst_caps_infos[i].name) != 0)
			continue;

		if (strcmp (name, "video/x-raw") == 0 &&
		    strcmp (format, arv_gst_caps_infos[i].format) == 0)
			return arv_gst_caps_infos[i].pixel_format;
		
		if (strcmp (name, "video/x-bayer") == 0 &&
		    strcmp (format, arv_gst_caps_infos[i].format) == 0)
			return arv_gst_caps_infos[i].pixel_format;		
	}

	return 0;
}

const char *
arv_pixel_format_to_gst_0_10_caps_string (ArvPixelFormat pixel_format)
{
	int i;

	for (i = 0; i < G_N_ELEMENTS (arv_gst_caps_infos); i++)
		if (arv_gst_caps_infos[i].pixel_format == pixel_format)
			break;

	if (i == G_N_ELEMENTS (arv_gst_caps_infos)) {
		arv_warning_misc ("[PixelFormat::to_gst_0_10_caps_string] 0x%08x not found", pixel_format);
		return NULL;
	}

	arv_log_misc ("[PixelFormat::to_gst_0_10_caps_string] 0x%08x -> %s",
		      pixel_format, arv_gst_caps_infos[i].gst_0_10_caps_string);

	return arv_gst_caps_infos[i].gst_0_10_caps_string;
}

ArvPixelFormat
arv_pixel_format_from_gst_0_10_caps (const char *name, int bpp, int depth, guint32 fourcc)
{
	unsigned int i;

	g_return_val_if_fail (name != NULL, 0);

	for (i = 0; i < G_N_ELEMENTS (arv_gst_caps_infos); i++) {
		if (strcmp (name, arv_gst_caps_infos[i].name_0_10) != 0)
			continue;

		if (strcmp (name, "video/x-raw-yuv") == 0 &&
		    fourcc == arv_gst_caps_infos[i].fourcc)
			return arv_gst_caps_infos[i].pixel_format;

		if (depth == arv_gst_caps_infos[i].depth &&
		    bpp == arv_gst_caps_infos[i].bpp &&
		    fourcc == arv_gst_caps_infos[i].fourcc)
			return arv_gst_caps_infos[i].pixel_format;
	}

	return 0;
}
