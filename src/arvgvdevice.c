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

/**
 * SECTION: arvgvdevice
 * @short_description: GigEVision device
 */

#include <arvgvdeviceprivate.h>
#include <arvdeviceprivate.h>
#include <arvgc.h>
#include <arvgccommand.h>
#include <arvgcboolean.h>
#include <arvgcregisterdescriptionnode.h>
#include <arvdebug.h>
#include <arvgvstreamprivate.h>
#include <arvgvcpprivate.h>
#include <arvgvspprivate.h>
#include <arvzip.h>
#include <arvstr.h>
#include <arvmisc.h>
#include <arvenumtypes.h>
#include <string.h>
#include <stdlib.h>
#ifndef __APPLE__
#include <linux/ip.h>
#endif
#include <netinet/udp.h>

/* Shared data (main thread - heartbeat) */

#ifdef __APPLE__
struct iphdr
  {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ihl:4;
    unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error  "Please fix <bits/endian.h>"
#endif
    u_int8_t tos;
    u_int16_t tot_len;
    u_int16_t id;
    u_int16_t frag_off;
    u_int8_t ttl;
    u_int8_t protocol;
    u_int16_t check;
    u_int32_t saddr;
    u_int32_t daddr;
    /*The options start here. */
  };
#endif

enum
{
	PROP_0,
	PROP_GV_DEVICE_INTERFACE_ADDRESS,
	PROP_GV_DEVICE_DEVICE_ADDRESS
};

