/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_GV_INTERFACE_H
#define ARV_GV_INTERFACE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvinterface.h>

G_BEGIN_DECLS

/**
 * ArvGvInterfaceFlags:
 * @ARV_GV_INTERFACE_FLAGS_ALLOW_BROADCAST_DISCOVERY_ACK: allow gv devices to broadcast the discovery acknowledge packet
 *
 * Since: 0.8.23
 */

typedef enum {
        ARV_GV_INTERFACE_FLAGS_ALLOW_BROADCAST_DISCOVERY_ACK =  1 << 0
} ArvGvInterfaceFlags;

#define ARV_TYPE_GV_INTERFACE             (arv_gv_interface_get_type ())
ARV_API G_DECLARE_FINAL_TYPE (ArvGvInterface, arv_gv_interface, ARV, GV_INTERFACE, ArvInterface)

ARV_API ArvInterface *		arv_gv_interface_get_instance		        (void);
ARV_API void                    arv_gv_interface_set_discovery_interface_name   (const char *discovery_interface);
ARV_API char *                  arv_gv_interface_dup_discovery_interface_name   (void);

G_END_DECLS

#endif
