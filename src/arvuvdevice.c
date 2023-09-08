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
 * SECTION: arvuvdevice
 * @short_description: USB3Vision device
 */

#include <arvuvstreamprivate.h>
#include <arvuvdeviceprivate.h>
#include <arvuvinterfaceprivate.h>
#include <arvuvcpprivate.h>
#include <arvgc.h>
#include <arvdebug.h>
#include <arvenumtypes.h>
#include <libusb.h>
#include <string.h>
#include <arvstr.h>
#include <arvzip.h>
#include <arvmisc.h>

enum
{
	PROP_0,
	PROP_UV_DEVICE_VENDOR,
	PROP_UV_DEVICE_PRODUCT,
	PROP_UV_DEVICE_SERIAL_NUMBER,
	PROP_UV_DEVICE_GUID
};

#define ARV_UV_DEVICE_N_TRIES_MAX	5

typedef struct {
	char *vendor;
	char *product;
	char *serial_number;
	char *guid;

	libusb_context *usb;
	libusb_device_handle *usb_device;

       	libusb_hotplug_callback_handle hotplug_cb_handle;

	ArvGc *genicam;

	char *genicam_xml;
	size_t genicam_xml_size;

	guint16 packet_id;

	guint timeout_ms;
	guint cmd_packet_size_max;
	guint ack_packet_size_max;
	guint control_interface;
	guint data_interface;
        guint8 control_endpoint;
        guint8 data_endpoint;
	gboolean disconnected;

	ArvUvUsbMode usb_mode;

	int event_thread_run;
	GThread* event_thread;

        GMutex transfer_mutex;
} ArvUvDevicePrivate;

struct _ArvUvDevice {
	ArvDevice device;
};

struct _ArvUvDeviceClass {
	ArvDeviceClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvUvDevice, arv_uv_device, ARV_TYPE_DEVICE, G_ADD_PRIVATE (ArvUvDevice))

static ArvDeviceError
arv_uvcp_status_to_device_error (ArvUvcpStatus status)
{
        switch (status) {
                case ARV_UVCP_STATUS_NOT_IMPLEMENTED:
                        return ARV_DEVICE_ERROR_PROTOCOL_ERROR_NOT_IMPLEMENTED;
                case ARV_UVCP_STATUS_INVALID_PARAMETER:
                        return ARV_DEVICE_ERROR_PROTOCOL_ERROR_INVALID_PARAMETER;
                case ARV_UVCP_STATUS_INVALID_ADDRESS:
                        return ARV_DEVICE_ERROR_PROTOCOL_ERROR_INVALID_ADDRESS;
                case ARV_UVCP_STATUS_WRITE_PROTECT:
                        return ARV_DEVICE_ERROR_PROTOCOL_ERROR_WRITE_PROTECT;
                case ARV_UVCP_STATUS_BAD_ALIGNMENT:
                        return ARV_DEVICE_ERROR_PROTOCOL_ERROR_BAD_ALIGNMENT;
                case ARV_UVCP_STATUS_ACCESS_DENIED:
                        return ARV_DEVICE_ERROR_PROTOCOL_ERROR_ACCESS_DENIED;
                case ARV_UVCP_STATUS_BUSY:
                        return ARV_DEVICE_ERROR_PROTOCOL_ERROR_BUSY;
                default:
                        break;
        }

        return ARV_DEVICE_ERROR_PROTOCOL_ERROR;
}

/* ArvDevice implementation */

/* ArvUvDevice implementation */

gboolean
arv_uv_device_is_connected (ArvUvDevice *uv_device)
{
        ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (uv_device);

        return !priv->disconnected;
}

void
arv_uv_device_fill_bulk_transfer (struct libusb_transfer* transfer, ArvUvDevice *uv_device,
                                  ArvUvEndpointType endpoint_type, unsigned char endpoint_flags,
                                  void *data, size_t size,
                                  libusb_transfer_cb_fn callback, void* callback_data,
                                  unsigned int timeout)
{
        ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (uv_device);
        guint8 endpoint;

	endpoint = (endpoint_type == ARV_UV_ENDPOINT_CONTROL) ? priv->control_endpoint : priv->data_endpoint;

        libusb_fill_bulk_transfer (transfer, priv->usb_device, endpoint | endpoint_flags, data, size,
                                   callback, callback_data, timeout );
}

gboolean
arv_uv_device_bulk_transfer (ArvUvDevice *uv_device, ArvUvEndpointType endpoint_type, unsigned char endpoint_flags, void *data,
			     size_t size, size_t *transferred_size, guint32 timeout_ms, GError **error)
{
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (uv_device);
	gboolean success;
	guint8 endpoint;
	int transferred = 0;
	int result;

	g_return_val_if_fail (ARV_IS_UV_DEVICE (uv_device), FALSE);
	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (size > 0, FALSE);

	if (priv->disconnected) {
		g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_CONNECTED,
			     "Not connected");
		return FALSE;
	}


	endpoint = (endpoint_type == ARV_UV_ENDPOINT_CONTROL) ? priv->control_endpoint : priv->data_endpoint;
	result = libusb_bulk_transfer (priv->usb_device, endpoint | endpoint_flags, data, size, &transferred,
				       timeout_ms > 0 ? timeout_ms : priv->timeout_ms);

	success = result >= 0;

	if (!success)
		g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_TRANSFER_ERROR,
			     "%s", libusb_error_name (result));

	if (transferred_size != NULL)
		*transferred_size = transferred;

	if (result == LIBUSB_ERROR_NO_DEVICE) {
                if (!priv->disconnected) {
                        priv->disconnected = TRUE;
                        arv_device_emit_control_lost_signal (ARV_DEVICE (uv_device));
                }
        }

	return success;
}

