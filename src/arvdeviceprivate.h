/* Aravis - Digital camera library
 *
 * Copyright © 2009-2014 Emmanuel Pacaud
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

#ifndef ARV_DEVICE_PRIVATE_H
#define ARV_DEVICE_PRIVATE_H

#include <arvdeviceprivate.h>

G_BEGIN_DECLS

#define ARV_DEVICE_ERROR arv_device_error_quark()

GQuark 		arv_device_error_quark 		(void);

void 		arv_device_emit_control_lost_signal 	(ArvDevice *device);

G_END_DECLS

#endif
