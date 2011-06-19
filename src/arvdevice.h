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

#ifndef ARV_DEVICE_H
#define ARV_DEVICE_H

#include <arvtypes.h>
#include <arvstream.h>

G_BEGIN_DECLS

#define ARV_TYPE_DEVICE             (arv_device_get_type ())
#define ARV_DEVICE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_DEVICE, ArvDevice))
#define ARV_DEVICE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_DEVICE, ArvDeviceClass))
#define ARV_IS_DEVICE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_DEVICE))
#define ARV_IS_DEVICE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_DEVICE))
#define ARV_DEVICE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_DEVICE, ArvDeviceClass))

typedef struct _ArvDeviceClass ArvDeviceClass;

struct _ArvDevice {
	GObject	object;
};

struct _ArvDeviceClass {
	GObjectClass parent_class;

	ArvStream *	(*create_stream)	(ArvDevice *device, ArvStreamCallback callback, void *user_data);

	const char *	(*get_genicam_xml)	(ArvDevice *device, size_t *size);
	ArvGc *		(*get_genicam)		(ArvDevice *device);

	gboolean	(*read_memory)		(ArvDevice *device, guint32 address, guint32 size, void *buffer);
	gboolean	(*write_memory)		(ArvDevice *device, guint32 address, guint32 size, void *buffer);
	gboolean	(*read_register)	(ArvDevice *device, guint32 address, guint32 *value);
	gboolean	(*write_register)	(ArvDevice *device, guint32 address, guint32 value);
};

GType arv_device_get_type (void);

ArvStream *	arv_device_create_stream	(ArvDevice *device, ArvStreamCallback callback, void *user_data);
gboolean	arv_device_read_memory 		(ArvDevice *device, guint32 address, guint32 size,
						 void *buffer);
gboolean	arv_device_write_memory	 	(ArvDevice *device, guint32 address, guint32 size,
						 void *buffer);
gboolean 	arv_device_read_register	(ArvDevice *device, guint32 address, guint32 *value);
gboolean	arv_device_write_register 	(ArvDevice *device, guint32 address, guint32 value);

const char * 	arv_device_get_genicam_xml 		(ArvDevice *device, size_t *size);
ArvGc *		arv_device_get_genicam			(ArvDevice *device);

void 		arv_device_execute_command 		(ArvDevice *device, const char *feature);

ArvGcNode *	arv_device_get_feature			(ArvDevice *device, const char *feature);

void		arv_device_set_string_feature_value	(ArvDevice *device, const char *feature, const char *value);
const char *	arv_device_get_string_feature_value	(ArvDevice *device, const char *feature);

void		arv_device_set_integer_feature_value	(ArvDevice *device, const char *feature, gint64 value);
gint64		arv_device_get_integer_feature_value	(ArvDevice *device, const char *feature);
void 		arv_device_get_integer_feature_bounds 	(ArvDevice *device, const char *feature,
							 gint64 *min, gint64 *max);

void		arv_device_set_float_feature_value	(ArvDevice *device, const char *feature, double value);
double		arv_device_get_float_feature_value	(ArvDevice *device, const char *feature);
void 		arv_device_get_float_feature_bounds 	(ArvDevice *device, const char *feature,
							 double *min, double *max);

G_END_DECLS

#endif
