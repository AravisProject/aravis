/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_GVSP_PRIVATE_H
#define ARV_GVSP_PRIVATE_H

#include <arvtypes.h>
#include <arvbuffer.h>
#include <arvdebugprivate.h>

G_BEGIN_DECLS

#define ARV_GVSP_PACKET_EXTENDED_ID_MODE_MASK	0x80000000
#define ARV_GVSP_PACKET_ID_MASK			0x00ffffff
#define ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_MASK	0x7f000000
#define ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_POS	24
#define ARV_GVSP_PACKET_INFOS_N_PARTS_MASK   	0x000000ff

/**
 * ArvGvspPacketStatus:
 * @ARV_GVSP_PACKET_STATUS_SUCCESS: valid packet
 * @ARV_GVSP_PACKET_STATUS_PACKET_RESEND: resent packet (BlackFly PointGrey camera support)
 * @ARV_GVSP_PACKET_STATUS_NOT_IMPLEMENTED:
 * @ARV_GVSP_PACKET_STATUS_INVALID_PARAMETER:
 * @ARV_GVSP_PACKET_STATUS_INVALID_ADDRESS:
 * @ARV_GVSP_PACKET_STATUS_WRITE_PROTECT:
 * @ARV_GVSP_PACKET_STATUS_BAD_ALIGNMENT:
 * @ARV_GVSP_PACKET_STATUS_ACCESS_DENIED:
 * @ARV_GVSP_PACKET_STATUS_BUSY:
 * @ARV_GVSP_PACKET_STATUS_LOCAL_PROBLEM:
 * @ARV_GVSP_PACKET_STATUS_MSG_MISMATCH:
 * @ARV_GVSP_PACKET_STATUS_INVALID_PROTOCOL:
 * @ARV_GVSP_PACKET_STATUS_NO_MSG:
 * @ARV_GVSP_PACKET_STATUS_PACKET_UNAVAILABLE: error packet, indicating invalid resend request
 * @ARV_GVSP_PACKET_STATUS_DATA_OVERRUN:
 * @ARV_GVSP_PACKET_STATUS_INVALID_HEADER:
 * @ARV_GVSP_PACKET_STATUS_WRONG_CONFIG:
 * @ARV_GVSP_PACKET_STATUS_PACKET_NOT_YET_AVAILABLE:
 * @ARV_GVSP_PACKET_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY:
 * @ARV_GVSP_PACKET_STATUS_PACKET_REMOVED_FROM_MEMORY:
 * @ARV_GVSP_PACKET_STATUS_NO_REF_TIME:
 * @ARV_GVSP_PACKET_STATUS_PACKET_TEMPORARILY_UNAVAILABLE:
 * @ARV_GVSP_PACKET_STATUS_OVERFLOW:
 * @ARV_GVSP_PACKET_STATUS_ACTION_LATE:
 * @ARV_GVSP_PACKET_STATUS_LEADER_TRAILER_OVERFLOW:
 * @ARV_GVSP_PACKET_STATUS_ERROR: generic error
 */

