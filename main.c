#include <gio/gio.h>
#include <stdlib.h>

typedef enum {
	ARV_CONTROL_HEADER_1_COMMAND = 	0x0000,
	ARV_CONTROL_HEADER_1_ANSWER = 	0x4201
} ArvControlHeader1;

typedef enum {
	ARV_CONTROL_HEADER_2_DISCOVER =	0x0002,
	ARV_CONTROL_HEADER_2_HELLO =	0x0003,
	ARV_CONTROL_HEADER_2_BYE = 	0x0004
} ArvControlHeader2;

typedef struct {
	ArvControlHeader1 header1;
	ArvControlHeader2 header2;
	unsigned int length;
	unsigned int address;
	unsigned char data[];
} ArvControlPacket;

int
main (int argc, char **argv)
{
	GSocket *socket;
	GInetAddress *address;
	GSocketAddress *socket_address;
	GError *error = NULL;
	unsigned char buffer[1024];

	g_type_init ();

	socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, &error);
	address = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
	socket_address = g_inet_socket_address_new (address, 3956);

	g_socket_bind (socket, socket_address, TRUE, &error);

	while (1) {
		gsize count, i;

		count = g_socket_receive (socket, buffer, 1024, NULL, &error);
		if (count != 0)
			g_message ("packet received");
		for (i = 0; i < count; i++)
			g_printf ("0x%02x ", buffer[i]);
		g_printf ("\n");
	}

	g_object_unref (socket_address);
	g_object_unref (address);
	g_object_unref (socket);

	return EXIT_SUCCESS;
}