static ArvStream *
arv_uv_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data, GDestroyNotify destroy, GError **error)
{
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (ARV_UV_DEVICE (device));
	return arv_uv_stream_new (ARV_UV_DEVICE (device), callback, user_data, destroy, priv->usb_mode, error);
}

static gboolean
_send_cmd_and_receive_ack (ArvUvDevice *uv_device, ArvUvcpCommand command,
			   guint64 address, guint32 size, void *buffer, GError **error)
{
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (uv_device);
	ArvUvcpCommand expected_ack_command;
	ArvUvcpPacket *ack_packet;
	ArvUvcpPacket *packet;
	const char *operation;
	size_t packet_size;
	size_t ack_size;
	unsigned n_tries = 0;
	gboolean success = FALSE;
	ArvUvcpStatus status = ARV_UVCP_STATUS_SUCCESS;

	switch (command) {
		case ARV_UVCP_COMMAND_READ_MEMORY_CMD:
			operation = "read_memory";
			expected_ack_command = ARV_UVCP_COMMAND_READ_MEMORY_ACK;
			ack_size = arv_uvcp_packet_get_read_memory_ack_size (size);
			break;
		case ARV_UVCP_COMMAND_WRITE_MEMORY_CMD:
			operation = "write_memory";
			expected_ack_command = ARV_UVCP_COMMAND_WRITE_MEMORY_ACK;
			ack_size = arv_uvcp_packet_get_write_memory_ack_size ();
			break;
		default:
			g_assert_not_reached ();
	}

	if (ack_size > priv->ack_packet_size_max) {
		arv_info_device ("Invalid uv %s acknowledge packet size (%" G_GSIZE_FORMAT " / max: %d)",
				  operation, ack_size, priv->ack_packet_size_max);
		return FALSE;
	}

	switch (command) {
		case ARV_UVCP_COMMAND_READ_MEMORY_CMD:
			packet = arv_uvcp_packet_new_read_memory_cmd (address, size, 0, &packet_size);
			break;
		case ARV_UVCP_COMMAND_WRITE_MEMORY_CMD:
			packet = arv_uvcp_packet_new_write_memory_cmd (address, size, 0, &packet_size);
			break;
		default:
			g_assert_not_reached ();
	}

	if (packet_size > priv->cmd_packet_size_max) {
		arv_info_device ("Invalid us %s command packet size (%" G_GSIZE_FORMAT " / max: %d)",
				  operation, packet_size, priv->cmd_packet_size_max);
		arv_uvcp_packet_free (packet);
		return FALSE;
	}


	switch (command) {
		case ARV_UVCP_COMMAND_READ_MEMORY_CMD:
			break;
		case ARV_UVCP_COMMAND_WRITE_MEMORY_CMD:
			memcpy (arv_uvcp_packet_get_write_memory_cmd_data (packet), buffer, size);
			break;
		default:
			g_assert_not_reached ();
	}

	ack_packet = g_malloc (ack_size);

	g_mutex_lock (&priv->transfer_mutex);

	do {
		GError *local_error = NULL;
		size_t transferred;

		priv->packet_id = arv_uvcp_next_packet_id (priv->packet_id);
		arv_uvcp_packet_set_packet_id (packet, priv->packet_id);

		arv_uvcp_packet_debug (packet, ARV_DEBUG_LEVEL_DEBUG);

		success = TRUE;
		success = success && arv_uv_device_bulk_transfer (uv_device,
								  ARV_UV_ENDPOINT_CONTROL,
								  LIBUSB_ENDPOINT_OUT,
								  packet, packet_size,
								  NULL, 0, &local_error);
		if (success) {
			gint timeout_ms;
                        gint64 timeout_stop_ms;

			gboolean pending_ack;
			gboolean expected_answer;

			timeout_stop_ms = g_get_monotonic_time () / 1000 + priv->timeout_ms;

			do {
				pending_ack = FALSE;

				timeout_ms = timeout_stop_ms - g_get_monotonic_time () / 1000;
                                if (timeout_ms < 0)
                                        timeout_ms = 0;

				success = TRUE;
				success = success && arv_uv_device_bulk_transfer (uv_device,
										  ARV_UV_ENDPOINT_CONTROL,
										  LIBUSB_ENDPOINT_IN,
										  ack_packet, ack_size,
										  &transferred, timeout_ms,
										  &local_error);

				if (success) {
					ArvUvcpCommand ack_command;
					guint16 packet_id;

					arv_uvcp_packet_debug (ack_packet, ARV_DEBUG_LEVEL_DEBUG);

					status = arv_uvcp_packet_get_status (ack_packet);
					ack_command = arv_uvcp_packet_get_command (ack_packet);
					packet_id = arv_uvcp_packet_get_packet_id (ack_packet);

					if (ack_command == ARV_UVCP_COMMAND_PENDING_ACK) {
						gint64 pending_ack_timeout_ms;
						pending_ack = TRUE;
						expected_answer = FALSE;

						pending_ack_timeout_ms = arv_uvcp_packet_get_pending_ack_timeout (ack_packet);

						timeout_stop_ms = g_get_monotonic_time () / 1000 + pending_ack_timeout_ms;

						arv_debug_device ("[UvDevice::%s] Try %d/%d: "
                                                                  "pending ack timeout = %" G_GINT64_FORMAT,
                                                                  operation, n_tries + 1, ARV_UV_DEVICE_N_TRIES_MAX,
                                                                  pending_ack_timeout_ms);
					} if (status != ARV_UVCP_STATUS_SUCCESS) {
						expected_answer = ack_command == expected_ack_command &&
							packet_id == priv->packet_id;
						if (!expected_answer) {
							arv_info_device ("[[UvDevice::%s] Try %d/%d: "
                                                                         "unexpected answer (0x%04x)",
                                                                         operation, n_tries + 1,
                                                                         ARV_UV_DEVICE_N_TRIES_MAX,
									 status);
						}
					} else {
						expected_answer = status == ARV_UVCP_STATUS_SUCCESS &&
							ack_command == expected_ack_command &&
							packet_id == priv->packet_id;
						if (!expected_answer)
							arv_info_device ("[[UvDevice::%s] Try %d/%d: "
                                                                         "unexpected answer (0x%04x)",
                                                                         operation, n_tries + 1,
                                                                         ARV_UV_DEVICE_N_TRIES_MAX,
									 status);
					}
				} else {
					expected_answer = FALSE;
					if (local_error != NULL)
                                                arv_warning_device ("[UvDevice::%s] Try %d/%d: ack reception error: %s",
                                                                    operation, n_tries + 1, ARV_UV_DEVICE_N_TRIES_MAX,
                                                                    local_error->message);
                                        g_clear_error (&local_error);
                                }

			} while (pending_ack || (!expected_answer && timeout_ms));

			success = success && expected_answer;

			if (success && status == ARV_UVCP_STATUS_SUCCESS) {
				switch (command) {
					case ARV_UVCP_COMMAND_READ_MEMORY_CMD:
						memcpy (buffer, arv_uvcp_packet_get_read_memory_ack_data (ack_packet), size);
						break;
					case ARV_UVCP_COMMAND_WRITE_MEMORY_CMD:
						break;
					default:
						g_assert_not_reached();
				}
			}
		} else {
			if (local_error != NULL)
				arv_warning_device ("[UvDevice::%s] Try %d/%d: command sending error: %s",
                                                    operation, n_tries + 1, ARV_UV_DEVICE_N_TRIES_MAX,
                                                    local_error->message);
                        g_clear_error (&local_error);
                }

		n_tries++;
	} while (!success && n_tries < ARV_UV_DEVICE_N_TRIES_MAX);

	g_mutex_unlock (&priv->transfer_mutex);

	g_free (ack_packet);
	arv_uvcp_packet_free (packet);

	success = success && status == ARV_UVCP_STATUS_SUCCESS;

	if (!success) {
                if (status != ARV_UVCP_STATUS_SUCCESS)
                        g_set_error (error, ARV_DEVICE_ERROR, arv_uvcp_status_to_device_error (status),
                                     "USB3Vision %s error (%s)", operation,
                                     arv_uvcp_status_to_string (status));
                else
                        g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_TIMEOUT,
                                     "USB3Vision %s timeout", operation);
        }

	return success;
}