typedef enum {
	ARV_GVSP_PACKET_STATUS_SUCCESS =	                        0x0000,
	ARV_GVSP_PACKET_STATUS_PACKET_RESEND =		                0x0100,
        ARV_GVSP_PACKET_STATUS_NOT_IMPLEMENTED =                        0x8001,
        ARV_GVSP_PACKET_STATUS_INVALID_PARAMETER =                      0x8002,
        ARV_GVSP_PACKET_STATUS_INVALID_ADDRESS =                        0x8003,
        ARV_GVSP_PACKET_STATUS_WRITE_PROTECT =                          0x8004,
        ARV_GVSP_PACKET_STATUS_BAD_ALIGNMENT =                          0x8005,
        ARV_GVSP_PACKET_STATUS_ACCESS_DENIED =                          0x8006,
        ARV_GVSP_PACKET_STATUS_BUSY =                                   0x8007,
        ARV_GVSP_PACKET_STATUS_LOCAL_PROBLEM =                          0x8008, /* deprecated */
        ARV_GVSP_PACKET_STATUS_MSG_MISMATCH =                           0x8009, /* deprecated */
        ARV_GVSP_PACKET_STATUS_INVALID_PROTOCOL =                       0x800A, /* deprecated */
        ARV_GVSP_PACKET_STATUS_NO_MSG =                                 0x800B, /* deprecated */
        ARV_GVSP_PACKET_STATUS_PACKET_UNAVAILABLE =                     0x800C,
        ARV_GVSP_PACKET_STATUS_DATA_OVERRUN =                           0x800D,
        ARV_GVSP_PACKET_STATUS_INVALID_HEADER =                         0x800E,
        ARV_GVSP_PACKET_STATUS_WRONG_CONFIG =                           0x800F, /* deprecated */
        ARV_GVSP_PACKET_STATUS_PACKET_NOT_YET_AVAILABLE =               0x8010,
        ARV_GVSP_PACKET_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY =    0x8011,
        ARV_GVSP_PACKET_STATUS_PACKET_REMOVED_FROM_MEMORY =             0x8012,
        ARV_GVSP_PACKET_STATUS_NO_REF_TIME =                            0x8013, /* GEV 2.0 */
        ARV_GVSP_PACKET_STATUS_PACKET_TEMPORARILY_UNAVAILABLE =         0x8014, /* GEV 2.0 */
        ARV_GVSP_PACKET_STATUS_OVERFLOW =                               0x8015, /* GEV 2.0 */
        ARV_GVSP_PACKET_STATUS_ACTION_LATE =                            0x8016, /* GEV 2.0 */
        ARV_GVSP_PACKET_STATUS_LEADER_TRAILER_OVERFLOW =                0x8017, /* GEV 2.1 */
        ARV_GVSP_PACKET_STATUS_ERROR =                                  0x8fff
} ArvGvspPacketStatus;

/**
 * ArvGvspContentType:
 * @ARV_GVSP_CONTENT_TYPE_LEADER: leader packet
 * @ARV_GVSP_CONTENT_TYPE_TRAILER: trailer packet
 * @ARV_GVSP_CONTENT_TYPE_PAYLOAD: data packet
 * @ARV_GVSP_CONTENT_TYPE_ALL_IN: leader + data + trailer packet
 * @ARV_GVSP_CONTENT_TYPE_H264: h264 data packet
 * @ARV_GVSP_CONTENT_TYPE_MULTIZONE: multizone data packet
 * @ARV_GVSP_CONTENT_TYPE_MULTIPART: multipart data packet
 * @ARV_GVSP_CONTENT_TYPE_GENDC: GenDC data packet
 */

typedef enum {
	ARV_GVSP_CONTENT_TYPE_LEADER = 	        0x01,
	ARV_GVSP_CONTENT_TYPE_TRAILER = 	0x02,
	ARV_GVSP_CONTENT_TYPE_PAYLOAD =	        0x03,
	ARV_GVSP_CONTENT_TYPE_ALL_IN =		0x04,
        ARV_GVSP_CONTENT_TYPE_H264 =            0x05,
        ARV_GVSP_CONTENT_TYPE_MULTIZONE =       0x06,
        ARV_GVSP_CONTENT_TYPE_MULTIPART =       0x07,
        ARV_GVSP_CONTENT_TYPE_GENDC =           0x08
} ArvGvspContentType;

#pragma pack(push,1)

/**
 * ArvGvspHeader:
 * @frame_id: frame identifier
 * @packet_infos: #ArvGvspContentType and packet identifier in a 32 bit value
 * @data: data byte array
 *
 * GVSP packet header structure.
 */

typedef struct {
	guint16 frame_id;
	guint32 packet_infos;
	guint8 data[];
} ArvGvspHeader;

