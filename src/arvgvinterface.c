#include <arvgvinterface.h>
#include <arvgvdevice.h>
#include <arvgvcp.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>

static GObjectClass *parent_class = NULL;

static gboolean
arv_gv_interface_socket_set_broadcast (GSocket *socket, gboolean enable)
{
	int socket_fd;
	int result;

	socket_fd = g_socket_get_fd (socket);

	result = setsockopt (socket_fd, SOL_SOCKET, SO_BROADCAST, (char*)&enable, sizeof (enable));

	return result == 0;
}

static void
arv_gv_interface_send_discover_packet (ArvGvInterface *gv_interface)
{
	ArvGvcpPacket *packet;
	size_t size;

	packet = arv_gvcp_discover_packet_new (&size);

	arv_gv_interface_socket_set_broadcast (gv_interface->socket, TRUE);
	g_socket_send_to (gv_interface->socket,
			  gv_interface->broadcast_address,
			  (const char *) packet, size,
			  NULL, NULL);
	arv_gv_interface_socket_set_broadcast (gv_interface->socket, FALSE);

	arv_gvcp_packet_free (packet);
}

static void
arv_gv_interface_receive_hello_packet (ArvGvInterface *gv_interface)
{
	char buffer[1024];
	GPollFD poll_fd;
	int count;

	poll_fd.fd = g_socket_get_fd (gv_interface->socket);
	poll_fd.events =  G_IO_IN;
	poll_fd.revents = 0;

	do {
		if (g_poll (&poll_fd, 1, 1000) == 0)
			return;

		g_socket_set_blocking (gv_interface->socket, FALSE);
		count = g_socket_receive (gv_interface->socket, buffer, 1024, NULL, NULL);
		g_socket_set_blocking (gv_interface->socket, TRUE);

		if (count != 0) {
			ArvGvcpPacket *packet = (ArvGvcpPacket *) buffer;
			char *packet_string;

			packet_string = arv_gvcp_packet_to_string (packet);
			g_message ("%s", packet_string);
			g_free (packet_string);

			if (g_ntohs (packet->header.command) == ARV_GVCP_COMMAND_DISCOVER_ANS &&
			    g_ntohs (packet->header.count) == 0xffff) {
				ArvDevice *device;
				char *data = buffer + sizeof (ArvGvcpHeader);
				char *address;

				address = g_strdup_printf ("%d.%d.%d.%d",
							   data[ARV_GVCP_IP_ADDRESS] & 0xff,
							   data[ARV_GVCP_IP_ADDRESS + 1] & 0xff,
							   data[ARV_GVCP_IP_ADDRESS + 2] & 0xff,
							   data[ARV_GVCP_IP_ADDRESS + 3] & 0xff);

				device = g_hash_table_lookup (gv_interface->devices, address);

				if (device != NULL)
					g_free (address);
				else {
					GInetAddress *inet_address;

					inet_address = g_inet_address_new_from_string (address);
					device = arv_gv_device_new (inet_address);
					g_hash_table_insert (gv_interface->devices, address, device);
					g_object_unref (inet_address);
				}
			}
		}
	} while (1);
}

static void
arv_gv_interface_update_device_list (ArvInterface *interface)
{
	arv_gv_interface_send_discover_packet (ARV_GV_INTERFACE (interface));
	arv_gv_interface_receive_hello_packet (ARV_GV_INTERFACE (interface));
}

static ArvDevice *
arv_gv_interface_get_device (ArvInterface *interface, int property, const char *value)
{
	ArvGvInterface *gv_interface;
	ArvDevice *device;
	GList *devices;

	gv_interface = ARV_GV_INTERFACE (interface);

	devices = g_hash_table_get_values (gv_interface->devices);

	device = devices != NULL ? devices->data : NULL;

	g_list_free (devices);

	if (device != NULL)
		g_object_ref (device);

	return device;
}

ArvInterface *
arv_gv_interface_get_instance (void)
{
	static ArvInterface *gv_interface = NULL;
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

	g_static_mutex_lock (&mutex);

	if (gv_interface == NULL)
		gv_interface = g_object_new (ARV_TYPE_GV_INTERFACE, NULL);
	else
		g_object_ref (gv_interface);

	g_static_mutex_unlock (&mutex);

	return ARV_INTERFACE (gv_interface);
}

static void
arv_gv_interface_init (ArvGvInterface *gv_interface)
{
	GInetAddress *inet_address;

	gv_interface->socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
					     G_SOCKET_TYPE_DATAGRAM,
					     G_SOCKET_PROTOCOL_UDP, NULL);

	inet_address = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
	gv_interface->control_address = g_inet_socket_address_new (inet_address, 0);
	g_object_unref (inet_address);

	inet_address = g_inet_address_new_from_string ("255.255.255.255");
	gv_interface->broadcast_address = g_inet_socket_address_new (inet_address, ARV_GVCP_PORT);
	g_object_unref (inet_address);

	g_socket_bind (gv_interface->socket, gv_interface->control_address, TRUE, NULL);

	gv_interface->devices = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}

static void
arv_gv_interface_finalize (GObject *object)
{
	ArvGvInterface *gv_interface = ARV_GV_INTERFACE (object);

	g_hash_table_unref (gv_interface->devices);
	gv_interface->devices = NULL;

	g_object_unref (gv_interface->socket);
	g_object_unref (gv_interface->broadcast_address);
	g_object_unref (gv_interface->control_address);

	parent_class->finalize (object);
}

static void
arv_gv_interface_class_init (ArvGvInterfaceClass *gv_interface_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (gv_interface_class);
	ArvInterfaceClass *interface_class = ARV_INTERFACE_CLASS (gv_interface_class);

	parent_class = g_type_class_peek_parent (gv_interface_class);

	object_class->finalize = arv_gv_interface_finalize;

	interface_class->update_device_list = arv_gv_interface_update_device_list;
	interface_class->get_device = arv_gv_interface_get_device;
}

G_DEFINE_TYPE (ArvGvInterface, arv_gv_interface, ARV_TYPE_INTERFACE)
