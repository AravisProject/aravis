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

#ifndef ARV_GV_STREAM_H
#define ARV_GV_STREAM_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvstream.h>

G_BEGIN_DECLS

/**
 * ArvGvStreamOption:
 * @ARV_GV_STREAM_OPTION_NONE: no option specified
 * @ARV_GV_STREAM_OPTION_PACKET_SOCKET_DISABLED: use of packet socket is disabled
 */

typedef enum {
	ARV_GV_STREAM_OPTION_NONE =                             0,
	ARV_GV_STREAM_OPTION_PACKET_SOCKET_DISABLED =           1 << 0,
} ArvGvStreamOption;

/**
 * ArvGvStreamSocketBuffer:
 * @ARV_GV_STREAM_SOCKET_BUFFER_FIXED: socket buffer is set to a given fixed value
 * @ARV_GV_STREAM_SOCKET_BUFFER_AUTO: socket buffer size is set to the payload size
 */

typedef enum {
	ARV_GV_STREAM_SOCKET_BUFFER_FIXED,
	ARV_GV_STREAM_SOCKET_BUFFER_AUTO
} ArvGvStreamSocketBuffer;

/**
 * ArvGvStreamPacketResend:
 * @ARV_GV_STREAM_PACKET_RESEND_NEVER: never request a packet resend
 * @ARV_GV_STREAM_PACKET_RESEND_ALWAYS: request a packet resend if a packet was missing
 */

typedef enum {
	ARV_GV_STREAM_PACKET_RESEND_NEVER,
	ARV_GV_STREAM_PACKET_RESEND_ALWAYS
} ArvGvStreamPacketResend;

#define ARV_TYPE_GV_STREAM             (arv_gv_stream_get_type ())
ARV_API G_DECLARE_FINAL_TYPE (ArvGvStream, arv_gv_stream, ARV, GV_STREAM, ArvStream)

ARV_API guint16		arv_gv_stream_get_port		(ArvGvStream *gv_stream);
ARV_API void		arv_gv_stream_get_statistics	(ArvGvStream *gv_stream,
							 guint64 *n_resent_packets,
							 guint64 *n_missing_packets);

G_END_DECLS

#endif
