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

#ifndef ARV_REALTIME_PRIVATE_H
#define ARV_REALTIME_PRIVATE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvrealtime.h>
#include <gio/gio.h>

#ifdef G_OS_WIN32
#include <unistd.h> /* for pid_t */
#endif

int 		arv_rtkit_get_max_realtime_priority	(GDBusConnection *connection, GError **error);
int 		arv_rtkit_get_min_nice_level 		(GDBusConnection *connection, GError **error);
gint64		arv_rtkit_get_rttime_usec_max 		(GDBusConnection *connection, GError **error);
void		arv_rtkit_make_realtime 		(GDBusConnection *connection, pid_t thread, int priority, GError **error);
void		arv_rtkit_make_high_priority 		(GDBusConnection *connection, pid_t thread, int nice_level, GError **error);

#endif
