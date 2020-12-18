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

#ifndef ARV_CAMERA_H
#define ARV_CAMERA_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvtypes.h>
#include <arvstream.h>
#include <arvgvstream.h>
#include <arvgvdevice.h>

G_BEGIN_DECLS

#define ARV_TYPE_CAMERA             (arv_camera_get_type ())
G_DECLARE_DERIVABLE_TYPE (ArvCamera, arv_camera, ARV, CAMERA, GObject)

struct _ArvCameraClass {
	GObjectClass parent_class;
};

ArvCamera *	arv_camera_new			(const char *name, GError **error);
ArvCamera * 	arv_camera_new_with_device 	(ArvDevice *device, GError **error);
ArvDevice *	arv_camera_get_device		(ArvCamera *camera);

ArvStream *	arv_camera_create_stream	(ArvCamera *camera, ArvStreamCallback callback, void *user_data, GError **error);

/* Device informations */

const char *	arv_camera_get_vendor_name	(ArvCamera *camera, GError **error);
const char *	arv_camera_get_model_name	(ArvCamera *camera, GError **error);
const char *	arv_camera_get_device_id	(ArvCamera *camera, GError **error);

/* Image format control */

void 		arv_camera_get_sensor_size 		(ArvCamera *camera, gint *width, gint *height, GError **error);
void		arv_camera_set_region			(ArvCamera *camera, gint x, gint y, gint width, gint height, GError **error);
void		arv_camera_get_region			(ArvCamera *camera, gint *x, gint *y, gint *width, gint *height, GError **error);
void		arv_camera_get_x_offset_bounds		(ArvCamera *camera, gint *min, gint *max, GError **error);
gint		arv_camera_get_x_offset_increment	(ArvCamera *camera, GError **error);
void		arv_camera_get_y_offset_bounds		(ArvCamera *camera, gint *min, gint *max, GError **error);
gint		arv_camera_get_y_offset_increment	(ArvCamera *camera, GError **error);
void		arv_camera_get_width_bounds		(ArvCamera *camera, gint *min, gint *max, GError **error);
gint		arv_camera_get_width_increment		(ArvCamera *camera, GError **error);
void		arv_camera_get_height_bounds		(ArvCamera *camera, gint *min, gint *max, GError **error);
gint		arv_camera_get_height_increment		(ArvCamera *camera, GError **error);
void		arv_camera_set_binning			(ArvCamera *camera, gint dx, gint dy, GError **error);
void		arv_camera_get_binning			(ArvCamera *camera, gint *dx, gint *dy, GError **error);
void		arv_camera_get_x_binning_bounds		(ArvCamera *camera, gint *min, gint *max, GError **error);
gint		arv_camera_get_x_binning_increment	(ArvCamera *camera, GError **error);
void		arv_camera_get_y_binning_bounds		(ArvCamera *camera, gint *min, gint *max, GError **error);
gint		arv_camera_get_y_binning_increment	(ArvCamera *camera, GError **error);

gboolean	arv_camera_is_binning_available 	(ArvCamera *camera, GError **error);

void 		arv_camera_set_pixel_format 				(ArvCamera *camera, ArvPixelFormat format, GError **error);
void		arv_camera_set_pixel_format_from_string 		(ArvCamera *camera, const char * format, GError **error);
ArvPixelFormat	arv_camera_get_pixel_format 				(ArvCamera *camera, GError **error);
const char * 	arv_camera_get_pixel_format_as_string			(ArvCamera *camera, GError **error);
gint64 *	arv_camera_dup_available_pixel_formats			(ArvCamera *camera, guint *n_pixel_formats, GError **error);
const char **	arv_camera_dup_available_pixel_formats_as_strings	(ArvCamera *camera, guint *n_pixel_formats, GError **error);
const char **	arv_camera_dup_available_pixel_formats_as_display_names	(ArvCamera *camera, guint *n_pixel_formats, GError **error);

/* Acquisition control */

