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

/**
 * SECTION: arvgvdevice
 * @short_description: Gigabit ethernet camera device
 */

#include <arvgvdevice.h>
#include <arvdeviceprivate.h>
#include <arvgc.h>
#include <arvgcregisterdescriptionnode.h>
#include <arvdebug.h>
#include <arvgvstream.h>
#include <arvgvcp.h>
#include <arvgvsp.h>
#include <arvzip.h>
#include <arvstr.h>
#include <arvmisc.h>
#include <string.h>
#include <stdlib.h>

static GObjectClass *parent_class = NULL;

/* Shared data (main thread - heartbeat) */

typedef struct {
#if GLIB_CHECK_VERSION(2,32,0)
	GMutex mutex;
#else
	GMutex *mutex;
#endif

	guint16 packet_id;

	GSocket *socket;
	GSocketAddress	*interface_address;
	GSocketAddress	*device_address;

	GPollFD poll_in_event;

	void *buffer;

	unsigned int gvcp_n_retries;
	unsigned int gvcp_timeout_ms;

	gboolean is_controller;
} ArvGvDeviceIOData;

struct _ArvGvDevicePrivate {
	ArvGvDeviceIOData *io_data;

	void *heartbeat_thread;
	void *heartbeat_data;

	ArvGc *genicam;

	char *genicam_xml;
	size_t genicam_xml_size;

	gboolean is_packet_resend_supported;
	gboolean is_write_memory_supported;
};

GRegex *
arv_gv_device_get_url_regex (void)
{
static GRegex *arv_gv_device_url_regex = NULL;

	if (arv_gv_device_url_regex == NULL)
		arv_gv_device_url_regex = g_regex_new ("^(local:|file:|http:)(.+\\.[^;]+);?(?:0x)?([0-9:a-f]*)?;?(?:0x)?([0-9:a-f]*)?$",
						       G_REGEX_CASELESS, 0, NULL);

	return arv_gv_device_url_regex;
}

static gboolean
_read_memory (ArvGvDeviceIOData *io_data, guint32 address, guint32 size, void *buffer, GError **error)
{
	ArvGvcpPacket *packet;
	size_t packet_size;
	size_t answer_size;
	int count;
	unsigned int n_retries = 0;
	gboolean success = FALSE;

	answer_size = arv_gvcp_packet_get_read_memory_ack_size (size);

	g_return_val_if_fail (answer_size <= ARV_GV_DEVICE_BUFFER_SIZE, FALSE);

#if GLIB_CHECK_VERSION(2,32,0)
	g_mutex_lock (&io_data->mutex);
#else
	g_mutex_lock (io_data->mutex);
#endif

	packet = arv_gvcp_packet_new_read_memory_cmd (address,
						      ((size + sizeof (guint32) - 1)
						       / sizeof (guint32)) * sizeof (guint32),
						      0, &packet_size);

	do {
		io_data->packet_id = arv_gvcp_next_packet_id (io_data->packet_id);
		arv_gvcp_packet_set_packet_id (packet, io_data->packet_id);

		arv_gvcp_packet_debug (packet, ARV_DEBUG_LEVEL_LOG);

		g_socket_send_to (io_data->socket, io_data->device_address,
				  (const char *) packet, packet_size,
				  NULL, NULL);

		if (g_poll (&io_data->poll_in_event, 1, io_data->gvcp_timeout_ms) > 0) {
			count = g_socket_receive (io_data->socket, io_data->buffer,
						  ARV_GV_DEVICE_BUFFER_SIZE, NULL, NULL);
			if (count >= answer_size) {
				ArvGvcpPacket *ack_packet = io_data->buffer;
				ArvGvcpPacketType packet_type;
				ArvGvcpCommand command;
				guint16 packet_id;

				arv_gvcp_packet_debug (ack_packet, ARV_DEBUG_LEVEL_LOG);

				packet_type = arv_gvcp_packet_get_packet_type (ack_packet);
				command = arv_gvcp_packet_get_command (ack_packet);
				packet_id = arv_gvcp_packet_get_packet_id (ack_packet);

				if (packet_type == ARV_GVCP_PACKET_TYPE_ACK &&
				    command == ARV_GVCP_COMMAND_READ_MEMORY_ACK &&
				    packet_id == io_data->packet_id) {
					memcpy (buffer, arv_gvcp_packet_get_read_memory_ack_data (ack_packet), size);
					success = TRUE;
				} else
					arv_gvcp_packet_debug (ack_packet, ARV_DEBUG_LEVEL_WARNING);
			}
		}

		n_retries++;

	} while (!success && n_retries < io_data->gvcp_n_retries);

	arv_gvcp_packet_free (packet);

#if GLIB_CHECK_VERSION(2,32,0)
	g_mutex_unlock (&io_data->mutex);
#else
	g_mutex_unlock (io_data->mutex);
#endif

	if (!success) {
		if (error != NULL && *error == NULL)
			*error = g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_STATUS_TIMEOUT,
					      "[ArvDevice::read_memory] Timeout");
	}

	return success;
}

