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

/**
 * SECTION: arvfakecamera
 * @short_description: Fake camera internals
 *
 * #ArvFakeCamera is a class that simulate a real camera, which provides
 * methods for the implementation of #ArvFakeDevice and #ArvFakeStream.
 *
 * arv-fake-gv-camera is a GV camera simulator based on this class.
 */

#include <arvfakecamera.h>
#include <arvversion.h>
#include <arvgc.h>
#include <arvgcregisternode.h>
#include <arvgvcpprivate.h>
#include <arvbufferprivate.h>
#include <arvdebug.h>
#include <arvmiscprivate.h>
#include <string.h>
#include <math.h>

static char *arv_fake_camera_genicam_filename = NULL;

/*
 * arv_set_fake_camera_genicam_filename:
 * @filename: path to genicam file
 *
 * Sets name of genicam file. This needs to be called prior to
 * instantiation of the fake camera. This function is not thread safe.
 */

void
arv_set_fake_camera_genicam_filename (const char *filename)
{
	g_clear_pointer (&arv_fake_camera_genicam_filename, g_free);
	arv_fake_camera_genicam_filename = g_strdup (filename);
}

static const char *
arv_get_fake_camera_genicam_filename (void)
{
	return arv_fake_camera_genicam_filename;
}

typedef struct {
	void *memory;

	char *genicam_xml;
	size_t genicam_xml_size;

	guint32 frame_id;
	double trigger_frequency;

	GMutex fill_pattern_mutex;

	ArvFakeCameraFillPattern fill_pattern_callback;
	void *fill_pattern_data;
} ArvFakeCameraPrivate;

struct _ArvFakeCamera {
	GObject object;

	ArvFakeCameraPrivate *priv;
};

struct _ArvFakeCameraClass {
	GObjectClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvFakeCamera, arv_fake_camera, G_TYPE_OBJECT, G_ADD_PRIVATE (ArvFakeCamera))

/* ArvFakeCamera implementation */

gboolean
arv_fake_camera_read_memory (ArvFakeCamera *camera, guint32 address, guint32 size, void *buffer)
{
	guint32 read_size;

	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), FALSE);
	g_return_val_if_fail (buffer != NULL, FALSE);
	g_return_val_if_fail (size > 0, FALSE);

	if (address < ARV_FAKE_CAMERA_MEMORY_SIZE) {
		read_size = MIN (address  + size, ARV_FAKE_CAMERA_MEMORY_SIZE) - address;

		memcpy (buffer, ((char *) camera->priv->memory) + address, read_size);

		if (read_size == size)
			return TRUE;

		size = size - read_size;
		address = ARV_FAKE_CAMERA_MEMORY_SIZE;
		buffer = ((char *) buffer) + read_size;
	}

	address -= ARV_FAKE_CAMERA_MEMORY_SIZE;
	read_size = MIN (address + size, camera->priv->genicam_xml_size) - address;

	memcpy (buffer, ((char *) camera->priv->genicam_xml) + address, read_size);
	if (read_size < size)
		memset (((char *) buffer) + read_size, 0, size - read_size);

	return TRUE;
}

gboolean
arv_fake_camera_write_memory (ArvFakeCamera *camera, guint32 address, guint32 size, const void *buffer)
{
	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), FALSE);
	g_return_val_if_fail (address + size < ARV_FAKE_CAMERA_MEMORY_SIZE + camera->priv->genicam_xml_size, FALSE);
	g_return_val_if_fail (buffer != NULL, FALSE);
	g_return_val_if_fail (size > 0, FALSE);

	/* genicam_data are read only */
	if (address + size > ARV_FAKE_CAMERA_MEMORY_SIZE)
		return FALSE;

	memcpy (((char *) camera->priv->memory) + address, buffer, size);

	return TRUE;
}

/**
 * arv_fake_camera_read_register:
 * @camera: a #ArvFakeCamera
 * @address: the register address
 * @value: (out): the register value
 *
 * Return value: true if the read succeeded, false otherwise
 */

gboolean
arv_fake_camera_read_register (ArvFakeCamera *camera, guint32 address, guint32 *value)
{
	gboolean success;
	guint32 be_value = 0;

	g_return_val_if_fail (value != NULL, FALSE);

	success = arv_fake_camera_read_memory (camera, address, sizeof (*value), &be_value);

	*value = GUINT32_FROM_BE (be_value);

	return success;
}

gboolean
arv_fake_camera_write_register (ArvFakeCamera *camera, guint32 address, guint32 value)
{
	guint32 be_value = GUINT32_TO_BE (value);

	return arv_fake_camera_write_memory (camera, address, sizeof (value), &be_value);
}

