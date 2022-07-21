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

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#ifndef ARV_GV_FAKE_CAMERA_H
#define ARV_GV_FAKE_CAMERA_H

#include <arvapi.h>
#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_GV_FAKE_CAMERA_DEFAULT_SERIAL_NUMBER	"GV01"
#define ARV_GV_FAKE_CAMERA_DEFAULT_INTERFACE		"127.0.0.1"

#define ARV_TYPE_GV_FAKE_CAMERA (arv_gv_fake_camera_get_type ())
ARV_API G_DECLARE_FINAL_TYPE (ArvGvFakeCamera, arv_gv_fake_camera, ARV, GV_FAKE_CAMERA, GObject)

ARV_API ArvGvFakeCamera *		arv_gv_fake_camera_new			(const char *interface_name, const char *serial_number);
ARV_API ArvGvFakeCamera *		arv_gv_fake_camera_new_full		(const char *interface_name, const char *serial_number, const char *genicam_filename);
ARV_API gboolean			arv_gv_fake_camera_is_running		(ArvGvFakeCamera *gv_fake_camera);
ARV_API ArvFakeCamera *			arv_gv_fake_camera_get_fake_camera	(ArvGvFakeCamera *gv_fake_camera);

G_END_DECLS

#endif
