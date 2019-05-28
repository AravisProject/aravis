/* Aravis - Digital camera library
 *
 * Copyright © 2009-2019 Emmanuel Pacaud
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

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#ifndef ARV_GV_FAKE_CAMERA_H
#define ARV_GV_FAKE_CAMERA_H

#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_TYPE_GV_FAKE_CAMERA             (arv_gv_fake_camera_get_type ())
#define ARV_GV_FAKE_CAMERA(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GV_FAKE_CAMERA, ArvGvFakeCamera))
#define ARV_GV_FAKE_CAMERA_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GV_FAKE_CAMERA, ArvGvFakeCameraClass))
#define ARV_IS_GV_FAKE_CAMERA(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GV_FAKE_CAMERA))
#define ARV_IS_GV_FAKE_CAMERA_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GV_FAKE_CAMERA))
#define ARV_GV_FAKE_CAMERA_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GV_FAKE_CAMERA, ArvGvFakeCameraClass))

typedef struct _ArvGvFakeCameraPrivate ArvGvFakeCameraPrivate;
typedef struct _ArvGvFakeCameraClass ArvGvFakeCameraClass;

struct _ArvGvFakeCamera {
	GObject	object;

	ArvGvFakeCameraPrivate *priv;
};

struct _ArvGvFakeCameraClass {
	GObjectClass parent_class;
};

GType arv_gv_fake_camera_get_type (void);

ArvGvFakeCamera *		arv_gv_fake_camera_new		(const char *interface_name);
gboolean 			arv_gv_fake_camera_start	(ArvGvFakeCamera *gv_fake_camera);
void	 			arv_gv_fake_camera_stop		(ArvGvFakeCamera *gv_fake_camera);

G_END_DECLS

#endif
