#include <arvgvcp.h>
#include <arvenumtypes.h>
#include <string.h>

void
arv_gvcp_packet_free (ArvGvcpPacket *packet)
{
	g_free (packet);
}

ArvGvcpPacket *
arv_gvcp_read_packet_new (guint32 address, guint32 size, guint32 packet_count, size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 n_address = g_htonl (address);
	guint32 n_size = g_htonl (size);

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + 2 * sizeof (guint32);

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_COMMAND);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_READ_CMD);
	packet->header.size = g_htons (2 * sizeof (guint32));
	packet->header.count = g_htons (packet_count);

	memcpy (&packet->data, &n_address, sizeof (guint32));
	memcpy (&packet->data[sizeof(guint32)], &n_size, sizeof (guint32));

	return packet;
}

ArvGvcpPacket *
arv_gvcp_read_register_packet_new (guint32 address, guint32 packet_count, size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 n_address = g_htonl (address);

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + sizeof (guint32);

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_COMMAND);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_READ_REGISTER_CMD);
	packet->header.size = g_htons (sizeof (guint32));
	packet->header.count = g_htons (packet_count);

	memcpy (&packet->data, &n_address, sizeof (guint32));

	return packet;
}

ArvGvcpPacket *
arv_gvcp_discover_packet_new (size_t *packet_size)
{
	ArvGvcpPacket *packet;

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader);

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_COMMAND);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_DISCOVER_CMD);
	packet->header.size = g_htons (0x0000);
	packet->header.count = g_htons (0xffff);

	return packet;
}

static const char *
arv_enum_to_string (GType type,
		    guint enum_value)
{
	GEnumClass *enum_class;
	GEnumValue *value;
	const char *retval = NULL;

	enum_class = g_type_class_ref (type);

	value = g_enum_get_value (enum_class, enum_value);
	if (value)
		retval = value->value_nick;

	g_type_class_unref (enum_class);

	return retval;
}

static const char *
arv_gvcp_packet_type_to_string (ArvGvcpPacketType value)
{
	return arv_enum_to_string (ARV_TYPE_GVCP_PACKET_TYPE, value);
}

static const char *
arv_gvcp_command_to_string (ArvGvcpCommand value)
{
	return arv_enum_to_string (ARV_TYPE_GVCP_COMMAND, value);
}

char *
arv_gvcp_packet_to_string (const ArvGvcpPacket *packet)
{
	GString *string;
	char *c_string;
	char *data;
	int i, j, packet_size, index;
	guint32 value;

	g_return_val_if_fail (packet != NULL, NULL);

	string = g_string_new ("");

	g_string_append_printf (string, "packet_type = %s\n",
				arv_gvcp_packet_type_to_string (g_ntohs (packet->header.packet_type)));
	g_string_append_printf (string, "command     = %s\n",
				arv_gvcp_command_to_string (g_ntohs (packet->header.command)));
	g_string_append_printf (string, "size        = %d\n", g_ntohs (packet->header.size));
	g_string_append_printf (string, "count       = %d\n", g_ntohs (packet->header.count));

	data = (char *) &packet->data;

	switch (g_ntohs (packet->header.command)) {
		case ARV_GVCP_COMMAND_DISCOVER_CMD:
			break;
		case ARV_GVCP_COMMAND_DISCOVER_ANS:
			g_string_append_printf (string, "supplier    = %s\n",
						&data[ARV_GVCP_SUPPLIER_NAME_ADDRESS]);
			g_string_append_printf (string, "name        = %s\n",
						&data[ARV_GVCP_CAMERA_NAME_ADDRESS]);
			g_string_append_printf (string, "model       = %s\n",
						&data[ARV_GVCP_MODEL_NAME_ADDRESS]);
			g_string_append_printf (string, "address     = %d.%d.%d.%d\n",
						data[ARV_GVCP_IP_ADDRESS] & 0xff,
						data[ARV_GVCP_IP_ADDRESS + 1] & 0xff,
						data[ARV_GVCP_IP_ADDRESS + 2] & 0xff,
						data[ARV_GVCP_IP_ADDRESS + 3] & 0xff);
			break;
		case ARV_GVCP_COMMAND_READ_REGISTER_CMD:
			value = g_ntohl (*((guint32 *) &data[0]));
			g_string_append_printf (string, "address     = %10d (0x%08x)\n",
						value, value);
			break;
		case ARV_GVCP_COMMAND_READ_REGISTER_ANS:
			break;
		case ARV_GVCP_COMMAND_READ_CMD:
			value = g_ntohl (*((guint32 *) &data[0]));
			g_string_append_printf (string, "size        = %10d (0x%08x)\n",
						value, value);
			value = g_ntohl (*((guint32 *) &data[4]));
			g_string_append_printf (string, "address     = %10d (0x%08x)\n",
						value, value);
			break;
		case ARV_GVCP_COMMAND_READ_ANS:
			g_string_append_printf (string, "address     = %s\n", &data[0]);
			break;
	}

	packet_size = sizeof (ArvGvcpHeader) + g_ntohs (packet->header.size); 
	for (i = 0; i < (packet_size + 15) / 16; i++) {
		for (j = 0; j < 16; j++) {
			index = i * 16 + j;
			if (j == 0)
				g_string_append_printf (string, "%04x", i * 16);
			if (index < packet_size)
				g_string_append_printf (string, " %02x", *((guint8 *) ((void *) packet) + index));
			else
				g_string_append (string, "   ");
		}
		for (j = 0; j < 16; j++) {
			index = i * 16 + j;
			if (j == 0)
				g_string_append (string, "  ");
			if (index < packet_size)
				if (*((char *) ((void *) packet) + index) >= ' ')
					g_string_append_c (string, *((char *) ((void *) packet) + index));
				else g_string_append_c (string, '.');
			else
				g_string_append_c (string, ' ');
		}
		if (index < packet_size)
			g_string_append (string, "\n");
	}

	c_string = string->str;

	g_string_free (string, FALSE);

	return c_string;
}

void
arv_gvcp_packet_dump (const ArvGvcpPacket *packet)
{
	char *string;

	string = arv_gvcp_packet_to_string (packet);
	g_message ("%s", string);
	g_free (string);
}
