/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_GVCP_PRIVATE_H
#define ARV_GVCP_PRIVATE_H

#include <arvtypes.h>
#include <arvdebugprivate.h>

G_BEGIN_DECLS

/**
 * ARV_GVCP_PORT:
 *
 * Standard device listening port for GVCP packets
 */
#define ARV_GVCP_PORT	3956

#define ARV_GVBS_VERSION_OFFSET				0x00000000
#define ARV_GVBS_VERSION_MINOR_MASK			0x0000ffff
#define ARV_GVBS_VERSION_MINOR_POS			0
#define ARV_GVBS_VERSION_MAJOR_MASK			0xffff0000
#define ARV_GVBS_VERSION_MAJOR_POS			16

#define ARV_GVBS_DEVICE_MODE_OFFSET			0x00000004
#define ARV_GVBS_DEVICE_MODE_BIG_ENDIAN			1 << 31
#define ARV_GVBS_DEVICE_MODE_CHARACTER_SET_MASK		0x0000ffff
#define ARV_GVBS_DEVICE_MODE_CHARACTER_SET_POS		0

#define ARV_GVBS_DEVICE_MAC_ADDRESS_HIGH_OFFSET		0x00000008
#define ARV_GVBS_DEVICE_MAC_ADDRESS_LOW_OFFSET		0x0000000c

#define ARV_GVBS_SUPPORTED_IP_CONFIGURATION_OFFSET	0x00000010
#define ARV_GVBS_CURRENT_IP_CONFIGURATION_OFFSET	0x00000014
#define ARV_GVBS_IP_CONFIGURATION_PERSISTENT		1 << 0
#define ARV_GVBS_IP_CONFIGURATION_DHCP			1 << 1
#define ARV_GVBS_IP_CONFIGURATION_LLA			1 << 2

#define ARV_GVBS_CURRENT_IP_ADDRESS_OFFSET		0x00000024
#define ARV_GVBS_CURRENT_SUBNET_MASK_OFFSET		0x00000034
#define ARV_GVBS_CURRENT_GATEWAY_OFFSET			0x00000044

#define ARV_GVBS_MANUFACTURER_NAME_OFFSET		0x00000048
#define ARV_GVBS_MANUFACTURER_NAME_SIZE			32

#define ARV_GVBS_MODEL_NAME_OFFSET			0x00000068
#define ARV_GVBS_MODEL_NAME_SIZE			32

#define ARV_GVBS_DEVICE_VERSION_OFFSET			0x00000088
#define ARV_GVBS_DEVICE_VERSION_SIZE			32

#define ARV_GVBS_MANUFACTURER_INFO_OFFSET       	0x000000a8
#define ARV_GVBS_MANUFACTURER_INFO_SIZE		        48

#define ARV_GVBS_SERIAL_NUMBER_OFFSET			0x000000d8
#define ARV_GVBS_SERIAL_NUMBER_SIZE			16

#define ARV_GVBS_USER_DEFINED_NAME_OFFSET		0x000000e8
#define ARV_GVBS_USER_DEFINED_NAME_SIZE			16

#define ARV_GVBS_DISCOVERY_DATA_SIZE			0xf8

#define ARV_GVBS_XML_URL_0_OFFSET			0x00000200
#define ARV_GVBS_XML_URL_1_OFFSET			0x00000400
#define ARV_GVBS_XML_URL_SIZE				512

#define ARV_GVBS_N_NETWORK_INTERFACES_OFFSET		0x00000600

#define	ARV_GVBS_PERSISTENT_IP_ADDRESS_0_OFFSET		0x0000064c
#define	ARV_GVBS_PERSISTENT_SUBNET_MASK_0_OFFSET	0x0000065c
#define	ARV_GVBS_PERSISTENT_GATEWAY_0_OFFSET		0x0000066c

