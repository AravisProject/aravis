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

#ifndef ARV_SYSTEM_H
#define ARV_SYSTEM_H

#include <arvtypes.h>

G_BEGIN_DECLS

unsigned int 		arv_get_n_interfaces 		(void);
const char * 		arv_get_interface_id 		(unsigned int index);
void 			arv_enable_interface 		(const char *interface_id);
void 			arv_disable_interface		(const char *interface_id);

void 			arv_update_device_list 		(void);
unsigned int 		arv_get_n_devices 		(void);
const char * 		arv_get_device_id 		(unsigned int index);

ArvDevice * 		arv_open_device 		(const char *device_id);

void 			arv_shutdown 			(void);

G_END_DECLS

#endif

