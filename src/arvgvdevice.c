#include <arvgvdevice.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

size_t
arv_gv_device_read_register (ArvDevice *device, guint64 address, size_t size, void *buffer)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);
	ArvGvHeader command;
	ArvGvControlPacket *answer;

	gv_device->packet_count++;

	command.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_COMMAND);
	command.command = g_htons (ARV_GVCP_COMMAND_REGISTER_READ_CMD);
	command.size = g_htons (size);
	command.count = g_htons (gv_device->packet_count);

	answer = g_malloc (sizeof (ArvGvHeader) + size);

	g_free (answer);

	return 0;
}

size_t
arv_gv_device_write_register (ArvDevice *device, guint64 address, size_t size, void *buffer)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);
	ArvGvControlPacket *command;

	gv_device->packet_count++;

	command = g_malloc (sizeof (ArvGvHeader) + size);

	command->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_COMMAND);
	command->header.command = g_htons (ARV_GVCP_COMMAND_REGISTER_WRITE_CMD);
	command->header.size = size;
	command->header.count = gv_device->packet_count;

	memcpy (&command->data, buffer, size);

	g_free (command);

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
}

static void
arv_gv_device_finalize (GObject *object)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (object);

	g_object_unref (gv_device->device_address);
	g_object_unref (gv_device->control_address);
	g_object_unref (gv_device->socket);

	parent_class->finalize (object);
}

static void
arv_gv_device_class_init (ArvGvDeviceClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (node_class);

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_gv_device_finalize;

	device_class->read_register = arv_gv_device_read_register;
	device_class->write_register = arv_gv_device_write_register;
}

G_DEFINE_TYPE (ArvGvDevice, arv_gv_device, ARV_TYPE_DEVICE)