static guint32
_get_register (ArvFakeCamera *camera, guint32 address)
{
	guint32 value;

	if (address + sizeof (guint32) > ARV_FAKE_CAMERA_MEMORY_SIZE)
		return 0;

	value = *((guint32 *) (((char *)(camera->priv->memory) + address)));

	return GUINT32_FROM_BE (value);
}

size_t
arv_fake_camera_get_payload (ArvFakeCamera *camera)
{
        guint32 width, height, pixel_format;

	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), 0);

	width = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_WIDTH);
	height = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_HEIGHT);
        pixel_format = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_PIXEL_FORMAT);

	return width * height * ARV_PIXEL_FORMAT_BIT_PER_PIXEL(pixel_format)/8;
}

/**
 * arv_fake_camera_get_sleep_time_for_next_frame:
 * @camera: a #ArvFakeCamera
 * @next_timestamp_us: (out) (optional): the timestamp for the next frame in microseconds
 *
 * Return value: the sleep time for the next frame
 */

guint64
arv_fake_camera_get_sleep_time_for_next_frame (ArvFakeCamera *camera, guint64 *next_timestamp_us)
{
	guint64 time_us;
	guint64 sleep_time_us;
	guint64 frame_period_time_us;

	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), 0);

	if (_get_register (camera, ARV_FAKE_CAMERA_REGISTER_TRIGGER_MODE) == 1)
		frame_period_time_us = 1000000L / camera->priv->trigger_frequency;
	else
		frame_period_time_us = (guint64) _get_register (camera, ARV_FAKE_CAMERA_REGISTER_ACQUISITION_FRAME_PERIOD_US);

	if (frame_period_time_us == 0) {
		arv_warning_misc ("Invalid zero frame period, defaulting to 1 second");
		frame_period_time_us = 1000000L;
	}

	time_us = g_get_real_time ();
	sleep_time_us = frame_period_time_us - (time_us % frame_period_time_us);

	if (next_timestamp_us != NULL)
		*next_timestamp_us = time_us + sleep_time_us;

	return sleep_time_us;
}

void
arv_fake_camera_wait_for_next_frame (ArvFakeCamera *camera)
{
	g_return_if_fail (ARV_IS_FAKE_CAMERA (camera));

	g_usleep (arv_fake_camera_get_sleep_time_for_next_frame (camera, NULL));
}

