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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_FAKE_CAMERA_H
#define ARV_FAKE_CAMERA_H

#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_FAKE_CAMERA_MEMORY_SIZE	0x1000

/* To keep in sync with arv-fake-camera.xml */

#define ARV_FAKE_CAMERA_REGISTER_SENSOR_WIDTH		0x11c
#define ARV_FAKE_CAMERA_REGISTER_SENSOR_HEIGHT		0x118
#define ARV_FAKE_CAMERA_REGISTER_WIDTH			0x100
#define ARV_FAKE_CAMERA_REGISTER_HEIGHT			0x104
#define ARV_FAKE_CAMERA_REGISTER_BINNING_HORIZONTAL	0x108
#define ARV_FAKE_CAMERA_REGISTER_BINNING_VERTICAL	0x10c
#define ARV_FAKE_CAMERA_REGISTER_GAIN_RAW		0x110
#define ARV_FAKE_CAMERA_REGISTER_GAIN_MODE		0x114
#define ARV_FAKE_CAMERA_REGISTER_EXPOSURE_TIME_US	0x120

#define ARV_TYPE_FAKE_CAMERA             (arv_fake_camera_get_type ())
#define ARV_FAKE_CAMERA(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_FAKE_CAMERA, ArvFakeCamera))
#define ARV_FAKE_CAMERA_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_FAKE_CAMERA, ArvFakeCameraClass))
#define ARV_IS_FAKE_CAMERA(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_FAKE_CAMERA))
#define ARV_IS_FAKE_CAMERA_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_FAKE_CAMERA))
#define ARV_FAKE_CAMERA_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_FAKE_CAMERA, ArvFakeCameraClass))

typedef struct _ArvFakeCameraClass ArvFakeCameraClass;
typedef struct _ArvFakeCameraPrivate ArvFakeCameraPrivate;

struct _ArvFakeCamera {
	GObject object;

	ArvFakeCameraPrivate *priv;
};

struct _ArvFakeCameraClass {
	GObjectClass parent_class;
};

GType arv_fake_camera_get_type (void);

ArvFakeCamera * arv_fake_camera_new 		(const char *serial_number);
gboolean	arv_fake_camera_read_memory 	(ArvFakeCamera *camera, guint32 address, guint32 size, void *buffer);
gboolean	arv_fake_camera_write_memory	(ArvFakeCamera *camera, guint32 address, guint32 size, void *buffer);
gboolean 	arv_fake_camera_read_register	(ArvFakeCamera *camera, guint32 address, guint32 *value);
gboolean	arv_fake_camera_write_register 	(ArvFakeCamera *camera, guint32 address, guint32 value);

const char *		arv_get_fake_camera_genicam_data	(size_t *size);

G_END_DECLS

#endif

