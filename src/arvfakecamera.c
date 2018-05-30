/* Aravis - Digital camera library
 *
 * Copyright © 2009-2011 Emmanuel Pacaud
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

/**
 * SECTION: arvfakecamera
 * @short_description: Fake camera internals
 *
 * #ArvFakeCamera is a class that simulate a real camera, which provides
 * methods for the implementation of #ArvFakeDevice and #ArvFakeStream.
 *
 * arv-fake-gv-camera is a GV camera simulator based on this class.
 */

#include <arvconfig.h>
#include <arvfakecamera.h>
#include <arvgc.h>
#include <arvgcregisternode.h>
#include <arvgvcp.h>
#include <arvbufferprivate.h>
#include <arvdebug.h>
#include <arvmisc.h>
#include <string.h>
#include <math.h>

static GObjectClass *parent_class = NULL;

struct _ArvFakeCameraPrivate {
	void *memory;
	const void *genicam_xml;
	size_t genicam_xml_size;

	guint32 frame_id;
	double trigger_frequency;

	GMutex fill_pattern_mutex;

	ArvFakeCameraFillPattern fill_pattern_callback;
	void *fill_pattern_data;
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

	if (address < ARV_FAKE_CAMERA_MEMORY_SIZE) {
		read_size = MIN (address  + size, ARV_FAKE_CAMERA_MEMORY_SIZE) - address;

		memcpy (buffer, ((char *) camera->priv->memory) + address, read_size);

		if (read_size == size)
			return TRUE;

		size = size - read_size;
		address = ARV_FAKE_CAMERA_MEMORY_SIZE;
		buffer = buffer + read_size;
	}

	address -= ARV_FAKE_CAMERA_MEMORY_SIZE;
	read_size = MIN (address + size, camera->priv->genicam_xml_size) - address;

	memcpy (buffer, ((char *) camera->priv->genicam_xml) + address, read_size);
	if (read_size < size)
		memset (buffer + read_size, 0, size - read_size);

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
	guint32 width, height;

	g_return_val_if_fail (ARV_IS_FAKE_CAMERA (camera), 0);

	width = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_WIDTH);
	height = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_HEIGHT);

	return width * height;
}

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

	if (buffer == NULL)
		return;

	if (pixel_format != ARV_PIXEL_FORMAT_MONO_8)
		return;

	width = buffer->priv->width;
	height = buffer->priv->height;

	scale = 1.0 + gain + log10 ((double) exposure_time_us / 10000.0);

	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++) {
			pixel_value = (x + buffer->priv->frame_id + y) % 255;
			pixel_value *= scale;

			if (pixel_value < 0.0)
				((unsigned char *) buffer->priv->data)[y * width + x] = 0;
			else if (pixel_value > 255.0)
				((unsigned char *) buffer->priv->data)[y * width + x] = 255;
			else
				((unsigned char *) buffer->priv->data)[y * width + x] = pixel_value;
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

	width = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_WIDTH);
	height = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_HEIGHT);
	payload = width * height;

	if (buffer->priv->size < payload) {
		buffer->priv->status = ARV_BUFFER_STATUS_SIZE_MISMATCH;
		return;
	}

	buffer->priv->payload_type = ARV_BUFFER_PAYLOAD_TYPE_IMAGE;
	buffer->priv->chunk_endianness = G_BIG_ENDIAN;
	buffer->priv->width = width;
	buffer->priv->height = height;
	buffer->priv->status = ARV_BUFFER_STATUS_SUCCESS;
	buffer->priv->timestamp_ns = g_get_real_time () * 1000;
	buffer->priv->system_timestamp_ns = buffer->priv->timestamp_ns;
	buffer->priv->frame_id = camera->priv->frame_id++;
	buffer->priv->pixel_format = _get_register (camera, ARV_FAKE_CAMERA_REGISTER_PIXEL_FORMAT);

	g_mutex_lock (&camera->priv->fill_pattern_mutex);

	arv_fake_camera_read_register (camera, ARV_FAKE_CAMERA_REGISTER_EXPOSURE_TIME_US, &exposure_time_us);
	arv_fake_camera_read_register (camera, ARV_FAKE_CAMERA_REGISTER_GAIN_RAW, &gain);
	arv_fake_camera_read_register (camera, ARV_FAKE_CAMERA_REGISTER_PIXEL_FORMAT, &pixel_format);
	camera->priv->fill_pattern_callback (buffer, camera->priv->fill_pattern_data,
					     exposure_time_us, gain, pixel_format);

	g_mutex_unlock (&camera->priv->fill_pattern_mutex);

	if (packet_size != NULL)
		*packet_size = _get_register (camera, ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET);
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