typedef struct {
	GMutex mutex;

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

typedef struct {
	GInetAddress *interface_address;
	GInetAddress *device_address;

	ArvGvDeviceIOData *io_data;

	void *heartbeat_thread;
	void *heartbeat_data;

	ArvGc *genicam;

	char *genicam_xml;
	size_t genicam_xml_size;

	gboolean is_packet_resend_supported;
	gboolean is_write_memory_supported;

	ArvGvStreamOption stream_options;
} ArvGvDevicePrivate ;

struct _ArvGvDevice {
	ArvDevice device;
};

struct _ArvGvDeviceClass {
	ArvDeviceClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvGvDevice, arv_gv_device, ARV_TYPE_DEVICE, G_ADD_PRIVATE (ArvGvDevice))

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
_send_cmd_and_receive_ack (ArvGvDeviceIOData *io_data, ArvGvcpCommand command,
			   guint64 address, size_t size, void *buffer, GError **error)
{
	ArvGvcpCommand ack_command;
	ArvGvcpPacket *ack_packet = io_data->buffer;
	ArvGvcpPacket *packet;
	const char *operation;
	size_t packet_size;
	size_t ack_size;
	unsigned int n_retries = 0;
	gboolean success = FALSE;
	ArvGvcpError command_error = ARV_GVCP_ERROR_NONE;
	int count;

	switch (command) {
		case ARV_GVCP_COMMAND_READ_MEMORY_CMD:
			operation = "read_memory";
			ack_command = ARV_GVCP_COMMAND_READ_MEMORY_ACK;
			ack_size = arv_gvcp_packet_get_read_memory_ack_size (size);
			break;
		case ARV_GVCP_COMMAND_WRITE_MEMORY_CMD:
			operation = "write_memory";
			ack_command = ARV_GVCP_COMMAND_WRITE_MEMORY_ACK;
			ack_size = arv_gvcp_packet_get_write_memory_ack_size ();
			break;
		case ARV_GVCP_COMMAND_READ_REGISTER_CMD:
			operation = "read_register";
			ack_command = ARV_GVCP_COMMAND_READ_REGISTER_ACK;
			ack_size = arv_gvcp_packet_get_read_register_ack_size ();
			break;
		case ARV_GVCP_COMMAND_WRITE_REGISTER_CMD:
			operation = "write_register";
			ack_command = ARV_GVCP_COMMAND_WRITE_REGISTER_ACK;
			ack_size = arv_gvcp_packet_get_write_register_ack_size ();
			break;
		default:
			g_assert_not_reached ();
	}

	g_return_val_if_fail (ack_size <= ARV_GV_DEVICE_BUFFER_SIZE, FALSE);

	g_mutex_lock (&io_data->mutex);

	io_data->packet_id = arv_gvcp_next_packet_id (io_data->packet_id);

	switch (command) {
		case ARV_GVCP_COMMAND_READ_MEMORY_CMD:
			packet = arv_gvcp_packet_new_read_memory_cmd (address, size,
								      io_data->packet_id, &packet_size);
			break;
		case ARV_GVCP_COMMAND_WRITE_MEMORY_CMD:
			packet = arv_gvcp_packet_new_write_memory_cmd (address, size, buffer,
								       io_data->packet_id, &packet_size);
			break;
		case ARV_GVCP_COMMAND_READ_REGISTER_CMD:
			packet = arv_gvcp_packet_new_read_register_cmd (address,
									io_data->packet_id, &packet_size);
			break;
		case ARV_GVCP_COMMAND_WRITE_REGISTER_CMD:
			packet = arv_gvcp_packet_new_write_register_cmd (address, *((guint32 *) buffer),
									 io_data->packet_id, &packet_size);
			break;
		default:
			g_assert_not_reached ();
	}

	do {
		GError *local_error = NULL;

		arv_gvcp_packet_debug (packet, ARV_DEBUG_LEVEL_LOG);

		success = g_socket_send_to (io_data->socket, io_data->device_address,
					    (const char *) packet, packet_size,
					    NULL, &local_error) >= 0;

		if (success) {
			gint timeout_ms;
			gint64 timeout_stop_ms;
			gboolean pending_ack;
			gboolean expected_answer;

			timeout_stop_ms = g_get_monotonic_time () / 1000 + io_data->gvcp_timeout_ms;

			do {
				pending_ack = FALSE;

				timeout_ms = MAX (0, timeout_stop_ms - g_get_monotonic_time () / 1000);

				success = TRUE;
				success = success && g_poll (&io_data->poll_in_event, 1, timeout_ms) > 0;

				if (success)
					count = g_socket_receive (io_data->socket, io_data->buffer,
								  ARV_GV_DEVICE_BUFFER_SIZE, NULL, &local_error);
				else
					count = 0;
				success = success && (count >= sizeof (ArvGvcpHeader));

				if (success) {
					ArvGvcpPacketType packet_type;
					ArvGvcpCommand command;
					guint16 packet_id;

					arv_gvcp_packet_debug (ack_packet, ARV_DEBUG_LEVEL_LOG);

					packet_type = arv_gvcp_packet_get_packet_type (ack_packet);
					command = arv_gvcp_packet_get_command (ack_packet);
					packet_id = arv_gvcp_packet_get_packet_id (ack_packet);

					if (command == ARV_GVCP_COMMAND_PENDING_ACK &&
					    count >= arv_gvcp_packet_get_pending_ack_size ()) {
						gint64 pending_ack_timeout_ms = arv_gvcp_packet_get_pending_ack_timeout (ack_packet);
						pending_ack = TRUE;
						expected_answer = FALSE;

						timeout_stop_ms = g_get_monotonic_time () / 1000 + pending_ack_timeout_ms;

						arv_log_device ("[GvDevice::%s] Pending ack timeout = %d",
								operation, pending_ack_timeout_ms);
					} else if (packet_type == ARV_GVCP_PACKET_TYPE_ERROR) {
						expected_answer = command == ack_command &&
							packet_id == io_data->packet_id;
						if (!expected_answer) {
							arv_debug_device ("[GvDevice::%s] Unexpected answer (0x%04x)", operation,
									  packet_type);
						} else
							command_error = arv_gvcp_packet_get_packet_flags (ack_packet);
					} else  {
						expected_answer = packet_type == ARV_GVCP_PACKET_TYPE_ACK &&
							command == ack_command &&
							packet_id == io_data->packet_id &&
							count >= ack_size;
						if (!expected_answer) {
							arv_debug_device ("[GvDevice::%s] Unexpected answer (0x%04x)", operation,
									  packet_type);
						}
					}
				} else {
					expected_answer = FALSE;
					if (local_error != NULL)
						arv_warning_device ("[GvDevice::%s] Ack reception error: %s", operation,
								    local_error->message);
					else
						arv_warning_device ("[GvDevice::%s] Ack reception timeout", operation);
					g_clear_error (&local_error);
				}
			} while (pending_ack || (!expected_answer && timeout_ms > 0));

			success = success && expected_answer;

			if (success && command_error == ARV_GVCP_ERROR_NONE) {
				switch (command) {
					case ARV_GVCP_COMMAND_READ_MEMORY_CMD:
						memcpy (buffer, arv_gvcp_packet_get_read_memory_ack_data (ack_packet), size);
						break;
					case ARV_GVCP_COMMAND_WRITE_MEMORY_CMD:
						break;
					case ARV_GVCP_COMMAND_READ_REGISTER_CMD:
						*((gint32 *) buffer) = arv_gvcp_packet_get_read_register_ack_value (ack_packet);
						break;
					case ARV_GVCP_COMMAND_WRITE_REGISTER_CMD:
						break;
					default:
						g_assert_not_reached ();
				}
			}
		} else {
			if (local_error != NULL)
				arv_warning_device ("[GvDevice::%s] Command sending error: %s", operation, local_error->message);
			g_clear_error (&local_error);
		}

		n_retries++;
	} while (!success && n_retries < io_data->gvcp_n_retries);

	arv_gvcp_packet_free (packet);

	g_mutex_unlock (&io_data->mutex);

	success = success && command_error == ARV_GVCP_ERROR_NONE;

	if (!success) {
		switch (command) {
			case ARV_GVCP_COMMAND_READ_MEMORY_CMD:
				memset (buffer, 0, size);
				break;
			case ARV_GVCP_COMMAND_WRITE_MEMORY_CMD:
				break;
			case ARV_GVCP_COMMAND_READ_REGISTER_CMD:
				*((guint32 *) buffer) = 0;
				break;
			case ARV_GVCP_COMMAND_WRITE_REGISTER_CMD:
				break;
			default:
				g_assert_not_reached ();
		}

		if (error != NULL && *error == NULL) {
			if (command_error != ARV_GVCP_ERROR_NONE)
				*error = g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
						      "GigEVision %s error (%s)", operation,
						      arv_gvcp_error_to_string (command_error));
			else
				*error = g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_TIMEOUT,
						      "GigEVision %s timeout", operation);
		}
	}

	return success;
}

static gboolean
_read_memory (ArvGvDeviceIOData *io_data, guint64 address, guint32 size, void *buffer, GError **error)
{
	return _send_cmd_and_receive_ack (io_data, ARV_GVCP_COMMAND_READ_MEMORY_CMD,
					  address, size, buffer, error);
}

static gboolean
_write_memory (ArvGvDeviceIOData *io_data, guint64 address, guint32 size, void *buffer, GError **error)
{
	return  _send_cmd_and_receive_ack (io_data, ARV_GVCP_COMMAND_WRITE_MEMORY_CMD,
					   address, size, buffer, error);
}

static gboolean
_read_register (ArvGvDeviceIOData *io_data, guint32 address, guint32 *value_placeholder, GError **error)
{
	return _send_cmd_and_receive_ack (io_data, ARV_GVCP_COMMAND_READ_REGISTER_CMD,
					  address, sizeof (guint32), value_placeholder, error);
}

