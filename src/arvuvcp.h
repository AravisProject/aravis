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

#ifndef ARV_UVCP_H
#define ARV_UVCP_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvtypes.h>
#include <arvdebug.h>

G_BEGIN_DECLS

#define ARV_UVCP_MAGIC	0x43563355

#define ARV_UVCP_DEFAULT_RESPONSE_TIME_MS		5

#define ARV_ABRM_GENCP_VERSION			0x0000
#define ARV_ABRM_MANUFACTURER_NAME		0x0004
#define ARV_ABRM_MODEL_NAME			0x0044
#define ARV_ABRM_FAMILY_NAME			0x0084
#define ARV_ABRM_DEVICE_VERSION			0x00c4
#define ARV_ABRM_MANUFACTURER_INFO		0x0104
#define ARV_ABRM_SERIAL_NUMBER			0x0144
#define ARV_ABRM_USER_DEFINED_NAME		0x0184
#define ARV_ABRM_DEVICE_CAPABILITY		0x01c4
#define ARV_ABRM_MAX_DEVICE_RESPONSE_TIME	0x01cc		/* in ms */
#define ARV_ABRM_MANIFEST_TABLE_ADDRESS		0x01d0
#define ARV_ABRM_SBRM_ADDRESS			0x01d8
#define ARV_ABRM_DEVICE_CONFIGURATION		0x01e0
#define ARV_ABRM_HEARTBEAT_TIMEOUT		0x01e8
#define ARV_ABRM_MESSAGE_CHANNEL_ID		0x01ec
#define ARV_ABRM_TIMESTAMP			0x01f0
#define ARV_ABRM_TIMESTAMP_LATCH		0x01f8
#define ARV_ABRM_TIMESTAMP_INCREMENT		0x01fc
#define ARV_ABRM_ACCESS_PRIVILEGE		0x0204
#define ARV_ABRM_PROTOCOL_ENDIANESS		0x0208
#define ARV_ABRM_IMPLEMENTATION_ENDIANESS	0x020c
#define ARV_ABRM_RESERVED			0x0210

#define ARV_SBRM_U3V_VERSION			0x0000
#define ARV_SBRM_U3VCP_CAPABILITY		0x0004
#define ARV_SBRM_U3VCP_CONFIGURATION		0x000c
#define ARV_SBRM_MAX_CMD_TRANSFER		0x0014
#define ARV_SBRM_MAX_ACK_TRANSFER		0x0018
#define ARV_SBRM_NUM_STREAM_CHANNELS		0x001c
#define ARV_SBRM_SIRM_ADDRESS			0x0020
#define ARV_SBRM_SIRM_LENGTH			0x0028
#define ARV_SBRM_EIRM_ADDRESS			0x002c
#define ARV_SBRM_EIRM_LENGTH			0x0034
#define ARV_SBRM_IIDC2_ADDRESS			0x0038
#define ARV_SBRM_CURRENT_SPEED			0x0040
#define ARV_SBRM_RESERVED			0x0044

#define ARV_SI_INFO				0x0000
#define ARV_SI_CONTROL				0x0004
#define ARV_SI_REQ_PAYLOAD_SIZE			0x0008
#define ARV_SI_REQ_LEADER_SIZE			0x0010
#define ARV_SI_REQ_TRAILER_SIZE			0x0014
#define ARV_SI_MAX_LEADER_SIZE			0x0018
#define ARV_SI_PAYLOAD_SIZE			0x001C
#define ARV_SI_PAYLOAD_COUNT			0x0020
#define ARV_SI_TRANSFER1_SIZE			0x0024
#define ARV_SI_TRANSFER2_SIZE			0x0028
#define ARV_SI_MAX_TRAILER_SIZE			0x002C
#define ARV_SI_INFO_ALIGNMENT_MASK		0xFF000000
#define ARV_SI_INFO_ALIGNMENT_SHIFT		0x0018

/**
 * ArvUvcpPacketType:
 * @ARV_UVCP_PACKET_TYPE_ERROR: error packet
 * @ARV_UVCP_PACKET_TYPE_ACK: acknowledge packet
 * @ARV_UVCP_PACKET_TYPE_CMD: command packet
 */

