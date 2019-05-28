/* Aravis - Digital camera library
 *
 * Copyright © 2009-2019 Emmanuel Pacaud
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

#ifndef ARV_GV_STREAM_PRIVATE_H
#define ARV_GV_STREAM_PRIVATE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvgvstream.h>
#include <arvstream.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _ArvGvStreamPrivate ArvGvStreamPrivate;
typedef struct _ArvGvStreamClass ArvGvStreamClass;

struct _ArvGvStream {
	ArvStream	stream;

	ArvGvStreamPrivate *priv;
};

struct _ArvGvStreamClass {
	ArvStreamClass parent_class;
};

ArvStream * 	arv_gv_stream_new			(ArvGvDevice *gv_device,
							 GInetAddress *interface_address,
							 GInetAddress *device_address,
							 ArvStreamCallback callback, void *user_data);

G_END_DECLS

#endif
