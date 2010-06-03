#include <arv.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <ifaddrs.h>

#define ARV_FAKE_GV_CAMERA_BUFFER_SIZE	65536

static gboolean cancel = FALSE;

static void
set_cancel (int signal)
{
	cancel = TRUE;
}

typedef struct {
	ArvFakeCamera *camera;
	GPollFD gvcp_fds[2];
	guint n_gvcp_fds;
	GSocket *gvcp_socket;
	GSocket *gvsp_socket;
	GSocket *discovery_socket;

	GThread *gvsp_thread;
	gboolean cancel;
} ArvFakeGvCamera;

void *
arv_fake_gv_camera_thread (void *user_data)
{
	ArvFakeGvCamera *gv_camera = user_data;
	ArvBuffer *image_buffer = NULL;
	GError *error = NULL;
	GSocketAddress *stream_address = NULL;
	void *packet_buffer;
	size_t packet_size;
	size_t payload = 0;
	guint32 frame_id = 0;
	guint16 block_id;
	ptrdiff_t offset;

	packet_buffer = g_malloc (ARV_FAKE_GV_CAMERA_BUFFER_SIZE);

	do {
		if (arv_fake_camera_get_acquisition_status (gv_camera->camera) == 0) {
			if (stream_address != NULL) {
				g_object_unref (stream_address);
				stream_address = NULL;
				g_object_unref (image_buffer);
				image_buffer = NULL;
				arv_debug ("camera", "[FakeGvCamera::stream_thread] Stop stream");
			}
			g_usleep (100000);
		} else {
			if (stream_address == NULL) {
				GInetAddress *inet_address;
				char *inet_address_string;

				stream_address = arv_fake_camera_get_stream_address (gv_camera->camera);
				inet_address = g_inet_socket_address_get_address
					(G_INET_SOCKET_ADDRESS (stream_address));
				inet_address_string = g_inet_address_to_string (inet_address);
				arv_debug ("camera", "[FakeGvCamera::stream_thread] Start stream to %s (%d)",
					   inet_address_string,
					   g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (stream_address)));
				g_free (inet_address_string);

				payload = arv_fake_camera_get_payload (gv_camera->camera);
				image_buffer = arv_buffer_new (payload, NULL);
			}

			arv_fake_camera_wait_for_next_frame (gv_camera->camera);
			arv_fake_camera_fill_buffer (gv_camera->camera, image_buffer);

			block_id = 0;

			packet_size = ARV_FAKE_GV_CAMERA_BUFFER_SIZE;
			arv_gvsp_packet_new_data_leader (image_buffer->frame_id,
							 block_id,
							 image_buffer->timestamp_ns,
							 image_buffer->pixel_format,
							 image_buffer->width, image_buffer->height,
							 image_buffer->x_offset, image_buffer->y_offset,
							 packet_buffer, &packet_size);

			g_socket_send_to (gv_camera->gvsp_socket, stream_address,
					  packet_buffer, packet_size, NULL, &error);
			if (error != NULL) {
				arv_debug ("camera", "[ArvFakeGvCamera::stream_thread] Socket send error [%s]",
					   error->message);
				g_error_free (error);
				error = NULL;
			}

			block_id++;

			offset = 0;
			while (offset < payload) {
				size_t data_size;

				data_size = MIN (1500, payload - offset);

				packet_size = ARV_FAKE_GV_CAMERA_BUFFER_SIZE;
				arv_gvsp_packet_new_data_block (image_buffer->frame_id, block_id,
								data_size, image_buffer->data + offset,
								packet_buffer, &packet_size);

				g_socket_send_to (gv_camera->gvsp_socket, stream_address,
						  packet_buffer, packet_size, NULL, NULL);

				offset += data_size;
				block_id++;
			}

			packet_size = ARV_FAKE_GV_CAMERA_BUFFER_SIZE;
			arv_gvsp_packet_new_data_trailer (image_buffer->frame_id, block_id,
							  packet_buffer, &packet_size);

			g_socket_send_to (gv_camera->gvsp_socket, stream_address,
					  packet_buffer, packet_size, NULL, NULL);

			frame_id++;
		}
	} while (!cancel);

	if (stream_address != NULL)
		g_object_unref (stream_address);
	if (image_buffer != NULL)
		g_object_unref (image_buffer);

	g_free (packet_buffer);

	return NULL;
}

