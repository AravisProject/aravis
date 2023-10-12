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

#ifndef ARV_CAMERA_H
#define ARV_CAMERA_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvstream.h>
#include <arvgvstream.h>
#include <arvgvdevice.h>

G_BEGIN_DECLS

typedef enum {
        ARV_COMPONENT_SELECTION_FLAGS_NONE,
        ARV_COMPONENT_SELECTION_FLAGS_ENABLE,
        ARV_COMPONENT_SELECTION_FLAGS_DISABLE,
        ARV_COMPONENT_SELECTION_FLAGS_EXCLUSIVE_ENABLE,
        ARV_COMPONENT_SELECTION_FLAGS_ENABLE_ALL,
} ArvComponentSelectionFlags;

#define ARV_TYPE_CAMERA             (arv_camera_get_type ())
ARV_API G_DECLARE_DERIVABLE_TYPE (ArvCamera, arv_camera, ARV, CAMERA, GObject)

struct _ArvCameraClass {
	GObjectClass parent_class;
};

ARV_API ArvCamera *	arv_camera_new			(const char *name, GError **error);
ARV_API ArvCamera *	arv_camera_new_with_device	(ArvDevice *device, GError **error);
ARV_API ArvDevice *	arv_camera_get_device		(ArvCamera *camera);

ARV_API ArvStream *	arv_camera_create_stream	(ArvCamera *camera, ArvStreamCallback callback, void *user_data, GError **error);
ARV_API ArvStream *	arv_camera_create_stream_full	(ArvCamera *camera, ArvStreamCallback callback, void *user_data, GDestroyNotify destroy, GError **error);

/* Device informations */

ARV_API const char *	arv_camera_get_vendor_name		(ArvCamera *camera, GError **error);
ARV_API const char *	arv_camera_get_model_name		(ArvCamera *camera, GError **error);
ARV_API const char * 	arv_camera_get_device_serial_number	(ArvCamera *camera, GError **error);
ARV_API const char *	arv_camera_get_device_id		(ArvCamera *camera, GError **error);

/* Image format control */

ARV_API gboolean        arv_camera_is_region_offset_available    (ArvCamera *camera, GError **error);

