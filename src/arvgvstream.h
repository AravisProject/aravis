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

#ifndef ARV_GV_STREAM_H
#define ARV_GV_STREAM_H

#include <arvstream.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef enum {
	ARV_GV_STREAM_OPTION_SOCKET_BUFFER_FIXED,
	ARV_GV_STREAM_OPTION_SOCKET_BUFFER_AUTO
} ArvGvStreamOption;

#define ARV_TYPE_GV_STREAM             (arv_gv_stream_get_type ())
#define ARV_GV_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GV_STREAM, ArvGvStream))
#define ARV_GV_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GV_STREAM, ArvGvStreamClass))
#define ARV_IS_GV_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GV_STREAM))
#define ARV_IS_GV_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GV_STREAM))
#define ARV_GV_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GV_STREAM, ArvGvStreamClass))

typedef struct _ArvGvStreamClass ArvGvStreamClass;

struct _ArvGvStream {
	ArvStream	stream;

	GSocket *socket;
	GSocketAddress *incoming_address;

	GThread *thread;
	void *thread_data;
};

struct _ArvGvStreamClass {
	ArvStreamClass parent_class;
};

GType arv_gv_stream_get_type (void);

ArvStream * 		arv_gv_stream_new 		(GInetAddress *device_address, guint16 port);
guint16 		arv_gv_stream_get_port		(ArvGvStream *gv_stream);
void			arv_gv_stream_set_option	(ArvGvStream *gv_stream, ArvGvStreamOption option,
							 int value);

G_END_DECLS

#endif