static gboolean
arv_uv_device_read_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	ArvUvDevice *uv_device = ARV_UV_DEVICE (device);
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (uv_device);
	int i;
	gint32 block_size;
	guint data_size_max;

	data_size_max = priv->ack_packet_size_max - sizeof (ArvUvcpHeader);

	for (i = 0; i < (size + data_size_max - 1) / data_size_max; i++) {
		block_size = MIN (data_size_max, size - i * data_size_max);
		if (!_send_cmd_and_receive_ack (uv_device, ARV_UVCP_COMMAND_READ_MEMORY_CMD,
						address + i * data_size_max,
						block_size, ((char *) buffer) + i * data_size_max, error))
			return FALSE;
	}

	return TRUE;
}

static gboolean
arv_uv_device_write_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	ArvUvDevice *uv_device = ARV_UV_DEVICE (device);
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (uv_device);
	int i;
	gint32 block_size;
	guint data_size_max;

	data_size_max = priv->ack_packet_size_max - sizeof (ArvUvcpHeader);

	for (i = 0; i < (size + data_size_max - 1) / data_size_max; i++) {
		block_size = MIN (data_size_max, size - i * data_size_max);
		if (!_send_cmd_and_receive_ack (uv_device, ARV_UVCP_COMMAND_WRITE_MEMORY_CMD,
						address + i * data_size_max,
						block_size, ((char *) buffer) + i * data_size_max, error))
			return FALSE;
	}

	return TRUE;
}

static gboolean
arv_uv_device_read_register (ArvDevice *device, guint64 address, guint32 *value, GError **error)
{
	return arv_uv_device_read_memory (device, address, sizeof (guint32), value, error);
}

static gboolean
arv_uv_device_write_register (ArvDevice *device, guint64 address, guint32 value, GError **error)
{
	return arv_uv_device_write_memory (device, address, sizeof (guint32), &value, error);
}

