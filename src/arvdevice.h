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

#ifndef ARV_DEVICE_H
#define ARV_DEVICE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvtypes.h>
#include <arvstream.h>
#include <arvchunkparser.h>

G_BEGIN_DECLS

#define ARV_DEVICE_ERROR arv_device_error_quark()

GQuark 		arv_device_error_quark 		(void);

/**
 * ArvDeviceError:
 * @ARV_DEVICE_ERROR_WRONG_FEATURE: Wrong feature type
 * @ARV_DEVICE_ERROR_FEATURE_NOT_FOUND: Feature node not found
 * @ARV_DEVICE_ERROR_NOT_CONNECTED: Device is not connected
 * @ARV_DEVICE_ERROR_PROTOCOL_ERROR: Protocol error
 * @ARV_DEVICE_ERROR_TRANSFER_ERROR: Transfer error
 * @ARV_DEVICE_ERROR_TIMEOUT: Timeout detected
 * @ARV_DEVICE_ERROR_NOT_FOUND: Device not found
 * @ARV_DEVICE_ERROR_INVALID_PARAMETER: Invalid construction parameter
 * @ARV_DEVICE_ERROR_GENICAM_NOT_FOUND: Missing Genicam data
 * @ARV_DEVICE_ERROR_NO_STREAM_CHANNEL: No stream channel found
 * @ARV_DEVICE_ERROR_NOT_CONTROLLER: Controller privilege required
 * @ARV_DEVICE_ERROR_UNKNOWN: Unknown error
 */

typedef enum {
	ARV_DEVICE_ERROR_WRONG_FEATURE,
	ARV_DEVICE_ERROR_FEATURE_NOT_FOUND,
	ARV_DEVICE_ERROR_NOT_CONNECTED,
	ARV_DEVICE_ERROR_PROTOCOL_ERROR,
	ARV_DEVICE_ERROR_TRANSFER_ERROR,
	ARV_DEVICE_ERROR_TIMEOUT,
	ARV_DEVICE_ERROR_NOT_FOUND,
	ARV_DEVICE_ERROR_INVALID_PARAMETER,
	ARV_DEVICE_ERROR_GENICAM_NOT_FOUND,
	ARV_DEVICE_ERROR_NO_STREAM_CHANNEL,
	ARV_DEVICE_ERROR_NOT_CONTROLLER,
	ARV_DEVICE_ERROR_UNKNOWN
} ArvDeviceError;

#define ARV_TYPE_DEVICE             (arv_device_get_type ())
G_DECLARE_DERIVABLE_TYPE (ArvDevice, arv_device, ARV, DEVICE, GObject)

struct _ArvDeviceClass {
	GObjectClass parent_class;

	ArvStream *	(*create_stream)	(ArvDevice *device, ArvStreamCallback callback, void *user_data, GError **error);

	const char *	(*get_genicam_xml)	(ArvDevice *device, size_t *size);
	ArvGc *		(*get_genicam)		(ArvDevice *device);

	gboolean	(*read_memory)		(ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error);
	gboolean	(*write_memory)		(ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error);
	gboolean	(*read_register)	(ArvDevice *device, guint64 address, guint32 *value, GError **error);
	gboolean	(*write_register)	(ArvDevice *device, guint64 address, guint32 value, GError **error);

	/* signals */
	void		(*control_lost)		(ArvDevice *device);
};

ArvStream *	arv_device_create_stream	(ArvDevice *device, ArvStreamCallback callback, void *user_data, GError **error);

gboolean	arv_device_read_memory 		(ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error);
gboolean	arv_device_write_memory	 	(ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error);
gboolean 	arv_device_read_register	(ArvDevice *device, guint64 address, guint32 *value, GError **error);
gboolean	arv_device_write_register 	(ArvDevice *device, guint64 address, guint32 value, GError **error);

const char * 	arv_device_get_genicam_xml 		(ArvDevice *device, size_t *size);
ArvGc *		arv_device_get_genicam			(ArvDevice *device);

gboolean 	arv_device_is_feature_available 	(ArvDevice *device, const char *feature, GError **error);
ArvGcNode *	arv_device_get_feature			(ArvDevice *device, const char *feature);

ArvChunkParser *arv_device_create_chunk_parser		(ArvDevice *device);

void		arv_device_execute_command 		(ArvDevice *device, const char *feature, GError **error);

void		arv_device_set_boolean_feature_value	(ArvDevice *device, const char *feature, gboolean value, GError **error);
gboolean	arv_device_get_boolean_feature_value	(ArvDevice *device, const char *feature, GError **error);
void		arv_device_get_boolean_feature_value_gi	(ArvDevice *device, const char *feature, gboolean *value, GError **error);

void		arv_device_set_string_feature_value	(ArvDevice *device, const char *feature, const char *value, GError **error);
const char *	arv_device_get_string_feature_value	(ArvDevice *device, const char *feature, GError **error);

void		arv_device_set_integer_feature_value	(ArvDevice *device, const char *feature, gint64 value, GError **error);
gint64		arv_device_get_integer_feature_value	(ArvDevice *device, const char *feature, GError **error);
void 		arv_device_get_integer_feature_bounds 	(ArvDevice *device, const char *feature, gint64 *min, gint64 *max, GError **error);
gint64		arv_device_get_integer_feature_increment(ArvDevice *device, const char *feature, GError **error);

void		arv_device_set_float_feature_value	(ArvDevice *device, const char *feature, double value, GError **error);
double		arv_device_get_float_feature_value	(ArvDevice *device, const char *feature, GError **error);
void 		arv_device_get_float_feature_bounds 	(ArvDevice *device, const char *feature, double *min, double *max, GError **error);

gint64 *	arv_device_dup_available_enumeration_feature_values			(ArvDevice *device, const char *feature,
											 guint *n_values, GError **error);
const char **	arv_device_dup_available_enumeration_feature_values_as_strings		(ArvDevice *device, const char *feature,
											 guint *n_values, GError **error);
const char **	arv_device_dup_available_enumeration_feature_values_as_display_names	(ArvDevice *device, const char *feature,
											 guint *n_values, GError **error);


gboolean 	arv_device_set_features_from_string 	(ArvDevice *device, const char *string, GError **error);

void		arv_device_set_register_cache_policy	(ArvDevice *device, ArvRegisterCachePolicy policy);

G_END_DECLS

#endif
