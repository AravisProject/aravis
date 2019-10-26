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

/**
 * ArvPixelFormat:
 *
 * A datatype to hold a pixel format.
 */

typedef guint32 ArvPixelFormat;

/**
 * ARV_TYPE_PIXEL_FORMAT:
 *
 * The #GType of a #ArvPixelFormat.
 */

#define ARV_TYPE_PIXEL_FORMAT G_TYPE_UINT32

#define ARV_PIXEL_FORMAT_BIT_PER_PIXEL(pixel_format) (((pixel_format) >> 16) & 0xff)

/* Grey pixel formats */

#define	ARV_PIXEL_FORMAT_MONO_8			((ArvPixelFormat) 0x01080001u)
#define	ARV_PIXEL_FORMAT_MONO_8_SIGNED		((ArvPixelFormat) 0x01080002u)

#define	ARV_PIXEL_FORMAT_MONO_10		((ArvPixelFormat) 0x01100003u)
#define ARV_PIXEL_FORMAT_MONO_10_PACKED		((ArvPixelFormat) 0x010c0004u)

#define ARV_PIXEL_FORMAT_MONO_12		((ArvPixelFormat) 0x01100005u)
#define ARV_PIXEL_FORMAT_MONO_12_PACKED		((ArvPixelFormat) 0x010c0006u)

#define ARV_PIXEL_FORMAT_MONO_14		((ArvPixelFormat) 0x01100025u) /* https://bugzilla.gnome.org/show_bug.cgi?id=655131 */

#define ARV_PIXEL_FORMAT_MONO_16		((ArvPixelFormat) 0x01100007u)

#define ARV_PIXEL_FORMAT_BAYER_GR_8		((ArvPixelFormat) 0x01080008u)
#define ARV_PIXEL_FORMAT_BAYER_RG_8		((ArvPixelFormat) 0x01080009u)
#define ARV_PIXEL_FORMAT_BAYER_GB_8		((ArvPixelFormat) 0x0108000au)
#define ARV_PIXEL_FORMAT_BAYER_BG_8		((ArvPixelFormat) 0x0108000bu)

#define ARV_PIXEL_FORMAT_BAYER_GR_10		((ArvPixelFormat) 0x0110000cu)
#define ARV_PIXEL_FORMAT_BAYER_RG_10		((ArvPixelFormat) 0x0110000du)
#define ARV_PIXEL_FORMAT_BAYER_GB_10		((ArvPixelFormat) 0x0110000eu)
#define ARV_PIXEL_FORMAT_BAYER_BG_10		((ArvPixelFormat) 0x0110000fu)

#define ARV_PIXEL_FORMAT_BAYER_GR_12		((ArvPixelFormat) 0x01100010u)
#define ARV_PIXEL_FORMAT_BAYER_RG_12		((ArvPixelFormat) 0x01100011u)
#define ARV_PIXEL_FORMAT_BAYER_GB_12		((ArvPixelFormat) 0x01100012u)
#define ARV_PIXEL_FORMAT_BAYER_BG_12		((ArvPixelFormat) 0x01100013u)

#define ARV_PIXEL_FORMAT_BAYER_GR_16		((ArvPixelFormat) 0x0110002eu)
#define ARV_PIXEL_FORMAT_BAYER_RG_16		((ArvPixelFormat) 0x0110002fu)
#define ARV_PIXEL_FORMAT_BAYER_GB_16		((ArvPixelFormat) 0x01100030u)
#define ARV_PIXEL_FORMAT_BAYER_BG_16		((ArvPixelFormat) 0x01100031u)

#define ARV_PIXEL_FORMAT_BAYER_BG_10P		((ArvPixelFormat) 0x010a0052u)
#define ARV_PIXEL_FORMAT_BAYER_GB_10P		((ArvPixelFormat) 0x010a0054u)
#define ARV_PIXEL_FORMAT_BAYER_GR_10P		((ArvPixelFormat) 0x010a0056u)
#define ARV_PIXEL_FORMAT_BAYER_RG_10P		((ArvPixelFormat) 0x010a0058u)

