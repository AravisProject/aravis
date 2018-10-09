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
 * SECTION: arvgvinterface
 * @short_description: GigEVision interface
 */

#include <arpa/inet.h>
#include <arvgvinterfaceprivate.h>
#include <arvinterfaceprivate.h>
#include <arvgvdeviceprivate.h>
#include <arvgvcp.h>
#include <arvdebug.h>
#include <arvmisc.h>
#include <arvstr.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <string.h>

/* ArvGvDiscoverSocket implementation */

typedef struct {
	GSocketAddress *interface_address;
	GSocket *socket;
} ArvGvDiscoverSocket;

static gboolean
arv_gv_discover_socket_set_broadcast (ArvGvDiscoverSocket *discover_socket, gboolean enable)
{
	int socket_fd;
	int result;

	socket_fd = g_socket_get_fd (discover_socket->socket);

	result = setsockopt (socket_fd, SOL_SOCKET, SO_BROADCAST, (char*)&enable, sizeof (enable));

	return result == 0;
}

typedef struct {
	unsigned int n_sockets;
	GSList *sockets;
	GPollFD *poll_fds;
} ArvGvDiscoverSocketList;

static ArvGvDiscoverSocketList *
arv_gv_discover_socket_list_new (void)
{
	ArvGvDiscoverSocketList *socket_list;
	GSList *iter;
	struct ifaddrs *ifap  = NULL;
	struct ifaddrs *ifap_iter;
	int i;

	socket_list = g_new0 (ArvGvDiscoverSocketList, 1);

	if (getifaddrs (&ifap) < 0)
		return socket_list;

	for (ifap_iter = ifap; ifap_iter != NULL; ifap_iter = ifap_iter->ifa_next) {
		if ((ifap_iter->ifa_flags & IFF_UP) != 0 &&
		    (ifap_iter->ifa_flags & IFF_POINTOPOINT) == 0 &&
		    (ifap_iter->ifa_addr != NULL) &&
		    (ifap_iter->ifa_addr->sa_family == AF_INET)) {
			ArvGvDiscoverSocket *discover_socket = g_new0 (ArvGvDiscoverSocket, 1);
			GSocketAddress *socket_address;
			GInetAddress *inet_address;
			char *inet_address_string;
			GError *error = NULL;

			socket_address = g_socket_address_new_from_native (ifap_iter->ifa_addr,
									   sizeof (struct sockaddr));
			inet_address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (socket_address));
			inet_address_string = g_inet_address_to_string (inet_address);
			arv_debug_interface ("[GvDiscoverSocket::new] Add interface %s", inet_address_string);
			g_free (inet_address_string);
			discover_socket->interface_address = g_inet_socket_address_new (inet_address, 0);
			g_object_unref (socket_address);

			discover_socket->socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
								G_SOCKET_TYPE_DATAGRAM,
								G_SOCKET_PROTOCOL_UDP, NULL);
			g_socket_bind (discover_socket->socket, discover_socket->interface_address, FALSE, &error);

			socket_list->sockets = g_slist_prepend (socket_list->sockets, discover_socket);
			socket_list->n_sockets++;
		}
	}

	freeifaddrs (ifap);

	socket_list->poll_fds = g_new (GPollFD, socket_list->n_sockets);
	for (i = 0, iter = socket_list->sockets; iter != NULL; i++, iter = iter->next) {
		ArvGvDiscoverSocket *discover_socket = iter->data;

		socket_list->poll_fds[i].fd = g_socket_get_fd (discover_socket->socket);
		socket_list->poll_fds[i].events =  G_IO_IN;
		socket_list->poll_fds[i].revents = 0;
	}

	return socket_list;
}

