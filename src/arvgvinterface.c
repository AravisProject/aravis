#include <arvgvinterface.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>

static GObjectClass *parent_class = NULL;

#define ARV_GV_CONTROL_PORT	3956

typedef enum {
	ARV_GV_HEADER_1_ANSWER = 	0x0000,
	ARV_GV_HEADER_1_COMMAND = 	0x4201
} ArvGvHeader1;

typedef enum {
	ARV_GV_HEADER_2_DISCOVER =	0x0002,
	ARV_GV_HEADER_2_HELLO =		0x0003,
	ARV_GV_HEADER_2_BYE = 		0x0004
} ArvGvHeader2;

typedef struct {
	guint16 header1;
	guint16 header2;
	guint16 length;
	guint16 address;
}  __attribute__((__packed__)) ArvGvHeader;

typedef struct {
	ArvGvHeader header;
	unsigned char data[];
} ArvGvControlPacket;

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
arv_gv_interface_update_device_list (ArvGvInterface *gv_interface)
{
	static ArvGvHeader arv_gv_discover_packet;

	arv_gv_discover_packet.header1 = g_htons (ARV_GV_HEADER_1_COMMAND);
	arv_gv_discover_packet.header2 = g_htons (ARV_GV_HEADER_2_DISCOVER);
	arv_gv_discover_packet.address = g_htons (0x0000);
	arv_gv_discover_packet.length = g_htons (0xffff);

	arv_gv_interface_socket_set_broadcast (gv_interface->socket, TRUE);
	g_socket_send_to (gv_interface->socket,
			  gv_interface->broadcast_address,
			  (const char *) &arv_gv_discover_packet,
			  sizeof (arv_gv_discover_packet), NULL, NULL);
	arv_gv_interface_socket_set_broadcast (gv_interface->socket, FALSE);

	while (1) {
		gsize count, i;
		char buffer[1024];

		count = g_socket_receive (gv_interface->socket, buffer, 1024, NULL, NULL);
		if (count != 0)
			g_message ("packet received");
		for (i = 0; i < count; i++)
			g_printf ("0x%02x ", (unsigned char) buffer[i]);
		g_printf ("\n");
	}
}

static void
arv_gv_interface_get_devices (ArvInterface *interface)
{
	arv_gv_interface_update_device_list (ARV_GV_INTERFACE (interface));
}

ArvInterface *
arv_gv_interface_get_instance (void)
{
	static ArvInterface *gv_interface = NULL;
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

	g_static_mutex_lock (&mutex);

	if (gv_interface == NULL)
		gv_interface = g_object_new (ARV_TYPE_GV_INTERFACE, NULL);
	else
		g_object_ref (gv_interface);

	g_static_mutex_unlock (&mutex);

	return ARV_INTERFACE (gv_interface);
}

static void
arv_gv_interface_init (ArvGvInterface *gv_interface)
{
	GInetAddress *inet_address;

	gv_interface->socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
					     G_SOCKET_TYPE_DATAGRAM,
					     G_SOCKET_PROTOCOL_UDP, NULL);

	inet_address = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
	gv_interface->control_address = g_inet_socket_address_new (inet_address, 0);
	g_object_unref (inet_address);

	inet_address = g_inet_address_new_from_string ("255.255.255.255");
	gv_interface->broadcast_address = g_inet_socket_address_new (inet_address, ARV_GV_CONTROL_PORT);
	g_object_unref (inet_address);

	g_socket_bind (gv_interface->socket, gv_interface->control_address, TRUE, NULL);
}

static void
arv_gv_interface_finalize (GObject *object)
{
	ArvGvInterface *gv_interface = ARV_GV_INTERFACE (object);

	g_object_unref (gv_interface->socket);
	g_object_unref (gv_interface->broadcast_address);
	g_object_unref (gv_interface->control_address);

	parent_class->finalize (object);
}

static void
arv_gv_interface_class_init (ArvGvInterfaceClass *gv_interface_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (gv_interface_class);
	ArvInterfaceClass *interface_class = ARV_INTERFACE_CLASS (gv_interface_class);

	parent_class = g_type_class_peek_parent (gv_interface_class);

	object_class->finalize = arv_gv_interface_finalize;

	interface_class->get_devices = arv_gv_interface_get_devices;
}

G_DEFINE_TYPE (ArvGvInterface, arv_gv_interface, ARV_TYPE_INTERFACE)
