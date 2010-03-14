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

#include <arv.h>
#include <arvstream.h>
#include <arvgc.h>

G_BEGIN_DECLS

#define ARV_TYPE_DEVICE             (arv_device_get_type ())
#define ARV_DEVICE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_DEVICE, ArvDevice))
#define ARV_DEVICE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_DEVICE, ArvDeviceClass))
#define ARV_IS_DEVICE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_DEVICE))
#define ARV_IS_DEVICE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_DEVICE))
#define ARV_DEVICE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_DEVICE, ArvDeviceClass))

typedef struct _ArvDeviceClass ArvDeviceClass;
typedef struct _ArvDevicePrivate ArvDevicePrivate;

struct _ArvDevice {
	GObject	object;

	ArvDevicePrivate *priv;
};

struct _ArvDeviceClass {
	GObjectClass parent_class;

	ArvStream *	(*create_stream)	(ArvDevice *device);

	gboolean	(*read_memory)		(ArvDevice *device, guint32 address, guint32 size, void *buffer);
	gboolean	(*write_memory)		(ArvDevice *device, guint32 address, guint32 size, void *buffer);
	gboolean	(*read_register)	(ArvDevice *device, guint32 address, guint32 *value);
	gboolean	(*write_register)	(ArvDevice *device, guint32 address, guint32 value);
};

GType arv_device_get_type (void);

ArvStream *	arv_device_get_stream			(ArvDevice *device);

gboolean	arv_device_read_memory 			(ArvDevice *device, guint32 address, guint32 size,
							 void *buffer);
gboolean	arv_device_write_memory	 		(ArvDevice *device, guint32 address, guint32 size,
							 void *buffer);
gboolean 	arv_device_read_register		(ArvDevice *device, guint32 address, guint32 *value);
gboolean	arv_device_write_register 		(ArvDevice *device, guint32 address, guint32 value);

void 		arv_device_set_genicam_data		(ArvDevice *device, char *genicam, size_t size);
const char * 	arv_device_get_genicam_data		(ArvDevice *device, size_t *size);
ArvGc *		arv_device_get_genicam			(ArvDevice *device);

G_END_DECLS

#endif