static struct {
	unsigned char r,g,b;
} jet_colormap [] =
  {
   {0  ,   0  , 132},
   {0  ,   0  , 136},
   {0  ,   0  , 140},
   {0  ,   0  , 144},
   {0  ,   0  , 148},
   {0  ,   0  , 152},
   {0  ,   0  , 156},
   {0  ,   0  , 160},
   {0  ,   0  , 164},
   {0  ,   0  , 168},
   {0  ,   0  , 172},
   {0  ,   0  , 176},
   {0  ,   0  , 180},
   {0  ,   0  , 184},
   {0  ,   0  , 188},
   {0  ,   0  , 192},
   {0  ,   0  , 196},
   {0  ,   0  , 200},
   {0  ,   0  , 204},
   {0  ,   0  , 208},
   {0  ,   0  , 212},
   {0  ,   0  , 216},
   {0  ,   0  , 220},
   {0  ,   0  , 224},
   {0  ,   0  , 228},
   {0  ,   0  , 232},
   {0  ,   0  , 236},
   {0  ,   0  , 240},
   {0  ,   0  , 244},
   {0  ,   0  , 248},
   {0  ,   0  , 252},
   {0  ,   0  , 255},
   {0  ,   4  , 255},
   {0  ,   8  , 255},
   {0  ,  12  , 255},
   {0  ,  16  , 255},
   {0  ,  20  , 255},
   {0  ,  24  , 255},
   {0  ,  28  , 255},
   {0  ,  32  , 255},
   {0  ,  36  , 255},
   {0  ,  40  , 255},
   {0  ,  44  , 255},
   {0  ,  48  , 255},
   {0  ,  52  , 255},
   {0  ,  56  , 255},
   {0  ,  60  , 255},
   {0  ,  64  , 255},
   {0  ,  68  , 255},
   {0  ,  72  , 255},
   {0  ,  76  , 255},
   {0  ,  80  , 255},
   {0  ,  84  , 255},
   {0  ,  88  , 255},
   {0  ,  92  , 255},
   {0  ,  96  , 255},
   {0  , 100  , 255},
   {0  , 104  , 255},
   {0  , 108  , 255},
   {0  , 112  , 255},
   {0  , 116  , 255},
   {0  , 120  , 255},
   {0  , 124  , 255},
   {0  , 128  , 255},
   {0  , 132  , 255},
   {0  , 136  , 255},
   {0  , 140  , 255},
   {0  , 144  , 255},
   {0  , 148  , 255},
   {0  , 152  , 255},
   {0  , 156  , 255},
   {0  , 160  , 255},
   {0  , 164  , 255},
   {0  , 168  , 255},
   {0  , 172  , 255},
   {0  , 176  , 255},
   {0  , 180  , 255},
   {0  , 184  , 255},
   {0  , 188  , 255},
   {0  , 192  , 255},
   {0  , 196  , 255},
   {0  , 200  , 255},
   {0  , 204  , 255},
   {0  , 208  , 255},
   {0  , 212  , 255},
   {0  , 216  , 255},
   {0  , 220  , 255},
   {0  , 224  , 255},
   {0  , 228  , 255},
   {0  , 232  , 255},
   {0  , 236  , 255},
   {0  , 240  , 255},
   {0  , 244  , 255},
   {0  , 248  , 255},
   {0  , 252  , 255},
   {0  , 255  , 255},
   {4  , 255  , 252},
   {8  , 255  , 248},
   {12 , 255 ,  244},
   {16 , 255 ,  240},
   {20 , 255 ,  236},
   {24 , 255 ,  232},
   {28 , 255 ,  228},
   {32 , 255 ,  224},
   {36 , 255 ,  220},
   {40 , 255 ,  216},
   {44 , 255 ,  212},
   {48 , 255 ,  208},
   {52 , 255 ,  204},
   {56 , 255 ,  200},
   {60 , 255 ,  196},
   {64 , 255 ,  192},
   {68 , 255 ,  188},
   {72 , 255 ,  184},
   {76 , 255 ,  180},
   {80 , 255 ,  176},
   {84 , 255 ,  172},
   {88 , 255 ,  168},
   {92 , 255 ,  164},
   {96 , 255 ,  160},
   {100,   255, 156},
   {104,   255, 152},
   {108,   255, 148},
   {112,   255, 144},
   {116,   255, 140},
   {120,   255, 136},
   {124,   255, 132},
   {128,   255, 128},
   {132,   255, 124},
   {136,   255, 120},
   {140,   255, 116},
   {144,   255, 112},
   {148,   255, 108},
   {152,   255, 104},
   {156,   255, 100},
   {160,   255,  96},
   {164,   255,  92},
   {168,   255,  88},
   {172,   255,  84},
   {176,   255,  80},
   {180,   255,  76},
   {184,   255,  72},
   {188,   255,  68},
   {192,   255,  64},
   {196,   255,  60},
   {200,   255,  56},
   {204,   255,  52},
   {208,   255,  48},
   {212,   255,  44},
   {216,   255,  40},
   {220,   255,  36},
   {224,   255,  32},
   {228,   255,  28},
   {232,   255,  24},
   {236,   255,  20},
   {240,   255,  16},
   {244,   255,  12},
   {248,   255,   8},
   {252,   255,   4},
   {255,   255,   0},
   {255,   252,   0},
   {255,   248,   0},
   {255,   244,   0},
   {255,   240,   0},
   {255,   236,   0},
   {255,   232,   0},
   {255,   228,   0},
   {255,   224,   0},
   {255,   220,   0},
   {255,   216,   0},
   {255,   212,   0},
   {255,   208,   0},
   {255,   204,   0},
   {255,   200,   0},
   {255,   196,   0},
   {255,   192,   0},
   {255,   188,   0},
   {255,   184,   0},
   {255,   180,   0},
   {255,   176,   0},
   {255,   172,   0},
   {255,   168,   0},
   {255,   164,   0},
   {255,   160,   0},
   {255,   156,   0},
   {255,   152,   0},
   {255,   148,   0},
   {255,   144,   0},
   {255,   140,   0},
   {255,   136,   0},
   {255,   132,   0},
   {255,   128,   0},
   {255,   124,   0},
   {255,   120,   0},
   {255,   116,   0},
   {255,   112,   0},
   {255,   108,   0},
   {255,   104,   0},
   {255,   100,   0},
   {255,    96,   0},
   {255,    92,   0},
   {255,    88,   0},
   {255,    84,   0},
   {255,    80,   0},
   {255,    76,   0},
   {255,    72,   0},
   {255,    68,   0},
   {255,    64,   0},
   {255,    60,   0},
   {255,    56,   0},
   {255,    52,   0},
   {255,    48,   0},
   {255,    44,   0},
   {255,    40,   0},
   {255,    36,   0},
   {255,    32,   0},
   {255,    28,   0},
   {255,    24,   0},
   {255,    20,   0},
   {255,    16,   0},
   {255,    12,   0},
   {255,     8,   0},
   {255,     4,   0},
   {255,     0,   0},
   {252,     0,   0},
   {248,     0,   0},
   {244,     0,   0},
   {240,     0,   0},
   {236,     0,   0},
   {232,     0,   0},
   {228,     0,   0},
   {224,     0,   0},
   {220,     0,   0},
   {216,     0,   0},
   {212,     0,   0},
   {208,     0,   0},
   {204,     0,   0},
   {200,     0,   0},
   {196,     0,   0},
   {192,     0,   0},
   {188,     0,   0},
   {184,     0,   0},
   {180,     0,   0},
   {176,     0,   0},
   {172,     0,   0},
   {168,     0,   0},
   {164,     0,   0},
   {160,     0,   0},
   {156,     0,   0},
   {152,     0,   0},
   {148,     0,   0},
   {144,     0,   0},
   {140,     0,   0},
   {136,     0,   0},
   {132,     0,   0},
   {128,     0,   0},
  };

