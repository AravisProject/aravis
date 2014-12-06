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

#ifndef ARV_GV_STREAM_H
#define ARV_GV_STREAM_H

#include <arvtypes.h>
#include <arvstream.h>
#include <gio/gio.h>

G_BEGIN_DECLS

/**
 * ArvGvStreamSocketBuffer:
 * @ARV_GV_STREAM_SOCKET_BUFFER_FIXED: socket buffer is set to a given fixed value
 * @ARV_GV_STREAM_SOCKET_BUFFER_AUTO: sockect buffer is set with respect to the payload size
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
#define ARV_GV_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GV_STREAM, ArvGvStream))
#define ARV_GV_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GV_STREAM, ArvGvStreamClass))
#define ARV_IS_GV_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GV_STREAM))
#define ARV_IS_GV_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GV_STREAM))
#define ARV_GV_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GV_STREAM, ArvGvStreamClass))

typedef struct _ArvGvStreamPrivate ArvGvStreamPrivate;
typedef struct _ArvGvStreamClass ArvGvStreamClass;

struct _ArvGvStream {
	ArvStream	stream;

	ArvGvStreamPrivate *priv;
};

struct _ArvGvStreamClass {
	ArvStreamClass parent_class;
};

GType arv_gv_stream_get_type (void);

ArvStream * 	arv_gv_stream_new			(GInetAddress *device_address, guint16 port,
							 ArvStreamCallback callback, void *user_data,
							 guint64 timestamp_tick_frequency,
							 guint packet_size);
guint16 	arv_gv_stream_get_port			(ArvGvStream *gv_stream);

void		arv_gv_stream_get_statistics		(ArvGvStream *gv_stream,
							 guint64 *n_resent_packets,
							 guint64 *n_missing_packets);

G_END_DECLS

#endif
