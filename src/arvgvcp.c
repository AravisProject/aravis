/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvgvcp
 * @short_description: Gvcp packet handling (control)
 */

#include <arvgvcp.h>
#include <arvenumtypes.h>
#include <string.h>
#include <arvdebug.h>
#include <arvstr.h>

void
arv_gvcp_packet_free (ArvGvcpPacket *packet)
{
	g_free (packet);
}

/**
 * arv_gvcp_packet_new_read_memory_cmd: (skip)
 * @address: read address
 * @size: read size, in bytes
 * @packet_id: packet id
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #ArvGvcpPacket
 *
 * Create a gvcp packet for a memory read command.
 */

ArvGvcpPacket *
arv_gvcp_packet_new_read_memory_cmd (guint32 address, guint32 size, guint16 packet_id, size_t *packet_size)
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
	packet->header.id = g_htons (packet_id);

	memcpy (&packet->data, &n_address, sizeof (guint32));
	memcpy (&packet->data[sizeof(guint32)], &n_size, sizeof (guint32));

	return packet;
}

/**
 * arv_gvcp_packet_new_read_memory_ack: (skip)
 * @address: read address
 * @size: read size, in bytes
 * @packet_id: packet id
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #ArvGvcpPacket
 *
 * Create a gvcp packet for a memory read acknowledge.
 */

ArvGvcpPacket *
arv_gvcp_packet_new_read_memory_ack (guint32 address, guint32 size, guint16 packet_id, size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 n_address = g_htonl (address);

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + sizeof (guint32) + size;

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_ACK);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_READ_MEMORY_ACK);
	packet->header.size = g_htons (sizeof (guint32) + size);
	packet->header.id = g_htons (packet_id);

	memcpy (&packet->data, &n_address, sizeof (guint32));

	return packet;
}

/**
 * arv_gvcp_packet_new_write_memory_cmd: (skip)
 * @address: write address
 * @size: write size, in bytes
 * @packet_id: packet id
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #ArvGvcpPacket
 *
 * Create a gvcp packet for a memory write command.
 */

ArvGvcpPacket *
arv_gvcp_packet_new_write_memory_cmd (guint32 address, guint32 size, guint16 packet_id, size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 n_address = g_htonl (address);

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + sizeof (guint32) + size;

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_CMD);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_WRITE_MEMORY_CMD);
	packet->header.size = g_htons (sizeof (guint32) + size);
	packet->header.id = g_htons (packet_id);

	memcpy (&packet->data, &n_address, sizeof (guint32));

	return packet;
}

/**
 * arv_gvcp_packet_new_write_memory_ack: (skip)
 * @address: write address
 * @packet_id: packet id
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #ArvGvcpPacket
 *
 * Create a gvcp packet for a memory write acknowledge.
 */

ArvGvcpPacket *
arv_gvcp_packet_new_write_memory_ack (guint32 address,
				      guint16 packet_id,
				      size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 n_address = g_htonl (address);

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + sizeof (guint32);

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_ACK);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_WRITE_MEMORY_ACK);
	packet->header.size = g_htons (sizeof (guint32));
	packet->header.id = g_htons (packet_id);

	memcpy (&packet->data, &n_address, sizeof (guint32));

	return packet;
}

/**
 * arv_gvcp_packet_new_read_register_cmd: (skip)
 * @address: write address
 * @packet_id: packet id
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #ArvGvcpPacket
 *
 * Create a gvcp packet for a register read command.
 */

ArvGvcpPacket *
arv_gvcp_packet_new_read_register_cmd (guint32 address,
				       guint16 packet_id,
				       size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 n_address = g_htonl (address);

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + sizeof (guint32);

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_CMD);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_READ_REGISTER_CMD);
	packet->header.size = g_htons (sizeof (guint32));
	packet->header.id = g_htons (packet_id);

	memcpy (&packet->data, &n_address, sizeof (guint32));

	return packet;
}

/**
 * arv_gvcp_packet_new_read_register_ack: (skip)
 * @value: read value
 * @packet_id: packet id
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #ArvGvcpPacket
 *
 * Create a gvcp packet for a register read acknowledge.
 */

ArvGvcpPacket *
arv_gvcp_packet_new_read_register_ack (guint32 value,
				       guint16 packet_id,
				       size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 n_value = g_htonl (value);

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + sizeof (guint32);

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_ACK);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_READ_REGISTER_ACK);
	packet->header.size = g_htons (sizeof (guint32));
	packet->header.id = g_htons (packet_id);

	memcpy (&packet->data, &n_value, sizeof (guint32));

	return packet;
}

/**
 * arv_gvcp_packet_new_write_register_cmd: (skip)
 * @address: write address
 * @value: value to write
 * @packet_id: packet id
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #ArvGvcpPacket
 *
 * Create a gvcp packet for a register write command.
 */

ArvGvcpPacket *
arv_gvcp_packet_new_write_register_cmd (guint32 address,
					guint32 value,
					guint16 packet_id,
					size_t *packet_size)
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
	packet->header.id = g_htons (packet_id);

	memcpy (&packet->data, &n_address, sizeof (guint32));
	memcpy (&packet->data[sizeof (guint32)], &n_value, sizeof (guint32));

	return packet;
}

/**
 * arv_gvcp_packet_new_write_register_ack: (skip)
 * @address: write address
 * @packet_id: packet id
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #ArvGvcpPacket
 *
 * Create a gvcp packet for a register write acknowledge.
 */

