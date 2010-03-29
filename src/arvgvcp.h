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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_GVCP_H
#define ARV_GVCP_H

#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_GVCP_PORT	3956

#define ARV_GVBS_CURRENT_IP_ADDRESS			0x00000024
#define ARV_GVBS_MANUFACTURER_NAME			0x00000048
#define ARV_GVBS_MANUFACTURER_NAME_SIZE			32
#define ARV_GVBS_MODEL_NAME				0x00000068
#define ARV_GVBS_MODEL_NAME_SIZE			32
#define ARV_GVBS_DEVICE_VERSION 			0x00000088
#define ARV_GVBS_DEVICE_VERSION_SIZE			32
#define ARV_GVBS_USER_DEFINED_NAME			0x000000e8
#define ARV_GVBS_USER_DEFINED_NAME_SIZE			16
#define ARV_GVBS_SERIAL_NUMBER				0x000000d8
#define ARV_GVBS_SERIAL_NUMBER_SIZE			16

#define ARV_GVBS_DISCOVERY_DATA_SIZE			0xf8

#define ARV_GVBS_FIRST_XML_URL				0x00000200
#define ARV_GVBS_SECOND_XML_URL				0x00000400
#define ARV_GVBS_XML_URL_SIZE				512

#define ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE		0x00000a00
#define ARV_GVBS_FIRST_STREAM_CHANNEL_PORT		0x00000d00
#define ARV_GVBS_FIRST_STREAM_CHANNEL_PACKET_SIZE	0x00000d04
#define ARV_GVBS_FIRST_STREAM_CHANNEL_IP_ADDRESS	0x00000d18

#define ARV_GVCP_DATA_SIZE_MAX				512

typedef enum {
	ARV_GVCP_PACKET_TYPE_ACK =		0x0000,
	ARV_GVCP_PACKET_TYPE_RESEND =		0x4200,
	ARV_GVCP_PACKET_TYPE_CMD = 		0x4201,
	ARV_GVCP_PACKET_TYPE_ERROR =		0x8006
} ArvGvcpPacketType;

typedef enum {
	ARV_GVCP_COMMAND_DISCOVERY_CMD =	0x0002,
	ARV_GVCP_COMMAND_DISCOVERY_ACK =	0x0003,
	ARV_GVCP_COMMAND_BYE_CMD = 		0x0004,
	ARV_GVCP_COMMAND_BYE_ACK = 		0x0005,
	ARV_GVCP_COMMAND_PACKET_RESEND_CMD =	0x0040,
	ARV_GVCP_COMMAND_PACKET_RESEND_ACK =	0x0041,
	ARV_GVCP_COMMAND_READ_REGISTER_CMD =	0x0080,
	ARV_GVCP_COMMAND_READ_REGISTER_ACK =	0x0081,
	ARV_GVCP_COMMAND_WRITE_REGISTER_CMD =	0x0082,
	ARV_GVCP_COMMAND_WRITE_REGISTER_ACK =	0x0083,
	ARV_GVCP_COMMAND_READ_MEMORY_CMD =	0x0084,
	ARV_GVCP_COMMAND_READ_MEMORY_ACK =	0x0085,
	ARV_GVCP_COMMAND_WRITE_MEMORY_CMD =	0x0086,
	ARV_GVCP_COMMAND_WRITE_MEMORY_ACK =	0x0087
} ArvGvcpCommand;

typedef struct {
	guint16 packet_type;
	guint16 command;
	guint16 size;
	guint16 count;
}  __attribute__((__packed__)) ArvGvcpHeader;

typedef struct {
	ArvGvcpHeader header;
	unsigned char data[];
} ArvGvcpPacket;

void 			arv_gvcp_packet_free 			(ArvGvcpPacket *packet);
ArvGvcpPacket * 	arv_gvcp_packet_new_read_memory_cmd 	(guint32 address, guint32 size,
								 guint32 packet_count, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_write_memory_cmd	(guint32 address, guint32 size,
								 guint32 packet_count, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_read_register_cmd 	(guint32 address,
								 guint32 packet_count, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_write_register_cmd 	(guint32 address, guint32 value,
								 guint32 packet_count, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_discovery_cmd 	(size_t *size);
ArvGvcpPacket * 	arv_gvcp_packet_new_packet_resend_cmd 	(guint32 frame_id,
								 guint32 first_block, guint32 last_block,
								 guint32 packet_count, size_t *packet_size);
char * 			arv_gvcp_packet_to_string 		(const ArvGvcpPacket *packet);
void 			arv_gvcp_packet_debug 			(const ArvGvcpPacket *packet);

static inline void
arv_gvcp_packet_set_packet_count (ArvGvcpPacket *packet, guint16 count)
{
	if (packet != NULL)
		packet->header.count = g_htons (count);
}

static inline guint16
arv_gvcp_packet_get_packet_count (ArvGvcpPacket *packet)
{
	if (packet == NULL)
		return 0;

	return g_ntohs (packet->header.count);
}

static inline size_t
arv_gvcp_packet_get_read_memory_ack_size (guint32 data_size)
{
	return sizeof (ArvGvcpHeader) + sizeof (guint32) + data_size;
}

static inline void *
arv_gvcp_packet_get_read_memory_ack_data (const ArvGvcpPacket *packet)
{
	return (void *) packet + sizeof (ArvGvcpHeader) + sizeof (guint32);
}

static inline void *
arv_gvcp_packet_get_write_memory_cmd_data (const ArvGvcpPacket *packet)
{
	return (void *) packet + sizeof (ArvGvcpPacket) + sizeof (guint32);
}

static inline size_t
arv_gvcp_packet_get_write_memory_ack_size (void)
{
	return sizeof (ArvGvcpPacket) + sizeof (guint32);
}

static inline guint32
arv_gvcp_packet_get_read_register_ack_value (const ArvGvcpPacket *packet)
{
	return g_ntohl (*((guint32 *) ((void *) packet + sizeof (ArvGvcpPacket))));
}

G_END_DECLS

#endif