static gboolean
_write_register (ArvGvDeviceIOData *io_data, guint32 address, guint32 value, GError **error)
{
	return _send_cmd_and_receive_ack (io_data, ARV_GVCP_COMMAND_WRITE_REGISTER_CMD,
					  address, sizeof (guint32), &value, error);
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
			       !g_atomic_int_get (&thread_data->cancel)) {
				g_usleep (ARV_GV_DEVICE_HEARTBEAT_RETRY_DELAY_US);
				counter++;
			}

			if (!g_atomic_int_get (&thread_data->cancel)) {
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
	} while (!g_atomic_int_get (&thread_data->cancel));

	g_timer_destroy (timer);

	return NULL;
}

/* ArvGvDevice implemenation */

static gboolean
arv_gv_device_take_control (ArvGvDevice *gv_device)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (gv_device);
	gboolean success;

	success = arv_device_write_register (ARV_DEVICE (gv_device),
					     ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_OFFSET,
					     ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_CONTROL, NULL);

	priv->io_data->is_controller = success;

	/* Disable GVSP extended ID mode for now, it is not supported yet by ArvGvStream */
	if (success)
		arv_device_set_string_feature_value (ARV_DEVICE (gv_device), "GevGVSPExtendedIDMode", "Off", NULL);

	if (!success)
		arv_warning_device ("[GvDevice::take_control] Can't get control access");

	return success;
}

static gboolean
arv_gv_device_leave_control (ArvGvDevice *gv_device)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (gv_device);
	gboolean success;

	priv->io_data->is_controller = FALSE;

	success = arv_device_write_register (ARV_DEVICE (gv_device),
					    ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0, NULL);

	return success;
}

guint64
arv_gv_device_get_timestamp_tick_frequency (ArvGvDevice *gv_device, GError **error)
{
	GError *local_error = NULL;
	guint32 timestamp_tick_frequency_high;
	guint32 timestamp_tick_frequency_low;
	guint64 timestamp_tick_frequency;

	g_return_val_if_fail (ARV_IS_GV_DEVICE (gv_device), 0);

	arv_device_read_register (ARV_DEVICE (gv_device),
				  ARV_GVBS_TIMESTAMP_TICK_FREQUENCY_HIGH_OFFSET,
				  &timestamp_tick_frequency_high, &local_error);
	if (local_error == NULL)
		arv_device_read_register (ARV_DEVICE (gv_device),
					  ARV_GVBS_TIMESTAMP_TICK_FREQUENCY_LOW_OFFSET,
					  &timestamp_tick_frequency_low, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	timestamp_tick_frequency = ((guint64) timestamp_tick_frequency_high << 32) |
		timestamp_tick_frequency_low;

	return timestamp_tick_frequency;
}

guint
arv_gv_device_get_packet_size (ArvGvDevice *gv_device, GError **error)
{
	return arv_device_get_integer_feature_value (ARV_DEVICE (gv_device), "GevSCPSPacketSize", error);
}

void
arv_gv_device_set_packet_size (ArvGvDevice *gv_device, gint packet_size, GError **error)
{
	g_return_if_fail (packet_size > 0);

	arv_device_set_integer_feature_value (ARV_DEVICE (gv_device), "GevSCPSPacketSize", packet_size, error);
}

/**
 * arv_gv_device_auto_packet_size:
 * @gv_device: a #ArvGvDevice
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Automatically determine the biggest packet size that can be used data
 * streaming, and set GevSCPSPacketSize value accordingly. This function relies
 * on the GevSCPSFireTestPacket feature. If this feature is not available, the
 * packet size will be set to a default value (1500 bytes).
 *
 * Returns: The packet size, in bytes.
 *
 * Since: 0.6.0
 */

guint
arv_gv_device_auto_packet_size (ArvGvDevice *gv_device, GError **error)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (gv_device);
	ArvDevice *device = ARV_DEVICE (gv_device);
	ArvGcNode *node;
	GSocket *socket;
	GInetAddress *interface_address;
	GSocketAddress *interface_socket_address;
	GInetSocketAddress *local_address;
	GPollFD poll_fd;
	const guint8 *address_bytes;
	guint16 port;
	gboolean do_not_fragment;
	gboolean is_command;
	int n_events;
	guint max_size, min_size, current_size;
	guint packet_size = 1500;
	gint64 minimum, maximum;
	guint inc;
	char *buffer;
	guint last_size = 0;

	g_return_val_if_fail (ARV_IS_GV_DEVICE (gv_device), 1500);

	node = arv_device_get_feature (device, "GevSCPSFireTestPacket");
	if (!ARV_IS_GC_COMMAND (node) && !ARV_IS_GC_BOOLEAN (node)) {
		arv_debug_device ("[GvDevice::auto_packet_size] No GevSCPSFireTestPacket feature found, "
				  "use default packet size (%d bytes)",
				  packet_size);
		return packet_size;
	}

	inc = MAX (1, arv_device_get_integer_feature_increment (device, "GevSCPSPacketSize", NULL));
	arv_device_get_integer_feature_bounds (device, "GevSCPSPacketSize", &minimum, &maximum, NULL);
	max_size = MIN (65536, maximum);
	min_size = MAX (ARV_GVSP_PACKET_PROTOCOL_OVERHEAD, minimum);

	if (max_size < min_size ||
	    inc > max_size - min_size ||
	    max_size < packet_size ||
	    min_size > packet_size ||
	    inc > 16) {
		arv_warning_device ("[GvDevice::auto_packet_size] Invalid GevSCPSPacketSize properties");
		return packet_size;
	}

	is_command = ARV_IS_GC_COMMAND (node);

	interface_address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (priv->io_data->interface_address));
	interface_socket_address = g_inet_socket_address_new (interface_address, 0);
	socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, NULL);
	g_socket_bind (socket, interface_socket_address, FALSE, NULL);
	local_address = G_INET_SOCKET_ADDRESS (g_socket_get_local_address (socket, NULL));
	port = g_inet_socket_address_get_port (local_address);

	address_bytes = g_inet_address_to_bytes (interface_address);
	arv_device_set_integer_feature_value (ARV_DEVICE (gv_device), "GevSCDA", g_htonl (*((guint32 *) address_bytes)), NULL);
	arv_device_set_integer_feature_value (ARV_DEVICE (gv_device), "GevSCPHostPort", port, NULL);

	g_clear_object (&local_address);
	g_clear_object (&interface_socket_address);

	do_not_fragment = arv_device_get_boolean_feature_value (device, "GevSCPSDoNotFragment", NULL);
	arv_device_set_boolean_feature_value (device, "GevSCPSDoNotFragment", TRUE, NULL);

	poll_fd.fd = g_socket_get_fd (socket);
	poll_fd.events =  G_IO_IN;
	poll_fd.revents = 0;

	current_size = 1500;

	buffer = g_malloc (max_size);

	do {
		size_t read_count;
		unsigned n_tries = 0;

		current_size = ((current_size + inc - 1) / inc) * inc;

		arv_debug_device ("[GvDevice::auto_packet_size] Try packet size = %d", current_size);
		arv_device_set_integer_feature_value (device, "GevSCPSPacketSize", current_size, NULL);

		current_size = arv_device_get_integer_feature_value(device, "GevSCPSPacketSize", NULL);

		if (current_size == last_size)
			break;

		last_size = current_size;

		do {
			if (is_command) {
				arv_device_execute_command (device, "GevSCPSFireTestPacket", NULL);
			} else {
				arv_device_set_boolean_feature_value (device, "GevSCPSFireTestPacket", FALSE, NULL);
				arv_device_set_boolean_feature_value (device, "GevSCPSFireTestPacket", TRUE, NULL);
			}

			do {
				n_events = g_poll (&poll_fd, 1, 10);
				if (n_events != 0)
					read_count = g_socket_receive (socket, buffer, 16384, NULL, NULL);
				else
					read_count = 0;
				/* Discard late packets, read_count should be equal to packet size minus IP and UDP headers */
			} while (n_events != 0 && read_count != (current_size - sizeof (struct iphdr) - sizeof (struct udphdr)));

			n_tries++;
		} while (n_events == 0 && n_tries < 3);

		if (n_events != 0) {
			arv_debug_device ("[GvDevice::auto_packet_size] Received %d bytes", (int) read_count);

			packet_size = current_size;
			min_size = current_size;
			current_size = (max_size - min_size) / 2 + min_size;
		} else {
			max_size = current_size;
			current_size = (max_size - min_size) / 2 + min_size;
		}
	} while ((max_size - min_size) > 16);

	g_clear_pointer (&buffer, g_free);
	g_clear_object (&socket);

	arv_debug_device ("[GvDevice::auto_packet_size] Packet size set to %d bytes", packet_size);

	arv_device_set_boolean_feature_value (device, "GevSCPSDoNotFragment", do_not_fragment, NULL);
	arv_device_set_integer_feature_value (device, "GevSCPSPacketSize", packet_size, NULL);

	return packet_size;
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
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (gv_device);
	char *xml;


	if (priv->genicam_xml != NULL) {
		*size = priv->genicam_xml_size;
		return priv->genicam_xml;
	}

	*size = 0;

	xml = _load_genicam (gv_device, ARV_GVBS_XML_URL_0_OFFSET, size);
	if (xml == NULL)
		xml = _load_genicam (gv_device, ARV_GVBS_XML_URL_1_OFFSET, size);

	priv->genicam_xml = xml;
	priv->genicam_xml_size = *size;

	return xml;
}

