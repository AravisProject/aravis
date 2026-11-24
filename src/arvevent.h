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

#ifndef ARV_EVENT_H
#define ARV_EVENT_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_TYPE_EVENT             (arv_event_get_type ())
ARV_API G_DECLARE_FINAL_TYPE (ArvEvent, arv_event, ARV, EVENT, GObject)

ARV_API guint		arv_event_get_id		(ArvEvent *event);
ARV_API guint64		arv_event_get_timestamp		(ArvEvent *event);
ARV_API guint64		arv_event_get_block_id		(ArvEvent *event);
ARV_API guint16		arv_event_get_stream_channel	(ArvEvent *event);
ARV_API GBytes *	arv_event_get_data		(ArvEvent *event);
ARV_API const void *	arv_event_peek_data		(ArvEvent *event, size_t *size);

G_END_DECLS

#endif
