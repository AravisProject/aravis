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

#ifndef ARV_NETWORK_H
#define ARV_NETWORK_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvtypes.h>

#ifdef G_OS_WIN32
// for socklen_t
#include <gio/gnetworking.h>
#endif

G_BEGIN_DECLS

#define _ARV_IFACES

#ifdef _ARV_IFACES
GList* arv_enumerate_network_interfaces(void);
void arv_enumerate_network_interfaces_free(GList*);
#endif

#ifdef G_OS_WIN32
// prototype is missing on mingw, let's reimplement
const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt);
#endif

#endif