static void
arv_gv_discover_socket_list_free (ArvGvDiscoverSocketList *socket_list)
{
	GSList *iter;

	g_return_if_fail (socket_list != NULL);

	for (iter = socket_list->sockets; iter != NULL; iter = iter->next) {
		ArvGvDiscoverSocket *discover_socket = iter->data;

		g_object_unref (discover_socket->interface_address);
		g_object_unref (discover_socket->socket);
		g_free (discover_socket);
	}
	g_slist_free (socket_list->sockets);
	g_free (socket_list->poll_fds);

	socket_list->sockets = NULL;
	socket_list->n_sockets = 0;
	socket_list->poll_fds = NULL;

	g_free (socket_list);
}

static void
arv_gv_discover_socket_list_send_discover_packet (ArvGvDiscoverSocketList *socket_list)
{
	GInetAddress *broadcast_address;
	GSocketAddress *broadcast_socket_address;
	ArvGvcpPacket *packet;
	GSList *iter;
	size_t size;

	packet = arv_gvcp_packet_new_discovery_cmd (&size);

	broadcast_address = g_inet_address_new_from_string ("255.255.255.255");
	broadcast_socket_address = g_inet_socket_address_new (broadcast_address, ARV_GVCP_PORT);
	g_object_unref (broadcast_address);

	for (iter = socket_list->sockets; iter != NULL; iter = iter->next) {
		ArvGvDiscoverSocket *discover_socket = iter->data;
		GError *error = NULL;

		arv_gv_discover_socket_set_broadcast (discover_socket, TRUE);
		g_socket_send_to (discover_socket->socket,
				  broadcast_socket_address,
				  (const char *) packet, size,
				  NULL, &error);
		if (error != NULL) {
			arv_warning_interface ("[ArvGVInterface::send_discover_packet] Error: %s", error->message);
			g_error_free (error);
		}
		arv_gv_discover_socket_set_broadcast (discover_socket, FALSE);
	}

	g_object_unref (broadcast_socket_address);

	arv_gvcp_packet_free (packet);
}

/* ArvGvInterfaceDeviceInfos implementation */

typedef struct {
	char *id;
	char *user_id;
	char *vendor_serial;
	char *vendor_alias_serial;
	char *vendor;
	char *model;
	char *serial;
	char *mac;

	GInetAddress *interface_address;

	guchar discovery_data[ARV_GVBS_DISCOVERY_DATA_SIZE];

	volatile gint ref_count;
} ArvGvInterfaceDeviceInfos;

static ArvGvInterfaceDeviceInfos *
arv_gv_interface_device_infos_new (GInetAddress *interface_address,
				   void *discovery_data)
{
	ArvGvInterfaceDeviceInfos *infos;

	g_return_val_if_fail (G_IS_INET_ADDRESS (interface_address), NULL);
	g_return_val_if_fail (discovery_data != NULL, NULL);

	g_object_ref (interface_address);

	infos = g_new0 (ArvGvInterfaceDeviceInfos, 1);

	memcpy (infos->discovery_data, discovery_data, ARV_GVBS_DISCOVERY_DATA_SIZE);

	infos->vendor = g_strndup ((char *) &infos->discovery_data[ARV_GVBS_MANUFACTURER_NAME_OFFSET],
				   ARV_GVBS_MANUFACTURER_NAME_SIZE);
	infos->model = g_strndup ((char *) &infos->discovery_data[ARV_GVBS_MODEL_NAME_OFFSET],
				  ARV_GVBS_MODEL_NAME_SIZE);
	infos->serial = g_strndup ((char *) &infos->discovery_data[ARV_GVBS_SERIAL_NUMBER_OFFSET],
				   ARV_GVBS_SERIAL_NUMBER_SIZE);
	infos->user_id = g_strndup ((char *) &infos->discovery_data[ARV_GVBS_USER_DEFINED_NAME_OFFSET],
				    ARV_GVBS_USER_DEFINED_NAME_SIZE);

	infos->id = g_strdup_printf ("%s-%s-%s", infos->vendor, infos->model, infos->serial);
	arv_str_strip (infos->id, ARV_DEVICE_NAME_ILLEGAL_CHARACTERS, ARV_DEVICE_NAME_REPLACEMENT_CHARACTER);

	infos->vendor_alias_serial = g_strdup_printf ("%s-%s", arv_vendor_alias_lookup (infos->vendor), infos->serial);
	arv_str_strip (infos->vendor_alias_serial, ARV_DEVICE_NAME_ILLEGAL_CHARACTERS, ARV_DEVICE_NAME_REPLACEMENT_CHARACTER);

	infos->vendor_serial = g_strdup_printf ("%s-%s", infos->vendor, infos->serial);
	arv_str_strip (infos->vendor_serial, ARV_DEVICE_NAME_ILLEGAL_CHARACTERS, ARV_DEVICE_NAME_REPLACEMENT_CHARACTER);

	infos->interface_address = interface_address;

	infos->mac = g_strdup_printf ("%02x:%02x:%02x:%02x:%02x:%02x",
				      infos->discovery_data[ARV_GVBS_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 2],
				      infos->discovery_data[ARV_GVBS_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 3],
				      infos->discovery_data[ARV_GVBS_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 4],
				      infos->discovery_data[ARV_GVBS_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 5],
				      infos->discovery_data[ARV_GVBS_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 6],
				      infos->discovery_data[ARV_GVBS_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 7]);
	infos->ref_count = 1;

	return infos;
}

