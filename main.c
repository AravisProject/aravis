#include <gio/gio.h>
#include <glib/gprintf.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>

#define ARV_GVCP_PORT	3956

typedef enum {
	ARV_GV_HEADER_1_ANSWER = 	0x0000,
	ARV_GV_HEADER_1_COMMAND = 	0x4201
} ArvGVHeader1;

typedef enum {
	ARV_GV_HEADER_2_DISCOVER =	0x0002,
	ARV_GV_HEADER_2_HELLO =		0x0003,
	ARV_GV_HEADER_2_BYE = 		0x0004
} ArvGVHeader2;

typedef struct {
	guint16 header1;
	guint16 header2;
	guint16 length;
	guint16 address;
}  __attribute__((__packed__)) ArvGVHeader;

typedef struct {
	ArvGVHeader header;
	unsigned char data[];
} ArvControlPacket;

static const ArvControlPacket arv_discover_packet = {
	{
		g_htons (ARV_GV_HEADER_1_COMMAND),
		g_htons (ARV_GV_HEADER_2_DISCOVER),
		g_htons (0x0000),
		g_htons (0xffff)
	}
};

gboolean
arv_socket_set_broadcast (GSocket *socket, gboolean enable)
{
	int socket_fd;
	int result;

	socket_fd = g_socket_get_fd (socket);

	result = setsockopt (socket_fd, SOL_SOCKET, SO_BROADCAST, (char*)&enable, sizeof (enable));

	return result == 0;
}

int
main (int argc, char **argv)
{
	GSocket *socket;
	GInetAddress *inet_address;
	GSocketAddress *gvcp_address;
	GSocketAddress *broadcast_address;
	GError *error = NULL;
	char buffer[1024];

	g_type_init ();

	socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, &error);

	inet_address = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
	gvcp_address = g_inet_socket_address_new (inet_address, 0);
	g_object_unref (inet_address);

	inet_address = g_inet_address_new_from_string ("255.255.255.255");
	broadcast_address = g_inet_socket_address_new (inet_address, ARV_GVCP_PORT);
	g_object_unref (inet_address);

	g_socket_bind (socket, gvcp_address, TRUE, &error);

	if (error != NULL)
		g_message ("%s", error->message);

	arv_socket_set_broadcast (socket, TRUE);
	g_socket_send_to (socket, broadcast_address,
			  (const char *) &arv_discover_packet, sizeof (arv_discover_packet), NULL, &error);
	arv_socket_set_broadcast (socket, FALSE);

	while (1) {
		gsize count, i;

		count = g_socket_receive (socket, buffer, 1024, NULL, &error);
		if (count != 0)
			g_message ("packet received");
		for (i = 0; i < count; i++)
			g_printf ("0x%02x ", (unsigned char) buffer[i]);
		g_printf ("\n");
	}

	g_object_unref (gvcp_address);
	g_object_unref (broadcast_address);
	g_object_unref (socket);

	return EXIT_SUCCESS;
}