static void
arv_fake_camera_diagonal_ramp (ArvBuffer *buffer, void *fill_pattern_data,
			       guint32 exposure_time_us,
			       guint32 gain,
			       ArvPixelFormat pixel_format)
{
	double pixel_value;
	double scale;
	guint32 x, y;
	guint32 width;
	guint32 height;

        g_return_if_fail (buffer != NULL);
        g_return_if_fail (buffer->priv->n_parts == 1);

	width = buffer->priv->parts[0].width;
	height = buffer->priv->parts[0].height;

	scale = 1.0 + gain + log10 ((double) exposure_time_us / 10000.0);

	switch (pixel_format)
	{
		case ARV_PIXEL_FORMAT_MONO_8:
			if (height * width <= buffer->priv->allocated_size) {
				for (y = 0; y < height; y++) {
					for (x = 0; x < width; x++) {
						unsigned char *pixel = &buffer->priv->data [y * width + x];

						pixel_value = (x + buffer->priv->frame_id + y) % 255;
						pixel_value *= scale;

						*pixel = CLAMP (pixel_value, 0, 255);
					}
				}
                                buffer->priv->received_size = height * width;
			}
			break;

		case ARV_PIXEL_FORMAT_MONO_16:
			if (2 * height * width <= buffer->priv->allocated_size) {
				for (y = 0; y < height; y++) {
					for (x = 0; x < width; x++) {
						unsigned short *pixel = (unsigned short *)&buffer->priv->data [2*y * width + 2*x];

						pixel_value = (256*x + 256*buffer->priv->frame_id + 256*y) % 65535;
						pixel_value *= scale;

						*pixel = CLAMP (pixel_value, 0, 65535);
					}
				}
                                buffer->priv->received_size = 2 * height * width;
			}
			break;

		case ARV_PIXEL_FORMAT_BAYER_BG_8:
			if (height * width <= buffer->priv->allocated_size) {
				for (y = 0; y < height; y++) {
					for (x = 0; x < width; x++) {
						unsigned int index;
						unsigned char *pixel;

						pixel_value = (x + buffer->priv->frame_id + y) % 255;
						pixel_value *= scale;
						index = CLAMP (pixel_value, 0, 255);

						// BG
						// GR
						pixel = &buffer->priv->data [y * width + x];
						if (x & 1) {
							if (y & 1)
								*pixel = jet_colormap [index].b;
							else
								*pixel = jet_colormap [index].g;
						} else {
							if (y & 1)
								*pixel = jet_colormap [index].g;
							else
								*pixel = jet_colormap [index].r;
						}
					}
				}
                                buffer->priv->received_size = height * width;
			}
			break;

		case ARV_PIXEL_FORMAT_BAYER_GB_8:
			if (height * width <= buffer->priv->allocated_size) {
				for (y = 0; y < height; y++) {
					for (x = 0; x < width; x++) {
						unsigned int index;
						unsigned char *pixel;

						pixel_value = (x + buffer->priv->frame_id + y) % 255;
						pixel_value *= scale;
						index = CLAMP (pixel_value, 0, 255);

						// GB
						// RG
						pixel = &buffer->priv->data [y * width + x];
						if (x & 1) {
							if (y & 1)
								*pixel = jet_colormap [index].g;
							else
								*pixel = jet_colormap [index].b;
						} else {
							if (y & 1)
								*pixel = jet_colormap [index].r;
							else
								*pixel = jet_colormap [index].g;
						}
					}
				}
                                buffer->priv->received_size = height * width;
			}
			break;

		case ARV_PIXEL_FORMAT_BAYER_GR_8:
			if (height * width <= buffer->priv->allocated_size) {
				for (y = 0; y < height; y++) {
					for (x = 0; x < width; x++) {
						unsigned int index;
						unsigned char *pixel;

						pixel_value = (x + buffer->priv->frame_id + y) % 255;
						pixel_value *= scale;
						index = CLAMP (pixel_value, 0, 255);

						// GR
						// BG
						pixel = &buffer->priv->data [y * width + x];
						if (x & 1) {
							if (y & 1)
								*pixel = jet_colormap [index].g;
							else
								*pixel = jet_colormap [index].r;
						} else {
							if (y & 1)
								*pixel = jet_colormap [index].b;
							else
								*pixel = jet_colormap [index].g;
						}
					}
				}
                                buffer->priv->received_size = height * width;
			}
			break;

		case ARV_PIXEL_FORMAT_BAYER_RG_8:
			if (height * width <= buffer->priv->allocated_size) {
				for (y = 0; y < height; y++) {
					for (x = 0; x < width; x++) {
						unsigned int index;
						unsigned char *pixel;

						pixel_value = (x + buffer->priv->frame_id + y) % 255;
						pixel_value *= scale;
						index = CLAMP (pixel_value, 0, 255);

						// RG
						// GB
						pixel = &buffer->priv->data [y * width + x];
						if (x & 1) {
							if (y & 1)
								*pixel = jet_colormap [index].r;
							else
								*pixel = jet_colormap [index].g;
						} else {
							if (y & 1)
								*pixel = jet_colormap [index].g;
							else
								*pixel = jet_colormap [index].b;
						}
					}
				}
                                buffer->priv->received_size = height * width;
			}
			break;

		case ARV_PIXEL_FORMAT_RGB_8_PACKED:
			if (3 * height * width <= buffer->priv->allocated_size) {
				for (y = 0; y < height; y++) {
					for (x = 0; x < width; x++) {
						unsigned char *pixel = &buffer->priv->data [3 * (y * width + x)];
						unsigned int index;

						pixel_value = (x + buffer->priv->frame_id + y) % 255;
						pixel_value *= scale;

						index = CLAMP (pixel_value, 0, 255);

						pixel[0] = jet_colormap [index].r;
						pixel[1] = jet_colormap [index].g;
						pixel[2] = jet_colormap [index].b;
					}
				}
                                buffer->priv->received_size = 3 * height * width;
			}
			break;

		default:
			g_critical ("Unsupported pixel format");
			break;
	}
}

