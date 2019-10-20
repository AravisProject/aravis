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

#ifndef ARV_ENUMS_H
#define ARV_ENUMS_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * ArvStatus:
 * @ARV_STATUS_UNKNOWN: unknown status
 * @ARV_STATUS_SUCCESS: no error has occured
 * @ARV_STATUS_TIMEOUT: action failed on a timeout
 * @ARV_STATUS_WRITE_ERROR: write on a read only node
 * @ARV_STATUS_TRANSFER_ERROR: error during data transfer
 * @ARV_STATUS_PROTOCOL_ERROR: protocol specific error
 * @ARV_STATUS_NOT_CONNECTED: device not connected
 *
 * Since: 0.8.0
 */

typedef enum {
	ARV_STATUS_UNKNOWN = -1,
	ARV_STATUS_SUCCESS =  0,
	ARV_STATUS_TIMEOUT,
	ARV_STATUS_WRITE_ERROR,
	ARV_STATUS_TRANSFER_ERROR,
	ARV_STATUS_PROTOCOL_ERROR,
	ARV_STATUS_NOT_CONNECTED
} ArvStatus;

/**
 * ArvAuto:
 * @ARV_AUTO_OFF: manual setting
 * @ARV_AUTO_ONCE: automatic setting done once, then returns to manual
 * @ARV_AUTO_CONTINUOUS: setting is adjusted continuously
 */

typedef enum {
	ARV_AUTO_OFF,
	ARV_AUTO_ONCE,
	ARV_AUTO_CONTINUOUS
} ArvAuto;

const char * 		arv_auto_to_string 		(ArvAuto value);
ArvAuto 		arv_auto_from_string		(const char *string);

/**
 * ArvAcquisitionMode:
 * @ARV_ACQUISITION_MODE_CONTINUOUS: continuous acquisition
 * @ARV_ACQUISITION_MODE_SINGLE_FRAME: only one frame will be acquired
 * @ARV_ACQUISITION_MODE_MULTI_FRAME: multiple frames will be acquired
 */

typedef enum {
	ARV_ACQUISITION_MODE_CONTINUOUS,
	ARV_ACQUISITION_MODE_SINGLE_FRAME,
	ARV_ACQUISITION_MODE_MULTI_FRAME
} ArvAcquisitionMode;

const char * 		arv_acquisition_mode_to_string 		(ArvAcquisitionMode value);
ArvAcquisitionMode 	arv_acquisition_mode_from_string	(const char *string);

typedef guint32 ArvPixelFormat;

#define ARV_PIXEL_FORMAT_BIT_PER_PIXEL(pixel_format) (((pixel_format) >> 16) & 0xff)

/* Grey pixel formats */

#define	ARV_PIXEL_FORMAT_MONO_8			0x01080001
#define	ARV_PIXEL_FORMAT_MONO_8_SIGNED		0x01080002

#define	ARV_PIXEL_FORMAT_MONO_10		0x01100003
#define ARV_PIXEL_FORMAT_MONO_10_PACKED		0x010c0004

#define ARV_PIXEL_FORMAT_MONO_12		0x01100005
#define ARV_PIXEL_FORMAT_MONO_12_PACKED		0x010c0006

#define ARV_PIXEL_FORMAT_MONO_14		0x01100025 /* https://bugzilla.gnome.org/show_bug.cgi?id=655131 */

#define ARV_PIXEL_FORMAT_MONO_16		0x01100007

#define ARV_PIXEL_FORMAT_BAYER_GR_8		0x01080008
#define ARV_PIXEL_FORMAT_BAYER_RG_8		0x01080009
#define ARV_PIXEL_FORMAT_BAYER_GB_8		0x0108000a
#define ARV_PIXEL_FORMAT_BAYER_BG_8		0x0108000b

#define ARV_PIXEL_FORMAT_BAYER_GR_10		0x0110000c
#define ARV_PIXEL_FORMAT_BAYER_RG_10		0x0110000d
#define ARV_PIXEL_FORMAT_BAYER_GB_10		0x0110000e
#define ARV_PIXEL_FORMAT_BAYER_BG_10		0x0110000f