typedef struct {
	guint16 flags;
	guint32 packet_infos;
	guint64 frame_id;
	guint32 packet_id;
	guint8 data[];
} ArvGvspExtendedHeader;

/**
 * ArvGvspLeader:
 * @flags: generic flags
 * @payload_type: ID of the payload type
 */

typedef struct {
	guint16 flags;
	guint16 payload_type;
	guint32 timestamp_high;
	guint32 timestamp_low;
} ArvGvspLeader;

/**
 * ArvGvspImageInfos:
 * @pixel_format: a #ArvPixelFormat identifier
 * @width: frame width, in pixels
 * @height: frame height, in pixels
 * @x_offset: frame x offset, in pixels
 * @y_offset: frame y offset, in pixels
 */

typedef struct {
	guint32 pixel_format;
	guint32 width;
	guint32 height;
	guint32	x_offset;
	guint32	y_offset;
	guint16	x_padding;
	guint16	y_padding;
} ArvGvspImageInfos;

/**
 * ArvGvspImageLeader:
 * @flags: generic flags
 * @payload_type: ID of the payload type
 * @timestamp_high: most significant bits of frame timestamp
 * @timestamp_low: least significant bits of frame timestamp_low
 * @infos: image infos
 */

typedef struct {
	guint16 flags;
	guint16 payload_type;
	guint32 timestamp_high;
	guint32 timestamp_low;
        ArvGvspImageInfos infos;
} ArvGvspImageLeader;

typedef struct {
        guint16 data_type;
        guint16 part_length_high;
        guint32 part_length_low;
	guint32 pixel_format;
        guint16 reserved_0;
        guint8 source_id;
        guint8 additional_zones;
        guint32 zone_directions;
        guint16 data_purpose_id;
        guint16 region_id;
        guint32 width;
        guint32 height;
        guint32 x_offset;
        guint32 y_offset;
        guint16 x_padding;
        guint16 y_padding;
        guint32 reserved_1;
} ArvGvspPartInfos; /* 48 bytes */

typedef struct {
	guint16 flags;
	guint16 payload_type;
	guint32 timestamp_high;
	guint32 timestamp_low;
        ArvGvspPartInfos parts[];
} ArvGvspMultipartLeader;

typedef struct {
        guint8 part_id;
        guint8 zone_info;
        guint16 offset_high;
        guint32 offset_low;
} ArvGvspMultipart;

/**
 * ArvGvspTrailer:
 * @payload_type: ID of the payload type
 * @data0: unused
 *
 * GVSP data trailer packet data area.
 */

typedef struct {
	guint32 payload_type;
	guint32 data0;
} ArvGvspTrailer;


/**
 * ArvGvspPacket:
 * @status: status
 * @header: common GVSP packet header
 *
 * GVSP packet structure.
 */

typedef struct {
	guint16 status;
	guint8 header[];
} ArvGvspPacket;

/* Minimum ethernet frame size minus ethernet protocol overhead */
#define ARV_GVSP_MINIMUM_PACKET_SIZE                            (64 - 14 - 4)
/* Maximum ethernet frame size minus ethernet protocol overhead */
#define ARV_GVSP_MAXIMUM_PACKET_SIZE                            (65536 - 14 - 4)
 /* IP + UDP */
#define ARV_GVSP_PACKET_UDP_OVERHEAD    		        (20 + 8)
 /* IP + UDP + GVSP headers or IP + UDP + GVSP extended headers */
#define ARV_GVSP_PACKET_PROTOCOL_OVERHEAD(ext_ids)	        ((ext_ids) ? \
                                                                 20 + 8 + \
                                                                 sizeof (ArvGvspPacket) + \
                                                                 sizeof (ArvGvspExtendedHeader) : \
                                                                 20 + 8 + \
                                                                 sizeof (ArvGvspPacket) + \
                                                                 sizeof (ArvGvspHeader))