/**
 * arv_fake_camera_set_fill_pattern:
 * @camera: a #ArvFakeCamera
 * @fill_pattern_callback: (scope call): callback for image filling
 * @fill_pattern_data: (closure): image filling user data
 *
 * Sets the fill pattern callback for custom test images.
 */

void
arv_fake_camera_set_fill_pattern (ArvFakeCamera *camera,
				  ArvFakeCameraFillPattern fill_pattern_callback,
				  void *fill_pattern_data)
{
	g_return_if_fail (ARV_IS_FAKE_CAMERA (camera));

	g_mutex_lock (&camera->priv->fill_pattern_mutex);

	if (fill_pattern_callback != NULL) {
		camera->priv->fill_pattern_callback = fill_pattern_callback;
		camera->priv->fill_pattern_data = fill_pattern_data;
	} else {
		camera->priv->fill_pattern_callback = arv_fake_camera_diagonal_ramp;
		camera->priv->fill_pattern_data = NULL;
	}

	g_mutex_unlock (&camera->priv->fill_pattern_mutex);
}

/**
 * arv_fake_camera_fill_buffer:
 * @camera: a #ArvFakeCamera
 * @buffer: the #ArvBuffer to fill
 * @packet_size: (out) (optional): the packet size
 *
 * Fill a buffer with data from the fake camera.
 */

