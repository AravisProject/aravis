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

#include <arvmiscprivate.h>
#include <arvdebugprivate.h>
#include <arvversion.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <zlib.h>

#ifdef G_OS_WIN32
	 #include <windows.h>
#endif

/**
 * arv_get_major_version:
 *
 * Returns the major version number of the Aravis library.
 *
 * For example, in Aravis version 0.8.20 this is 0.
 *
 * This function is in the library, so it represents the Aravis library
 * your code is running against. Contrast with the %ARAVIS_MAJOR_VERSION
 * macro, which represents the major version of the Aravis headers you
 * have included when compiling your code.
 *
 * Returns: the major version number of the Aravis library
 *
 * Since: 0.8.20
 */

guint
arv_get_major_version (void)
{
        return ARAVIS_MAJOR_VERSION;
}

/**
 * arv_get_minor_version:
 *
 * Returns the minor version number of the Aravis library.
 *
 * For example, in Aravis version 0.8.20 this is 8.
 *
 * This function is in the library, so it represents the Aravis library
 * your code is running against. Contrast with the %ARAVIS_MINOR_VERSION
 * macro, which represents the minor version of the Aravis headers you
 * have included when compiling your code.
 *
 * Returns: the minor version number of the Aravis library
 *
 * Since: 0.8.20
 */

guint
arv_get_minor_version (void)
{
        return ARAVIS_MINOR_VERSION;
}

/**
 * arv_get_micro_version:
 *
 * Returns the micro version number of the Aravis library.
 *
 * For example, in Aravis version 0.8.20 this is 20.
 *
 * This function is in the library, so it represents the Aravis library
 * your code is running against. Contrast with the %ARAVIS_MICRO_VERSION
 * macro, which represents the micro version of the Aravis headers you
 * have included when compiling your code.
 *
 * Returns: the micro version number of the Aravis library
 *
 * Since: 0.8.20
 */

guint
arv_get_micro_version (void)
{
        return ARAVIS_MICRO_VERSION;
}

typedef struct _ArvHistogramVariable ArvHistogramVariable;

struct _ArvHistogramVariable {
	char *name;

	guint64 counter;

	guint64	and_more;
	guint64	and_less;
        guint64	last_seen_maximum;

	double maximum;
        double minimum;

	guint64 *bins;
};

struct _ArvHistogram {
	guint n_variables;
	guint n_bins;
	double bin_step;
        double offset;

	ArvHistogramVariable *variables;

        gint  ref_count;
};

/**
 * arv_histogram_new: (skip)
 * @n_variables: number of variables
 * @n_bins: number of bins for each histogram
 * @bin_step: bin step
 * @offset: offset of the first bin
 * Return value: a new #ArvHistogram structure
 */

ArvHistogram *
arv_histogram_new (unsigned int n_variables, unsigned n_bins, double bin_step, double offset)
{
	ArvHistogram *histogram;
	unsigned int i;

	g_return_val_if_fail (n_variables > 0, NULL);
	g_return_val_if_fail (n_bins > 0, NULL);
	g_return_val_if_fail (bin_step > 0, NULL);

	histogram = g_new0 (ArvHistogram, 1);

        histogram->ref_count = 1;

	histogram->n_variables = n_variables;
	histogram->n_bins = n_bins;
	histogram->bin_step = bin_step;
	histogram->offset = offset;

	histogram->variables = g_new0 (ArvHistogramVariable, n_variables);

	for (i = 0; i < histogram->n_variables; i++) {
		histogram->variables[i].name = g_strdup_printf ("var%d", i);
		histogram->variables[i].bins = g_new (guint64, histogram->n_bins);
	}

	arv_histogram_reset (histogram);

	return histogram;
}

ArvHistogram *
arv_histogram_ref (ArvHistogram *histogram)
{
        g_return_val_if_fail (histogram != NULL, NULL);

        g_atomic_int_inc (&histogram->ref_count);

        return histogram;
}

