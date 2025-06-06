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

/*
 * SECTION: arvgvsp
 * @short_description: GigEVision stream packet handling
 */

#include <arvdebug.h>
#include <arvmiscprivate.h>
#include <arvenumtypes.h>
#include <arvgvspprivate.h>
#include <arvenumtypesprivate.h>
#include <stddef.h>
#include <string.h>

static ArvGvspPacket *
arv_gvsp_packet_new (ArvGvspContentType content_type,
		     guint16 frame_id, guint32 packet_id, size_t data_size, void *buffer, size_t buffer_size,
                     size_t *packet_size)
{
	ArvGvspPacket *packet;
	ArvGvspHeader *header;
	size_t size;

	size = sizeof (ArvGvspPacket) + sizeof (ArvGvspHeader) + data_size;
	if (buffer != NULL && size > buffer_size) {
                if (packet_size != NULL)
                        *packet_size = 0;
		return NULL;
        }

	if (packet_size != NULL)
		*packet_size = size;

	if (buffer != NULL)
		packet = buffer;
	else
		packet = g_malloc (size);

	packet->status = ARV_GVSP_PACKET_STATUS_SUCCESS;

	header = (void *) &packet->header;
	header->frame_id = g_htons (frame_id);
	header->packet_infos = g_htonl ((packet_id & ARV_GVSP_PACKET_ID_MASK) |
					((content_type << ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_POS) &
					 ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_MASK));

	return packet;
}

ArvGvspPacket *
arv_gvsp_packet_new_image_leader (guint16 frame_id, guint32 packet_id,
                                  guint64 timestamp, ArvPixelFormat pixel_format,
                                  guint32 width, guint32 height,
                                  guint32 x_offset, guint32 y_offset,
                                  guint32 x_padding, guint32 y_padding,
                                  void *buffer, size_t buffer_size,
                                  size_t *packet_size)
{
        ArvGvspPacket *packet;
        size_t size;

	packet = arv_gvsp_packet_new (ARV_GVSP_CONTENT_TYPE_LEADER,
				      frame_id, packet_id, sizeof (ArvGvspImageLeader), buffer, buffer_size, &size);

        if (packet_size != NULL)
                *packet_size = size;

	if (packet != NULL) {
		ArvGvspImageLeader *leader;

		leader = arv_gvsp_packet_get_data (packet, size);
		leader->flags = 0;
		leader->payload_type = g_htons (ARV_BUFFER_PAYLOAD_TYPE_IMAGE);
		leader->timestamp_high = g_htonl (((guint64) timestamp >> 32));
		leader->timestamp_low  = g_htonl ((guint64) timestamp & 0xffffffff);
		leader->infos.pixel_format = g_htonl (pixel_format);
		leader->infos.width = g_htonl (width);
		leader->infos.height = g_htonl (height);
		leader->infos.x_offset = g_htonl (x_offset);
		leader->infos.y_offset = g_htonl (y_offset);
		leader->infos.x_padding = g_htonl (x_padding);
		leader->infos.y_padding = g_htonl (y_padding);
	}

	return packet;
}

ArvGvspPacket *
arv_gvsp_packet_new_data_trailer (guint16 frame_id, guint32 packet_id, guint32 height,
				  void *buffer, size_t buffer_size,
                                  size_t *packet_size)
{
	ArvGvspPacket *packet;
        size_t size;

	packet = arv_gvsp_packet_new (ARV_GVSP_CONTENT_TYPE_TRAILER,
				      frame_id, packet_id, sizeof (ArvGvspTrailer), buffer, buffer_size, &size);

        if (packet_size != NULL)
                *packet_size = size;

	if (packet != NULL) {
		ArvGvspTrailer *trailer;

		trailer = arv_gvsp_packet_get_data (packet, size);
		trailer->payload_type = g_htonl (ARV_BUFFER_PAYLOAD_TYPE_IMAGE);
		trailer->data0 = g_htonl (height);
	}

	return packet;
}