void
arv_set_fake_camera_genicam_filename (const char *filename)
{
	arv_fake_camera_genicam_filename = filename;
}

const char *
arv_get_fake_camera_genicam_xml (size_t *size)
{
	static GMappedFile *genicam_file = NULL;
	static GMutex mutex;

	g_mutex_lock (&mutex);

	if (genicam_file == NULL ) {
		char *filename;

		if (arv_fake_camera_genicam_filename == NULL)
			filename = g_build_filename (ARAVIS_DATA_DIR, "arv-fake-camera.xml", NULL);
		else
			filename = g_strdup (arv_fake_camera_genicam_filename);

		genicam_file = g_mapped_file_new (filename, FALSE, NULL);

		if (genicam_file != NULL) {
			arv_debug_genicam ("[get_fake_camera_genicam_data] %s [size = %d]", filename,
					   g_mapped_file_get_length (genicam_file));
			arv_log_genicam (g_mapped_file_get_contents (genicam_file));
		}

		g_free (filename);
	}

	g_mutex_unlock (&mutex);

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

	g_mutex_init (&fake_camera->priv->fill_pattern_mutex);
	fake_camera->priv->fill_pattern_callback = arv_fake_camera_diagonal_ramp;
	fake_camera->priv->fill_pattern_data = NULL;

	fake_camera->priv->genicam_xml = arv_get_fake_camera_genicam_xml (&fake_camera->priv->genicam_xml_size);
	fake_camera->priv->memory = memory;

	strcpy (((char *) memory) + ARV_GVBS_MANUFACTURER_NAME_OFFSET, "Aravis");
	strcpy (((char *) memory) + ARV_GVBS_MODEL_NAME_OFFSET, "Fake");
	strcpy (((char *) memory) + ARV_GVBS_DEVICE_VERSION_OFFSET, PACKAGE_VERSION);
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

	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_GAIN_RAW, 0);
	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_GAIN_MODE, 1);

	arv_fake_camera_write_register (fake_camera, ARV_GVBS_HEARTBEAT_TIMEOUT_OFFSET, 3000);
	arv_fake_camera_write_register (fake_camera, ARV_GVBS_TIMESTAMP_TICK_FREQUENCY_HIGH_OFFSET, 0);
	arv_fake_camera_write_register (fake_camera, ARV_GVBS_TIMESTAMP_TICK_FREQUENCY_LOW_OFFSET, 1000000000);
	arv_fake_camera_write_register (fake_camera, ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0);

	arv_fake_camera_write_register (fake_camera, ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET, 2000);

	arv_fake_camera_write_register (fake_camera, ARV_GVBS_N_STREAM_CHANNELS_OFFSET, 1);

	arv_fake_camera_write_register (fake_camera, ARV_FAKE_CAMERA_REGISTER_TEST, ARV_FAKE_CAMERA_TEST_REGISTER_DEFAULT);

	return fake_camera;
}

static void
arv_fake_camera_init (ArvFakeCamera *fake_camera)
{
	fake_camera->priv = G_TYPE_INSTANCE_GET_PRIVATE (fake_camera, ARV_TYPE_FAKE_CAMERA, ArvFakeCameraPrivate);

	fake_camera->priv->trigger_frequency = 25.0;
	fake_camera->priv->frame_id = 65000; /* Trigger circular counter bugs sooner */
}

static void
arv_fake_camera_finalize (GObject *object)
{
	ArvFakeCamera *fake_camera = ARV_FAKE_CAMERA (object);

	g_free (fake_camera->priv->memory);

	g_mutex_clear (&fake_camera->priv->fill_pattern_mutex);

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