static ArvGvInterfaceDeviceInfos *
arv_gv_interface_device_infos_ref (ArvGvInterfaceDeviceInfos *infos)
{
	g_return_val_if_fail (infos != NULL, NULL);
	g_return_val_if_fail (g_atomic_int_get (&infos->ref_count) > 0, NULL);

	g_atomic_int_inc (&infos->ref_count);

	return infos;
}

static void
arv_gv_interface_device_infos_unref (ArvGvInterfaceDeviceInfos *infos)
{
	g_return_if_fail (infos != NULL);
	g_return_if_fail (g_atomic_int_get (&infos->ref_count) > 0);

	if (g_atomic_int_dec_and_test (&infos->ref_count)) {
		g_object_unref (infos->interface_address);
		g_free (infos->id);
		g_free (infos->user_id);
		g_free (infos->vendor_serial);
		g_free (infos->vendor_alias_serial);
		g_free (infos->vendor);
		g_free (infos->model);
		g_free (infos->serial);
		g_free (infos->mac);
		g_free (infos);
	}
}

/* ArvGvInterface implementation */

static GObjectClass *parent_class = NULL;

struct _ArvGvInterfacePrivate {
	GHashTable *devices;
};

static ArvGvInterfaceDeviceInfos *
_discover (GHashTable *devices, const char *device_id)
{
	ArvGvDiscoverSocketList *socket_list;
	GSList *iter;
	char buffer[ARV_GV_INTERFACE_SOCKET_BUFFER_SIZE];
	int count;
	int i;

	g_assert (devices == NULL || device_id == NULL);

	if (devices != NULL)
		g_hash_table_remove_all (devices);

	socket_list = arv_gv_discover_socket_list_new ();

	if (socket_list->n_sockets < 1) {
		arv_gv_discover_socket_list_free (socket_list);
		return NULL;
	}

	arv_gv_discover_socket_list_send_discover_packet (socket_list);

	do {
		if (g_poll (socket_list->poll_fds, socket_list->n_sockets, ARV_GV_INTERFACE_DISCOVERY_TIMEOUT_MS) == 0) {
			arv_gv_discover_socket_list_free (socket_list);
			return NULL;
		}

		for (i = 0, iter = socket_list->sockets; iter != NULL; i++, iter = iter->next) {
			ArvGvDiscoverSocket *discover_socket = iter->data;

			do {
				g_socket_set_blocking (discover_socket->socket, FALSE);
				count = g_socket_receive (discover_socket->socket, buffer, ARV_GV_INTERFACE_SOCKET_BUFFER_SIZE,
							  NULL, NULL);
				g_socket_set_blocking (discover_socket->socket, TRUE);

				if (count > 0) {
					ArvGvcpPacket *packet = (ArvGvcpPacket *) buffer;

					if (g_ntohs (packet->header.command) == ARV_GVCP_COMMAND_DISCOVERY_ACK &&
					    g_ntohs (packet->header.id) == 0xffff) {
						ArvGvInterfaceDeviceInfos *device_infos;
						GInetAddress *interface_address;
						char *address_string;
						char *data = buffer + sizeof (ArvGvcpHeader);

						arv_gvcp_packet_debug (packet, ARV_DEBUG_LEVEL_LOG);

						interface_address = g_inet_socket_address_get_address
							(G_INET_SOCKET_ADDRESS (discover_socket->interface_address));
						device_infos = arv_gv_interface_device_infos_new (interface_address,
												  data);
						address_string = g_inet_address_to_string (interface_address);

						arv_debug_interface ("[GvInterface::discovery] Device '%s' found "
								     "(interface %s) user_id '%s' - MAC '%s'",
								     device_infos->id,
								     address_string,
								     device_infos->user_id,
								     device_infos->mac);

						g_free (address_string);

						if (devices != NULL) {
							if (device_infos->id != NULL && device_infos->id[0] != '\0')
								g_hash_table_replace (devices, device_infos->id,
										      arv_gv_interface_device_infos_ref (device_infos));
							if (device_infos->user_id != NULL && device_infos->user_id[0] != '\0')
								g_hash_table_replace (devices, device_infos->user_id,
										      arv_gv_interface_device_infos_ref (device_infos));
							if (device_infos->vendor_serial != NULL && device_infos->vendor_serial[0] != '\0')
								g_hash_table_replace (devices, device_infos->vendor_serial,
										      arv_gv_interface_device_infos_ref (device_infos));
							if (device_infos->vendor_alias_serial != NULL && device_infos->vendor_alias_serial[0] != '\0')
								g_hash_table_replace (devices, device_infos->vendor_alias_serial,
										      arv_gv_interface_device_infos_ref (device_infos));
							g_hash_table_replace (devices, device_infos->mac,
									      arv_gv_interface_device_infos_ref (device_infos));
						} else {
							if (device_id == NULL ||
							    g_strcmp0 (device_infos->id, device_id) == 0 ||
							    g_strcmp0 (device_infos->user_id, device_id) == 0 ||
							    g_strcmp0 (device_infos->vendor_serial, device_id) == 0 ||
							    g_strcmp0 (device_infos->vendor_alias_serial, device_id) == 0 ||
							    g_strcmp0 (device_infos->mac, device_id) == 0) {
								arv_gv_discover_socket_list_free (socket_list);

								return device_infos;
							}
						}

						arv_gv_interface_device_infos_unref (device_infos);
					}
				}
			} while (count > 0);
		}
	} while (1);
}

