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

#define ARV_TYPE_DEVICE             (arv_device_get_type ())
#define ARV_DEVICE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_DEVICE, ArvDevice))
#define ARV_DEVICE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_DEVICE, ArvDeviceClass))
#define ARV_IS_DEVICE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_DEVICE))
#define ARV_IS_DEVICE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_DEVICE))
#define ARV_DEVICE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_DEVICE, ArvDeviceClass))

typedef struct _ArvDevicePrivate ArvDevicePrivate;
typedef struct _ArvDeviceClass ArvDeviceClass;

struct _ArvDevice {
	GObject	object;

	ArvDevicePrivate *priv;
};

struct _ArvDeviceClass {
	GObjectClass parent_class;

	ArvStream *	(*create_stream)	(ArvDevice *device, ArvStreamCallback callback, void *user_data);

	const char *	(*get_genicam_xml)	(ArvDevice *device, size_t *size);
	ArvGc *		(*get_genicam)		(ArvDevice *device);

	gboolean	(*read_memory)		(ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error);
	gboolean	(*write_memory)		(ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error);
	gboolean	(*read_register)	(ArvDevice *device, guint64 address, guint32 *value, GError **error);
	gboolean	(*write_register)	(ArvDevice *device, guint64 address, guint32 value, GError **error);

	/* signals */
	void		(*control_lost)		(ArvDevice *device);
};

GType arv_device_get_type (void);

ArvStream *	arv_device_create_stream	(ArvDevice *device, ArvStreamCallback callback, void *user_data);

gboolean	arv_device_read_memory 		(ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error);
gboolean	arv_device_write_memory	 	(ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error);
gboolean 	arv_device_read_register	(ArvDevice *device, guint64 address, guint32 *value, GError **error);
gboolean	arv_device_write_register 	(ArvDevice *device, guint64 address, guint32 value, GError **error);

const char * 	arv_device_get_genicam_xml 		(ArvDevice *device, size_t *size);
ArvGc *		arv_device_get_genicam			(ArvDevice *device);

ArvGcNode *	arv_device_get_feature			(ArvDevice *device, const char *feature);

ArvChunkParser *arv_device_create_chunk_parser		(ArvDevice *device);

void		arv_device_execute_command 		(ArvDevice *device, const char *feature, GError **error);

void		arv_device_set_boolean_feature_value	(ArvDevice *device, const char *feature, gboolean value, GError **error);
gboolean	arv_device_get_boolean_feature_value	(ArvDevice *device, const char *feature, GError **error);

void		arv_device_set_string_feature_value	(ArvDevice *device, const char *feature, const char *value, GError **error);
const char *	arv_device_get_string_feature_value	(ArvDevice *device, const char *feature, GError **error);

void		arv_device_set_integer_feature_value	(ArvDevice *device, const char *feature, gint64 value, GError **error);
gint64		arv_device_get_integer_feature_value	(ArvDevice *device, const char *feature, GError **error);
void 		arv_device_get_integer_feature_bounds 	(ArvDevice *device, const char *feature, gint64 *min, gint64 *max, GError **error);
gint64		arv_device_get_integer_feature_increment(ArvDevice *device, const char *feature, GError **error);

void		arv_device_set_float_feature_value	(ArvDevice *device, const char *feature, double value, GError **error);
double		arv_device_get_float_feature_value	(ArvDevice *device, const char *feature, GError **error);
void 		arv_device_get_float_feature_bounds 	(ArvDevice *device, const char *feature, double *min, double *max, GError **error);

gint64 *	arv_device_get_available_enumeration_feature_values			(ArvDevice *device, const char *feature,
											 guint *n_values, GError **error);
const char **	arv_device_get_available_enumeration_feature_values_as_strings		(ArvDevice *device, const char *feature,
											 guint *n_values, GError **error);
const char **	arv_device_get_available_enumeration_feature_values_as_display_names	(ArvDevice *device, const char *feature,
											 guint *n_values, GError **error);

void		arv_device_set_register_cache_policy	(ArvDevice *device, ArvRegisterCachePolicy policy);

G_END_DECLS

#endif