static gboolean
_bootstrap (ArvUvDevice *uv_device)
{
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (uv_device);
	ArvDevice *device = ARV_DEVICE (uv_device);
	guint64 offset;
	guint32 response_time;
	guint64 manifest_table_address;
	guint64 device_capability;
	guint32 max_cmd_transfer;
	guint32 max_ack_transfer;
	guint64 u3vcp_capability;
	guint64 sirm_offset;
	guint32 si_info;
	guint32 si_control;
	guint64 si_req_payload_size;
	guint32 si_req_leader_size;
	guint32 si_req_trailer_size;
	guint32 si_max_leader_size;
	guint32 si_payload_size;
	guint32 si_payload_count;
	guint32 si_transfer1_size;
	guint32 si_transfer2_size;
	guint32 si_max_trailer_size;
	guint64 manifest_n_entries;
	ArvUvcpManifestEntry entry;
	ArvUvcpManifestSchemaType schema_type;
	GString *string;
	void *data;
	char manufacturer[64];
	gboolean success = TRUE;

	arv_info_device ("Get genicam");

	success = success && arv_device_read_memory(device, ARV_ABRM_MANUFACTURER_NAME, 64, &manufacturer, NULL);
	if (!success) {
		arv_warning_device ("[UvDevice::_bootstrap] Error during memory read");
		return FALSE;
	}
	manufacturer[63] = 0;
	arv_info_device ("MANUFACTURER_NAME =        '%s'", manufacturer);

	success = success && arv_device_read_memory (device, ARV_ABRM_SBRM_ADDRESS,
                                                     sizeof (offset), &offset, NULL);
	success = success && arv_device_read_memory (device, ARV_ABRM_MAX_DEVICE_RESPONSE_TIME,
                                                     sizeof (response_time), &response_time, NULL);
	success = success && arv_device_read_memory (device, ARV_ABRM_DEVICE_CAPABILITY,
                                                     sizeof (device_capability), &device_capability, NULL);
	success = success && arv_device_read_memory (device, ARV_ABRM_MANIFEST_TABLE_ADDRESS,
                                                     sizeof (manifest_table_address), &manifest_table_address, NULL);
	if (!success) {
		arv_warning_device ("[UvDevice::_bootstrap] Error during memory read");
		return FALSE;
	}

	arv_info_device ("MAX_DEVICE_RESPONSE_TIME = 0x%08x", response_time);
	arv_info_device ("DEVICE_CAPABILITY        = 0x%016" G_GINT64_MODIFIER "x", device_capability);
	arv_info_device ("SRBM_ADDRESS =             0x%016" G_GINT64_MODIFIER "x", offset);
	arv_info_device ("MANIFEST_TABLE_ADDRESS =   0x%016" G_GINT64_MODIFIER "x", manifest_table_address);

	priv->timeout_ms = MAX (ARV_UVCP_DEFAULT_RESPONSE_TIME_MS, response_time);

	success = success && arv_device_read_memory (device, offset + ARV_SBRM_U3VCP_CAPABILITY,
                                                     sizeof (u3vcp_capability), &u3vcp_capability, NULL);
	success = success && arv_device_read_memory (device, offset + ARV_SBRM_MAX_CMD_TRANSFER,
                                                     sizeof (max_cmd_transfer), &max_cmd_transfer, NULL);
	success = success && arv_device_read_memory (device, offset + ARV_SBRM_MAX_ACK_TRANSFER,
                                                     sizeof (max_ack_transfer), &max_ack_transfer, NULL);
	success = success && arv_device_read_memory (device, offset + ARV_SBRM_SIRM_ADDRESS,
                                                     sizeof (sirm_offset), &sirm_offset, NULL);
	if (!success) {
		arv_warning_device ("[UvDevice::_bootstrap] Error during memory read");
		return FALSE;
	}

	arv_info_device ("U3VCP_CAPABILITY =         0x%016" G_GINT64_MODIFIER "x", u3vcp_capability);
	arv_info_device ("MAX_CMD_TRANSFER =         0x%08x", max_cmd_transfer);
	arv_info_device ("MAX_ACK_TRANSFER =         0x%08x", max_ack_transfer);
	arv_info_device ("SIRM_OFFSET =              0x%016" G_GINT64_MODIFIER "x", sirm_offset);

	priv->cmd_packet_size_max = MIN (priv->cmd_packet_size_max, max_cmd_transfer);
	priv->ack_packet_size_max = MIN (priv->ack_packet_size_max, max_ack_transfer);

	success = success && arv_device_read_memory (device, sirm_offset + ARV_SIRM_INFO,
                                                     sizeof (si_info), &si_info, NULL);
	success = success && arv_device_read_memory (device, sirm_offset + ARV_SIRM_CONTROL,
                                                     sizeof (si_control), &si_control, NULL);
	success = success && arv_device_read_memory (device, sirm_offset + ARV_SIRM_REQ_PAYLOAD_SIZE,
                                                     sizeof (si_req_payload_size), &si_req_payload_size, NULL);
	success = success && arv_device_read_memory (device, sirm_offset + ARV_SIRM_REQ_LEADER_SIZE,
                                                     sizeof (si_req_leader_size), &si_req_leader_size, NULL);
	success = success && arv_device_read_memory (device, sirm_offset + ARV_SIRM_REQ_TRAILER_SIZE,
                                                     sizeof (si_req_trailer_size), &si_req_trailer_size, NULL);
	success = success && arv_device_read_memory (device, sirm_offset + ARV_SIRM_MAX_LEADER_SIZE,
                                                     sizeof (si_max_leader_size), &si_max_leader_size, NULL);
	success = success && arv_device_read_memory (device, sirm_offset + ARV_SIRM_PAYLOAD_SIZE,
                                                     sizeof (si_payload_size), &si_payload_size, NULL);
	success = success && arv_device_read_memory (device, sirm_offset + ARV_SIRM_PAYLOAD_COUNT,
                                                     sizeof (si_payload_count), &si_payload_count, NULL);
	success = success && arv_device_read_memory (device, sirm_offset + ARV_SIRM_TRANSFER1_SIZE,
                                                     sizeof (si_transfer1_size), &si_transfer1_size, NULL);
	success = success && arv_device_read_memory (device, sirm_offset + ARV_SIRM_TRANSFER2_SIZE,
                                                     sizeof (si_transfer2_size), &si_transfer2_size, NULL);
	success = success && arv_device_read_memory (device, sirm_offset + ARV_SIRM_MAX_TRAILER_SIZE,
                                                     sizeof (si_max_trailer_size), &si_max_trailer_size, NULL);
	if (!success) {
		arv_warning_device ("[UvDevice::_bootstrap] Error during memory read");
		return FALSE;
	}

	arv_info_device ("SIRM_INFO =                0x%08x", si_info);
	arv_info_device ("SIRM_CONTROL =             0x%08x", si_control);
	arv_info_device ("SIRM_REQ_PAYLOAD_SIZE =    0x%016" G_GINT64_MODIFIER "x", si_req_payload_size);
	arv_info_device ("SIRM_REQ_LEADER_SIZE =     0x%08x", si_req_leader_size);
	arv_info_device ("SIRM_REQ_TRAILER_SIZE =    0x%08x", si_req_trailer_size);
	arv_info_device ("SIRM_MAX_LEADER_SIZE =     0x%08x", si_max_leader_size);
	arv_info_device ("SIRM_PAYLOAD_SIZE =        0x%08x", si_payload_size);
	arv_info_device ("SIRM_PAYLOAD_COUNT =       0x%08x", si_payload_count);
	arv_info_device ("SIRM_TRANSFER1_SIZE =      0x%08x", si_transfer1_size);
	arv_info_device ("SIRM_TRANSFER2_SIZE =      0x%08x", si_transfer2_size);
	arv_info_device ("SIRM_MAX_TRAILER_SIZE =    0x%08x", si_max_trailer_size);

	success = success && arv_device_read_memory (device, manifest_table_address,
                                                     sizeof (manifest_n_entries), &manifest_n_entries, NULL);
	success = success && arv_device_read_memory (device, manifest_table_address + 0x08,
                                                     sizeof (entry), &entry, NULL);
	if (!success) {
		arv_warning_device ("[UvDevice::_bootstrap] Error during memory read");
		return FALSE;
	}

	arv_info_device ("MANIFEST_N_ENTRIES =       0x%016" G_GINT64_MODIFIER "x", manifest_n_entries);

	string = g_string_new ("");
	arv_g_string_append_hex_dump (string, &entry, sizeof (entry));
	arv_info_device ("MANIFEST ENTRY\n%s", string->str);
	g_string_free (string, TRUE);

	arv_info_device ("genicam address =          0x%016" G_GINT64_MODIFIER "x", entry.address);
	arv_info_device ("genicam size    =          0x%016" G_GINT64_MODIFIER "x", entry.size);

	data = g_malloc0 (entry.size);
	success = success && arv_device_read_memory (device, entry.address, entry.size, data, NULL);
	if (!success){
		arv_warning_device ("[UvDevice::_bootstrap] Error during memory read");
		g_free(data);
		return FALSE;
	}

#if 0
	string = g_string_new ("");
	arv_g_string_append_hex_dump (string, data, entry.size);
	arv_info_device ("GENICAM\n%s", string->str);
	g_string_free (string, TRUE);
#endif

	schema_type = arv_uvcp_manifest_entry_get_schema_type (&entry);

	switch (schema_type) {
		case ARV_UVCP_SCHEMA_ZIP:
			{
				ArvZip *zip;
				const GSList *zip_files;

				zip = arv_zip_new (data, entry.size);
				zip_files = arv_zip_get_file_list (zip);

				if (zip_files != NULL) {
					const char *zip_filename;

					zip_filename = arv_zip_file_get_name (zip_files->data);
					priv->genicam_xml = arv_zip_get_file (zip,
											 zip_filename,
											 &priv->genicam_xml_size);

					arv_info_device ("zip file =                 %s", zip_filename);

#if 0
					string = g_string_new ("");
					arv_g_string_append_hex_dump (string, priv->genicam_xml,
								      priv->genicam_xml_size);
					arv_info_device ("GENICAM\n%s", string->str);
					g_string_free (string, TRUE);
#endif

					priv->genicam = arv_gc_new (ARV_DEVICE (uv_device),
								    priv->genicam_xml,
								    priv->genicam_xml_size);
				}

				arv_zip_free (zip);
				g_free (data);
			}
			break;
		case ARV_UVCP_SCHEMA_RAW:
			{
				priv->genicam_xml = data;
				priv->genicam_xml_size = entry.size;
				priv->genicam = arv_gc_new (ARV_DEVICE (uv_device),
							    priv->genicam_xml,
							    priv->genicam_xml_size);
			}
			break;
		default:
			arv_warning_device ("Unknown USB3Vision manifest schema type (%d)", schema_type);
	}

#if 0
	arv_info_device("GENICAM\n:%s", priv->genicam_xml);
#endif

	return TRUE;
}