#define ARV_GVSP_PAYLOAD_PACKET_PROTOCOL_OVERHEAD(ext_ids)      ARV_GVSP_PACKET_PROTOCOL_OVERHEAD(ext_ids)
#define ARV_GVSP_MULTIPART_PACKET_PROTOCOL_OVERHEAD(ext_ids)	((ext_ids) ? \
                                                                 20 + 8 + \
                                                                 sizeof (ArvGvspPacket) + \
                                                                 sizeof (ArvGvspExtendedHeader) + \
                                                                 sizeof (ArvGvspMultipart) : \
                                                                 20 + 8 + \
                                                                 sizeof (ArvGvspPacket) + \
                                                                 sizeof (ArvGvspHeader) + \
                                                                 sizeof (ArvGvspMultipart))

#pragma pack(pop)

ArvGvspPacket *		arv_gvsp_packet_new_image_leader	(guint16 frame_id, guint32 packet_id,
								 guint64 timestamp, ArvPixelFormat pixel_format,
								 guint32 width, guint32 height,
								 guint32 x_offset, guint32 y_offset,
								 guint32 x_padding, guint32 y_padding,
								 void *buffer, size_t buffer_size,
                                                                 size_t *packet_size);
ArvGvspPacket *		arv_gvsp_packet_new_data_trailer	(guint16 frame_id, guint32 packet_id, guint32 height,
								 void *buffer, size_t buffer_size,
                                                                 size_t *packet_size);
ArvGvspPacket *		arv_gvsp_packet_new_payload		(guint16 frame_id, guint32 packet_id,
								 size_t payload_size, void *data,
								 void *buffer, size_t buffer_size,
                                                                 size_t *packet_size);
char * 			arv_gvsp_packet_to_string 		(const ArvGvspPacket *packet, size_t packet_size);
void 			arv_gvsp_packet_debug 			(const ArvGvspPacket *packet, size_t packet_size,
								 ArvDebugLevel level);
static inline ArvGvspPacketStatus
arv_gvsp_packet_get_status (const ArvGvspPacket *packet, size_t packet_size)
{
        if (G_LIKELY(packet != NULL && packet_size >= sizeof (ArvGvspPacket)))
                return (ArvGvspPacketStatus) g_ntohs (packet->status);

        return ARV_GVSP_PACKET_STATUS_ERROR;
}

static inline gboolean
arv_gvsp_packet_status_is_error (const ArvGvspPacketStatus status)
{
	return (status & 0x8000) != 0;
}

static inline gboolean
arv_gvsp_packet_has_extended_ids (const ArvGvspPacket *packet, size_t packet_size)
{
        ArvGvspHeader *header = (ArvGvspHeader *) &packet->header;

        if (G_LIKELY(packet != NULL && packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspHeader)))
                return (g_ntohl (header->packet_infos) & ARV_GVSP_PACKET_EXTENDED_ID_MODE_MASK) != 0;

        return FALSE;
}

static inline ArvGvspContentType
arv_gvsp_packet_get_content_type (const ArvGvspPacket *packet, size_t packet_size)
{
        if (G_LIKELY (packet != NULL)) {
                if (arv_gvsp_packet_has_extended_ids (packet, packet_size)) {
                        ArvGvspExtendedHeader *header = (ArvGvspExtendedHeader *) &packet->header;

                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspExtendedHeader)))
                                return (ArvGvspContentType) ((g_ntohl (header->packet_infos) &
                                                              ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_MASK) >>
                                                             ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_POS);
                } else {
                        ArvGvspHeader *header = (ArvGvspHeader *) &packet->header;

                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspHeader)))
                                return (ArvGvspContentType) ((g_ntohl (header->packet_infos) &
                                                              ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_MASK) >>
                                                             ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_POS);
                }
        }

        return 0;
}