static void
arv_gv_device_load_genicam (ArvGvDevice *gv_device)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (gv_device);
	const char *genicam;
	size_t size;

	genicam = arv_gv_device_get_genicam_xml (ARV_DEVICE (gv_device), &size);
	if (genicam != NULL) {
		priv->genicam = arv_gc_new (ARV_DEVICE (gv_device), genicam, size);

		arv_gc_set_default_node_data (priv->genicam, "DeviceVendorName",
					      "<StringReg Name=\"DeviceVendorName\">"
					      "<DisplayName>Vendor Name</DisplayName>"
					      "<Address>0x48</Address>"
					      "<Length>32</Length>"
					      "<AccessMode>RO</AccessMode>"
					      "<pPort>Device</pPort>"
					      "</StringReg>", NULL);
		arv_gc_set_default_node_data (priv->genicam, "DeviceModelName",
					      "<StringReg Name=\"DeviceModelName\">"
					      "<DisplayName>Model Name</DisplayName>"
					      "<Address>0x68</Address>"
					      "<Length>32</Length>"
					      "<AccessMode>RO</AccessMode>"
					      "<pPort>Device</pPort>"
					      "</StringReg>", NULL);
		arv_gc_set_default_node_data (priv->genicam, "DeviceVersion",
					      "<StringReg Name=\"DeviceVersion\">"
					      "<DisplayName>Device Version</DisplayName>"
					      "<Address>0x88</Address>"
					      "<Length>32</Length>"
					      "<AccessMode>RO</AccessMode>"
					      "<pPort>Device</pPort>"
					      "</StringReg>", NULL);
		arv_gc_set_default_node_data (priv->genicam, "DeviceManufacturerInfo",
					      "<StringReg Name=\"DeviceManufacturerInfo\">"
					      "<DisplayName>Manufacturer Info</DisplayName>"
					      "<Address>0xa8</Address>"
					      "<Length>48</Length>"
					      "<AccessMode>RO</AccessMode>"
					      "<pPort>Device</pPort>"
					      "</StringReg>", NULL);
		arv_gc_set_default_node_data (priv->genicam, "DeviceID",
					      "<StringReg Name=\"DeviceID\">"
					      "<DisplayName>Device ID</DisplayName>"
					      "<Address>0xd8</Address>"
					      "<Length>16</Length>"
					      "<AccessMode>RO</AccessMode>"
					      "<pPort>Device</pPort>"
					      "</StringReg>", NULL);
		arv_gc_set_default_node_data (priv->genicam, "GevStreamChannelCount",
					      "<IntReg Name=\"GevStreamChannelCount\">"
					      "<Address>0x904</Address>"
					      "<Length>4</Length>"
					      "<AccessMode>RO</AccessMode>"
					      "<Endianess>BigEndian</Endianess>"
					      "<pPort>Device</pPort>"
					      "</IntReg>", NULL);
		arv_gc_set_default_node_data (priv->genicam, "GevTimestampTickFrequency",
					      "<Integer Name=\"GevTimestampTickFrequency\">"
					      "<pValue>ArvGevTimestampTickFrequencyCalc</pValue>"
					      "</Integer>",
					      "<IntSwissKnife Name=\"ArvGevTimestampTickFrequencyCalc\">"
					      "<pVariable Name=\"HIGH\">ArvGevTimestampTickFrequencyHigh</pVariable>"
					      "<pVariable Name=\"LOW\">ArvGevTimestampTickFrequencyLow</pVariable>"
					      "<Formula>(HIGH&lt;&lt; 32) | LOW</Formula>"
					      "</IntSwissKnife>",
					      "<MaskedIntReg Name=\"ArvGevTimestampTickFrequencyHigh\">"
					      "<Visibility>Invisible</Visibility>"
					      "<Address>0x93C</Address>"
					      "<Length>4</Length>"
					      "<AccessMode>RO</AccessMode>"
					      "<pPort>Device</pPort>"
					      "<LSB>31</LSB>"
					      "<MSB>0</MSB>"
					      "<Sign>Unsigned</Sign>"
					      "<Endianess>BigEndian</Endianess>"
					      "</MaskedIntReg>",
					      "<MaskedIntReg Name=\"ArvGevTimestampTickFrequencyLow\">"
					      "<Visibility>Invisible</Visibility>"
					      "<Address>0x940</Address>"
					      "<Length>4</Length>"
					      "<AccessMode>RO</AccessMode>"
					      "<pPort>Device</pPort>"
					      "<LSB>31</LSB>"
					      "<MSB>0</MSB>"
					      "<Sign>Unsigned</Sign>"
					      "<Endianess>BigEndian</Endianess>"
					      "</MaskedIntReg>", NULL);
		arv_gc_set_default_node_data (priv->genicam, "GevSCPHostPort",
					      "<Integer Name=\"GevSCPHostPort\">"
					      "  <Visibility>Expert</Visibility>"
					      "  <pIsLocked>TLParamsLocked</pIsLocked>"
					      "  <pValue>ArvGevSCPHostPortReg</pValue>"
					      "</Integer>",
					      "<MaskedIntReg Name=\"ArvGevSCPHostPortReg\">"
					      "  <Address>0xd00</Address>"
					      "  <pAddress>GevSCPAddrCalc</pAddress>"
					      "  <Length>4</Length>"
					      "  <AccessMode>RW</AccessMode>"
					      "  <pPort>Device</pPort>"
					      "  <Cachable>NoCache</Cachable>"
					      "  <LSB>31</LSB>"
					      "  <MSB>16</MSB>"
					      "  <Sign>Unsigned</Sign>"
					      "  <Endianess>BigEndian</Endianess>"
					      "</MaskedIntReg>",
					      NULL);
#if 0
		arv_gc_set_default_node_data (priv->genicam, "GevSCPSFireTestPacket",
					      "<Command Name=\"GevSCPSFireTestPacket\">"
					      "<pIsLocked>TLParamsLocked</pIsLocked>"
					      "<pValue>GevSCPSFireTestPacketReg</pValue>"
					      "<CommandValue>1</CommandValue>"
					      "</Boolean>");
		arv_gc_set_default_node_data (priv->genicam, "GevSCPSFireTestPacketReg",
					      "<MaskedIntReg Name=\"GevSCPSFireTestPacketReg\">"
					      "<Address>0x0d04</Address>"
					      "<pAddress>GevSCPAddrCalc</pAddress>"
					      "<Length>4</Length>"
					      "<AccessMode>RW</AccessMode>"
					      "<Bit>0</Bit>"
					      "</MaskedIntReg>");
#endif
		arv_gc_set_default_node_data (priv->genicam, "GevSCPSDoNotFragment",
					      "<Boolean Name=\"GevSCPSDoNotFragment\">"
					      "  <pIsLocked>TLParamsLocked</pIsLocked>"
					      "  <pValue>ArvGevSCPSDoNotFragmentReg</pValue>"
					      "</Boolean>",
					      "<MaskedIntReg Name=\"ArvGevSCPSDoNotFragmentReg\">"
					      "  <Address>0x0d04</Address>"
					      "  <pAddress>GevSCPAddrCalc</pAddress>"
					      "  <Length>4</Length>"
					      "  <AccessMode>RW</AccessMode>"
					      "  <pPort>Device</pPort>"
					      "  <Cachable>NoCache</Cachable>"
					      "  <Bit>1</Bit>"
					      "</MaskedIntReg>",
					      NULL);
		arv_gc_set_default_node_data (priv->genicam, "GevSCPSBigEndian",
					      "<Boolean Name=\"GevSCPSBigEndian\">"
					      "  <pIsLocked>TLParamsLocked</pIsLocked>"
					      "  <pValue>ArvGevSCPSBigEndianReg</pValue>"
					      "</Boolean>",
					      "  <MaskedIntReg Name=\"ArvGevSCPSBigEndianReg\">"
					      "  <Address>0x0d04</Address>"
					      "  <pAddress>GevSCPAddrCalc</pAddress>"
					      "  <Length>4</Length>"
					      "  <AccessMode>RW</AccessMode>"
					      "  <pPort>Device</pPort>"
					      "  <Cachable>NoCache</Cachable>"
					      "  <Bit>2</Bit>"
					      "</MaskedIntReg>",
					      NULL);
		arv_gc_set_default_node_data (priv->genicam, "GevSCPSPacketSize",
					      "<Integer Name=\"GevSCPSPacketSize\">"
					      "  <Visibility>Expert</Visibility>"
					      "  <pIsLocked>TLParamsLocked</pIsLocked>"
					      "  <pValue>ArvGevSCPSPacketSizeReg</pValue>"
					      "</Integer>",
					      "<MaskedIntReg Name=\"ArvGevSCPSPacketSizeReg\">"
					      "  <Address>0xd04</Address>"
					      "  <pAddress>GevSCPAddrCalc</pAddress>"
					      "  <Length>4</Length>"
					      "  <AccessMode>RW</AccessMode>"
					      "  <pPort>Device</pPort>"
					      "  <Cachable>NoCache</Cachable>"
					      "  <LSB>31</LSB>"
					      "  <MSB>16</MSB>"
					      "  <Sign>Unsigned</Sign>"
					      "  <Endianess>BigEndian</Endianess>"
					      "</MaskedIntReg>",
					      NULL);
		arv_gc_set_default_node_data (priv->genicam, "GevSCDA",
					      "<Integer Name=\"GevSCDA\">"
					      "  <Visibility>Expert</Visibility>"
					      "  <pIsLocked>TLParamsLocked</pIsLocked>"
					      "  <pValue>ArvGevSCDAReg</pValue>"
					      "</Integer>",
					      "<IntReg Name=\"ArvGevSCDAReg\">"
					      "  <Address>0xd18</Address>"
					      "  <pAddress>GevSCPAddrCalc</pAddress>"
					      "  <Length>4</Length>"
					      "  <AccessMode>RW</AccessMode>"
					      "  <pPort>Device</pPort>"
					      "  <Cachable>NoCache</Cachable>"
					      "  <Sign>Unsigned</Sign>"
					      "  <Endianess>BigEndian</Endianess>"
					      "</IntReg>",
					      NULL);
		arv_gc_set_default_node_data (priv->genicam, "GevSCSP",
					      "<Integer Name=\"GevSCSP\">"
					      "  <Visibility>Expert</Visibility>"
					      "  <pIsLocked>TLParamsLocked</pIsLocked>"
					      "  <pValue>ArvGevSCSPReg</pValue>"
					      "</Integer>",
					      "<MaskedIntReg Name=\"ArvGevSCSPReg\">"
					      "  <Address>0xd1c</Address>"
					      "  <pAddress>GevSCPAddrCalc</pAddress>"
					      "  <Length>4</Length>"
					      "  <AccessMode>RO</AccessMode>"
					      "  <pPort>Device</pPort>"
					      "  <Cachable>NoCache</Cachable>"
					      "  <LSB>31</LSB>"
					      "  <MSB>16</MSB>"
					      "  <Sign>Unsigned</Sign>"
					      "  <Endianess>BigEndian</Endianess>"
					      "</MaskedIntReg>",
					      NULL);
		arv_gc_set_default_node_data (priv->genicam, "GevSCPAddrCalc",
					      "<IntSwissKnife Name= \"GevSCPAddrCalc\">"
					      "  <pVariable Name=\"SEL\">ArvGevStreamChannelSelector</pVariable>"
					      "  <Formula>SEL * 0x40</Formula>"
					      "</IntSwissKnife>",
					      "<Integer Name=\"ArvGevStreamChannelSelector\">"
					      "  <Value>0</Value>"
					      "  <Min>0</Min>"
					      "  <pMax>ArvGevStreamChannelSelectorMax</pMax>"
					      "  <Inc>1</Inc>"
					      "</Integer>",
					      "<IntSwissKnife Name=\"ArvGevStreamChannelSelectorMax\">"
					      "  <pVariable Name=\"N_STREAM_CHANNELS\">NumberOfStreamChannels</pVariable>"
					      "  <Formula>N_STREAM_CHANNELS - 1</Formula>"
					      "</IntSwissKnife>",
					      NULL);
		arv_gc_set_default_node_data (priv->genicam, "TLParamsLocked",
					      "<Integer Name=\"TLParamsLocked\">"
					      "<Visibility>Invisible</Visibility>"
					      "<Value>0</Value>"
					      "<Min>0</Min>"
					      "<Max>1</Max>"
					      "</Integer>",
					      NULL);
	}
}