void
arv_histogram_unref (ArvHistogram *histogram)
{
	g_return_if_fail (histogram != NULL);

        if (g_atomic_int_dec_and_test (&histogram->ref_count)) {
                guint j;

                if (histogram->variables != NULL) {
                        for (j = 0; j < histogram->n_variables && histogram->variables[j].bins != NULL; j++) {
                                g_free (histogram->variables[j].name);
                                g_free (histogram->variables[j].bins);
                        }
                        g_free (histogram->variables);
                }
                g_free (histogram);
        }
}

GType
arv_histogram_get_type (void)
{
	GType type_id = 0;

	if (type_id == 0)
		type_id = g_pointer_type_register_static ("ArvHistogram");

	return type_id;
}

void
arv_histogram_reset (ArvHistogram *histogram)
{
	ArvHistogramVariable *variable;
	int i, j;

	g_return_if_fail (histogram != NULL);

	for (j = 0; j < histogram->n_variables; j++) {
		variable = &histogram->variables[j];

		variable->minimum = G_MAXDOUBLE;
		variable->maximum = -G_MAXDOUBLE;
		variable->last_seen_maximum = 0;
		variable->and_more = variable->and_less = 0;
                variable->counter = 0;
		for (i = 0; i < histogram->n_bins; i++)
			variable->bins[i] = 0;
	}
}

void
arv_histogram_set_variable_name (ArvHistogram *histogram, unsigned int id, char const *name)
{
	g_return_if_fail (histogram != NULL);
	g_return_if_fail (id < histogram->n_variables);

        g_free (histogram->variables[id].name);
        histogram->variables[id].name = g_strdup (name);
}

gboolean
arv_histogram_fill (ArvHistogram *histogram, guint id, int value)
{
	ArvHistogramVariable *variable;
	unsigned int class;

        g_return_val_if_fail (histogram != NULL, FALSE);
        g_return_val_if_fail (id < histogram->n_variables, FALSE);

	variable = &histogram->variables[id];

	if (variable->minimum > value)
		variable->minimum = value;

	if (variable->maximum < value) {
		variable->maximum = value;
		variable->last_seen_maximum = variable->counter;
	}

	class = (value - histogram->offset) / histogram->bin_step;

	if (value < histogram->offset)
		variable->and_less++;
	else if (class >= histogram->n_bins)
		variable->and_more++;
	else
		variable->bins[class]++;

	variable->counter++;

	return TRUE;
}

