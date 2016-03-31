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

#ifndef ARV_UV_INTERFACE_H
#define ARV_UV_INTERFACE_H

#include <arvtypes.h>
#include <arvinterface.h>

G_BEGIN_DECLS

#define ARV_TYPE_UV_INTERFACE             (arv_uv_interface_get_type ())
#define ARV_UV_INTERFACE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_UV_INTERFACE, ArvUvInterface))
#define ARV_UV_INTERFACE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_UV_INTERFACE, ArvUvInterfaceClass))
#define ARV_IS_UV_INTERFACE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_UV_INTERFACE))
#define ARV_IS_UV_INTERFACE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_UV_INTERFACE))
#define ARV_UV_INTERFACE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_UV_INTERFACE, ArvUvInterfaceClass))

typedef struct _ArvUvInterfacePrivate ArvUvInterfacePrivate;
typedef struct _ArvUvInterfaceClass ArvUvInterfaceClass;

struct _ArvUvInterface {
	ArvInterface	interface;

	ArvUvInterfacePrivate *priv;
};

struct _ArvUvInterfaceClass {
	ArvInterfaceClass parent_class;
};

GType arv_uv_interface_get_type (void);

ArvInterface * 		arv_uv_interface_get_instance 		(void);
void 			arv_uv_interface_destroy_instance 	(void);

G_END_DECLS

#endif