/* ArvDevice implemenation */

static ArvStream *
arv_gv_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data, GError **error)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (gv_device);
	ArvStream *stream;
	guint32 n_stream_channels;

	n_stream_channels = arv_device_get_integer_feature_value (device, "GevStreamChannelCount", NULL);
	arv_debug_device ("[GvDevice::create_stream] Number of stream channels = %d", n_stream_channels);

	if (n_stream_channels < 1) {
		g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NO_STREAM_CHANNEL,
			     "No stream channel found");
		return NULL;
	}

	if (!priv->io_data->is_controller) {
		arv_warning_device ("[GvDevice::create_stream] Can't create stream without control access");
		g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_CONTROLLER,
			     "Controller privilege required for streaming control");
		return NULL;
	}

	stream = arv_gv_stream_new (gv_device, callback, user_data, error);
	if (!ARV_IS_STREAM (stream))
		return NULL;

	if (!priv->is_packet_resend_supported)
		g_object_set (stream, "packet-resend", ARV_GV_STREAM_PACKET_RESEND_NEVER, NULL);

	return stream;
}

static ArvGc *
arv_gv_device_get_genicam (ArvDevice *device)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (ARV_GV_DEVICE (device));

	return priv->genicam;
}

static gboolean
arv_gv_device_read_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (ARV_GV_DEVICE (device));
	int i;
	gint32 block_size;

	for (i = 0; i < (size + ARV_GVCP_DATA_SIZE_MAX - 1) / ARV_GVCP_DATA_SIZE_MAX; i++) {
		block_size = MIN (ARV_GVCP_DATA_SIZE_MAX, size - i * ARV_GVCP_DATA_SIZE_MAX);
		if (!_read_memory (priv->io_data,
				   address + i * ARV_GVCP_DATA_SIZE_MAX,
				   block_size, ((char *) buffer) + i * ARV_GVCP_DATA_SIZE_MAX, error))
			return FALSE;
	}

	return TRUE;
}

