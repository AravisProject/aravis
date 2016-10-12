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

#ifndef ARV_GV_DEVICE_PRIVATE_H
#define ARV_GV_DEVICE_PRIVATE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvgvdevice.h>

G_BEGIN_DECLS

#ifdef ARAVIS_FAST_HEARTBEAT
    #define ARV_GV_DEVICE_GVCP_N_RETRIES_DEFAULT    3
    #define ARV_GV_DEVICE_GVCP_TIMEOUT_MS_DEFAULT   25
    #define ARV_GV_DEVICE_HEARTBEAT_PERIOD_US       50000
    #define ARV_GV_DEVICE_HEARTBEAT_RETRY_DELAY_US  1000
    #define ARV_GV_DEVICE_HEARTBEAT_RETRY_TIMEOUT_S 0.25
#else
    #define ARV_GV_DEVICE_GVCP_N_RETRIES_DEFAULT    5
    #define ARV_GV_DEVICE_GVCP_TIMEOUT_MS_DEFAULT   500
    #define ARV_GV_DEVICE_HEARTBEAT_PERIOD_US       1000000
    #define ARV_GV_DEVICE_HEARTBEAT_RETRY_DELAY_US  10000
    #define ARV_GV_DEVICE_HEARTBEAT_RETRY_TIMEOUT_S 5.0		/* FIXME */
#endif

#define ARV_GV_DEVICE_GVSP_PACKET_SIZE_DEFAULT	1500

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

G_END_DECLS

#endif