static ArvGc *
arv_uv_device_get_genicam (ArvDevice *device)
{
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (ARV_UV_DEVICE (device));

	return priv->genicam;
}

static const char *
arv_uv_device_get_genicam_xml (ArvDevice *device, size_t *size)
{
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (ARV_UV_DEVICE (device));

	if (size != NULL)
		*size = priv->genicam_xml_size;

	return priv->genicam_xml;
}

static void
reset_endpoint (libusb_device_handle *usb_device, guint8 endpoint, guint8 endpoint_flags)
{
	int errcode;

	/* Set endpoint in halt condition */
	errcode = libusb_control_transfer(usb_device,
					      LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_ENDPOINT,
					      LIBUSB_REQUEST_SET_FEATURE,
					      0, /* Value: 0=endpoint_halt */
					      endpoint | endpoint_flags,
					      0, 0,
					      1000);
	if (errcode < 0) {
		arv_warning_device("Failed to set endpoint %x in halt condition: %s",
				   endpoint|endpoint_flags, libusb_error_name (errcode));
		return;
	}

	/* Clear halt condtion on the endpoint, effectivelly resetting the pipe */
	errcode = libusb_clear_halt(usb_device, endpoint | endpoint_flags);
	if (errcode < 0) {
		arv_warning_device("Failed to clear halt contidion on endpoint: %s",
				   libusb_error_name (errcode));
		return;
	}
}

