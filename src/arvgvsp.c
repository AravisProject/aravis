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

/**
 * SECTION: arvgvsp
 * @short_description: GigEVision stream packet handling
 */

#include <arvgvsp.h>
#include <arvdebug.h>
#include <arvenumtypes.h>
#include <string.h>
#include <arvmisc.h>

static ArvGvspPacket *
arv_gvsp_packet_new (ArvGvspContentType content_type,
		     guint16 frame_id, guint32 packet_id, size_t data_size, void *buffer, size_t *buffer_size)
{
	ArvGvspPacket *packet;
	size_t packet_size;

	packet_size = sizeof (ArvGvspPacket) + data_size;
	if (packet_size == 0 || (buffer != NULL && (buffer_size == NULL || packet_size > *buffer_size)))
		return NULL;

	if (buffer_size != NULL)
		*buffer_size = packet_size;

	if (buffer != NULL)
		packet = buffer;
	else
		packet = g_malloc (packet_size);

	packet->header.packet_type = 0;
	packet->header.frame_id = g_htons (frame_id);
	packet->header.packet_infos = g_htonl ((packet_id & ARV_GVSP_PACKET_INFOS_ID_MASK) |
					       ((content_type << ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_POS) &
						ARV_GVSP_PACKET_INFOS_CONTENT_TYPE_MASK));

	return packet;
}

ArvGvspPacket *
arv_gvsp_packet_new_data_leader	(guint16 frame_id, guint32 packet_id,
				 guint64 timestamp, ArvPixelFormat pixel_format,
				 guint32 width, guint32 height,
				 guint32 x_offset, guint32 y_offset,
				 void *buffer, size_t *buffer_size)
{
	ArvGvspPacket *packet;

	packet = arv_gvsp_packet_new (ARV_GVSP_CONTENT_TYPE_DATA_LEADER,
				      frame_id, packet_id, sizeof (ArvGvspDataLeader), buffer, buffer_size);

	if (packet != NULL) {
		ArvGvspDataLeader *leader;

		leader = (ArvGvspDataLeader *) &packet->data;
		leader->flags = 0;
		leader->payload_type = g_htons (ARV_GVSP_PAYLOAD_TYPE_IMAGE);
		leader->timestamp_high = g_htonl (((guint64) timestamp >> 32));
		leader->timestamp_low  = g_htonl ((guint64) timestamp & 0xffffffff);
		leader->pixel_format = g_htonl (pixel_format);
		leader->width = g_htonl (width);
		leader->height = g_htonl (height);
		leader->x_offset = g_htonl (x_offset);
		leader->y_offset = g_htonl (y_offset);
	}

	return packet;
}

ArvGvspPacket *
arv_gvsp_packet_new_data_trailer (guint16 frame_id, guint32 packet_id,
				  void *buffer, size_t *buffer_size)
{
	ArvGvspPacket *packet;

	packet = arv_gvsp_packet_new (ARV_GVSP_CONTENT_TYPE_DATA_TRAILER,
				      frame_id, packet_id, sizeof (ArvGvspDataTrailer), buffer, buffer_size);

	if (packet != NULL) {
		ArvGvspDataTrailer *trailer;

		trailer = (ArvGvspDataTrailer *) &packet->data;
		trailer->payload_type = g_htonl (ARV_GVSP_PAYLOAD_TYPE_IMAGE);
		trailer->data0 = 0;
	}

	return packet;
}

ArvGvspPacket *
arv_gvsp_packet_new_data_block (guint16 frame_id, guint32 packet_id,
				size_t size, void *data,
				void *buffer, size_t *buffer_size)
{
	ArvGvspPacket *packet;

	packet = arv_gvsp_packet_new (ARV_GVSP_CONTENT_TYPE_DATA_BLOCK,
				      frame_id, packet_id, size, buffer, buffer_size);

	if (packet != NULL)
		memcpy (&packet->data, data, size);

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
arv_gvsp_packet_type_to_string (ArvGvspPacketType value)
{
	return arv_enum_to_string (ARV_TYPE_GVSP_PACKET_TYPE, value);
}

static const char *
arv_gvsp_content_type_to_string (ArvGvspContentType value)
{
	return arv_enum_to_string (ARV_TYPE_GVSP_CONTENT_TYPE, value);
}

char *
arv_gvsp_packet_to_string (const ArvGvspPacket *packet, size_t packet_size)
{
	ArvGvspDataLeader *leader;
	ArvGvspPacketType packet_type;
	ArvGvspContentType content_type;
	GString *string;
	char *c_string;

	string = g_string_new ("");

	packet_type = arv_gvsp_packet_get_packet_type (packet);
	content_type = arv_gvsp_packet_get_content_type (packet);

	g_string_append_printf (string, "packet_type  = %8s (0x%04x)\n", arv_gvsp_packet_type_to_string (packet_type), packet_type);
	g_string_append_printf (string, "content_type = %8s (0x%04x)\n", arv_gvsp_content_type_to_string (content_type), content_type);

	switch (content_type) {
		case ARV_GVSP_CONTENT_TYPE_DATA_LEADER:
			leader = (ArvGvspDataLeader *) &packet->data;
			switch (g_ntohl (leader->payload_type)) {
				case ARV_GVSP_PAYLOAD_TYPE_IMAGE:
					g_string_append (string, "payload_type = image\n");
					break;
				case ARV_GVSP_PAYLOAD_TYPE_CHUNK_DATA:
					g_string_append (string, "payload_type = chunk\n");
					break;
				case ARV_GVSP_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA:
					g_string_append (string, "payload_type = extended chunk\n");
					break;
				case ARV_GVSP_PAYLOAD_TYPE_H264:
					g_string_append (string, "payload_type = h264\n");
					break;
				default:
					g_string_append (string, "payload_type = unknown\n");
					break;
			}
			g_string_append_printf (string, "pixel format = %s\n",
						arv_pixel_format_to_gst_caps_string (g_ntohl (leader->pixel_format)));
			g_string_append_printf (string, "width        = %d\n", g_ntohl (leader->width));
			g_string_append_printf (string, "height       = %d\n", g_ntohl (leader->height));
			g_string_append_printf (string, "x_offset     = %d\n", g_ntohl (leader->x_offset));
			g_string_append_printf (string, "y_offset     = %d\n", g_ntohl (leader->y_offset));
			break;
		case ARV_GVSP_CONTENT_TYPE_DATA_TRAILER:
			break;
		case ARV_GVSP_CONTENT_TYPE_DATA_BLOCK:
			break;
	}

	c_string = string->str;

	g_string_free (string, FALSE);

	return c_string;
}

void
arv_gvsp_packet_debug (const ArvGvspPacket *packet, size_t packet_size, ArvDebugLevel level)
{
	char *string;

	if (!arv_debug_check (&arv_debug_category_sp, level))
		return;

	string = arv_gvsp_packet_to_string (packet, packet_size);
	switch (level) {
		case ARV_DEBUG_LEVEL_LOG:
			arv_log_sp ("%s", string);
			break;
		case ARV_DEBUG_LEVEL_DEBUG:
			arv_debug_sp ("%s", string);
			break;
		case ARV_DEBUG_LEVEL_WARNING:
			arv_warning_sp ("%s", string);
			break;
		default:
			break;
	}
	g_free (string);
}
