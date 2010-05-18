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

#ifndef ARV_ENUMS_H
#define ARV_ENUMS_H

#include <glib-object.h>
#include <arvconfig.h>

G_BEGIN_DECLS

typedef enum {
	ARV_GC_NAME_SPACE_STANDARD,
	ARV_GC_NAME_SPACE_CUSTOM
} ArvGcNameSpace;

typedef enum {
	ARV_GC_ACCESS_MODE_RO,
	ARV_GC_ACCESS_MODE_WO,
	ARV_GC_ACCESS_MODE_RW
} ArvGcAccessMode;

typedef enum {
	ARV_GC_CACHABLE_NO_CACHE,
	ARV_GC_CACHABLE_WRITE_TRHOUGH,
	ARV_GC_CACHABLE_WRITE_AROUND
} ArvGcCachable;

typedef enum {
	ARV_ACQUISITION_MODE_CONTINUOUS,
	ARV_ACQUISITION_MODE_SINGLE_FRAME
} ArvAcquisitionMode;

typedef enum {
	ARV_TRIGGER_SOURCE_LINE_0,
	ARV_TRIGGER_SOURCE_LINE_1,
	ARV_TRIGGER_SOURCE_LINE_2
} ArvTriggerSource;

typedef enum {
	ARV_PIXEL_FORMAT_MONO_8			= 0x01080001,
	ARV_PIXEL_FORMAT_MONO_10		= 0x01100003,
	ARV_PIXEL_FORMAT_MONO_12		= 0x01100005,
	ARV_PIXEL_FORMAT_MONO_12_PACKED		= 0x010c0006,
	ARV_PIXEL_FORMAT_MONO_16		= 0x01100007,
	ARV_PIXEL_FORMAT_BAYER_GR_8		= 0x01080008,
	ARV_PIXEL_FORMAT_BAYER_RG_8		= 0x01080009,
	ARV_PIXEL_FORMAT_BAYER_GB_8		= 0x0108000a,
	ARV_PIXEL_FORMAT_BAYER_BG_8		= 0x0108000b,
	ARV_PIXEL_FORMAT_BAYER_GR_10		= 0x0110000c,
	ARV_PIXEL_FORMAT_BAYER_RG_10		= 0x0110000d,
	ARV_PIXEL_FORMAT_BAYER_GB_10		= 0x0110000e,
	ARV_PIXEL_FORMAT_BAYER_BG_10		= 0x0110000f,
	ARV_PIXEL_FORMAT_BAYER_GR_12		= 0x01100010,
	ARV_PIXEL_FORMAT_BAYER_RG_12		= 0x01100011,
	ARV_PIXEL_FORMAT_BAYER_GB_12		= 0x01100012,
	ARV_PIXEL_FORMAT_BAYER_BG_12		= 0x01100013,
	ARV_PIXEL_FORMAT_RGB_8_PACKED		= 0x02180014,
	ARV_PIXEL_FORMAT_BGR_8_PACKED		= 0x02180015,
	ARV_PIXEL_FORMAT_RGBA_8_PACKED		= 0x02200016,
	ARV_PIXEL_FORMAT_BGRA_8_PACKED		= 0x02200017,
	ARV_PIXEL_FORMAT_RGB_10_PACKED		= 0x02300018,
	ARV_PIXEL_FORMAT_BGR_10_PACKED		= 0x02300019,
	ARV_PIXEL_FORMAT_RGB_12_PACKED		= 0x0230001a,
	ARV_PIXEL_FORMAT_BGR_12_PACKED		= 0x0230001b,
	ARV_PIXEL_FORMAT_YUV_411_PACKED		= 0x020c001e,
	ARV_PIXEL_FORMAT_YUV_422_PACKED		= 0x0210001f,
	ARV_PIXEL_FORMAT_YUV_444_PACKED		= 0x02180020,
	ARV_PIXEL_FORMAT_BAYER_GR_12_PACKED  	= 0x810c0001,
	ARV_PIXEL_FORMAT_BAYER_RG_12_PACKED  	= 0x810c0002,
	ARV_PIXEL_FORMAT_BAYER_BG_12_PACKED  	= 0x810c0004,
	ARV_PIXEL_FORMAT_BAYER_GB_12_PACKED  	= 0x810c0003,
	ARV_PIXEL_FORMAT_YUV_422_YUYV_PACKED 	= 0x82100005,
	ARV_PIXEL_FORMAT_BAYER_GR_16		= 0x81100006,
	ARV_PIXEL_FORMAT_BAYER_RG_16		= 0x81100007,
	ARV_PIXEL_FORMAT_BAYER_GB_16		= 0x81100008,
	ARV_PIXEL_FORMAT_BAYER_BG_16		= 0x81100009
} ArvPixelFormat;

const char * 		arv_acquisition_mode_to_string 		(ArvAcquisitionMode value);
ArvAcquisitionMode 	arv_acquisition_mode_from_string	(const char *string);

const char * 		arv_trigger_source_to_string 		(ArvTriggerSource value);
ArvTriggerSource 	arv_trigger_source_from_string		(const char *string);

G_END_DECLS

#endif
