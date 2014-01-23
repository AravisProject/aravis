/* Aravis - Digital camera library
 *
 * Copyright © 2009-2011 Emmanuel Pacaud
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

#ifndef ARV_FAKE_CAMERA_H
#define ARV_FAKE_CAMERA_H

#include <arvtypes.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef void (*ArvFakeCameraFillPattern) (ArvBuffer *buffer, void *fill_pattern_data,
					  guint32 exposure_time_us, guint32 gain,
					  ArvPixelFormat pixel_format);

#define ARV_FAKE_CAMERA_MEMORY_SIZE	0x10000

/* To keep in sync with arv-fake-camera.xml */

/* Image format control */

#define ARV_FAKE_CAMERA_REGISTER_SENSOR_WIDTH		0x11c
#define ARV_FAKE_CAMERA_REGISTER_SENSOR_HEIGHT		0x118
#define ARV_FAKE_CAMERA_REGISTER_WIDTH			0x100
#define ARV_FAKE_CAMERA_REGISTER_HEIGHT			0x104
#define ARV_FAKE_CAMERA_REGISTER_X_OFFSET		0x130
#define ARV_FAKE_CAMERA_REGISTER_Y_OFFSET		0x134
#define ARV_FAKE_CAMERA_REGISTER_BINNING_HORIZONTAL	0x108
#define ARV_FAKE_CAMERA_REGISTER_BINNING_VERTICAL	0x10c
#define ARV_FAKE_CAMERA_REGISTER_PIXEL_FORMAT		0x128
#define ARV_FAKE_CAMERA_REGISTER_TEST			0x1f0

#define ARV_FAKE_CAMERA_SENSOR_WIDTH			2048
#define ARV_FAKE_CAMERA_SENSOR_HEIGHT			2048
#define ARV_FAKE_CAMERA_WIDTH_DEFAULT			512
#define ARV_FAKE_CAMERA_HEIGHT_DEFAULT			512
#define ARV_FAKE_CAMERA_BINNING_HORIZONTAL_DEFAULT	1
#define ARV_FAKE_CAMERA_BINNING_VERTICAL_DEFAULT	1
#define ARV_FAKE_CAMERA_PIXEL_FORMAT_DEFAULT		ARV_PIXEL_FORMAT_MONO_8
#define ARV_FAKE_CAMERA_TEST_REGISTER_DEFAULT		0x12345678

/* Acquisition control */

#define ARV_FAKE_CAMERA_REGISTER_ACQUISITION_MODE	0x12c

#define ARV_FAKE_CAMERA_REGISTER_ACQUISITION_FRAME_PERIOD_US	0x138

#define ARV_FAKE_CAMERA_REGISTER_FRAME_START_OFFSET		0x000
#define ARV_FAKE_CAMERA_REGISTER_ACQUISITION_START_OFFSET	0x020

#define ARV_FAKE_CAMERA_REGISTER_TRIGGER_MODE		0x300
#define ARV_FAKE_CAMERA_REGISTER_TRIGGER_SOURCE		0x304
#define ARV_FAKE_CAMERA_REGISTER_TRIGGER_ACTIVATION	0x308

#define ARV_FAKE_CAMERA_REGISTER_ACQUISITION		0x124
#define ARV_FAKE_CAMERA_REGISTER_EXPOSURE_TIME_US	0x120

#define ARV_FAKE_CAMERA_ACQUISITION_FRAME_RATE_DEFAULT	25.0
#define ARV_FAKE_CAMERA_EXPOSURE_TIME_US_DEFAULT	10000.0

/* Analog control */

#define ARV_FAKE_CAMERA_REGISTER_GAIN_RAW		0x110
#define ARV_FAKE_CAMERA_REGISTER_GAIN_MODE		0x114

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
gboolean	arv_fake_camera_write_memory	(ArvFakeCamera *camera, guint32 address, guint32 size,
						 const void *buffer);
gboolean 	arv_fake_camera_read_register	(ArvFakeCamera *camera, guint32 address, guint32 *value);
gboolean	arv_fake_camera_write_register 	(ArvFakeCamera *camera, guint32 address, guint32 value);

size_t 		arv_fake_camera_get_payload 		(ArvFakeCamera *camera);
void 		arv_fake_camera_wait_for_next_frame 	(ArvFakeCamera *camera);
void		arv_fake_camera_fill_buffer		(ArvFakeCamera *camera, ArvBuffer *buffer,
							 guint32 *packet_size);

guint32 	arv_fake_camera_get_acquisition_status 	(ArvFakeCamera *camera);
GSocketAddress *arv_fake_camera_get_stream_address 	(ArvFakeCamera *camera);
void		arv_fake_camera_set_inet_address	(ArvFakeCamera *camera, GInetAddress *address);

guint32		arv_fake_camera_get_control_channel_privilege	(ArvFakeCamera *camera);
void		arv_fake_camera_set_control_channel_privilege	(ArvFakeCamera *camera, guint32 privilege);
guint32		arv_fake_camera_get_heartbeat_timeout		(ArvFakeCamera *camera);

void		arv_fake_camera_set_fill_pattern	(ArvFakeCamera *camera,
							 ArvFakeCameraFillPattern fill_pattern_callback,
							 void *fill_pattern_data);
void 		arv_fake_camera_set_trigger_frequency 	(ArvFakeCamera *camera, double frequency);

void 		arv_set_fake_camera_genicam_filename 	(const char *filename);
const char *	arv_get_fake_camera_genicam_xml		(size_t *size);

G_END_DECLS

#endif