#define ARV_GVBS_N_MESSAGE_CHANNELS_OFFSET		0x00000900
#define ARV_GVBS_N_STREAM_CHANNELS_OFFSET		0x00000904

#define ARV_GVBS_GVCP_CAPABILITY_OFFSET			0x00000934
#define ARV_GVBS_GVCP_CAPABILITY_CONCATENATION			1 << 0
#define ARV_GVBS_GVCP_CAPABILITY_WRITE_MEMORY			1 << 1
#define ARV_GVBS_GVCP_CAPABILITY_PACKET_RESEND			1 << 2
#define ARV_GVBS_GVCP_CAPABILITY_EVENT				1 << 3
#define ARV_GVBS_GVCP_CAPABILITY_EVENT_DATA			1 << 4
#define ARV_GVBS_GVCP_CAPABILITY_PENDING_ACK			1 << 5
#define ARV_GVBS_GVCP_CAPABILITY_ACTION				1 << 6
#define ARV_GVBS_GVCP_CAPABILITY_PRIMARY_APPLICATION_SWITCHOVER 1 << 21
#define ARV_GVBS_GVCP_CAPABILITY_EXTENDED_STATUS_CODES		1 << 22
#define ARV_GVBS_GVCP_CAPABILITY_DISCOVERY_ACK_DELAY_WRITABLE	1 << 23
#define ARV_GVBS_GVCP_CAPABILITY_DISCOVERY_ACK_DELAY		1 << 24
#define ARV_GVBS_GVCP_CAPABILITY_TEST_DATA			1 << 25
#define ARV_GVBS_GVCP_CAPABILITY_MANIFEST_TABLE			1 << 26
#define ARV_GVBS_GVCP_CAPABILITY_CCP_APPLICATION_SOCKET 	1 << 27
#define ARV_GVBS_GVCP_CAPABILITY_LINK_SPEED			1 << 28
#define ARV_GVBS_GVCP_CAPABILITY_HEARTBEAT_DISABLE		1 << 29
#define ARV_GVBS_GVCP_CAPABILITY_SERIAL_NUMBER			1 << 30
#define ARV_GVBS_GVCP_CAPABILITY_NAME_REGISTER			1 << 31

#define ARV_GVBS_HEARTBEAT_TIMEOUT_OFFSET		0x00000938
#define ARV_GVBS_TIMESTAMP_TICK_FREQUENCY_HIGH_OFFSET	0x0000093c
#define ARV_GVBS_TIMESTAMP_TICK_FREQUENCY_LOW_OFFSET	0x00000940
#define ARV_GVBS_TIMESTAMP_CONTROL_OFFSET		0x00000944
#define ARV_GVBS_TIMESTAMP_LATCHED_VALUE_HIGH_OFFSET	0x00000948
#define ARV_GVBS_TIMESTAMP_LATCHED_VALUE_LOW_OFFSET	0x0000094c

#define ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_OFFSET	0x00000a00
#define ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_CONTROL	1 << 1
#define ARV_GVBS_CONTROL_CHANNEL_PRIVILEGE_EXCLUSIVE	1 << 0

#define ARV_GVBS_STREAM_CHANNEL_0_PORT_OFFSET		0x00000d00

#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET		0x00000d04
#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_MASK		0x0000ffff
#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_POS		0
#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_BIG_ENDIAN		1 << 29
#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_DO_NOT_FRAGMENT	1 << 30
#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_FIRE_TEST		1 << 31

#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_DELAY_OFFSET		0x00000d08

#define ARV_GVBS_STREAM_CHANNEL_0_IP_ADDRESS_OFFSET		0x00000d18

#define ARV_GVBS_DEVICE_LINK_SPEED_0_OFFSET			0x0000b000

