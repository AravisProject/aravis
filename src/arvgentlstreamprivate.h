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
 * Author: Xiaoqiang Wang <xiaoqiang.wang@psi.ch>
 */

#ifndef ARV_GENTL_STREAM_PRIVATE_H
#define ARV_GENTL_STREAM_PRIVATE_H

#include <arvgentldeviceprivate.h>
#include <arvgentlstream.h>
#include <arvstream.h>


G_BEGIN_DECLS

#define ARV_GENTL_STREAM_MIN_N_BUFFERS 3
#define ARV_GENTL_STREAM_MAX_N_BUFFERS 1000
#define ARV_GENTL_STREAM_DEFAULT_N_BUFFERS 10


ArvStream * 	arv_gentl_stream_new		        (ArvGenTLDevice *gentl_device,
                                                         ArvStreamCallback callback, void *callback_data,
                                                         GDestroyNotify destroy, GError **error);

G_END_DECLS

#endif