static void
arv_gv_interface_discover (ArvGvInterface *gv_interface)
{
	_discover (gv_interface->priv->devices, NULL);
}

static GInetAddress *
_device_infos_to_ginetaddress (ArvGvInterfaceDeviceInfos *device_infos)
{
	GInetAddress *device_address;

	device_address = g_inet_address_new_from_bytes
		(&device_infos->discovery_data[ARV_GVBS_CURRENT_IP_ADDRESS_OFFSET],
		 G_SOCKET_FAMILY_IPV4);

	return device_address;
}

static void
arv_gv_interface_update_device_list (ArvInterface *interface, GArray *device_ids)
{
	ArvGvInterface *gv_interface;
	GHashTableIter iter;
	gpointer key, value;

	g_assert (device_ids->len == 0);

	gv_interface = ARV_GV_INTERFACE (interface);

	arv_gv_interface_discover (gv_interface);

	g_hash_table_iter_init (&iter, gv_interface->priv->devices);
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		ArvGvInterfaceDeviceInfos *infos = value;

		if (g_strcmp0 (key, infos->id) == 0) {
			ArvInterfaceDeviceIds *ids;
			GInetAddress *device_address;

			device_address = _device_infos_to_ginetaddress (infos);

			ids = g_new0 (ArvInterfaceDeviceIds, 1);
			ids->device = g_strdup (key);
			ids->physical = g_strdup (infos->mac);
			ids->address = g_inet_address_to_string (device_address);
			ids->vendor = g_strdup (infos->vendor);
			ids->model = g_strdup (infos->model);
			ids->serial_nbr = g_strdup (infos->serial);

			g_array_append_val (device_ids, ids);

			g_object_unref (device_address);
		}
	}
}

