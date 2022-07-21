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

#ifndef ARV_INTERFACE_H
#define ARV_INTERFACE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvdevice.h>

G_BEGIN_DECLS

#define ARV_TYPE_INTERFACE             (arv_interface_get_type ())
ARV_API G_DECLARE_DERIVABLE_TYPE (ArvInterface, arv_interface, ARV, INTERFACE, GObject)

struct _ArvInterfaceClass {
	GObjectClass parent_class;

	void 		(*update_device_list)		(ArvInterface *iface, GArray *device_ids);
	ArvDevice *	(*open_device)			(ArvInterface *iface, const char *device_id, GError **error);

	const char *	protocol;
};

ARV_API void		arv_interface_update_device_list	        (ArvInterface *iface);
ARV_API unsigned int	arv_interface_get_n_devices		        (ArvInterface *iface);
ARV_API const char *	arv_interface_get_device_id		        (ArvInterface *iface, unsigned int index);
ARV_API const char *	arv_interface_get_device_physical_id	        (ArvInterface *iface, unsigned int index);
ARV_API const char *	arv_interface_get_device_address	        (ArvInterface *iface, unsigned int index);
ARV_API const char *	arv_interface_get_device_vendor		        (ArvInterface *iface, unsigned int index);
ARV_API const char *	arv_interface_get_device_manufacturer_info	(ArvInterface *iface, unsigned int index);
ARV_API const char *	arv_interface_get_device_model		        (ArvInterface *iface, unsigned int index);
ARV_API const char *	arv_interface_get_device_serial_nbr	        (ArvInterface *iface, unsigned int index);
ARV_API const char *	arv_interface_get_device_protocol	        (ArvInterface *iface, unsigned int index);
ARV_API ArvDevice *	arv_interface_open_device		        (ArvInterface *iface, const char *device_id,
                                                                         GError **error);

G_END_DECLS

#endif
