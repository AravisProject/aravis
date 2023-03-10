/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#ifndef ARV_GV_DEVICE_H
#define ARV_GV_DEVICE_H

#include <arvapi.h>
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
	ARV_GV_PACKET_SIZE_ADJUSTMENT_DEFAULT = ARV_GV_PACKET_SIZE_ADJUSTMENT_ON_FAILURE_ONCE
} ArvGvPacketSizeAdjustment;

typedef enum
{
	ARV_GV_IP_CONFIGURATION_MODE_NONE,
	ARV_GV_IP_CONFIGURATION_MODE_PERSISTENT_IP,
	ARV_GV_IP_CONFIGURATION_MODE_DHCP,
	ARV_GV_IP_CONFIGURATION_MODE_LLA,
	ARV_GV_IP_CONFIGURATION_MODE_FORCE_IP
} ArvGvIpConfigurationMode;

#define ARV_TYPE_GV_DEVICE             (arv_gv_device_get_type ())
ARV_API G_DECLARE_FINAL_TYPE (ArvGvDevice, arv_gv_device, ARV, GV_DEVICE, ArvDevice)

ARV_API ArvDevice *		arv_gv_device_new				(GInetAddress *interface_address,
                                                                                 GInetAddress *device_address,
										 GError **error);

ARV_API gboolean		arv_gv_device_take_control			(ArvGvDevice *gv_device, GError **error);
ARV_API gboolean		arv_gv_device_leave_control			(ArvGvDevice *gv_device, GError **error);

ARV_API guint64			arv_gv_device_get_timestamp_tick_frequency	(ArvGvDevice *gv_device, GError **error);

ARV_API GSocketAddress *	arv_gv_device_get_interface_address		(ArvGvDevice *device);
ARV_API GSocketAddress *	arv_gv_device_get_device_address		(ArvGvDevice *device);

ARV_API guint			arv_gv_device_get_packet_size			(ArvGvDevice *gv_device, GError **error);
ARV_API void			arv_gv_device_set_packet_size			(ArvGvDevice *gv_device, gint packet_size,
                                                                                 GError **error);
ARV_API void			arv_gv_device_set_packet_size_adjustment	(ArvGvDevice *gv_device,
										 ArvGvPacketSizeAdjustment adjustment);
ARV_API guint			arv_gv_device_auto_packet_size			(ArvGvDevice *gv_device, GError **error);

ARV_API ArvGvStreamOption	arv_gv_device_get_stream_options		(ArvGvDevice *gv_device);
ARV_API void			arv_gv_device_set_stream_options		(ArvGvDevice *gv_device,
                                                                                 ArvGvStreamOption options);

ARV_API gboolean		arv_gv_device_get_current_ip			(ArvGvDevice *gv_device,
                                                                                 GInetAddress **ip,
                                                                                 GInetAddressMask **mask,
                                                                                 GInetAddress **gateway, GError **error);
ARV_API gboolean		arv_gv_device_get_persistent_ip			(ArvGvDevice *gv_device,
                                                                                 GInetAddress **ip,
                                                                                 GInetAddressMask **mask,
                                                                                 GInetAddress **gateway, GError **error);
ARV_API gboolean		arv_gv_device_set_persistent_ip			(ArvGvDevice *gv_device,
                                                                                 GInetAddress *ip,
                                                                                 GInetAddressMask *mask,
                                                                                 GInetAddress *gateway, GError **error);
ARV_API gboolean		arv_gv_device_set_persistent_ip_from_string	(ArvGvDevice *gv_device,
                                                                                 const char *ip,
                                                                                 const char *mask,
                                                                                 const char *gateway, GError **error);
ARV_API ArvGvIpConfigurationMode	arv_gv_device_get_ip_configuration_mode	(ArvGvDevice *gv_device, GError **error);
ARV_API gboolean			arv_gv_device_set_ip_configuration_mode	(ArvGvDevice *gv_device,
                                                                                 ArvGvIpConfigurationMode mode,
                                                                                 GError **error);

ARV_API gboolean		arv_gv_device_is_controller			(ArvGvDevice *gv_device);

G_END_DECLS

#endif
