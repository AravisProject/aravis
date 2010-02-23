#include <arvgvinterface.h>
#include <arvgvdevice.h>
#include <arvgvcp.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <stdlib.h>

static GObjectClass *parent_class = NULL;

typedef struct {
	GSocketAddress *interface_address;
	GSocketAddress *broadcast_address;
	GSocket *socket;
} ArvGvInterfaceDiscoverInfos;

static void
arv_gv_interface_free_discover_infos_list (ArvGvInterface *gv_interface)
{
	GSList *iter;

	for (iter = gv_interface->discover_infos_list; iter != NULL; iter = iter->next) {
		ArvGvInterfaceDiscoverInfos *infos = iter->data;

		g_object_unref (infos->interface_address);
		g_object_unref (infos->broadcast_address);
		g_object_unref (infos->socket);
		g_free (infos);
	}

	g_slist_free (gv_interface->discover_infos_list);

	gv_interface->n_discover_infos = 0;
	gv_interface->discover_infos_list = NULL;
}

static void
arv_gv_interface_build_discover_infos_list (ArvGvInterface *gv_interface)
{
	struct ifaddrs *ifap;
	int n_interfaces;

	arv_gv_interface_free_discover_infos_list (gv_interface);

	n_interfaces = getifaddrs (&ifap);
	for (;ifap != NULL; ifap = ifap->ifa_next) {
		if ((ifap->ifa_flags & IFF_UP) != 0 &&
		    (ifap->ifa_flags & (IFF_LOOPBACK | IFF_POINTOPOINT)) == 0 &&
		    (ifap->ifa_addr->sa_family == AF_INET)) {
			ArvGvInterfaceDiscoverInfos *infos = g_new (ArvGvInterfaceDiscoverInfos, 1);
			GSocketAddress *socket_address;
			GInetAddress *inet_address;
			GError *error = NULL;

			socket_address = g_socket_address_new_from_native (ifap->ifa_addr, sizeof (ifap->ifa_addr));
			inet_address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (socket_address));
			infos->interface_address = g_inet_socket_address_new (inet_address, 0);
			g_object_unref (socket_address);

			socket_address = g_socket_address_new_from_native (ifap->ifa_broadaddr,
									   sizeof (ifap->ifa_broadaddr));
			inet_address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (socket_address));
			infos->broadcast_address = g_inet_socket_address_new (inet_address, ARV_GVCP_PORT);
			g_object_unref (socket_address);

			infos->socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
						      G_SOCKET_TYPE_DATAGRAM,
						      G_SOCKET_PROTOCOL_UDP, NULL);
			g_socket_bind (infos->socket, infos->interface_address, TRUE, &error);

			gv_interface->discover_infos_list = g_slist_prepend (gv_interface->discover_infos_list,
									     infos);
			gv_interface->n_discover_infos++;
		}
	}
}

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
	GSList *iter;
	size_t size;

	arv_gv_interface_build_discover_infos_list (gv_interface);

	packet = arv_gvcp_discover_packet_new (&size);

	for (iter = gv_interface->discover_infos_list; iter != NULL; iter = iter->next) {
		ArvGvInterfaceDiscoverInfos *infos = iter->data;
		GError *error = NULL;

		arv_gv_interface_socket_set_broadcast (infos->socket, TRUE);
		g_socket_send_to (infos->socket,
				  infos->broadcast_address,
				  (const char *) packet, size,
				  NULL, &error);
		if (error != NULL)
			g_message ("error: %s", error->message);
		arv_gv_interface_socket_set_broadcast (infos->socket, FALSE);
	}

	arv_gvcp_packet_free (packet);
}

static void
arv_gv_interface_receive_hello_packet (ArvGvInterface *gv_interface)
{
	GPollFD *poll_fd;
	GSList *iter;
	char buffer[ARV_GV_INTERFACE_SOCKET_BUFFER_SIZE];
	int count;
	int i;

	if (gv_interface->n_discover_infos ==0)
		return;

	poll_fd = g_new (GPollFD, gv_interface->n_discover_infos);

	for (i = 0, iter = gv_interface->discover_infos_list; iter != NULL; i++, iter = iter->next) {
		ArvGvInterfaceDiscoverInfos *infos = iter->data;

		poll_fd[i].fd = g_socket_get_fd (infos->socket);
		poll_fd[i].events =  G_IO_IN;
		poll_fd[i].revents = 0;
	}

	do {
		if (g_poll (poll_fd, gv_interface->n_discover_infos, ARV_GV_INTERFACE_DISCOVER_TIMEOUT_MS) == 0) {
			g_free (poll_fd);
			return;
		}

		for (i = 0, iter = gv_interface->discover_infos_list; iter != NULL; i++, iter = iter->next) {
			ArvGvInterfaceDiscoverInfos *infos = iter->data;

			g_socket_set_blocking (infos->socket, FALSE);
			count = g_socket_receive (infos->socket, buffer, ARV_GV_INTERFACE_SOCKET_BUFFER_SIZE,
						  NULL, NULL);
			g_socket_set_blocking (infos->socket, TRUE);

			if (count != 0) {
				ArvGvcpPacket *packet = (ArvGvcpPacket *) buffer;

				if (g_ntohs (packet->header.command) == ARV_GVCP_COMMAND_DISCOVER_ANS &&
				    g_ntohs (packet->header.count) == 0xffff) {
					ArvDevice *device;
					char *data = buffer + sizeof (ArvGvcpHeader);
					char *address;

					arv_gvcp_packet_dump (packet);

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
	gv_interface->n_discover_infos = 0;
	gv_interface->discover_infos_list = NULL;

	gv_interface->devices = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}

static void
arv_gv_interface_finalize (GObject *object)
{
	ArvGvInterface *gv_interface = ARV_GV_INTERFACE (object);

	g_hash_table_unref (gv_interface->devices);
	gv_interface->devices = NULL;

	arv_gv_interface_free_discover_infos_list (gv_interface);

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
