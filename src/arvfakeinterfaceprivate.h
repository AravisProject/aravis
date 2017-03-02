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

#ifndef ARV_FAKE_INTERFACE_PRIVATE_H
#define ARV_FAKE_INTERFACE_PRIVATE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvinterfaceprivate.h>
#include <arvfakeinterface.h>

G_BEGIN_DECLS

#define ARV_FAKE_INTERFACE_DISCOVERY_TIMEOUT_MS	1000
#define ARV_FAKE_INTERFACE_SOCKET_BUFFER_SIZE	1024

G_END_DECLS

typedef struct _ArvFakeInterfacePrivate ArvFakeInterfacePrivate;
typedef struct _ArvFakeInterfaceClass ArvFakeInterfaceClass;

struct _ArvFakeInterface {
	ArvInterface	interface;

	ArvFakeInterfacePrivate *priv;
};

struct _ArvFakeInterfaceClass {
	ArvInterfaceClass parent_class;
};

void 			arv_fake_interface_destroy_instance 		(void);

#endif
