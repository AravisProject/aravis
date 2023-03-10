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

#include <arvgvfakecamera.h>
#include <arvfakecamera.h>
#include <arvbufferprivate.h>
#include <arvgvcpprivate.h>
#include <arvgvspprivate.h>
#include <arvmisc.h>
#include <arvmiscprivate.h>
#include <arvnetworkprivate.h>

/**
 * SECTION: arvgvfakecamera
 * @short_description: GigE Vision Simulator
 *
 * #ArvGvFakeCamera is a class that simulates a real GigEVision camera.
 */

#define ARV_GV_FAKE_CAMERA_BUFFER_SIZE	65536

enum {
	ARV_GV_FAKE_CAMERA_INPUT_SOCKET_GVCP = 0,
	ARV_GV_FAKE_CAMERA_INPUT_SOCKET_GLOBAL_DISCOVERY,
	ARV_GV_FAKE_CAMERA_INPUT_SOCKET_SUBNET_DISCOVERY,
	ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS
};

enum
{
  PROP_0,
  PROP_INTERFACE_NAME,
  PROP_SERIAL_NUMBER,
  PROP_GENICAM_FILENAME,
  PROP_GVSP_LOST_PACKET_RATIO,
  PROP_CM_DOMAIN
};

typedef struct {
	char *interface_name;
	char *serial_number;
	char *genicam_filename;

	ArvFakeCamera *camera;

	gboolean is_running;

	GPollFD socket_fds[ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS];
	guint n_socket_fds;

	GSocketAddress *controller_address;
	gint64 controller_time;

	GSocket *input_sockets[ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS];

	GSocket *gvsp_socket;

	GThread *thread;
	gboolean cancel;

	double gvsp_lost_packet_ratio;
} ArvGvFakeCameraPrivate;

struct _ArvGvFakeCamera {
	GObject	object;

	ArvGvFakeCameraPrivate *priv;
};

struct _ArvGvFakeCameraClass {
	GObjectClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvGvFakeCamera, arv_gv_fake_camera, G_TYPE_OBJECT, G_ADD_PRIVATE (ArvGvFakeCamera))

static gboolean
_g_inet_socket_address_is_equal (GInetSocketAddress *a, GInetSocketAddress *b)
{
	if (!G_IS_INET_SOCKET_ADDRESS (a) ||
	    !G_IS_INET_SOCKET_ADDRESS (b))
		return FALSE;

	if (g_inet_socket_address_get_port (a) != g_inet_socket_address_get_port (b))
		return FALSE;

	return g_inet_address_equal (g_inet_socket_address_get_address (a),
				     g_inet_socket_address_get_address (b));
}

