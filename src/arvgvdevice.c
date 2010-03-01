#include <arvgvdevice.h>
#include <arvdebug.h>
#include <arvgvstream.h>
#include <string.h>
#include <stdlib.h>

static GObjectClass *parent_class = NULL;
static GRegex *arv_gv_device_url_regex = NULL;

/* ArvGvDevice implemenation */

static gboolean
arv_gv_device_take_control (ArvGvDevice *gv_device)
{
	gv_device->is_controller = arv_device_write_register (ARV_DEVICE (gv_device),
							      ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE, 2);

	return gv_device->is_controller;
}

static gboolean
arv_gv_device_leave_control (ArvGvDevice *gv_device)
{
	gboolean result;

	result = arv_device_write_register (ARV_DEVICE (gv_device),
					    ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE, 0);
	gv_device->is_controller = FALSE;

	return result;
}

gboolean
arv_gv_device_heartbeat (ArvGvDevice *gv_device)
{
	guint32 value;

	g_return_val_if_fail (ARV_IS_GV_DEVICE (gv_device), FALSE);

	if (!arv_device_read_register (ARV_DEVICE (gv_device), ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE, &value) ||
	    value != 0)
		gv_device->is_controller = FALSE;

	return gv_device->is_controller;
}

static char *
_load_genicam (ArvGvDevice *gv_device, guint32 address)
{
	char filename[ARV_GVBS_XML_URL_SIZE];
	char **tokens;
	char *genicam = NULL;

	arv_device_read_memory (ARV_DEVICE (gv_device), address, ARV_GVBS_XML_URL_SIZE, filename);
	filename[ARV_GVBS_XML_URL_SIZE - 1] = '\0';

	arv_debug ("GvDevice::load_genicam] xml url = '%s' at 0x%x", filename, address);

	tokens = g_regex_split (arv_gv_device_url_regex, filename, 0);

	if (tokens[0] != NULL) {
		if (g_strcmp0 (tokens[1], "File:") == 0)
			g_file_get_contents (filename, &genicam, NULL, NULL);
		else if (g_strcmp0 (tokens[1], "Local:") == 0 &&
			 tokens[2] != NULL &&
			 tokens[3] != NULL &&
			 tokens[4] != NULL) {
			guint32 file_address;
			guint32 file_size;

			file_address = strtoul (tokens[3], NULL, 16);
			file_size = strtoul (tokens[4], NULL, 16);

			arv_debug ("[GvDevice::load_genicam] Xml address = 0x%x - size = 0x%x",
				   file_address, file_size);

			if (file_size > 0) {
				genicam = g_malloc (file_size);
				arv_device_read_memory (ARV_DEVICE (gv_device), file_address, file_size,
							genicam);
				genicam [file_size - 1] = '\0';
			}
		}
	}

	g_strfreev (tokens);

	return genicam;
}

static void
arv_gv_device_load_genicam (ArvGvDevice *gv_device)
{
	char *genicam;

	genicam = _load_genicam (gv_device, ARV_GVBS_FIRST_XML_URL);
	if (genicam == NULL)
		genicam = _load_genicam (gv_device, ARV_GVBS_SECOND_XML_URL);

	if (genicam != NULL)
		arv_device_set_genicam (ARV_DEVICE (gv_device), genicam);
}

/* ArvDevice implemenation */

static ArvStream *
arv_gv_device_create_stream (ArvDevice *device)
{
	ArvStream *stream;
	guint32 stream_port;

	stream = arv_gv_stream_new (0);

	stream_port = arv_gv_stream_get_port (ARV_GV_STREAM (stream));

	arv_device_write_register (device, ARV_GVBS_FIRST_STREAM_CHANNEL_PORT, stream_port);
	arv_device_read_register (device, ARV_GVBS_FIRST_STREAM_CHANNEL_PORT, &stream_port);
	g_message ("stream port = %d", stream_port);
	arv_device_write_register (device, ARV_GVBS_FIRST_STREAM_CHANNEL_IP_ADDRESS, 0x869E61B4);
	arv_device_read_register (device, ARV_GVBS_FIRST_STREAM_CHANNEL_PORT, &stream_port);
	g_message ("stream port = %d", stream_port);

	return stream;
}