static gboolean
_write_memory (ArvGvDeviceIOData *io_data, guint32 address, guint32 size, void *buffer, GError **error)
{
	ArvGvcpPacket *packet;
	size_t packet_size;
	int count;
	unsigned int n_retries = 0;
	gboolean success = FALSE;

#if GLIB_CHECK_VERSION(2,32,0)
	g_mutex_lock (&io_data->mutex);
#else
	g_mutex_lock (io_data->mutex);
#endif

	packet = arv_gvcp_packet_new_write_memory_cmd (address,
						       ((size + sizeof (guint32) - 1) /
							sizeof (guint32)) * sizeof (guint32),
						       0, &packet_size);

	memcpy (arv_gvcp_packet_get_write_memory_cmd_data (packet), buffer, size);

	do {
		io_data->packet_id = arv_gvcp_next_packet_id (io_data->packet_id);
		arv_gvcp_packet_set_packet_id (packet, io_data->packet_id);

		arv_gvcp_packet_debug (packet, ARV_DEBUG_LEVEL_LOG);

		g_socket_send_to (io_data->socket, io_data->device_address,
				  (const char *) packet, packet_size,
				  NULL, NULL);

		if (g_poll (&io_data->poll_in_event, 1, io_data->gvcp_timeout_ms) > 0) {
			count = g_socket_receive (io_data->socket, io_data->buffer,
						  ARV_GV_DEVICE_BUFFER_SIZE, NULL, NULL);
			if (count >= arv_gvcp_packet_get_write_memory_ack_size ()) {
				ArvGvcpPacket *ack_packet = io_data->buffer;
				ArvGvcpPacketType packet_type;
				ArvGvcpCommand command;
				guint16 packet_id;

				arv_gvcp_packet_debug (ack_packet, ARV_DEBUG_LEVEL_LOG);

				packet_type = arv_gvcp_packet_get_packet_type (ack_packet);
				command = arv_gvcp_packet_get_command (ack_packet);
				packet_id = arv_gvcp_packet_get_packet_id (ack_packet);

				if (packet_type == ARV_GVCP_PACKET_TYPE_ACK &&
				    command == ARV_GVCP_COMMAND_WRITE_MEMORY_ACK &&
				    packet_id == io_data->packet_id)
					success = TRUE;
				else
					arv_gvcp_packet_debug (ack_packet, ARV_DEBUG_LEVEL_WARNING);
			}
		}

		n_retries++;

	} while (!success && n_retries < io_data->gvcp_n_retries);

	arv_gvcp_packet_free (packet);

#if GLIB_CHECK_VERSION(2,32,0)
	g_mutex_unlock (&io_data->mutex);
#else
	g_mutex_unlock (io_data->mutex);
#endif

	if (!success) {
		if (error != NULL && *error == NULL)
			*error = g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_STATUS_TIMEOUT,
					      "[ArvDevice::write_memory] Timeout");
	}

	return success;
}

gboolean
_read_register (ArvGvDeviceIOData *io_data, guint32 address, guint32 *value_placeholder, GError **error)
{
	ArvGvcpPacket *packet;
	size_t packet_size;
	int count;
	unsigned int n_retries = 0;
	gboolean success = FALSE;

#if GLIB_CHECK_VERSION(2,32,0)
	g_mutex_lock (&io_data->mutex);
#else
	g_mutex_lock (io_data->mutex);
#endif

	packet = arv_gvcp_packet_new_read_register_cmd (address, 0, &packet_size);

	do {
		io_data->packet_id = arv_gvcp_next_packet_id (io_data->packet_id);
		arv_gvcp_packet_set_packet_id (packet, io_data->packet_id);

		arv_gvcp_packet_debug (packet, ARV_DEBUG_LEVEL_LOG);

		g_socket_send_to (io_data->socket, io_data->device_address,
				  (const char *) packet, packet_size,
				  NULL, NULL);

		if (g_poll (&io_data->poll_in_event, 1, io_data->gvcp_timeout_ms) > 0) {
			count = g_socket_receive (io_data->socket, io_data->buffer,
						  ARV_GV_DEVICE_BUFFER_SIZE, NULL, NULL);
			if (count > 0) {
				ArvGvcpPacket *ack_packet = io_data->buffer;
				ArvGvcpPacketType packet_type;
				ArvGvcpCommand command;
				guint16 packet_id;

				arv_gvcp_packet_debug (ack_packet, ARV_DEBUG_LEVEL_LOG);

				packet_type = arv_gvcp_packet_get_packet_type (ack_packet);
				command = arv_gvcp_packet_get_command (ack_packet);
				packet_id = arv_gvcp_packet_get_packet_id (ack_packet);

				if (packet_type == ARV_GVCP_PACKET_TYPE_ACK &&
				    command == ARV_GVCP_COMMAND_READ_REGISTER_ACK &&
				    packet_id == io_data->packet_id) {
					*value_placeholder = arv_gvcp_packet_get_read_register_ack_value (ack_packet);
					success = TRUE;
				} else
					arv_gvcp_packet_debug (ack_packet, ARV_DEBUG_LEVEL_WARNING);
			}
		}

		n_retries++;

	} while (!success && n_retries < io_data->gvcp_n_retries);

	arv_gvcp_packet_free (packet);

#if GLIB_CHECK_VERSION(2,32,0)
	g_mutex_unlock (&io_data->mutex);
#else
	g_mutex_unlock (io_data->mutex);
#endif

	if (!success) {
		*value_placeholder = 0;

		if (error != NULL && *error == NULL)
			*error = g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_STATUS_TIMEOUT,
					      "[ArvDevice::read_register] Timeout");
	}

	return success;
}