static gboolean
arv_gv_device_write_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (ARV_GV_DEVICE (device));
	int i;
	gint32 block_size;

	for (i = 0; i < (size + ARV_GVCP_DATA_SIZE_MAX - 1) / ARV_GVCP_DATA_SIZE_MAX; i++) {
		block_size = MIN (ARV_GVCP_DATA_SIZE_MAX, size - i * ARV_GVCP_DATA_SIZE_MAX);
		if (!_write_memory (priv->io_data,
				    address + i * ARV_GVCP_DATA_SIZE_MAX,
				    block_size, ((char *) buffer) + i * ARV_GVCP_DATA_SIZE_MAX, error))
			return FALSE;
	}

	return TRUE;
}

static gboolean
arv_gv_device_read_register (ArvDevice *device, guint64 address, guint32 *value, GError **error)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (ARV_GV_DEVICE (device));

	return _read_register (priv->io_data, address, value, error);
}

static gboolean
arv_gv_device_write_register (ArvDevice *device, guint64 address, guint32 value, GError **error)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (ARV_GV_DEVICE (device));

	return _write_register (priv->io_data, address, value, error);
}

/**
 * arv_gv_device_get_stream_options:
 * @gv_device: a #ArvGvDevice
 *
 * Returns: options for stream creation
 *
 * Since: 0.6.0
 */

