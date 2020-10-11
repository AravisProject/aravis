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

#ifndef ARV_NETWORK_PRIVATE_H
#define ARV_NETWORK_PRIVATE_H

#include <arvnetwork.h>

#ifndef __MINGW32__
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#endif

#include <gio/gnetworking.h>

#if !defined(__APPLE__) && !defined(G_OS_WIN32)
#include <linux/ip.h>
#endif

#ifndef G_OS_WIN32
#include <netinet/udp.h>
#endif


#ifdef G_OS_WIN32
#include<stdint.h>
typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
struct udphdr
	{
		uint16_t source;
		uint16_t dest;
		uint16_t len;
		uint16_t check;
	};
#endif

#if defined(__APPLE__) || defined(G_OS_WIN32)
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

struct _ArvNetworkInterface{
	struct sockaddr *addr;
	struct sockaddr *netmask;
	struct sockaddr *broadaddr;
	char* name;
};

void arv_network_interface_free(ArvNetworkInterface* a);

gboolean arv_socket_set_recv_buffer_size(int socket_fd, gint buffer_size);

#endif
