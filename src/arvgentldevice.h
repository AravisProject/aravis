/* Aravis - Digital camera library
 *
 * Copyright © 2023 Xiaoqiang Wang
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
 * Author: Xiaoqiang Wang <xiaoqiang.wang@psi.ch>
 */

#ifndef ARV_GENTL_DEVICE_H
#define ARV_GENTL_DEVICE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvdevice.h>
#include <arvgentlsystem.h>

G_BEGIN_DECLS

#define ARV_TYPE_GENTL_DEVICE (arv_gentl_device_get_type ())
ARV_API G_DECLARE_FINAL_TYPE (ArvGenTLDevice, arv_gentl_device, ARV, GENTL_DEVICE, ArvDevice)

ARV_API ArvDevice *	arv_gentl_device_new 	(ArvGenTLSystem *gentl_system,
                                                 const char *interface_id, const char *device_id,
                                                 GError **error);

G_END_DECLS

#endif
