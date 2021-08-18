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

#ifndef ARV_UV_DEVICE_H
#define ARV_UV_DEVICE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvtypes.h>
#include <arvdevice.h>

G_BEGIN_DECLS

/**
 * ArvUvUSBMode:
 * @ARV_UV_USB_MODE_SYNC: utilize libusb synchronous device I/O API
 * @ARV_UV_USB_MODE_ASYNC: utilize libusb asynchronous device I/O API
 */
typedef enum
{
	ARV_UV_USB_MODE_SYNC,
	ARV_UV_USB_MODE_ASYNC
} ArvUvUSBMode;

#define ARV_TYPE_UV_DEVICE             (arv_uv_device_get_type ())
G_DECLARE_FINAL_TYPE (ArvUvDevice, arv_uv_device, ARV, UV_DEVICE, ArvDevice)

ArvDevice * 	arv_uv_device_new 			(const char *vendor, const char *product, const char *serial_number,
							 GError **error);

void 		arv_uv_device_set_usb_mode		(ArvUvDevice *uv_device, ArvUvUSBMode usb_mode);

G_END_DECLS

#endif
