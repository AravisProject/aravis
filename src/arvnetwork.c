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

#include <arvnetworkprivate.h>
#include <arvdebugprivate.h>
#include <arvmiscprivate.h>

#ifndef G_OS_WIN32
	#include <ifaddrs.h>
#else
	#include <winsock2.h>
	#include <iphlpapi.h>
	#include <winnt.h>	/* For PWCHAR */
#endif

struct _ArvNetworkInterface{
	struct sockaddr *addr;
	struct sockaddr *netmask;
	struct sockaddr *broadaddr;
	char* name;
};

#ifdef G_OS_WIN32

ARV_DEFINE_CONSTRUCTOR (arv_initialize_networking)
static void
arv_initialize_networking (void)
{
	long res;
	WSADATA wsaData;

	/* not sure which version is really needed, just use 2.2 (latest) */
	res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != 0) {
		/* error description functions should not be used when WSAStartup failed (not sure), do manually */
		char *desc = "[unknown error code]";

		switch (res) {
			case WSASYSNOTREADY:
				desc = "The underlying network subsystem is not ready for network communication.";
				break;
			case WSAVERNOTSUPPORTED:
				desc = "The version of Windows Sockets support requested is not provided by this particular Windows Sockets implementation.";
				break;
			case WSAEINPROGRESS:
				desc = "A blocking Windows Sockets 1.1 operation is in progress.";
				break;
			case WSAEPROCLIM:
				desc = "A limit on the number of tasks supported by the Windows Sockets implementation has been reached.";
				break;
			case WSAEFAULT:
				desc = "The lpWSAData parameter is not a valid pointer.";
				break;
		};
		g_critical ("WSAStartup failed with error %ld: %s", res, desc);
	}
	arv_info_interface ("WSAStartup done.");
}

ARV_DEFINE_DESTRUCTOR (arv_cleanup_networking)
static void
arv_cleanup_networking (void)
{
	long res;

	res=WSACleanup();
	if (res != 0){
		char *desc = "[unknown error code]";

		switch (res) {
			case WSANOTINITIALISED:
				desc = "A successful WSAStartup call must occur before using this function.";
				break;
			case WSAENETDOWN:
				desc = "The network subsystem has failed.";
				break;
			case WSAEINPROGRESS:
				desc = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
				break;
		};
		g_critical ("WSACleanup failed with error %ld: %s", res, desc);
	}
}

