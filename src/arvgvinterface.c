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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvgvinterface
 * @short_description: Gigabit ethernet camera interface
 */

#include <arvgvinterface.h>
#include <arvgvdevice.h>
#include <arvgvcp.h>
#include <arvdebug.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

typedef struct {
	GInetAddress *interface_address;
	guchar discovery_data[ARV_GVBS_DISCOVERY_DATA_SIZE];
} ArvGvInterfaceDeviceInfos;

static ArvGvInterfaceDeviceInfos *
arv_gv_interface_device_infos_new (GInetAddress *interface_address,
				   void *discovery_data)
{
	ArvGvInterfaceDeviceInfos *infos;

	g_return_val_if_fail (G_IS_INET_ADDRESS (interface_address), NULL);
	g_return_val_if_fail (discovery_data != NULL, NULL);

	g_object_ref (interface_address);

	infos = g_new (ArvGvInterfaceDeviceInfos, 1);
	infos->interface_address = interface_address;
	memcpy (infos->discovery_data, discovery_data, ARV_GVBS_DISCOVERY_DATA_SIZE);

	return infos;
}

static void
arv_gv_interface_device_infos_free (ArvGvInterfaceDeviceInfos *infos)
{
	g_return_if_fail (infos != NULL);
	g_object_unref (infos->interface_address);
	g_free (infos);
}

struct _ArvGvInterfacePrivate {
	unsigned int n_discover_infos;
	GSList *discover_infos_list;

	GHashTable *devices;
};

typedef struct {
	GSocketAddress *interface_address;
	GSocketAddress *broadcast_address;
	GSocket *socket;
} ArvGvInterfaceDiscoverInfos;

static void
arv_gv_interface_free_discover_infos_list (ArvGvInterface *gv_interface)
{
	GSList *iter;

	for (iter = gv_interface->priv->discover_infos_list; iter != NULL; iter = iter->next) {
		ArvGvInterfaceDiscoverInfos *infos = iter->data;

		g_object_unref (infos->interface_address);
		g_object_unref (infos->broadcast_address);
		g_object_unref (infos->socket);
		g_free (infos);
	}

	g_slist_free (gv_interface->priv->discover_infos_list);

	gv_interface->priv->n_discover_infos = 0;
	gv_interface->priv->discover_infos_list = NULL;
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
		    (ifap->ifa_flags & IFF_POINTOPOINT) == 0 &&
		    (ifap->ifa_addr->sa_family == AF_INET)) {
			ArvGvInterfaceDiscoverInfos *infos = g_new (ArvGvInterfaceDiscoverInfos, 1);
			GSocketAddress *socket_address;
			GInetAddress *inet_address;
			char *inet_address_string;
			GError *error = NULL;

			socket_address = g_socket_address_new_from_native (ifap->ifa_addr, sizeof (struct sockaddr));
			inet_address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (socket_address));
			inet_address_string = g_inet_address_to_string (inet_address);
			arv_debug ("interface", "[GvInterface::build_discover_infos_list] Add interface %s",
				   inet_address_string);
			g_free (inet_address_string);
			infos->interface_address = g_inet_socket_address_new (inet_address, 0);
			g_object_unref (socket_address);

			socket_address = g_socket_address_new_from_native (ifap->ifa_broadaddr,
									   sizeof (struct sockaddr));
			inet_address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (socket_address));
			infos->broadcast_address = g_inet_socket_address_new (inet_address, ARV_GVCP_PORT);
			inet_address_string = g_inet_address_to_string (inet_address);
			arv_debug ("interface", "[GvInterface::build_discover_infos_list] Broadcast address is %s",
				   inet_address_string);
			g_free (inet_address_string);
			g_object_unref (socket_address);

			infos->socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
						      G_SOCKET_TYPE_DATAGRAM,
						      G_SOCKET_PROTOCOL_UDP, NULL);
			g_socket_bind (infos->socket, infos->interface_address, TRUE, &error);

			gv_interface->priv->discover_infos_list =
				g_slist_prepend (gv_interface->priv->discover_infos_list,
						 infos);
			gv_interface->priv->n_discover_infos++;
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

	packet = arv_gvcp_packet_new_discovery_cmd (&size);

	for (iter = gv_interface->priv->discover_infos_list; iter != NULL; iter = iter->next) {
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

	if (gv_interface->priv->n_discover_infos ==0)
		return;

	poll_fd = g_new (GPollFD, gv_interface->priv->n_discover_infos);

	for (i = 0, iter = gv_interface->priv->discover_infos_list; iter != NULL; i++, iter = iter->next) {
		ArvGvInterfaceDiscoverInfos *infos = iter->data;

		poll_fd[i].fd = g_socket_get_fd (infos->socket);
		poll_fd[i].events =  G_IO_IN;
		poll_fd[i].revents = 0;
	}

	do {
		if (g_poll (poll_fd, gv_interface->priv->n_discover_infos,
			    ARV_GV_INTERFACE_DISCOVERY_TIMEOUT_MS) == 0) {
			g_free (poll_fd);
			return;
		}

		for (i = 0, iter = gv_interface->priv->discover_infos_list; iter != NULL; i++, iter = iter->next) {
			ArvGvInterfaceDiscoverInfos *infos = iter->data;

			do {
				g_socket_set_blocking (infos->socket, FALSE);
				count = g_socket_receive (infos->socket, buffer, ARV_GV_INTERFACE_SOCKET_BUFFER_SIZE,
							  NULL, NULL);
				g_socket_set_blocking (infos->socket, TRUE);

				if (count > 0) {
					ArvGvcpPacket *packet = (ArvGvcpPacket *) buffer;

					if (g_ntohs (packet->header.command) == ARV_GVCP_COMMAND_DISCOVERY_ACK &&
					    g_ntohs (packet->header.count) == 0xffff) {
						ArvGvInterfaceDeviceInfos *device_infos;
						GInetAddress *interface_address;
						char *address_string;
						char *data = buffer + sizeof (ArvGvcpHeader);
						char *serial_number;
						char *manufacturer;
						char *key;

						arv_gvcp_packet_debug (packet);

						manufacturer = g_strndup (&data[ARV_GVBS_MANUFACTURER_NAME],
									  ARV_GVBS_MANUFACTURER_NAME_SIZE);
						serial_number = g_strndup (&data[ARV_GVBS_SERIAL_NUMBER],
									   ARV_GVBS_SERIAL_NUMBER_SIZE);
						key = g_strdup_printf ("%s-%s", manufacturer, serial_number);
						g_free (manufacturer);
						g_free (serial_number);

						interface_address = g_inet_socket_address_get_address
							(G_INET_SOCKET_ADDRESS (infos->interface_address));
						device_infos = arv_gv_interface_device_infos_new (interface_address,
												  data);
						address_string = g_inet_address_to_string (interface_address);

						arv_debug ("interface",
							   "[GvInterface::discovery] Device '%s' found (interface %s)",
							   key, address_string);

						g_free (address_string);

						g_hash_table_insert (gv_interface->priv->devices,
								     key, device_infos);
					}
				}
			} while (count > 0);
		}
	} while (1);
}