gboolean
_write_register (ArvGvDeviceIOData *io_data, guint32 address, guint32 value, GError **error)
{
	ArvGvcpPacket *packet;
	size_t packet_size;
	int count;
	unsigned int n_retries = 0;
	gboolean success = FALSE;

#if GLIB_CHECK_VERSION(2,32,0)
	g_mutex_lock (&io_data->mutex);
#else
	g_mutex_lock (io_data->mutex);
#endif

	packet = arv_gvcp_packet_new_write_register_cmd (address, value, io_data->packet_id, &packet_size);

	do {
		io_data->packet_id = arv_gvcp_next_packet_id (io_data->packet_id);
		arv_gvcp_packet_set_packet_id (packet, io_data->packet_id);

		arv_gvcp_packet_debug (packet, ARV_DEBUG_LEVEL_LOG);

		g_socket_send_to (io_data->socket, io_data->device_address, (const char *) packet, packet_size,
				  NULL, NULL);

		if (g_poll (&io_data->poll_in_event, 1, io_data->gvcp_timeout_ms) > 0) {
			count = g_socket_receive (io_data->socket, io_data->buffer,
						  ARV_GV_DEVICE_BUFFER_SIZE, NULL, NULL);
			if (count > 0) {
				ArvGvcpPacket *ack_packet = io_data->buffer;
				ArvGvcpPacketType packet_type;
				ArvGvcpCommand command;
				guint16 packet_id;

				arv_gvcp_packet_debug (ack_packet, ARV_DEBUG_LEVEL_LOG);

				packet_type = arv_gvcp_packet_get_packet_type (ack_packet);
				command = arv_gvcp_packet_get_command (ack_packet);
				packet_id = arv_gvcp_packet_get_packet_id (ack_packet);

				arv_log_gvcp ("%d, %d, %d", packet_type, command, packet_id);

				if (packet_type == ARV_GVCP_PACKET_TYPE_ACK &&
				    command == ARV_GVCP_COMMAND_WRITE_REGISTER_ACK &&
				    packet_id == io_data->packet_id)
					success = TRUE;
				else
					arv_gvcp_packet_debug (ack_packet, ARV_DEBUG_LEVEL_WARNING);
			}
		}

		n_retries++;

	} while (!success && n_retries < io_data->gvcp_n_retries);

	arv_gvcp_packet_free (packet);

#if GLIB_CHECK_VERSION(2,32,0)
	g_mutex_unlock (&io_data->mutex);
#else
	g_mutex_unlock (io_data->mutex);
#endif

	if (!success) {
		if (error != NULL && *error == NULL)
			*error = g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_STATUS_TIMEOUT,
					      "[ArvDevice::write_register] Timeout");
	}

	return success;
}

/* Heartbeat thread */

typedef struct {
	ArvGvDevice *gv_device;
	ArvGvDeviceIOData *io_data;
	int period_us;
	gboolean cancel;
} ArvGvDeviceHeartbeatData;