GList *
arv_enumerate_network_interfaces (void)
{
	/*
	 * docs: https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersaddresses
	 *
	 * example source: https://github.com/zeromq/czmq/blob/master/src/ziflist.c#L284
	 * question about a better solution: https://stackoverflow.com/q/64348510/761090
	 *
	 * note: >= Vista code untested
	 */
	ULONG outBufLen = 15000;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	PIP_ADAPTER_ADDRESSES pAddrIter = NULL;
	GList* ret;
	int iter = 0;
	ULONG dwRetVal;

	/* pre-Vista windows don't have PIP_ADAPTER_UNICAST_ADDRESS onLinePrefixLength field.
	 * To get netmask, we build pIPAddrTable (IPv4-only) and find netmask associated to each addr.
	 * This means IPv6 will only work with >= Vista.
	 * See https://stackoverflow.com/a/64358443/761090 for thorough explanation.
	 */
	#if WINVER < _WIN32_WINNT_VISTA
		// https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getipaddrtable
		PMIB_IPADDRTABLE pIPAddrTable;
		DWORD dwSize = 0;
		/* Variables used to return error message */
		pIPAddrTable = (MIB_IPADDRTABLE *) g_malloc(sizeof (MIB_IPADDRTABLE));
		if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
			g_free(pIPAddrTable);
			pIPAddrTable = (MIB_IPADDRTABLE *) g_malloc(dwSize);
		}
		dwRetVal = GetIpAddrTable( pIPAddrTable, &dwSize, 0 );
		if (dwRetVal != NO_ERROR ) {
			arv_warning_interface("GetIpAddrTable failed.");
			g_free(pIPAddrTable);
		}
	#endif

	do {
		pAddresses = (IP_ADAPTER_ADDRESSES*) g_malloc (outBufLen);
		/* change family to AF_UNSPEC for both IPv4 and IPv6, later */
		dwRetVal = GetAdaptersAddresses(
			/* Family */ AF_INET,
			/* Flags */
				GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
			/* Reserved */ NULL,
			pAddresses,
			&outBufLen
		);
		if (dwRetVal==ERROR_BUFFER_OVERFLOW) {
			g_free (pAddresses);
		} else {
			break;
		}
		iter++;
	} while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (iter<3));

	if (dwRetVal != ERROR_SUCCESS){
		arv_warning_interface ("Failed to enumerate network interfaces (GetAdaptersAddresses returned %lu)",
				       dwRetVal);
		return NULL;
	}


	ret = NULL;

	for(pAddrIter = pAddresses; pAddrIter != NULL; pAddrIter = pAddrIter->Next){
		PIP_ADAPTER_UNICAST_ADDRESS pUnicast;
		for (pUnicast = pAddrIter->FirstUnicastAddress; pUnicast != NULL; pUnicast = pUnicast->Next){
			// PIP_ADAPTER_PREFIX pPrefix = pAddrIter->FirstPrefix;
			struct sockaddr* lpSockaddr = pUnicast->Address.lpSockaddr;
			ArvNetworkInterface* a;

			gboolean ok = (pAddrIter->OperStatus == IfOperStatusUp)
				/* extend for IPv6 here, later */
				&& ((lpSockaddr->sa_family == AF_INET)
				#if 0 && WINVER >= _WIN32_WINNT_VISTA
					|| (lpSockaddr->sa_family == AF_INET6)
				#endif
				)
			;

			if (!ok) continue;

			a = (ArvNetworkInterface*) g_malloc0(sizeof(ArvNetworkInterface));
			if (lpSockaddr->sa_family == AF_INET){
				struct sockaddr_in* mask;
				struct sockaddr_in* broadaddr;

				/* copy 3x so that sa_family is already set for netmask and broadaddr */
				a->addr = arv_memdup (lpSockaddr, sizeof(struct sockaddr));
				a->netmask = arv_memdup (lpSockaddr, sizeof(struct sockaddr));
				a->broadaddr = arv_memdup (lpSockaddr, sizeof(struct sockaddr));
				/* adjust mask & broadcast */
				mask = (struct sockaddr_in*) a->netmask;
				#if WINVER >= _WIN32_WINNT_VISTA
					mask->sin_addr.s_addr = htonl ((0xffffffffU) << (32 - pUnicast->OnLinkPrefixLength));
				#else
					{
						int i;
						gboolean match = FALSE;

						for (i=0; i < pIPAddrTable->dwNumEntries; i++){
							MIB_IPADDRROW* row=&pIPAddrTable->table[i];
							#if 0
								fprintf(stderr,"row %d: %08lx: match with %08lx (mask %08lx): %d\n",i,((struct sockaddr_in*)a->addr)->sin_addr.s_addr,row->dwAddr,row->dwMask,row->dwAddr == ((struct sockaddr_in*)a->addr)->sin_addr.s_addr);
							#endif
							/* both are in network byte order, no need to convert */
							if (row->dwAddr == ((struct sockaddr_in*)a->addr)->sin_addr.s_addr){
								match = TRUE;
								mask->sin_addr.s_addr = row->dwMask;
								break;
							}
						}
						if (!match){
							arv_warning_interface ("Failed to obtain netmask for %08lx (secondary address?), using 255.255.0.0.",((struct sockaddr_in*)a->addr)->sin_addr.s_addr);
							mask->sin_addr.s_addr = htonl(0xffff0000U);
						}
					}
				#endif
				broadaddr = (struct sockaddr_in*) a->broadaddr;
				broadaddr->sin_addr.s_addr |= ~(mask->sin_addr.s_addr);
			}
			#if WINVER >= _WIN32_WINNT_VISTA
			else if (lpSockaddr->sa_family == AF_INET6){
				arv_warning_interface("IPv6 support not yet implemented.");
			}
			#endif
			/* name is common to IPv4 and IPv6 */
			{
				PWCHAR name = pAddrIter->FriendlyName;
				size_t asciiSize = wcstombs (0, name, 0) + 1;
				a->name = (char *) g_malloc (asciiSize);
				wcstombs (a->name, name, asciiSize);
			}

			ret = g_list_prepend(ret, a);
		}
	}
	g_free (pAddresses);
	#if WINVER < _WIN32_WINNT_VISTA
		g_free(pIPAddrTable);
	#endif

	ret = g_list_reverse(ret);
	return ret;
}

/*
 * mingw only defines inet_ntoa (ipv4-only), inet_ntop (IPv4 & IPv6) is missing from it headers
 * for _WIN32_WINNT < 0x0600; therefore we define it ourselves
 * The code comes from https://www.mail-archive.com/users@ipv6.org/msg02107.html.
 */
#if _WIN32_WINNT < 0x0600
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
#endif /* _WIN32_WINNT < 0x0600 */


