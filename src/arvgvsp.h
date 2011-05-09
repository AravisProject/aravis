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

#ifndef ARV_GVSP_H
#define ARV_GVSP_H

#include <arvtypes.h>
#include <arvdebug.h>

G_BEGIN_DECLS

#define ARV_GVSP_PACKET_INFOS_ID_MASK		0x00ffffff
#define ARV_GVSP_PACKET_INFOS_TYPE_MASK		0xff000000
#define ARV_GVSP_PACKET_INFOS_TYPE_POS		24

#define ARV_GVSP_PACKET_PROTOCOL_OVERHEAD	(20 + 8 + 8)		/* IP + UDP + GVSP headers */

typedef enum {
	ARV_GVSP_PACKET_TYPE_DATA_LEADER = 	0x01,
	ARV_GVSP_PACKET_TYPE_DATA_TRAILER = 	0x02,
	ARV_GVSP_PACKET_TYPE_DATA_BLOCK =	0x03
} ArvGvspPacketType;

typedef struct {
	guint32 frame_id;
	guint32 packet_infos;
} __attribute__((__packed__)) ArvGvspHeader;

typedef struct {
	guint32 data0;
	guint32 timestamp_high;
	guint32 timestamp_low;
	guint32 pixel_format;
	guint32 width;
	guint32 height;
	guint32	x_offset;
	guint32	y_offset;
} __attribute__((__packed__)) ArvGvspDataLeader;

typedef struct {
	guint32 data0;
	guint32 data1;
} __attribute__((__packed__)) ArvGvspDataTrailer;

typedef struct {
	ArvGvspHeader header;
	guint8 data[];
} ArvGvspPacket;

ArvGvspPacket *		arv_gvsp_packet_new_data_leader		(guint32 frame_id, guint32 packet_id,
								 guint64 timestamp, ArvPixelFormat pixel_format,
								 guint32 width, guint32 height,
								 guint32 x_offset, guint32 y_offset,
								 void *buffer, size_t *buffer_size);
ArvGvspPacket *		arv_gvsp_packet_new_data_trailer	(guint32 frame_id, guint32 packet_id,
								 void *buffer, size_t *buffer_size);
ArvGvspPacket *		arv_gvsp_packet_new_data_block		(guint32 frame_id, guint32 packet_id,
								 size_t size, void *data,
								 void *buffer, size_t *buffer_size);
char * 			arv_gvsp_packet_to_string 		(const ArvGvspPacket *packet, size_t packet_size);
void 			arv_gvsp_packet_debug 			(const ArvGvspPacket *packet, size_t packet_size,
								 ArvDebugLevel level);

static inline ArvGvspPacketType
arv_gvsp_packet_get_packet_type	(const ArvGvspPacket *packet)
{
	return (ArvGvspPacketType) ((g_ntohl (packet->header.packet_infos) & ARV_GVSP_PACKET_INFOS_TYPE_MASK) >>
				    ARV_GVSP_PACKET_INFOS_TYPE_POS);
}

static inline guint16
arv_gvsp_packet_get_packet_id (const ArvGvspPacket *packet)
{
	return g_ntohl (packet->header.packet_infos) & ARV_GVSP_PACKET_INFOS_ID_MASK;
}

static inline guint32
arv_gvsp_packet_get_frame_id (const ArvGvspPacket *packet)
{
	return g_ntohl (packet->header.frame_id);
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