gboolean
arv_uv_device_reset_stream_endpoint (ArvUvDevice *device)
{
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (ARV_UV_DEVICE (device));

        g_return_val_if_fail(ARV_IS_UV_DEVICE(device), FALSE);

        reset_endpoint (priv->usb_device, priv->data_endpoint, LIBUSB_ENDPOINT_IN);

        return TRUE;
}

static int
get_guid_index(libusb_device * device) {
	struct libusb_config_descriptor *config;
	const struct libusb_interface *inter;
	const struct libusb_interface_descriptor *interdesc;
	int guid_index = -1;
	int i, j;

	libusb_get_config_descriptor (device, 0, &config);
	for (i = 0; i < (int) config->bNumInterfaces; i++) {
		inter = &config->interface[i];
		for (j = 0; j < inter->num_altsetting; j++) {
			interdesc = &inter->altsetting[j];
			if (interdesc->bInterfaceClass == ARV_UV_INTERFACE_INTERFACE_CLASS &&
			    interdesc->bInterfaceSubClass == ARV_UV_INTERFACE_INTERFACE_SUBCLASS) {
				if (interdesc->bInterfaceProtocol == ARV_UV_INTERFACE_CONTROL_PROTOCOL &&
				    interdesc->extra &&
				    interdesc->extra_length >= ARV_UV_INTERFACE_GUID_INDEX_OFFSET + sizeof(unsigned char)) {
					guid_index = (int) (*(interdesc->extra + ARV_UV_INTERFACE_GUID_INDEX_OFFSET));
				}
			}
		}
	}
	libusb_free_config_descriptor (config);

	return guid_index;
}

static gboolean
_open_usb_device (ArvUvDevice *uv_device, GError **error)
{
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (uv_device);
	libusb_device **devices;
	unsigned i, j, k;
	ssize_t count;

	count = libusb_get_device_list (priv->usb, &devices);
	if (count < 0) {
		g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
			     "Failed to get USB device list: %s", libusb_error_name (count));
		return FALSE;
	}

	for (i = 0; i < count && priv->usb_device == NULL; i++) {
		libusb_device_handle *usb_device;
		struct libusb_device_descriptor desc;

		if (libusb_get_device_descriptor (devices[i], &desc) >= 0 &&
		    libusb_open (devices[i], &usb_device) == LIBUSB_SUCCESS) {
			unsigned char *manufacturer;
			unsigned char *product;
			unsigned char *serial_number;
			unsigned char *guid;
			int index;

			manufacturer = g_malloc0 (256);
			product = g_malloc0 (256);
			serial_number = g_malloc0 (256);
			guid = g_malloc0 (256);

			index = desc.iManufacturer;
			if (index > 0)
				libusb_get_string_descriptor_ascii (usb_device, index, manufacturer, 256);
			index = desc.iProduct;
			if (index > 0)
				libusb_get_string_descriptor_ascii (usb_device, index, product, 256);
			index = desc.iSerialNumber;
			if (index > 0)
				libusb_get_string_descriptor_ascii (usb_device, index, serial_number, 256);
			index = get_guid_index(devices[i]);
			if (index > 0)
				libusb_get_string_descriptor_ascii (usb_device, index, guid, 256);

			if ((priv->vendor != NULL &&
                             g_strcmp0 ((char * ) manufacturer, priv->vendor) == 0 &&
                             priv->product != NULL &&
                             g_strcmp0 ((char * ) product, priv->product) == 0 &&
                             priv->serial_number != NULL &&
                             g_strcmp0 ((char * ) serial_number, priv->serial_number) == 0) ||
                            (priv->guid != NULL &&
                             g_strcmp0 ((char * ) guid, priv->guid) == 0)) {
                            struct libusb_config_descriptor *config;
                            struct libusb_endpoint_descriptor endpoint;
                            const struct libusb_interface *inter;
                            const struct libusb_interface_descriptor *interdesc;
                            int result;

				priv->usb_device = usb_device;

                                result = libusb_set_auto_detach_kernel_driver (usb_device, 1);
                                if (result != 0) {
                                        arv_warning_device ("Failed to set auto kernel detach feature "
                                                            "for USB device '%s-%s-%s': %s",
                                                            priv->vendor, priv->product, priv->serial_number,
                                                            libusb_error_name (result));
                                }

				libusb_get_config_descriptor (devices[i], 0, &config);
				for (j = 0; j < (int) config->bNumInterfaces; j++) {
					inter = &config->interface[j];
					for (k = 0; k < inter->num_altsetting; k++) {
						interdesc = &inter->altsetting[k];
						if (interdesc->bInterfaceClass == ARV_UV_INTERFACE_INTERFACE_CLASS &&
						    interdesc->bInterfaceSubClass == ARV_UV_INTERFACE_INTERFACE_SUBCLASS) {
							if (interdesc->bInterfaceProtocol == ARV_UV_INTERFACE_CONTROL_PROTOCOL) {
								endpoint = interdesc->endpoint[0];
								priv->control_endpoint = endpoint.bEndpointAddress & 0x0f;
								priv->control_interface = interdesc->bInterfaceNumber;
							}
							if (interdesc->bInterfaceProtocol == ARV_UV_INTERFACE_DATA_PROTOCOL) {
								endpoint = interdesc->endpoint[0];
								priv->data_endpoint = endpoint.bEndpointAddress & 0x0f;
								priv->data_interface = interdesc->bInterfaceNumber;
							}
						}
					}
				}
				libusb_free_config_descriptor (config);
			} else
				libusb_close (usb_device);

			g_free (manufacturer);
			g_free (product);
			g_free (serial_number);
			g_free (guid);
		}
	}

	libusb_free_device_list (devices, 1);

	if (priv->usb_device == NULL) {
		g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
			     "USB device '%s:%s:%s' not found", priv->vendor, priv->product, priv->serial_number);
		return FALSE;
	}

	return TRUE;
}

