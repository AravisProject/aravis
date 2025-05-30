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

#ifndef ARV_GV_STREAM_PRIVATE_H
#define ARV_GV_STREAM_PRIVATE_H

#include <arvgvstream.h>
#include <arvstream.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define ARV_GV_STREAM_POLL_TIMEOUT_US			1000000
#define ARV_GV_STREAM_INITIAL_PACKET_TIMEOUT_US_DEFAULT	1000
#define ARV_GV_STREAM_PACKET_TIMEOUT_US_DEFAULT		20000
#define ARV_GV_STREAM_FRAME_RETENTION_US_DEFAULT	100000
#define ARV_GV_STREAM_PACKET_REQUEST_RATIO_DEFAULT	0.25

ArvStream * 	arv_gv_stream_new		(ArvGvDevice *gv_device, ArvStreamCallback callback, void *callback_data, GDestroyNotify destroy, GError **error);

G_END_DECLS

#endif
