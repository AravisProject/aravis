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

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#ifndef ARV_GV_DEVICE_H
#define ARV_GV_DEVICE_H

#include <arvtypes.h>
#include <arvdevice.h>
#include <arvgvstream.h>
#include <gio/gio.h>

G_BEGIN_DECLS

/**
 * ArvGvPacketSizeAdjustment:
 * @ARV_GV_PACKET_SIZE_ADJUSTMENT_NEVER: never adjust packet size
 * @ARV_GV_PACKET_SIZE_ADJUSTMENT_ON_FAILURE_ONCE: adjust packet size if test packet check fails the with current
 * packet size, only on the first stream creation
 * @ARV_GV_PACKET_SIZE_ADJUSTMENT_ON_FAILURE: adjust packet size if test packet check fails with current packet size
 * @ARV_GV_PACKET_SIZE_ADJUSTMENT_ONCE: adjust packet size on the first stream creation
 * @ARV_GV_PACKET_SIZE_ADJUSTMENT_ALWAYS: always adjust the stream packet size
 * @ARV_GV_PACKET_SIZE_ADJUSTMENT_DEFAULT: default adjustment, which is ON_FAILURE_ONCE (Since 0.8.8)
 */

typedef enum
{
	ARV_GV_PACKET_SIZE_ADJUSTMENT_NEVER,
	ARV_GV_PACKET_SIZE_ADJUSTMENT_ON_FAILURE_ONCE,
	ARV_GV_PACKET_SIZE_ADJUSTMENT_ON_FAILURE,
	ARV_GV_PACKET_SIZE_ADJUSTMENT_ONCE,
	ARV_GV_PACKET_SIZE_ADJUSTMENT_ALWAYS,
	ARV_GV_PACKET_SIZE_ADJUSTMENT_DEFAULT = ARV_GV_PACKET_SIZE_ADJUSTMENT_NEVER
} ArvGvPacketSizeAdjustment;

#define ARV_TYPE_GV_DEVICE             (arv_gv_device_get_type ())
G_DECLARE_FINAL_TYPE (ArvGvDevice, arv_gv_device, ARV, GV_DEVICE, ArvDevice)

ArvDevice * 		arv_gv_device_new 				(GInetAddress *interface_address, GInetAddress *device_address,
									 GError **error);

gboolean		arv_gv_device_take_control			(ArvGvDevice *gv_device, GError **error);
gboolean		arv_gv_device_leave_control			(ArvGvDevice *gv_device, GError **error);

guint64 		arv_gv_device_get_timestamp_tick_frequency	(ArvGvDevice *gv_device, GError **error);

GSocketAddress *	arv_gv_device_get_interface_address  		(ArvGvDevice *device);
GSocketAddress *	arv_gv_device_get_device_address  		(ArvGvDevice *device);

guint			arv_gv_device_get_packet_size 			(ArvGvDevice *gv_device, GError **error);
void			arv_gv_device_set_packet_size 			(ArvGvDevice *gv_device, gint packet_size, GError **error);
void 			arv_gv_device_set_packet_size_adjustment 	(ArvGvDevice *gv_device,
									 ArvGvPacketSizeAdjustment adjustment);
guint			arv_gv_device_auto_packet_size 			(ArvGvDevice *gv_device, GError **error);

ArvGvStreamOption	arv_gv_device_get_stream_options		(ArvGvDevice *gv_device);
void 			arv_gv_device_set_stream_options 		(ArvGvDevice *gv_device, ArvGvStreamOption options);

gboolean		arv_gv_device_is_controller			(ArvGvDevice *gv_device);

G_END_DECLS

#endif