static gboolean
_handle_control_packet (ArvGvFakeCamera *gv_fake_camera, GSocket *socket,
			GSocketAddress *remote_address,
			ArvGvcpPacket *packet, size_t size)
{
	ArvGvcpPacket *ack_packet = NULL;
	size_t ack_packet_size;
	guint32 block_address;
	guint32 block_size;
	guint16 packet_id;
	guint16 packet_type;
	guint32 register_address;
	guint32 register_value;
	gboolean write_access;
	gboolean success = FALSE;

	if (gv_fake_camera->priv->controller_address != NULL) {
		gint64 time;
		guint64 elapsed_ms;

		time = g_get_real_time ();

		elapsed_ms = (time - gv_fake_camera->priv->controller_time) / 1000;

		if (elapsed_ms > arv_fake_camera_get_heartbeat_timeout (gv_fake_camera->priv->camera)) {
			g_object_unref (gv_fake_camera->priv->controller_address);
			gv_fake_camera->priv->controller_address = NULL;
			write_access = TRUE;
			arv_warning_device ("[GvFakeCamera::handle_control_packet] Heartbeat timeout");
			arv_fake_camera_set_control_channel_privilege (gv_fake_camera->priv->camera, 0);
		} else
			write_access = _g_inet_socket_address_is_equal
				(G_INET_SOCKET_ADDRESS (remote_address),
				 G_INET_SOCKET_ADDRESS (gv_fake_camera->priv->controller_address));
	} else
		write_access = TRUE;


	arv_gvcp_packet_debug (packet, ARV_DEBUG_LEVEL_DEBUG);

	packet_id = arv_gvcp_packet_get_packet_id (packet);
	packet_type = arv_gvcp_packet_get_packet_type (packet);

	if (packet_type != ARV_GVCP_PACKET_TYPE_CMD) {
		arv_warning_device ("[GvFakeCamera::handle_control_packet] Unknown packet type");
		return FALSE;
	}

	switch (g_ntohs (packet->header.command)) {
		case ARV_GVCP_COMMAND_DISCOVERY_CMD:
			ack_packet = arv_gvcp_packet_new_discovery_ack (packet_id, &ack_packet_size);
			arv_info_device ("[GvFakeCamera::handle_control_packet] Discovery command");
			arv_fake_camera_read_memory (gv_fake_camera->priv->camera, 0, ARV_GVBS_DISCOVERY_DATA_SIZE,
						     &ack_packet->data);
			break;
		case ARV_GVCP_COMMAND_READ_MEMORY_CMD:
			arv_gvcp_packet_get_read_memory_cmd_infos (packet, &block_address, &block_size);
			arv_info_device ("[GvFakeCamera::handle_control_packet] Read memory command %d (%d)",
					  block_address, block_size);
			ack_packet = arv_gvcp_packet_new_read_memory_ack (block_address, block_size,
									  packet_id, &ack_packet_size);
			arv_fake_camera_read_memory (gv_fake_camera->priv->camera, block_address, block_size,
						     arv_gvcp_packet_get_read_memory_ack_data (ack_packet));
			break;
		case ARV_GVCP_COMMAND_WRITE_MEMORY_CMD:
			arv_gvcp_packet_get_write_memory_cmd_infos (packet, &block_address, &block_size);
			if (!write_access) {
				arv_warning_device("[GvFakeCamera::handle_control_packet] Ignore Write memory command %d (%d) not controller",
					block_address, block_size);
				break;
			}

			arv_info_device ("[GvFakeCamera::handle_control_packet] Write memory command %d (%d)",
					  block_address, block_size);
			arv_fake_camera_write_memory (gv_fake_camera->priv->camera, block_address, block_size,
						      arv_gvcp_packet_get_write_memory_cmd_data (packet));
			ack_packet = arv_gvcp_packet_new_write_memory_ack (block_address, packet_id,
									   &ack_packet_size);
			break;
		case ARV_GVCP_COMMAND_READ_REGISTER_CMD:
			arv_gvcp_packet_get_read_register_cmd_infos (packet, &register_address);
			arv_fake_camera_read_register (gv_fake_camera->priv->camera, register_address, &register_value);
			arv_info_device ("[GvFakeCamera::handle_control_packet] Read register command %d -> %d",
					  register_address, register_value);
			ack_packet = arv_gvcp_packet_new_read_register_ack (register_value, packet_id,
									    &ack_packet_size);

			if (register_address == ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_OFFSET)
				gv_fake_camera->priv->controller_time = g_get_real_time ();

			break;
		case ARV_GVCP_COMMAND_WRITE_REGISTER_CMD:
			arv_gvcp_packet_get_write_register_cmd_infos (packet, &register_address, &register_value);
			if (!write_access) {
				arv_warning_device("[GvFakeCamera::handle_control_packet] Ignore Write register command %d (%d) not controller",
					register_address, register_value);
				break;
			}

			arv_fake_camera_write_register (gv_fake_camera->priv->camera, register_address, register_value);
			arv_info_device ("[GvFakeCamera::handle_control_packet] Write register command %d -> %d",
					  register_address, register_value);
			ack_packet = arv_gvcp_packet_new_write_register_ack (1, packet_id,
									     &ack_packet_size);
			break;
		default:
			arv_warning_device ("[GvFakeCamera::handle_control_packet] Unknown command");
	}

	if (ack_packet != NULL) {
		g_socket_send_to (socket, remote_address, (char *) ack_packet, ack_packet_size, NULL, NULL);
		arv_gvcp_packet_debug (ack_packet, ARV_DEBUG_LEVEL_DEBUG);
		g_free (ack_packet);

		success = TRUE;
	}

	if (gv_fake_camera->priv->controller_address == NULL &&
	    arv_fake_camera_get_control_channel_privilege (gv_fake_camera->priv->camera) != 0) {
		g_object_ref (remote_address);
		arv_info_device("[GvFakeCamera::handle_control_packet] New controller");
		gv_fake_camera->priv->controller_address = remote_address;
		gv_fake_camera->priv->controller_time = g_get_real_time ();
	}
	else if (gv_fake_camera->priv->controller_address != NULL &&
	    arv_fake_camera_get_control_channel_privilege (gv_fake_camera->priv->camera) == 0) {
		g_object_unref (gv_fake_camera->priv->controller_address);
		arv_info_device("[GvFakeCamera::handle_control_packet] Controller releases");
		gv_fake_camera->priv->controller_address = NULL;
		gv_fake_camera->priv->controller_time = g_get_real_time ();
	}

	return success;
}

