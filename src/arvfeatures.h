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

#ifndef ARV_FEATURES_H
#define ARV_FEATURES_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

/**
 * ARAVIS_HAS_USB
 *
 * ARAVIS_HAS_USB is defined as 1 if aravis is compiled with USB support, 0 if not.
 *
 * Since: 0.6.0
 */

#define ARAVIS_HAS_USB 1

/**
 * ARAVIS_HAS_PACKET_SOCKET
 *
 * ARAVIS_HAS_PACKET_SOCKET is defined as 1 if aravis is compiled with packet socket support, 0 if not.
 *
 * Since: 0.6.0
 */

#define ARAVIS_HAS_PACKET_SOCKET 1

/**
 * ARAVIS_HAS_FAST_HEARTBEAT
 *
 * ARAVIS_HAS_FAST_HEARTBEAT is defined as 1 if aravis is compiled with fast hearbeat option, 0 if not.
 *
 * Since: 0.8.0
 */

#define ARAVIS_HAS_FAST_HEARTBEAT 0

#endif
