/* Aravis - Digital camera library
 *
 * Copyright © 2009-2023 Emmanuel Pacaud
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
 * ArvUvUsbMode:
 * @ARV_UV_USB_MODE_SYNC: utilize libusb synchronous device I/O API
 * @ARV_UV_USB_MODE_ASYNC: utilize libusb asynchronous device I/O API
 * @ARV_UV_USB_MODE_DEFAULT: default usb mode
 */

typedef enum
{
	ARV_UV_USB_MODE_SYNC,
	ARV_UV_USB_MODE_ASYNC,
        ARV_UV_USB_MODE_DEFAULT = ARV_UV_USB_MODE_ASYNC
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

#define ARV_PIXEL_FORMAT_COORD3D_ABC_8          ((ArvPixelFormat) 0x021800B2u)
#define ARV_PIXEL_FORMAT_COORD3D_ABC_8_PLANAR   ((ArvPixelFormat) 0x021800B3u)
#define ARV_PIXEL_FORMAT_COORD3D_ABC_10P        ((ArvPixelFormat) 0x021E00DBu)
#define ARV_PIXEL_FORMAT_COORD3D_ABC_10P_PLANAR ((ArvPixelFormat) 0x021E00DCu)
#define ARV_PIXEL_FORMAT_COORD3D_ABC_12P        ((ArvPixelFormat) 0x022400DEu)
#define ARV_PIXEL_FORMAT_COORD3D_ABC_12P_PLANAR ((ArvPixelFormat) 0x022400DFu)
#define ARV_PIXEL_FORMAT_COORD3D_ABC_16         ((ArvPixelFormat) 0x023000B9u)
#define ARV_PIXEL_FORMAT_COORD3D_ABC_16_PLANAR  ((ArvPixelFormat) 0x023000BAu)
#define ARV_PIXEL_FORMAT_COORD3D_ABC_32F        ((ArvPixelFormat) 0x026000C0u)
#define ARV_PIXEL_FORMAT_COORD3D_ABC_32F_PLANAR ((ArvPixelFormat) 0x026000C1u)
#define ARV_PIXEL_FORMAT_COORD3D_AC_8           ((ArvPixelFormat) 0x021000B4u)
#define ARV_PIXEL_FORMAT_COORD3D_AC_8_PLANAR    ((ArvPixelFormat) 0x021000B5u)
#define ARV_PIXEL_FORMAT_COORD3D_AC_10P         ((ArvPixelFormat) 0x021400F0u)
#define ARV_PIXEL_FORMAT_COORD3D_AC_10P_PLANAR  ((ArvPixelFormat) 0x021400F1u)
#define ARV_PIXEL_FORMAT_COORD3D_AC_12P         ((ArvPixelFormat) 0x021800F2u)
#define ARV_PIXEL_FORMAT_COORD3D_AC_12P_PLANAR  ((ArvPixelFormat) 0x021800F3u)
#define ARV_PIXEL_FORMAT_COORD3D_AC_16          ((ArvPixelFormat) 0x022000BBu)
#define ARV_PIXEL_FORMAT_COORD3D_AC_16_PLANAR   ((ArvPixelFormat) 0x022000BCu)
#define ARV_PIXEL_FORMAT_COORD3D_AC_32F         ((ArvPixelFormat) 0x024000C2u)
#define ARV_PIXEL_FORMAT_COORD3D_AC_32F_PLANAR  ((ArvPixelFormat) 0x024000C3u)
#define ARV_PIXEL_FORMAT_COORD3D_A_8            ((ArvPixelFormat) 0x010800AFu)
#define ARV_PIXEL_FORMAT_COORD3D_A_10P          ((ArvPixelFormat) 0x010A00D5u)
#define ARV_PIXEL_FORMAT_COORD3D_A_12P          ((ArvPixelFormat) 0x010C00D8u)
#define ARV_PIXEL_FORMAT_COORD3D_A_16           ((ArvPixelFormat) 0x011000B6u)
#define ARV_PIXEL_FORMAT_COORD3D_A_32F          ((ArvPixelFormat) 0x012000BDu)
#define ARV_PIXEL_FORMAT_COORD3D_B_8            ((ArvPixelFormat) 0x010800B0u)
#define ARV_PIXEL_FORMAT_COORD3D_B_10P          ((ArvPixelFormat) 0x010A00D6u)
#define ARV_PIXEL_FORMAT_COORD3D_B_12P          ((ArvPixelFormat) 0x010C00D9u)
#define ARV_PIXEL_FORMAT_COORD3D_B_16           ((ArvPixelFormat) 0x011000B7u)
#define ARV_PIXEL_FORMAT_COORD3D_B_32F          ((ArvPixelFormat) 0x012000BEu)
#define ARV_PIXEL_FORMAT_COORD3D_C_8            ((ArvPixelFormat) 0x010800B1u)
#define ARV_PIXEL_FORMAT_COORD3D_C_10P          ((ArvPixelFormat) 0x010A00D7u)
#define ARV_PIXEL_FORMAT_COORD3D_C_12P          ((ArvPixelFormat) 0x010C00DAu)
#define ARV_PIXEL_FORMAT_COORD3D_C_16           ((ArvPixelFormat) 0x011000B8u)
#define ARV_PIXEL_FORMAT_COORD3D_C_32F          ((ArvPixelFormat) 0x012000BFu)

#define ARV_PIXEL_FORMAT_DATA_8                 ((ArvPixelFormat) 0x01080116u)
#define ARV_PIXEL_FORMAT_DATA_8S                ((ArvPixelFormat) 0x01080117u)
#define ARV_PIXEL_FORMAT_DATA_16                ((ArvPixelFormat) 0x01100118u)
#define ARV_PIXEL_FORMAT_DATA_16S               ((ArvPixelFormat) 0x01100119u)
#define ARV_PIXEL_FORMAT_DATA_32                ((ArvPixelFormat) 0x0120011Au)
#define ARV_PIXEL_FORMAT_DATA_32F               ((ArvPixelFormat) 0x0120011Cu)
#define ARV_PIXEL_FORMAT_DATA_32S               ((ArvPixelFormat) 0x0120011Bu)
#define ARV_PIXEL_FORMAT_DATA_64                ((ArvPixelFormat) 0x0140011Du)
#define ARV_PIXEL_FORMAT_DATA_64F               ((ArvPixelFormat) 0x0140011Fu)
#define ARV_PIXEL_FORMAT_DATA_64S               ((ArvPixelFormat) 0x0140011Eu)

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