static GInetAddress *
arv_gv_interface_camera_locate (ArvGvInterface *gv_interface, GInetAddress *device_address)
{
	ArvGvDiscoverSocketList *socket_list;
	ArvGvcpPacket *packet;
	char buffer[ARV_GV_INTERFACE_SOCKET_BUFFER_SIZE];
	GSList *iter;
	GSocketAddress *device_socket_address;
	size_t size;
	int i, count;

	socket_list = arv_gv_discover_socket_list_new ();

	if (socket_list->n_sockets < 1) {
		arv_gv_discover_socket_list_free (socket_list);
		return NULL;
	}

	/* Just read a random register from the camera socket */
	packet = arv_gvcp_packet_new_read_register_cmd (ARV_GVBS_N_STREAM_CHANNELS_OFFSET, 0, &size);
	device_socket_address = g_inet_socket_address_new (device_address, ARV_GVCP_PORT);

	for (iter = socket_list->sockets; iter != NULL; iter = iter->next) {
		ArvGvDiscoverSocket *socket = iter->data;
		GError *error = NULL;

		g_socket_send_to (socket->socket,
				device_socket_address,
				(const char *) packet, size,
				NULL, &error);
		if (error != NULL) {
			arv_warning_interface ("[ArvGVInterface::arv_gv_interface_camera_locate] Error: %s", error->message);
			g_error_free (error);
		}
	}

	arv_gvcp_packet_free (packet);

	do {
		/* Now parse the result */
		if (g_poll (socket_list->poll_fds, socket_list->n_sockets,
					ARV_GV_INTERFACE_DISCOVERY_TIMEOUT_MS) == 0) {
			arv_gv_discover_socket_list_free (socket_list);
			return NULL;
		}

		for (i = 0, iter = socket_list->sockets; iter != NULL; i++, iter = iter->next) {
			ArvGvDiscoverSocket *socket = iter->data;

			do {
				g_socket_set_blocking (socket->socket, FALSE);
				count = g_socket_receive (socket->socket, buffer,
						ARV_GV_INTERFACE_SOCKET_BUFFER_SIZE,
						NULL, NULL);
				g_socket_set_blocking (socket->socket, TRUE);

				if (count > 0) {
					ArvGvcpPacket *packet = (ArvGvcpPacket *) buffer;

					if (g_ntohs (packet->header.command) == ARV_GVCP_COMMAND_READ_REGISTER_CMD ||
							g_ntohs (packet->header.command) == ARV_GVCP_COMMAND_READ_REGISTER_ACK) {
						GInetAddress *interface_address = g_inet_socket_address_get_address(
								G_INET_SOCKET_ADDRESS (socket->interface_address));

						g_object_ref(interface_address);
						arv_gv_discover_socket_list_free (socket_list);
						return interface_address;
					}
				}
			} while (count > 0);
		}
	} while (1);
	arv_gv_discover_socket_list_free (socket_list);
	return NULL;
}