static size_t
_read_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);
	ArvGvcpPacket *packet;
	GPollFD poll_fd;
	size_t packet_size;
	size_t answer_size;
	int count;

	answer_size = arv_gvcp_packet_get_read_memory_ack_size (size);

	g_return_val_if_fail (answer_size <= ARV_GV_DEVICE_BUFFER_SIZE, 0);

	gv_device->packet_count++;

	packet = arv_gvcp_packet_new_read_memory_cmd (address,
						      ((size + sizeof (guint32) - 1)
						       / sizeof (guint32)) * sizeof (guint32),
						      gv_device->packet_count, &packet_size);

	arv_gvcp_packet_debug (packet);

	g_socket_send_to (gv_device->socket, gv_device->device_address, (const char *) packet, packet_size,
			  NULL, NULL);

	arv_gvcp_packet_free (packet);

	poll_fd.fd = g_socket_get_fd (gv_device->socket);
	poll_fd.events =  G_IO_IN;
	poll_fd.revents = 0;

	if (g_poll (&poll_fd, 1, ARV_GV_DEVICE_ACKNOWLEDGE_TIMEOUT) == 0)
		return 0;

	count = g_socket_receive (gv_device->socket, gv_device->socket_buffer,
				  ARV_GV_DEVICE_BUFFER_SIZE, NULL, NULL);

	if (count >= answer_size)
		memcpy (buffer, arv_gvcp_packet_get_read_memory_ack_data (gv_device->socket_buffer),
			size);

	arv_gvcp_packet_debug ((ArvGvcpPacket *) gv_device->socket_buffer);

	return packet_size;
}

static size_t
_write_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);
	ArvGvcpPacket *packet;
	size_t packet_size;
	GPollFD poll_fd;
	int count;

	gv_device->packet_count++;

	packet = arv_gvcp_packet_new_write_memory_cmd (address,
						       ((size + sizeof (guint32) - 1) /
							sizeof (guint32)) * sizeof (guint32),
						       gv_device->packet_count, &packet_size);

	memcpy (arv_gvcp_packet_get_write_memory_cmd_data (packet), buffer, size);

	arv_gvcp_packet_debug (packet);

	g_socket_send_to (gv_device->socket, gv_device->device_address, (const char *) packet, packet_size,
			  NULL, NULL);

	arv_gvcp_packet_free (packet);

	poll_fd.fd = g_socket_get_fd (gv_device->socket);
	poll_fd.events =  G_IO_IN;
	poll_fd.revents = 0;

	if (g_poll (&poll_fd, 1, ARV_GV_DEVICE_ACKNOWLEDGE_TIMEOUT) == 0)
		return 0;

	count = g_socket_receive (gv_device->socket, gv_device->socket_buffer,
				  ARV_GV_DEVICE_BUFFER_SIZE, NULL, NULL);

	arv_gvcp_packet_debug ((ArvGvcpPacket *) gv_device->socket_buffer);

	return size;
}

gboolean
arv_gv_device_read_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	int i;
	gint32 block_size;
	size_t read_size = 0;

	for (i = 0; i < (size + ARV_GVCP_DATA_SIZE_MAX - 1) / ARV_GVCP_DATA_SIZE_MAX; i++) {
		block_size = MIN (ARV_GVCP_DATA_SIZE_MAX, size - i * ARV_GVCP_DATA_SIZE_MAX);
		read_size += _read_memory (device, address + i * ARV_GVCP_DATA_SIZE_MAX,
					   block_size, buffer + i * ARV_GVCP_DATA_SIZE_MAX);
	}

	return (size == read_size);
}

gboolean
arv_gv_device_write_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer)
{
	int i;
	gint32 block_size;
	size_t written_size = 0;

	for (i = 0; i < (size + ARV_GVCP_DATA_SIZE_MAX - 1) / ARV_GVCP_DATA_SIZE_MAX; i++) {
		block_size = MIN (ARV_GVCP_DATA_SIZE_MAX, size - i * ARV_GVCP_DATA_SIZE_MAX);
		written_size += _write_memory (device, address + i * ARV_GVCP_DATA_SIZE_MAX,
					       block_size, buffer + i * ARV_GVCP_DATA_SIZE_MAX);
	}

	return (size == written_size);
}