#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_MIN_OFFSET		0x0000c000
#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_MAX_OFFSET		0x0000c004
#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_DELAY_MIN_OFFSET		0x0000c008
#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_DELAY_MAX_OFFSET		0x0000c00c
#define ARV_GVBS_STREAM_CHANNEL_0_FRAME_TRANSMISSION_DELAY_OFFSET	0x0000c010
#define ARV_GVBS_STREAM_CHANNEL_0_FRAME_TRANSMISSION_DELAY_MIN_OFFSET	0x0000c014
#define ARV_GVBS_STREAM_CHANNEL_0_FRAME_TRANSMISSION_DELAY_MAX_OFFSET	0x0000c018
#define ARV_GVBS_STREAM_CHANNEL_0_BANDWITDH_RESERVE_OFFSET		0x0000c01c
#define ARV_GVBS_STREAM_CHANNEL_0_BANDWITDH_RESERVE_MIN_OFFSET		0x0000c020
#define ARV_GVBS_STREAM_CHANNEL_0_BANDWITDH_RESERVE_MAX_OFFSET		0x0000c024
#define ARV_GVBS_STREAM_CHANNEL_0_BANDWITDH_RESERVE_ACCUMULATION_OFFSET	0x0000c028
#define ARV_GVBS_STREAM_CHANNEL_0_BANDWITDH_RESERVE_ACCUMULATION_MIN_OFFSET	0x0000c02c
#define ARV_GVBS_STREAM_CHANNEL_0_BANDWITDH_RESERVE_ACCUMULATION_MAX_OFFSET	0x0000c030
#define ARV_GVBS_STREAM_CHANNEL_0_THROUGHPUT_MAX_OFFSET			0x0000c100
#define ARV_GVBS_STREAM_CHANNEL_0_CURRENT_THOURGHPUT_OFFSET		0x0000c104
#define ARV_GVBS_STREAM_CHANNEL_0_ASSIGNED_BANDWIDTH_OFFSET		0x0000c108
#define ARV_GVBS_STREAM_CHANNEL_0_FRAME_JITTER_MAX_OFFSET		0x0000c10c

#define ARV_GVCP_DATA_SIZE_MAX				512

/**
 * ArvGvcpPacketType:
 * @ARV_GVCP_PACKET_TYPE_ACK: acknowledge packet
 * @ARV_GVCP_PACKET_TYPE_CMD: command packet
 * @ARV_GVCP_PACKET_TYPE_ERROR: error packet
 * @ARV_GVCP_PACKET_TYPE_UNKNOWN_ERROR: unknown error
 */

typedef enum {
	ARV_GVCP_PACKET_TYPE_ACK =		0x00,
	ARV_GVCP_PACKET_TYPE_CMD = 		0x42,
	ARV_GVCP_PACKET_TYPE_ERROR =		0x80,
	ARV_GVCP_PACKET_TYPE_UNKNOWN_ERROR =	0x8f
} ArvGvcpPacketType;

/**
 * ArvGvcpError:
 * @ARV_GVCP_ERROR_NONE: none
 * @ARV_GVCP_ERROR_NOT_IMPLEMENTED: not implemented
 * @ARV_GVCP_ERROR_INVALID_PARAMETER: invalid parameter
 * @ARV_GVCP_ERROR_INVALID_ACCESS: inavlid access
 * @ARV_GVCP_ERROR_WRITE_PROTECT: write protect
 * @ARV_GVCP_ERROR_BAD_ALIGNMENT: bad alignment
 * @ARV_GVCP_ERROR_ACCESS_DENIED: access denied
 * @ARV_GVCP_ERROR_BUSY: busy
 * @ARV_GVCP_ERROR_LOCAL_PROBLEM: local problem
 * @ARV_GVCP_ERROR_MESSAGE_MISMATCH: message mismatch
 * @ARV_GVCP_ERROR_INVALID_PROTOCOL: invalid protocol
 * @ARV_GVCP_ERROR_NO_MESSAGE: no message
 * @ARV_GVCP_ERROR_PACKET_UNAVAILABLE: packet unavailable
 * @ARV_GVCP_ERROR_DATA_OVERRUN: data overrun
 * @ARV_GVCP_ERROR_INVALID_HEADER: invalid header
 * @ARV_GVCP_ERROR_WRONG_CONFIG: wrong config
 * @ARV_GVCP_ERROR_PACKET_NOT_YET_AVAILABLE: packet not yet available
 * @ARV_GVCP_ERROR_PACKET_AND_PREVIOUS_REMOVED_FROM_MEMORY: packet and previous removed from memmory
 * @ARV_GVCP_ERROR_PACKET__REMOVED_FROM_MEMORY: packet removed from memory
 * @ARV_GVCP_ERROR_NO_REFERENCE_TIME: no reference time
 * @ARV_GVCP_ERROR_PACKET_TEMPORARILY_UNAVAILABLE: packet temporarily unavailable
 * @ARV_GVCP_ERROR_OVERFLOW: overflow
 * @ARV_GVCP_ERROR_ACTION_LATE: action late
 * @ARV_GVCP_ERROR_LEADER_TRAILER_OVERFLOW: leader trailer overflow
 */

