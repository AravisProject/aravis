#include <gio/gio.h>
#include <glib/gprintf.h>
#include <stdlib.h>

#define ARV_GVCP_PORT	3596

typedef enum {
	ARV_GV_HEADER_1_COMMAND = 	0x0000,
	ARV_GV_HEADER_1_ANSWER = 	0x4201
} ArvGVHeader1;

typedef enum {
	ARV_GV_HEADER_2_DISCOVER =	0x0002,
	ARV_GV_HEADER_2_HELLO =		0x0003,
	ARV_GV_HEADER_2_BYE = 		0x0004
} ArvGVHeader2;

typedef struct {
	ArvGVHeader1 header1;
	ArvGVHeader2 header2;
	unsigned int length;
	unsigned int address;
} ArvGVHeader;

typedef struct {
	ArvGVHeader header;
	unsigned char data[];
} ArvControlPacket;

static const ArvControlPacket arv_discover_packet = {
	{
		ARV_GV_HEADER_1_COMMAND,
		ARV_GV_HEADER_2_DISCOVER,
		0, 0xffff
	}
};

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

	inet_address = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
	broadcast_address = g_inet_socket_address_new (inet_address, 3596);
	g_object_unref (inet_address);

	g_socket_bind (socket, gvcp_address, TRUE, &error);

	g_socket_send_to (socket, broadcast_address,
			  (const char *) &arv_discover_packet, sizeof (arv_discover_packet), NULL, &error);

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