static gpointer
event_thread_func(void *p)
{
	ArvUvDevicePrivate *priv = (ArvUvDevicePrivate *)p;

	struct timeval tv = { 0, 100000 };

        while (priv->event_thread_run)
        {
                libusb_handle_events_timeout(priv->usb, &tv);
        }

        return NULL;
}

/**
 * arv_uv_device_set_usb_mode:
 * @uv_device: a #ArvUvDevice
 * @usb_mode: a #ArvUvUsbMode option
 *
 * Sets the option to utilize the USB synchronous or asynchronous device I/O API. The default mode is
 * @ARV_UV_USB_MODE_SYNC, which means USB bulk transfer will be synchronously executed. This mode is qualified to work,
 * but it has the performance issue with some high framerate device. Using @ARV_UV_USB_MODE_ASYNC possibly improves the
 * bandwidth.
 *
 * Since: 0.8.17
 */

void
arv_uv_device_set_usb_mode (ArvUvDevice *uv_device, ArvUvUsbMode usb_mode)
{
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (uv_device);

	g_return_if_fail (ARV_IS_UV_DEVICE (uv_device));

	priv->usb_mode = usb_mode;
}

/**
 * arv_uv_device_new:
 * @vendor: USB3 vendor string
 * @product: USB3 product string
 * @serial_number: device serial number
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: a newly created #ArvDevice using USB3 based protocol
 *
 * Since: 0.8.0
 */

ArvDevice *
arv_uv_device_new (const char *vendor, const char *product, const char *serial_number, GError **error)
{
	return g_initable_new (ARV_TYPE_UV_DEVICE, NULL, error,
			       "vendor", vendor,
			       "product", product,
			       "serial-number", serial_number,
			       NULL);
}

/**
 * arv_uv_device_new_from_guid:
 * @guid: device GUID
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: a newly created #ArvDevice using USB3 based protocol
 *
 * Since: 0.8.17
 */

ArvDevice *
arv_uv_device_new_from_guid (const char *guid, GError **error)
{
	return g_initable_new (ARV_TYPE_UV_DEVICE, NULL, error,
			       "guid", guid,
			       NULL);
}

static int LIBUSB_CALL _disconnect_event (libusb_context *ctx,
                              libusb_device *device,
                              libusb_hotplug_event event,
                              void *user_data)
{
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (ARV_UV_DEVICE (user_data));

        if (device == libusb_get_device (priv->usb_device)) {
                if (!priv->disconnected) {
                        priv->disconnected = TRUE;
                        arv_device_emit_control_lost_signal (ARV_DEVICE (user_data));
                }
        }

        return 0;
}

static void
arv_uv_device_constructed (GObject *object)
{
	ArvUvDevice *uv_device = ARV_UV_DEVICE (object);
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (uv_device);
	GError *error = NULL;
        int result;

        G_OBJECT_CLASS (arv_uv_device_parent_class)->constructed (object);

        g_mutex_init (&priv->transfer_mutex);

	result = libusb_init (&priv->usb);
        if (result != 0) {
                arv_device_take_init_error (ARV_DEVICE (uv_device),
                                            g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                                                         "Failed to initialize USB library: %s",
                                                         libusb_error_name (result)));
                return;
        }

        if (priv->vendor != NULL)
                arv_info_device ("[UvDevice::new] Vendor  = %s", priv->vendor);
        if (priv->product != NULL)
                arv_info_device ("[UvDevice::new] Product = %s", priv->product);
        if (priv->serial_number != NULL)
                arv_info_device ("[UvDevice::new] S/N     = %s", priv->serial_number);
        if (priv->guid != NULL)
                arv_info_device ("[UvDevice::new] GUID    = %s", priv->guid);

	priv->packet_id = 65300; /* Start near the end of the circular counter */
	priv->timeout_ms = 32;

	if (!_open_usb_device (uv_device, &error)) {
		arv_device_take_init_error (ARV_DEVICE (uv_device), error);
                return;
	}

	arv_info_device("[UvDevice::new] Using control endpoint %d, interface %d",
			 priv->control_endpoint, priv->control_interface);
	arv_info_device("[UvDevice::new] Using data endpoint %d, interface %d",
			 priv->data_endpoint, priv->data_interface);

        result = libusb_claim_interface (priv->usb_device, priv->control_interface);
        if (result != 0) {
                arv_device_take_init_error (ARV_DEVICE (uv_device),
                                            g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                                                         "Failed to claim USB control interface to '%s-%s-%s-%s': %s",
                                                         priv->vendor, priv->product, priv->serial_number, priv->guid,
                                                         libusb_error_name (result)));
                return;
        }

        result = libusb_claim_interface (priv->usb_device, priv->data_interface);
        if (result != 0) {
                arv_device_take_init_error (ARV_DEVICE (uv_device),
                                            g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                                                         "Failed to claim USB data interface to '%s-%s-%s-%s': %s",
                                                         priv->vendor, priv->product, priv->serial_number, priv->guid,
                                                         libusb_error_name (result)));
                return;
        }

	if ( !_bootstrap (uv_device)){
		arv_device_take_init_error (ARV_DEVICE (uv_device),
                                            g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                                                         "Failed to bootstrap USB device '%s-%s-%s-%s'",
                                                         priv->vendor, priv->product, priv->serial_number, priv->guid));
                return;
        }

	if (!ARV_IS_GC (priv->genicam)) {
		arv_device_take_init_error (ARV_DEVICE (uv_device),
                                            g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_GENICAM_NOT_FOUND,
                                                         "Failed to load Genicam data for USB device '%s-%s-%s-%s'",
                                                         priv->vendor, priv->product, priv->serial_number, priv->guid));
                return;
        }

        libusb_hotplug_register_callback (priv->usb, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0,
                                          LIBUSB_HOTPLUG_MATCH_ANY,
                                          LIBUSB_HOTPLUG_MATCH_ANY,
                                          LIBUSB_HOTPLUG_MATCH_ANY,
                                          _disconnect_event,
                                          uv_device,
                                          &priv->hotplug_cb_handle);

	priv->usb_mode = ARV_UV_USB_MODE_DEFAULT;

	priv->event_thread_run = 1;
	priv->event_thread = g_thread_new ( "arv_libusb", event_thread_func, priv);
}

