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

#ifndef ARV_GV_INTERFACE_H
#define ARV_GV_INTERFACE_H

#include <arvtypes.h>
#include <arvinterface.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define ARV_GV_INTERFACE_DISCOVERY_TIMEOUT_MS	1000
#define ARV_GV_INTERFACE_SOCKET_BUFFER_SIZE	1024

#define ARV_TYPE_GV_INTERFACE             (arv_gv_interface_get_type ())
#define ARV_GV_INTERFACE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GV_INTERFACE, ArvGvInterface))
#define ARV_GV_INTERFACE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GV_INTERFACE, ArvGvInterfaceClass))
#define ARV_IS_GV_INTERFACE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GV_INTERFACE))
#define ARV_IS_GV_INTERFACE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GV_INTERFACE))
#define ARV_GV_INTERFACE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GV_INTERFACE, ArvGvInterfaceClass))

typedef struct _ArvGvInterfacePrivate ArvGvInterfacePrivate;
typedef struct _ArvGvInterfaceClass ArvGvInterfaceClass;

struct _ArvGvInterface {
	ArvInterface	interface;

	ArvGvInterfacePrivate *priv;
};

struct _ArvGvInterfaceClass {
	ArvInterfaceClass parent_class;
};

GType arv_gv_interface_get_type (void);

ArvInterface * 		arv_gv_interface_get_instance 		(void);
void 			arv_gv_interface_destroy_instance 	(void);

G_END_DECLS

#endif