typedef enum {
	ARV_GVCP_ERROR_NONE = 						0x00,
	ARV_GVCP_ERROR_NOT_IMPLEMENTED = 				0x01,
	ARV_GVCP_ERROR_INVALID_PARAMETER = 				0x02,
	ARV_GVCP_ERROR_INVALID_ACCESS =					0x03,
	ARV_GVCP_ERROR_WRITE_PROTECT =					0x04,
	ARV_GVCP_ERROR_BAD_ALIGNMENT = 					0x05,
	ARV_GVCP_ERROR_ACCESS_DENIED =					0x06,
	ARV_GVCP_ERROR_BUSY =						0x07,
	ARV_GVCP_ERROR_LOCAL_PROBLEM =					0x08,
	ARV_GVCP_ERROR_MESSAGE_MISMATCH =				0x09,
	ARV_GVCP_ERROR_INVALID_PROTOCOL =				0x0a,
	ARV_GVCP_ERROR_NO_MESSAGE =					0x0b,
	ARV_GVCP_ERROR_PACKET_UNAVAILABLE =				0x0c,
	ARV_GVCP_ERROR_DATA_OVERRUN =					0x0d,
	ARV_GVCP_ERROR_INVALID_HEADER =					0x0e,
	ARV_GVCP_ERROR_WRONG_CONFIG =					0x0f,
	ARV_GVCP_ERROR_PACKET_NOT_YET_AVAILABLE =			0x10,
	ARV_GVCP_ERROR_PACKET_AND_PREVIOUS_REMOVED_FROM_MEMORY =	0x11,
	ARV_GVCP_ERROR_PACKET_REMOVED_FROM_MEMORY =			0x12,
	ARV_GVCP_ERROR_NO_REFERENCE_TIME =				0x13,
	ARV_GVCP_ERROR_PACKET_TEMPORARILY_UNAVAILABLE =			0x14,
	ARV_GVCP_ERROR_OVERFLOW =					0x15,
	ARV_GVCP_ERROR_ACTION_LATE =					0x16,
	ARV_GVCP_ERROR_LEADER_TRAILER_OVERFLOW =			0x17,
        ARV_GVCP_ERROR_GENERIC =                                        0xff
} ArvGvcpError;

/**
 * ArvGvcpCmdPacketFlags:
 * @ARV_GVCP_CMD_PACKET_FLAGS_NONE: no flag defined
 * @ARV_GVCP_CMD_PACKET_FLAGS_ACK_REQUIRED: acknowledge required
 * @ARV_GVCP_CMD_PACKET_FLAGS_EXTENDED_IDS: use extended ids
 */

typedef enum {
	ARV_GVCP_CMD_PACKET_FLAGS_NONE =			0x00,
	ARV_GVCP_CMD_PACKET_FLAGS_ACK_REQUIRED =		0x01,
	ARV_GVCP_CMD_PACKET_FLAGS_EXTENDED_IDS =		0x10,
} ArvGvcpCmdPacketFlags;

