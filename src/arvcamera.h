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

#ifndef ARV_CAMERA_H
#define ARV_CAMERA_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvtypes.h>
#include <arvstream.h>
#include <arvgvstream.h>

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
void		arv_camera_get_x_offset_bounds	(ArvCamera *camera, gint *min, gint *max);
void		arv_camera_get_y_offset_bounds	(ArvCamera *camera, gint *min, gint *max);
void		arv_camera_get_width_bounds	(ArvCamera *camera, gint *min, gint *max);
void		arv_camera_get_height_bounds	(ArvCamera *camera, gint *min, gint *max);
void		arv_camera_set_binning		(ArvCamera *camera, gint dx, gint dy);
void		arv_camera_get_binning		(ArvCamera *camera, gint *dx, gint *dy);
void		arv_camera_get_x_binning_bounds	(ArvCamera *camera, gint *min, gint *max);
void		arv_camera_get_y_binning_bounds	(ArvCamera *camera, gint *min, gint *max);

gboolean	arv_camera_is_binning_available (ArvCamera *camera);

void 		arv_camera_set_pixel_format 				(ArvCamera *camera, ArvPixelFormat format);
void		arv_camera_set_pixel_format_from_string 		(ArvCamera *camera, const char * format);
ArvPixelFormat	arv_camera_get_pixel_format 				(ArvCamera *camera);
const char * 	arv_camera_get_pixel_format_as_string			(ArvCamera *camera);
gint64 *	arv_camera_get_available_pixel_formats			(ArvCamera *camera, guint *n_pixel_formats);
const char **	arv_camera_get_available_pixel_formats_as_strings	(ArvCamera *camera, guint *n_pixel_formats);
const char **	arv_camera_get_available_pixel_formats_as_display_names	(ArvCamera *camera, guint *n_pixel_formats);

/* Acquisition control */

void		arv_camera_start_acquisition		(ArvCamera *camera);
void		arv_camera_stop_acquisition		(ArvCamera *camera);
void		arv_camera_abort_acquisition		(ArvCamera *camera);

ArvBuffer *	arv_camera_acquisition			(ArvCamera *camera, guint64 timeout);

void			arv_camera_set_acquisition_mode 	(ArvCamera *camera, ArvAcquisitionMode value);
ArvAcquisitionMode 	arv_camera_get_acquisition_mode 	(ArvCamera *camera);

void 		arv_camera_set_frame_count	(ArvCamera *camera, gint64 frame_count);
gint64		arv_camera_get_frame_count	(ArvCamera *camera);
void		arv_camera_get_frame_count_bounds	(ArvCamera *camera, gint64 *min, gint64 *max);

gboolean 	arv_camera_is_frame_rate_available 	(ArvCamera *camera);

void		arv_camera_set_frame_rate		(ArvCamera *camera, double frame_rate);
double 		arv_camera_get_frame_rate 		(ArvCamera *camera);
void		arv_camera_get_frame_rate_bounds 	(ArvCamera *camera, double *min, double *max);
void		arv_camera_set_trigger			(ArvCamera *camera, const char *source);
void 		arv_camera_set_trigger_source		(ArvCamera *camera, const char *source);
const char *	arv_camera_get_trigger_source		(ArvCamera *camera);
const char **	arv_camera_get_available_trigger_sources(ArvCamera *camera, guint *n_sources);
const char**    arv_camera_get_available_triggers       (ArvCamera *camera, guint *n_triggers);
void            arv_camera_clear_triggers               (ArvCamera* camera);
void 		arv_camera_software_trigger 		(ArvCamera *camera);

gboolean 	arv_camera_is_exposure_time_available	(ArvCamera *camera);
gboolean 	arv_camera_is_exposure_auto_available	(ArvCamera *camera);

void 		arv_camera_set_exposure_time 		(ArvCamera *camera, double exposure_time_us);
double 		arv_camera_get_exposure_time 		(ArvCamera *camera);
void		arv_camera_get_exposure_time_bounds	(ArvCamera *camera, double *min, double *max);
void		arv_camera_set_exposure_time_auto	(ArvCamera *camera, ArvAuto auto_mode);
ArvAuto		arv_camera_get_exposure_time_auto	(ArvCamera *camera);

/* Analog control */

gboolean 	arv_camera_is_gain_available		(ArvCamera *camera);
gboolean 	arv_camera_is_gain_auto_available	(ArvCamera *camera);

void 		arv_camera_set_gain	 	(ArvCamera *camera, double gain);
double 		arv_camera_get_gain 		(ArvCamera *camera);
void		arv_camera_get_gain_bounds	(ArvCamera *camera, double *min, double *max);
void		arv_camera_set_gain_auto	(ArvCamera *camera, ArvAuto auto_mode);
ArvAuto		arv_camera_get_gain_auto	(ArvCamera *camera);

/* Transport layer control */

guint		arv_camera_get_payload		(ArvCamera *camera);

/* GigEVision specific API */

gboolean	arv_camera_is_gv_device			(ArvCamera *camera);

gint		arv_camera_gv_get_n_stream_channels	(ArvCamera *camera);
void		arv_camera_gv_select_stream_channel	(ArvCamera *camera, gint channel_id);
int 		arv_camera_gv_get_current_stream_channel(ArvCamera *camera);

void		arv_camera_gv_set_packet_delay		(ArvCamera *camera, gint64 delay_ns);
gint64 		arv_camera_gv_get_packet_delay 		(ArvCamera *camera);
void 		arv_camera_gv_set_packet_size 		(ArvCamera *camera, guint packet_size);
guint		arv_camera_gv_get_packet_size		(ArvCamera *camera);
guint		arv_camera_gv_auto_packet_size		(ArvCamera *camera);

void 		arv_camera_gv_set_stream_options 	(ArvCamera *camera, ArvGvStreamOption options);

/* USB3Vision specific API */

gboolean        arv_camera_is_uv_device                 	(ArvCamera *camera);
gboolean        arv_camera_uv_is_bandwidth_control_available 	(ArvCamera *camera);
void            arv_camera_uv_set_bandwidth             	(ArvCamera *camera, guint bandwidth);
guint           arv_camera_uv_get_bandwidth             	(ArvCamera *camera);
void            arv_camera_uv_get_bandwidth_bounds      	(ArvCamera *camera, guint* min, guint* max);

/* Chunk data */

void 			arv_camera_set_chunk_mode 	(ArvCamera *camera, gboolean is_active);
gboolean 		arv_camera_get_chunk_mode 	(ArvCamera *camera);
void 			arv_camera_set_chunk_state 	(ArvCamera *camera, const char *chunk, gboolean is_enabled);
gboolean 		arv_camera_get_chunk_state 	(ArvCamera *camera, const char *chunk);
void 			arv_camera_set_chunks 		(ArvCamera *camera, const char *chunk_list);
ArvChunkParser * 	arv_camera_create_chunk_parser 	(ArvCamera *camera);

G_END_DECLS

#endif