gboolean
arv_gv_device_read_register (ArvDevice *device, guint32 address, guint32 *value_placeholder)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);
	ArvGvcpPacket *packet;
	size_t packet_size;
	GPollFD poll_fd;
	int count;
	guint32 value;

	gv_device->packet_count++;

	packet = arv_gvcp_packet_new_read_register_cmd (address, gv_device->packet_count, &packet_size);

	arv_gvcp_packet_debug (packet);

	g_socket_send_to (gv_device->socket, gv_device->device_address, (const char *) packet, packet_size,
			  NULL, NULL);

	arv_gvcp_packet_free (packet);

	poll_fd.fd = g_socket_get_fd (gv_device->socket);
	poll_fd.events =  G_IO_IN;
	poll_fd.revents = 0;

	if (g_poll (&poll_fd, 1, ARV_GV_DEVICE_ACKNOWLEDGE_TIMEOUT) == 0) {
		*value_placeholder = 0;
		return FALSE;
	}

	count = g_socket_receive (gv_device->socket, gv_device->socket_buffer,
				  ARV_GV_DEVICE_BUFFER_SIZE, NULL, NULL);

	value = arv_gvcp_packet_get_read_register_ack_value (gv_device->socket_buffer);

	arv_gvcp_packet_debug ((ArvGvcpPacket *) gv_device->socket_buffer);

	*value_placeholder = value;

	return TRUE;
}

gboolean
arv_gv_device_write_register (ArvDevice *device, guint32 address, guint32 value)
{
	ArvGvDevice *gv_device = ARV_GV_DEVICE (device);
	ArvGvcpPacket *packet;
	size_t packet_size;
	GPollFD poll_fd;
	int count;

	gv_device->packet_count++;

	packet = arv_gvcp_packet_new_write_register_cmd (address, value, gv_device->packet_count, &packet_size);

	arv_gvcp_packet_debug (packet);

	g_socket_send_to (gv_device->socket, gv_device->device_address, (const char *) packet, packet_size,
			  NULL, NULL);

	arv_gvcp_packet_free (packet);

	poll_fd.fd = g_socket_get_fd (gv_device->socket);
	poll_fd.events =  G_IO_IN;
	poll_fd.revents = 0;

	if (g_poll (&poll_fd, 1, ARV_GV_DEVICE_ACKNOWLEDGE_TIMEOUT) == 0)
		return FALSE;

	count = g_socket_receive (gv_device->socket, gv_device->socket_buffer,
				  ARV_GV_DEVICE_BUFFER_SIZE, NULL, NULL);

	arv_gvcp_packet_debug ((ArvGvcpPacket *) gv_device->socket_buffer);

	return TRUE;
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

	arv_gv_device_load_genicam (gv_device);

	arv_gv_device_take_control (gv_device);

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

	arv_gv_device_leave_control (gv_device);

	g_object_unref (gv_device->device_address);
	g_object_unref (gv_device->control_address);
	g_object_unref (gv_device->socket);

	g_free (gv_device->socket_buffer);

	parent_class->finalize (object);
}

static void
arv_gv_device_class_init (ArvGvDeviceClass *gv_device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (gv_device_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (gv_device_class);

	parent_class = g_type_class_peek_parent (gv_device_class);

	object_class->finalize = arv_gv_device_finalize;

	device_class->create_stream = arv_gv_device_create_stream;
	device_class->read_memory = arv_gv_device_read_memory;
	device_class->write_memory = arv_gv_device_write_memory;
	device_class->read_register = arv_gv_device_read_register;
	device_class->write_register = arv_gv_device_write_register;

	arv_gv_device_url_regex = g_regex_new ("^(local:|file:)(.+\\.xml);?([0-9:a-f]*)?;?([0-9:a-f]*)?$",
					       G_REGEX_CASELESS, 0, NULL);
}

G_DEFINE_TYPE (ArvGvDevice, arv_gv_device, ARV_TYPE_DEVICE)