static void *
arv_gv_device_heartbeat_thread (void *data)
{
	ArvGvDeviceHeartbeatData *thread_data = data;
	ArvGvDeviceIOData *io_data = thread_data->io_data;
	GTimer *timer;
	guint32 value;

	timer = g_timer_new ();

	do {
		g_usleep (thread_data->period_us);

		if (io_data->is_controller) {
			guint counter = 1;

			/* TODO: Instead of reading the control register, Pylon does write the heartbeat
			 * timeout value, which is interresting, as doing this we could get an error
			 * ack packet which will indicate we lost the control access. */

			g_timer_start (timer);

			while (!_read_register (io_data, ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, NULL) &&
			       g_timer_elapsed (timer, NULL) < ARV_GV_DEVICE_HEARTBEAT_RETRY_TIMEOUT_S &&
			       !thread_data->cancel) {
				g_usleep (ARV_GV_DEVICE_HEARTBEAT_RETRY_DELAY_US);
				counter++;
			}

			if (!thread_data->cancel) {
				arv_log_device ("[GvDevice::Heartbeat] Ack value = %d", value);

				if (counter > 1)
					arv_log_device ("[GvDevice::Heartbeat] Tried %u times", counter);

				if ((value & (ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_CONTROL |
					      ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_EXCLUSIVE)) == 0) {
					arv_warning_device ("[GvDevice::Heartbeat] Control access lost");

					arv_device_emit_control_lost_signal (ARV_DEVICE (thread_data->gv_device));

					io_data->is_controller = FALSE;
				}
			} else
				io_data->is_controller = FALSE;
		}
	} while (!thread_data->cancel);

	g_timer_destroy (timer);

	return NULL;
}

/* ArvGvDevice implemenation */

static gboolean
arv_gv_device_take_control (ArvGvDevice *gv_device)
{
	gboolean success;

	success = arv_device_write_register (ARV_DEVICE (gv_device),
					     ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_OFFSET,
					     ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_CONTROL, NULL);

	gv_device->priv->io_data->is_controller = success;

	if (!success)
		arv_warning_device ("[GvDevice::take_control] Can't get control access");

	return success;
}

static gboolean
arv_gv_device_leave_control (ArvGvDevice *gv_device)
{
	gboolean success;

	gv_device->priv->io_data->is_controller = FALSE;

	success = arv_device_write_register (ARV_DEVICE (gv_device),
					    ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0, NULL);

	return success;
}

guint64
arv_gv_device_get_timestamp_tick_frequency (ArvGvDevice *gv_device)
{
	guint32 timestamp_tick_frequency_high;
	guint32 timestamp_tick_frequency_low;

	g_return_val_if_fail (ARV_IS_GV_DEVICE (gv_device), 0);

	if (arv_device_read_register (ARV_DEVICE (gv_device),
				      ARV_GVBS_TIMESTAMP_TICK_FREQUENCY_HIGH_OFFSET,
				      &timestamp_tick_frequency_high, NULL) &&
	    arv_device_read_register (ARV_DEVICE (gv_device),
				      ARV_GVBS_TIMESTAMP_TICK_FREQUENCY_LOW_OFFSET,
				      &timestamp_tick_frequency_low, NULL)) {
		guint64 timestamp_tick_frequency;

		timestamp_tick_frequency = ((guint64) timestamp_tick_frequency_high << 32) |
			timestamp_tick_frequency_low;
		return timestamp_tick_frequency;
	}

	return 0;
}

guint
arv_gv_device_get_packet_size (ArvGvDevice *gv_device)
{
	return arv_device_get_integer_feature_value (ARV_DEVICE (gv_device), "GevSCPSPacketSize");
}

void
arv_gv_device_set_packet_size (ArvGvDevice *gv_device, guint packet_size)
{
	g_return_if_fail (packet_size > 0);

	arv_device_set_integer_feature_value (ARV_DEVICE (gv_device), "GevSCPSPacketSize", packet_size);
}