static inline guint32
arv_gvsp_packet_get_packet_id (const ArvGvspPacket *packet, size_t packet_size)
{
        if (G_LIKELY (packet != NULL)) {
                if (arv_gvsp_packet_has_extended_ids (packet, packet_size)) {
                        ArvGvspExtendedHeader *header = (ArvGvspExtendedHeader *) &packet->header;

                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspExtendedHeader)))
                                return g_ntohl (header->packet_id);
                } else {
                        ArvGvspHeader *header = (ArvGvspHeader *) &packet->header;

                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspHeader)))
                                return g_ntohl (header->packet_infos) & ARV_GVSP_PACKET_ID_MASK;
                }
        }

        return 0;
}

static inline guint64
arv_gvsp_packet_get_frame_id (const ArvGvspPacket *packet, size_t packet_size)
{
        if (G_LIKELY (packet != NULL)) {
                if (arv_gvsp_packet_has_extended_ids (packet, packet_size)) {
                        ArvGvspExtendedHeader *header = (ArvGvspExtendedHeader *) &packet->header;

                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspExtendedHeader)))
                                return GUINT64_FROM_BE(header->frame_id);
                } else {
                        ArvGvspHeader *header = (ArvGvspHeader *) &packet->header;

                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspHeader)))
                                return g_ntohs (header->frame_id);
                }
        }

        return 0;
}

static inline void *
arv_gvsp_packet_get_data (const ArvGvspPacket *packet, size_t packet_size)
{
        if (G_LIKELY (packet != NULL)) {
                if (arv_gvsp_packet_has_extended_ids (packet, packet_size)) {
                        ArvGvspExtendedHeader *header = (ArvGvspExtendedHeader *) &packet->header;

                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspExtendedHeader)))
                                return &header->data;
                } else {
                        ArvGvspHeader *header = (ArvGvspHeader *) &packet->header;

                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspHeader)))
                                return &header->data;
                }
        }

        return NULL;
}

static inline size_t
arv_gvsp_packet_get_data_size (const ArvGvspPacket *packet, size_t packet_size)
{
        if (G_LIKELY (packet != NULL)) {
                if (arv_gvsp_packet_has_extended_ids (packet, packet_size)) {
                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspExtendedHeader)))
                                return packet_size - sizeof (ArvGvspPacket) - sizeof (ArvGvspExtendedHeader);
                } else {
                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspHeader)))
                                return packet_size - sizeof (ArvGvspPacket) - sizeof (ArvGvspHeader);
                }
        }

        return 0;
}

static inline ArvBufferPayloadType
arv_gvsp_leader_packet_get_buffer_payload_type (const ArvGvspPacket *packet, size_t packet_size, gboolean *has_chunks)
{
        if (G_LIKELY (arv_gvsp_packet_get_content_type (packet, packet_size) == ARV_GVSP_CONTENT_TYPE_LEADER)) {
                ArvGvspLeader *leader;
                guint16 payload_type;

                leader = (ArvGvspLeader *) arv_gvsp_packet_get_data (packet, packet_size);

                if (G_LIKELY (leader != NULL)) {
                        payload_type = g_ntohs (leader->payload_type);

                        if (has_chunks != NULL)
                                *has_chunks = ((payload_type & 0x4000) != 0 ||
                                               (payload_type == ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA) ||
                                               (payload_type == ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA));

                        return (ArvBufferPayloadType) (payload_type & 0x3fff);
                }
        }

        return ARV_BUFFER_PAYLOAD_TYPE_UNKNOWN;
}

static inline guint64
arv_gvsp_leader_packet_get_timestamp (const ArvGvspPacket *packet, size_t packet_size)
{
        if (G_LIKELY (arv_gvsp_packet_get_content_type (packet, packet_size) == ARV_GVSP_CONTENT_TYPE_LEADER)) {
                ArvGvspLeader *leader;

                leader = (ArvGvspLeader *) arv_gvsp_packet_get_data (packet, packet_size);

                if (G_LIKELY(leader) != NULL)
                        return ((guint64) g_ntohl (leader->timestamp_high) << 32) | g_ntohl (leader->timestamp_low);
        }

        return 0;
}

