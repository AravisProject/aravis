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

#ifndef ARV_GENTL_DEVICE_PRIVATE_H
#define ARV_GENTL_DEVICE_PRIVATE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvgentldevice.h>
#include <arvgentlsystemprivate.h>

G_BEGIN_DECLS

ArvGenTLSystem *	arv_gentl_device_get_system                     (ArvGenTLDevice *device);
DS_HANDLE       	arv_gentl_device_open_stream_handle             (ArvGenTLDevice *device);
uint64_t	        arv_gentl_device_get_timestamp_tick_frequency   (ArvGenTLDevice *device);

G_END_DECLS

#endif