/**
 * ArvGvcpEventPacketFlags:
 * @ARV_GVCP_EVENT_PACKET_FLAGS_NONE: no flag defined
 * @ARV_GVCP_EVENT_PACKET_FLAGS_64BIT_ID: extended id
 */

typedef enum {
	ARV_GVCP_EVENT_PACKET_FLAGS_NONE =			0x00,
	ARV_GVCP_EVENT_PACKET_FLAGS_64BIT_ID =			0x10,
} ArvGvcpEventPacketFlags;

/**
 * ArvGvcpDiscoveryPacketFlags:
 * @ARV_GVCP_DISCOVERY_PACKET_FLAGS_NONE: no flag defined
 * @ARV_GVCP_DISCOVERY_PACKET_FLAGS_ALLOW_BROADCAST_ACK: allow broadcast acknowledge
 */

typedef enum {
	ARV_GVCP_DISCOVERY_PACKET_FLAGS_NONE =			0x00,
	ARV_GVCP_DISCOVERY_PACKET_FLAGS_ALLOW_BROADCAST_ACK = 	0x10,
} ArvGvcpDiscoveryPacketFlags;

/**
 * ArvGvcpCommand:
 * @ARV_GVCP_COMMAND_DISCOVERY_CMD: discovery command
 * @ARV_GVCP_COMMAND_DISCOVERY_ACK: discovery acknowledge
 * @ARV_GVCP_COMMAND_BYE_CMD: goodbye command, for connection termination
 * @ARV_GVCP_COMMAND_BYE_ACK: goodbye acknowledge
 * @ARV_GVCP_COMMAND_PACKET_RESEND_CMD: packet resend request
 * @ARV_GVCP_COMMAND_PACKET_RESEND_ACK: packet resend acknowledge (not used ?)
 * @ARV_GVCP_COMMAND_READ_REGISTER_CMD: read register command
 * @ARV_GVCP_COMMAND_READ_REGISTER_ACK: read register acknowledge
 * @ARV_GVCP_COMMAND_WRITE_REGISTER_CMD: write register command
 * @ARV_GVCP_COMMAND_WRITE_REGISTER_ACK: write register acknowledge
 * @ARV_GVCP_COMMAND_READ_MEMORY_CMD: read memory command
 * @ARV_GVCP_COMMAND_READ_MEMORY_ACK: read memory acknowledge
 * @ARV_GVCP_COMMAND_WRITE_MEMORY_CMD: write memory command
 * @ARV_GVCP_COMMAND_WRITE_MEMORY_ACK: write memory acknowledge
 * @ARV_GVCP_COMMAND_PENDING_ACK: pending command acknowledge
 */

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
	ARV_GVCP_COMMAND_WRITE_MEMORY_ACK =	0x0087,
	ARV_GVCP_COMMAND_PENDING_ACK =		0x0089
} ArvGvcpCommand;

#pragma pack(push,1)

/**
 * ArvGvcpHeader:
 * @packet_type: a #ArvGvcpPacketType identifier
 * @packet_flags: set of packet flags
 * @command: a #ArvGvcpCommand identifier
 * @size: data size
 * @id: packet identifier
 *
 * GVCP packet header structure.
 */

typedef struct {
	guint8 packet_type;
	guint8 packet_flags;
	guint16 command;
	guint16 size;
	guint16 id;
} ArvGvcpHeader;

/**
 * ArvGvcpPacket:
 * @header: packet header
 * @data: variable size byte array
 *
 * GVCP packet structure.
 */

typedef struct {
	ArvGvcpHeader header;
	unsigned char data[];
} ArvGvcpPacket;

#pragma pack(pop)