char *
arv_histogram_to_string (const ArvHistogram *histogram)
{
	int i, j, bin_max;
	gboolean max_found = FALSE;
	GString *string;

	g_return_val_if_fail (histogram != NULL, NULL);

	string = g_string_new ("");

	bin_max = 0;
	for (i = histogram->n_bins - 1; i > 0 && !max_found; i--) {
		for (j = 0; j < histogram->n_variables && !max_found; j++) {
			if (histogram->variables[j].bins[i] != 0) {
				bin_max = i;
				max_found = TRUE;
			}
		}
	}

	if (bin_max >= histogram->n_bins)
		bin_max = histogram->n_bins - 1;

	for (j = 0; j < histogram->n_variables; j++) {
		if (j == 0)
			g_string_append (string, "    bins    ");
		g_string_append_printf (string, ";%12.12s",
					histogram->variables[j].name != NULL ?
					histogram->variables[j].name :
					"  ----  ");
	}
	g_string_append (string, "\n");

	for (i = 0; i <= bin_max; i++) {
		for (j = 0; j < histogram->n_variables; j++) {
			if (j == 0)
				g_string_append_printf (string, "%12g", (double) i * histogram->bin_step + histogram->offset);
			g_string_append_printf (string, ";%12llu", (unsigned long long) histogram->variables[j].bins[i]);
		}
		g_string_append (string, "\n");
	}

	g_string_append (string, "-------------\n");

	for (j = 0; j < histogram->n_variables; j++) {
		if (j == 0)
			g_string_append_printf (string, ">=%10g", (double) i * histogram->bin_step + histogram->offset);
		g_string_append_printf (string, ";%12llu", (unsigned long long) histogram->variables[j].and_more);
	}
	g_string_append (string, "\n");

	for (j = 0; j < histogram->n_variables; j++) {
		if (j == 0)
			g_string_append_printf (string, "< %10g", histogram->offset);
		g_string_append_printf (string, ";%12" G_GUINT64_FORMAT , histogram->variables[j].and_less);
	}
	g_string_append (string, "\n");

	for (j = 0; j < histogram->n_variables; j++) {
		if (j == 0)
			g_string_append (string, "min         ");
		if (histogram->variables[j].minimum != G_MAXDOUBLE)
			g_string_append_printf (string, "%c%12g", j == 0 ? ':' : ';',
                                                histogram->variables[j].minimum);
		else
			g_string_append_printf (string, "%c%12s", j == 0 ? ':' : ';', "n/a");
	}
	g_string_append (string, "\n");

	for (j = 0; j < histogram->n_variables; j++) {
		if (j == 0)
			g_string_append (string, "max         ");
		if (histogram->variables[j].maximum != -G_MAXDOUBLE)
			g_string_append_printf (string, "%c%12g", j == 0 ? ':' : ';',
                                                histogram->variables[j].maximum);
		else
			g_string_append_printf (string, "%c%12s", j == 0 ? ':' : ';', "n/a");
	}
	g_string_append (string, "\n");

	for (j = 0; j < histogram->n_variables; j++) {
		if (j == 0)
			g_string_append (string, "last max at ");
		g_string_append_printf (string, "%c%12" G_GUINT64_FORMAT, j == 0 ? ':' : ';',
                                        histogram->variables[j].last_seen_maximum);
	}
	g_string_append (string, "\n");

	for (j = 0; j < histogram->n_variables; j++) {
		if (j == 0)
			g_string_append (string, "counter     ");
		g_string_append_printf (string, ":%12llu", (unsigned long long) histogram->variables[j].counter);
	}

        return arv_g_string_free_and_steal(string);
}

ArvValue *
arv_value_new_double (double v_double)
{
	ArvValue *value = g_new (ArvValue, 1);
	value->type = G_TYPE_DOUBLE;
	value->data.v_double = v_double;

	return value;
}

ArvValue *
arv_value_new_int64 (gint64 v_int64)
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