void
arv_fake_camera_fill_buffer (ArvFakeCamera *camera, ArvBuffer *buffer, guint32 *packet_size)
{
	guint32 width;
	guint32 height;
	guint32 exposure_time_us;
	guint32 gain;
	guint32 pixel_format;
	size_t payload;

	if (camera == NULL || buffer == NULL)
		return;

        arv_buffer_set_n_parts(buffer, 1);

	width = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_WIDTH);
	height = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_HEIGHT);
	payload = arv_fake_camera_get_payload (camera);

	if (buffer->priv->allocated_size < payload) {
		buffer->priv->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
		return;
	}

	/* frame id is a 16 bit value, 0 is invalid */
	camera->priv->frame_id = (camera->priv->frame_id + 1) % 65536;
	if (camera->priv->frame_id == 0)
		camera->priv->frame_id = 1;

	buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_IMAGE;
	buffer->priv->chunk_endianness = G_BIG_ENDIAN;
	buffer->priv->status = ARV_BUFFER_STATUS_SUCCESS;
	buffer->priv->timestamp_ns = g_get_real_time () * 1000;
	buffer->priv->system_timestamp_ns = buffer->priv->timestamp_ns;
	buffer->priv->frame_id = camera->priv->frame_id;

        buffer->priv->parts[0].data_offset = 0;
        buffer->priv->parts[0].component_id = 0;
        buffer->priv->parts[0].data_type = ARV_BUFFER_PART_DATA_TYPE_2D_IMAGE;
	buffer->priv->parts[0].pixel_format = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_PIXEL_FORMAT);
	buffer->priv->parts[0].width = width;
	buffer->priv->parts[0].height = height;
        buffer->priv->parts[0].x_offset = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_X_OFFSET);
        buffer->priv->parts[0].y_offset = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_Y_OFFSET);
        buffer->priv->parts[0].x_padding = 0;
        buffer->priv->parts[0].y_padding = 0;

	g_mutex_lock (&camera->priv->fill_pattern_mutex);

	arv_fake_camera_read_register (camera, ARV_FAKE_CAMERA_REGISTER_EXPOSURE_TIME_US, &exposure_time_us);
	arv_fake_camera_read_register (camera, ARV_FAKE_CAMERA_REGISTER_GAIN_RAW, &gain);
	arv_fake_camera_read_register (camera, ARV_FAKE_CAMERA_REGISTER_PIXEL_FORMAT, &pixel_format);
	camera->priv->fill_pattern_callback (buffer, camera->priv->fill_pattern_data,
					     exposure_time_us, gain, pixel_format);

	g_mutex_unlock (&camera->priv->fill_pattern_mutex);

        buffer->priv->parts[0].size = buffer->priv->received_size;

	if (packet_size != NULL)
		*packet_size =
			(_get_register (camera, ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET) >>
			 ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_POS) &
			ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_MASK;
}

void
arv_fake_camera_set_inet_address (ArvFakeCamera *camera, GInetAddress *address)
{
	const guint8 *bytes;

	g_return_if_fail (ARV_IS_FAKE_CAMERA (camera));
	g_return_if_fail (G_IS_INET_ADDRESS (address));
	g_return_if_fail (g_inet_address_get_family (address) == G_SOCKET_FAMILY_IPV4);

	bytes = g_inet_address_to_bytes (address);

	arv_fake_camera_write_memory (camera, ARV_GVBS_CURRENT_IP_ADDRESS_OFFSET,
				      g_inet_address_get_native_size (address), (char *) bytes);
}

guint32
arv_fake_camera_get_acquisition_status (ArvFakeCamera *camera)
{
	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), 0);

	return _get_register (camera, ARV_FAKE_CAMERA_REGISTER_ACQUISITION);
}

/**
 * arv_fake_camera_get_stream_address:
 * @camera: a #ArvFakeCamera
 *
 * Return value: (transfer full): the data stream #GSocketAddress for this camera
 */

GSocketAddress *
arv_fake_camera_get_stream_address (ArvFakeCamera *camera)
{
	GSocketAddress *stream_socket_address;
	GInetAddress *inet_address;
	guint32 value;

	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), NULL);

	arv_fake_camera_read_memory (camera, ARV_GVBS_STREAM_CHANNEL_0_IP_ADDRESS_OFFSET, sizeof (value), &value);

	inet_address = g_inet_address_new_from_bytes ((guint8 *) &value, G_SOCKET_FAMILY_IPV4);
	stream_socket_address = g_inet_socket_address_new
		(inet_address,
		 _get_register (camera, ARV_GVBS_STREAM_CHANNEL_0_PORT_OFFSET));

	g_object_unref (inet_address);

	return stream_socket_address;
}

void
arv_fake_camera_set_trigger_frequency (ArvFakeCamera *camera, double frequency)
{
	g_return_if_fail (ARV_IS_FAKE_CAMERA (camera));
	g_return_if_fail (frequency > 0.0);

	camera->priv->trigger_frequency = frequency;
}

gboolean
arv_fake_camera_check_and_acknowledge_software_trigger (ArvFakeCamera *camera)
{
	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), FALSE);


	if (_get_register (camera, ARV_FAKE_CAMERA_REGISTER_TRIGGER_SOFTWARE) == 1) {
		arv_fake_camera_write_register (camera, ARV_FAKE_CAMERA_REGISTER_TRIGGER_SOFTWARE, 0);
		return TRUE;
	}
	return FALSE;
}

