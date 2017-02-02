/* Aravis - Digital camera library
 *
 * Copyright © 2009-2016 Emmanuel Pacaud
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
 * SECTION: arvuvcp
 * @short_description: USB3Vision control packet handling
 */

#include <arvuvcp.h>
#include <arvenumtypes.h>
#include <arvdebug.h>
#include <arvstr.h>
#include <string.h>

void
arv_uvcp_packet_free (ArvUvcpPacket *packet)
{
	g_free (packet);
}

/**
 * arv_uvcp_packet_new_read_memory_cmd: (skip)
 * @address: read address
 * @size: read size, in bytes
 * @packet_id: packet id
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #ArvUvcpPacket
 *
 * Create a uvcp packet for a memory read command.
 */

ArvUvcpPacket *
arv_uvcp_packet_new_read_memory_cmd (guint64 address, guint32 size, guint16 packet_id, size_t *packet_size)
{
	ArvUvcpReadMemoryCmd *packet;

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvUvcpReadMemoryCmd);

	packet = g_malloc (*packet_size);

	packet->header.magic = GUINT32_TO_LE (ARV_UVCP_MAGIC);
	packet->header.packet_type = GUINT16_TO_LE (ARV_UVCP_PACKET_TYPE_CMD);
	packet->header.command = GUINT16_TO_LE (ARV_UVCP_COMMAND_READ_MEMORY_CMD);
	packet->header.size = GUINT16_TO_LE (sizeof (ArvUvcpReadMemoryCmdInfos));
	packet->header.id = GUINT16_TO_LE (packet_id);
	packet->infos.address = GUINT64_TO_LE (address);
	packet->infos.size = GUINT16_TO_LE (size);

	return (ArvUvcpPacket *) packet;
}

/**
 * arv_uvcp_packet_new_write_memory_cmd: (skip)
 * @address: write address
 * @size: write size, in bytes
 * @packet_id: packet id
 * @packet_size: (out): packet size, in bytes
 * Return value: (transfer full): a new #ArvUvcpPacket
 *
 * Create a uvcp packet for a memory write command.
 */

ArvUvcpPacket *
arv_uvcp_packet_new_write_memory_cmd (guint64 address, guint32 size, guint16 packet_id, size_t *packet_size)
{
	ArvUvcpWriteMemoryCmd *packet;

	g_return_val_if_fail (packet_size != NULL, NULL);

	*packet_size = sizeof (ArvUvcpWriteMemoryCmd) + size;

	packet = g_malloc (*packet_size);

	packet->header.magic = GUINT32_TO_LE (ARV_UVCP_MAGIC);
	packet->header.packet_type = GUINT16_TO_LE (ARV_UVCP_PACKET_TYPE_CMD);
	packet->header.command = GUINT16_TO_LE (ARV_UVCP_COMMAND_WRITE_MEMORY_CMD);
	packet->header.size = GUINT16_TO_LE (sizeof (ArvUvcpWriteMemoryCmdInfos) + size);
	packet->header.id = GUINT16_TO_LE (packet_id);
	packet->infos.address = GUINT64_TO_LE (address);

	return (ArvUvcpPacket *) packet;
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

const char *
arv_uvcp_packet_type_to_string (ArvUvcpPacketType value)
{
	return arv_enum_to_string (ARV_TYPE_UVCP_PACKET_TYPE, value);
}

const char *
arv_uvcp_command_to_string (ArvUvcpCommand value)
{
	return arv_enum_to_string (ARV_TYPE_UVCP_COMMAND, value);
}

/**
 * arv_uvcp_packet_to_string:
 * @packet: a #ArvUvcpPacket
 *
 * Converts @packet into a human readable string.
 *
 * return value: (transfer full): A newly allocated string.
 */

char *
arv_uvcp_packet_to_string (const ArvUvcpPacket *packet)
{
	GString *string;
	char *c_string;
	int packet_size;
	guint64 value;

	g_return_val_if_fail (packet != NULL, NULL);

	string = g_string_new ("");

	g_string_append_printf (string, "packet_type  = %s\n",
				arv_uvcp_packet_type_to_string (GUINT16_FROM_LE (packet->header.packet_type)));
	g_string_append_printf (string, "command      = %s\n",
				arv_uvcp_command_to_string (GUINT16_FROM_LE (packet->header.command)));
	g_string_append_printf (string, "size         = %d\n", GUINT16_FROM_LE (packet->header.size));
	g_string_append_printf (string, "id           = %d\n", GUINT16_FROM_LE (packet->header.id));

	switch (GUINT16_FROM_LE (packet->header.command)) {
		case ARV_UVCP_COMMAND_READ_MEMORY_CMD:
			{
				ArvUvcpReadMemoryCmd *cmd_packet = (void *) packet;

				value = GUINT64_FROM_LE (cmd_packet->infos.address);
				g_string_append_printf (string, "address      = 0x%016lx\n", value);
				value = GUINT16_FROM_LE (cmd_packet->infos.size);
				g_string_append_printf (string, "size         = %10lu (0x%08lx)\n",
							value, value);
				break;
			}
		case ARV_UVCP_COMMAND_READ_MEMORY_ACK:
			{
				break;
			}
		case ARV_UVCP_COMMAND_WRITE_MEMORY_CMD:
			{
				ArvUvcpWriteMemoryCmd *cmd_packet = (void *) packet;

				value = GUINT64_FROM_LE (cmd_packet->infos.address);
				g_string_append_printf (string, "address      = 0x%016lx\n", value);
				break;
			}
		case ARV_UVCP_COMMAND_WRITE_MEMORY_ACK:
			{
				ArvUvcpWriteMemoryAck *cmd_packet = (void *) packet;

				value = GUINT64_FROM_LE (cmd_packet->infos.bytes_written);
				g_string_append_printf (string, "written      = %10lu (0x%08lx)\n",
							value, value);
				break;
			}
	}

	packet_size = sizeof (ArvUvcpHeader) + GUINT16_FROM_LE (packet->header.size);

	arv_g_string_append_hex_dump (string, packet, packet_size);

	c_string = string->str;

	g_string_free (string, FALSE);

	return c_string;
}

/**
 * arv_uvcp_packet_debug:
 * @packet: a #ArvUvcpPacket
 * @level: debug level
 *
 * Dumps the content of @packet if level is lower or equal to the current debug level for the cp debug category. See arv_debug_enable().
 */

void
arv_uvcp_packet_debug (const ArvUvcpPacket *packet, ArvDebugLevel level)
{
	char *string;

	if (!arv_debug_check (&arv_debug_category_cp, level))
		return;

	string = arv_uvcp_packet_to_string (packet);
	switch (level) {
		case ARV_DEBUG_LEVEL_LOG:
			arv_log_cp ("%s", string);
			break;
		case ARV_DEBUG_LEVEL_DEBUG:
			arv_debug_cp ("%s", string);
			break;
		case ARV_DEBUG_LEVEL_WARNING:
			arv_warning_cp ("%s", string);
			break;
		default:
			break;
	}
	g_free (string);
}
