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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_GV_DEVICE_H
#define ARV_GV_DEVICE_H

#include <arv.h>
#include <arvdevice.h>
#include <arvgvinterface.h>

G_BEGIN_DECLS

#define	ARV_GV_DEVICE_GVCP_N_RETRIES_DEFAULT	5
#define	ARV_GV_DEVICE_GVCP_TIMEOUT_MS_DEFAULT	500

#define ARV_TYPE_GV_DEVICE             (arv_gv_device_get_type ())
#define ARV_GV_DEVICE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GV_DEVICE, ArvGvDevice))
#define ARV_GV_DEVICE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GV_DEVICE, ArvGvDeviceClass))
#define ARV_IS_GV_DEVICE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GV_DEVICE))
#define ARV_IS_GV_DEVICE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GV_DEVICE))
#define ARV_GV_DEVICE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GV_DEVICE, ArvGvDeviceClass))

#define ARV_GV_DEVICE_BUFFER_SIZE	1024

typedef struct _ArvGvDeviceClass ArvGvDeviceClass;
typedef struct _ArvGvDevicePrivate ArvGvDevicePrivate;

struct _ArvGvDevice {
	ArvDevice device;

	ArvGvDevicePrivate *priv;
};

struct _ArvGvDeviceClass {
	ArvDeviceClass parent_class;
};

GType arv_gv_device_get_type (void);

ArvDevice * 		arv_gv_device_new 		(GInetAddress *interface_address, GInetAddress *device_address);

G_END_DECLS

#endif
