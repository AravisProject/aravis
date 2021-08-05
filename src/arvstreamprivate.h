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

#ifndef ARV_STREAM_PRIVATE_H
#define ARV_STREAM_PRIVATE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvstream.h>

G_BEGIN_DECLS

ArvBuffer *	arv_stream_pop_input_buffer		(ArvStream *stream);
void		arv_stream_push_output_buffer		(ArvStream *stream, ArvBuffer *buffer);
void		arv_stream_take_init_error		(ArvStream *device, GError *error);

void            arv_stream_declare_info                 (ArvStream *stream, const char *name, GType type, gpointer data);

G_END_DECLS

#endif