static inline guint8
arv_gvsp_leader_packet_get_multipart_n_parts (const ArvGvspPacket *packet, size_t packet_size)
{
        if (G_LIKELY (arv_gvsp_leader_packet_get_buffer_payload_type (packet, packet_size, NULL) ==
                       ARV_BUFFER_PAYLOAD_TYPE_MULTIPART)) {
                if (arv_gvsp_packet_has_extended_ids (packet, packet_size)) {
                        ArvGvspExtendedHeader *header = (ArvGvspExtendedHeader *) &packet->header;

                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspExtendedHeader)))
                                return (g_ntohl (header->packet_infos) & ARV_GVSP_PACKET_INFOS_N_PARTS_MASK);
                } else {
                        ArvGvspHeader *header = (ArvGvspHeader *) &packet->header;

                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspHeader)))
                                return (g_ntohl (header->packet_infos) & ARV_GVSP_PACKET_INFOS_N_PARTS_MASK);
                }
        }

        return 0;
}

static inline gboolean
arv_gvsp_leader_packet_get_multipart_infos (const ArvGvspPacket *packet,
                                            size_t packet_size,
                                            guint part_id,
                                            guint *purpose_id,
                                            ArvBufferPartDataType *data_type,
                                            size_t *size,
                                            ArvPixelFormat *pixel_format,
                                            guint32 *width,
                                            guint32 *height,
                                            guint32 *x_offset,
                                            guint32 *y_offset,
                                            guint32 *x_padding,
                                            guint32 *y_padding)
{
        unsigned int n_parts;
        ArvGvspMultipartLeader *leader;
        ArvGvspPartInfos *infos;

        g_return_val_if_fail (purpose_id != NULL, FALSE);
        g_return_val_if_fail (data_type != NULL, FALSE);
        g_return_val_if_fail (size != NULL, FALSE);
        g_return_val_if_fail (pixel_format != NULL, FALSE);
        g_return_val_if_fail (width != NULL, FALSE);
        g_return_val_if_fail (height != NULL, FALSE);
        g_return_val_if_fail (x_offset != NULL, FALSE);
        g_return_val_if_fail (y_offset != NULL, FALSE);
        g_return_val_if_fail (x_padding != NULL, FALSE);
        g_return_val_if_fail (y_padding != NULL, FALSE);

        if (G_LIKELY (arv_gvsp_leader_packet_get_buffer_payload_type (packet, packet_size, NULL) ==
                        ARV_BUFFER_PAYLOAD_TYPE_MULTIPART)) {
                n_parts = arv_gvsp_leader_packet_get_multipart_n_parts (packet, packet_size);

                if (G_LIKELY (part_id < n_parts)) {
                        leader = (ArvGvspMultipartLeader *) arv_gvsp_packet_get_data (packet, packet_size);

                        if (G_LIKELY(leader != NULL)) {
                                infos = &leader->parts[part_id];

                                *purpose_id = g_ntohs(infos->data_purpose_id);
                                *data_type = (ArvBufferPartDataType) g_ntohs (infos->data_type);
                                *size = g_ntohl (infos->part_length_low) + (((guint64) g_ntohs (infos->part_length_high)) << 32);
                                *pixel_format = g_ntohl (infos->pixel_format);
                                *width = g_ntohl (infos->width);
                                *height = g_ntohl (infos->height);
                                *x_offset = g_ntohl (infos->x_offset);
                                *y_offset = g_ntohl (infos->y_offset);
                                *x_padding = g_ntohs(infos->x_padding);
                                *y_padding = g_ntohs (infos->y_padding);

                                return TRUE;
                        }
                }
        }

        *purpose_id = 0;
        *data_type = 0;
        *size = 0;
        *pixel_format = 0;
        *width = 0;
        *height = 0;
        *x_offset = 0;
        *y_offset = 0;
        *x_padding = 0;
        *y_padding = 0;

        return FALSE;
}