static char *
_load_genicam (ArvGvDevice *gv_device, guint32 address, size_t  *size)
{
	char filename[ARV_GVBS_XML_URL_SIZE];
	char **tokens;
	char *genicam = NULL;

	g_return_val_if_fail (size != NULL, NULL);

	*size = 0;

	if (!arv_device_read_memory (ARV_DEVICE (gv_device), address, ARV_GVBS_XML_URL_SIZE, filename, NULL))
		return NULL;

	filename[ARV_GVBS_XML_URL_SIZE - 1] = '\0';

	arv_debug_device ("[GvDevice::load_genicam] xml url = '%s' at 0x%x", filename, address);

	tokens = g_regex_split (arv_gv_device_get_url_regex (), filename, 0);

	if (tokens[0] != NULL && tokens[1] != NULL) {
		if (g_ascii_strcasecmp (tokens[1], "file:") == 0) {
			gsize len;
			g_file_get_contents (tokens[2], &genicam, &len, NULL);
			if (genicam)
				*size = len;
		} else if (g_ascii_strcasecmp (tokens[1], "local:") == 0 &&
			 tokens[2] != NULL &&
			 tokens[3] != NULL &&
			 tokens[4] != NULL) {
			guint32 file_address;
			guint32 file_size;

			file_address = strtoul (tokens[3], NULL, 16);
			file_size = strtoul (tokens[4], NULL, 16);

			arv_debug_device ("[GvDevice::load_genicam] Xml address = 0x%x - size = 0x%x - %s",
					  file_address, file_size, tokens[2]);

			if (file_size > 0) {
				genicam = g_malloc (file_size);
				if (arv_device_read_memory (ARV_DEVICE (gv_device), file_address, file_size,
							    genicam, NULL)) {

					if (arv_debug_check (&arv_debug_category_misc, ARV_DEBUG_LEVEL_LOG)) {
						GString *string = g_string_new ("");
						
						g_string_append_printf (string,
									"[GvDevice::load_genicam] Raw data size = 0x%x\n", file_size);
						arv_g_string_append_hex_dump (string, genicam, file_size);

						arv_log_misc ("%s", string->str);

						g_string_free (string, TRUE);
					}

					if (g_str_has_suffix (tokens[2], ".zip")) {
						ArvZip *zip;
						const GSList *zip_files;

						arv_debug_device ("[GvDevice::load_genicam] Zipped xml data");

						zip = arv_zip_new (genicam, file_size);
						zip_files = arv_zip_get_file_list (zip);

						if (zip_files != NULL) {
							const char *zip_filename;
							void *tmp_buffer;
							size_t tmp_buffer_size;

							zip_filename = arv_zip_file_get_name (zip_files->data);
							tmp_buffer = arv_zip_get_file (zip, zip_filename,
										       &tmp_buffer_size);

							g_free (genicam);
							file_size = tmp_buffer_size;
							genicam = tmp_buffer;
						} else
							arv_warning_device ("[GvDevice::load_genicam] Invalid format");
						arv_zip_free (zip);
					}
					*size = file_size;
				} else {
					g_free (genicam);
					genicam = NULL;
					*size = 0;
				}
			}
		} else if (g_ascii_strcasecmp (tokens[1], "http:") == 0) {
			GFile *file;
			GFileInputStream *stream;

			file = g_file_new_for_uri (filename);
			stream = g_file_read (file, NULL, NULL);
			if(stream) {
				GDataInputStream *data_stream;
				gsize len;

				data_stream = g_data_input_stream_new (G_INPUT_STREAM (stream));
				genicam = g_data_input_stream_read_upto (data_stream, "", 0, &len, NULL, NULL);

				if (genicam)
					*size = len;

				g_object_unref (data_stream);
				g_object_unref (stream);
			}
			g_object_unref (file);
		}
	}

	g_strfreev (tokens);

	return genicam;
}

static const char *
arv_gv_device_get_genicam_xml (ArvDevice *device, size_t *size)
{
	char *xml;

	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);

	if (gv_device->priv->genicam_xml != NULL) {
		*size = gv_device->priv->genicam_xml_size;
		return gv_device->priv->genicam_xml;
	}

	*size = 0;

	xml = _load_genicam (gv_device, ARV_GVBS_XML_URL_0_OFFSET, size);
	if (xml == NULL)
		xml = _load_genicam (gv_device, ARV_GVBS_XML_URL_1_OFFSET, size);

	gv_device->priv->genicam_xml = xml;
	gv_device->priv->genicam_xml_size = *size;

	return xml;
}