void		arv_camera_start_acquisition		(ArvCamera *camera, GError **error);
void		arv_camera_stop_acquisition		(ArvCamera *camera, GError **error);
void		arv_camera_abort_acquisition		(ArvCamera *camera, GError **error);

ArvBuffer *	arv_camera_acquisition			(ArvCamera *camera, guint64 timeout, GError **error);

void			arv_camera_set_acquisition_mode (ArvCamera *camera, ArvAcquisitionMode value, GError **error);
ArvAcquisitionMode 	arv_camera_get_acquisition_mode (ArvCamera *camera, GError **error);

void 		arv_camera_set_frame_count		(ArvCamera *camera, gint64 frame_count, GError **error);
gint64		arv_camera_get_frame_count		(ArvCamera *camera, GError **error);
void		arv_camera_get_frame_count_bounds	(ArvCamera *camera, gint64 *min, gint64 *max, GError **error);

gboolean 	arv_camera_is_frame_rate_available 	(ArvCamera *camera, GError **error);

void		arv_camera_set_frame_rate		(ArvCamera *camera, double frame_rate, GError **error);
double 		arv_camera_get_frame_rate 		(ArvCamera *camera, GError **error);
void		arv_camera_get_frame_rate_bounds 	(ArvCamera *camera, double *min, double *max, GError **error);
void		arv_camera_set_trigger			(ArvCamera *camera, const char *source, GError **error);
void 		arv_camera_set_trigger_source		(ArvCamera *camera, const char *source, GError **error);
const char *	arv_camera_get_trigger_source		(ArvCamera *camera, GError **error);
const char **	arv_camera_dup_available_trigger_sources(ArvCamera *camera, guint *n_sources, GError **error);
const char **   arv_camera_dup_available_triggers       (ArvCamera *camera, guint *n_triggers, GError **error);
void            arv_camera_clear_triggers               (ArvCamera* camera, GError **error);
void 		arv_camera_software_trigger 		(ArvCamera *camera, GError **error);

gboolean 	arv_camera_is_exposure_time_available	(ArvCamera *camera, GError **error);
gboolean 	arv_camera_is_exposure_auto_available	(ArvCamera *camera, GError **error);

void 		arv_camera_set_exposure_time 		(ArvCamera *camera, double exposure_time_us, GError **error);
double 		arv_camera_get_exposure_time 		(ArvCamera *camera, GError **error);
void		arv_camera_get_exposure_time_bounds	(ArvCamera *camera, double *min, double *max, GError **error);
void		arv_camera_set_exposure_time_auto	(ArvCamera *camera, ArvAuto auto_mode, GError **error);
ArvAuto		arv_camera_get_exposure_time_auto	(ArvCamera *camera, GError **error);

/* Analog control */

gboolean 	arv_camera_is_gain_available		(ArvCamera *camera, GError **error);
gboolean 	arv_camera_is_gain_auto_available	(ArvCamera *camera, GError **error);

void 		arv_camera_set_gain	 		(ArvCamera *camera, double gain, GError **error);
double 		arv_camera_get_gain 			(ArvCamera *camera, GError **error);
void		arv_camera_get_gain_bounds		(ArvCamera *camera, double *min, double *max, GError **error);
void		arv_camera_set_gain_auto		(ArvCamera *camera, ArvAuto auto_mode, GError **error);
ArvAuto		arv_camera_get_gain_auto		(ArvCamera *camera, GError **error);

/* Transport layer control */

guint		arv_camera_get_payload			(ArvCamera *camera, GError **error);

/* Generic feature control */

void 		arv_camera_execute_command 		(ArvCamera *camera, const char *feature, GError **error);

void		arv_camera_set_boolean			(ArvCamera *camera, const char *feature, gboolean value, GError **error);
gboolean	arv_camera_get_boolean			(ArvCamera *camera, const char *feature, GError **error);
void		arv_camera_get_boolean_gi		(ArvCamera *camera, const char *feature, gboolean *value, GError **error);

