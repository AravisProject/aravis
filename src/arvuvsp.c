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

#include <arvuvspprivate.h>
#include <arvstr.h>
#include <arvmiscprivate.h>

/*
 * SECTION: arvuvsp
 * @short_description: USB3Vision stream packet handling
 */

/**
 * arv_uvsp_packet_to_string:
 * @packet: a #ArvUvspPacket
 *
 * Converts @packet into a human readable string.
 *
 * return value: (transfer full): A newly allocated string.
 */

char *
arv_uvsp_packet_to_string (const ArvUvspPacket *packet)
{
	ArvUvspLeader *leader = (ArvUvspLeader *) packet;
	ArvUvspTrailer *trailer = (ArvUvspTrailer *) packet;
	GString *string;

	g_return_val_if_fail (packet != NULL, NULL);

	string = g_string_new ("");

	switch (GUINT32_FROM_LE (packet->header.magic)) {
		case ARV_UVSP_LEADER_MAGIC:
			g_string_append (string, "packet_type  = leader\n");
			g_string_append_printf (string, "size         = %d\n", GUINT16_FROM_LE (packet->header.size));
			g_string_append_printf (string, "frame id     = %" G_GUINT64_FORMAT "\n",
						GUINT64_FROM_LE (packet->header.frame_id));
			switch (GUINT16_FROM_LE (leader->infos.payload_type)) {
				case ARV_BUFFER_PAYLOAD_TYPE_NO_DATA:
					g_string_append (string, "payload_type = no data\n");
                                        break;
				case ARV_BUFFER_PAYLOAD_TYPE_IMAGE:
					g_string_append (string, "payload_type = image\n");
					break;
				default:
					g_string_append (string, "payload_type = unknown\n");
					break;
			}
			g_string_append_printf (string, "pixel format = %s\n",
						arv_pixel_format_to_gst_caps_string (GUINT32_FROM_LE (leader->infos.pixel_format)));
			g_string_append_printf (string, "width        = %d\n",
						GUINT16_FROM_LE (leader->infos.width));
			g_string_append_printf (string, "height       = %d\n",
						GUINT16_FROM_LE (leader->infos.height));
			g_string_append_printf (string, "x_offset     = %d\n",
						GUINT16_FROM_LE (leader->infos.x_offset));
			g_string_append_printf (string, "y_offset     = %d",
						GUINT16_FROM_LE (leader->infos.y_offset));
			break;
		case ARV_UVSP_TRAILER_MAGIC:
			g_string_append (string, "packet_type  = trailer\n");
			g_string_append_printf (string, "size         = %d\n", GUINT16_FROM_LE (packet->header.size));
			g_string_append_printf (string, "frame id     = %" G_GUINT64_FORMAT "\n",
						GUINT64_FROM_LE (packet->header.frame_id));
			g_string_append_printf (string, "payload_size = %" G_GUINT64_FORMAT "",
						GUINT64_FROM_LE (trailer->infos.payload_size));
			break;
		default:
			g_string_append (string, "packet_type  = image");
			break;
	}

#if 0
	{
		size_t packet_size;

		packet_size = sizeof (ArvUvspHeader) + GUINT16_FROM_LE (packet->header.size);

		arv_g_string_append_hex_dump (string, packet, packet_size);
	}
#endif

        return arv_g_string_free_and_steal(string);
}

/**
 * arv_uvsp_packet_debug:
 * @packet: a #ArvUvspPacket
 * @level: debug level
 *
 * Dumps the content of @packet if level is lower or equal to the current debug level for the sp debug category. See arv_debug_enable().
 */

void
arv_uvsp_packet_debug (const ArvUvspPacket *packet, ArvDebugLevel level)
{
	char *string;

	if (!arv_debug_check (ARV_DEBUG_CATEGORY_SP, level))
		return;

	string = arv_uvsp_packet_to_string (packet);
	switch (level) {
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
