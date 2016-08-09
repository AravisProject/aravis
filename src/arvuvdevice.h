/* Aravis - Digital camera library
 *
 * Copyright © 2009-2016 Emmanuel Pacaud
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

#include <arvtypes.h>
#include <arvdevice.h>

G_BEGIN_DECLS

#define ARV_TYPE_UV_DEVICE             (arv_uv_device_get_type ())
#define ARV_UV_DEVICE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_UV_DEVICE, ArvUvDevice))
#define ARV_UV_DEVICE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_UV_DEVICE, ArvUvDeviceClass))
#define ARV_IS_UV_DEVICE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_UV_DEVICE))
#define ARV_IS_UV_DEVICE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_UV_DEVICE))
#define ARV_UV_DEVICE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_UV_DEVICE, ArvUvDeviceClass))

typedef struct _ArvUvDeviceClass ArvUvDeviceClass;
typedef struct _ArvUvDevicePrivate ArvUvDevicePrivate;

struct _ArvUvDevice {
	ArvDevice device;

	ArvUvDevicePrivate *priv;
};

struct _ArvUvDeviceClass {
	ArvDeviceClass parent_class;
};

GType arv_uv_device_get_type (void);

ArvDevice * 	arv_uv_device_new 			(const char *vendor, const char *product, const char *serial_nbr);

G_END_DECLS

#endif
