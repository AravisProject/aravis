#include <arvgvcp.h>
#include <arvenumtypes.h>
#include <string.h>
#include <arvdebug.h>

void
arv_gvcp_packet_free (ArvGvcpPacket *packet)
{
	g_free (packet);
}

ArvGvcpPacket *
arv_gvcp_packet_new_read_memory_cmd (guint32 address, guint32 size, guint32 packet_count, size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 n_address = g_htonl (address);
	guint32 n_size = g_htonl (size);

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + 2 * sizeof (guint32);

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_CMD);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_READ_MEMORY_CMD);
	packet->header.size = g_htons (2 * sizeof (guint32));
	packet->header.count = g_htons (packet_count);

	memcpy (&packet->data, &n_address, sizeof (guint32));
	memcpy (&packet->data[sizeof(guint32)], &n_size, sizeof (guint32));

	return packet;
}

ArvGvcpPacket *
arv_gvcp_packet_new_write_memory_cmd (guint32 address, guint32 size, guint32 packet_count, size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 n_address = g_htonl (address);

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + sizeof (guint32) + size;

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_CMD);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_WRITE_MEMORY_CMD);
	packet->header.size = g_htons (sizeof (guint32) + size);
	packet->header.count = g_htons (packet_count);

	memcpy (&packet->data, &n_address, sizeof (guint32));

	return packet;
}

ArvGvcpPacket *
arv_gvcp_packet_new_read_register_cmd (guint32 address, guint32 packet_count, size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 n_address = g_htonl (address);

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + sizeof (guint32);

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_CMD);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_READ_REGISTER_CMD);
	packet->header.size = g_htons (sizeof (guint32));
	packet->header.count = g_htons (packet_count);

	memcpy (&packet->data, &n_address, sizeof (guint32));

	return packet;
}

ArvGvcpPacket *
arv_gvcp_packet_new_write_register_cmd (guint32 address, guint32 value,
					guint32 packet_count, size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 n_address = g_htonl (address);
	guint32 n_value = g_htonl (value);

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + 2 * sizeof (guint32);

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_CMD);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_WRITE_REGISTER_CMD);
	packet->header.size = g_htons (2 * sizeof (guint32));
	packet->header.count = g_htons (packet_count);

	memcpy (&packet->data, &n_address, sizeof (guint32));
	memcpy (&packet->data[sizeof (guint32)], &n_value, sizeof (guint32));

	return packet;
}

ArvGvcpPacket *
arv_gvcp_packet_new_discovery_cmd (size_t *packet_size)
{
	ArvGvcpPacket *packet;

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader);

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_CMD);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_DISCOVERY_CMD);
	packet->header.size = g_htons (0x0000);
	packet->header.count = g_htons (0xffff);

	return packet;
}

ArvGvcpPacket *
arv_gvcp_packet_new_packet_resend_cmd (guint32 frame_id,
				       guint32 first_block, guint32 last_block,
				       guint32 packet_count, size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 *data;

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + 3 * sizeof (guint32);

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_RESEND);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_PACKET_RESEND_CMD);
	packet->header.size = g_htons (3 * sizeof (guint32));
	packet->header.count = g_htons (packet_count);

	data = (guint32 *) &packet->data;

	data[0] = g_htonl (frame_id);
	data[1] = g_htonl (first_block);
	data[2] = g_htonl (last_block);

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

	g_string_append_printf (string, "packet_type  = %s\n",
				arv_gvcp_packet_type_to_string (g_ntohs (packet->header.packet_type)));
	g_string_append_printf (string, "command      = %s\n",
				arv_gvcp_command_to_string (g_ntohs (packet->header.command)));
	g_string_append_printf (string, "size         = %d\n", g_ntohs (packet->header.size));
	g_string_append_printf (string, "count        = %d\n", g_ntohs (packet->header.count));

	data = (char *) &packet->data;

	switch (g_ntohs (packet->header.command)) {
		case ARV_GVCP_COMMAND_DISCOVERY_CMD:
			break;
		case ARV_GVCP_COMMAND_DISCOVERY_ACK:
			g_string_append_printf (string, "manufacturer = %s\n",
						&data[ARV_GVBS_MANUFACTURER_NAME]);
			g_string_append_printf (string, "name         = %s\n",
						&data[ARV_GVBS_USER_DEFINED_NAME]);
			g_string_append_printf (string, "model        = %s\n",
						&data[ARV_GVBS_MODEL_NAME]);
			g_string_append_printf (string, "address      = %d.%d.%d.%d\n",
						data[ARV_GVBS_CURRENT_IP_ADDRESS] & 0xff,
						data[ARV_GVBS_CURRENT_IP_ADDRESS + 1] & 0xff,
						data[ARV_GVBS_CURRENT_IP_ADDRESS + 2] & 0xff,
						data[ARV_GVBS_CURRENT_IP_ADDRESS + 3] & 0xff);
			break;
		case ARV_GVCP_COMMAND_WRITE_REGISTER_CMD:
			value = g_ntohl (*((guint32 *) &data[0]));
			g_string_append_printf (string, "address      = %10u (0x%08x)\n",
						value, value);
			value = g_ntohl (*((guint32 *) &data[4]));
			g_string_append_printf (string, "value        = %10u (0x%08x)\n",
						value, value);
			break;
		case ARV_GVCP_COMMAND_READ_REGISTER_CMD:
			value = g_ntohl (*((guint32 *) &data[0]));
			g_string_append_printf (string, "address      = %10u (0x%08x)\n",
						value, value);
			break;
		case ARV_GVCP_COMMAND_READ_REGISTER_ACK:
			value = g_ntohl (*((guint32 *) &data[0]));
			g_string_append_printf (string, "success      = %10u (0x%08x)\n",
						value, value);
			break;
		case ARV_GVCP_COMMAND_READ_MEMORY_CMD:
			value = g_ntohl (*((guint32 *) &data[0]));
			g_string_append_printf (string, "address      = %10u (0x%08x)\n",
						value, value);
			value = g_ntohl (*((guint32 *) &data[4]));
			g_string_append_printf (string, "size         = %10u (0x%08x)\n",
						value, value);
			break;
		case ARV_GVCP_COMMAND_READ_MEMORY_ACK:
			value = g_ntohl (*((guint32 *) &data[0]));
			g_string_append_printf (string, "address      = %10u (0x%08x)\n",
						value, value);
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
				if (*((char *) ((void *) packet) + index) >= ' ' &&
				    *((char *) ((void *) packet) + index) <  '\x7f')
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
arv_gvcp_packet_debug (const ArvGvcpPacket *packet)
{
	char *string;

	if (!arv_debug_check (ARV_DEBUG_LEVEL_GVCP))
		return;

	string = arv_gvcp_packet_to_string (packet);
	arv_debug (ARV_DEBUG_LEVEL_GVCP, "%s", string);
	g_free (string);
}