static void
arv_gv_device_load_genicam (ArvGvDevice *gv_device)
{
	const char *genicam;
	size_t size;

	genicam = arv_gv_device_get_genicam_xml (ARV_DEVICE (gv_device), &size);
	if (genicam != NULL) {
		gv_device->priv->genicam = arv_gc_new (ARV_DEVICE (gv_device), genicam, size);

		arv_gc_set_default_node_data (gv_device->priv->genicam, "DeviceVendorName",
					      "<StringReg Name=\"DeviceVendorName\">"
					      "<DisplayName>Vendor Name</DisplayName>"
					      "<Address>0x48</Address>"
					      "<Length>32</Length>"
					      "<AccessMode>RO</AccessMode>"
					      "<pPort>Device</pPort>"
					      "</StringReg>");
		arv_gc_set_default_node_data (gv_device->priv->genicam, "DeviceModelName",
					      "<StringReg Name=\"DeviceModelName\">"
					      "<DisplayName>Model Name</DisplayName>"
					      "<Address>0x68</Address>"
					      "<Length>32</Length>"
					      "<AccessMode>RO</AccessMode>"
					      "<pPort>Device</pPort>"
					      "</StringReg>");
		arv_gc_set_default_node_data (gv_device->priv->genicam, "DeviceVersion",
					      "<StringReg Name=\"DeviceVersion\">"
					      "<DisplayName>Device Version</DisplayName>"
					      "<Address>0x88</Address>"
					      "<Length>32</Length>"
					      "<AccessMode>RO</AccessMode>"
					      "<pPort>Device</pPort>"
					      "</StringReg>");
		arv_gc_set_default_node_data (gv_device->priv->genicam, "DeviceManufacturerInfo",
					      "<StringReg Name=\"DeviceManufacturerInfo\">"
					      "<DisplayName>Manufacturer Info</DisplayName>"
					      "<Address>0xa8</Address>"
					      "<Length>48</Length>"
					      "<AccessMode>RO</AccessMode>"
					      "<pPort>Device</pPort>"
					      "</StringReg>");
		arv_gc_set_default_node_data (gv_device->priv->genicam, "DeviceID",
					      "<StringReg Name=\"DeviceID\">"
					      "<DisplayName>Device ID</DisplayName>"
					      "<Address>0xd8</Address>"
					      "<Length>16</Length>"
					      "<AccessMode>RO</AccessMode>"
					      "<pPort>Device</pPort>"
					      "</StringReg>");
		arv_gc_set_default_node_data (gv_device->priv->genicam, "GevSCPSPacketSize",
					      "<Integer Name=\"GevSCPSPacketSize\">"
					      "<Visibility>Expert</Visibility>"
					      "<pIsLocked>TLParamsLocked</pIsLocked>"
					      "<pValue>GevSCPSPacketSizeReg</pValue>"
					      "</Integer>");
		arv_gc_set_default_node_data (gv_device->priv->genicam, "GevSCPSPacketSizeReg",
					      "<MaskedIntReg Name=\"GevSCPSPacketSizeReg\">"
					      "<Address>0xd04</Address>"
					      "<Length>4</Length>"
					      "<AccessMode>RW</AccessMode>"
					      "<pPort>Device</pPort>"
					      "<LSB>31</LSB>"
					      "<MSB>16</MSB>"
					      "<Sign>Unsigned</Sign>"
					      "<Endianess>BigEndian</Endianess>"
					      "</MaskedIntReg>");
		arv_gc_set_default_node_data (gv_device->priv->genicam, "TLParamsLocked",
					      "<Integer Name=\"TLParamsLocked\">"
					      "<Visibility>Invisible</Visibility>"
					      "<Value>0</Value>"
					      "<Min>0</Min>"
					      "<Max>1</Max>"
					      "</Integer>");
	}
}

/* ArvDevice implemenation */

static ArvStream *
arv_gv_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);
	ArvGvDeviceIOData *io_data = gv_device->priv->io_data;
	ArvStream *stream;
	const guint8 *address_bytes;
	guint32 stream_port;
	guint packet_size;
	guint32 n_stream_channels;
	GInetAddress *interface_address;
	GInetAddress *device_address;

	arv_device_read_register (device, ARV_GVBS_N_STREAM_CHANNELS_OFFSET, &n_stream_channels, NULL);
	arv_debug_device ("[GvDevice::create_stream] Number of stream channels = %d", n_stream_channels);

	if (n_stream_channels < 1)
		return NULL;

	if (!io_data->is_controller) {
		arv_warning_device ("[GvDevice::create_stream] Can't create stream without control access");
		return NULL;
	}

	interface_address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (io_data->interface_address));
	device_address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (io_data->device_address));
	address_bytes = g_inet_address_to_bytes (interface_address);

	/* On some cameras, the default packet size after reset is incorrect.
	 * So, if the value is obviously incorrect, set it to a default size. */
	packet_size = arv_gv_device_get_packet_size (gv_device);
	if (packet_size <= ARV_GVSP_PACKET_PROTOCOL_OVERHEAD) {
		arv_gv_device_set_packet_size (gv_device, ARV_GV_DEVICE_GVSP_PACKET_SIZE_DEFAULT);
		arv_debug_device ("[GvDevice::create_stream] Packet size set to default value (%d)",
				  ARV_GV_DEVICE_GVSP_PACKET_SIZE_DEFAULT);
	}

	packet_size = arv_gv_device_get_packet_size (gv_device);
	arv_debug_device ("[GvDevice::create_stream] Packet size = %d byte(s)", packet_size);

	stream = arv_gv_stream_new (device_address, 0, callback, user_data,
				    arv_gv_device_get_timestamp_tick_frequency (gv_device), packet_size);

	stream_port = arv_gv_stream_get_port (ARV_GV_STREAM (stream));

	if (!arv_device_write_register (device, ARV_GVBS_STREAM_CHANNEL_0_IP_ADDRESS_OFFSET,
					g_htonl(*((guint32 *) address_bytes)), NULL) ||
	    !arv_device_write_register (device, ARV_GVBS_STREAM_CHANNEL_0_PORT_OFFSET, stream_port, NULL)) {
		arv_warning_device ("[GvDevice::create_stream] Stream configuration failed");

		g_object_unref (stream);
		return NULL;
	}

	if (!gv_device->priv->is_packet_resend_supported)
		g_object_set (stream, "packet-resend", ARV_GV_STREAM_PACKET_RESEND_NEVER, NULL); 

	arv_debug_device ("[GvDevice::create_stream] Stream port = %d", stream_port);

	return stream;
}