void 			arv_gvcp_packet_free 			(ArvGvcpPacket *packet);
ArvGvcpPacket * 	arv_gvcp_packet_new_read_memory_cmd 	(guint32 address, guint32 size,
								 guint16 packet_id, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_read_memory_ack 	(guint32 address, guint32 size, guint16 packet_id,
								 size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_write_memory_cmd	(guint32 address, guint32 size, const char *buffer,
								 guint16 packet_id, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_write_memory_ack	(guint32 address,
								 guint16 packet_id, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_read_register_cmd 	(guint32 address,
								 guint16 packet_id, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_read_register_ack 	(guint32 value,
								 guint16 packet_id, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_write_register_cmd 	(guint32 address, guint32 value,
								 guint16 packet_id, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_write_register_ack 	(guint32 data_index,
								 guint16 packet_id, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_discovery_cmd 	(gboolean allow_broadcast_discovery_ack, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_discovery_ack 	(guint16 packet_id, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_packet_new_packet_resend_cmd 	(guint64 frame_id,
								 guint32 first_block, guint32 last_block,
								 gboolean extended_ids,
								 guint16 packet_id, size_t *packet_size);

const char *		arv_gvcp_packet_type_to_string 		(ArvGvcpPacketType value);
const char * 		arv_gvcp_command_to_string 		(ArvGvcpCommand value);
char *	 		arv_gvcp_packet_flags_to_string_new 	(ArvGvcpCommand command, guint8 flags);
const char * 		arv_gvcp_error_to_string 		(ArvGvcpError value);

char * 			arv_gvcp_packet_to_string 		(const ArvGvcpPacket *packet);
void 			arv_gvcp_packet_debug 			(const ArvGvcpPacket *packet, ArvDebugLevel level);

/**
 * arv_gvcp_packet_get_packet_type:
 * @packet: a #ArvGvcpPacket
 *
 * Return value: The #ArvGvcpPacketType code of @packet.
 */

static inline ArvGvcpPacketType
arv_gvcp_packet_get_packet_type (ArvGvcpPacket *packet)
{
	if (packet == NULL)
		return ARV_GVCP_PACKET_TYPE_ERROR;

	return (ArvGvcpPacketType) packet->header.packet_type;
}

/**
 * arv_gvcp_packet_get_packet_flags:
 * @packet: a #ArvGvcpPacket
 *
 * Return value: The packet flags.
 */

static inline guint8
arv_gvcp_packet_get_packet_flags (ArvGvcpPacket *packet)
{
	if (packet == NULL)
		return 0;

	return (ArvGvcpPacketType) packet->header.packet_flags;
}

/**
 * arv_gvcp_packet_get_command:
 * @packet: a #ArvGvcpPacket
 *
 * Return value: The #ArvGvcpCommand code of @packet.
 */

static inline ArvGvcpCommand
arv_gvcp_packet_get_command (ArvGvcpPacket *packet)
{
	if (packet == NULL)
		return (ArvGvcpCommand) 0;

	return (ArvGvcpCommand) g_ntohs (packet->header.command);
}

static inline void
arv_gvcp_packet_set_packet_id (ArvGvcpPacket *packet, guint16 id)
{
	if (packet != NULL)
		packet->header.id = g_htons (id);
}

static inline guint16
arv_gvcp_packet_get_packet_id (ArvGvcpPacket *packet)
{
	if (packet == NULL)
		return 0;

	return g_ntohs (packet->header.id);
}

static inline void
arv_gvcp_packet_get_read_memory_cmd_infos (const ArvGvcpPacket *packet, guint32 *address, guint32 *size)
{
	if (packet == NULL) {
		if (address != NULL)
			*address = 0;
		if (size != NULL)
			*size = 0;
		return;
	}
	if (address != NULL)
		*address = g_ntohl (*((guint32 *) ((char *) packet + sizeof (ArvGvcpPacket))));
	if (size != NULL)
		*size = (g_ntohl (*((guint32 *) ((char *) packet + sizeof (ArvGvcpPacket) + sizeof (guint32))))) &
			0xffff;
}

static inline size_t
arv_gvcp_packet_get_read_memory_ack_size (guint32 data_size)
{
	return sizeof (ArvGvcpHeader) + sizeof (guint32) + data_size;
}

static inline void *
arv_gvcp_packet_get_read_memory_ack_data (const ArvGvcpPacket *packet)
{
	return (char *) packet + sizeof (ArvGvcpHeader) + sizeof (guint32);
}

static inline void
arv_gvcp_packet_get_write_memory_cmd_infos (const ArvGvcpPacket *packet, guint32 *address, guint32 *size)
{
	if (packet == NULL) {
		if (address != NULL)
			*address = 0;
		if (size != NULL)
			*size = 0;
		return;
	}
	if (address != NULL)
		*address = g_ntohl (*((guint32 *) ((char *) packet + sizeof (ArvGvcpPacket))));
	if (size != NULL)
		*size = g_ntohs (packet->header.size) - sizeof (guint32);
}

static inline void *
arv_gvcp_packet_get_write_memory_cmd_data (const ArvGvcpPacket *packet)
{
	return (char *) packet + sizeof (ArvGvcpPacket) + sizeof (guint32);
}

static inline size_t
arv_gvcp_packet_get_write_memory_ack_size (void)
{
	return sizeof (ArvGvcpPacket) + sizeof (guint32);
}

static inline void
arv_gvcp_packet_get_read_register_cmd_infos (const ArvGvcpPacket *packet, guint32 *address)
{
	if (packet == NULL) {
		if (address != NULL)
			*address = 0;
		return;
	}
	if (address != NULL)
		*address = g_ntohl (*((guint32 *) ((char *) packet + sizeof (ArvGvcpPacket))));
}

static inline guint32
arv_gvcp_packet_get_read_register_ack_value (const ArvGvcpPacket *packet)
{
	if (packet == NULL)
		return 0;
	return g_ntohl (*((guint32 *) ((char *) packet + sizeof (ArvGvcpPacket))));
}

static inline size_t
arv_gvcp_packet_get_read_register_ack_size (void)
{
	return sizeof (ArvGvcpHeader) + sizeof (guint32);
}

static inline void
arv_gvcp_packet_get_write_register_cmd_infos (const ArvGvcpPacket *packet, guint32 *address, guint32 *value)
{
	if (packet == NULL) {
		if (address != NULL)
			*address = 0;
		if (value != NULL)
			*value = 0;
		return;
	}
	if (address != NULL)
		*address = g_ntohl (*((guint32 *) ((char *) packet + sizeof (ArvGvcpPacket))));
	if (value != NULL)
		*value = g_ntohl (*((guint32 *) ((char *) packet + sizeof (ArvGvcpPacket) + sizeof (guint32))));
}

static inline size_t
arv_gvcp_packet_get_write_register_ack_size (void)
{
	return sizeof (ArvGvcpHeader) + sizeof (guint32);
}

static inline guint16
arv_gvcp_next_packet_id (guint16 packet_id)
{
	/* packet_id == 0 is an error value */
	if (packet_id == 0xffff)
		return 1;
	return packet_id + 1;
}

static inline size_t
arv_gvcp_packet_get_pending_ack_size (void)
{
	return sizeof (ArvGvcpHeader) + sizeof (guint32);
}

/**
 * arv_gvcp_packet_get_pending_ack_timeout:
 * @packet: a #ArvGvcpPacket
 *
 * Returns: The pending acknowledge timeout stored in @packet, in ms.
 *
 * Since: 0.6.0
 */

static inline guint32
arv_gvcp_packet_get_pending_ack_timeout (const ArvGvcpPacket *packet)
{
	return packet != NULL ? g_ntohl (*((guint32 *) ((char *) packet + sizeof (ArvGvcpPacket)))) : 0;
}

G_END_DECLS

#endif