/*
 * g_poll does not work with sockets under windows correctly. The reason is that
 * g_poll uses WaitForMultipleObjectEx API function which does not handle Winsock
 * sockets descriptors directly. Instead, one should
 *
 * (a) create a new event (WSAEVENT) associated with each socket polled, and its
 *     descriptor passed to g_poll instead (WSACreateEvent, WSAEventSelect);
 *
 * (b) when g_poll returns, the event must be cleared so that it is not returned
 *     again (WSAEnumNetworkEvents);
 *
 * (c) events created in (a) must be closed (WSACloseEvent).
 *
 * These three points are respectively handled by arv_gpollfd_prepare_all,
 * arv_gpollfd_clear_one and arv_gpollfd_finish_all. These functions are no-op
 * for non-Windows builds.
 *
 * https://discourse.gnome.org/t/g-poll-times-out-with-windows is the original
 * discussion of the issue.
 *
 * https://gitlab.gnome.org/GNOME/glib/-/issues/214 is GLib bug report
 * (g_poll does not work on win32 sockets)
 *
 */

void
arv_gpollfd_prepare_all (GPollFD *fds, guint nfds){
	guint i;
	int wsaRes;

	for (i = 0; i < nfds; i++) {
		gint64 fd = fds[i].fd;
		fds[i].fd = (gint64) WSACreateEvent();
		g_assert ((WSAEVENT) fds[i].fd != WSA_INVALID_EVENT);
		wsaRes = WSAEventSelect (fd, (WSAEVENT) fds[i].fd, FD_READ);
		g_assert (wsaRes == 0);
	}
}

void
arv_gpollfd_clear_one (GPollFD *fd, GSocket* socket){
	WSANETWORKEVENTS wsaNetEvents;
	int wsaRes;

	wsaRes = WSAEnumNetworkEvents (g_socket_get_fd(socket), (WSAEVENT) fd->fd, &wsaNetEvents);
	/* TODO: check return value, check wsaNetEvents for errors */
}

void
arv_gpollfd_finish_all (GPollFD *fds, guint nfds){
	guint i;

	for (i = 0; i < nfds; i++){
		WSACloseEvent( (WSAEVENT) fds[i].fd);
	}
}


#else /* not G_OS_WIN32 */

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
			(ifap_iter->ifa_addr != NULL) &&
			(ifap_iter->ifa_addr->sa_family == AF_INET)) {
			ArvNetworkInterface* a;

			a = g_new0 (ArvNetworkInterface, 1);

			a->addr = arv_memdup (ifap_iter->ifa_addr, sizeof(struct sockaddr));
			if (ifap_iter->ifa_netmask)
				a->netmask = arv_memdup (ifap_iter->ifa_netmask, sizeof(struct sockaddr));
#if (defined(__APPLE__) && defined(__MACH__)) || defined(BSD)
			if (ifap_iter->ifa_broadaddr &&
			    ifap_iter->ifa_broadaddr->sa_len != 0) {
				a->broadaddr = arv_memdup(ifap_iter->ifa_broadaddr, ifap_iter->ifa_broadaddr->sa_len);
			} else {
				/* Interface have no broadcast address,
				 * IFF_BROADCAST probably not set,
				 * this workaround to pass fakecamera
				 * test that uses 127.0.0.1 address on
				 * loopback interface. */
				a->broadaddr = arv_memdup(ifap_iter->ifa_addr, ifap_iter->ifa_addr->sa_len);
			}
#else
			if (ifap_iter->ifa_ifu.ifu_broadaddr)
				a->broadaddr = arv_memdup(ifap_iter->ifa_ifu.ifu_broadaddr, sizeof(struct sockaddr));
#endif

			if (ifap_iter->ifa_name)
				a->name = g_strdup(ifap_iter->ifa_name);

			ret = g_list_prepend (ret, a);
		}
	}

	freeifaddrs (ifap);

	return g_list_reverse (ret);
};

/* no-op functions for Win32 GLib bug workaround, see above */

void
arv_gpollfd_prepare_all(GPollFD *fds, guint nfds)
{
	return;
}
void
arv_gpollfd_clear_one(GPollFD *fds, GSocket* socket)
{
	return;
}
void
arv_gpollfd_finish_all(GPollFD *fds, guint nfds)
{
	return;
}

#endif /* G_OS_WIN32 */

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
arv_network_interface_free(ArvNetworkInterface *a)
{
	g_clear_pointer (&a->addr, g_free);
	g_clear_pointer (&a->netmask, g_free);
	g_clear_pointer (&a->broadaddr, g_free);
	g_clear_pointer (&a->name, g_free);
	g_free (a);
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
		result = setsockopt (socket_fd, SOL_SOCKET, SO_RCVBUF,
				     (const char*) &_buffer_size, sizeof (_buffer_size));
	}
