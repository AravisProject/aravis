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

#ifndef ARV_UV_STREAM_PRIVATE_H
#define ARV_UV_STREAM_PRIVATE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvuvstream.h>
#include <arvstream.h>
#include <arvuvdeviceprivate.h>

G_BEGIN_DECLS

#define ARV_UV_STREAM_MAXIMUM_TRANSFER_SIZE_DEFAULT	(1*1024*1024)
#define ARV_UV_STREAM_N_MAXIMUM_SUBMITS                 8

ArvStream * 	arv_uv_stream_new	(ArvUvDevice *uv_device,
                                         ArvStreamCallback callback, void *user_data, GDestroyNotify destroy,
                                         ArvUvUsbMode usb_mode, guint64 maximum_transfer_size, GError **error);

G_END_DECLS

#endif
