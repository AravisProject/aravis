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

#ifndef ARV_UV_INTERFACE_PRIVATE_H
#define ARV_UV_INTERFACE_PRIVATE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvuvinterface.h>
#include <arvinterfaceprivate.h>

#define ARV_UV_INTERFACE_DEVICE_CLASS			0xef	/* Miscellaneous device */
#define ARV_UV_INTERFACE_DEVICE_SUBCLASS		0x02
#define ARV_UV_INTERFACE_DEVICE_PROTOCOL		0x01
#define ARV_UV_INTERFACE_INTERFACE_CLASS		0xef
#define ARV_UV_INTERFACE_INTERFACE_SUBCLASS		0x05
#define ARV_UV_INTERFACE_CONTROL_PROTOCOL		0x00
#define ARV_UV_INTERFACE_EVENT_PROTOCOL			0x01
#define ARV_UV_INTERFACE_DATA_PROTOCOL			0x02
#define ARV_UV_INTERFACE_GUID_INDEX_OFFSET		11

G_BEGIN_DECLS

void 			arv_uv_interface_destroy_instance 	(void);

G_END_DECLS

#endif
