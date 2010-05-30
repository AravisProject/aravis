#include <arv.h>
#include <stdlib.h>

#define ARV_FAKE_GV_CAMERA_BUFFER_SIZE	65536

static gboolean cancel = FALSE;

static void
set_cancel (int signal)
{
	cancel = TRUE;
}

typedef struct {
	ArvFakeCamera *camera;
} ArvFakeGvCamera;

void *
arv_fake_gv_camera_thread (void *user_data)
{
	return NULL;
}

void
handle_control_packet (ArvFakeGvCamera *gv_camera, GSocket *socket,
		       GSocketAddress *remote_address,
		       ArvGvcpPacket *packet, size_t size)
{
	ArvGvcpPacket *ack_packet;
	size_t ack_packet_size;

	switch (g_ntohs (packet->header.command)) {
		case ARV_GVCP_COMMAND_DISCOVERY_CMD:
			arv_gvcp_packet_debug (packet);
			ack_packet = arv_gvcp_packet_new_discovery_ack (&ack_packet_size);
			arv_fake_camera_read_memory (gv_camera->camera, 0, ARV_GVBS_DISCOVERY_DATA_SIZE,
						     &ack_packet->data);
			g_socket_send_to (socket, remote_address, (char *) ack_packet, ack_packet_size, NULL, NULL);
			arv_gvcp_packet_debug (ack_packet);
			g_free (ack_packet);
			break;
		case ARV_GVCP_COMMAND_READ_MEMORY_CMD:
			g_message ("Read command ");
			break;
		default:
			g_message ("Unknown command");
	}
}

int
main (int argc, char **argv)
{
	ArvFakeGvCamera *gv_camera;
	GSocket *socket;
	GSocketAddress *socket_address;
	GInetAddress *address;
	GPollFD poll_fd;
	int n_events;
	GInputVector input_vector;

	g_thread_init (NULL);
	g_type_init ();

	gv_camera = g_new0 (ArvFakeGvCamera, 1);
	gv_camera->camera = arv_fake_camera_new ("Fake_1");

	socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
			       G_SOCKET_TYPE_DATAGRAM,
			       G_SOCKET_PROTOCOL_UDP, NULL);

	address = g_inet_address_new_from_string ("127.0.0.1");
	socket_address = g_inet_socket_address_new (address, ARV_GVCP_PORT);
	arv_fake_camera_set_inet_address (gv_camera->camera, address);
	g_object_unref (address);
	g_socket_bind (socket, socket_address, TRUE, NULL);

	input_vector.buffer = g_malloc0 (ARV_FAKE_GV_CAMERA_BUFFER_SIZE);
	input_vector.size = ARV_FAKE_GV_CAMERA_BUFFER_SIZE;

	poll_fd.fd = g_socket_get_fd (socket);
	poll_fd.events =  G_IO_IN;
	poll_fd.revents = 0;

	signal (SIGINT, set_cancel);

	do {
		n_events = g_poll (&poll_fd, 1, 1000);
		if (n_events > 0) {
			GSocketAddress *remote_address;
			int count;

			count = g_socket_receive_message (socket, &remote_address, &input_vector, 1, NULL, NULL,
							  G_SOCKET_MSG_NONE, NULL, NULL);
			if (count > 0)
				handle_control_packet (gv_camera, socket, remote_address, input_vector.buffer, count);
		}
	} while (!cancel);

	g_free (input_vector.buffer);
	g_object_unref (socket);
	g_object_unref (gv_camera->camera);
	g_free (gv_camera);

	return EXIT_SUCCESS;
}
