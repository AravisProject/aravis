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

#ifndef ARV_EVENT_PRIVATE_H
#define ARV_EVENT_PRIVATE_H

#include <arvevent.h>

G_BEGIN_DECLS

#define ARV_EVENT_STREAM_CHANNEL_NONE	0xffff

ARV_API ArvEvent *	arv_event_new	(guint id, guint64 timestamp, guint64 block_id, guint16 stream_channel,
					 const void *data, size_t size);

G_END_DECLS

#endif
