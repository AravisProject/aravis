/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
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

#ifndef ARV_INTERFACE_H
#define ARV_INTERFACE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvtypes.h>
#include <arvdevice.h>

G_BEGIN_DECLS

#define ARV_TYPE_INTERFACE             (arv_interface_get_type ())
#define ARV_INTERFACE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_INTERFACE, ArvInterface))
#define ARV_INTERFACE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_INTERFACE, ArvInterfaceClass))
#define ARV_IS_INTERFACE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_INTERFACE))
#define ARV_IS_INTERFACE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_INTERFACE))
#define ARV_INTERFACE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_INTERFACE, ArvInterfaceClass))

typedef struct _ArvInterfacePrivate ArvInterfacePrivate;
typedef struct _ArvInterfaceClass ArvInterfaceClass;

struct _ArvInterface {
	GObject	object;

	ArvInterfacePrivate *priv;
};

struct _ArvInterfaceClass {
	GObjectClass parent_class;

	void 		(*update_device_list)		(ArvInterface *interface, GArray *device_ids);
	ArvDevice *	(*open_device)			(ArvInterface *interface, const char *device_id);

	const char *	protocol;
};

GType arv_interface_get_type (void);

void 			arv_interface_update_device_list 	(ArvInterface *interface);
unsigned int 		arv_interface_get_n_devices 		(ArvInterface *interface);
const char * 		arv_interface_get_device_id 		(ArvInterface *interface, unsigned int index);
const char * 		arv_interface_get_device_physical_id 	(ArvInterface *interface, unsigned int index);
const char * 		arv_interface_get_device_address	(ArvInterface *interface, unsigned int index);
const char * 		arv_interface_get_device_vendor		(ArvInterface *interface, unsigned int index);
const char * 		arv_interface_get_device_model		(ArvInterface *interface, unsigned int index);
const char * 		arv_interface_get_device_serial_nbr	(ArvInterface *interface, unsigned int index);
const char * 		arv_interface_get_device_protocol	(ArvInterface *interface, unsigned int index);
ArvDevice * 		arv_interface_open_device 		(ArvInterface *interface, const char *device_id);

G_END_DECLS

#endif