typedef enum {
	ARV_UVCP_PACKET_TYPE_ERROR =		0xffff,
	ARV_UVCP_PACKET_TYPE_ACK =		0x0000,
	ARV_UVCP_PACKET_TYPE_CMD = 		0x4000
} ArvUvcpPacketType;

/**
 * ArvUvcpCommand:
 * @ARV_UVCP_COMMAND_READ_MEMORY_CMD: read memory command
 * @ARV_UVCP_COMMAND_READ_MEMORY_ACK: read memory acknowledge
 * @ARV_UVCP_COMMAND_WRITE_MEMORY_CMD: write memory command
 * @ARV_UVCP_COMMAND_WRITE_MEMORY_ACK: write memory acknowledge
 * @ARV_UVCP_COMMAND_PENDING_ACK: pending command acknowledge
 * @ARV_UVCP_COMMAND_EVENT_CMD: event command
 */

typedef enum {
	ARV_UVCP_COMMAND_READ_MEMORY_CMD =	0x0800,
	ARV_UVCP_COMMAND_READ_MEMORY_ACK =	0x0801,
	ARV_UVCP_COMMAND_WRITE_MEMORY_CMD =	0x0802,
	ARV_UVCP_COMMAND_WRITE_MEMORY_ACK =	0x0803,
	ARV_UVCP_COMMAND_PENDING_ACK =		0x0805,
	ARV_UVCP_COMMAND_EVENT_CMD =		0x0c00
} ArvUvcpCommand;

#define ARAVIS_PACKED_STRUCTURE __attribute__((__packed__))

/**
 * ArvUvcpHeader:
 *
 * UVCP packet header structure.
 */

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint32 magic;
	guint16 packet_type;
	guint16 command;
	guint16 size;
	guint16 id;
} ArvUvcpHeader;

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint64 address;
        guint16 unknown;	/* Listed as reserved, always 0 */
	guint16 size;
} ArvUvcpReadMemoryCmdInfos;

typedef struct ARAVIS_PACKED_STRUCTURE {
	ArvUvcpHeader header;
	ArvUvcpReadMemoryCmdInfos infos;
} ArvUvcpReadMemoryCmd;

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint64 address;
} ArvUvcpWriteMemoryCmdInfos;

typedef struct ARAVIS_PACKED_STRUCTURE {
	ArvUvcpHeader header;
	ArvUvcpWriteMemoryCmdInfos infos;
} ArvUvcpWriteMemoryCmd;

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint16 unknown;
	guint16 bytes_written;
} ArvUvcpWriteMemoryAckInfos;

typedef struct ARAVIS_PACKED_STRUCTURE {
	ArvUvcpHeader header;
	ArvUvcpWriteMemoryAckInfos infos;
} ArvUvcpWriteMemoryAck;

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint16 unknown;
	guint16 timeout;
} ArvUvcpPendingAckInfos;

typedef struct ARAVIS_PACKED_STRUCTURE {
	ArvUvcpHeader header;
	ArvUvcpPendingAckInfos infos;
} ArvUvcpPendingAck;

/**
 * ArvUvcpPacket:
 * @header: packet header
 * @data: variable size byte array
 *
 * UVCP packet structure.
 */

typedef struct ARAVIS_PACKED_STRUCTURE {
	ArvUvcpHeader header;
	unsigned char data[];
} ArvUvcpPacket;

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint16 file_version_subminor;
	guint8 file_version_minor;
	guint8 file_version_major;
	guint32 schema;
	guint64 address;
	guint64 size;
	guint64 unknown3;
	guint64 unknown4;
	guint64 unknown5;
	guint64 unknown6;
	guint64 unknown7;
} ArvUvcpManifestEntry;

/**
 * ArvUvcpManifestSchemaType:
 * @ARV_UVCP_SCHEMA_RAW: uncompressed genicam data
 * @ARV_UVCP_SCHEMA_ZIP: zipped genicam data
 *
 * This is packed into the 32-bit schema type as bits 10-15
 */