ArvFakeGvCamera *
arv_fake_gv_camera_new (const char *interface_name)
{
	ArvFakeGvCamera *gv_camera;
	struct ifaddrs *ifap;
	int n_interfaces;
	gboolean interface_found = FALSE;

	g_return_val_if_fail (interface_name != NULL, NULL);

	gv_camera = g_new0 (ArvFakeGvCamera, 1);
	gv_camera->camera = arv_fake_camera_new ("GV01");

	n_interfaces = getifaddrs (&ifap);
	for (;ifap != NULL && !interface_found; ifap = ifap->ifa_next) {
		if ((ifap->ifa_flags & IFF_UP) != 0 &&
		    (ifap->ifa_flags & IFF_POINTOPOINT) == 0 &&
		    (ifap->ifa_addr->sa_family == AF_INET) &&
		    g_strcmp0 (ifap->ifa_name, interface_name) == 0) {
			GSocketAddress *socket_address;
			GSocketAddress *inet_socket_address;
			GInetAddress *inet_address;
			char *gvcp_address_string;
			char *discovery_address_string;
			GError *error = NULL;

			socket_address = g_socket_address_new_from_native (ifap->ifa_addr, sizeof (ifap->ifa_addr));
			inet_address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (socket_address));
			gvcp_address_string = g_inet_address_to_string (inet_address);
			arv_debug ("camera", "[FakeGvCamera::new] Interface address = %s", gvcp_address_string);

			inet_socket_address = g_inet_socket_address_new (inet_address, ARV_GVCP_PORT);
			gv_camera->gvcp_socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
							       G_SOCKET_TYPE_DATAGRAM,
							       G_SOCKET_PROTOCOL_UDP, NULL);
			g_socket_bind (gv_camera->gvcp_socket, inet_socket_address, FALSE, &error);
			g_socket_set_blocking (gv_camera->gvcp_socket, FALSE);
			g_assert (error == NULL);
			arv_fake_camera_set_inet_address (gv_camera->camera, inet_address);
			g_object_unref (inet_socket_address);

			inet_socket_address = g_inet_socket_address_new (inet_address, 0);
			gv_camera->gvsp_socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
							       G_SOCKET_TYPE_DATAGRAM,
							       G_SOCKET_PROTOCOL_UDP, NULL);
			g_socket_bind (gv_camera->gvsp_socket, inet_socket_address, FALSE, &error);
			g_assert (error == NULL);
			g_object_unref (inet_socket_address);

			g_object_unref (socket_address);

			socket_address = g_socket_address_new_from_native (ifap->ifa_broadaddr,
									   sizeof (ifap->ifa_broadaddr));
			inet_address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (socket_address));
			discovery_address_string = g_inet_address_to_string (inet_address);
			arv_debug ("camera", "[FakeGvCamera::new] Discovery address = %s", discovery_address_string);
			inet_socket_address = g_inet_socket_address_new (inet_address, ARV_GVCP_PORT);
			if (g_strcmp0 (gvcp_address_string, discovery_address_string) == 0)
				gv_camera->discovery_socket = NULL;
			else {
				gv_camera->discovery_socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
									    G_SOCKET_TYPE_DATAGRAM,
									    G_SOCKET_PROTOCOL_UDP, NULL);
				g_socket_bind (gv_camera->discovery_socket, inet_socket_address, FALSE, &error);
				g_socket_set_blocking (gv_camera->discovery_socket, FALSE);
				g_assert (error == NULL);
			}
			g_object_unref (inet_socket_address);
			g_object_unref (socket_address);

			g_free (gvcp_address_string);
			g_free (discovery_address_string);

			gv_camera->gvcp_fds[0].fd = g_socket_get_fd (gv_camera->gvcp_socket);
			gv_camera->gvcp_fds[0].events = G_IO_IN;
			gv_camera->gvcp_fds[0].revents = 0;
			if (gv_camera->discovery_socket != NULL) {
				gv_camera->gvcp_fds[1].fd = g_socket_get_fd (gv_camera->discovery_socket);
				gv_camera->gvcp_fds[1].events = G_IO_IN;
				gv_camera->gvcp_fds[1].revents = 0;
				gv_camera->n_gvcp_fds = 2;
			} else
				gv_camera->n_gvcp_fds = 1;

			interface_found = TRUE;
		}
	}

	if (!interface_found)
		goto INTERFACE_ERROR;

	gv_camera->cancel = FALSE;
	gv_camera->gvsp_thread = g_thread_create (arv_fake_gv_camera_thread,
						  gv_camera, TRUE, NULL);

	return gv_camera;

