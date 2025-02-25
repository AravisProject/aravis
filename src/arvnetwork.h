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

#ifndef ARV_NETWORK_H
#define ARV_NETWORK_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_NETWORK_ERROR arv_network_error_quark()

ARV_API GQuark		arv_network_error_quark		(void);

/**
 * ArvNetworkError:
 * @ARV_NETWORK_ERROR_PORT_EXHAUSTION: No more available port in constrained port range
 */

typedef enum {
	ARV_NETWORK_ERROR_PORT_EXHAUSTION
} ArvNetworkError;

ARV_API gboolean        arv_set_gv_port_range              (guint16 port_min, guint16 port_max);
ARV_API gboolean        arv_set_gv_port_range_from_string  (const char *range);

G_END_DECLS

#endif