static inline guint64
arv_gvsp_leader_packet_get_multipart_size (const ArvGvspPacket *packet,
                                           size_t packet_size,
                                           unsigned int part_id)
{
        unsigned int n_parts;
        ArvGvspMultipartLeader *leader;
        ArvGvspPartInfos *infos;

        if (G_LIKELY (arv_gvsp_leader_packet_get_buffer_payload_type (packet, packet_size, NULL) ==
                      ARV_BUFFER_PAYLOAD_TYPE_MULTIPART)) {
                n_parts = arv_gvsp_leader_packet_get_multipart_n_parts (packet, packet_size);

                if (G_LIKELY (part_id < n_parts)) {
                        leader = (ArvGvspMultipartLeader *) arv_gvsp_packet_get_data (packet, packet_size);

                        if (G_LIKELY (leader != NULL)) {
                            infos = &leader->parts[part_id];

                            return g_ntohl (infos->part_length_low) +
                                    (((guint64) g_ntohs (infos->part_length_high)) << 32);
                        }
                }
        }

        return 0;
}

static inline gboolean
arv_gvsp_leader_packet_get_image_infos (const ArvGvspPacket *packet,
                                        size_t packet_size,
                                        ArvPixelFormat *pixel_format,
                                        guint32 *width, guint32 *height,
                                        guint32 *x_offset, guint32 *y_offset,
                                        guint32 *x_padding, guint32 *y_padding)
{
        ArvBufferPayloadType payload_type;

        g_return_val_if_fail (pixel_format != NULL, FALSE);
        g_return_val_if_fail (width != NULL, FALSE);
        g_return_val_if_fail (height != NULL, FALSE);
        g_return_val_if_fail (x_offset != NULL, FALSE);
        g_return_val_if_fail (y_offset != NULL, FALSE);
        g_return_val_if_fail (x_padding != NULL, FALSE);
        g_return_val_if_fail (y_padding != NULL, FALSE);

        payload_type = arv_gvsp_leader_packet_get_buffer_payload_type (packet, packet_size, NULL);

        if (G_LIKELY (payload_type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE ||
                      payload_type == ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA)) {
                ArvGvspImageLeader *leader;

                leader = (ArvGvspImageLeader *) arv_gvsp_packet_get_data (packet, packet_size);

                if (G_LIKELY (leader != NULL)) {
                        *pixel_format = g_ntohl (leader->infos.pixel_format);
                        *width = g_ntohl (leader->infos.width);
                        *height = g_ntohl (leader->infos.height);
                        *x_offset = g_ntohl (leader->infos.x_offset);
                        *y_offset = g_ntohl (leader->infos.y_offset);
                        *x_padding = g_ntohs (leader->infos.x_padding);
                        *y_padding = g_ntohs (leader->infos.y_padding);

                        return TRUE;
                }
        }

        *pixel_format = 0;
        *width = 0;
        *height = 0;
        *x_offset = 0;
        *y_offset = 0;
        *x_padding = 0;
        *y_padding = 0;

        return FALSE;
}

static inline size_t
arv_gvsp_payload_packet_get_data_size (const ArvGvspPacket *packet, size_t packet_size)
{
        if (G_LIKELY(arv_gvsp_packet_get_content_type (packet, packet_size) == ARV_GVSP_CONTENT_TYPE_PAYLOAD)) {
                if (arv_gvsp_packet_has_extended_ids (packet, packet_size)) {
                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspExtendedHeader)))
                                return packet_size - sizeof (ArvGvspPacket) - sizeof (ArvGvspExtendedHeader);
                } else {
                        if (G_LIKELY(packet_size >= sizeof (ArvGvspPacket) + sizeof (ArvGvspHeader)))
                        return packet_size - sizeof (ArvGvspPacket) - sizeof (ArvGvspHeader);
                }
        }

        return 0;
}