INTERFACE_ERROR:
	g_object_unref (gv_camera->camera);
	g_free (gv_camera);

	return NULL;
}

void
arv_fake_gv_camera_free (ArvFakeGvCamera *gv_camera)
{
	g_return_if_fail (gv_camera != NULL);

	if (gv_camera->gvsp_thread != NULL) {
		gv_camera->cancel = TRUE;
		g_thread_join (gv_camera->gvsp_thread);
		gv_camera->gvsp_thread = NULL;
	}

	g_object_unref (gv_camera->gvcp_socket);
	if (gv_camera->discovery_socket != NULL)
		g_object_unref (gv_camera->discovery_socket);
	g_object_unref (gv_camera->camera);
	g_free (gv_camera);
}

void
handle_control_packet (ArvFakeGvCamera *gv_camera, GSocket *socket,
		       GSocketAddress *remote_address,
		       ArvGvcpPacket *packet, size_t size)
{
	ArvGvcpPacket *ack_packet = NULL;
	size_t ack_packet_size;
	guint32 block_address;
	guint32 block_size;
	guint32 packet_count;
	guint32 register_address;
	guint32 register_value;

	arv_gvcp_packet_debug (packet);

	packet_count = arv_gvcp_packet_get_packet_count (packet);

	switch (g_ntohs (packet->header.command)) {
		case ARV_GVCP_COMMAND_DISCOVERY_CMD:
			ack_packet = arv_gvcp_packet_new_discovery_ack (&ack_packet_size);
			arv_debug ("camera", "[FakeGvCamera::handle_control_packet] Discovery command");
			arv_fake_camera_read_memory (gv_camera->camera, 0, ARV_GVBS_DISCOVERY_DATA_SIZE,
						     &ack_packet->data);
			break;
		case ARV_GVCP_COMMAND_READ_MEMORY_CMD:
			arv_gvcp_packet_get_read_memory_cmd_infos (packet, &block_address, &block_size);
			arv_debug ("camera", "[FakeGvCamera::handle_control_packet] Read memory command %d (%d)",
				   block_address, block_size);
			ack_packet = arv_gvcp_packet_new_read_memory_ack (block_address, block_size,
									  packet_count, &ack_packet_size);
			arv_fake_camera_read_memory (gv_camera->camera, block_address, block_size,
						     arv_gvcp_packet_get_read_memory_ack_data (ack_packet));
			break;
		case ARV_GVCP_COMMAND_WRITE_MEMORY_CMD:
			arv_gvcp_packet_get_write_memory_cmd_infos (packet, &block_address, &block_size);
			arv_debug ("camera", "[FakeGvCamera::handle_control_packet] Write memory command %d (%d)",
				   block_address, block_size);
			arv_fake_camera_write_memory (gv_camera->camera, block_address, block_size,
						      arv_gvcp_packet_get_write_memory_cmd_data (packet));
			ack_packet = arv_gvcp_packet_new_write_memory_ack (block_address, packet_count,
									   &ack_packet_size);
			break;
		case ARV_GVCP_COMMAND_READ_REGISTER_CMD:
			arv_gvcp_packet_get_read_register_cmd_infos (packet, &register_address);
			arv_fake_camera_read_register (gv_camera->camera, register_address, &register_value);
			arv_debug ("camera", "[FakeGvCamera::handle_control_packet] Read register command %d -> %d",
				   register_address, register_value);
			ack_packet = arv_gvcp_packet_new_read_register_ack (register_value, packet_count,
									    &ack_packet_size);
			break;
		case ARV_GVCP_COMMAND_WRITE_REGISTER_CMD:
			arv_gvcp_packet_get_write_register_cmd_infos (packet, &register_address, &register_value);
			arv_fake_camera_write_register (gv_camera->camera, register_address, register_value);
			arv_debug ("camera", "[FakeGvCamera::handle_control_packet] Write register command %d -> %d",
				   register_address, register_value);
			ack_packet = arv_gvcp_packet_new_write_register_ack (register_value, packet_count,
									     &ack_packet_size);
			break;
		default:
			arv_debug ("camera", "[FakeGvCamera::handle_control_packet] Unknown command");
	}

	if (ack_packet != NULL) {
		g_socket_send_to (socket, remote_address, (char *) ack_packet, ack_packet_size, NULL, NULL);
		arv_gvcp_packet_debug (ack_packet);
		g_free (ack_packet);
	}
}