static void
arv_uv_device_init (ArvUvDevice *uv_device)
{
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (uv_device);

	priv->cmd_packet_size_max = 65536 + sizeof (ArvUvcpHeader);
	priv->ack_packet_size_max = 65536 + sizeof (ArvUvcpHeader);
	priv->disconnected = FALSE;
}

static void
arv_uv_device_finalize (GObject *object)
{
	ArvUvDevice *uv_device = ARV_UV_DEVICE (object);

	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (uv_device);

        libusb_hotplug_deregister_callback (priv->usb, priv->hotplug_cb_handle);

	priv->event_thread_run = 0;
        if (priv->event_thread)
                g_thread_join (priv->event_thread);

	g_clear_object (&priv->genicam);

	g_clear_pointer (&priv->vendor, g_free);
	g_clear_pointer (&priv->product, g_free);
	g_clear_pointer (&priv->serial_number, g_free);
        g_clear_pointer (&priv->guid, g_free);
	g_clear_pointer (&priv->genicam_xml, g_free);
	if (priv->usb_device != NULL) {
		libusb_release_interface (priv->usb_device, priv->control_interface);
		libusb_release_interface (priv->usb_device, priv->data_interface);
		libusb_close (priv->usb_device);
	}
        if (priv->usb != NULL)
                libusb_exit (priv->usb);
        g_mutex_clear (&priv->transfer_mutex);

	G_OBJECT_CLASS (arv_uv_device_parent_class)->finalize (object);
}

static void
arv_uv_device_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ArvUvDevicePrivate *priv = arv_uv_device_get_instance_private (ARV_UV_DEVICE (self));

	switch (prop_id)
	{
		case PROP_UV_DEVICE_VENDOR:
			g_free (priv->vendor);
			priv->vendor = g_value_dup_string (value);
			break;
		case PROP_UV_DEVICE_PRODUCT:
			g_free (priv->product);
			priv->product = g_value_dup_string (value);
			break;
		case PROP_UV_DEVICE_SERIAL_NUMBER:
			g_free (priv->serial_number);
			priv->serial_number = g_value_dup_string (value);
			break;
		case PROP_UV_DEVICE_GUID:
			g_free (priv->guid);
			priv->guid = g_value_dup_string (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
			break;
	}
}

static void
arv_uv_device_class_init (ArvUvDeviceClass *uv_device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (uv_device_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (uv_device_class);

	object_class->finalize = arv_uv_device_finalize;
	object_class->constructed = arv_uv_device_constructed;
	object_class->set_property = arv_uv_device_set_property;

	device_class->create_stream = arv_uv_device_create_stream;
	device_class->get_genicam_xml = arv_uv_device_get_genicam_xml;
	device_class->get_genicam = arv_uv_device_get_genicam;
	device_class->read_memory = arv_uv_device_read_memory;
	device_class->write_memory = arv_uv_device_write_memory;
	device_class->read_register = arv_uv_device_read_register;
	device_class->write_register = arv_uv_device_write_register;

	g_object_class_install_property
		(object_class,
		 PROP_UV_DEVICE_VENDOR,
		 g_param_spec_string ("vendor",
				      "Vendor",
				      "USB3 device vendor string",
				      NULL,
				      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property
		(object_class,
		 PROP_UV_DEVICE_PRODUCT,
		 g_param_spec_string ("product",
				      "Product",
				      "USB3 device product string",
				      NULL,
				      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property
		(object_class,
		 PROP_UV_DEVICE_SERIAL_NUMBER,
		 g_param_spec_string ("serial-number",
				      "Serial number",
				      "USB3 device serial number",
				      NULL,
				      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property
		(object_class,
		 PROP_UV_DEVICE_GUID,
		 g_param_spec_string ("guid",
				      "GUID",
				      "USB3 device GUID",
				      NULL,
				      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}