static void *
_thread (void *user_data)
{
	ArvGvFakeCamera *gv_fake_camera = user_data;
	ArvBuffer *image_buffer = NULL;
	GError *error = NULL;
	GSocketAddress *stream_address = NULL;
	void *packet_buffer;
	size_t packet_size;
	size_t payload = 0;
	guint16 block_id;
	ptrdiff_t offset;
	guint32 gv_packet_size;
	GInputVector input_vector;
	int n_events;
	gboolean is_streaming = FALSE;

	input_vector.buffer = g_malloc0 (ARV_GV_FAKE_CAMERA_BUFFER_SIZE);
	input_vector.size = ARV_GV_FAKE_CAMERA_BUFFER_SIZE;

	packet_buffer = g_malloc (ARV_GV_FAKE_CAMERA_BUFFER_SIZE);

	do {
		guint64 next_timestamp_us;

		if (is_streaming) {
			arv_fake_camera_get_sleep_time_for_next_frame (gv_fake_camera->priv->camera, &next_timestamp_us);
		} else {
			next_timestamp_us = g_get_real_time () + 100000;
		}

		do {
			gint timeout_ms;

			timeout_ms =  (next_timestamp_us - g_get_real_time ()) / 1000LL;
			if (timeout_ms < 0)
				timeout_ms = 0;
			else if (timeout_ms > 100)
				timeout_ms = 100;

			n_events = g_poll (gv_fake_camera->priv->socket_fds, gv_fake_camera->priv->n_socket_fds, timeout_ms);
			if (n_events > 0) {
				unsigned int i;

				for (i = 0; i < ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS; i++) {
					GSocket *socket = gv_fake_camera->priv->input_sockets[i];
					int count;

					if (G_IS_SOCKET (socket)) {
						GSocketAddress *remote_address = NULL;

						arv_gpollfd_clear_one (&gv_fake_camera->priv->socket_fds[i], socket);

						count = g_socket_receive_message (socket, &remote_address, &input_vector, 1, NULL, NULL,
										  NULL, NULL, NULL);
						if (count > 0) {
							if (_handle_control_packet (gv_fake_camera, socket,
										    remote_address, input_vector.buffer, count))
								arv_info_device ("[GvFakeCamera::thread] Control packet received");
						}
						g_clear_object (&remote_address);
					}
				}

				if (arv_fake_camera_get_control_channel_privilege (gv_fake_camera->priv->camera) == 0 ||
				    arv_fake_camera_get_acquisition_status (gv_fake_camera->priv->camera) == 0) {
					if (stream_address != NULL) {
						g_object_unref (stream_address);
						stream_address = NULL;
						g_object_unref (image_buffer);
						image_buffer = NULL;
						arv_info_stream_thread ("[GvFakeCamera::thread] Stop stream");
					}
					is_streaming = FALSE;
				}
			}
		} while (!g_atomic_int_get (&gv_fake_camera->priv->cancel) && g_get_real_time () < next_timestamp_us);

		if (arv_fake_camera_get_control_channel_privilege (gv_fake_camera->priv->camera) != 0 &&
		    arv_fake_camera_get_acquisition_status (gv_fake_camera->priv->camera) != 0) {
			if (stream_address == NULL) {
				GInetAddress *inet_address;
				char *inet_address_string;

				stream_address = arv_fake_camera_get_stream_address (gv_fake_camera->priv->camera);
				inet_address = g_inet_socket_address_get_address
					(G_INET_SOCKET_ADDRESS (stream_address));
				inet_address_string = g_inet_address_to_string (inet_address);
				arv_info_stream_thread ("[GvFakeCamera::thread] Start stream to %s (%d)",
							 inet_address_string,
							 g_inet_socket_address_get_port
							 (G_INET_SOCKET_ADDRESS (stream_address)));
				g_free (inet_address_string);

				payload = arv_fake_camera_get_payload (gv_fake_camera->priv->camera);
				image_buffer = arv_buffer_new (payload, NULL);
			}

			if (arv_fake_camera_is_in_free_running_mode (gv_fake_camera->priv->camera) ||
			    (arv_fake_camera_is_in_software_trigger_mode (gv_fake_camera->priv->camera) &&
			     arv_fake_camera_check_and_acknowledge_software_trigger (gv_fake_camera->priv->camera))) {
				arv_fake_camera_fill_buffer (gv_fake_camera->priv->camera, image_buffer, &gv_packet_size);

				arv_info_stream_thread ("[GvFakeCamera::thread] Send frame %" G_GUINT64_FORMAT, image_buffer->priv->frame_id);

				block_id = 0;

				packet_size = ARV_GV_FAKE_CAMERA_BUFFER_SIZE;
                                arv_gvsp_packet_new_image_leader (image_buffer->priv->frame_id,
                                                                  block_id,
                                                                  arv_buffer_get_timestamp(image_buffer),
                                                                  arv_buffer_get_image_pixel_format(image_buffer),
                                                                  arv_buffer_get_image_width(image_buffer),
                                                                  arv_buffer_get_image_height(image_buffer),
                                                                  arv_buffer_get_image_x(image_buffer),
                                                                  arv_buffer_get_image_y(image_buffer),
                                                                  0, 0,
                                                                  packet_buffer, &packet_size);

				if (g_random_double () >= gv_fake_camera->priv->gvsp_lost_packet_ratio)
					g_socket_send_to (gv_fake_camera->priv->gvsp_socket, stream_address,
							packet_buffer, packet_size, NULL, &error);
				else
					arv_info_stream_thread ("Drop GVSP leader packet frame: %" G_GUINT64_FORMAT, image_buffer->priv->frame_id);

				if (error != NULL) {
					arv_warning_stream_thread ("[GvFakeCamera::thread] Failed to send leader for frame %" G_GUINT64_FORMAT
								": %s", image_buffer->priv->frame_id, error->message);
					g_clear_error (&error);
				}

				block_id++;

				offset = 0;
				while (offset < payload) {
					size_t data_size;

					data_size = MIN (gv_packet_size - ARV_GVSP_PACKET_PROTOCOL_OVERHEAD (FALSE),
							payload - offset);

					packet_size = ARV_GV_FAKE_CAMERA_BUFFER_SIZE;
                                        arv_gvsp_packet_new_payload (image_buffer->priv->frame_id, block_id,
                                                                     data_size, ((char *) image_buffer->priv->data) + offset,
                                                                     packet_buffer, &packet_size);

					if (g_random_double () >= gv_fake_camera->priv->gvsp_lost_packet_ratio)
						g_socket_send_to (gv_fake_camera->priv->gvsp_socket, stream_address,
								packet_buffer, packet_size, NULL, &error);
					else
						arv_info_stream_thread ("Drop GVSP data packet frame:%" G_GUINT64_FORMAT
									", block:%u", image_buffer->priv->frame_id, block_id);

					if (error != NULL) {
						arv_info_stream_thread ("[GvFakeCamera::thread] Failed to send frame block %d for frame"
									" %" G_GUINT64_FORMAT ": %s",
									block_id, image_buffer->priv->frame_id, error->message);
						g_clear_error (&error);
					}

					offset += data_size;
					block_id++;
				}

				packet_size = ARV_GV_FAKE_CAMERA_BUFFER_SIZE;
				arv_gvsp_packet_new_data_trailer (image_buffer->priv->frame_id, block_id,
								packet_buffer, &packet_size);

				if (g_random_double () >= gv_fake_camera->priv->gvsp_lost_packet_ratio)
					g_socket_send_to (gv_fake_camera->priv->gvsp_socket, stream_address,
							packet_buffer, packet_size, NULL, &error);
				else
					arv_info_stream_thread ("Drop GVSP trailer packet frame: %" G_GUINT64_FORMAT,
								image_buffer->priv->frame_id);

				if (error != NULL) {
					arv_info_stream_thread ("[GvFakeCamera::thread] Failed to send trailer for frame %" G_GUINT64_FORMAT
								": %s", image_buffer->priv->frame_id, error->message);
					g_clear_error (&error);
				}

				is_streaming = TRUE;
			}
		}

	} while (!g_atomic_int_get (&gv_fake_camera->priv->cancel));

	if (stream_address != NULL)
		g_object_unref (stream_address);
	if (image_buffer != NULL)
		g_object_unref (image_buffer);

	g_free (packet_buffer);
	g_free (input_vector.buffer);

	return NULL;
}

