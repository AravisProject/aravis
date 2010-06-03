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

/**
 * SECTION: arvfakecamera
 * @short_description: Fake camera internals
 *
 * #ArvFakeCamera is a class that simulate a real camera, which provides
 * methods for the implementation of #ArvFakeDevice and #ArvFakeStream. It's
 * foresen to use this class for the implementation of a fake ethernet camera
 * too, but it's still a TODO.
 */

#include <arvfakecamera.h>
#include <arvgc.h>
#include <arvgcregister.h>
#include <arvgvcp.h>
#include <arvbuffer.h>
#include <arvdebug.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

struct _ArvFakeCameraPrivate {
	void *memory;
	const void *genicam_data;
	size_t genicam_data_size;

	guint32 frame_id;
	double trigger_frequency;
};

static const char *arv_fake_camera_genicam_filename = NULL;

/* ArvFakeCamera implementation */

gboolean
arv_fake_camera_read_memory (ArvFakeCamera *camera, guint32 address, guint32 size, void *buffer)
{
	guint32 read_size;

	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), FALSE);
	g_return_val_if_fail (buffer != NULL, FALSE);
	g_return_val_if_fail (size > 0, FALSE);

	/* TODO Handle read accross register space and genicam data */

	if (address < ARV_FAKE_CAMERA_MEMORY_SIZE) {
		g_return_val_if_fail (address + size < ARV_FAKE_CAMERA_MEMORY_SIZE, FALSE);

		memcpy (buffer, camera->priv->memory + address, size);

		return TRUE;
	}

	address -= ARV_FAKE_CAMERA_MEMORY_SIZE;
	read_size = MIN (address + size, camera->priv->genicam_data_size) - address;

	memcpy (buffer, camera->priv->genicam_data + address, read_size);

	return TRUE;
}

gboolean
arv_fake_camera_write_memory (ArvFakeCamera *camera, guint32 address, guint32 size, const void *buffer)
{
	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), FALSE);
	g_return_val_if_fail (address + size < ARV_FAKE_CAMERA_MEMORY_SIZE + camera->priv->genicam_data_size, FALSE);
	g_return_val_if_fail (buffer != NULL, FALSE);
	g_return_val_if_fail (size > 0, FALSE);

	/* genicam_data are read only */
	if (address + size > ARV_FAKE_CAMERA_MEMORY_SIZE)
		return FALSE;

	memcpy (camera->priv->memory + address, buffer, size);

	return TRUE;
}

gboolean
arv_fake_camera_read_register (ArvFakeCamera *camera, guint32 address, guint32 *value)
{
	return arv_fake_camera_read_memory (camera, address, sizeof (*value), value);
}

gboolean
arv_fake_camera_write_register (ArvFakeCamera *camera, guint32 address, guint32 value)
{
	return arv_fake_camera_write_memory (camera, address, sizeof (value), &value);
}

static guint32
_get_register (ArvFakeCamera *camera, guint32 address)
{
	if (address + sizeof (guint32) > ARV_FAKE_CAMERA_MEMORY_SIZE)
		return 0;

	return *((guint32 *) ((void*) (camera->priv->memory + address)));
}

size_t
arv_fake_camera_get_payload (ArvFakeCamera *camera)
{
	guint32 width, height;

	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), 0);

	width = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_WIDTH);
	height = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_HEIGHT);

	return width * height;
}

void
arv_fake_camera_wait_for_next_frame (ArvFakeCamera *camera)
{
	struct timespec time;
	struct timespec sleep_time;
	guint64 sleep_time_ns;
	guint64 frame_period_time_ns;

	if (_get_register (camera, ARV_FAKE_CAMERA_REGISTER_TRIGGER_MODE) == 1)
		frame_period_time_ns = 1000000000L / camera->priv->trigger_frequency;
	else
		frame_period_time_ns = (guint64) _get_register (camera,
								ARV_FAKE_CAMERA_REGISTER_ACQUISITION_FRAME_PERIOD_US) *
			1000L;

	clock_gettime (CLOCK_MONOTONIC, &time);
	sleep_time_ns = frame_period_time_ns - (((guint64) time.tv_sec * 1000000000L +
						 (guint64) time.tv_nsec) % frame_period_time_ns);

	sleep_time.tv_sec = sleep_time_ns / 1000000000L;
	sleep_time.tv_nsec = sleep_time_ns % 1000000000L;

	nanosleep (&sleep_time, NULL);
}

void
arv_fake_camera_fill_buffer (ArvFakeCamera *camera, ArvBuffer *buffer)
{
	struct timespec time;
	guint32 width;
	guint32 height;
	guint32 x_offset, y_offset;
	size_t payload;
	guint32 x, y;

	if (camera == NULL || buffer == NULL)
		return;

	clock_gettime (CLOCK_MONOTONIC, &time);

	width = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_WIDTH);
	height = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_HEIGHT);
	x_offset = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_X_OFFSET);
	y_offset = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_Y_OFFSET);
	payload = width * height;

	if (buffer->size < payload) {
		buffer->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
		return;
	}

	buffer->width = width;
	buffer->height = height;
	buffer->status = ARV_BUFFER_STATUS_SUCCESS;
	buffer->timestamp_ns = time.tv_sec * 1000000000LL + time.tv_nsec;
	buffer->frame_id = camera->priv->frame_id++;
	buffer->pixel_format = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_PIXEL_FORMAT);

	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++)
			((char *) buffer->data)[y * width + x] = (x + buffer->frame_id + y) % 255;
}

