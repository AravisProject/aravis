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

#ifndef ARV_CAMERA_H
#define ARV_CAMERA_H

#include <arvtypes.h>
#include <arvstream.h>

G_BEGIN_DECLS

#define ARV_TYPE_CAMERA             (arv_camera_get_type ())
#define ARV_CAMERA(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_CAMERA, ArvCamera))
#define ARV_CAMERA_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_CAMERA, ArvCameraClass))
#define ARV_IS_CAMERA(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_CAMERA))
#define ARV_IS_CAMERA_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_CAMERA))
#define ARV_CAMERA_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_CAMERA, ArvCameraClass))

typedef struct _ArvCameraPrivate ArvCameraPrivate;
typedef struct _ArvCameraClass ArvCameraClass;

struct _ArvCamera {
	GObject	object;

	ArvCameraPrivate *priv;
};

struct _ArvCameraClass {
	GObjectClass parent_class;
};

GType arv_camera_get_type (void);

ArvCamera *	arv_camera_new			(const char *name);
ArvDevice *	arv_camera_get_device		(ArvCamera *camera);

ArvStream *	arv_camera_create_stream	(ArvCamera *camera, ArvStreamCallback callback, void *user_data);

/* Device control */

const char *	arv_camera_get_vendor_name	(ArvCamera *camera);
const char *	arv_camera_get_model_name	(ArvCamera *camera);
const char *	arv_camera_get_device_id	(ArvCamera *camera);

/* Image format control */

void 		arv_camera_get_sensor_size 	(ArvCamera *camera, gint *width, gint *height);
void		arv_camera_set_region		(ArvCamera *camera, gint x, gint y, gint width, gint height);
void		arv_camera_get_region		(ArvCamera *camera, gint *x, gint *y, gint *width, gint *height);
void		arv_camera_get_width_bounds	(ArvCamera *camera, gint *min, gint *max);
void		arv_camera_get_height_bounds	(ArvCamera *camera, gint *min, gint *max);
void		arv_camera_set_binning		(ArvCamera *camera, gint dx, gint dy);
void		arv_camera_get_binning		(ArvCamera *camera, gint *dx, gint *dy);

void 		arv_camera_set_pixel_format 		(ArvCamera *camera, ArvPixelFormat format);
ArvPixelFormat 	arv_camera_get_pixel_format 		(ArvCamera *camera);
gint64 *	arv_camera_get_available_pixel_formats 	(ArvCamera *camera, guint *n_pixel_formats);

/* Acquisition control */

void		arv_camera_start_acquisition		(ArvCamera *camera);
void		arv_camera_stop_acquisition		(ArvCamera *camera);

void			arv_camera_set_acquisition_mode 	(ArvCamera *camera, ArvAcquisitionMode value);
ArvAcquisitionMode 	arv_camera_get_acquisition_mode 	(ArvCamera *camera);

void		arv_camera_set_frame_rate		(ArvCamera *camera, double frame_rate);
double 		arv_camera_get_frame_rate 		(ArvCamera *camera);
void		arv_camera_set_trigger			(ArvCamera *camera, const char *source);
void 		arv_camera_set_trigger_source		(ArvCamera *camera, const char *source);
const char *	arv_camera_get_trigger_source		(ArvCamera *camera);

void 		arv_camera_software_trigger 		(ArvCamera *camera);

void 		arv_camera_set_exposure_time 		(ArvCamera *camera, double exposure_time_us);
double 		arv_camera_get_exposure_time 		(ArvCamera *camera);
void		arv_camera_get_exposure_time_bounds	(ArvCamera *camera, double *min, double *max);
void		arv_camera_set_exposure_time_auto	(ArvCamera *camera, ArvAuto auto_mode);
ArvAuto		arv_camera_get_exposure_time_auto	(ArvCamera *camera);

/* Analog control */

void 		arv_camera_set_gain	 	(ArvCamera *camera, gint gain);
gint 		arv_camera_get_gain 		(ArvCamera *camera);
void		arv_camera_get_gain_bounds	(ArvCamera *camera, gint *min, gint *max);
void		arv_camera_set_gain_auto	(ArvCamera *camera, ArvAuto auto_mode);
ArvAuto		arv_camera_get_gain_auto	(ArvCamera *camera);

/* Transport layer control */

guint		arv_camera_get_payload		(ArvCamera *camera);

G_END_DECLS

#endif