static gboolean
_create_and_bind_input_socket (GSocket **socket_out, const char *socket_name,
			       GInetAddress *inet_address, unsigned int port,
			       gboolean allow_reuse, gboolean blocking)
{
	GSocket *socket;
	GSocketAddress *socket_address;
	GError *error = NULL;
	gboolean success;
	char *address_string;

	address_string = g_inet_address_to_string (inet_address);
	if (port > 0)
		arv_info_device ("%s address = %s:%d",socket_name,  address_string, port);
	else
		arv_info_device ("%s address = %s",socket_name,  address_string);
	g_clear_pointer (&address_string, g_free);

	socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, NULL);
	if (!G_IS_SOCKET (socket)) {
		*socket_out = NULL;
		return FALSE;
	}

	socket_address = g_inet_socket_address_new (inet_address, port);
	success = g_socket_bind (socket, socket_address, allow_reuse, &error);

	if (error != NULL) {
		arv_warning_device ("Failed to bind %s socket: %s", socket_name, error->message);
		g_clear_error (&error);
	}

	g_clear_object (&socket_address);

	if (success)
		g_socket_set_blocking (socket, blocking);
	else
		g_clear_object (&socket);

	*socket_out = socket;

	return G_IS_SOCKET (socket);
}

static gboolean
arv_gv_fake_camera_start (ArvGvFakeCamera *gv_fake_camera)
{
	ArvNetworkInterface *iface;
	GSocketAddress *socket_address;
	GInetAddress *inet_address;
	GInetAddress *gvcp_inet_address;
	unsigned int i;
	unsigned int n_socket_fds;

	g_return_val_if_fail (ARV_IS_GV_FAKE_CAMERA (gv_fake_camera), FALSE);

	/* get the interface by name or address */
	iface = arv_network_get_interface_by_address(gv_fake_camera->priv->interface_name);
	if (iface == NULL) iface = arv_network_get_interface_by_name(gv_fake_camera->priv->interface_name);
	#ifdef G_OS_WIN32
		/* fake the default loopback interface in windows, in case not found */
		if(iface == NULL && g_strcmp0 (gv_fake_camera->priv->interface_name,ARV_GV_FAKE_CAMERA_DEFAULT_INTERFACE) == 0)
			iface = arv_network_get_fake_ipv4_loopback();
	#endif
	if (iface == NULL) {
		arv_warning_device ("[GvFakeCamera::start] No network interface with address or name '%s' found.",gv_fake_camera->priv->interface_name);
		return FALSE;
	}

	socket_address = g_socket_address_new_from_native (arv_network_interface_get_addr(iface),
								sizeof (struct sockaddr));
	gvcp_inet_address = g_object_ref (g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (socket_address)));
	arv_fake_camera_set_inet_address (gv_fake_camera->priv->camera, gvcp_inet_address);

	_create_and_bind_input_socket (&gv_fake_camera->priv->gvsp_socket,
								 "GVSP", gvcp_inet_address, 0, FALSE, TRUE);
	_create_and_bind_input_socket
		(&gv_fake_camera->priv->input_sockets[ARV_GV_FAKE_CAMERA_INPUT_SOCKET_GVCP],
		 "GVCP", gvcp_inet_address, ARV_GVCP_PORT, FALSE, FALSE);

	inet_address = g_inet_address_new_from_string ("255.255.255.255");
	if (!g_inet_address_equal (gvcp_inet_address, inet_address))
		_create_and_bind_input_socket
			(&gv_fake_camera->priv->input_sockets[ARV_GV_FAKE_CAMERA_INPUT_SOCKET_GLOBAL_DISCOVERY],
			 "Global discovery", inet_address, ARV_GVCP_PORT, TRUE, FALSE);
	g_clear_object (&inet_address);
	g_clear_object (&socket_address);

	socket_address = g_socket_address_new_from_native (arv_network_interface_get_broadaddr(iface),
								sizeof (struct sockaddr));
	inet_address = g_object_ref (g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (socket_address)));
	if (!g_inet_address_equal (gvcp_inet_address, inet_address))
		_create_and_bind_input_socket
			(&gv_fake_camera->priv->input_sockets[ARV_GV_FAKE_CAMERA_INPUT_SOCKET_SUBNET_DISCOVERY],
			 "Subnet discovery", inet_address, ARV_GVCP_PORT, FALSE, FALSE);
	g_clear_object (&inet_address);
	g_clear_object (&socket_address);

	g_clear_object (&gvcp_inet_address);

	arv_network_interface_free(iface);

	n_socket_fds = 0;
	for (i = 0; i < ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS; i++) {
		GSocket *socket = gv_fake_camera->priv->input_sockets[i];

		if (G_IS_SOCKET (socket)) {
			gv_fake_camera->priv->socket_fds[n_socket_fds].fd = g_socket_get_fd (socket);
			gv_fake_camera->priv->socket_fds[n_socket_fds].events = G_IO_IN;
			gv_fake_camera->priv->socket_fds[n_socket_fds].revents = 0;

			n_socket_fds++;
		}
	}
	arv_info_device ("Listening to %d sockets", n_socket_fds);

	gv_fake_camera->priv->n_socket_fds = n_socket_fds;

	arv_gpollfd_prepare_all (gv_fake_camera->priv->socket_fds, n_socket_fds);

	gv_fake_camera->priv->cancel = FALSE;
	gv_fake_camera->priv->thread = g_thread_new ("arv_fake_gv_fake_camera", _thread, gv_fake_camera);

	return TRUE;
}