ArvGvStreamOption
arv_gv_device_get_stream_options (ArvGvDevice *gv_device)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (gv_device);

	g_return_val_if_fail (ARV_IS_GV_DEVICE (gv_device), ARV_GV_STREAM_OPTION_NONE);

	return priv->stream_options;
}

/**
 * arv_gv_device_set_stream_options:
 * @gv_device: a #ArvGvDevice
 * @options: options for stream creation
 *
 * Sets the option used during stream creation. It must be called before arv_device_create_stream().
 *
 * Since: 0.6.0
 */

void
arv_gv_device_set_stream_options (ArvGvDevice *gv_device, ArvGvStreamOption options)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (gv_device);

	g_return_if_fail (ARV_IS_GV_DEVICE (gv_device));

	priv->stream_options = options;
}

/**
 * arv_gv_device_new:
 * @interface_address: address of the interface connected to the device
 * @device_address: device address
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: a newly created #ArvDevice using GigE protocol
 *
 * Since: 0.8.0
 */

ArvDevice *
arv_gv_device_new (GInetAddress *interface_address, GInetAddress *device_address, GError **error)
{
	return g_initable_new (ARV_TYPE_GV_DEVICE, NULL, error,
			       "interface-address", interface_address,
			       "device-address", device_address,
			       NULL);
}

static void
arv_gv_device_constructed (GObject *object)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (object);
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (gv_device);
	ArvGvDeviceIOData *io_data;
	ArvGvDeviceHeartbeatData *heartbeat_data;
	ArvGcRegisterDescriptionNode *register_description;
	ArvDomDocument *document;
	GError *local_error = NULL;
	char *address_string;
	guint32 capabilities;

	if (!G_IS_INET_ADDRESS (priv->interface_address) ||
	    !G_IS_INET_ADDRESS (priv->device_address)) {
		arv_device_take_init_error (ARV_DEVICE (object), g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_INVALID_PARAMETER,
									     "Invalid interface or device address"));
		return;
	}

	address_string = g_inet_address_to_string (priv->interface_address);
	arv_debug_device ("[GvDevice::new] Interface address = %s", address_string);
	g_free (address_string);
	address_string = g_inet_address_to_string (priv->device_address);
	arv_debug_device ("[GvDevice::new] Device address = %s", address_string);
	g_free (address_string);

	io_data = g_new0 (ArvGvDeviceIOData, 1);

	g_mutex_init (&io_data->mutex);

	io_data->packet_id = 65300; /* Start near the end of the circular counter */

	io_data->interface_address = g_inet_socket_address_new (priv->interface_address, 0);
	io_data->device_address = g_inet_socket_address_new (priv->device_address, ARV_GVCP_PORT);
	io_data->socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
					G_SOCKET_TYPE_DATAGRAM,
					G_SOCKET_PROTOCOL_UDP, NULL);
	if (!g_socket_bind (io_data->socket, io_data->interface_address, FALSE, &local_error)) {
		if (local_error)
			local_error = g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_UNKNOWN,
						   "Unknown error trying to bind device interface");
		arv_device_take_init_error (ARV_DEVICE (gv_device), local_error);
		local_error = NULL;
	}

	io_data->buffer = g_malloc (ARV_GV_DEVICE_BUFFER_SIZE);
	io_data->gvcp_n_retries = ARV_GV_DEVICE_GVCP_N_RETRIES_DEFAULT;
	io_data->gvcp_timeout_ms = ARV_GV_DEVICE_GVCP_TIMEOUT_MS_DEFAULT;
	io_data->poll_in_event.fd = g_socket_get_fd (io_data->socket);
	io_data->poll_in_event.events =  G_IO_IN;
	io_data->poll_in_event.revents = 0;

	priv->io_data = io_data;

	arv_gv_device_load_genicam (gv_device);

	if (!ARV_IS_GC (priv->genicam)) {
		arv_device_take_init_error (ARV_DEVICE (object),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_GENICAM_NOT_FOUND,
							 "Invalid Genicam data"));
		return;
	}

	arv_gv_device_take_control (gv_device);

	heartbeat_data = g_new (ArvGvDeviceHeartbeatData, 1);
	heartbeat_data->gv_device = gv_device;
	heartbeat_data->io_data = io_data;
	heartbeat_data->period_us = ARV_GV_DEVICE_HEARTBEAT_PERIOD_US;
	heartbeat_data->cancel = FALSE;

	priv->heartbeat_data = heartbeat_data;

	priv->heartbeat_thread = g_thread_new ("arv_gv_heartbeat", arv_gv_device_heartbeat_thread, priv->heartbeat_data);

	arv_device_read_register (ARV_DEVICE (gv_device), ARV_GVBS_GVCP_CAPABILITY_OFFSET, &capabilities, NULL);
	priv->is_packet_resend_supported = (capabilities & ARV_GVBS_GVCP_CAPABILITY_PACKET_RESEND) != 0;
	priv->is_write_memory_supported = (capabilities & ARV_GVBS_GVCP_CAPABILITY_WRITE_MEMORY) != 0;

	arv_debug_device ("[GvDevice::new] Packet resend = %s", priv->is_packet_resend_supported ? "yes" : "no");
	arv_debug_device ("[GvDevice::new] Write memory = %s", priv->is_write_memory_supported ? "yes" : "no");

	document = ARV_DOM_DOCUMENT (priv->genicam);
	register_description = ARV_GC_REGISTER_DESCRIPTION_NODE (arv_dom_document_get_document_element (document));
	arv_debug_device ("[GvDevice::new] Legacy endianess handling = %s",
			  arv_gc_register_description_node_compare_schema_version (register_description, 1, 1, 0) < 0 ?
			  "yes" : "no");
}