void		arv_camera_set_string			(ArvCamera *camera, const char *feature, const char *value, GError **error);
const char *	arv_camera_get_string			(ArvCamera *camera, const char *feature, GError **error);

void		arv_camera_set_integer			(ArvCamera *camera, const char *feature, gint64 value, GError **error);
gint64		arv_camera_get_integer			(ArvCamera *camera, const char *feature, GError **error);
void 		arv_camera_get_integer_bounds 		(ArvCamera *camera, const char *feature, gint64 *min, gint64 *max, GError **error);
gint64		arv_camera_get_integer_increment	(ArvCamera *camera, const char *feature, GError **error);

void		arv_camera_set_float			(ArvCamera *camera, const char *feature, double value, GError **error);
double		arv_camera_get_float			(ArvCamera *camera, const char *feature, GError **error);
void 		arv_camera_get_float_bounds 		(ArvCamera *camera, const char *feature, double *min, double *max, GError **error);

gint64 *	arv_camera_dup_available_enumerations			(ArvCamera *camera, const char *feature, guint *n_values,
									 GError **error);
const char **	arv_camera_dup_available_enumerations_as_strings	(ArvCamera *camera, const char *feature, guint *n_values,
									 GError **error);
const char ** 	arv_camera_dup_available_enumerations_as_display_names 	(ArvCamera *camera, const char *feature, guint *n_values,
									 GError **error);

gboolean 	arv_camera_is_feature_available 	(ArvCamera *camera, const char *feature, GError **error);

/* GigEVision specific API */

gboolean	arv_camera_is_gv_device			(ArvCamera *camera);

gint		arv_camera_gv_get_n_stream_channels	(ArvCamera *camera, GError **error);
void		arv_camera_gv_select_stream_channel	(ArvCamera *camera, gint channel_id, GError **error);
int 		arv_camera_gv_get_current_stream_channel(ArvCamera *camera, GError **error);

void		arv_camera_gv_set_packet_delay		(ArvCamera *camera, gint64 delay_ns, GError **error);
gint64 		arv_camera_gv_get_packet_delay 		(ArvCamera *camera, GError **error);
void 		arv_camera_gv_set_packet_size 		(ArvCamera *camera, gint packet_size, GError **error);
guint		arv_camera_gv_get_packet_size		(ArvCamera *camera, GError **error);
guint		arv_camera_gv_auto_packet_size		(ArvCamera *camera, GError **error);
void		arv_camera_gv_set_packet_size_adjustment 	(ArvCamera *camera,
								 ArvGvPacketSizeAdjustment adjustment);

void 		arv_camera_gv_set_stream_options 	(ArvCamera *camera, ArvGvStreamOption options);

/* USB3Vision specific API */

gboolean        arv_camera_is_uv_device                 	(ArvCamera *camera);
gboolean        arv_camera_uv_is_bandwidth_control_available 	(ArvCamera *camera, GError **error);
void            arv_camera_uv_set_bandwidth             	(ArvCamera *camera, guint bandwidth, GError **error);
guint           arv_camera_uv_get_bandwidth             	(ArvCamera *camera, GError **error);
void            arv_camera_uv_get_bandwidth_bounds      	(ArvCamera *camera, guint *min, guint *max, GError **error);

/* Chunk data */

void 			arv_camera_set_chunk_mode 	(ArvCamera *camera, gboolean is_active, GError **error);
gboolean 		arv_camera_get_chunk_mode 	(ArvCamera *camera, GError **error);
void 			arv_camera_set_chunk_state 	(ArvCamera *camera, const char *chunk, gboolean is_enabled, GError **error);
gboolean 		arv_camera_get_chunk_state 	(ArvCamera *camera, const char *chunk, GError **error);
void 			arv_camera_set_chunks 		(ArvCamera *camera, const char *chunk_list, GError **error);
ArvChunkParser * 	arv_camera_create_chunk_parser 	(ArvCamera *camera);

G_END_DECLS

#endif