static ArvGc *
arv_gv_device_get_genicam (ArvDevice *device)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);

	return gv_device->priv->genicam;
}

static gboolean
arv_gv_device_read_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer, GError **error)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);
	int i;
	gint32 block_size;

	for (i = 0; i < (size + ARV_GVCP_DATA_SIZE_MAX - 1) / ARV_GVCP_DATA_SIZE_MAX; i++) {
		block_size = MIN (ARV_GVCP_DATA_SIZE_MAX, size - i * ARV_GVCP_DATA_SIZE_MAX);
		if (!_read_memory (gv_device->priv->io_data,
				   address + i * ARV_GVCP_DATA_SIZE_MAX,
				   block_size, ((char *) buffer) + i * ARV_GVCP_DATA_SIZE_MAX, error))
			return FALSE;
	}

	return TRUE;
}

static gboolean
arv_gv_device_write_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer, GError **error)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);
	int i;
	gint32 block_size;

	for (i = 0; i < (size + ARV_GVCP_DATA_SIZE_MAX - 1) / ARV_GVCP_DATA_SIZE_MAX; i++) {
		block_size = MIN (ARV_GVCP_DATA_SIZE_MAX, size - i * ARV_GVCP_DATA_SIZE_MAX);
		if (!_write_memory (gv_device->priv->io_data,
				    address + i * ARV_GVCP_DATA_SIZE_MAX,
				    block_size, ((char *) buffer) + i * ARV_GVCP_DATA_SIZE_MAX, error))
			return FALSE;
	}

	return TRUE;
}

static gboolean
arv_gv_device_read_register (ArvDevice *device, guint32 address, guint32 *value, GError **error)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);

	return _read_register (gv_device->priv->io_data, address, value, error);
}

static gboolean
arv_gv_device_write_register (ArvDevice *device, guint32 address, guint32 value, GError **error)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);

	return _write_register (gv_device->priv->io_data, address, value, error);
}

ArvDevice *
arv_gv_device_new (GInetAddress *interface_address, GInetAddress *device_address)
{
	ArvGvDevice *gv_device;
	ArvGvDeviceIOData *io_data;
	ArvGvDeviceHeartbeatData *heartbeat_data;
	ArvGcRegisterDescriptionNode *register_description;
	ArvDomDocument *document;
	char *address_string;
	guint32 capabilities;

	g_return_val_if_fail (G_IS_INET_ADDRESS (interface_address), NULL);
	g_return_val_if_fail (G_IS_INET_ADDRESS (device_address), NULL);

	address_string = g_inet_address_to_string (interface_address);
	arv_debug_device ("[GvDevice::new] Interface address = %s", address_string);
	g_free (address_string);
	address_string = g_inet_address_to_string (device_address);
	arv_debug_device ("[GvDevice::new] Device address = %s", address_string);
	g_free (address_string);

	gv_device = g_object_new (ARV_TYPE_GV_DEVICE, NULL);

	io_data = g_new0 (ArvGvDeviceIOData, 1);

#if GLIB_CHECK_VERSION(2,32,0)
	g_mutex_init (&io_data->mutex);
#else
	io_data->mutex = g_mutex_new ();
#endif
	io_data->packet_id = 65300; /* Start near the end of the circular counter */

	io_data->interface_address = g_inet_socket_address_new (interface_address, 0);
	io_data->device_address = g_inet_socket_address_new (device_address, ARV_GVCP_PORT);
	io_data->socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
					G_SOCKET_TYPE_DATAGRAM,
					G_SOCKET_PROTOCOL_UDP, NULL);
	g_socket_bind (io_data->socket, io_data->interface_address, TRUE, NULL);

	io_data->buffer = g_malloc (ARV_GV_DEVICE_BUFFER_SIZE);
	io_data->gvcp_n_retries = ARV_GV_DEVICE_GVCP_N_RETRIES_DEFAULT;
	io_data->gvcp_timeout_ms = ARV_GV_DEVICE_GVCP_TIMEOUT_MS_DEFAULT;
	io_data->poll_in_event.fd = g_socket_get_fd (io_data->socket);
	io_data->poll_in_event.events =  G_IO_IN;
	io_data->poll_in_event.revents = 0;

	gv_device->priv->io_data = io_data;

	arv_gv_device_load_genicam (gv_device);

	if (!ARV_IS_GC (gv_device->priv->genicam)) {
		arv_warning_device ("[GvDevice::new] Failed to load genicam data");
		g_object_unref (gv_device);
		return NULL;
	}

	arv_gv_device_take_control (gv_device);

	heartbeat_data = g_new (ArvGvDeviceHeartbeatData, 1);
	heartbeat_data->gv_device = gv_device;
	heartbeat_data->io_data = io_data;
	heartbeat_data->period_us = ARV_GV_DEVICE_HEARTBEAT_PERIOD_US;
	heartbeat_data->cancel = FALSE;

	gv_device->priv->heartbeat_data = heartbeat_data;

	gv_device->priv->heartbeat_thread = arv_g_thread_new ("arv_gv_heartbeat", arv_gv_device_heartbeat_thread,
							      gv_device->priv->heartbeat_data);

	arv_device_read_register (ARV_DEVICE (gv_device), ARV_GVBS_GVCP_CAPABILITY_OFFSET, &capabilities, NULL);
	gv_device->priv->is_packet_resend_supported = (capabilities & ARV_GVBS_GVCP_CAPABILITY_PACKET_RESEND) != 0;
	gv_device->priv->is_write_memory_supported = (capabilities & ARV_GVBS_GVCP_CAPABILITY_WRITE_MEMORY) != 0;

	arv_debug_device ("[GvDevice::new] Packet resend = %s", gv_device->priv->is_packet_resend_supported ? "yes" : "no");
	arv_debug_device ("[GvDevice::new] Write memory = %s", gv_device->priv->is_write_memory_supported ? "yes" : "no");

	document = ARV_DOM_DOCUMENT (gv_device->priv->genicam);
	register_description = ARV_GC_REGISTER_DESCRIPTION_NODE (arv_dom_document_get_document_element (document));
	if (!arv_gc_register_description_node_check_schema_version (register_description, 1, 1, 0))
		arv_debug_device ("[GvDevice::new] Register workaround = yes");

	return ARV_DEVICE (gv_device);
}