static void
arv_gv_fake_camera_stop (ArvGvFakeCamera *gv_fake_camera)
{
	unsigned int i;

	g_return_if_fail (ARV_IS_GV_FAKE_CAMERA (gv_fake_camera));

	if (gv_fake_camera->priv->thread != NULL) {

		g_atomic_int_set (&gv_fake_camera->priv->cancel, TRUE);
		g_thread_join (gv_fake_camera->priv->thread);
		gv_fake_camera->priv->thread = NULL;
	}

	arv_gpollfd_finish_all (gv_fake_camera->priv->socket_fds, gv_fake_camera->priv->n_socket_fds);

	for (i = 0; i < ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS; i++) {
		g_clear_object (&gv_fake_camera->priv->input_sockets[i]);
	}
	g_clear_object (&gv_fake_camera->priv->gvsp_socket);

	g_clear_object (&gv_fake_camera->priv->controller_address);
}

/**
 * arv_gv_fake_camera_new_full:
 * @interface_name: (nullable): listening network interface, by name or IP address, default is 127.0.0.1
 * @serial_number: (nullable): fake device serial number, default is GV01
 * @genicam_filename: (nullable): path to alternative genicam data
 *
 * Returns: a new #ArvGvFakeCamera
 *
 * Since: 0.8.0
 */