static ArvValue *
arv_value_duplicate (const ArvValue *from)
{
	ArvValue *value;

	if (from == NULL)
		return NULL;

	value = g_new (ArvValue, 1);
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
arv_copy_memory_with_endianness (void *to, size_t to_size, guint to_endianness,
				void *from, size_t from_size, guint from_endianness)
{
	char *to_ptr;
	char *from_ptr;
	int i;

	g_return_if_fail (to != NULL);
	g_return_if_fail (from != NULL);

	if (to_endianness == G_LITTLE_ENDIAN &&
	    from_endianness == G_BIG_ENDIAN) {
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
	} else if (to_endianness == G_BIG_ENDIAN &&
		   from_endianness == G_LITTLE_ENDIAN) {
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
	} else if (to_endianness == G_LITTLE_ENDIAN &&
		   from_endianness == G_LITTLE_ENDIAN) {
		if (to_size <= from_size)
			memcpy (to, from, to_size);
		else {
			memcpy (to, from, from_size);
			memset (((char *) to) + from_size, 0, to_size - from_size);
		}
	} else if (to_endianness == G_BIG_ENDIAN &&
		   from_endianness == G_BIG_ENDIAN) {
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

		arv_info_misc ("[Decompress] Input ptr = 0x%p - Chunk size = %d - %c",
				(void *) stream.next_in, stream.avail_in, *stream.next_in);

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
		"video/x-raw", 		"GRAY8",
		"video/x-raw-gray, bpp=(int)8, depth=(int)8",
		"video/x-raw-gray",	8,	8,	0
	},
	{
		ARV_PIXEL_FORMAT_MONO_16,
		"video/x-raw, format=(string)GRAY16_LE",
		"video/x-raw",		"GRAY16_LE",
		"video/x-raw-gray, bpp=(int)16, depth=(int)16",
		"video/x-raw-gray",	16,	16,	0
	},
	{
		ARV_PIXEL_FORMAT_MONO_12,
		"video/x-raw, format=(string)GRAY16_LE",
		"video/x-raw",		"GRAY16_LE",
		"video/x-raw-gray, bpp=(int)16, depth=(int)12",
		"video/x-raw-gray",	16,	12,	0
	},
	{
		ARV_PIXEL_FORMAT_MONO_12_PACKED,
		"video/x-raw, format=(string)GRAY16_LE",
		"video/x-raw",		"GRAY16_LE",
		"video/x-raw-gray, bpp=(int)12, depth=(int)12",
		"video/x-raw-gray",	12,	12,	0
	},
	{
		ARV_PIXEL_FORMAT_MONO_14,
		"video/x-raw, format=(string)GRAY16_LE",
		"video/x-raw",		"GRAY16_LE",
		"video/x-raw-gray, bpp=(int)16, depth=(int)14",
		"video/x-raw-gray",	16,	14,	0
	},
	{
		ARV_PIXEL_FORMAT_MONO_10,
		"video/x-raw, format=(string)GRAY16_LE",
		"video/x-raw", 		"GRAY16_LE",
		"video/x-raw-gray, bpp=(int)16, depth=(int)10",
		"video/x-raw-gray",	16,	10,	0
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
 * This feature is discussed in bug https://gitlab.freedesktop.org/gstreamer/gst-plugins-bad/-/issues/86 .*/

	{
		ARV_PIXEL_FORMAT_YUV_422_PACKED,
		"video/x-raw, format=(string)UYVY",
		"video/x-raw",		"UYVY",
		"video/x-raw-yuv, format=(fourcc)UYVY",
		"video/x-raw-yuv",	0,	0,	ARV_MAKE_FOURCC ('U','Y','V','Y')
	},
	{
		ARV_PIXEL_FORMAT_YUV_422_YUYV_PACKED,
		"video/x-raw, format=(string)YUY2",
		"video/x-raw", 		"YUY2",
		"video/x-raw-yuv, format=(fourcc)YUYU2",
		"video/x-raw-yuv",	0,	0,	ARV_MAKE_FOURCC ('Y','U','Y','2')
	},
	{
		ARV_PIXEL_FORMAT_RGB_8_PACKED,
		"video/x-raw, format=(string)RGB",
		"video/x-raw",		"RGB",
		"video/x-raw-rgb, format=(string)RGB, bpp=(int)24, depth=(int)8",
		"video/x-raw-rgb",	24,	24,	0
	},
	{
		ARV_PIXEL_FORMAT_RGBA_8_PACKED,
		"video/x-raw, format=(string)RGBA",
		"video/x-raw",		"RGBA",
		"video/x-raw-rgba, format=(string)RGBA, bpp=(int)32, depth=(int)8",
		"video/x-raw-rgba",	32,	8,	0
	},
	{
		ARV_PIXEL_FORMAT_CUSTOM_YUV_422_YUYV_PACKED,
		"video/x-raw, format=(string)YUY2",
		"video/x-raw",		"YUY2",
		"video/x-raw-yuv, format=(fourcc)YUYU2",
		"video/x-raw-yuv",	0,	0,	ARV_MAKE_FOURCC ('Y','U','Y','2')
	},

	{
		ARV_PIXEL_FORMAT_COORD3D_A_8,
		"video/x-raw, format=(string)GRAY8",
		"video/x-raw", 		"GRAY8",
		"video/x-raw-gray, bpp=(int)8, depth=(int)8",
		"video/x-raw-gray",	8,	8,	0
	},
	{
		ARV_PIXEL_FORMAT_COORD3D_B_8,
		"video/x-raw, format=(string)GRAY8",
		"video/x-raw", 		"GRAY8",
		"video/x-raw-gray, bpp=(int)8, depth=(int)8",
		"video/x-raw-gray",	8,	8,	0
	},
	{
		ARV_PIXEL_FORMAT_COORD3D_C_8,
		"video/x-raw, format=(string)GRAY8",
		"video/x-raw", 		"GRAY8",
		"video/x-raw-gray, bpp=(int)8, depth=(int)8",
		"video/x-raw-gray",	8,	8,	0
	},
	{
		ARV_PIXEL_FORMAT_COORD3D_A_16,
		"video/x-raw, format=(string)GRAY16_LE",
		"video/x-raw",		"GRAY16_LE",
		"video/x-raw-gray, bpp=(int)16, depth=(int)16",
		"video/x-raw-gray",	16,	16,	0
	},
	{
		ARV_PIXEL_FORMAT_COORD3D_B_16,
		"video/x-raw, format=(string)GRAY16_LE",
		"video/x-raw",		"GRAY16_LE",
		"video/x-raw-gray, bpp=(int)16, depth=(int)16",
		"video/x-raw-gray",	16,	16,	0
	},
	{
		ARV_PIXEL_FORMAT_COORD3D_C_16,
		"video/x-raw, format=(string)GRAY16_LE",
		"video/x-raw",		"GRAY16_LE",
		"video/x-raw-gray, bpp=(int)16, depth=(int)16",
		"video/x-raw-gray",	16,	16,	0
	},
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

	arv_debug_misc ("[PixelFormat::to_gst_caps_string] 0x%08x -> %s",
		      pixel_format, arv_gst_caps_infos[i].gst_caps_string);

	return arv_gst_caps_infos[i].gst_caps_string;
}

ArvPixelFormat
arv_pixel_format_from_gst_caps (const char *name, const char *format, int bpp, int depth)
{
	unsigned int i;

	g_return_val_if_fail (name != NULL, 0);

	for (i = 0; i < G_N_ELEMENTS (arv_gst_caps_infos); i++) {
		if (strcmp (name, arv_gst_caps_infos[i].name) != 0 ||
		    (depth > 0 && depth != arv_gst_caps_infos[i].depth) ||
		    (bpp > 0 && bpp != arv_gst_caps_infos[i].bpp))
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

	arv_debug_misc ("[PixelFormat::to_gst_0_10_caps_string] 0x%08x -> %s",
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
		    (fourcc <= 0 || fourcc == arv_gst_caps_infos[i].fourcc))
			return arv_gst_caps_infos[i].pixel_format;

		if ((depth <= 0 || depth == arv_gst_caps_infos[i].depth) &&
		    (bpp <= 0 || bpp == arv_gst_caps_infos[i].bpp) &&
		    fourcc == arv_gst_caps_infos[i].fourcc)
			return arv_gst_caps_infos[i].pixel_format;
	}

	return 0;
}

static struct {
	const char *vendor;
	const char *alias;
} vendor_aliases[] = {
	{ "The Imaging Source Europe GmbH",		"TIS"},
	{ "Point Grey Research",			"PointGrey"},
	{ "Lucid Vision Labs",				"LucidVision"},
	{ "New-Imaging-Technologies",			"NIT"}
};

/**
 * arv_vendor_alias_lookup:
 * @vendor: a vendor string
 *
 * Returns: vendor alias string if found, or @vendor if not found.
 *
 * Since: 0.6.0
 */

const char *
arv_vendor_alias_lookup	(const char *vendor)
{
	int i;

	if (vendor == NULL)
		return NULL;

	for (i = 0; i < G_N_ELEMENTS (vendor_aliases); i++)
		if (g_strcmp0 (vendor_aliases[i].vendor, vendor) == 0)
			return vendor_aliases[i].alias;

	return vendor;
}

/**
 * arv_parse_genicam_url:
 * @url: a genicam data URL
 * @url_length: length of the URL string, -1 if NULL terminated
 * @scheme: (allow-none): placeholder for scheme
 * @authority: (allow-none): placeholder for authority
 * @path: (allow-none): placeholder for path
 * @query: (allow-none): placeholder for query
 * @fragment: (allow-none): placeholder for fragment
 * @address: (allow-none): placeholder for data adress in case of local URL
 * @size: (allow-none): placeholder for data size in case of local URL
 *
 * Parse the Genicam data URL. The URL should at least contain a scheme and a path. If scheme is 'local', the path
 * should also define the Genicam data address and size in the device memory.
 *
 * The placeholder are at least set to %NULL or 0 if they are valid addresses, and set to the corresponding URL part if
 * found in the string. The returned strings must be freed using g_free().
 *
 * Returns: %TRUE if the URL was successfully parsed.
 */

gboolean
arv_parse_genicam_url (const char *url, gssize url_length,
		       char **scheme, char **authority, char **path, char **query, char **fragment,
		       guint64 *address, guint64 *size)
{
	GRegex *regex = NULL;
	GRegex *local_regex = NULL;
	GStrv tokens = NULL;
	GStrv local_tokens = NULL;
	char *l_scheme = NULL;
	char *l_authority = NULL;
	char *l_path = NULL;
	char *l_query = NULL;
	char *l_fragment = NULL;

	if (scheme != NULL)
		*scheme = NULL;
	if (authority != NULL)
		*authority = NULL;
	if (path != NULL)
		*path = NULL;
	if (query != NULL)
		*query = NULL;
	if (fragment != NULL)
		*fragment = NULL;
	if (address != NULL)
		*address = 0;
	if (size != NULL)
		*size = 0;

	g_return_val_if_fail (url != NULL, FALSE);

	/* https://tools.ietf.org/html/rfc3986#appendix-B */
	regex = g_regex_new ("^(([^:\\/?#]+):)?(\\/\\/([^\\/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?",G_REGEX_CASELESS, 0, NULL);
	if (regex == NULL)
		return FALSE;

	tokens = g_regex_split_full (regex, url, url_length, 0, 0, 10, NULL);
	g_clear_pointer (&regex, g_regex_unref);

	if (g_strv_length (tokens) < 6 || tokens[5][0] == '\0') {
		g_strfreev (tokens);
		return FALSE;
	}

	l_scheme = tokens[2][0] != '\0' ? tokens[2] : NULL;
	l_authority = tokens[4][0] != '\0' ? tokens[4] : NULL;

	if (g_ascii_strcasecmp (l_scheme, "local") == 0) {
	local_regex = g_regex_new ("(?:\\s*)?(.+);(?:\\s*)?(?:0x)?([0-9:a-f]*);(?:\\s*)?(?:0x)?([0-9:a-f]*)",
                                           G_REGEX_CASELESS, 0, NULL);

		if (local_regex == NULL) {
			g_strfreev (tokens);
			return FALSE;
		}

		local_tokens = g_regex_split (local_regex, tokens[5], 0);
		g_clear_pointer (&local_regex, g_regex_unref);

		if (g_strv_length (local_tokens) < 4) {
			g_strfreev (tokens);
			g_strfreev (local_tokens);
			return FALSE;
		}

		l_path = local_tokens[1];

		if (address != NULL)
			*address = g_ascii_strtoll (local_tokens[2], NULL, 16);
		if (size != NULL)
			*size = g_ascii_strtoll (local_tokens[3], NULL, 16);
	} else {
		l_path = tokens[5];
	}

	if (tokens[6] != NULL && tokens[7] != NULL) {
		l_query = tokens[7][0] != '\0' ? tokens[7] : NULL;

		if (tokens[8] != NULL && tokens[9] != NULL)
			l_fragment = tokens[9][0] != '\0' ? tokens[9] : NULL;
	}

	if (scheme != NULL)
		*scheme = g_strdup( l_scheme);
	if (authority != NULL)
		*authority = g_strdup( l_authority);
	if (path != NULL)
		*path = g_strdup( l_path);
	if (query != NULL)
		*query = g_strdup( l_query);
	if (fragment != NULL)
		*fragment = g_strdup( l_fragment);

	g_strfreev (tokens);
	g_strfreev (local_tokens);

	return TRUE;
}


gint64 arv_monotonic_time_us (void)
{
	 #ifdef G_OS_WIN32
		  static LARGE_INTEGER freq = { .QuadPart = 0 };
		  LARGE_INTEGER t;

		  if (freq.QuadPart == 0) { QueryPerformanceFrequency(&freq); }
		  QueryPerformanceCounter(&t);
		  return (t.QuadPart*1000000) / freq.QuadPart;
	 #else
		  return g_get_monotonic_time();
	 #endif
}

GRegex *
arv_regex_new_from_glob_pattern (const char *glob, gboolean caseless)
{
        GRegex *regex;
	GString *regex_pattern;
	const char *iter;
	char **globs;
	gunichar character;
	unsigned int i;

	g_return_val_if_fail (g_utf8_validate (glob, -1, NULL), NULL);

	regex_pattern = g_string_new ("");

	globs = g_strsplit (glob, "|", -1);

	for (i = 0; globs[i] != NULL; i++) {
		/* Ignore empty strings */
		if (globs[i][0] == '\0')
		    continue;

		if (i > 0)
			g_string_append (regex_pattern, "|^");
		else
			g_string_append (regex_pattern, "^");

		iter = g_strstrip (globs[i]);
		while (iter != NULL && *iter != '\0') {
			character = g_utf8_get_char (iter);
			switch (character) {
				case '\\':
					g_string_append (regex_pattern, "\\\\");
					break;
				case '^':
					g_string_append (regex_pattern, "\\^");
					break;
				case '$':
					g_string_append (regex_pattern, "\\$");
					break;
				case '.':
					g_string_append (regex_pattern, "\\.");
					break;
				case '[':
					g_string_append (regex_pattern, "\\[");
					break;
				case '|':
					g_string_append (regex_pattern, "\\|");
					break;
				case '(':
					g_string_append (regex_pattern, "\\(");
					break;
				case ')':
					g_string_append (regex_pattern, "\\)");
					break;
				case '?':
					g_string_append (regex_pattern, ".");
					break;
				case '*':
					g_string_append (regex_pattern, ".*");
					break;
				case '+':
					g_string_append (regex_pattern, "\\+");
					break;
				case '{':
					g_string_append (regex_pattern, "\\{");
					break;
				default:
					g_string_append_unichar (regex_pattern, character);
					break;
			}
			iter = g_utf8_find_next_char (iter, NULL);
		}

		g_string_append (regex_pattern, "$");
	}

	g_strfreev (globs);

	arv_debug_misc ("Regex '%s' created from glob '%s'", regex_pattern->str, glob);

	regex = g_regex_new (regex_pattern->str, G_REGEX_OPTIMIZE | (caseless ? G_REGEX_CASELESS : 0), 0, NULL);
	g_string_free (regex_pattern, TRUE);

	return regex;
}

char *
arv_g_string_free_and_steal (GString *string)
{
#if GLIB_CHECK_VERSION(2,75,4)
        return g_string_free_and_steal(string);
#else
        char *buffer = string->str;

        g_string_free (string, FALSE);

        return buffer;
#endif
}