typedef enum
{
	ARV_UVCP_SCHEMA_RAW = 0x0,
	ARV_UVCP_SCHEMA_ZIP = 0x1
}
ArvUvcpManifestSchemaType;

static inline ArvUvcpManifestSchemaType
arv_uvcp_manifest_entry_get_schema_type (ArvUvcpManifestEntry *entry)
{
	g_return_val_if_fail (entry != NULL, ARV_UVCP_SCHEMA_RAW);

	return (entry->schema >> 10) & 0x0000001f;
}

#undef ARAVIS_PACKED_STRUCTURE

void 			arv_uvcp_packet_free 			(ArvUvcpPacket *packet);
ArvUvcpPacket * 	arv_uvcp_packet_new_read_memory_cmd 	(guint64 address, guint32 size,
								 guint16 packet_id, size_t *packet_size);
ArvUvcpPacket * 	arv_uvcp_packet_new_write_memory_cmd	(guint64 address, guint32 size,
								 guint16 packet_id, size_t *packet_size);
char * 			arv_uvcp_packet_to_string 		(const ArvUvcpPacket *packet);
void 			arv_uvcp_packet_debug 			(const ArvUvcpPacket *packet, ArvDebugLevel level);
const char * 		arv_uvcp_packet_type_to_string 		(ArvUvcpPacketType value);
const char * 		arv_uvcp_command_to_string 		(ArvUvcpCommand value);


/**
 * arv_uvcp_packet_get_packet_type:
 * @packet: a #ArvUvcpPacket
 *
 * Return value: The #ArvUvcpPacketType code of @packet.
 */

static inline ArvUvcpPacketType
arv_uvcp_packet_get_packet_type (ArvUvcpPacket *packet)
{
	if (packet == NULL)
		return ARV_UVCP_PACKET_TYPE_ERROR;

	return (ArvUvcpPacketType) GUINT16_FROM_LE (packet->header.packet_type);
}

/**
 * arv_uvcp_packet_get_command:
 * @packet: a #ArvUvcpPacket
 *
 * Return value: The #ArvUvcpCommand code of @packet.
 */

static inline ArvUvcpCommand
arv_uvcp_packet_get_command (ArvUvcpPacket *packet)
{
	if (packet == NULL)
		return (ArvUvcpCommand) 0;

	return (ArvUvcpCommand) GUINT16_FROM_LE (packet->header.command);
}

static inline void
arv_uvcp_packet_set_packet_id (ArvUvcpPacket *packet, guint16 id)
{
	if (packet != NULL)
		packet->header.id = GUINT16_TO_LE (id);
}

static inline guint16
arv_uvcp_packet_get_packet_id (ArvUvcpPacket *packet)
{
	if (packet == NULL)
		return 0;

	return GUINT16_FROM_LE (packet->header.id);
}

static inline void *
arv_uvcp_packet_get_read_memory_ack_data (const ArvUvcpPacket *packet)
{
	return (char *) packet + sizeof (ArvUvcpHeader);
}

static inline size_t
arv_uvcp_packet_get_read_memory_ack_size (size_t data_size)
{
	return sizeof (ArvUvcpHeader) + data_size;
}

static inline void *
arv_uvcp_packet_get_write_memory_cmd_data (const ArvUvcpPacket *packet)
{
	return (char *) packet + sizeof (ArvUvcpWriteMemoryCmd);
}

static inline size_t
arv_uvcp_packet_get_write_memory_ack_size (void)
{
	return sizeof (ArvUvcpWriteMemoryAck);
}

static inline guint16
arv_uvcp_packet_get_pending_ack_timeout (ArvUvcpPacket *packet)
{
	return GUINT16_FROM_LE (((ArvUvcpPendingAck *) packet)->infos.timeout);
}

static inline guint16
arv_uvcp_next_packet_id (guint16 packet_id)
{
	/* packet_id == 0 is an error value */
	if (packet_id == 0xffff)
		return 1;
	return packet_id + 1;
}

G_END_DECLS

#endif
