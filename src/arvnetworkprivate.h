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
 * Authors: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *          Václav Šmilauer <eu@doxos.eu>
 */

#ifndef ARV_NETWORK_PRIVATE_H
#define ARV_NETWORK_PRIVATE_H

#include <arvapi.h>

#include <gio/gnetworking.h>
#include <gio/gio.h>

#ifndef G_OS_WIN32
#include <sys/param.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#endif

#ifdef __linux__
#include <linux/ip.h>
#endif

#if defined(__APPLE__) || defined(BSD)
#include <netinet/ip.h>
#define iphdr ip
#endif

#ifndef G_OS_WIN32
#include <netinet/udp.h>
#endif

#ifdef G_OS_WIN32
#include <stdint.h>
typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
struct udphdr {
	uint16_t source;
	uint16_t dest;
	uint16_t len;
	uint16_t check;
};
#endif

#if defined(G_OS_WIN32)
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

typedef struct _ArvNetworkInterface ArvNetworkInterface;

/* private, but used by tests */
ARV_API GList *		arv_enumerate_network_interfaces	(void);
ArvNetworkInterface*	arv_network_get_interface_by_name	(const char* name);
ArvNetworkInterface*	arv_network_get_interface_by_address	(const char* addr);
ArvNetworkInterface*	arv_network_get_fake_ipv4_loopback	(void);

/* private, but used by tests */
ARV_API void 			arv_network_interface_free		(ArvNetworkInterface *a);
ARV_API struct sockaddr *	arv_network_interface_get_addr		(ArvNetworkInterface *a);
ARV_API struct sockaddr *	arv_network_interface_get_netmask	(ArvNetworkInterface *a);
ARV_API struct sockaddr *	arv_network_interface_get_broadaddr	(ArvNetworkInterface *a);
ARV_API const char *		arv_network_interface_get_name		(ArvNetworkInterface *a);
ARV_API gboolean		arv_network_interface_is_loopback	(ArvNetworkInterface *a);

gboolean			arv_socket_set_recv_buffer_size		(int socket_fd, gint buffer_size);

#ifdef G_OS_WIN32
	/* mingw only defines with _WIN32_WINNT>=0x0600, see
	 * https://github.com/AravisProject/aravis/issues/416#issuecomment-717220610 */
	#if _WIN32_WINNT < 0x0600
		const char * inet_ntop (int af, const void *src, char *dst, socklen_t cnt);
	#endif
#endif

/* Workaround for broken socket g_poll under Windows, otherwise no-op */

void			arv_gpollfd_prepare_all			(GPollFD *fds, guint nfds);
void			arv_gpollfd_clear_one			(GPollFD *fd, GSocket* socket);
void 			arv_gpollfd_finish_all			(GPollFD *fds, guint nfds);

#endif
