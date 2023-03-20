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

#ifndef ARV_UVSP_PRIVATE_H
#define ARV_UVSP_PRIVATE_H

#include <arvtypes.h>
#include <arvdebug.h>
#include <arvbufferprivate.h>

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

#pragma pack(push,1)

typedef struct {
	guint32 magic;
	guint16 unknown0;
	guint16 size;
	guint64 frame_id;
} ArvUvspHeader;

typedef struct {
	ArvUvspHeader header;
	void *data;
} ArvUvspPacket;

typedef struct {
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

typedef struct {
	ArvUvspHeader header;
	ArvUvspLeaderInfos infos;
} ArvUvspLeader;

typedef struct {
	guint32 unknown0;
	guint64 payload_size;
} ArvUvspTrailerInfos;

typedef struct {
	ArvUvspHeader header;
	ArvUvspTrailerInfos infos;
} ArvUvspTrailer;

#pragma pack(pop)

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

static inline ArvBufferPayloadType
arv_uvsp_packet_get_buffer_payload_type (ArvUvspPacket *packet, gboolean *has_chunks)
{
	ArvUvspLeader *leader;
	guint16 payload_type;

	if (packet == NULL)
		return ARV_BUFFER_PAYLOAD_TYPE_UNKNOWN;

	leader = (ArvUvspLeader *) packet;

	payload_type = GUINT16_FROM_LE (leader->infos.payload_type);

        if (has_chunks != NULL)
                *has_chunks = (payload_type & 0x4000) != 0;

        return (ArvBufferPayloadType) (payload_type & 0x3fff);
}

static inline guint64
arv_uvsp_packet_get_frame_id (ArvUvspPacket *packet)
{
	if (packet == NULL)
		return 0;

	return (GUINT64_FROM_LE (packet->header.frame_id));
}

static inline void
arv_uvsp_packet_get_region (ArvUvspPacket *packet,
                            guint32 *width, guint32 *height,
                            guint32 *x_offset, guint32 *y_offset,
                            guint32 *x_padding, guint32 *y_padding)
{
        ArvUvspLeader *leader;

	if (packet == NULL)
		return;

	leader = (ArvUvspLeader *)packet;
	*width = GUINT32_FROM_LE (leader->infos.width);
	*height = GUINT32_FROM_LE (leader->infos.height);
	*x_offset = GUINT32_FROM_LE (leader->infos.x_offset);
	*y_offset = GUINT32_FROM_LE (leader->infos.y_offset);
	*x_padding = GUINT32_FROM_LE (leader->infos.x_padding);
        *y_padding = 0;
}

static inline ArvPixelFormat
arv_uvsp_packet_get_pixel_format (ArvUvspPacket *packet)
{
	ArvUvspLeader *leader;

	if (packet == NULL)
		return 0;

	leader = (ArvUvspLeader *)packet;
	return GUINT32_FROM_LE (leader->infos.pixel_format);
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
