#include <arv.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/arvnetworkprivate.h"

#define _ALEN 16
#define _ALENS "16"
#define _LINEFMT "%20s %10s %" _ALENS "s %" _ALENS "s %" _ALENS "s\n"

int
main (int argc, char **argv){
	GList *ifaces;
	GList *iface_iter;

	ifaces = arv_enumerate_network_interfaces ();
	if (!ifaces) {
		fprintf (stderr,"No network interfaces found (or enumeration failed).");
		return 1;
	}
	printf(_LINEFMT,"interface","proto","address","mask","broadcast");

	for (iface_iter=ifaces; iface_iter!=NULL; iface_iter=iface_iter->next){
		ArvNetworkInterface* ani = (ArvNetworkInterface*)iface_iter->data;
		char addr[_ALEN];
		char netmask[_ALEN];
		char broadaddr[_ALEN];
		int fam = arv_network_interface_get_addr(ani)->sa_family;
		if (fam==AF_INET){
			inet_ntop (fam, &((struct sockaddr_in*)arv_network_interface_get_addr(ani))->sin_addr, &addr[0], _ALEN);
			inet_ntop (fam, &((struct sockaddr_in*)arv_network_interface_get_netmask(ani))->sin_addr, &netmask[0], _ALEN);
			inet_ntop (fam, &((struct sockaddr_in*)arv_network_interface_get_addr(ani))->sin_addr, &broadaddr[0], _ALEN);
			printf (_LINEFMT, arv_network_interface_get_name(ani), "IPv4", addr, netmask, broadaddr);
		}
		else if (fam==AF_INET6){
			fprintf (stderr,"%s: IPv6 not yet reported correctly", arv_network_interface_get_name(ani));
		}
		else {
			fprintf (stderr,"%s: unrecognized family %d", arv_network_interface_get_name(ani), fam);
		}
	}
	g_list_free_full (ifaces, (GDestroyNotify) arv_network_interface_free);
	return 0;
}


