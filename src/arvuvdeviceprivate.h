/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_UV_DEVICE_PRIVATE_H
#define ARV_UV_DEVICE_PRIVATE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvuvdevice.h>
#include <arvdeviceprivate.h>
#include <libusb.h>

G_BEGIN_DECLS

typedef enum {
	ARV_UV_ENDPOINT_CONTROL,
	ARV_UV_ENDPOINT_DATA
} ArvUvEndpointType;

gboolean        arv_uv_device_bulk_transfer             (ArvUvDevice *uv_device,
							 ArvUvEndpointType endpoint_type, unsigned char endpoint_flags,
							 void *data, size_t size, size_t *transferred_size,
							 guint32 timeout_ms, GError **error);

void            arv_uv_device_fill_bulk_transfer        (struct libusb_transfer* transfer, ArvUvDevice *uv_device,
                                                         ArvUvEndpointType endpoint_type, unsigned char endpoint_flags,
                                                         void *data, size_t size,
                                                         libusb_transfer_cb_fn callback, void* callback_data,
                                                         unsigned int timeout);

void *          arv_uv_device_usb_mem_alloc             (ArvUvDevice *uv_device, size_t size);
void            arv_uv_device_usb_mem_free              (ArvUvDevice *uv_device, void *data, size_t size);

gboolean        arv_uv_device_is_connected              (ArvUvDevice *uv_device);

gboolean        arv_uv_device_reset_stream_endpoint     (ArvUvDevice *device);

G_END_DECLS

#endif