ArvGvcpPacket *
arv_gvcp_packet_new_write_register_ack 	(guint32 address,
					 guint16 packet_id,
					 size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 n_address = g_htonl (address);

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + sizeof (guint32);

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_ACK);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_WRITE_REGISTER_ACK);
	packet->header.size = g_htons (sizeof (guint32));
	packet->header.id = g_htons (packet_id);

	memcpy (&packet->data, &n_address, sizeof (guint32));

	return packet;
}

/**
 * arv_gvcp_packet_new_discovery_cmd: (skip)
 * @size: (out): packet size, in bytes
 * Return value: (transfer full): a new #ArvGvcpPacket
 *
 * Create a gvcp packet for a discovery command.
 */

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
	packet->header.id = g_htons (0xffff);

	return packet;
}

/**
 * arv_gvcp_packet_new_discovery_ack: (skip)
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #ArvGvcpPacket
 *
 * Create a gvcp packet for a discovery acknowledge.
 */

ArvGvcpPacket *
arv_gvcp_packet_new_discovery_ack (size_t *packet_size)
{
	ArvGvcpPacket *packet;

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + ARV_GVBS_DISCOVERY_DATA_SIZE ;

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_ACK);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_DISCOVERY_ACK);
	packet->header.size = g_htons (ARV_GVBS_DISCOVERY_DATA_SIZE);
	packet->header.id = g_htons (0xffff);

	return packet;
}

/**
 * arv_gvcp_packet_new_packet_resend_cmd: (skip)
 * @frame_id: frame id
 * @first_block: first missing packet
 * @last_block: last missing packet
 * @packet_id: packet id
 * @packet_size: (out): packet size, in bytes
 *
 * Create a gvcp packet for a packet resend command.
 *
 * Return value: (transfer full): a new #ArvGvcpPacket
 */

ArvGvcpPacket *
arv_gvcp_packet_new_packet_resend_cmd (guint32 frame_id,
				       guint32 first_block, guint32 last_block,
				       guint16 packet_id, size_t *packet_size)
{
	ArvGvcpPacket *packet;
	guint32 *data;

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvGvcpHeader) + 3 * sizeof (guint32);

	packet = g_malloc (*packet_size);

	packet->header.packet_type = g_htons (ARV_GVCP_PACKET_TYPE_RESEND);
	packet->header.command = g_htons (ARV_GVCP_COMMAND_PACKET_RESEND_CMD);
	packet->header.size = g_htons (3 * sizeof (guint32));
	packet->header.id = g_htons (packet_id);

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

/**
 * arv_gvcp_packet_to_string:
 * @packet: a #ArvGvcpPacket
 *
 * Converts @packet into a human readable string.
 *
 * return value: (transfer full): A newly allocated string.
 */

char *
arv_gvcp_packet_to_string (const ArvGvcpPacket *packet)
{
	GString *string;
	char *c_string;
	char *data;
	int packet_size;
	guint32 value;

	g_return_val_if_fail (packet != NULL, NULL);

	string = g_string_new ("");

	g_string_append_printf (string, "packet_type  = %s\n",
				arv_gvcp_packet_type_to_string (g_ntohs (packet->header.packet_type)));
	g_string_append_printf (string, "command      = %s\n",
				arv_gvcp_command_to_string (g_ntohs (packet->header.command)));
	g_string_append_printf (string, "size         = %d\n", g_ntohs (packet->header.size));
	g_string_append_printf (string, "id           = %d\n", g_ntohs (packet->header.id));

	data = (char *) &packet->data;

	switch (g_ntohs (packet->header.command)) {
		case ARV_GVCP_COMMAND_DISCOVERY_CMD:
			break;
		case ARV_GVCP_COMMAND_DISCOVERY_ACK:
			g_string_append_printf (string, "manufacturer = %s\n",
						&data[ARV_GVBS_MANUFACTURER_NAME_OFFSET]);
			g_string_append_printf (string, "name         = %s\n",
						&data[ARV_GVBS_USER_DEFINED_NAME_OFFSET]);
			g_string_append_printf (string, "model        = %s\n",
						&data[ARV_GVBS_MODEL_NAME_OFFSET]);
			g_string_append_printf (string, "address      = %d.%d.%d.%d\n",
						data[ARV_GVBS_CURRENT_IP_ADDRESS_OFFSET] & 0xff,
						data[ARV_GVBS_CURRENT_IP_ADDRESS_OFFSET + 1] & 0xff,
						data[ARV_GVBS_CURRENT_IP_ADDRESS_OFFSET + 2] & 0xff,
						data[ARV_GVBS_CURRENT_IP_ADDRESS_OFFSET + 3] & 0xff);
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

	arv_g_string_append_hex_dump (string, packet, packet_size);

	c_string = string->str;

	g_string_free (string, FALSE);

	return c_string;
}

/**
 * arv_gvcp_packet_debug:
 * @packet: a #ArvGvcpPacket
 * @level: debug level
 *
 * Dumps the content of @packet if level is lower or equal to the current debug level for the gvcp debug category. See arv_debug_enable().
 */

void
arv_gvcp_packet_debug (const ArvGvcpPacket *packet, ArvDebugLevel level)
{
	char *string;

	if (!arv_debug_check (&arv_debug_category_gvcp, level))
		return;

	string = arv_gvcp_packet_to_string (packet);
	switch (level) {
		case ARV_DEBUG_LEVEL_LOG:
			arv_log_gvcp ("%s", string);
			break;
		case ARV_DEBUG_LEVEL_DEBUG:
			arv_debug_gvcp ("%s", string);
			break;
		case ARV_DEBUG_LEVEL_WARNING:
			arv_warning_gvcp ("%s", string);
			break;
		default:
			break;
	}
	g_free (string);
}