static inline gboolean
arv_gvsp_multipart_packet_get_infos (const ArvGvspPacket *packet,  size_t packet_size,
                                     guint *part_id, ptrdiff_t *offset)
{
        ArvGvspMultipart *multipart;

        g_return_val_if_fail (part_id != NULL, FALSE);
        g_return_val_if_fail (offset != NULL, FALSE);

        if (G_LIKELY (arv_gvsp_packet_get_content_type (packet, packet_size) == ARV_GVSP_CONTENT_TYPE_MULTIPART)) {
                multipart = (ArvGvspMultipart *) arv_gvsp_packet_get_data(packet, packet_size);

                if (G_LIKELY (multipart != NULL) &&
                    arv_gvsp_packet_get_data_size(packet, packet_size) >= sizeof (ArvGvspMultipart)) {
                        *part_id = multipart->part_id;
                        *offset = ( (guint64) g_ntohs(multipart->offset_high) << 32) + g_ntohl(multipart->offset_low);
                        return TRUE;
                }
        }

        *part_id = 0;
        *offset = 0;
        return FALSE;
}

static inline size_t
arv_gvsp_multipart_packet_get_data_size (const ArvGvspPacket *packet, size_t packet_size)
{
        if (G_LIKELY (arv_gvsp_packet_get_content_type (packet, packet_size) == ARV_GVSP_CONTENT_TYPE_MULTIPART)) {
                if (arv_gvsp_packet_has_extended_ids (packet, packet_size)) {
                        if (G_LIKELY(packet_size >= (sizeof (ArvGvspPacket) +
                                                       sizeof (ArvGvspExtendedHeader) +
                                                       sizeof (ArvGvspMultipart))))
                                return packet_size -
                                        sizeof (ArvGvspPacket) -
                                        sizeof (ArvGvspExtendedHeader) -
                                        sizeof (ArvGvspMultipart);
                } else {
                        if (G_LIKELY(packet_size >= (sizeof (ArvGvspPacket) +
                                                      sizeof (ArvGvspHeader) +
                                                      sizeof (ArvGvspMultipart))))
                                return packet_size -
                                        sizeof (ArvGvspPacket) -
                                        sizeof (ArvGvspHeader) -
                                        sizeof (ArvGvspMultipart);
                }
        }

        return 0;
}

static inline void *
arv_gvsp_multipart_packet_get_data (const ArvGvspPacket *packet, size_t packet_size)
{
        if (G_LIKELY (arv_gvsp_packet_get_content_type (packet, packet_size) == ARV_GVSP_CONTENT_TYPE_MULTIPART)) {
                if (arv_gvsp_packet_has_extended_ids (packet, packet_size)) {
                        if (G_LIKELY(packet_size >= (sizeof (ArvGvspPacket) +
                                                      sizeof (ArvGvspExtendedHeader) +
                                                      sizeof (ArvGvspMultipart))))
                                return (char *) packet +
                                        sizeof (ArvGvspPacket) +
                                        sizeof (ArvGvspExtendedHeader) +
                                        sizeof (ArvGvspMultipart);
                } else {
                        if (G_LIKELY(packet_size >= (sizeof (ArvGvspPacket) +
                                                     sizeof (ArvGvspHeader) +
                                                     sizeof (ArvGvspMultipart))))
                                return (char *) packet +
                                        sizeof (ArvGvspPacket) +
                                        sizeof (ArvGvspHeader) +
                                        sizeof (ArvGvspMultipart);
                }
        }

        return NULL;
}

static inline guint64
arv_gvsp_timestamp_to_ns (guint64 timestamp, guint64 timestamp_tick_frequency)
{
	guint64 timestamp_s;
	guint64 timestamp_ns;

	if (timestamp_tick_frequency < 1)
		return 0;

	timestamp_s = timestamp / timestamp_tick_frequency;
	timestamp_ns = ((timestamp % timestamp_tick_frequency) * 1000000000) / timestamp_tick_frequency;

	timestamp_ns += timestamp_s * 1000000000;

	return timestamp_ns;
}

G_END_DECLS

#endif