#endif

	return result == 0;
}


ArvNetworkInterface*
arv_network_get_interface_by_name (const char* name)
{
	GList *ifaces;
	GList *iface_iter;
	ArvNetworkInterface *ret = NULL;

	ifaces = arv_enumerate_network_interfaces ();

	for (iface_iter = ifaces; iface_iter != NULL; iface_iter = iface_iter->next) {
		if (g_strcmp0 (name, arv_network_interface_get_name (iface_iter->data)) == 0)
			break;
	}

	if(iface_iter != NULL){
		/* remove the interface node from the list (deleted below) but don't delete its data */
		ret = iface_iter->data;
		ifaces = g_list_remove_link(ifaces, iface_iter);
		g_list_free(iface_iter);
	}

	g_list_free_full (ifaces, (GDestroyNotify) arv_network_interface_free);

	return ret;
}

ArvNetworkInterface*
arv_network_get_interface_by_address (const char* addr)
{
	GInetSocketAddress *iaddr_s = NULL;
	GInetAddress *iaddr = NULL;
	GList *ifaces;
	GList *iface_iter;
	ArvNetworkInterface *ret = NULL;

	ifaces = arv_enumerate_network_interfaces ();

	if (!g_hostname_is_ip_address(addr))
		return NULL;

	iaddr_s = G_INET_SOCKET_ADDRESS (g_inet_socket_address_new_from_string (addr, 0));
	if (iaddr_s == NULL)
		return NULL;

	iaddr = g_inet_socket_address_get_address(iaddr_s);

	for (iface_iter = ifaces; iface_iter != NULL; iface_iter = iface_iter->next) {
		GSocketAddress *iface_sock_addr;
		GInetAddress *iface_inet_addr;

		iface_sock_addr = g_socket_address_new_from_native
			(arv_network_interface_get_addr (iface_iter->data), sizeof(struct sockaddr));
		iface_inet_addr = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (iface_sock_addr));
		if (g_inet_address_equal (iaddr,iface_inet_addr)) {
			g_clear_object (&iface_sock_addr);
			break;
		}
		g_clear_object(&iface_sock_addr);
	}

	if (iface_iter != NULL) {
		ret = iface_iter->data;
		ifaces = g_list_remove_link (ifaces, iface_iter);
		g_list_free (iface_iter);
	}

	g_clear_object(&iaddr_s);
	g_list_free_full (ifaces, (GDestroyNotify) arv_network_interface_free);

	return ret;
}

ArvNetworkInterface*
arv_network_get_fake_ipv4_loopback (void)
{
	ArvNetworkInterface* ret = (ArvNetworkInterface*) g_malloc0(sizeof(ArvNetworkInterface));

	ret->name = g_strdup ("<fake IPv4 localhost>");
	ret->addr = g_malloc0 (sizeof(struct sockaddr_in));
	ret->addr->sa_family = AF_INET;
	((struct sockaddr_in*)ret->addr)->sin_addr.s_addr = htonl(0x7f000001); // INADDR_LOOPBACK
	ret->netmask = g_malloc0 (sizeof(struct sockaddr_in));
	ret->netmask->sa_family = AF_INET;
	((struct sockaddr_in*)ret->netmask)->sin_addr.s_addr = htonl(0xff000000);
	ret->broadaddr = g_malloc0 (sizeof(struct sockaddr_in));
	ret->broadaddr->sa_family = AF_INET;
	((struct sockaddr_in*)ret->broadaddr)->sin_addr.s_addr = htonl(0x7fffffff);

	return ret;
}

gboolean
arv_network_interface_is_loopback(ArvNetworkInterface *a)
{
	if(!a)
		return FALSE;

	if (a->addr->sa_family==AF_INET)
		return (ntohl(((struct sockaddr_in*)a->addr)->sin_addr.s_addr)>>24)==0x7f;

	if (a->addr->sa_family==AF_INET6) {
		unsigned int pos;
		/* 16 unsigned chars in network byte order (big-endian),
		   loopback is ::1, i.e. zeros and 1 at the end */
		unsigned char* i6=(unsigned char*)(&(((struct sockaddr_in6*)a->addr)->sin6_addr));

		for (pos=0; pos<16; pos++) {
			if (i6[pos]!=0)
				return FALSE;
		}

		if (i6[16]!=1)
			return FALSE;

		return TRUE;
	}

	return FALSE;
}