#define ARV_PIXEL_FORMAT_BAYER_GR_12		0x01100010
#define ARV_PIXEL_FORMAT_BAYER_RG_12		0x01100011
#define ARV_PIXEL_FORMAT_BAYER_GB_12		0x01100012
#define ARV_PIXEL_FORMAT_BAYER_BG_12		0x01100013

#define ARV_PIXEL_FORMAT_BAYER_GR_16		0x0110002e
#define ARV_PIXEL_FORMAT_BAYER_RG_16		0x0110002f
#define ARV_PIXEL_FORMAT_BAYER_GB_16		0x01100030
#define ARV_PIXEL_FORMAT_BAYER_BG_16		0x01100031

#define ARV_PIXEL_FORMAT_BAYER_BG_10P		0x010a0052
#define ARV_PIXEL_FORMAT_BAYER_GB_10P		0x010a0054
#define ARV_PIXEL_FORMAT_BAYER_GR_10P		0x010a0056
#define ARV_PIXEL_FORMAT_BAYER_RG_10P		0x010a0058

#define ARV_PIXEL_FORMAT_BAYER_BG_12P		0x010c0053
#define ARV_PIXEL_FORMAT_BAYER_GB_12P		0x010c0055
#define ARV_PIXEL_FORMAT_BAYER_GR_12P		0x010c0057
#define ARV_PIXEL_FORMAT_BAYER_RG_12P		0x010c0059

#define ARV_PIXEL_FORMAT_BAYER_GR_12_PACKED	0x010c002a
#define ARV_PIXEL_FORMAT_BAYER_RG_12_PACKED	0x010c002b
#define ARV_PIXEL_FORMAT_BAYER_GB_12_PACKED	0x010c002c
#define ARV_PIXEL_FORMAT_BAYER_BG_12_PACKED	0x010c002d

#define ARV_PIXEL_FORMAT_BAYER_GR_10_PACKED	0x010c0026
#define ARV_PIXEL_FORMAT_BAYER_RG_10_PACKED	0x010c0027
#define ARV_PIXEL_FORMAT_BAYER_GB_10_PACKED	0x010c0028
#define ARV_PIXEL_FORMAT_BAYER_BG_10_PACKED	0x010c0029

/* Color pixel formats */

#define ARV_PIXEL_FORMAT_RGB_8_PACKED		0x02180014
#define ARV_PIXEL_FORMAT_BGR_8_PACKED		0x02180015

#define ARV_PIXEL_FORMAT_RGBA_8_PACKED		0x02200016
#define ARV_PIXEL_FORMAT_BGRA_8_PACKED		0x02200017

#define ARV_PIXEL_FORMAT_RGB_10_PACKED		0x02300018
#define ARV_PIXEL_FORMAT_BGR_10_PACKED		0x02300019

#define ARV_PIXEL_FORMAT_RGB_12_PACKED		0x0230001a
#define ARV_PIXEL_FORMAT_BGR_12_PACKED		0x0230001b

#define ARV_PIXEL_FORMAT_YUV_411_PACKED		0x020c001e
#define ARV_PIXEL_FORMAT_YUV_422_PACKED		0x0210001f
#define ARV_PIXEL_FORMAT_YUV_444_PACKED		0x02180020

#define ARV_PIXEL_FORMAT_RGB_8_PLANAR		0x02180021
#define ARV_PIXEL_FORMAT_RGB_10_PLANAR		0x02300022
#define ARV_PIXEL_FORMAT_RGB_12_PLANAR		0x02300023
#define ARV_PIXEL_FORMAT_RGB_16_PLANAR		0x02300024

#define ARV_PIXEL_FORMAT_YUV_422_YUYV_PACKED 	0x02100032

/* Custom */

#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_12_PACKED  	0x810c0001u
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_12_PACKED  	0x810c0002u
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_12_PACKED  	0x810c0003u
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_12_PACKED  	0x810c0004u
#define ARV_PIXEL_FORMAT_CUSTOM_YUV_422_YUYV_PACKED 	0x82100005u
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_16		0x81100006u
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_16		0x81100007u
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_16		0x81100008u
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_16		0x81100009u

G_END_DECLS

#endif