ArvGvFakeCamera *
arv_gv_fake_camera_new_full (const char *interface_name, const char *serial_number, const char *genicam_filename)
{
	return g_object_new (ARV_TYPE_GV_FAKE_CAMERA,
			     "interface-name", interface_name != NULL ?
			     interface_name : ARV_GV_FAKE_CAMERA_DEFAULT_INTERFACE,
			     "serial-number", serial_number != NULL ?
			     serial_number : ARV_GV_FAKE_CAMERA_DEFAULT_SERIAL_NUMBER,
			     "genicam-filename", genicam_filename,
			     NULL);
}

/**
 * arv_gv_fake_camera_new:
 * @interface_name: (nullable): listening network interface ('lo' by default)
 * @serial_number: (nullable): fake device serial number ('GV01' by default)
 *
 * Returns: a new #ArvGvFakeCamera
 *
 * Since: 0.8.0
 */

ArvGvFakeCamera *
arv_gv_fake_camera_new (const char *interface_name, const char *serial_number)
{
	return arv_gv_fake_camera_new_full (interface_name, serial_number, NULL);
}

static void
_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ArvGvFakeCamera *gv_fake_camera = ARV_GV_FAKE_CAMERA (object);

	switch (prop_id)
	{
		case PROP_INTERFACE_NAME:
			g_free (gv_fake_camera->priv->interface_name);
			gv_fake_camera->priv->interface_name = g_value_dup_string (value);
			break;
		case PROP_SERIAL_NUMBER:
			g_free (gv_fake_camera->priv->serial_number);
			gv_fake_camera->priv->serial_number = g_value_dup_string (value);
			break;
		case PROP_GENICAM_FILENAME:
			g_free (gv_fake_camera->priv->genicam_filename);
			gv_fake_camera->priv->genicam_filename = g_value_dup_string (value);
			break;
		case PROP_GVSP_LOST_PACKET_RATIO:
			gv_fake_camera->priv->gvsp_lost_packet_ratio = g_value_get_double (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

/**
 * arv_gv_fake_camera_get_fake_camera:
 * @gv_fake_camera: a #ArvGvFakeCamera
 *
 * Retrieves the underlying #ArvFakeCamera object owned by @gv_fake_camera.
 *
 * Returns: (transfer none): underlying fake camera object.
 *
 * Since: 0.8.0
 */

ArvFakeCamera *
arv_gv_fake_camera_get_fake_camera (ArvGvFakeCamera *gv_fake_camera)
{
	g_return_val_if_fail (ARV_IS_GV_FAKE_CAMERA (gv_fake_camera), NULL);

        return gv_fake_camera->priv->camera;
}

/**
 * arv_gv_fake_camera_is_running:
 * @gv_fake_camera: a #ArvGvFakeCamera
 *
 * Returns: %TRUE if the fake camera is correctly listening on the GVCP port
 *
 * Since: 0.8.0
 */

gboolean
arv_gv_fake_camera_is_running (ArvGvFakeCamera *gv_fake_camera)
{
	g_return_val_if_fail (ARV_IS_GV_FAKE_CAMERA (gv_fake_camera), FALSE);

	return gv_fake_camera->priv->is_running;
}

static void
arv_gv_fake_camera_init (ArvGvFakeCamera *gv_fake_camera)
{
	gv_fake_camera->priv = arv_gv_fake_camera_get_instance_private (gv_fake_camera);
}

static void
_constructed (GObject *gobject)
{
	ArvGvFakeCamera *gv_fake_camera = ARV_GV_FAKE_CAMERA (gobject);

	G_OBJECT_CLASS (arv_gv_fake_camera_parent_class)->constructed (gobject);

	gv_fake_camera->priv->camera = arv_fake_camera_new_full (gv_fake_camera->priv->serial_number, gv_fake_camera->priv->genicam_filename);
	gv_fake_camera->priv->is_running = arv_gv_fake_camera_start (gv_fake_camera);
}

static void
_finalize (GObject *object)
{
	ArvGvFakeCamera *gv_fake_camera = ARV_GV_FAKE_CAMERA (object);

	gv_fake_camera->priv->is_running = FALSE;

	arv_gv_fake_camera_stop (gv_fake_camera);

	g_object_unref (gv_fake_camera->priv->camera);

	g_clear_pointer (&gv_fake_camera->priv->interface_name, g_free);
	g_clear_pointer (&gv_fake_camera->priv->serial_number, g_free);
	g_clear_pointer (&gv_fake_camera->priv->genicam_filename, g_free);

	G_OBJECT_CLASS (arv_gv_fake_camera_parent_class)->finalize (object);
}

static void
arv_gv_fake_camera_class_init (ArvGvFakeCameraClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);

	object_class->set_property = _set_property;
	object_class->constructed = _constructed;
	object_class->finalize = _finalize;

	g_object_class_install_property (object_class,
					 PROP_INTERFACE_NAME,
					 g_param_spec_string ("interface-name",
							      "Interface name",
							      "Interface name",
							      ARV_GV_FAKE_CAMERA_DEFAULT_INTERFACE,
							      G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_SERIAL_NUMBER,
					 g_param_spec_string ("serial-number",
							      "Serial number",
							      "Serial number",
							      ARV_GV_FAKE_CAMERA_DEFAULT_SERIAL_NUMBER,
							      G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_GENICAM_FILENAME,
					 g_param_spec_string ("genicam-filename",
							      "Genicam filename",
							      "Genicam filename",
							      NULL,
							      G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class,
					 PROP_GVSP_LOST_PACKET_RATIO,
					 g_param_spec_double ("gvsp-lost-ratio",
							      "GVSP lost packet ratio",
							      "GVSP lost packet ratio",
							      0.0, 1.0, 0.0,
							      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
}
