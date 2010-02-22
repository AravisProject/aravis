#include <arvgvdevice.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

static size_t
_read (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);
	ArvGvcpPacket *packet;
	GPollFD poll_fd;
	size_t packet_size;
	size_t answer_size;
	int count;

	answer_size = arv_gvcp_read_packet_get_answer_size (size);

	g_return_val_if_fail (answer_size <= ARV_GV_DEVICE_BUFFER_SIZE, 0);

	gv_device->packet_count++;

	packet = arv_gvcp_read_packet_new (address,
					   ((size + sizeof (guint32) - 1) / sizeof (guint32)) * sizeof (guint32),
					   gv_device->packet_count, &packet_size);

	arv_gvcp_packet_dump (packet);

	g_socket_send_to (gv_device->socket, gv_device->device_address, (const char *) packet, packet_size,
			  NULL, NULL);

	arv_gvcp_packet_free (packet);

	poll_fd.fd = g_socket_get_fd (gv_device->socket);
	poll_fd.events =  G_IO_IN;
	poll_fd.revents = 0;

	if (g_poll (&poll_fd, 1, 1000) == 0)
		return 0;

	count = g_socket_receive (gv_device->socket, gv_device->socket_buffer, 1024, NULL, NULL);

	if (count >= answer_size)
		memcpy (buffer, arv_gvcp_read_packet_get_answer_data (gv_device->socket_buffer),
			size);

	arv_gvcp_packet_dump ((ArvGvcpPacket *) gv_device->socket_buffer);

	return packet_size;
}

size_t
arv_gv_device_read (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	guint i;
	gint32 block_size;

	for (i = 0; i < (size + ARV_GVCP_DATA_SIZE_MAX - 1) / ARV_GVCP_DATA_SIZE_MAX; i++) {
		block_size = MIN (ARV_GVCP_DATA_SIZE_MAX, size - i * ARV_GVCP_DATA_SIZE_MAX);
		_read (device, address + i * ARV_GVCP_DATA_SIZE_MAX,
		       block_size, buffer + i * ARV_GVCP_DATA_SIZE_MAX);
	}

	return size;
}

size_t
arv_gv_device_write (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	return 0;
}

ArvDevice *
arv_gv_device_new (GInetAddress *inet_address)
{
	ArvGvDevice *gv_device;
	GInetAddress *incoming_inet_address;

	g_return_val_if_fail (G_IS_INET_ADDRESS (inet_address), NULL);


	gv_device = g_object_new (ARV_TYPE_GV_DEVICE, NULL);

	gv_device->socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
					  G_SOCKET_TYPE_DATAGRAM,
					  G_SOCKET_PROTOCOL_UDP, NULL);

	incoming_inet_address = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
	gv_device->control_address = g_inet_socket_address_new (incoming_inet_address, 0);
	g_object_unref (incoming_inet_address);

	gv_device->device_address = g_inet_socket_address_new (inet_address, ARV_GVCP_PORT);

	g_socket_bind (gv_device->socket, gv_device->control_address, TRUE, NULL);

	arv_device_load_genicam (ARV_DEVICE (gv_device));

	return ARV_DEVICE (gv_device);
}

static void
arv_gv_device_init (ArvGvDevice *gv_device)
{
	gv_device->packet_count = 0;
	gv_device->socket_buffer = g_malloc (ARV_GV_DEVICE_BUFFER_SIZE);
}

static void
arv_gv_device_finalize (GObject *object)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (object);

	g_object_unref (gv_device->device_address);
	g_object_unref (gv_device->control_address);
	g_object_unref (gv_device->socket);

	g_free (gv_device->socket_buffer);

	parent_class->finalize (object);
}

static void
arv_gv_device_class_init (ArvGvDeviceClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (node_class);

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_gv_device_finalize;

	device_class->read = arv_gv_device_read;
	device_class->write = arv_gv_device_write;
}

G_DEFINE_TYPE (ArvGvDevice, arv_gv_device, ARV_TYPE_DEVICE)