static ArvDevice *
_open_device (ArvInterface *interface, GHashTable *devices, const char *device_id)
{
	ArvGvInterface *gv_interface;
	ArvDevice *device = NULL;
	ArvGvInterfaceDeviceInfos *device_infos;
	GInetAddress *device_address;

	gv_interface = ARV_GV_INTERFACE (interface);

	if (device_id == NULL) {
		GList *device_list;

		device_list = g_hash_table_get_values (devices);
		device_infos = device_list != NULL ? device_list->data : NULL;
		g_list_free (device_list);
	} else
		device_infos = g_hash_table_lookup (devices, device_id);

	if (device_infos == NULL) {
		struct addrinfo hints;
		struct addrinfo *servinfo, *endpoint;

		if (device_id == NULL)
			return NULL;

		/* Try if device_id is a hostname/IP address */

		memset(&hints, 0, sizeof (hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;

		if (getaddrinfo(device_id, "3956", &hints, &servinfo) != 0) {
			return NULL;
		}

		for (endpoint=servinfo; endpoint!=NULL; endpoint=endpoint->ai_next) {
			char ipstr[INET_ADDRSTRLEN];
			struct sockaddr_in *ip = (struct sockaddr_in *) endpoint->ai_addr;

			inet_ntop (endpoint->ai_family, &ip->sin_addr, ipstr, sizeof (ipstr));

			device_address = g_inet_address_new_from_string (ipstr);
			if (device_address != NULL) {
				/* Try and find an interface that the camera will respond on */
				GInetAddress *interface_address =
					arv_gv_interface_camera_locate (gv_interface, device_address);

				if (interface_address != NULL) {
					device = arv_gv_device_new (interface_address, device_address);
					g_object_unref (interface_address);
				}
			}
			g_object_unref (device_address);
			if (device != NULL) {
				break;
			}
		}
		freeaddrinfo (servinfo);
		return device;
	}

	device_address = _device_infos_to_ginetaddress (device_infos);
	device = arv_gv_device_new (device_infos->interface_address, device_address);
	g_object_unref (device_address);

	return device;
}

static ArvDevice *
arv_gv_interface_open_device (ArvInterface *interface, const char *device_id)
{
	ArvDevice *device;
	ArvGvInterfaceDeviceInfos *device_infos;

	device = _open_device (interface, ARV_GV_INTERFACE (interface)->priv->devices, device_id);
	if (ARV_IS_DEVICE (device))
		return device;

	device_infos = _discover (NULL, device_id);
	if (device_infos != NULL) {
		GInetAddress *device_address;

		device_address = _device_infos_to_ginetaddress (device_infos);
		device = arv_gv_device_new (device_infos->interface_address, device_address);
		g_object_unref (device_address);

		arv_gv_interface_device_infos_unref (device_infos);

		return device;
	}

	return NULL;
}

static ArvInterface *gv_interface = NULL;
static GMutex gv_interface_mutex;

/**
 * arv_gv_interface_get_instance:
 *
 * Gets the unique instance of the GV interface.
 *
 * Returns: (transfer none): a #ArvInterface singleton.
 */

ArvInterface *
arv_gv_interface_get_instance (void)
{
	g_mutex_lock (&gv_interface_mutex);

	if (gv_interface == NULL)
		gv_interface = g_object_new (ARV_TYPE_GV_INTERFACE, NULL);

	g_mutex_unlock (&gv_interface_mutex);

	return ARV_INTERFACE (gv_interface);
}

void
arv_gv_interface_destroy_instance (void)
{
	g_mutex_lock (&gv_interface_mutex);

	if (gv_interface != NULL) {
		g_object_unref (gv_interface);
		gv_interface = NULL;
	}

	g_mutex_unlock (&gv_interface_mutex);
}

static void
arv_gv_interface_init (ArvGvInterface *gv_interface)
{
	gv_interface->priv = G_TYPE_INSTANCE_GET_PRIVATE (gv_interface, ARV_TYPE_GV_INTERFACE, ArvGvInterfacePrivate);

	gv_interface->priv->devices = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
							     (GDestroyNotify) arv_gv_interface_device_infos_unref);
}

static void
arv_gv_interface_finalize (GObject *object)
{
	ArvGvInterface *gv_interface = ARV_GV_INTERFACE (object);

	g_hash_table_unref (gv_interface->priv->devices);
	gv_interface->priv->devices = NULL;

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

	interface_class->protocol = "GigEVision";
}

G_DEFINE_TYPE (ArvGvInterface, arv_gv_interface, ARV_TYPE_INTERFACE)