#define ARV_PIXEL_FORMAT_BAYER_BG_12P		((ArvPixelFormat) 0x010c0053u)
#define ARV_PIXEL_FORMAT_BAYER_GB_12P		((ArvPixelFormat) 0x010c0055u)
#define ARV_PIXEL_FORMAT_BAYER_GR_12P		((ArvPixelFormat) 0x010c0057u)
#define ARV_PIXEL_FORMAT_BAYER_RG_12P		((ArvPixelFormat) 0x010c0059u)

#define ARV_PIXEL_FORMAT_BAYER_GR_12_PACKED	((ArvPixelFormat) 0x010c002au)
#define ARV_PIXEL_FORMAT_BAYER_RG_12_PACKED	((ArvPixelFormat) 0x010c002bu)
#define ARV_PIXEL_FORMAT_BAYER_GB_12_PACKED	((ArvPixelFormat) 0x010c002cu)
#define ARV_PIXEL_FORMAT_BAYER_BG_12_PACKED	((ArvPixelFormat) 0x010c002du)

#define ARV_PIXEL_FORMAT_BAYER_GR_10_PACKED	((ArvPixelFormat) 0x010c0026u)
#define ARV_PIXEL_FORMAT_BAYER_RG_10_PACKED	((ArvPixelFormat) 0x010c0027u)
#define ARV_PIXEL_FORMAT_BAYER_GB_10_PACKED	((ArvPixelFormat) 0x010c0028u)
#define ARV_PIXEL_FORMAT_BAYER_BG_10_PACKED	((ArvPixelFormat) 0x010c0029u)

/* Color pixel formats */

#define ARV_PIXEL_FORMAT_RGB_8_PACKED		((ArvPixelFormat) 0x02180014u)
#define ARV_PIXEL_FORMAT_BGR_8_PACKED		((ArvPixelFormat) 0x02180015u)

#define ARV_PIXEL_FORMAT_RGBA_8_PACKED		((ArvPixelFormat) 0x02200016u)
#define ARV_PIXEL_FORMAT_BGRA_8_PACKED		((ArvPixelFormat) 0x02200017u)

#define ARV_PIXEL_FORMAT_RGB_10_PACKED		((ArvPixelFormat) 0x02300018u)
#define ARV_PIXEL_FORMAT_BGR_10_PACKED		((ArvPixelFormat) 0x02300019u)

#define ARV_PIXEL_FORMAT_RGB_12_PACKED		((ArvPixelFormat) 0x0230001au)
#define ARV_PIXEL_FORMAT_BGR_12_PACKED		((ArvPixelFormat) 0x0230001bu)

#define ARV_PIXEL_FORMAT_YUV_411_PACKED		((ArvPixelFormat) 0x020c001eu)
#define ARV_PIXEL_FORMAT_YUV_422_PACKED		((ArvPixelFormat) 0x0210001fu)
#define ARV_PIXEL_FORMAT_YUV_444_PACKED		((ArvPixelFormat) 0x02180020u)

#define ARV_PIXEL_FORMAT_RGB_8_PLANAR		((ArvPixelFormat) 0x02180021u)
#define ARV_PIXEL_FORMAT_RGB_10_PLANAR		((ArvPixelFormat) 0x02300022u)
#define ARV_PIXEL_FORMAT_RGB_12_PLANAR		((ArvPixelFormat) 0x02300023u)
#define ARV_PIXEL_FORMAT_RGB_16_PLANAR		((ArvPixelFormat) 0x02300024u)

#define ARV_PIXEL_FORMAT_YUV_422_YUYV_PACKED 	((ArvPixelFormat) 0x02100032u)

/* Custom */

/**
 * ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_12_PACKED: (type ArvPixelFormat)
 */

#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_12_PACKED  	((ArvPixelFormat) 0x810c0001u)
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_12_PACKED  	((ArvPixelFormat) 0x810c0002u)
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_12_PACKED  	((ArvPixelFormat) 0x810c0003u)
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_12_PACKED  	((ArvPixelFormat) 0x810c0004u)
#define ARV_PIXEL_FORMAT_CUSTOM_YUV_422_YUYV_PACKED 	((ArvPixelFormat) 0x82100005u)
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_16		((ArvPixelFormat) 0x81100006u)
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_16		((ArvPixelFormat) 0x81100007u)
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_16		((ArvPixelFormat) 0x81100008u)
#define ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_16		((ArvPixelFormat) 0x81100009u)

G_END_DECLS

#endif