static void
arv_gv_device_init (ArvGvDevice *gv_device)
{
	gv_device->priv = G_TYPE_INSTANCE_GET_PRIVATE (gv_device, ARV_TYPE_GV_DEVICE, ArvGvDevicePrivate);

	gv_device->priv->genicam = NULL;
	gv_device->priv->genicam_xml = NULL;
	gv_device->priv->genicam_xml_size = 0;
}

static void
arv_gv_device_finalize (GObject *object)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (object);
	ArvGvDeviceIOData *io_data;

	if (gv_device->priv->heartbeat_thread != NULL) {
		ArvGvDeviceHeartbeatData *heartbeat_data;

		heartbeat_data = gv_device->priv->heartbeat_data;

		heartbeat_data->cancel = TRUE;
		g_thread_join (gv_device->priv->heartbeat_thread);
		g_free (heartbeat_data);

		gv_device->priv->heartbeat_data = NULL;
		gv_device->priv->heartbeat_thread = NULL;
	}

	arv_gv_device_leave_control (gv_device);

	io_data = gv_device->priv->io_data;
	g_object_unref (io_data->device_address);
	g_object_unref (io_data->interface_address);
	g_object_unref (io_data->socket);
	g_free (io_data->buffer);
#if GLIB_CHECK_VERSION(2,32,0)
	g_mutex_clear (&io_data->mutex);
#else
	g_mutex_free (io_data->mutex);
#endif

	g_free (gv_device->priv->io_data);

	if (gv_device->priv->genicam != NULL)
		g_object_unref (gv_device->priv->genicam);

	g_free (gv_device->priv->genicam_xml);

	parent_class->finalize (object);
}

/**
 * arv_gv_device_get_interface_address:
 * @device: a #ArvGvDevice
 *
 * Returns: (transfer none): the device host interface IP address.
 * 
 * Since: 0.2.0
 */

GSocketAddress *arv_gv_device_get_interface_address(ArvGvDevice *device)
{
	ArvGvDeviceIOData *io_data = device->priv->io_data;

	return io_data->interface_address;
}

/**
 * arv_gv_device_get_device_address:
 * @device: a #ArvGvDevice
 *
 * Returns: (transfer none): the device IP address.
 *
 * since: 0.2.0
 */

GSocketAddress *arv_gv_device_get_device_address(ArvGvDevice *device)
{
	ArvGvDeviceIOData *io_data = device->priv->io_data;

	return io_data->device_address;
}

static void
arv_gv_device_class_init (ArvGvDeviceClass *gv_device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (gv_device_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (gv_device_class);

	g_type_class_add_private (gv_device_class, sizeof (ArvGvDevicePrivate));

	parent_class = g_type_class_peek_parent (gv_device_class);

	object_class->finalize = arv_gv_device_finalize;

	device_class->create_stream = arv_gv_device_create_stream;
	device_class->get_genicam_xml = arv_gv_device_get_genicam_xml;
	device_class->get_genicam = arv_gv_device_get_genicam;
	device_class->read_memory = arv_gv_device_read_memory;
	device_class->write_memory = arv_gv_device_write_memory;
	device_class->read_register = arv_gv_device_read_register;
	device_class->write_register = arv_gv_device_write_register;
}

G_DEFINE_TYPE (ArvGvDevice, arv_gv_device, ARV_TYPE_DEVICE)