static char *arv_option_interface_name = "lo";
static char *arv_option_debug_domains = NULL;

static const GOptionEntry arv_option_entries[] =
{
	{ "interface",		'i', 0, G_OPTION_ARG_STRING,
		&arv_option_interface_name,	"Listening interface name", NULL},
	{ "debug", 		'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	"Debug mode", NULL },
	{ NULL }
};

int
main (int argc, char **argv)
{
	ArvFakeGvCamera *gv_camera;
	int n_events;
	GInputVector input_vector;
	GOptionContext *context;
	GError *error = NULL;

	g_thread_init (NULL);
	g_type_init ();

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, arv_option_entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	arv_debug_enable (arv_option_debug_domains);

	gv_camera = arv_fake_gv_camera_new (arv_option_interface_name);
	g_return_val_if_fail (gv_camera != NULL, EXIT_FAILURE);

	input_vector.buffer = g_malloc0 (ARV_FAKE_GV_CAMERA_BUFFER_SIZE);
	input_vector.size = ARV_FAKE_GV_CAMERA_BUFFER_SIZE;

	signal (SIGINT, set_cancel);

	do {
		n_events = g_poll (gv_camera->gvcp_fds, 2, 1000);
		g_print ("n_events = %d\n", n_events);
		if (n_events > 0) {
			GSocketAddress *remote_address;
			int count;

			count = g_socket_receive_message (gv_camera->gvcp_socket,
							  &remote_address, &input_vector, 1, NULL, NULL,
							  G_SOCKET_MSG_NONE, NULL, NULL);
			if (count > 0)
				handle_control_packet (gv_camera, gv_camera->gvcp_socket,
						       remote_address, input_vector.buffer, count);

			if (gv_camera->discovery_socket != NULL) {
				count = g_socket_receive_message (gv_camera->discovery_socket,
								  &remote_address, &input_vector, 1, NULL, NULL,
								  G_SOCKET_MSG_NONE, NULL, NULL);
				if (count > 0)
					handle_control_packet (gv_camera, gv_camera->discovery_socket,
							       remote_address, input_vector.buffer, count);
			}
		}
	} while (!cancel);

	g_free (input_vector.buffer);

	arv_fake_gv_camera_free (gv_camera);

	return EXIT_SUCCESS;
}
