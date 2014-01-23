/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
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

#ifndef ARV_FAKE_DEVICE_H
#define ARV_FAKE_DEVICE_H

#include <arvtypes.h>
#include <arvdevice.h>

G_BEGIN_DECLS

#define ARV_TYPE_FAKE_DEVICE             (arv_fake_device_get_type ())
#define ARV_FAKE_DEVICE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_FAKE_DEVICE, ArvFakeDevice))
#define ARV_FAKE_DEVICE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_FAKE_DEVICE, ArvFakeDeviceClass))
#define ARV_IS_FAKE_DEVICE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_FAKE_DEVICE))
#define ARV_IS_FAKE_DEVICE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_FAKE_DEVICE))
#define ARV_FAKE_DEVICE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_FAKE_DEVICE, ArvFakeDeviceClass))

typedef struct _ArvFakeDeviceClass ArvFakeDeviceClass;
typedef struct _ArvFakeDevicePrivate ArvFakeDevicePrivate;

struct _ArvFakeDevice {
	ArvDevice device;

	ArvFakeDevicePrivate *priv;
};

struct _ArvFakeDeviceClass {
	ArvDeviceClass parent_class;
};

GType arv_fake_device_get_type (void);

ArvDevice * 	arv_fake_device_new 			(const char *serial_number);

ArvFakeCamera *	arv_fake_device_get_fake_camera		(ArvFakeDevice *device);

G_END_DECLS

#endif
