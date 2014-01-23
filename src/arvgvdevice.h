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

#ifndef ARV_GV_DEVICE_H
#define ARV_GV_DEVICE_H

#include <arvtypes.h>
#include <arvdevice.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define	ARV_GV_DEVICE_GVCP_N_RETRIES_DEFAULT	5
#define	ARV_GV_DEVICE_GVCP_TIMEOUT_MS_DEFAULT	500
#define	ARV_GV_DEVICE_HEARTBEAT_PERIOD_US	1000000
#define	ARV_GV_DEVICE_HEARTBEAT_RETRY_DELAY_US	10000
#define	ARV_GV_DEVICE_HEARTBEAT_RETRY_TIMEOUT_S	5.0		/* FIXME */

#define ARV_GV_DEVICE_GVSP_PACKET_SIZE_DEFAULT	1500

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

ArvDevice * 	arv_gv_device_new 	(GInetAddress *interface_address, GInetAddress *device_address);

guint64 	arv_gv_device_get_timestamp_tick_frequency 	(ArvGvDevice *gv_device);
GRegex * 	arv_gv_device_get_url_regex 			(void);

GSocketAddress *arv_gv_device_get_interface_address  		(ArvGvDevice *device);
GSocketAddress *arv_gv_device_get_device_address  		(ArvGvDevice *device);

guint		arv_gv_device_get_packet_size (ArvGvDevice *gv_device);
void		arv_gv_device_set_packet_size (ArvGvDevice *gv_device, guint packet_size);

G_END_DECLS

#endif