ArvGvspPacket *
arv_gvsp_packet_new_payload (guint16 frame_id, guint32 packet_id,
                             size_t payload_size, void *data,
                             void *buffer, size_t buffer_size,
                             size_t *packet_size)
{
        ArvGvspPacket *packet;
        size_t size;

	packet = arv_gvsp_packet_new (ARV_GVSP_CONTENT_TYPE_PAYLOAD,
				      frame_id, packet_id, payload_size, buffer, buffer_size, &size);

        if (packet_size != NULL)
                *packet_size = size;

	if (packet != NULL)
		memcpy (arv_gvsp_packet_get_data (packet, size), data, payload_size);

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
arv_gvsp_packet_status_to_string (ArvGvspPacketStatus value)
{
	return arv_enum_to_string (ARV_TYPE_GVSP_PACKET_STATUS, value);
}

static const char *
arv_gvsp_content_type_to_string (ArvGvspContentType value)
{
	return arv_enum_to_string (ARV_TYPE_GVSP_CONTENT_TYPE, value);
}

char *
arv_gvsp_packet_to_string (const ArvGvspPacket *packet, size_t packet_size)
{
	ArvGvspPacketStatus packet_status;
        ArvBufferPayloadType payload_type;
	ArvGvspContentType content_type;
        guint part_id;
        ptrdiff_t offset;
	GString *string;

	string = g_string_new ("");

	packet_status = arv_gvsp_packet_get_status (packet, packet_size);
	content_type = arv_gvsp_packet_get_content_type (packet, packet_size);

	g_string_append_printf (string, "packet_type  = %8s (0x%04x)\n",
                                arv_gvsp_packet_status_to_string (packet_status), packet_status);
	g_string_append_printf (string, "content_type = %8s (0x%04x)\n",
                                arv_gvsp_content_type_to_string (content_type), content_type);
	g_string_append_printf (string, "frame_id     = %8" G_GUINT64_FORMAT " %s\n",
				arv_gvsp_packet_get_frame_id (packet, packet_size),
				arv_gvsp_packet_has_extended_ids (packet, packet_size) ? " extended" : "");
	g_string_append_printf (string, "packet_id    = %8u\n",
                                arv_gvsp_packet_get_packet_id (packet, packet_size));
	g_string_append_printf (string, "data_size    = %8" G_GSIZE_FORMAT "\n",
                                arv_gvsp_packet_get_data_size (packet, packet_size));

	switch (content_type) {
		case ARV_GVSP_CONTENT_TYPE_LEADER:
                        payload_type = arv_gvsp_leader_packet_get_buffer_payload_type (packet, packet_size, NULL);
			switch (payload_type) {
				case ARV_BUFFER_PAYLOAD_TYPE_IMAGE:
					g_string_append (string, "payload_type = image\n");
					break;
				case ARV_BUFFER_PAYLOAD_TYPE_CHUNK_DATA:
					g_string_append (string, "payload_type = chunk\n");
					break;
				case ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA:
					g_string_append (string, "payload_type = extended chunk\n");
					break;
				case ARV_BUFFER_PAYLOAD_TYPE_H264:
					g_string_append (string, "payload_type = h264\n");
					break;
				case ARV_BUFFER_PAYLOAD_TYPE_MULTIPART:
					g_string_append (string, "payload_type = multipart\n");
					break;
				case ARV_BUFFER_PAYLOAD_TYPE_JPEG:
					g_string_append (string, "payload_type = jpeg\n");
					break;
				case ARV_BUFFER_PAYLOAD_TYPE_JPEG2000:
					g_string_append (string, "payload_type = jpeg2000\n");
					break;
				case ARV_BUFFER_PAYLOAD_TYPE_RAWDATA:
					g_string_append (string, "payload_type = raw data\n");
					break;
				case ARV_BUFFER_PAYLOAD_TYPE_FILE:
					g_string_append (string, "payload_type = file\n");
					break;
				case ARV_BUFFER_PAYLOAD_TYPE_MULTIZONE_IMAGE:
					g_string_append (string, "payload_type = multizone image\n");
					break;
				default:
					g_string_append_printf (string, "payload_type = unknown (0x%08x)\n",
                                                                payload_type);
					break;
			}

                        if (payload_type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE ||
                            payload_type == ARV_BUFFER_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA) {
                                ArvPixelFormat pixel_format;
                                guint32 width, height, x_offset, y_offset, x_padding, y_padding;

                                if (arv_gvsp_leader_packet_get_image_infos (packet, packet_size,
                                                                            &pixel_format,
                                                                            &width, &height, &x_offset, &y_offset,
                                                                            &x_padding, &y_padding)) {
                                        g_string_append_printf (string, "pixel format = %s\n",
                                                                arv_pixel_format_to_gst_caps_string (pixel_format));
                                        g_string_append_printf (string, "width        = %8d\n", width);
                                        g_string_append_printf (string, "height       = %8d\n", height);
                                        g_string_append_printf (string, "x_offset     = %8d\n", x_offset);
                                        g_string_append_printf (string, "y_offset     = %8d\n", y_offset);
                                        g_string_append_printf (string, "x_padding    = %8d\n", x_padding);
                                        g_string_append_printf (string, "y_padding    = %8d\n", y_padding);
                                }
                        } else if (payload_type == ARV_BUFFER_PAYLOAD_TYPE_MULTIPART) {
                                g_string_append_printf (string, "n_parts      = %8u\n",
                                                        arv_gvsp_leader_packet_get_multipart_n_parts (packet,
                                                                                                      packet_size));
                        }
                        break;
                case ARV_GVSP_CONTENT_TYPE_TRAILER:
                        break;
                case ARV_GVSP_CONTENT_TYPE_PAYLOAD:
                        break;
                case ARV_GVSP_CONTENT_TYPE_H264:
                        break;
                case ARV_GVSP_CONTENT_TYPE_MULTIZONE:
                        break;
                case ARV_GVSP_CONTENT_TYPE_MULTIPART:
                        if (arv_gvsp_multipart_packet_get_infos (packet, packet_size, &part_id, &offset)) {
                                g_string_append_printf (string, "part_id      = %8d\n", part_id);
                                g_string_append_printf (string, "offset       = %8zu\n", offset);
                        }
                        break;
                case ARV_GVSP_CONTENT_TYPE_GENDC:
                        break;
                case ARV_GVSP_CONTENT_TYPE_ALL_IN:
                        break;
        }

        return arv_g_string_free_and_steal(string);
}

void
arv_gvsp_packet_debug (const ArvGvspPacket *packet, size_t packet_size, ArvDebugLevel level)
{
	char *string;

	if (!arv_debug_check (ARV_DEBUG_CATEGORY_SP, level))
		return;

	string = arv_gvsp_packet_to_string (packet, packet_size);
	switch (level) {
		case ARV_DEBUG_LEVEL_TRACE:
			arv_trace_sp ("%s", string);
			break;
		case ARV_DEBUG_LEVEL_DEBUG:
			arv_debug_sp ("%s", string);
			break;
		case ARV_DEBUG_LEVEL_INFO:
			arv_info_sp ("%s", string);
			break;
		case ARV_DEBUG_LEVEL_WARNING:
			arv_warning_sp ("%s", string);
			break;
		default:
			break;
	}
	g_free (string);
}
