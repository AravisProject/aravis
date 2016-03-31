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

#ifndef ARV_UVSP_H
#define ARV_UVSP_H

#include <arvtypes.h>
#include <arvdebug.h>

G_BEGIN_DECLS

#define ARV_UVSP_LEADER_MAGIC	0x4C563355
#define ARV_UVSP_TRAILER_MAGIC	0x54563355

/**
 * ArvUvspPacketType:
 * @ARV_UVSP_PACKET_TYPE_UNKNOWN: unknown packet
 * @ARV_UVSP_PACKET_TYPE_LEADER: leader packet
 * @ARV_UVSP_PACKET_TYPE_TRAILER: trailer packet
 * @ARV_UVSP_PACKET_TYPE_DATA: data packet
 */

typedef enum {
	ARV_UVSP_PACKET_TYPE_UNKNOWN,
	ARV_UVSP_PACKET_TYPE_LEADER,
	ARV_UVSP_PACKET_TYPE_TRAILER,
	ARV_UVSP_PACKET_TYPE_DATA
} ArvUvspPacketType;

typedef enum {
	ARV_UVSP_PAYLOAD_TYPE_IMAGE =			0x0001,
	ARV_UVSP_PAYLOAD_TYPE_CHUNK = 			0x4000,
	ARV_UVSP_PAYLOAD_TYPE_EXTENDED_CHUNK =		0x4001
} ArvUvspPayloadType;

#define ARAVIS_PACKED_STRUCTURE __attribute__((__packed__))

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint32 magic;
	guint16 unknown0;
	guint16 size;
	guint64 frame_id;
} ArvUvspHeader;

typedef struct ARAVIS_PACKED_STRUCTURE {
	ArvUvspHeader header;
	void *data;
} ArvUvspPacket;

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint16 unknown0;
	guint16 payload_type;
	guint64 timestamp;
	guint32 pixel_format;
	guint32 width;
	guint32 height;
	guint32	x_offset;
	guint32	y_offset;
	guint16	x_padding;
	guint16	unknown1;
} ArvUvspLeaderInfos;

typedef struct ARAVIS_PACKED_STRUCTURE {
	ArvUvspHeader header;
	ArvUvspLeaderInfos infos;
} ArvUvspLeader;

typedef struct ARAVIS_PACKED_STRUCTURE {
	guint32 unknown0;
	guint64 payload_size; 
} ArvUvspTrailerInfos;

typedef struct ARAVIS_PACKED_STRUCTURE {
	ArvUvspHeader header;
	ArvUvspTrailerInfos infos;
} ArvUvspTrailer;

#undef ARAVIS_PACKED_STRUCTURE

char * 			arv_uvsp_packet_to_string 		(const ArvUvspPacket *packet);
void 			arv_uvsp_packet_debug 			(const ArvUvspPacket *packet, ArvDebugLevel level);

static inline ArvUvspPacketType
arv_uvsp_packet_get_packet_type	(const ArvUvspPacket *packet)
{
	if (packet == NULL)
		return ARV_UVSP_PACKET_TYPE_UNKNOWN;
	else if (GUINT32_FROM_LE (packet->header.magic) == ARV_UVSP_LEADER_MAGIC)
		return ARV_UVSP_PACKET_TYPE_LEADER;
	else if (GUINT32_FROM_LE (packet->header.magic) == ARV_UVSP_TRAILER_MAGIC)
		return ARV_UVSP_PACKET_TYPE_TRAILER;
	else
		return ARV_UVSP_PACKET_TYPE_DATA;
}

static inline guint64
arv_uvsp_packet_get_frame_id (ArvUvspPacket *packet)
{
	if (packet == NULL)
		return 0;

	return (GUINT64_FROM_LE (packet->header.frame_id));
}

static inline void
arv_uvsp_packet_get_region (ArvUvspPacket *packet, guint32 *width, guint32 *height, guint32 *x_offset, guint32 *y_offset)
{
	ArvUvspLeader *leader;

	if (packet == NULL)
		return;

	leader = (ArvUvspLeader *)packet;
	*width = GUINT32_FROM_LE (leader->infos.width);
	*height = GUINT32_FROM_LE (leader->infos.height);
	*x_offset = GUINT32_FROM_LE (leader->infos.x_offset);
	*y_offset = GUINT32_FROM_LE (leader->infos.y_offset);
}

static inline guint64
arv_uvsp_packet_get_timestamp (ArvUvspPacket *packet)
{
	ArvUvspLeader *leader;

	if (packet == NULL)
		return 0;

	leader = (ArvUvspLeader *)packet;
	return GUINT64_FROM_LE (leader->infos.timestamp);
}

G_END_DECLS

#endif
