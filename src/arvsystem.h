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

#ifndef ARV_SYSTEM_H
#define ARV_SYSTEM_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>

G_BEGIN_DECLS

ARV_API unsigned int	arv_get_n_interfaces		        (void);
ARV_API const char *	arv_get_interface_id		        (unsigned int index);
ARV_API void		arv_enable_interface		        (const char *interface_id);
ARV_API void		arv_disable_interface		        (const char *interface_id);
ARV_API void		arv_set_interface_flags                 (const char *interface_id, int flags);

ARV_API void		arv_update_device_list		        (void);
ARV_API unsigned int	arv_get_n_devices		        (void);
ARV_API const char *	arv_get_device_id		        (unsigned int index);
ARV_API const char *	arv_get_device_physical_id	        (unsigned int index);
ARV_API const char *	arv_get_device_address		        (unsigned int index);
ARV_API const char *	arv_get_device_vendor		        (unsigned int index);
ARV_API const char *	arv_get_device_manufacturer_info        (unsigned int index);
ARV_API const char *	arv_get_device_model		        (unsigned int index);
ARV_API const char *	arv_get_device_serial_nbr	        (unsigned int index);
ARV_API const char *	arv_get_device_protocol		        (unsigned int index);

ARV_API ArvDevice *	arv_open_device			        (const char *device_id, GError **error);

ARV_API void		arv_shutdown			        (void);

G_END_DECLS

#endif