gboolean
arv_fake_camera_is_in_free_running_mode (ArvFakeCamera *camera)
{
	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), FALSE);

	if (_get_register (camera, ARV_FAKE_CAMERA_REGISTER_TRIGGER_MODE) == 0 &&
	    _get_register (camera, ARV_FAKE_CAMERA_REGISTER_ACQUISITION_FRAME_PERIOD_US) > 0) {
		return TRUE;
	}
	return FALSE;
}

gboolean
arv_fake_camera_is_in_software_trigger_mode (ArvFakeCamera *camera)
{
	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), FALSE);

	if (_get_register (camera, ARV_FAKE_CAMERA_REGISTER_TRIGGER_MODE) == 1 &&
	    _get_register (camera, ARV_FAKE_CAMERA_REGISTER_TRIGGER_SOURCE) == 1) {
		return TRUE;
	}
	return FALSE;
}

guint32
arv_fake_camera_get_control_channel_privilege (ArvFakeCamera *camera)
{
	guint32 value;

	arv_fake_camera_read_register (camera, ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value);

	return value;
}

void
arv_fake_camera_set_control_channel_privilege (ArvFakeCamera *camera, guint32 privilege)
{
	arv_fake_camera_write_register (camera, ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_OFFSET, privilege);
}

guint32
arv_fake_camera_get_heartbeat_timeout (ArvFakeCamera *camera)
{
	guint32 value;

	arv_fake_camera_read_register (camera, ARV_GVBS_HEARTBEAT_TIMEOUT_OFFSET, &value);

	return value;
}

/**
 * arv_fake_camera_get_genicam_xml:
 * @camera: a #ArvFakeCamera
 * @size: (out) (optional): the size of the returned XML string
 *
 * Return value: (transfer none): the genicam XML description of the camera
 */

const char *
arv_fake_camera_get_genicam_xml (ArvFakeCamera *camera, size_t *size)
{
	if (size != NULL)
		*size = 0;

	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), NULL);

	if (size != NULL)
		*size = camera->priv->genicam_xml_size;

	return camera->priv->genicam_xml;
}

/* GObject implemenation */