static void
arv_gv_device_init (ArvGvDevice *gv_device)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (gv_device);

	priv->genicam = NULL;
	priv->genicam_xml = NULL;
	priv->genicam_xml_size = 0;
	priv->stream_options = ARV_GV_STREAM_OPTION_NONE;
}

static void
arv_gv_device_finalize (GObject *object)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (object);
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (gv_device);
	ArvGvDeviceIOData *io_data;

	if (priv->heartbeat_thread != NULL) {
		ArvGvDeviceHeartbeatData *heartbeat_data;

		heartbeat_data = priv->heartbeat_data;

		g_atomic_int_set (&heartbeat_data->cancel, TRUE);
		g_thread_join (priv->heartbeat_thread);
		g_clear_pointer (&heartbeat_data, g_free);

		priv->heartbeat_data = NULL;
		priv->heartbeat_thread = NULL;
	}

	arv_gv_device_leave_control (gv_device);

	io_data = priv->io_data;
	g_clear_object (&io_data->device_address);
	g_clear_object (&io_data->interface_address);
	g_clear_object (&io_data->socket);
	g_clear_pointer (&io_data->buffer, g_free);
	g_mutex_clear (&io_data->mutex);

	g_clear_pointer (&priv->io_data, g_free);

	g_clear_object (&priv->genicam);
	g_clear_pointer (&priv->genicam_xml, g_free);

	g_clear_object (&priv->interface_address);
	g_clear_object (&priv->device_address);

	G_OBJECT_CLASS (arv_gv_device_parent_class)->finalize (object);
}

/**
 * arv_gv_device_get_interface_address:
 * @device: a #ArvGvDevice
 *
 * Returns: (transfer none): the device host interface IP address.
 *
 * Since: 0.2.0
 */

GSocketAddress *arv_gv_device_get_interface_address(ArvGvDevice *gv_device)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (gv_device);

	return priv->io_data->interface_address;
}

/**
 * arv_gv_device_get_device_address:
 * @device: a #ArvGvDevice
 *
 * Returns: (transfer none): the device IP address.
 *
 * since: 0.2.0
 */

GSocketAddress *arv_gv_device_get_device_address(ArvGvDevice *gv_device)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (gv_device);

	return priv->io_data->device_address;
}

static void
arv_gv_device_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ArvGvDevicePrivate *priv = arv_gv_device_get_instance_private (ARV_GV_DEVICE (self));

	switch (prop_id)
	{
		case PROP_GV_DEVICE_INTERFACE_ADDRESS:
			g_clear_object (&priv->interface_address);
			priv->interface_address = g_value_dup_object (value);
			break;
		case PROP_GV_DEVICE_DEVICE_ADDRESS:
			g_clear_object (&priv->device_address);
			priv->device_address = g_value_dup_object (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
			break;
	}
}

static void
arv_gv_device_class_init (ArvGvDeviceClass *gv_device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (gv_device_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (gv_device_class);

	object_class->finalize = arv_gv_device_finalize;
	object_class->constructed = arv_gv_device_constructed;
	object_class->set_property = arv_gv_device_set_property;

	device_class->create_stream = arv_gv_device_create_stream;
	device_class->get_genicam_xml = arv_gv_device_get_genicam_xml;
	device_class->get_genicam = arv_gv_device_get_genicam;
	device_class->read_memory = arv_gv_device_read_memory;
	device_class->write_memory = arv_gv_device_write_memory;
	device_class->read_register = arv_gv_device_read_register;
	device_class->write_register = arv_gv_device_write_register;

	g_object_class_install_property
		(object_class,
		 PROP_GV_DEVICE_INTERFACE_ADDRESS,
		 g_param_spec_object ("interface-address",
				      "Interface address",
				      "The address of the interface connected to the device",
				      G_TYPE_INET_ADDRESS,
				      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property
		(object_class,
		 PROP_GV_DEVICE_DEVICE_ADDRESS,
		 g_param_spec_object ("device-address",
				      "Device address",
				      "The device address",
				      G_TYPE_INET_ADDRESS,
				      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}
