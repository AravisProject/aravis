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

#ifndef ARV_ENUMS_H
#define ARV_ENUMS_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <glib-object.h>

G_BEGIN_DECLS

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

ARV_API const char *		arv_auto_to_string		(ArvAuto value);
ARV_API ArvAuto 		arv_auto_from_string		(const char *string);

/**
 * ArvAcquisitionMode:
 * @ARV_ACQUISITION_MODE_CONTINUOUS: frames are captured continuously until stopped with the AcquisitionStop command.
 * @ARV_ACQUISITION_MODE_SINGLE_FRAME: only one frame will be acquired
 * @ARV_ACQUISITION_MODE_MULTI_FRAME: the number of frames specified by AcquisitionFrameCount is captured.
 */

typedef enum {
	ARV_ACQUISITION_MODE_CONTINUOUS,
	ARV_ACQUISITION_MODE_SINGLE_FRAME,
	ARV_ACQUISITION_MODE_MULTI_FRAME
} ArvAcquisitionMode;

ARV_API const char *		arv_acquisition_mode_to_string		(ArvAcquisitionMode value);
ARV_API ArvAcquisitionMode	arv_acquisition_mode_from_string	(const char *string);

/**
 * ArvExposureMode:
 * @ARV_EXPOSURE_MODE_OFF: disables the Exposure and let the shutter open.
 * @ARV_EXPOSURE_MODE_TIMED: timed exposure. The exposure duration time is set using the ExposureTime or ExposureAuto
 * features and the exposure starts with the FrameStart or LineStart.
 * @ARV_EXPOSURE_MODE_TRIGGER_WIDTH: uses the width of the current Frame or Line trigger signal(s) pulse to control the
 * exposure duration. Note that if the Frame or Line TriggerActivation is RisingEdge or LevelHigh, the exposure duration
 * will be the time the trigger stays High. If TriggerActivation is FallingEdge or LevelLow, the exposure time will last
 * as long as the trigger stays Low.
 * @ARV_EXPOSURE_MODE_TRIGGER_CONTROLLED: uses one or more trigger signal(s) to control the exposure duration
 * independently from the current Frame or Line triggers. See ExposureStart, ExposureEnd and ExposureActive of
 * the TriggerSelector feature.
 */

typedef enum {
	ARV_EXPOSURE_MODE_OFF,
	ARV_EXPOSURE_MODE_TIMED,
	ARV_EXPOSURE_MODE_TRIGGER_WIDTH,
	ARV_EXPOSURE_MODE_TRIGGER_CONTROLLED
} ArvExposureMode;

ARV_API const char *		arv_exposure_mode_to_string		(ArvExposureMode value);
ARV_API ArvExposureMode		arv_exposure_mode_from_string		(const char *string);