ARV_API void 		arv_camera_get_sensor_size 		(ArvCamera *camera, gint *width, gint *height, GError **error);
ARV_API void		arv_camera_set_region			(ArvCamera *camera, gint x, gint y, gint width, gint height, GError **error);
ARV_API void		arv_camera_get_region			(ArvCamera *camera, gint *x, gint *y, gint *width, gint *height, GError **error);
ARV_API void		arv_camera_get_x_offset_bounds		(ArvCamera *camera, gint *min, gint *max, GError **error);
ARV_API gint		arv_camera_get_x_offset_increment	(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_get_y_offset_bounds		(ArvCamera *camera, gint *min, gint *max, GError **error);
ARV_API gint		arv_camera_get_y_offset_increment	(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_get_width_bounds		(ArvCamera *camera, gint *min, gint *max, GError **error);
ARV_API gint		arv_camera_get_width_increment		(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_get_height_bounds		(ArvCamera *camera, gint *min, gint *max, GError **error);
ARV_API gint		arv_camera_get_height_increment		(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_set_binning			(ArvCamera *camera, gint dx, gint dy, GError **error);
ARV_API void		arv_camera_get_binning			(ArvCamera *camera, gint *dx, gint *dy, GError **error);
ARV_API void		arv_camera_get_x_binning_bounds		(ArvCamera *camera, gint *min, gint *max, GError **error);
ARV_API gint		arv_camera_get_x_binning_increment	(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_get_y_binning_bounds		(ArvCamera *camera, gint *min, gint *max, GError **error);
ARV_API gint		arv_camera_get_y_binning_increment	(ArvCamera *camera, GError **error);

ARV_API gboolean	arv_camera_is_binning_available		(ArvCamera *camera, GError **error);

ARV_API void		arv_camera_set_pixel_format				(ArvCamera *camera, ArvPixelFormat format, GError **error);
ARV_API void		arv_camera_set_pixel_format_from_string			(ArvCamera *camera, const char * format, GError **error);
ARV_API ArvPixelFormat	arv_camera_get_pixel_format				(ArvCamera *camera, GError **error);
ARV_API const char *	arv_camera_get_pixel_format_as_string			(ArvCamera *camera, GError **error);
ARV_API gint64 *	arv_camera_dup_available_pixel_formats			(ArvCamera *camera, guint *n_pixel_formats, GError **error);
ARV_API const char **	arv_camera_dup_available_pixel_formats_as_strings	(ArvCamera *camera, guint *n_pixel_formats, GError **error);
ARV_API const char **	arv_camera_dup_available_pixel_formats_as_display_names	(ArvCamera *camera, guint *n_pixel_formats, GError **error);

/* Acquisition control */

ARV_API void		arv_camera_start_acquisition		(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_stop_acquisition		(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_abort_acquisition		(ArvCamera *camera, GError **error);

ARV_API ArvBuffer *	arv_camera_acquisition			(ArvCamera *camera, guint64 timeout, GError **error);

ARV_API void			arv_camera_set_acquisition_mode (ArvCamera *camera, ArvAcquisitionMode value, GError **error);
ARV_API ArvAcquisitionMode	arv_camera_get_acquisition_mode (ArvCamera *camera, GError **error);

ARV_API void		arv_camera_set_frame_count		(ArvCamera *camera, gint64 frame_count, GError **error);
ARV_API gint64		arv_camera_get_frame_count		(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_get_frame_count_bounds	(ArvCamera *camera, gint64 *min, gint64 *max, GError **error);
ARV_API void            arv_camera_set_frame_rate_enable       (ArvCamera *camera, gboolean enable, GError **error);

ARV_API gboolean	arv_camera_is_frame_rate_available	(ArvCamera *camera, GError **error);

ARV_API void		arv_camera_set_frame_rate		(ArvCamera *camera, double frame_rate, GError **error);
ARV_API double		arv_camera_get_frame_rate		(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_get_frame_rate_bounds	(ArvCamera *camera, double *min, double *max, GError **error);
ARV_API void		arv_camera_set_trigger			(ArvCamera *camera, const char *source, GError **error);
ARV_API void		arv_camera_set_trigger_source		(ArvCamera *camera, const char *source, GError **error);
ARV_API const char *	arv_camera_get_trigger_source		(ArvCamera *camera, GError **error);
ARV_API const char **	arv_camera_dup_available_trigger_sources(ArvCamera *camera, guint *n_sources, GError **error);
ARV_API const char **	arv_camera_dup_available_triggers	(ArvCamera *camera, guint *n_triggers, GError **error);
ARV_API void		arv_camera_clear_triggers		(ArvCamera *camera, GError **error);
ARV_API gboolean	arv_camera_is_software_trigger_supported(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_software_trigger		(ArvCamera *camera, GError **error);

ARV_API gboolean	arv_camera_is_exposure_time_available	(ArvCamera *camera, GError **error);
ARV_API gboolean	arv_camera_is_exposure_auto_available	(ArvCamera *camera, GError **error);

ARV_API void		arv_camera_set_exposure_time		(ArvCamera *camera, double exposure_time_us, GError **error);
ARV_API double		arv_camera_get_exposure_time		(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_get_exposure_time_bounds	(ArvCamera *camera, double *min, double *max, GError **error);
ARV_API void		arv_camera_set_exposure_time_auto	(ArvCamera *camera, ArvAuto auto_mode, GError **error);
ARV_API ArvAuto		arv_camera_get_exposure_time_auto	(ArvCamera *camera, GError **error);

ARV_API void		arv_camera_set_exposure_mode		(ArvCamera *camera, ArvExposureMode mode, GError **error);

/* Analog control */

ARV_API gboolean	arv_camera_is_gain_available		(ArvCamera *camera, GError **error);
ARV_API gboolean	arv_camera_is_gain_auto_available	(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_select_gain			(ArvCamera *camera, const char *selector, GError **error);
ARV_API const char **	arv_camera_dup_available_gains	        (ArvCamera *camera, guint *n_selectors, GError **error);

ARV_API void		arv_camera_set_gain			(ArvCamera *camera, double gain, GError **error);
ARV_API double		arv_camera_get_gain			(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_get_gain_bounds		(ArvCamera *camera, double *min, double *max, GError **error);
ARV_API void		arv_camera_set_gain_auto		(ArvCamera *camera, ArvAuto auto_mode, GError **error);
ARV_API ArvAuto		arv_camera_get_gain_auto		(ArvCamera *camera, GError **error);

ARV_API gboolean	arv_camera_is_black_level_available	(ArvCamera *camera, GError **error);
ARV_API gboolean	arv_camera_is_black_level_auto_available(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_select_black_level		(ArvCamera *camera, const char *selector, GError **error);
ARV_API const char **	arv_camera_dup_available_black_levels   (ArvCamera *camera, guint *n_selectors, GError **error);

ARV_API void		arv_camera_set_black_level		(ArvCamera *camera, double blacklevel, GError **error);
ARV_API double		arv_camera_get_black_level		(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_get_black_level_bounds	(ArvCamera *camera, double *min, double *max, GError **error);
ARV_API void		arv_camera_set_black_level_auto		(ArvCamera *camera, ArvAuto auto_mode, GError **error);
ARV_API ArvAuto		arv_camera_get_black_level_auto		(ArvCamera *camera, GError **error);

/* Component control */

ARV_API gboolean	arv_camera_is_component_available	(ArvCamera *camera, GError **error);
ARV_API const char **   arv_camera_dup_available_components     (ArvCamera *camera, guint *n_components, GError **error);
ARV_API void            arv_camera_select_and_enable_component  (ArvCamera *camera, const char *component,
                                                                 gboolean disable_others, GError **error);
ARV_API gboolean        arv_camera_select_component             (ArvCamera *camera, const char *component,
                                                                 ArvComponentSelectionFlags flags, guint *component_id,
                                                                 GError **error);

/* Transport layer control */

ARV_API guint		arv_camera_get_payload			(ArvCamera *camera, GError **error);

/* Generic feature control */

ARV_API void		arv_camera_execute_command		(ArvCamera *camera, const char *feature, GError **error);

ARV_API void		arv_camera_set_boolean			(ArvCamera *camera, const char *feature, gboolean value, GError **error);
ARV_API gboolean	arv_camera_get_boolean			(ArvCamera *camera, const char *feature, GError **error);
ARV_API void		arv_camera_get_boolean_gi		(ArvCamera *camera, const char *feature, gboolean *value, GError **error);

ARV_API void		arv_camera_set_string			(ArvCamera *camera, const char *feature, const char *value, GError **error);
ARV_API const char *	arv_camera_get_string			(ArvCamera *camera, const char *feature, GError **error);

ARV_API void		arv_camera_set_integer			(ArvCamera *camera, const char *feature, gint64 value, GError **error);
ARV_API gint64		arv_camera_get_integer			(ArvCamera *camera, const char *feature, GError **error);
ARV_API void		arv_camera_get_integer_bounds		(ArvCamera *camera, const char *feature, gint64 *min, gint64 *max, GError **error);
ARV_API gint64		arv_camera_get_integer_increment	(ArvCamera *camera, const char *feature, GError **error);

ARV_API void		arv_camera_set_float			(ArvCamera *camera, const char *feature, double value, GError **error);
ARV_API double		arv_camera_get_float			(ArvCamera *camera, const char *feature, GError **error);
ARV_API void		arv_camera_get_float_bounds		(ArvCamera *camera, const char *feature, double *min, double *max, GError **error);
ARV_API double		arv_camera_get_float_increment		(ArvCamera *camera, const char *feature, GError **error);

ARV_API gint64 *	arv_camera_dup_available_enumerations			(ArvCamera *camera, const char *feature,
										 guint *n_values, GError **error);
ARV_API const char **	arv_camera_dup_available_enumerations_as_strings	(ArvCamera *camera, const char *feature,
										 guint *n_values, GError **error);
ARV_API const char **	arv_camera_dup_available_enumerations_as_display_names	(ArvCamera *camera, const char *feature,
										 guint *n_values, GError **error);
ARV_API gboolean        arv_camera_is_enumeration_entry_available               (ArvCamera *camera, const char *feature,
										 const char *entry, GError **error);

ARV_API gboolean	arv_camera_is_feature_available			(ArvCamera *camera, const char *feature, GError **error);
ARV_API gboolean        arv_camera_is_feature_implemented               (ArvCamera *camera, const char *feature, GError **error);

/* Runtime policies */

ARV_API void		arv_camera_set_register_cache_policy		(ArvCamera *camera, ArvRegisterCachePolicy policy);
ARV_API void		arv_camera_set_range_check_policy		(ArvCamera *camera, ArvRangeCheckPolicy policy);
ARV_API void            arv_camera_set_access_check_policy	        (ArvCamera *camera, ArvAccessCheckPolicy policy);

/* GigEVision specific API */

ARV_API gboolean	arv_camera_is_gv_device				(ArvCamera *camera);

ARV_API gint            arv_camera_gv_get_n_network_interfaces          (ArvCamera *camera, GError **error);
ARV_API gint		arv_camera_gv_get_n_stream_channels		(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_gv_select_stream_channel		(ArvCamera *camera, gint channel_id, GError **error);
ARV_API int		arv_camera_gv_get_current_stream_channel	(ArvCamera *camera, GError **error);

ARV_API gboolean        arv_camera_gv_is_multipart_supported            (ArvCamera *camera, GError **error);
ARV_API void            arv_camera_gv_set_multipart                     (ArvCamera *camera, gboolean enable, GError **error);
ARV_API gboolean        arv_camera_gv_get_multipart                     (ArvCamera *camera, GError **error);

ARV_API void		arv_camera_gv_set_packet_delay			(ArvCamera *camera, gint64 delay_ns, GError **error);
ARV_API gint64		arv_camera_gv_get_packet_delay			(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_gv_set_packet_size			(ArvCamera *camera, gint packet_size, GError **error);
ARV_API guint		arv_camera_gv_get_packet_size			(ArvCamera *camera, GError **error);
ARV_API guint		arv_camera_gv_auto_packet_size			(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_gv_set_packet_size_adjustment	(ArvCamera *camera,
									 ArvGvPacketSizeAdjustment adjustment);

ARV_API void		arv_camera_gv_set_stream_options		(ArvCamera *camera, ArvGvStreamOption options);

ARV_API void		arv_camera_gv_get_persistent_ip			(ArvCamera *camera, GInetAddress **ip,
                                                                         GInetAddressMask **mask, GInetAddress **gateway,
                                                                         GError **error);
ARV_API void		arv_camera_gv_set_persistent_ip_from_string	(ArvCamera *camera, const char *ip,
                                                                         const char *mask, const char *gateway,
                                                                         GError **error);
ARV_API void		arv_camera_gv_set_persistent_ip			(ArvCamera *camera, GInetAddress *ip,
                                                                         GInetAddressMask *mask, GInetAddress *gateway,
                                                                         GError **error);
ARV_API ArvGvIpConfigurationMode	arv_camera_gv_get_ip_configuration_mode	(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_gv_set_ip_configuration_mode		(ArvCamera *camera,
                                                                         ArvGvIpConfigurationMode mode, GError **error);

ARV_API gboolean	arv_camera_is_uv_device				(ArvCamera *camera);
ARV_API gboolean	arv_camera_uv_is_bandwidth_control_available	(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_uv_set_bandwidth			(ArvCamera *camera, guint bandwidth, GError **error);
ARV_API guint		arv_camera_uv_get_bandwidth			(ArvCamera *camera, GError **error);
ARV_API void		arv_camera_uv_get_bandwidth_bounds		(ArvCamera *camera, guint *min, guint *max, GError **error);
ARV_API void            arv_camera_uv_set_usb_mode			(ArvCamera *camera, ArvUvUsbMode usb_mode);

/* Chunk data */

ARV_API gboolean		arv_camera_are_chunks_available		(ArvCamera *camera, GError **error);

ARV_API void			arv_camera_set_chunk_mode		(ArvCamera *camera, gboolean is_active, GError **error);
ARV_API gboolean		arv_camera_get_chunk_mode		(ArvCamera *camera, GError **error);
ARV_API void			arv_camera_set_chunk_state		(ArvCamera *camera, const char *chunk, gboolean is_enabled, GError **error);
ARV_API gboolean		arv_camera_get_chunk_state		(ArvCamera *camera, const char *chunk, GError **error);
ARV_API void			arv_camera_set_chunks			(ArvCamera *camera, const char *chunk_list, GError **error);
ARV_API ArvChunkParser *	arv_camera_create_chunk_parser		(ArvCamera *camera);

G_END_DECLS

#endif