ArvFakeCamera *
arv_fake_camera_new_full (const char *serial_number, const char *genicam_filename)
{
	ArvFakeCamera *fake_camera;
	GError *error = NULL;
	char *filename;
	void *memory;
	char *xml_url;

	g_return_val_if_fail (serial_number != NULL, NULL);
	g_return_val_if_fail (*serial_number != '\0', NULL);
	g_return_val_if_fail (strlen (serial_number) < ARV_GVBS_SERIAL_NUMBER_SIZE, NULL);

	fake_camera = g_object_new (ARV_TYPE_FAKE_CAMERA, NULL);

	memory = g_malloc0 (ARV_FAKE_CAMERA_MEMORY_SIZE);

	g_mutex_init (&fake_camera->priv->fill_pattern_mutex);
	fake_camera->priv->fill_pattern_callback = arv_fake_camera_diagonal_ramp;
	fake_camera->priv->fill_pattern_data = NULL;

	if (genicam_filename != NULL)
		filename = g_strdup (genicam_filename);
	else if (arv_get_fake_camera_genicam_filename () != NULL)
		filename = g_strdup (arv_get_fake_camera_genicam_filename ());
	else
		filename = NULL;

	if (filename) {
		if (!g_file_get_contents (filename,
					  &fake_camera->priv->genicam_xml,
					  &fake_camera->priv->genicam_xml_size,
					  &error)) {
			g_critical ("Failed to load genicam file '%s': %s",
				    filename, error != NULL ? error->message : "Unknown reason");
			g_clear_error (&error);
			fake_camera->priv->genicam_xml = NULL;
			fake_camera->priv->genicam_xml_size = 0;
		}
	} else {
		GBytes *bytes = g_resources_lookup_data("/org/aravis/arv-fake-camera.xml",
							G_RESOURCE_LOOKUP_FLAGS_NONE, &error);

		if (error != NULL) {
			g_critical ("Failed to load embedded resource arv-fake-camera.xml: %s",error->message);
			g_clear_error (&error);
		} else {
			fake_camera->priv->genicam_xml = g_strndup (g_bytes_get_data(bytes,NULL), g_bytes_get_size(bytes));
			fake_camera->priv->genicam_xml_size = g_bytes_get_size(bytes);
		}

		g_bytes_unref (bytes);
	}

	g_clear_pointer (&filename, g_free);

	fake_camera->priv->memory = memory;

	strcpy (((char *) memory) + ARV_GVBS_MANUFACTURER_NAME_OFFSET, "Aravis");
	strcpy (((char *) memory) + ARV_GVBS_MODEL_NAME_OFFSET, "Fake");
	strcpy (((char *) memory) + ARV_GVBS_MANUFACTURER_INFO_OFFSET, "none");
	strcpy (((char *) memory) + ARV_GVBS_DEVICE_VERSION_OFFSET, ARAVIS_VERSION);
	strcpy (((char *) memory) + ARV_GVBS_SERIAL_NUMBER_OFFSET, serial_number);

	xml_url = g_strdup_printf ("Local:arv-fake-camera.xml;%x;%x",
				   ARV_FAKE_CAMERA_MEMORY_SIZE,
				   (unsigned int) fake_camera->priv->genicam_xml_size);
	strcpy (((char *) memory) + ARV_GVBS_XML_URL_0_OFFSET, xml_url);
	g_free (xml_url);

	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_SENSOR_WIDTH,
					ARV_FAKE_CAMERA_SENSOR_WIDTH);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_SENSOR_HEIGHT,
					ARV_FAKE_CAMERA_SENSOR_HEIGHT);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_WIDTH,
				        ARV_FAKE_CAMERA_WIDTH_DEFAULT);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_HEIGHT,
					ARV_FAKE_CAMERA_HEIGHT_DEFAULT);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_X_OFFSET, 0);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_Y_OFFSET, 0);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_BINNING_HORIZONTAL,
					ARV_FAKE_CAMERA_BINNING_HORIZONTAL_DEFAULT);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_BINNING_VERTICAL,
					ARV_FAKE_CAMERA_BINNING_HORIZONTAL_DEFAULT);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_PIXEL_FORMAT, ARV_PIXEL_FORMAT_MONO_8);

	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_ACQUISITION, 0);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_ACQUISITION_MODE, 1);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_ACQUISITION_FRAME_PERIOD_US,
					1000000.0 / ARV_FAKE_CAMERA_ACQUISITION_FRAME_RATE_DEFAULT);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_EXPOSURE_TIME_US,
					ARV_FAKE_CAMERA_EXPOSURE_TIME_US_DEFAULT);

	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_TRIGGER_MODE, 0);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_TRIGGER_SOURCE, 0);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_TRIGGER_ACTIVATION, 0);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_TRIGGER_SOFTWARE, 0);

	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_GAIN_RAW, 0);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_GAIN_MODE, 1);

	arv_fake_camera_write_register (fake_camera, ARV_GVBS_HEARTBEAT_TIMEOUT_OFFSET, 3000);
	arv_fake_camera_write_register (fake_camera, ARV_GVBS_TIMESTAMP_TICK_FREQUENCY_HIGH_OFFSET, 0);
	arv_fake_camera_write_register (fake_camera, ARV_GVBS_TIMESTAMP_TICK_FREQUENCY_LOW_OFFSET, 1000000000);
	arv_fake_camera_write_register (fake_camera, ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0);

	arv_fake_camera_write_register (fake_camera, ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET, 1400);

	arv_fake_camera_write_register (fake_camera, ARV_GVBS_N_NETWORK_INTERFACES_OFFSET, 1);

	arv_fake_camera_write_register (fake_camera, ARV_GVBS_N_STREAM_CHANNELS_OFFSET, 1);

	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_TEST, ARV_FAKE_CAMERA_TEST_REGISTER_DEFAULT);

	return fake_camera;
}

ArvFakeCamera *
arv_fake_camera_new (const char *serial_number)
{
	return arv_fake_camera_new_full (serial_number, NULL);
}

static void
arv_fake_camera_init (ArvFakeCamera *fake_camera)
{
	fake_camera->priv = arv_fake_camera_get_instance_private (fake_camera);

	fake_camera->priv->trigger_frequency = 25.0;
	fake_camera->priv->frame_id = 65400; /* Trigger circular counter bugs sooner */
}

static void
arv_fake_camera_finalize (GObject *object)
{
	ArvFakeCamera *fake_camera = ARV_FAKE_CAMERA (object);

	g_mutex_clear (&fake_camera->priv->fill_pattern_mutex);
	g_clear_pointer (&fake_camera->priv->memory, g_free);
	g_clear_pointer (&fake_camera->priv->genicam_xml, g_free);

	G_OBJECT_CLASS (arv_fake_camera_parent_class)->finalize (object);
}

static void
arv_fake_camera_class_init (ArvFakeCameraClass *fake_camera_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (fake_camera_class);

	object_class->finalize = arv_fake_camera_finalize;
}

ARV_DEFINE_DESTRUCTOR (arv_fake_camera_destructor)
static void
arv_fake_camera_destructor (void)
{
	arv_set_fake_camera_genicam_filename (NULL);
}