static void
arv_gv_interface_update_device_list (ArvInterface *interface, GArray *device_ids)
{
	ArvGvInterface *gv_interface;
	GHashTableIter iter;
	gpointer key, value;

	gv_interface = ARV_GV_INTERFACE (interface);

	arv_gv_interface_send_discover_packet (gv_interface);
	arv_gv_interface_receive_hello_packet (gv_interface);

	g_array_set_size (device_ids, 0);

	g_hash_table_iter_init (&iter, gv_interface->priv->devices);
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		char *device_id = g_strdup (key);
		g_array_append_val (device_ids, device_id);
	}
}

static ArvDevice *
arv_gv_interface_open_device (ArvInterface *interface, const char *device_id)
{
	ArvGvInterface *gv_interface;
	ArvDevice *device = NULL;
	ArvGvInterfaceDeviceInfos *device_infos;
	GInetAddress *device_address;

	gv_interface = ARV_GV_INTERFACE (interface);

	if (device_id == NULL) {
		GList *device_list;

		device_list = g_hash_table_get_values (gv_interface->priv->devices);
		device_infos = device_list != NULL ? device_list->data : NULL;
		g_list_free (device_list);
	} else
		device_infos = g_hash_table_lookup (gv_interface->priv->devices, device_id);

	if (device_infos == NULL)
		return NULL;

	device_address = g_inet_address_new_from_bytes (&device_infos->discovery_data[ARV_GVBS_CURRENT_IP_ADDRESS],
							G_SOCKET_FAMILY_IPV4);
	device = arv_gv_device_new (device_infos->interface_address, device_address);
	g_object_unref (device_address);

	return device;
}

static ArvInterface *gv_interface = NULL;
static GStaticMutex gv_interface_mutex = G_STATIC_MUTEX_INIT;

ArvInterface *
arv_gv_interface_get_instance (void)
{
	g_static_mutex_lock (&gv_interface_mutex);

	if (gv_interface == NULL)
		gv_interface = g_object_new (ARV_TYPE_GV_INTERFACE, NULL);

	g_static_mutex_unlock (&gv_interface_mutex);

	return ARV_INTERFACE (gv_interface);
}

void
arv_gv_interface_destroy_instance (void)
{
	g_static_mutex_lock (&gv_interface_mutex);

	if (gv_interface != NULL) {
		g_object_unref (gv_interface);
		gv_interface = NULL;
	}

	g_static_mutex_unlock (&gv_interface_mutex);
}

static void
arv_gv_interface_init (ArvGvInterface *gv_interface)
{
	gv_interface->priv = G_TYPE_INSTANCE_GET_PRIVATE (gv_interface, ARV_TYPE_GV_INTERFACE, ArvGvInterfacePrivate);

	gv_interface->priv->n_discover_infos = 0;
	gv_interface->priv->discover_infos_list = NULL;

	gv_interface->priv->devices = g_hash_table_new_full (g_str_hash, g_str_equal,
							     g_free,
							     (GDestroyNotify) arv_gv_interface_device_infos_free);
}

static void
arv_gv_interface_finalize (GObject *object)
{
	ArvGvInterface *gv_interface = ARV_GV_INTERFACE (object);

	g_hash_table_unref (gv_interface->priv->devices);
	gv_interface->priv->devices = NULL;

	arv_gv_interface_free_discover_infos_list (gv_interface);

	parent_class->finalize (object);
}

static void
arv_gv_interface_class_init (ArvGvInterfaceClass *gv_interface_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (gv_interface_class);
	ArvInterfaceClass *interface_class = ARV_INTERFACE_CLASS (gv_interface_class);

	g_type_class_add_private (gv_interface_class, sizeof (ArvGvInterfacePrivate));

	parent_class = g_type_class_peek_parent (gv_interface_class);

	object_class->finalize = arv_gv_interface_finalize;

	interface_class->update_device_list = arv_gv_interface_update_device_list;
	interface_class->open_device = arv_gv_interface_open_device;
}

G_DEFINE_TYPE (ArvGvInterface, arv_gv_interface, ARV_TYPE_INTERFACE)
