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

#include <arvnetworkprivate.h>

#ifndef G_OS_WIN32
	#include <ifaddrs.h>
#endif

struct _ArvNetworkInterface{
	struct sockaddr *addr;
	struct sockaddr *netmask;
	struct sockaddr *broadaddr;
	char* name;
};

#ifdef G_OS_WIN32

GList *
arv_enumerate_network_interfaces (void)
{
	#warning arv_enumerate_network_interface not yet implemented for WIN32
	return NULL;
}

/*
 * mingw only defines inet_ntoa (ipv4-only), inet_ntop (IPv4 & IPv6) is missing from it headers
 * therefore we define it ourselves; code comes from https://www.mail-archive.com/users@ipv6.org/msg02107.html
 */

const char *
inet_ntop (int af, const void *src, char *dst, socklen_t cnt)
{
	if (af == AF_INET) {
		struct sockaddr_in in;

		memset (&in, 0, sizeof(in));
		in.sin_family = AF_INET;
		memcpy (&in.sin_addr, src, sizeof(struct in_addr));
		getnameinfo ((struct sockaddr *)&in, sizeof (struct sockaddr_in), dst, cnt, NULL, 0, NI_NUMERICHOST);
		return dst;
	} else if (af == AF_INET6) {
		struct sockaddr_in6 in;

		memset (&in, 0, sizeof(in));
		in.sin6_family = AF_INET6;
		memcpy (&in.sin6_addr, src, sizeof(struct in_addr6));
		getnameinfo ((struct sockaddr *)&in, sizeof (struct sockaddr_in6), dst, cnt, NULL, 0, NI_NUMERICHOST);
		return dst;
	}

	return NULL;
}

#else

GList*
arv_enumerate_network_interfaces (void)
{
	struct ifaddrs *ifap = NULL;
	struct ifaddrs *ifap_iter;
	GList* ret=NULL;

	if (getifaddrs (&ifap) <0)
		return NULL;

	for (ifap_iter = ifap; ifap_iter != NULL; ifap_iter = ifap_iter->ifa_next) {
		if ((ifap_iter->ifa_flags & IFF_UP) != 0 &&
			(ifap_iter->ifa_flags & IFF_POINTOPOINT) == 0 &&
			(ifap_iter->ifa_addr != NULL) &&
			(ifap_iter->ifa_addr->sa_family == AF_INET)) {
			ArvNetworkInterface* a;

			a = g_new0 (ArvNetworkInterface, 1);

			a->addr = g_memdup (ifap_iter->ifa_addr, sizeof(struct sockaddr));
			if (ifap_iter->ifa_netmask)
				a->netmask = g_memdup (ifap_iter->ifa_netmask, sizeof(struct sockaddr));
			if (ifap_iter->ifa_ifu.ifu_broadaddr)
				a->broadaddr = g_memdup(ifap_iter->ifa_ifu.ifu_broadaddr, sizeof(struct sockaddr));
			if (ifap_iter->ifa_name)
				a->name = g_strdup(ifap_iter->ifa_name);

			ret = g_list_prepend (ret, a);
		}
	}

	freeifaddrs (ifap);

	return g_list_reverse (ret);
};
#endif

struct sockaddr *
arv_network_interface_get_addr(ArvNetworkInterface* a)
{
	return a->addr;
}

struct sockaddr *
arv_network_interface_get_netmask(ArvNetworkInterface* a)
{
	return a->netmask;
}

struct sockaddr *
arv_network_interface_get_broadaddr(ArvNetworkInterface* a)
{
	return a->broadaddr;
}

const char *
arv_network_interface_get_name(ArvNetworkInterface* a)
{
	return a->name;
}

void
arv_network_interface_free(ArvNetworkInterface *a) {
	g_clear_pointer (&a->addr, g_free);
	g_clear_pointer (&a->netmask, g_free);
	g_clear_pointer (&a->broadaddr, g_free);
	g_clear_pointer (&a->name, g_free);
}


gboolean
arv_socket_set_recv_buffer_size (int socket_fd, gint buffer_size)
{
	int result;

#ifndef G_OS_WIN32
	result = setsockopt (socket_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof (buffer_size));
#else
	{
		DWORD _buffer_size=buffer_size;
		result = setsockopt (socket_fd, SOL_SOCKET, SO_RCVBUF, (const char*) &_buffer_size, sizeof (_buffer_size));
	}
#endif

	return result == 0;
}

