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

#ifndef ARV_GVSP_H
#define ARV_GVSP_H

#include <arvtypes.h>
#include <arvdebug.h>

G_BEGIN_DECLS

#define ARV_GVSP_PACKET_INFOS_ID_MASK		0x00ffffff
#define ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_MASK	0xff000000
#define ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_POS	24

#define ARV_GVSP_PACKET_PROTOCOL_OVERHEAD	(20 + 8 + 8)		/* IP + UDP + GVSP headers */

/**
 * ArvGvspPacketType:
 * @ARV_GVSP_PACKET_TYPE_OK: valid packet
 * @ARV_GVSP_PACKET_TYPE_RESEND: resent packet (BlackFly PointGrey camera support)
 * @ARV_GVSP_PACKET_TYPE_ERROR: error packet, indicating invalid resend request
 */

typedef enum {
	ARV_GVSP_PACKET_TYPE_OK =		0x0000,
	ARV_GVSP_PACKET_TYPE_RESEND =		0x0100,
	ARV_GVSP_PACKET_TYPE_ERROR =		0x800c
} ArvGvspPacketType;

/**
 * ArvGvspContentType:
 * @ARV_GVSP_CONTENT_TYPE_DATA_LEADER: leader packet
 * @ARV_GVSP_CONTENT_TYPE_DATA_TRAILER: trailer packet
 * @ARV_GVSP_CONTENT_TYPE_DATA_BLOCK: data packet
 */

typedef enum {
	ARV_GVSP_CONTENT_TYPE_DATA_LEADER = 	0x01,
	ARV_GVSP_CONTENT_TYPE_DATA_TRAILER = 	0x02,
	ARV_GVSP_CONTENT_TYPE_DATA_BLOCK =	0x03
} ArvGvspContentType;

/**
 * ArvGvspPayloadType:
 * @ARV_GVSP_PAYLOAD_TYPE_IMAGE: image data
 * @ARV_GVSP_PAYLOAD_TYPE_RAWDATA: raw data
 * @ARV_GVSP_PAYLOAD_TYPE_FILE: file
 * @ARV_GVSP_PAYLOAD_TYPE_CHUNK_DATA: chunk data
 * @ARV_GVSP_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA: extended chunk data
 * @ARV_GVSP_PAYLOAD_TYPE_JPEG: JPEG data
 * @ARV_GVSP_PAYLOAD_TYPE_JPEG2000: JPEG2000 data
 * @ARV_GVSP_PAYLOAD_TYPE_H264: h264 data
 * @ARV_GVSP_PAYLOAD_TYPE_MULTIZONE_IMAGE: multizone image
*/

typedef enum {
	ARV_GVSP_PAYLOAD_TYPE_IMAGE =			0x0001,
	ARV_GVSP_PAYLOAD_TYPE_RAWDATA = 		0x0002,
	ARV_GVSP_PAYLOAD_TYPE_FILE = 			0x0003,
	ARV_GVSP_PAYLOAD_TYPE_CHUNK_DATA = 		0x0004,
	ARV_GVSP_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA = 	0x0005, /* Deprecated */
	ARV_GVSP_PAYLOAD_TYPE_JPEG = 			0x0006,
	ARV_GVSP_PAYLOAD_TYPE_JPEG2000 = 		0x0007,
	ARV_GVSP_PAYLOAD_TYPE_H264 = 			0x0008,
	ARV_GVSP_PAYLOAD_TYPE_MULTIZONE_IMAGE = 	0x0009
} ArvGvspPayloadType;

#define ARAVIS_PACKED_STRUCTURE __attribute__((__packed__))

/**
 * ArvGvspHeader:
 * @packet_type: a #ArvGvspPacketType identifier
 * @frame_id: frame identifier
 * @packet_infos: #ArvGvspContentType and packet identifier in a 32 bit value
 *
 * GVSP packet header structure.
 */

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint16 packet_type;
	guint16 frame_id;
	guint32 packet_infos;
} ArvGvspHeader;

/**
 * ArvGvspDataLeader:
 * @payload_type: ID of the payload type
 * @timestamp_high: most significant bits of frame timestamp
 * @timestamp_low: least significant bits of frame timestamp_low
 * @pixel_format: a #ArvPixelFormat identifier
 * @width: frame width, in pixels
 * @height: frame height, in pixels
 * @x_offset: frame x offset, in pixels
 * @y_offset: frame y offset, in pixels
 *
 * GVSP data leader packet data area.
 */

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint32 payload_type;
	guint32 timestamp_high;
	guint32 timestamp_low;
	guint32 pixel_format;
	guint32 width;
	guint32 height;
	guint32	x_offset;
	guint32	y_offset;
} ArvGvspDataLeader;

/**
 * ArvGvspDataTrailer:
 * @data0: unused
 * @data1: unused
 *
 * GVSP data trailer packet data area.
 */

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint32 data0;
	guint32 data1;
} ArvGvspDataTrailer;