/**
 * ArvGainSelector:
 * @ARV_GAIN_SELECTOR_ALL: Gain will be applied to all channels or taps.
 * @ARV_GAIN_SELECTOR_RED: Gain will be applied to the red channel.
 * @ARV_GAIN_SELECTOR_GREEN: Gain will be applied to the green channel.
 * @ARV_GAIN_SELECTOR_BLUE: Gain will be applied to the blue channel.
 * @ARV_GAIN_SELECTOR_Y: Gain will be applied to Y channel.
 * @ARV_GAIN_SELECTOR_U: Gain will be applied to U channel.
 * @ARV_GAIN_SELECTOR_V: Gain will be applied to V channel.
 * @ARV_GAIN_SELECTOR_TAP1: Gain will be applied to Tap 1.
 * @ARV_GAIN_SELECTOR_TAP2: Gain will be applied to Tap 2.
 * ...
 * @ARV_GAIN_SELECTOR_ANALOG_ALL: Gain will be applied to all analog channels or taps.
 * @ARV_GAIN_SELECTOR_ANALOG_RED: Gain will be applied to the red analog channel.
 * @ARV_GAIN_SELECTOR_ANALOG_GREEN: Gain will be applied to the green analog channel.
 * @ARV_GAIN_SELECTOR_ANALOG_BLUE: Gain will be applied to the blue analog channel.
 * @ARV_GAIN_SELECTOR_ANALOG_Y: Gain will be applied to Y analog channel.
 * @ARV_GAIN_SELECTOR_ANALOG_U: Gain will be applied to U analog channel.
 * @ARV_GAIN_SELECTOR_ANALOG_V: Gain will be applied to V analog channel.
 * @ARV_GAIN_SELECTOR_ANALOG_TAP1: Analog gain will be applied to Tap 1.
 * @ARV_GAIN_SELECTOR_ANALOG_TAP2: Analog gain will be applied to Tap 2.
 * ...
 * @ARV_GAIN_SELECTOR_DIGITAL_ALL: Gain will be applied to all digital channels or taps.
 * @ARV_GAIN_SELECTOR_DIGITAL_RED: Gain will be applied to the red digital channel.
 * @ARV_GAIN_SELECTOR_DIGITAL_GREEN: Gain will be applied to the green digital channel.
 * @ARV_GAIN_SELECTOR_DIGITAL_BLUE: Gain will be applied to the blue digital channel.
 * @ARV_GAIN_SELECTOR_DIGITAL_Y: Gain will be applied to Y digital channel.
 * @ARV_GAIN_SELECTOR_DIGITAL_U: Gain will be applied to U digital channel. 
 */

typedef enum {
	ARV_GAIN_SELECTOR_ALL,
	ARV_GAIN_SELECTOR_RED,
	ARV_GAIN_SELECTOR_GREEN,
	ARV_GAIN_SELECTOR_BLUE,
	ARV_GAIN_SELECTOR_Y,
	ARV_GAIN_SELECTOR_U,
	ARV_GAIN_SELECTOR_V,
	ARV_GAIN_SELECTOR_TAP1,
	ARV_GAIN_SELECTOR_TAP2,
	ARV_GAIN_SELECTOR_ANALOG_ALL,
	ARV_GAIN_SELECTOR_ANALOG_RED,
	ARV_GAIN_SELECTOR_ANALOG_GREEN,
	ARV_GAIN_SELECTOR_ANALOG_BLUE,
	ARV_GAIN_SELECTOR_ANALOG_Y,
	ARV_GAIN_SELECTOR_ANALOG_U,
	ARV_GAIN_SELECTOR_ANALOG_V,
	ARV_GAIN_SELECTOR_ANALOG_TAP1,
	ARV_GAIN_SELECTOR_ANALOG_TAP2,
	ARV_GAIN_SELECTOR_DIGITAL_ALL,
	ARV_GAIN_SELECTOR_DIGITAL_RED,
	ARV_GAIN_SELECTOR_DIGITAL_GREEN,
	ARV_GAIN_SELECTOR_DIGITAL_BLUE,
	ARV_GAIN_SELECTOR_DIGITAL_Y,
	ARV_GAIN_SELECTOR_DIGITAL_U
} ArvSelector;

ARV_API const char *		arv_selector_to_string		(ArvSelector value);
ARV_API ArvSelector			arv_selector_from_string	(const char *string);

/**
 * ArvUvUsbMode:
 * @ARV_UV_USB_MODE_SYNC: utilize libusb synchronous device I/O API
 * @ARV_UV_USB_MODE_ASYNC: utilize libusb asynchronous device I/O API
 * @ARV_UV_USB_MODE_DEFAULT: default usb mode
 */

typedef enum
{
	ARV_UV_USB_MODE_SYNC,
	ARV_UV_USB_MODE_ASYNC,
        ARV_UV_USB_MODE_DEFAULT = ARV_UV_USB_MODE_SYNC
} ArvUvUsbMode;

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

#define ARV_PIXEL_FORMAT_MONO_14		((ArvPixelFormat) 0x01100025u)

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
