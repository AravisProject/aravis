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

#include <arvgvfakecamera.h>
#include <arvfakecamera.h>
#include <arvbufferprivate.h>
#include <arvgvcp.h>
#include <arvgvsp.h>
#include <arvmisc.h>
#include <net/if.h>
#include <ifaddrs.h>

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
  PROP_CM_DOMAIN
};

static GObjectClass *parent_class = NULL;

struct _ArvGvFakeCameraPrivate {
	char *interface_name;
	ArvFakeCamera *camera;

	GPollFD socket_fds[3];
	guint n_socket_fds;

	GSocketAddress *controller_address;
	gint64 controller_time;

	GSocket *input_sockets[ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS];

	GSocket *gvsp_socket;

	GThread *thread;
	gboolean cancel;
};

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

gboolean
handle_control_packet (ArvGvFakeCamera *gv_fake_camera, GSocket *socket,
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


	arv_gvcp_packet_debug (packet, ARV_DEBUG_LEVEL_LOG);

	packet_id = arv_gvcp_packet_get_packet_id (packet);
	packet_type = arv_gvcp_packet_get_packet_type (packet);

	if (packet_type != ARV_GVCP_PACKET_TYPE_CMD) {
		arv_warning_device ("[GvFakeCamera::handle_control_packet] Unknown packet type");
		return FALSE;
	}

	switch (g_ntohs (packet->header.command)) {
		case ARV_GVCP_COMMAND_DISCOVERY_CMD:
			ack_packet = arv_gvcp_packet_new_discovery_ack (packet_id, &ack_packet_size);
			arv_debug_device ("[GvFakeCamera::handle_control_packet] Discovery command");
			arv_fake_camera_read_memory (gv_fake_camera->priv->camera, 0, ARV_GVBS_DISCOVERY_DATA_SIZE,
						     &ack_packet->data);
			break;
		case ARV_GVCP_COMMAND_READ_MEMORY_CMD:
			arv_gvcp_packet_get_read_memory_cmd_infos (packet, &block_address, &block_size);
			arv_debug_device ("[GvFakeCamera::handle_control_packet] Read memory command %d (%d)",
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

			arv_debug_device ("[GvFakeCamera::handle_control_packet] Write memory command %d (%d)",
					  block_address, block_size);
			arv_fake_camera_write_memory (gv_fake_camera->priv->camera, block_address, block_size,
						      arv_gvcp_packet_get_write_memory_cmd_data (packet));
			ack_packet = arv_gvcp_packet_new_write_memory_ack (block_address, packet_id,
									   &ack_packet_size);
			break;
		case ARV_GVCP_COMMAND_READ_REGISTER_CMD:
			arv_gvcp_packet_get_read_register_cmd_infos (packet, &register_address);
			arv_fake_camera_read_register (gv_fake_camera->priv->camera, register_address, &register_value);
			arv_debug_device ("[GvFakeCamera::handle_control_packet] Read register command %d -> %d",
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
			arv_debug_device ("[GvFakeCamera::handle_control_packet] Write register command %d -> %d",
					  register_address, register_value);
			ack_packet = arv_gvcp_packet_new_write_register_ack (1, packet_id,
									     &ack_packet_size);
			break;
		default:
			arv_warning_device ("[GvFakeCamera::handle_control_packet] Unknown command");
	}

	if (ack_packet != NULL) {
		g_socket_send_to (socket, remote_address, (char *) ack_packet, ack_packet_size, NULL, NULL);
		arv_gvcp_packet_debug (ack_packet, ARV_DEBUG_LEVEL_LOG);
		g_free (ack_packet);

		success = TRUE;
	}

	if (gv_fake_camera->priv->controller_address == NULL &&
	    arv_fake_camera_get_control_channel_privilege (gv_fake_camera->priv->camera) != 0) {
		g_object_ref (remote_address);
		arv_debug_device("[GvFakeCamera::handle_control_packet] New controller");
		gv_fake_camera->priv->controller_address = remote_address;
		gv_fake_camera->priv->controller_time = g_get_real_time ();
	}
	else if (gv_fake_camera->priv->controller_address != NULL &&
	    arv_fake_camera_get_control_channel_privilege (gv_fake_camera->priv->camera) == 0) {
		g_object_unref (gv_fake_camera->priv->controller_address);
		arv_debug_device("[GvFakeCamera::handle_control_packet] Controller releases");
		gv_fake_camera->priv->controller_address = NULL;
		gv_fake_camera->priv->controller_time = g_get_real_time ();
	}

	return success;
}

void *
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
		guint64 sleep_time_us;

		if (is_streaming) {
			sleep_time_us = arv_fake_camera_get_sleep_time_for_next_frame (gv_fake_camera->priv->camera, &next_timestamp_us);
		} else {
			sleep_time_us = 100000;
			next_timestamp_us = g_get_real_time () + sleep_time_us;
		}

		do {
			gint timeout_ms;

			timeout_ms = MIN (100, (next_timestamp_us - g_get_real_time ()) / 1000LL);
			if (timeout_ms < 0)
				timeout_ms = 0;

			n_events = g_poll (gv_fake_camera->priv->socket_fds, gv_fake_camera->priv->n_socket_fds, timeout_ms);
			if (n_events > 0) {
				unsigned int i;

				for (i = 0; i < ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS; i++) {
					GSocket *socket = gv_fake_camera->priv->input_sockets[i];
					int count;

					if (G_IS_SOCKET (socket)) {
						GSocketAddress *remote_address = NULL;

						count = g_socket_receive_message (socket, &remote_address, &input_vector, 1, NULL, NULL,
										  NULL, NULL, NULL);
						if (count > 0) {
							if (handle_control_packet (gv_fake_camera, socket,
										   remote_address, input_vector.buffer, count))
								arv_debug_device ("[GvFakeCamera::thread] Control packet received");
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
						arv_debug_stream_thread ("[GvFakeCamera::thread] Stop stream");
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
				arv_debug_stream_thread ("[GvFakeCamera::thread] Start stream to %s (%d)",
							 inet_address_string,
							 g_inet_socket_address_get_port
							 (G_INET_SOCKET_ADDRESS (stream_address)));
				g_free (inet_address_string);

				payload = arv_fake_camera_get_payload (gv_fake_camera->priv->camera);
				image_buffer = arv_buffer_new (payload, NULL);
			}

			arv_fake_camera_fill_buffer (gv_fake_camera->priv->camera, image_buffer, &gv_packet_size);

			arv_debug_stream_thread ("[GvFakeCamera::thread] Send frame %d", image_buffer->priv->frame_id);

			block_id = 0;

			packet_size = ARV_GV_FAKE_CAMERA_BUFFER_SIZE;
			arv_gvsp_packet_new_data_leader (image_buffer->priv->frame_id,
							 block_id,
							 image_buffer->priv->timestamp_ns,
							 image_buffer->priv->pixel_format,
							 image_buffer->priv->width, image_buffer->priv->height,
							 image_buffer->priv->x_offset, image_buffer->priv->y_offset,
							 packet_buffer, &packet_size);

			g_socket_send_to (gv_fake_camera->priv->gvsp_socket, stream_address,
					  packet_buffer, packet_size, NULL, &error);
			if (error != NULL) {
				arv_warning_stream_thread ("[GvFakeCamera::thread] Failed to send leader for frame %d: %s",
							   image_buffer->priv->frame_id, error->message);
				g_clear_error (&error);
			}

			block_id++;

			offset = 0;
			while (offset < payload) {
				size_t data_size;

				data_size = MIN (gv_packet_size - ARV_GVSP_PACKET_PROTOCOL_OVERHEAD,
						 payload - offset);

				packet_size = ARV_GV_FAKE_CAMERA_BUFFER_SIZE;
				arv_gvsp_packet_new_data_block (image_buffer->priv->frame_id, block_id,
								data_size, ((char *) image_buffer->priv->data) + offset,
								packet_buffer, &packet_size);

				g_socket_send_to (gv_fake_camera->priv->gvsp_socket, stream_address,
						  packet_buffer, packet_size, NULL, &error);
				if (error != NULL) {
					arv_debug_stream_thread ("[GvFakeCamera::thread] Failed to send frame block %d for frame: %s",
								 block_id,
								 image_buffer->priv->frame_id,
								 error->message);
					g_clear_error (&error);
				}

				offset += data_size;
				block_id++;
			}

			packet_size = ARV_GV_FAKE_CAMERA_BUFFER_SIZE;
			arv_gvsp_packet_new_data_trailer (image_buffer->priv->frame_id, block_id,
							  packet_buffer, &packet_size);

			g_socket_send_to (gv_fake_camera->priv->gvsp_socket, stream_address,
					  packet_buffer, packet_size, NULL, &error);
			if (error != NULL) {
				arv_debug_stream_thread ("[GvFakeCamera::thread] Failed to send trailer for frame %d: %s",
							 image_buffer->priv->frame_id,
							 error->message);
				g_clear_error (&error);
			}

			is_streaming = TRUE;
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

gboolean
_create_and_bind_input_socket (GSocket **socket_out, const char *socket_name, GInetAddress *inet_address, unsigned int port, gboolean allow_reuse, gboolean blocking)
{
	GSocket *socket;
	GSocketAddress *socket_address;
	GError *error = NULL;
	gboolean success;
	char *address_string;

	address_string = g_inet_address_to_string (inet_address);
	if (port > 0)
		arv_debug_device ("%s address = %s:%d",socket_name,  address_string, port);
	else
		arv_debug_device ("%s address = %s",socket_name,  address_string);
	g_clear_pointer (&address_string, g_free);

	socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, NULL);
	if (!G_IS_SOCKET (socket)) {
		*socket_out = NULL;
		return FALSE;
	}

	socket_address = g_inet_socket_address_new (inet_address, port);
	success = g_socket_bind (socket, socket_address, allow_reuse, &error);

	if (error != NULL) {
		arv_warning_device ("Failed to bind socket: %s", error->message);
		g_clear_error (&error);
	}

	g_clear_object (&socket_address);

	if (success)
		g_socket_set_blocking (socket, FALSE);
	else
		g_clear_object (&socket);

	*socket_out = socket;

	return G_IS_SOCKET (socket);
}

gboolean
arv_gv_fake_camera_start (ArvGvFakeCamera *gv_fake_camera)
{
	struct ifaddrs *ifap = NULL;
	struct ifaddrs *ifap_iter;
	int return_value;
	gboolean interface_found = FALSE;
	gboolean success = TRUE;

	g_return_val_if_fail (ARV_IS_GV_FAKE_CAMERA (gv_fake_camera), FALSE);

	return_value = getifaddrs (&ifap);
	if (return_value < 0) {
		arv_warning_device ("[GvFakeCamera::start] No network interface found");
		return FALSE;
	}

	for (ifap_iter = ifap ;ifap_iter != NULL && !interface_found; ifap_iter = ifap_iter->ifa_next) {
		if ((ifap_iter->ifa_flags & IFF_UP) != 0 &&
		    (ifap_iter->ifa_flags & IFF_POINTOPOINT) == 0 &&
		    (ifap_iter->ifa_addr->sa_family == AF_INET) &&
		    g_strcmp0 (ifap_iter->ifa_name, gv_fake_camera->priv->interface_name) == 0) {
			GSocketAddress *socket_address;
			GInetAddress *inet_address;
			GInetAddress *gvcp_inet_address;
			unsigned int n_socket_fds;
			unsigned int i;

			socket_address = g_socket_address_new_from_native (ifap_iter->ifa_addr,
									   sizeof (struct sockaddr));
			gvcp_inet_address = g_object_ref (g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (socket_address)));
			g_clear_object (&socket_address);
			arv_fake_camera_set_inet_address (gv_fake_camera->priv->camera, gvcp_inet_address);

			success = success && _create_and_bind_input_socket (&gv_fake_camera->priv->gvsp_socket,
									    "GVSP", gvcp_inet_address, 0, FALSE, FALSE);
			success = success && _create_and_bind_input_socket
				(&gv_fake_camera->priv->input_sockets[ARV_GV_FAKE_CAMERA_INPUT_SOCKET_GVCP],
				 "GVCP", gvcp_inet_address, ARV_GVCP_PORT, FALSE, FALSE);

			inet_address = g_inet_address_new_from_string ("255.255.255.255");
			if (!g_inet_address_equal (gvcp_inet_address, inet_address) != 0)
				success = success && _create_and_bind_input_socket 
					(&gv_fake_camera->priv->input_sockets[ARV_GV_FAKE_CAMERA_INPUT_SOCKET_GLOBAL_DISCOVERY],
					 "Global discovery", inet_address, ARV_GVCP_PORT, TRUE, FALSE);
			g_clear_object (&inet_address);

			socket_address = g_socket_address_new_from_native (ifap_iter->ifa_broadaddr,
									   sizeof (struct sockaddr));
                        inet_address = g_object_ref (g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (socket_address)));
			g_clear_object (&socket_address);
			if (!g_inet_address_equal (gvcp_inet_address, inet_address))
				success = success && _create_and_bind_input_socket 
					(&gv_fake_camera->priv->input_sockets[ARV_GV_FAKE_CAMERA_INPUT_SOCKET_SUBNET_DISCOVERY],
					 "Subnet discovery", inet_address, ARV_GVCP_PORT, FALSE, FALSE);
			g_clear_object (&inet_address);

			g_clear_object (&gvcp_inet_address);

			n_socket_fds = 0;
			if (success) {
				for (i = 0; i < ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS; i++) {
					GSocket *socket = gv_fake_camera->priv->input_sockets[i];

					if (G_IS_SOCKET (socket)) {
						gv_fake_camera->priv->socket_fds[n_socket_fds].fd = g_socket_get_fd (socket);
						gv_fake_camera->priv->socket_fds[n_socket_fds].events = G_IO_IN;
						gv_fake_camera->priv->socket_fds[n_socket_fds].revents = 0;

						n_socket_fds++;
					}
				}

				arv_debug_device ("Listening to %d sockets", n_socket_fds);
			}

			gv_fake_camera->priv->n_socket_fds = n_socket_fds;

			interface_found = TRUE;
		}
	}

	freeifaddrs (ifap);

	if (!success) {
		unsigned int i;

		for (i = 0; i < ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS; i++)
			g_clear_object (&gv_fake_camera->priv->input_sockets[i]);
		g_clear_object (&gv_fake_camera->priv->gvsp_socket);

		return FALSE;
	}

	if (!interface_found) {
		return FALSE;
	}

	gv_fake_camera->priv->cancel = FALSE;
	gv_fake_camera->priv->thread = g_thread_new ("arv_fake_gv_fake_camera", _thread, gv_fake_camera);

	return TRUE;
}

void
arv_gv_fake_camera_stop (ArvGvFakeCamera *gv_fake_camera)
{
	unsigned int i;

	g_return_if_fail (ARV_IS_GV_FAKE_CAMERA (gv_fake_camera));

	if (gv_fake_camera->priv->thread == NULL)
		return;

	g_atomic_int_set (&gv_fake_camera->priv->cancel, TRUE);
	g_thread_join (gv_fake_camera->priv->thread);
	gv_fake_camera->priv->thread = NULL;

	for (i = 0; i < ARV_GV_FAKE_CAMERA_N_INPUT_SOCKETS; i++)
		g_clear_object (&gv_fake_camera->priv->input_sockets[i]);
	g_clear_object (&gv_fake_camera->priv->gvsp_socket);

	g_clear_object (&gv_fake_camera->priv->controller_address);
}

ArvGvFakeCamera *
arv_gv_fake_camera_new (const char *interface_name)
{
	return g_object_new (ARV_TYPE_GV_FAKE_CAMERA, "interface-name", interface_name, NULL);
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

static void
arv_gv_fake_camera_init (ArvGvFakeCamera *gv_fake_camera)
{
	gv_fake_camera->priv = G_TYPE_INSTANCE_GET_PRIVATE (gv_fake_camera, ARV_TYPE_GV_FAKE_CAMERA, ArvGvFakeCameraPrivate);
	gv_fake_camera->priv->interface_name = g_strdup ("lo");
}

static void
_constructed (GObject *gobject)
{
	ArvGvFakeCamera *gv_fake_camera = ARV_GV_FAKE_CAMERA (gobject);

	parent_class->constructed (gobject);

	gv_fake_camera->priv->camera = arv_fake_camera_new ("GV01");

}

static void
_finalize (GObject *object)
{
	ArvGvFakeCamera *gv_fake_camera = ARV_GV_FAKE_CAMERA (object);

	g_object_unref (gv_fake_camera->priv->camera);

	g_free (gv_fake_camera->priv->interface_name);

	parent_class->finalize (object);
}

static void
arv_gv_fake_camera_class_init (ArvGvFakeCameraClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);

#if !GLIB_CHECK_VERSION(2,38,0)
	g_type_class_add_private (this_class, sizeof (ArvGvFakeCameraPrivate));
#endif

	parent_class = g_type_class_peek_parent (this_class);

	object_class->set_property = _set_property;
	object_class->constructed = _constructed;
	object_class->finalize = _finalize;

	g_object_class_install_property (object_class,
					 PROP_INTERFACE_NAME,
					 g_param_spec_string ("interface-name",
							      "Interface name",
							      "Interface name",
							      NULL,
							      G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));
}

#if !GLIB_CHECK_VERSION(2,38,0)
G_DEFINE_TYPE (ArvGvFakeCamera, arv_gv_fake_camera, G_TYPE_OBJECT)
#else
G_DEFINE_TYPE_WITH_CODE (ArvGvFakeCamera, arv_gv_fake_camera, G_TYPE_OBJECT, G_ADD_PRIVATE (ArvGvFakeCamera))
#endif