/**
 * ArvGvspPacket:
 * @header: common GVSP packet header
 * @data: data byte array
 *
 * GVSP packet structure.
 */

typedef struct ARAVIS_PACKED_STRUCTURE {
	ArvGvspHeader header;
	guint8 data[];
} ArvGvspPacket;

#undef ARAVIS_PACKED_STRUCTURE

ArvGvspPacket *		arv_gvsp_packet_new_data_leader		(guint16 frame_id, guint32 packet_id,
								 guint64 timestamp, ArvPixelFormat pixel_format,
								 guint32 width, guint32 height,
								 guint32 x_offset, guint32 y_offset,
								 void *buffer, size_t *buffer_size);
ArvGvspPacket *		arv_gvsp_packet_new_data_trailer	(guint16 frame_id, guint32 packet_id,
								 void *buffer, size_t *buffer_size);
ArvGvspPacket *		arv_gvsp_packet_new_data_block		(guint16 frame_id, guint32 packet_id,
								 size_t size, void *data,
								 void *buffer, size_t *buffer_size);
char * 			arv_gvsp_packet_to_string 		(const ArvGvspPacket *packet, size_t packet_size);
void 			arv_gvsp_packet_debug 			(const ArvGvspPacket *packet, size_t packet_size,
								 ArvDebugLevel level);
static inline ArvGvspPacketType
arv_gvsp_packet_get_packet_type (const ArvGvspPacket *packet)
{
	return (ArvGvspPacketType) g_ntohs (packet->header.packet_type);
}

static inline ArvGvspContentType
arv_gvsp_packet_get_content_type (const ArvGvspPacket *packet)
{
	return (ArvGvspContentType) ((g_ntohl (packet->header.packet_infos) & ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_MASK) >>
		ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_POS);
}

static inline guint16
arv_gvsp_packet_get_packet_id (const ArvGvspPacket *packet)
{
	return g_ntohl (packet->header.packet_infos) & ARV_GVSP_PACKET_INFOS_ID_MASK;
}

static inline guint16
arv_gvsp_packet_get_frame_id (const ArvGvspPacket *packet)
{
	return g_ntohs (packet->header.frame_id);
}

static inline ArvGvspPayloadType
arv_gvsp_packet_get_payload_type (const ArvGvspPacket *packet)
{
	ArvGvspDataLeader *leader;

	leader = (ArvGvspDataLeader *) &packet->data;
	return (ArvGvspPayloadType) g_ntohl (leader->payload_type);
}

static inline guint32
arv_gvsp_packet_get_x_offset (const ArvGvspPacket *packet)
{
	ArvGvspDataLeader *leader;

	leader = (ArvGvspDataLeader *) &packet->data;
	return g_ntohl (leader->x_offset);
}

static inline guint32
arv_gvsp_packet_get_y_offset (const ArvGvspPacket *packet)
{
	ArvGvspDataLeader *leader;

	leader = (ArvGvspDataLeader *) &packet->data;
	return g_ntohl (leader->y_offset);
}

static inline guint32
arv_gvsp_packet_get_width (const ArvGvspPacket *packet)
{
	ArvGvspDataLeader *leader;

	leader = (ArvGvspDataLeader *) &packet->data;
	return g_ntohl (leader->width);
}

static inline guint32
arv_gvsp_packet_get_height (const ArvGvspPacket *packet)
{
	ArvGvspDataLeader *leader;

	leader = (ArvGvspDataLeader *) &packet->data;
	return g_ntohl (leader->height);
}

static inline ArvPixelFormat
arv_gvsp_packet_get_pixel_format (const ArvGvspPacket *packet)
{
	ArvGvspDataLeader *leader;

	leader = (ArvGvspDataLeader *) &packet->data;
	return (ArvPixelFormat) g_ntohl (leader->pixel_format);
}

static inline guint64
arv_gvsp_packet_get_timestamp (const ArvGvspPacket *packet, guint64 timestamp_tick_frequency)
{
	ArvGvspDataLeader *leader;
	guint64 timestamp_s;
	guint64 timestamp_ns;
	guint64 timestamp;

	if (timestamp_tick_frequency < 1)
		return 0;

	leader = (ArvGvspDataLeader *) &packet->data;

	timestamp = ( (guint64) g_ntohl (leader->timestamp_high) << 32) | g_ntohl (leader->timestamp_low);

	timestamp_s = timestamp / timestamp_tick_frequency;
	timestamp_ns = ((timestamp % timestamp_tick_frequency) * 1000000000) / timestamp_tick_frequency;

	timestamp_ns += timestamp_s * 1000000000;

	return timestamp_ns;
}

static inline size_t
arv_gvsp_packet_get_data_size (size_t packet_size)
{
	return packet_size - sizeof (ArvGvspHeader);
}

G_END_DECLS

#endif