void
arv_fake_camera_set_inet_address (ArvFakeCamera *camera, GInetAddress *address)
{
	const guint8 *bytes;

	g_return_if_fail (ARV_IS_FAKE_CAMERA (camera));
	g_return_if_fail (G_IS_INET_ADDRESS (address));
	g_return_if_fail (g_inet_address_get_family (address) == G_SOCKET_FAMILY_IPV4);

	bytes = g_inet_address_to_bytes (address);

	arv_fake_camera_write_memory (camera, ARV_GVBS_CURRENT_IP_ADDRESS,
				      g_inet_address_get_native_size (address), (char *) bytes);
}

guint32
arv_fake_camera_get_acquisition_status (ArvFakeCamera *camera)
{
	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), 0);

	return _get_register (camera, ARV_FAKE_CAMERA_REGISTER_ACQUISITION);
}

GSocketAddress *
arv_fake_camera_get_stream_address (ArvFakeCamera *camera)
{
	GSocketAddress *stream_socket_address;
	GInetAddress *inet_address;

	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), NULL);

	inet_address = g_inet_address_new_from_bytes (camera->priv->memory + ARV_GVBS_FIRST_STREAM_CHANNEL_IP_ADDRESS,
						      G_SOCKET_FAMILY_IPV4);
	stream_socket_address = g_inet_socket_address_new (inet_address,
							   _get_register (camera, ARV_GVBS_FIRST_STREAM_CHANNEL_PORT));
	g_object_unref (inet_address);

	return stream_socket_address;
}

void
arv_fake_camera_set_trigger_frequency (ArvFakeCamera *camera, double frequency)
{
	g_return_if_fail (ARV_IS_FAKE_CAMERA (camera));
	g_return_if_fail (frequency > 0.0);

	camera->priv->trigger_frequency = frequency;
	camera->priv->frame_id = 0;
}

void
arv_set_fake_camera_genicam_filename (const char *filename)
{
	arv_fake_camera_genicam_filename = filename;
}

const char *
arv_get_fake_camera_genicam_data (size_t *size)
{
	static GMappedFile *genicam_file = NULL;
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

	g_static_mutex_lock (&mutex);

	if (genicam_file == NULL ) {
		char *filename;

		if (arv_fake_camera_genicam_filename == NULL)
			filename = g_build_filename (ARAVIS_DATA_DIR, "arv-fake-camera.xml", NULL);
		else
			filename = g_strdup (arv_fake_camera_genicam_filename);

		genicam_file = g_mapped_file_new (filename, FALSE, NULL);

		if (genicam_file != NULL) {
			arv_debug ("fake-genicam", "[get_fake_camera_genicam_data] %s [size = %d]",
				   filename,
				   g_mapped_file_get_length (genicam_file));
			arv_debug ("fake-genicam", g_mapped_file_get_contents (genicam_file));
		}

		g_free (filename);
	}

	g_static_mutex_unlock (&mutex);

	g_return_val_if_fail( genicam_file != NULL, NULL);

	if (size != NULL)
		*size = g_mapped_file_get_length (genicam_file);

	return g_mapped_file_get_contents (genicam_file);
}

/* GObject implemenation */

ArvFakeCamera *
arv_fake_camera_new (const char *serial_number)
{
	ArvFakeCamera *fake_camera;
	void *memory;
	char *xml_url;

	g_return_val_if_fail (serial_number != NULL, NULL);
	g_return_val_if_fail (*serial_number != '\0', NULL);
	g_return_val_if_fail (strlen (serial_number) < ARV_GVBS_SERIAL_NUMBER_SIZE, NULL);

	fake_camera = g_object_new (ARV_TYPE_FAKE_CAMERA, NULL);

	memory = g_malloc0 (ARV_FAKE_CAMERA_MEMORY_SIZE);

	fake_camera->priv->genicam_data = arv_get_fake_camera_genicam_data (&fake_camera->priv->genicam_data_size);
	fake_camera->priv->memory = memory;

	strcpy (memory + ARV_GVBS_MANUFACTURER_NAME, "Aravis");
	strcpy (memory + ARV_GVBS_MODEL_NAME, "Fake");
	strcpy (memory + ARV_GVBS_DEVICE_VERSION, PACKAGE_VERSION);
	strcpy (memory + ARV_GVBS_SERIAL_NUMBER, serial_number);

	xml_url = g_strdup_printf ("Local:arv-fake-camera-%s.xml;%x;%x",
				   PACKAGE_VERSION,
				   ARV_FAKE_CAMERA_MEMORY_SIZE,
				   fake_camera->priv->genicam_data_size);
	strcpy (memory + ARV_GVBS_FIRST_XML_URL, xml_url);
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

	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_GAIN_RAW, 0);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_GAIN_MODE, 0);

	return fake_camera;
}

static void
arv_fake_camera_init (ArvFakeCamera *fake_camera)
{
	fake_camera->priv = G_TYPE_INSTANCE_GET_PRIVATE (fake_camera, ARV_TYPE_FAKE_CAMERA, ArvFakeCameraPrivate);

	fake_camera->priv->trigger_frequency = 25.0;
}

static void
arv_fake_camera_finalize (GObject *object)
{
	ArvFakeCamera *fake_camera = ARV_FAKE_CAMERA (object);

	g_free (fake_camera->priv->memory);

	parent_class->finalize (object);
}

static void
arv_fake_camera_class_init (ArvFakeCameraClass *fake_camera_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (fake_camera_class);

	g_type_class_add_private (fake_camera_class, sizeof (ArvFakeCameraPrivate));

	parent_class = g_type_class_peek_parent (fake_camera_class);

	object_class->finalize = arv_fake_camera_finalize;
}

G_DEFINE_TYPE (ArvFakeCamera, arv_fake_camera, G_TYPE_OBJECT)
