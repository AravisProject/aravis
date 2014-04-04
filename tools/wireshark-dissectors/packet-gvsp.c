/* packet-gvsp.c
 * Routines for AIA GigE Vision (TM) Streaming Protocol dissection
 * Copyright 2012, AIA <www.visiononline.org> All rights reserved
 *
 * GigE Vision (TM): GigE Vision a standard developed under the sponsorship of the AIA for 
 * the benefit of the machine vision industry. GVSP stands for GigE Vision (TM) Streaming
 * Protocol.
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*#ifdef HAVE_CONFIG_H */
#include "config.h"
/*#endif */

#ifdef PACKAGE
#undef PACKAGE
#endif

#define PACKAGE "gvsp"

#ifdef VERSION
#undef VERSION
#endif

#define VERSION "0.0.1"

#include <epan/packet.h>


/*
  Payload types
 */

#define GVSP_PAYLOAD_IMAGE ( 0x0001 )
#define GVSP_PAYLOAD_RAWDATA (0x0002 )
#define GVSP_PAYLOAD_FILE ( 0x0003 )
#define GVSP_PAYLOAD_CHUNKDATA ( 0x0004 )
#define GVSP_PAYLOAD_EXTENDEDCHUNKDATA ( 0x0005 ) /* Deprecated */
#define GVSP_PAYLOAD_JPEG ( 0x0006 )
#define GVSP_PAYLOAD_JPEG2000 ( 0x0007 )
#define GVSP_PAYLOAD_H264 ( 0x0008 )
#define GVSP_PAYLOAD_MULTIZONEIMAGE ( 0x0009 )
#define GVSP_PAYLOAD_DEVICEPSECIFICSTART ( 0x8000 )


/*
   GVSP packet types
 */

#define GVSP_PACKET_LEADER ( 1 )
#define GVSP_PACKET_TRAILER ( 2 )
#define GVSP_PACKET_PAYLOAD ( 3 )
#define GVSP_PACKET_ALLIN ( 4 )
#define GVSP_PACKET_PAYLOAD_H264 ( 5 )
#define GVSP_PACKET_PAYLOAD_MULTIZONE ( 6 )


/*
   GVSP statuses
 */

#define GEV_STATUS_SUCCESS (0x0000) 
#define GEV_STATUS_PACKET_RESEND (0x0100) 
#define GEV_STATUS_NOT_IMPLEMENTED (0x8001) 
#define GEV_STATUS_INVALID_PARAMETER (0x8002) 
#define GEV_STATUS_INVALID_ADDRESS (0x8003) 
#define GEV_STATUS_WRITE_PROTECT (0x8004) 
#define GEV_STATUS_BAD_ALIGNMENT (0x8005) 
#define GEV_STATUS_ACCESS_DENIED (0x8006) 
#define GEV_STATUS_BUSY (0x8007) 
#define GEV_STATUS_LOCAL_PROBLEM (0x8008)  /* deprecated */
#define GEV_STATUS_MSG_MISMATCH (0x8009) /* deprecated */
#define GEV_STATUS_INVALID_PROTOCOL (0x800A) /* deprecated */
#define GEV_STATUS_NO_MSG (0x800B) /* deprecated */
#define GEV_STATUS_PACKET_UNAVAILABLE (0x800C) 
#define GEV_STATUS_DATA_OVERRUN (0x800D) 
#define GEV_STATUS_INVALID_HEADER (0x800E) 
#define GEV_STATUS_WRONG_CONFIG (0x800F) /* deprecated */
#define GEV_STATUS_PACKET_NOT_YET_AVAILABLE (0x8010) 
#define GEV_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY (0x8011) 
#define GEV_STATUS_PACKET_REMOVED_FROM_MEMORY (0x8012) 
#define GEV_STATUS_ERROR (0x8FFF) 


/*
   Pixel type color
 */

#define GVSP_PIX_MONO             (0x01000000)
#define GVSP_PIX_RGB              (0x02000000)
#define GVSP_PIX_COLOR            (0x02000000)
#define GVSP_PIX_CUSTOM           (0x80000000)
#define GVSP_PIX_COLOR_MASK       (0xFF000000)


/*
   Pixel type size
 */

#define GVSP_PIX_OCCUPY1BIT       (0x00010000)
#define GVSP_PIX_OCCUPY2BIT       (0x00020000)
#define GVSP_PIX_OCCUPY4BIT       (0x00040000)
#define GVSP_PIX_OCCUPY8BIT       (0x00080000)
#define GVSP_PIX_OCCUPY12BIT      (0x000C0000)
#define GVSP_PIX_OCCUPY16BIT      (0x00100000)
#define GVSP_PIX_OCCUPY24BIT      (0x00180000)
#define GVSP_PIX_OCCUPY32BIT      (0x00200000)
#define GVSP_PIX_OCCUPY36BIT      (0x00240000)
#define GVSP_PIX_OCCUPY48BIT      (0x00300000)


/*
   Pxiel type masks, shifts 
 */
#define GVSP_PIX_EFFECTIVE_PIXEL_SIZE_MASK (0x00FF0000)
#define GVSP_PIX_EFFECTIVE_PIXEL_SIZE_SHIFT (16)

#define GVSP_PIX_ID_MASK (0x0000FFFF)


/*
   Pixel types
 */

#define GVSP_PIX_COUNT (0x46)

#define GVSP_PIX_MONO1P                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY1BIT  | 0x0037)
#define GVSP_PIX_MONO2P                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY2BIT  | 0x0038)
#define GVSP_PIX_MONO4P                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY4BIT  | 0x0039)
#define GVSP_PIX_MONO8                   (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0001)
#define GVSP_PIX_MONO8S                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0002)
#define GVSP_PIX_MONO10                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0003)
#define GVSP_PIX_MONO10_PACKED           (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0004)
#define GVSP_PIX_MONO12                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0005)
#define GVSP_PIX_MONO12_PACKED           (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0006)
#define GVSP_PIX_MONO14                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0025)
#define GVSP_PIX_MONO16                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0007)
											    
#define GVSP_PIX_BAYGR8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0008)
#define GVSP_PIX_BAYRG8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0009)
#define GVSP_PIX_BAYGB8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x000A)
#define GVSP_PIX_BAYBG8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x000B)
#define GVSP_PIX_BAYGR10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000C)
#define GVSP_PIX_BAYRG10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000D)
#define GVSP_PIX_BAYGB10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000E)
#define GVSP_PIX_BAYBG10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000F)
#define GVSP_PIX_BAYGR12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0010)
#define GVSP_PIX_BAYRG12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0011)
#define GVSP_PIX_BAYGB12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0012)
#define GVSP_PIX_BAYBG12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0013)
#define GVSP_PIX_BAYGR10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0026)
#define GVSP_PIX_BAYRG10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0027)
#define GVSP_PIX_BAYGB10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0028)
#define GVSP_PIX_BAYBG10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0029)
#define GVSP_PIX_BAYGR12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002A)
#define GVSP_PIX_BAYRG12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002B)
#define GVSP_PIX_BAYGB12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002C)
#define GVSP_PIX_BAYBG12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002D)
#define GVSP_PIX_BAYGR16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x002E)
#define GVSP_PIX_BAYRG16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x002F)
#define GVSP_PIX_BAYGB16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0030)
#define GVSP_PIX_BAYBG16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0031)
								         
#define GVSP_PIX_RGB8                    (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0014)
#define GVSP_PIX_BGR8                    (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0015)
#define GVSP_PIX_RGBA8                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x0016)
#define GVSP_PIX_BGRA8                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x0017)
#define GVSP_PIX_RGB10                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0018)
#define GVSP_PIX_BGR10                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0019)
#define GVSP_PIX_RGB12                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x001A)
#define GVSP_PIX_BGR12                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x001B)
#define GVSP_PIX_RGB16                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0033)
#define GVSP_PIX_RGB10V1_PACKED          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x001C)
#define GVSP_PIX_RGB10P32                (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x001D)
#define GVSP_PIX_RGB12V1_PACKED          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY36BIT | 0x0034)
#define GVSP_PIX_RGB565P                 (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0035)
#define GVSP_PIX_BGR565P                 (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0036)

#define GVSP_PIX_YUV411_8_UYYVYY         (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x001E)
#define GVSP_PIX_YUV422_8_UYVY           (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x001F)
#define GVSP_PIX_YUV422_8                (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0032)
#define GVSP_PIX_YUV8_UYV                (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0020)
#define GVSP_PIX_YCBCR8_CBYCR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x003A)
#define GVSP_PIX_YCBCR422_8              (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x003B)
#define GVSP_PIX_YCBCR422_8_CBYCRY       (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0043)
#define GVSP_PIX_YCBCR422_8_CBYYCRYY     (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x003C)
#define GVSP_PIX_YCBCR601_8_CBYCR        (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x003D)
#define GVSP_PIX_YCBCR601_422_8          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x003E)
#define GVSP_PIX_YCBCR601_422_8_CBYCRY   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0044)
#define GVSP_PIX_YCBCR601_411_8_CBYYCRYY (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x003F)
#define GVSP_PIX_YCBCR709_411_8_CBYCR    (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0040)
#define GVSP_PIX_YCBCR709_422_8          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0041)
#define GVSP_PIX_YCBCR709_422_8_CBYCRY   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0045)
#define GVSP_PIX_YCBCR709_411_8_CBYYCRYY (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x0042)

#define GVSP_PIX_RGB8_PLANAR             (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0021)
#define GVSP_PIX_RGB10_PLANAR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0022)
#define GVSP_PIX_RGB12_PLANAR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0023)
#define GVSP_PIX_RGB16_PLANAR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0024)


typedef struct _display_string 
{
	int display;
	gchar* string_display;

} display_string;


/* Structure to hold GVSP packet information */
typedef struct _gvsp_packet_info
{
	gint chunk;
	guint8 format;
	guint16 payloadtype;
	guint64 blockid;
	guint32 packetid;
	gint enhanced;
	gint flag_resendrangeerror;
	gint flag_previousblockdropped;
	gint flag_packetresend;

	const char *formatstring;
	const char *payloadstring;

} gvsp_packet_info;


/*Define the gvsp proto */
static int proto_gvsp = -1;
static int global_gvsp_port = 20202;


/* Define the tree for gvsp */
static int ett_gvsp = -1;
static int ett_gvsp_flags = -1;
static int ett_gvsp_header = -1;
static int ett_gvsp_payload = -1;
static int ett_gvsp_trailer = -1;


typedef struct _ftenum_string {
	enum ftenum type;
	gchar* string_type;
} ftenum_string;

static const value_string statusnames[] = {
    { GEV_STATUS_SUCCESS, "GEV_STATUS_SUCCESS" },
    { GEV_STATUS_PACKET_RESEND, "GEV_STATUS_PACKET_RESEND" },
    { GEV_STATUS_NOT_IMPLEMENTED, "GEV_STATUS_NOT_IMPLEMENTED" },
    { GEV_STATUS_INVALID_PARAMETER, "GEV_STATUS_INVALID_PARAMETER" },
    { GEV_STATUS_INVALID_ADDRESS, "GEV_STATUS_INVALID_ADDRESS" },
    { GEV_STATUS_WRITE_PROTECT, "GEV_STATUS_WRITE_PROTECT" },
    { GEV_STATUS_BAD_ALIGNMENT, "GEV_STATUS_BAD_ALIGNMENT" },
    { GEV_STATUS_ACCESS_DENIED, "GEV_STATUS_ACCESS_DENIED" },
    { GEV_STATUS_BUSY, "GEV_STATUS_BUSY" },
    { GEV_STATUS_LOCAL_PROBLEM, "GEV_STATUS_LOCAL_PROBLEM (deprecated)" },
    { GEV_STATUS_MSG_MISMATCH, "GEV_STATUS_MSG_MISMATCH (deprecated)" },
    { GEV_STATUS_INVALID_PROTOCOL, "GEV_STATUS_INVALID_PROTOCOL (deprecated)" },
    { GEV_STATUS_NO_MSG, "GEV_STATUS_NO_MSG (deprecated)" },
    { GEV_STATUS_PACKET_UNAVAILABLE, "GEV_STATUS_PACKET_UNAVAILABLE" },
    { GEV_STATUS_DATA_OVERRUN, "GEV_STATUS_DATA_OVERRUN" },
    { GEV_STATUS_INVALID_HEADER, "GEV_STATUS_INVALID_HEADER" },
    { GEV_STATUS_WRONG_CONFIG, "GEV_STATUS_WRONG_CONFIG (deprecated)" },
    { GEV_STATUS_PACKET_NOT_YET_AVAILABLE, "GEV_STATUS_PACKET_NOT_YET_AVAILABLE" },
    { GEV_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY, "GEV_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY" },
    { GEV_STATUS_PACKET_REMOVED_FROM_MEMORY, "GEV_STATUS_PACKET_REMOVED_FROM_MEMORY" },
    { GEV_STATUS_ERROR, "GEV_STATUS_ERROR" },
    { 0, NULL },
};

static const value_string formatnames[] = {
    { GVSP_PACKET_LEADER, "LEADER" },
    { GVSP_PACKET_TRAILER, "TRAILER" },
    { GVSP_PACKET_PAYLOAD, "PAYLOAD" },
    { GVSP_PACKET_ALLIN, "ALLIN" },
    { GVSP_PACKET_PAYLOAD_H264, "H264" },
    { GVSP_PACKET_PAYLOAD_MULTIZONE, "MULTIZONE" },
    { 0x80 | GVSP_PACKET_LEADER, "LEADER (ext IDs)" },
    { 0x80 | GVSP_PACKET_TRAILER, "TRAILER (ext IDs)" },
    { 0x80 | GVSP_PACKET_PAYLOAD, "PAYLOAD (ext IDs)" },
    { 0x80 | GVSP_PACKET_ALLIN, "ALLIN (ext IDs)" },
    { 0x80 | GVSP_PACKET_PAYLOAD_H264, "H264 (ext IDs)" },
    { 0x80 | GVSP_PACKET_PAYLOAD_MULTIZONE, "MULTIZONE (ext IDs)" },
    { 0, NULL },
};

static const value_string payloadtypenames[] = {
    { GVSP_PAYLOAD_IMAGE, "IMAGE" },
    { GVSP_PAYLOAD_RAWDATA, "RAWDATA" },
    { GVSP_PAYLOAD_FILE, "FILE" },
    { GVSP_PAYLOAD_CHUNKDATA, "CHUNKDATA" },
    { GVSP_PAYLOAD_EXTENDEDCHUNKDATA, "EXTENDEDCHUNKDATA (obsolete with v2.0)" },
    { GVSP_PAYLOAD_JPEG, "JPEG" },
    { GVSP_PAYLOAD_JPEG2000, "JPEG2000" },
    { GVSP_PAYLOAD_H264, "H264" },
    { GVSP_PAYLOAD_MULTIZONEIMAGE, "MUTLIZONEIAMGE" },
    { 0x4000 | GVSP_PAYLOAD_IMAGE, "IMAGE (v2.0 chunks)" },
    { 0x4000 | GVSP_PAYLOAD_RAWDATA, "RAWDATA (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_FILE, "FILE (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_CHUNKDATA, "CHUNKDATA (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_EXTENDEDCHUNKDATA, "EXTENDEDCHUNKDATA (v2.0 chunks?)" },
    { 0x4000 | GVSP_PAYLOAD_JPEG, "JPEG (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_JPEG2000, "JPEG2000 (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_H264, "H264 (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_MULTIZONEIMAGE, "MULTIZONEIMAGE (v2.0 Chunks)" },
    { 0, NULL },
};

static const value_string pixeltypenames[] = {
	{ GVSP_PIX_MONO1P, "GVSP_PIX_MONO1P" },                  
    { GVSP_PIX_MONO2P, "GVSP_PIX_MONO2P" },                  
    { GVSP_PIX_MONO4P, "GVSP_PIX_MONO4P" },
    { GVSP_PIX_MONO8, "GVSP_PIX_MONO8" },                   
    { GVSP_PIX_MONO8S, "GVSP_PIX_MONO8S" },                  
    { GVSP_PIX_MONO10, "GVSP_PIX_MONO10" },                  
    { GVSP_PIX_MONO10_PACKED, "GVSP_PIX_MONO10_PACKED" },           
    { GVSP_PIX_MONO12, "GVSP_PIX_MONO12" },                  
    { GVSP_PIX_MONO12_PACKED, "GVSP_PIX_MONO12_PACKED" },           
    { GVSP_PIX_MONO14, "GVSP_PIX_MONO14" },                  
    { GVSP_PIX_MONO16, "GVSP_PIX_MONO16" },                  
    { GVSP_PIX_BAYGR8, "GVSP_PIX_BAYGR8" },                  
    { GVSP_PIX_BAYRG8, "GVSP_PIX_BAYRG8" },                  
    { GVSP_PIX_BAYGB8, "GVSP_PIX_BAYGB8" },                  
    { GVSP_PIX_BAYBG8, "GVSP_PIX_BAYBG8" },                  
    { GVSP_PIX_BAYGR10, "GVSP_PIX_BAYGR10" },                 
    { GVSP_PIX_BAYRG10, "GVSP_PIX_BAYRG10" },                 
    { GVSP_PIX_BAYGB10, "GVSP_PIX_BAYGB10" },                 
    { GVSP_PIX_BAYBG10, "GVSP_PIX_BAYBG10" },                 
    { GVSP_PIX_BAYGR12, "GVSP_PIX_BAYGR12" },                 
    { GVSP_PIX_BAYRG12, "GVSP_PIX_BAYRG12" },                 
    { GVSP_PIX_BAYGB12, "GVSP_PIX_BAYGB12" },                 
    { GVSP_PIX_BAYBG12, "GVSP_PIX_BAYBG12" },                 
    { GVSP_PIX_BAYGR10_PACKED, "GVSP_PIX_BAYGR10_PACKED" },          
    { GVSP_PIX_BAYRG10_PACKED, "GVSP_PIX_BAYRG10_PACKED" },          
    { GVSP_PIX_BAYGB10_PACKED, "GVSP_PIX_BAYGB10_PACKED" },          
    { GVSP_PIX_BAYBG10_PACKED, "GVSP_PIX_BAYBG10_PACKED" },          
    { GVSP_PIX_BAYGR12_PACKED, "GVSP_PIX_BAYGR12_PACKED" },          
    { GVSP_PIX_BAYRG12_PACKED, "GVSP_PIX_BAYRG12_PACKED" },          
    { GVSP_PIX_BAYGB12_PACKED, "GVSP_PIX_BAYGB12_PACKED" },          
    { GVSP_PIX_BAYBG12_PACKED, "GVSP_PIX_BAYBG12_PACKED" },          
    { GVSP_PIX_BAYGR16, "GVSP_PIX_BAYGR16" },                 
    { GVSP_PIX_BAYRG16, "GVSP_PIX_BAYRG16" },                 
    { GVSP_PIX_BAYGB16, "GVSP_PIX_BAYGB16" },                 
    { GVSP_PIX_BAYBG16, "GVSP_PIX_BAYBG16" },                 
    { GVSP_PIX_RGB8, "GVSP_PIX_RGB8" },                    
    { GVSP_PIX_BGR8, "GVSP_PIX_BGR8" },                    
    { GVSP_PIX_RGBA8, "GVSP_PIX_RGBA8" },                   
    { GVSP_PIX_BGRA8, "GVSP_PIX_BGRA8" },                   
    { GVSP_PIX_RGB10, "GVSP_PIX_RGB10" },                   
    { GVSP_PIX_BGR10, "GVSP_PIX_BGR10" },                   
    { GVSP_PIX_RGB12, "GVSP_PIX_RGB12" },                   
    { GVSP_PIX_BGR12, "GVSP_PIX_BGR12" },                   
    { GVSP_PIX_RGB16, "GVSP_PIX_RGB16" },                   
    { GVSP_PIX_RGB10V1_PACKED, "GVSP_PIX_RGB10V1_PACKED" },          
    { GVSP_PIX_RGB10P32, "GVSP_PIX_RGB10P32" },                
    { GVSP_PIX_RGB12V1_PACKED, "GVSP_PIX_RGB12V1_PACKED" },          
    { GVSP_PIX_RGB565P, "GVSP_PIX_RGB565P" },                 
    { GVSP_PIX_BGR565P, "GVSP_PIX_BGR565P" },                 
    { GVSP_PIX_YUV411_8_UYYVYY, "GVSP_PIX_YUV411_8_UYYVYY" },         
    { GVSP_PIX_YUV422_8_UYVY, "GVSP_PIX_YUV422_8_UYVY" },           
    { GVSP_PIX_YUV422_8, "GVSP_PIX_YUV422_8" },                
    { GVSP_PIX_YUV8_UYV, "GVSP_PIX_YUV8_UYV" },                
    { GVSP_PIX_YCBCR8_CBYCR, "GVSP_PIX_YCBCR8_CBYCR" },            
    { GVSP_PIX_YCBCR422_8, "GVSP_PIX_YCBCR422_8" },              
    { GVSP_PIX_YCBCR422_8_CBYCRY, "GVSP_PIX_YCBCR422_8_CBYCRY" },       
    { GVSP_PIX_YCBCR422_8_CBYYCRYY, "GVSP_PIX_YCBCR422_8_CBYYCRYY" },     
    { GVSP_PIX_YCBCR601_8_CBYCR, "GVSP_PIX_YCBCR601_8_CBYCR" },        
    { GVSP_PIX_YCBCR601_422_8, "GVSP_PIX_YCBCR601_422_8" },          
    { GVSP_PIX_YCBCR601_422_8_CBYCRY, "GVSP_PIX_YCBCR601_422_8_CBYCRY" },   
    { GVSP_PIX_YCBCR601_411_8_CBYYCRYY, "GVSP_PIX_YCBCR601_411_8_CBYYCRYY" }, 
    { GVSP_PIX_YCBCR709_411_8_CBYCR, "GVSP_PIX_YCBCR709_411_8_CBYCR" },    
    { GVSP_PIX_YCBCR709_422_8, "GVSP_PIX_YCBCR709_422_8" },         
    { GVSP_PIX_YCBCR709_422_8, "GVSP_PIX_YCBCR709_422_8" },          
    { GVSP_PIX_YCBCR709_411_8_CBYYCRYY, "GVSP_PIX_YCBCR709_411_8_CBYYCRYY" }, 
    { GVSP_PIX_RGB8_PLANAR, "GVSP_PIX_RGB8_PLANAR" },             
    { GVSP_PIX_RGB10_PLANAR, "GVSP_PIX_RGB10_PLANAR" },            
    { GVSP_PIX_RGB12_PLANAR, "GVSP_PIX_RGB12_PLANAR" },            
    { GVSP_PIX_RGB16_PLANAR, "GVSP_PIX_RGB16_PLANAR" },            
    { 0, NULL },
};

static const value_string colornames[] = {
    { GVSP_PIX_MONO >> 24, "Mono" },
    { GVSP_PIX_COLOR >> 24, "Color" },
    { GVSP_PIX_CUSTOM >> 24, "Custom" },
    { 0, NULL },
};

static const true_false_string directionnames = {
	"Receiver",
	"Transmitter"
};


static int hf_gvsp_status = -1;
static int hf_gvsp_blockid16 = -1;
static int hf_gvsp_flags = -1;
static int hf_gvsp_flagdevicespecific0 = -1;
static int hf_gvsp_flagdevicespecific1 = -1;
static int hf_gvsp_flagdevicespecific2 = -1;
static int hf_gvsp_flagdevicespecific3 = -1;
static int hf_gvsp_flagdevicespecific4 = -1;
static int hf_gvsp_flagdevicespecific5 = -1;
static int hf_gvsp_flagdevicespecific6 = -1;
static int hf_gvsp_flagdevicespecific7 = -1;
static int hf_gvsp_flagresendrangeerror = -1;
static int hf_gvsp_flagpreviousblockdropped = -1;
static int hf_gvsp_flagpacketresend = -1;
static int hf_gvsp_format = -1;
static int hf_gvsp_packetid24 = -1;
static int hf_gvsp_blockid64 = -1;
static int hf_gvsp_packetid32 = -1;
static int hf_gvsp_payloadtype = -1;
static int hf_gvsp_payloaddata = -1;
static int hf_gvsp_timestamp = -1;
static int hf_gvsp_pixelformat = -1;
static int hf_gvsp_sizex = -1;
static int hf_gvsp_sizey = -1;
static int hf_gvsp_offsetx = -1;
static int hf_gvsp_offsety = -1;
static int hf_gvsp_paddingx = -1;
static int hf_gvsp_paddingy = -1;
static int hf_gvsp_payloaddatasize = -1;
static int hf_gvsp_pixelcolor = -1;
static int hf_gvsp_pixeloccupy = -1;
static int hf_gvsp_pixelid = -1;
static int hf_gvsp_filename = -1;
static int hf_gvsp_payloadlength = -1;
static int hf_gvsp_fieldinfo = -1;
static int hf_gvsp_fieldid = -1;
static int hf_gvsp_fieldcount = -1;
static int hf_gvsp_genericflags = -1;
static int hf_gvsp_chunklayoutid = -1;
static int hf_gvsp_timestamptickfrequency = -1;
static int hf_gvsp_dataformat = -1;
static int hf_gvsp_packetizationmode = -1;
static int hf_gvsp_packetsize = -1;
static int hf_gvsp_profileidc = -1;
static int hf_gvsp_cs = -1;
static int hf_gvsp_cs0 = -1;
static int hf_gvsp_cs1 = -1;
static int hf_gvsp_cs2 = -1;
static int hf_gvsp_cs3 = -1;
static int hf_gvsp_levelidc = -1;
static int hf_gvsp_sropinterleavingdepth = -1;
static int hf_gvsp_sropmaxdondiff = -1;
static int hf_gvsp_sropdeintbufreq = -1;
static int hf_gvsp_sropinitbuftime = -1;
static int hf_gvsp_zoneinfo = -1;
static int hf_gvsp_zoneid = -1;
static int hf_gvsp_endofzone = -1;
static int hf_gvsp_addressoffsethigh = -1;
static int hf_gvsp_addressoffsetlow = -1;
static int hf_gvsp_sc_zone_direction = -1;
static int hf_gvsp_sc_zone0_direction = -1;
static int hf_gvsp_sc_zone1_direction = -1;
static int hf_gvsp_sc_zone2_direction = -1;
static int hf_gvsp_sc_zone3_direction = -1;
static int hf_gvsp_sc_zone4_direction = -1;
static int hf_gvsp_sc_zone5_direction = -1;
static int hf_gvsp_sc_zone6_direction = -1;
static int hf_gvsp_sc_zone7_direction = -1;
static int hf_gvsp_sc_zone8_direction = -1;
static int hf_gvsp_sc_zone9_direction = -1;
static int hf_gvsp_sc_zone10_direction = -1;
static int hf_gvsp_sc_zone11_direction = -1;
static int hf_gvsp_sc_zone12_direction = -1;
static int hf_gvsp_sc_zone13_direction = -1;
static int hf_gvsp_sc_zone14_direction = -1;
static int hf_gvsp_sc_zone15_direction = -1;
static int hf_gvsp_sc_zone16_direction = -1;
static int hf_gvsp_sc_zone17_direction = -1;
static int hf_gvsp_sc_zone18_direction = -1;
static int hf_gvsp_sc_zone19_direction = -1;
static int hf_gvsp_sc_zone20_direction = -1;
static int hf_gvsp_sc_zone21_direction = -1;
static int hf_gvsp_sc_zone22_direction = -1;
static int hf_gvsp_sc_zone23_direction = -1;
static int hf_gvsp_sc_zone24_direction = -1;
static int hf_gvsp_sc_zone25_direction = -1;
static int hf_gvsp_sc_zone26_direction = -1;
static int hf_gvsp_sc_zone27_direction = -1;
static int hf_gvsp_sc_zone28_direction = -1;
static int hf_gvsp_sc_zone29_direction = -1;
static int hf_gvsp_sc_zone30_direction = -1;
static int hf_gvsp_sc_zone31_direction = -1;
static int hf_gvsp_chunkdatapayloadlengthex = -1;
static int hf_gvsp_chunklayoutidex = -1;


/*
    \brief Dissects the image leader
 */

gint DissectImageLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{	
	proto_item *item = NULL;
	proto_item *ti = NULL;
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Pixel format */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_pixelformat, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_pixelcolor, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixeloccupy, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixelid, tvb, offset + 12, 4, ENC_BIG_ENDIAN);

		/* Size X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizex, tvb, offset + 16, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 20, 4, ENC_BIG_ENDIAN);

		/* Offset X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsetx, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* Offset Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsety, tvb, offset + 28, 4, ENC_BIG_ENDIAN);

		/* Padding X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingx, tvb, offset + 32, 2, ENC_BIG_ENDIAN);

		/* Padding Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingy, tvb, offset + 34, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 36;
}


/*
    \brief Dissects the image trailer
 */

gint DissectImageTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{	
	gint offset;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 8;
}


/*
    \brief Dissects the raw data leader
 */

gint DissectRawDataLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 12, 8, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 20;
}


/*
    \brief Dissects a file leader
 */

gint DissectFileLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 12, 8, ENC_BIG_ENDIAN);

		/* Filename */
		proto_tree_add_item(gvsp_tree, hf_gvsp_filename, tvb, offset + 20, -1, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 20; /* Problem with the spec? All-in does not work if variable header size... */
}


/*
    \brief Dissects a chunk data leader
 */

gint DissectChunkDataLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 12;
}


/*
    \brief Dissects a chunk data trailer
 */

gint DissectChunkDataTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Payload data length */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadlength, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 8;
}


/*
    \brief Dissects extended chunk data leader
 */

gint DissectExtendedChunkDataLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;	
    proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Generic flags */
		proto_tree_add_item(gvsp_tree, hf_gvsp_genericflags, tvb, offset + 1, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Pixel format */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_pixelformat, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_pixelcolor, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixeloccupy, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixelid, tvb, offset + 12, 4, ENC_BIG_ENDIAN);

		/* Size X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizex, tvb, offset + 16, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 20, 4, ENC_BIG_ENDIAN);

		/* Offset X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsetx, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* Offset Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsety, tvb, offset + 28, 4, ENC_BIG_ENDIAN);

		/* Padding X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingx, tvb, offset + 32, 2, ENC_BIG_ENDIAN);

		/* Padding Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingy, tvb, offset + 34, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 36;
}


/*
    \brief Dissects extended chunk data trailer
 */

gint DissectExtendedChunkDataTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Payload data length */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadlength, tvb, offset + 4, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 8, 4, ENC_BIG_ENDIAN);

		/* Chunk layout ID */
		proto_tree_add_item(gvsp_tree, hf_gvsp_chunklayoutid, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 16;
}


/*
    \brief Dissects a JPEG leader
 */

gint DissectJPEGLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Generic flags */
		proto_tree_add_item(gvsp_tree, hf_gvsp_genericflags, tvb, offset + 1, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 12, 8, ENC_BIG_ENDIAN);

		/* Timestamp tick frequency */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamptickfrequency, tvb, offset + 20, 8, ENC_BIG_ENDIAN);

		/* Data format */
		proto_tree_add_item(gvsp_tree, hf_gvsp_dataformat, tvb, offset + 24, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 28;
}


/*
 \brief Dissects a H264 leader
 */

gint DissectH264Leader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* packetization_mode */
		proto_tree_add_item(gvsp_tree, hf_gvsp_packetizationmode, tvb, offset + 13, 1, ENC_BIG_ENDIAN);

		/* packet_size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_packetsize, tvb, offset + 14, 2, ENC_BIG_ENDIAN);

		/* profile_idc */
		proto_tree_add_item(gvsp_tree, hf_gvsp_profileidc, tvb, offset + 17, 1, ENC_BIG_ENDIAN);

		/* cs0, 1, 2 ,3 */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_cs, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs0, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs1, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs2, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs3, tvb, offset + 18, 1, ENC_BIG_ENDIAN);

		/* level_idc */
		proto_tree_add_item(gvsp_tree, hf_gvsp_levelidc, tvb, offset + 19, 1, ENC_BIG_ENDIAN);

		/* srop_interleaving_depth */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropinterleavingdepth, tvb, offset + 20, 2, ENC_BIG_ENDIAN);

		/* srop_max_don_diff */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropmaxdondiff, tvb, offset + 22, 2, ENC_BIG_ENDIAN);

		/* srop_deint_buf_req */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropdeintbufreq, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* srop_init_buf_time */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropinitbuftime, tvb, offset + 28, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 32;
}


/*
    \brief Dissects the multizone image leader
 */

gint DissectMultizoneImageLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
    proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Zone direction */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_sc_zone_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_sc_zone0_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone1_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone2_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone3_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone4_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone5_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone6_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone7_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone8_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone9_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone10_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone11_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone12_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone13_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone14_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone15_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone16_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone17_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone18_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone19_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone20_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone21_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone22_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone23_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone24_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone25_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone26_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone27_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone28_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone29_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone30_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone31_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);

		/* Pixel format */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_pixelformat, tvb, offset + 16, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_pixelcolor, tvb, offset + 16, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixeloccupy, tvb, offset + 16, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixelid, tvb, offset + 16, 4, ENC_BIG_ENDIAN);

		/* Size X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizex, tvb, offset + 20, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* Offset X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsetx, tvb, offset + 28, 4, ENC_BIG_ENDIAN);

		/* Offset Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsety, tvb, offset + 32, 4, ENC_BIG_ENDIAN);

		/* Padding X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingx, tvb, offset + 36, 2, ENC_BIG_ENDIAN);

		/* Padding Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingy, tvb, offset + 38, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 40;
}


/*
    \brief Dissects a generic trailer (contains just the payload type)
 */

gint DissectGenericTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 4;
}


/*
    \brief Dissects a generic trailer (contains just the payload type)
 */

gint DissectExtraChunkInfo(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Chunk data payload length */
		proto_tree_add_item(gvsp_tree, hf_gvsp_chunkdatapayloadlengthex, tvb, offset, 4, ENC_BIG_ENDIAN);

		/* Chunk layoud id */
		proto_tree_add_item(gvsp_tree, hf_gvsp_chunklayoutidex, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 8;
}


/*
    \brief Dissects a packet payload
 */

void DissectPacketPayload(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddata, tvb, offset, -1, ENC_BIG_ENDIAN);
	}
}


/*
    \brief Dissects a packet payload for H264
 */

void DissectPacketPayloadH264(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 0, 8, ENC_BIG_ENDIAN);

		/* Data */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddata, tvb, offset + 8, -1, ENC_BIG_ENDIAN);
	}
}


/*
    \brief Dissects a packet payload for multizone
 */

void DissectPacketPayloadMultizone(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Zone information */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_zoneinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_zoneid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_endofzone, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Address offset high */
		proto_tree_add_item(gvsp_tree, hf_gvsp_addressoffsethigh, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Address offset low */
		proto_tree_add_item(gvsp_tree, hf_gvsp_addressoffsetlow, tvb, offset + 4, 4, ENC_BIG_ENDIAN);

		/* Data */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddata, tvb, offset + 8, -1, ENC_BIG_ENDIAN);
	}
}


/*
    \brief Dissects an all in packet
 */

void DissectPacketAllIn(proto_tree *gvsp_tree, tvbuff_t *tvb, gint startoffset, packet_info *pinfo, gvsp_packet_info *info)
{
	gint offset;
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		switch (info->payloadtype)
		{
		case GVSP_PAYLOAD_IMAGE:
			offset += DissectImageLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectImageTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_RAWDATA:
			offset += DissectRawDataLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_FILE:
			offset += DissectFileLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_CHUNKDATA:
			offset += DissectChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_EXTENDEDCHUNKDATA:
			offset += DissectExtendedChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectExtendedChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_JPEG:
		case GVSP_PAYLOAD_JPEG2000:
			offset += DissectJPEGLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_H264:
			offset += DissectH264Leader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayloadH264(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_MULTIZONEIMAGE:
			offset += DissectMultizoneImageLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectImageTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayloadMultizone(gvsp_tree, tvb, pinfo, offset);
			break;
		}
	}
}


/*
    \brief Dissects a leader packet
 */

void DissectPacketLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, gint startoffset, packet_info *pinfo, gvsp_packet_info *info)
{
	gint offset;
	offset = startoffset;

	switch (info->payloadtype)
	{
	case GVSP_PAYLOAD_IMAGE:
		DissectImageLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_RAWDATA:
		DissectRawDataLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_FILE:
		DissectFileLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_CHUNKDATA:
		DissectChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_EXTENDEDCHUNKDATA:
		DissectExtendedChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_JPEG:
	case GVSP_PAYLOAD_JPEG2000:
		DissectJPEGLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_H264:
		DissectH264Leader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_MULTIZONEIMAGE:
		DissectMultizoneImageLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	default:
		break;
	}
}


/*
    \brief Dissects a trailer packet
 */

void DissectPacketTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, gint startoffset, packet_info *pinfo, gvsp_packet_info *info)
{
	gint offset;
	offset = startoffset;

	switch (info->payloadtype)
	{
	case GVSP_PAYLOAD_IMAGE:
	case GVSP_PAYLOAD_MULTIZONEIMAGE:
		offset += DissectImageTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_CHUNKDATA:
		offset += DissectChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_EXTENDEDCHUNKDATA:
		offset += DissectExtendedChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_RAWDATA:
	case GVSP_PAYLOAD_FILE:
	case GVSP_PAYLOAD_JPEG:
	case GVSP_PAYLOAD_JPEG2000:
	case GVSP_PAYLOAD_H264:
		offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	default:
		break;
	}

	if (info->chunk != 0)
	{
		offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
	}
}


/*
    \brief Point of entry of all GVSP dissection
 */

static void dissect_gvsp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) 
{
    proto_item *ti = NULL;
    gint offset = 0;
	proto_tree *gvsp_tree = NULL;
	proto_item *item = NULL;
	gvsp_packet_info info;
	guint8 firstbyte = 0;

	memset(&info, 0x00, sizeof(info));

	/* Set the protocol column */
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "GVSP");

	/* Clear out stuff in the info column */
	col_clear(pinfo->cinfo, COL_INFO);

	/* Adds "Gigabit-Ethernet Streaming Protocol" heading to protocol tree */
	/* We will add fields to this using the gvsp_tree pointer */
	if (tree != NULL)
	{
		ti = proto_tree_add_item(tree, proto_gvsp, tvb, offset, -1, ENC_BIG_ENDIAN);
		gvsp_tree = proto_item_add_subtree(ti, ett_gvsp);
	}

	firstbyte = tvb_get_guint8(tvb, offset);
	if (firstbyte == 0x42)
	{
		/* Add packet format to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "GVCP Generic Packet (Firewall)");

		/* GVCP magic byte, must be a firewall opener packet */
		return;
	}

	/* Get packet format byte, look for enhanced flag and then clear it */
	info.format = tvb_get_guint8(tvb, offset + 4);
	info.formatstring = val_to_str(info.format, formatnames, "Unknown Format");
	info.enhanced = info.format & 0x80;
	info.format &= 0x7F;

	/* Add packet format to info string */
	col_append_fstr(pinfo->cinfo, COL_INFO, "%s ", info.formatstring);

	/* Dissect status */
	if (tree != NULL)
	{
		proto_tree_add_item(gvsp_tree, hf_gvsp_status, tvb, offset, 2, ENC_BIG_ENDIAN);
	}
	offset += 2;

	if (info.enhanced == 0)
	{
		info.blockid = tvb_get_ntohs(tvb, offset);

		/* Dissect block id 16 bits */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_blockid16, tvb, offset, 2, ENC_BIG_ENDIAN);
		}
		offset += 2;
	}
	else
	{
		/* Dissect flags */
		if (tree != NULL)
		{
			guint8 flags;
			flags = tvb_get_guint8(tvb, offset + 1);
			info.flag_resendrangeerror = flags & 0x04;
			info.flag_previousblockdropped = flags & 0x02;
			info.flag_packetresend = flags & 0x01;

			item = proto_tree_add_item(gvsp_tree, hf_gvsp_flags, tvb, offset, 2, ENC_BIG_ENDIAN);
			ti = proto_item_add_subtree(item, ett_gvsp_flags);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific0, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific1, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific2, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific3, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific4, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific5, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific6, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific7, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagresendrangeerror, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagpreviousblockdropped, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagpacketresend, tvb, offset, 2, ENC_BIG_ENDIAN);
		}
		offset += 2;
	}

	/* Dissect packet format */
	if (tree != NULL)
	{
		proto_tree_add_item(gvsp_tree, hf_gvsp_format, tvb, offset, 1, ENC_BIG_ENDIAN);
	}
	offset++;

	if (info.enhanced == 0)
	{
		info.packetid = tvb_get_ntohl(tvb, offset - 1);
		info.packetid &= 0x00FFFFFF;

		/* Dissect packet ID */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_packetid24, tvb, offset, 3, ENC_BIG_ENDIAN);
		}
		offset += 3;
	}
	else
	{
		/* Nothing... */
		offset += 3;
	}

	if (info.enhanced != 0)
	{
		guint64 high;
		guint64 low;
		high = tvb_get_ntohl(tvb, offset);
		low = tvb_get_ntohl(tvb, offset + 4);
		info.blockid = low | (high << 32);

		/* Dissect 64 bit block ID */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_blockid64, tvb, offset, 8, ENC_BIG_ENDIAN);
		}
		offset += 8;

		info.packetid = tvb_get_ntohl(tvb, offset);

		/* Dissect 32 bit packet ID */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_packetid32, tvb, offset, 4, ENC_BIG_ENDIAN);
		}
		offset += 4;
	}

	col_append_fstr(pinfo->cinfo, COL_INFO, "[Block ID: %" G_GINT64_MODIFIER "u Packet ID: %d] ", (guint64)info.blockid, info.packetid);

	if (info.flag_resendrangeerror != 0)
	{
		/* Add range error to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "[RANGE_ERROR] ");
	}

	if (info.flag_previousblockdropped != 0)
	{
		/* Add block dropped to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "[BLOCK_DROPPED] ");
	}

	if (info.flag_packetresend != 0)
	{
		/* Add packet resend to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "[PACKET_RESEND] ");
	}

	/* Process packet types that are payload agnostic */
	switch (info.format)
	{
	case GVSP_PACKET_PAYLOAD:
		DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
		return;

	case GVSP_PACKET_PAYLOAD_H264:
		DissectPacketPayloadH264(gvsp_tree, tvb, pinfo, offset);
		return;

	case GVSP_PACKET_PAYLOAD_MULTIZONE:
		DissectPacketPayloadMultizone(gvsp_tree, tvb, pinfo, offset);
		return;

	default:
		break;
	}

	/* Get payload type, clear chunk bit */
	info.payloadtype = tvb_get_ntohs(tvb, offset + 2);
	info.payloadstring = val_to_str(info.payloadtype, payloadtypenames, "Unknown Payload Type");
	info.chunk = info.payloadtype & 0x4000;
	info.payloadtype &= 0x3FFF;

	/* Add payload type to information string */
	col_append_fstr(pinfo->cinfo, COL_INFO, "%s ", info.payloadstring);

	/* Process packet types for specific payload types */
	switch (info.format)
	{
	case GVSP_PACKET_ALLIN:
		DissectPacketAllIn(gvsp_tree, tvb, offset, pinfo, &info);
		return;

	case GVSP_PACKET_LEADER:
		DissectPacketLeader(gvsp_tree, tvb, offset, pinfo, &info);
		return;

	case GVSP_PACKET_TRAILER:
		DissectPacketTrailer(gvsp_tree, tvb, offset, pinfo, &info);
		return;

	default:
		break;
	}
}


/*
    \brief Registers the dissector. Invoked when the pluging library is loaded.
 */

void proto_register_gvsp(void) 
{
    static hf_register_info hfgvsp[] = 
    {
        {& hf_gvsp_status,          
        { "Status", "gvsp.status",
        FT_UINT16, BASE_HEX, VALS(statusnames), 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_blockid16,           
        { "Block ID (16 bits)", "gvsp.blockid16",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_flags,           
        { "Flags", "gvsp.flags",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific0,         
        { "Flag Device Specific 0", "gvsp.flag.devicespecific0",
        FT_UINT16, BASE_HEX, NULL, 0x8000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific1,         
        { "Flag Device Specific 1", "gvsp.flag.devicespecific1",
        FT_UINT16, BASE_HEX, NULL, 0x4000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific2,         
        { "Flag Device Specific 2", "gvsp.flag.devicespecific2",
        FT_UINT16, BASE_HEX, NULL, 0x2000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific3,         
        { "Flag Device Specific 3", "gvsp.flag.devicespecific3",
        FT_UINT16, BASE_HEX, NULL, 0x1000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific4,         
        { "Flag Device Specific 4", "gvsp.flag.devicespecific4",
        FT_UINT16, BASE_HEX, NULL, 0x0800,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific5,         
        { "Flag Device Specific 5", "gvsp.flag.devicespecific5",
        FT_UINT16, BASE_HEX, NULL, 0x0400,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific6,         
        { "Flag Device Specific 6", "gvsp.flag.devicespecific6",
        FT_UINT16, BASE_HEX, NULL, 0x0200,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific7,         
        { "Flag Device Specific 7", "gvsp.flag.devicespecific7",
        FT_UINT16, BASE_HEX, NULL, 0x0100,
        NULL, HFILL
        }},

        {& hf_gvsp_flagresendrangeerror,            
        { "Flag Resend Range Error 7", "gvsp.flag.resendrangeerror",
        FT_UINT16, BASE_HEX, NULL, 0x0004,
        NULL, HFILL
        }},

        {& hf_gvsp_flagpreviousblockdropped,            
        { "Flag Previous Block Dropped", "gvsp.flag.previousblockdropped",
        FT_UINT16, BASE_HEX, NULL, 0x0002,
        NULL, HFILL
        }},

        {& hf_gvsp_flagpacketresend,            
        { "Flag Packet Resend", "gvsp.flag.packetresend",
        FT_UINT16, BASE_HEX, NULL, 0x0001,
        NULL, HFILL
        }},

        {& hf_gvsp_format,          
        { "Format", "gvsp.format",
        FT_UINT8, BASE_HEX, VALS(formatnames), 0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetid24,          
        { "Packet ID (24 bits)", "gvsp.packetid24",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_blockid64,           
        { "Block ID (64 bits v2.0)", "gvsp.blockid64",
        FT_UINT64, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetid32,          
        { "Packet ID (32 bits v2.0)", "gvsp.packetid32",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_payloadtype,         
        { "Payload Type", "gvsp.payloadtype",
        FT_UINT16, BASE_HEX, VALS(payloadtypenames), 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_payloaddata,         
        { "Payload Data", "gvsp.payloaddata",
        FT_BYTES, BASE_NONE, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_timestamp,           
        { "Timestamp", "gvsp.timestamp",
        FT_UINT64, BASE_HEX, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_pixelformat,         
        { "Pixel Format", "gvsp.pixel",
        FT_UINT32, BASE_HEX, VALS(pixeltypenames), 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_sizex,           
        { "Size X", "gvsp.sizex",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_sizey,           
        { "Size Y", "gvsp.sizey",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_offsetx,         
        { "Offset X", "gvsp.offsetx",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_offsety,         
        { "Offset Y", "gvsp.offsety",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_paddingx,            
        { "Padding X", "gvsp.paddingx",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_paddingy,            
        { "Padding Y", "gvsp.paddingy",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_payloaddatasize,         
        { "Payload Data Size", "gvsp.payloaddatasize",
        FT_UINT64, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_pixelcolor,          
        { "Monochrome or Color", "gvsp.pixel.color",
        FT_UINT32, BASE_HEX, VALS(colornames), 0xFF000000,
        NULL, HFILL
        }},

        {& hf_gvsp_pixeloccupy,         
        { "Occupy Bits", "gvsp.pixel.occupy",
        FT_UINT32, BASE_DEC, NULL, 0x00FF0000,
        NULL, HFILL
        }},

        {& hf_gvsp_pixelid,         
        { "ID", "gvsp.pixel.id",
        FT_UINT32, BASE_HEX, NULL, 0x0000FFFF,
        NULL, HFILL
        }},

        {& hf_gvsp_filename,            
        { "ID", "gvsp.filename",
        FT_STRINGZ, BASE_NONE, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_payloadlength,           
        { "Payload Length", "gvsp.payloadlength",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_fieldinfo,           
        { "Field Info", "gvsp.fieldinfo",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_fieldid,         
        { "Field ID", "gvsp.fieldid",
        FT_UINT8, BASE_HEX, NULL, 0xF0,
        NULL, HFILL
        }},

        {& hf_gvsp_fieldcount,          
        { "Field Count", "gvsp.fieldcount",
        FT_UINT8, BASE_HEX, NULL, 0x0F,
        NULL, HFILL
        }},

        {& hf_gvsp_genericflags,            
        { "Generic Flag", "gvsp.genericflag",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_timestamptickfrequency ,         
        { "Timestamp Tick Frequency", "gvsp.timestamptickfrequency",
        FT_UINT64, BASE_HEX, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_dataformat,          
        { "Data Format", "gvsp.dataformat",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetizationmode,           
        { "packetization_mode", "gvsp.packetizationmode",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetsize,          
        { "packet_size", "gvsp.packetsize",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_profileidc,          
        { "profile_idc", "gvsp.profileidc",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_cs,          
        { "cs", "gvsp.cs",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_cs0,         
        { "cs0", "gvsp.cs0",
        FT_UINT8, BASE_HEX, NULL, 0x80,
        NULL, HFILL
        }},

        {& hf_gvsp_cs1,         
        { "cs1", "gvsp.cs1",
        FT_UINT8, BASE_HEX, NULL, 0x40,
        NULL, HFILL
        }},

        {& hf_gvsp_cs2,         
        { "cs2", "gvsp.cs2",
        FT_UINT8, BASE_HEX, NULL, 0x20,
        NULL, HFILL
        }},

        {& hf_gvsp_cs3,         
        { "cs3", "gvsp.cs3",
        FT_UINT8, BASE_HEX, NULL, 0x10,
        NULL, HFILL
        }},

        {& hf_gvsp_levelidc,            
        { "level_idc", "gvsp.levelidc",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropinterleavingdepth,           
        { "srop_interlaving_depth", "gvsp.sropinterleavingdepth",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropmaxdondiff,          
        { "srop_max_don_diff", "gvsp.sropmaxdondiff",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropdeintbufreq,         
        { "srop_deint_buf_req", "gvsp.sropdeintbufreq",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropinitbuftime,         
        { "srop_init_buf_time", "gvsp.sropinitbuftime",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_zoneinfo,            
        { "Zone Info", "gvsp.zoneinfo",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_zoneid,          
        { "Zone ID", "gvsp.zoneid",
        FT_UINT8, BASE_HEX, NULL, 0x3E,
        NULL, HFILL
        }},

        {& hf_gvsp_endofzone,           
        { "End of Zone", "gvsp.endofzone",
        FT_UINT8, BASE_HEX, NULL, 0x01,
        NULL, HFILL
        }},

        {& hf_gvsp_addressoffsethigh,           
        { "Address Offset High", "gvsp.addressoffsethigh",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_addressoffsetlow,            
        { "Address Offset Low", "gvsp.addressoffsetlow",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sc_zone_direction,   
        { "Zone Directions Mask", "gvcpzonedirection",
        FT_BOOLEAN, 32, TFS(&directionnames), 0,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone0_direction,  
        { "Zone 0 Direction", "gvcp.zone0direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x80000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone1_direction,  
        { "Zone 1 Direction", "gvcp.zone1direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x40000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone2_direction,  
        { "Zone 2 Direction", "gvcp.zone2direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x20000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone3_direction,  
        { "Zone 3 Direction", "gvcp.zone3direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x10000000,
        NULL, HFILL
        }}, 
        
        {& hf_gvsp_sc_zone4_direction,  
        { "Zone 4 Direction", "gvcp.zone4direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x08000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone5_direction,  
        { "Zone 5 Direction", "gvcp.zone5direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x04000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone6_direction,  
        { "Zone 6 Direction", "gvcp.zone6direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x02000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone7_direction,  
        { "Zone 7 Direction", "gvcp.zone7direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x01000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone8_direction,  
        { "Zone 8 Direction", "gvcp.zone8direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00800000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone9_direction,  
        { "Zone 9 Direction", "gvcp.zone9direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00400000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone10_direction, 
        { "Zone 10 Direction", "gvcp.zone10direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00200000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone11_direction, 
        { "Zone 11 Direction", "gvcp.zone1direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00100000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone12_direction, 
        { "Zone 12 Direction", "gvcp.zone12direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00080000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone13_direction, 
        { "Zone 13 Direction", "gvcp.zone13direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00040000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone14_direction, 
        { "Zone 14 Direction", "gvcp.zone14direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00020000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone15_direction, 
        { "Zone 15 Direction", "gvcp.zone15direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00010000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone16_direction, 
        { "Zone 16 Direction", "gvcp.zone16direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00008000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone17_direction, 
        { "Zone 17 Direction", "gvcp.zone17direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00004000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone18_direction, 
        { "Zone 18 Direction", "gvcp.zone18direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00002000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone19_direction, 
        { "Zone 19 Direction", "gvcp.zone19direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00001000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone20_direction, 
        { "Zone 20 Direction", "gvcp.zone20direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000800,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone21_direction, 
        { "Zone 21 Direction", "gvcp.zone21direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000400,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone22_direction, 
        { "Zone 22 Direction", "gvcp.zone22direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000200,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone23_direction, 
        { "Zone 23 Direction", "gvcp.zone23direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000100,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone24_direction, 
        { "Zone 24 Direction", "gvcp.zone24direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000080,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone25_direction, 
        { "Zone 25 Direction", "gvcp.zone25direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000040,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone26_direction, 
        { "Zone 26 Direction", "gvcp.zone26direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000020,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone27_direction, 
        { "Zone 27 Direction", "gvcp.zone27direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000010,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone28_direction, 
        { "Zone 28 Direction", "gvcp.zone28direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000008,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone29_direction, 
        { "Zone 29 Direction", "gvcp.zone29direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000004,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone30_direction, 
        { "Zone 30 Direction", "gvcp.zone30direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000002,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone31_direction, 
        { "Zone 31 Direction", "gvcp.zone31direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000001,
        NULL, HFILL
        }}, 

        {& hf_gvsp_chunkdatapayloadlengthex,            
        { "Chunk Data Payload Length", "gvsp.chunkdatapayloadlengthex",
        FT_UINT32, BASE_HEX_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_chunklayoutidex,         
        { "Chunk Layout ID", "gvsp.chunklayoutidex",
        FT_UINT32, BASE_HEX_DEC, NULL, 0x0,
        NULL, HFILL
        }},
    };

    static gint *ett[] = {
        &ett_gvsp,
		&ett_gvsp_flags,
		&ett_gvsp_header,
		&ett_gvsp_payload,
		&ett_gvsp_trailer,
    };

    proto_gvsp = proto_register_protocol("GigE Vision Streaming Protocol", "GVSP", "gvsp");

    register_dissector("gvsp", dissect_gvsp, proto_gvsp);

    proto_register_field_array(proto_gvsp, hfgvsp, array_length(hfgvsp));
    proto_register_subtree_array(ett, array_length(ett));
}


/*
    \brief Rules for GVSP packets identification (?)
 */

void proto_reg_handoff_gvsp(void) 
{
    static int gvsp_initialized = FALSE;
    static dissector_handle_t gvsp_handle;

    if (!gvsp_initialized) 
	{
        gvsp_handle = new_create_dissector_handle((new_dissector_t)dissect_gvsp, proto_gvsp);
        dissector_add_uint("udp.port", global_gvsp_port, gvsp_handle);
        gvsp_initialized = TRUE;
    } 
	else 
	{
        dissector_delete_uint("udp.port", global_gvsp_port, gvsp_handle);
    }
}

/* packet-gvsp.c
 * Routines for AIA GigE Vision (TM) Streaming Protocol dissection
 * Copyright 2012, AIA <www.visiononline.org> All rights reserved
 *
 * GigE Vision (TM): GigE Vision a standard developed under the sponsorship of the AIA for 
 * the benefit of the machine vision industry. GVSP stands for GigE Vision (TM) Streaming
 * Protocol.
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*#ifdef HAVE_CONFIG_H */
#include "config.h"
/*#endif */

#ifdef PACKAGE
#undef PACKAGE
#endif

#define PACKAGE "gvsp"

#ifdef VERSION
#undef VERSION
#endif

#define VERSION "0.0.1"

#include <epan/packet.h>


/*
  Payload types
 */

#define GVSP_PAYLOAD_IMAGE ( 0x0001 )
#define GVSP_PAYLOAD_RAWDATA (0x0002 )
#define GVSP_PAYLOAD_FILE ( 0x0003 )
#define GVSP_PAYLOAD_CHUNKDATA ( 0x0004 )
#define GVSP_PAYLOAD_EXTENDEDCHUNKDATA ( 0x0005 ) /* Deprecated */
#define GVSP_PAYLOAD_JPEG ( 0x0006 )
#define GVSP_PAYLOAD_JPEG2000 ( 0x0007 )
#define GVSP_PAYLOAD_H264 ( 0x0008 )
#define GVSP_PAYLOAD_MULTIZONEIMAGE ( 0x0009 )
#define GVSP_PAYLOAD_DEVICEPSECIFICSTART ( 0x8000 )


/*
   GVSP packet types
 */

#define GVSP_PACKET_LEADER ( 1 )
#define GVSP_PACKET_TRAILER ( 2 )
#define GVSP_PACKET_PAYLOAD ( 3 )
#define GVSP_PACKET_ALLIN ( 4 )
#define GVSP_PACKET_PAYLOAD_H264 ( 5 )
#define GVSP_PACKET_PAYLOAD_MULTIZONE ( 6 )


/*
   GVSP statuses
 */

#define GEV_STATUS_SUCCESS (0x0000) 
#define GEV_STATUS_PACKET_RESEND (0x0100) 
#define GEV_STATUS_NOT_IMPLEMENTED (0x8001) 
#define GEV_STATUS_INVALID_PARAMETER (0x8002) 
#define GEV_STATUS_INVALID_ADDRESS (0x8003) 
#define GEV_STATUS_WRITE_PROTECT (0x8004) 
#define GEV_STATUS_BAD_ALIGNMENT (0x8005) 
#define GEV_STATUS_ACCESS_DENIED (0x8006) 
#define GEV_STATUS_BUSY (0x8007) 
#define GEV_STATUS_LOCAL_PROBLEM (0x8008)  /* deprecated */
#define GEV_STATUS_MSG_MISMATCH (0x8009) /* deprecated */
#define GEV_STATUS_INVALID_PROTOCOL (0x800A) /* deprecated */
#define GEV_STATUS_NO_MSG (0x800B) /* deprecated */
#define GEV_STATUS_PACKET_UNAVAILABLE (0x800C) 
#define GEV_STATUS_DATA_OVERRUN (0x800D) 
#define GEV_STATUS_INVALID_HEADER (0x800E) 
#define GEV_STATUS_WRONG_CONFIG (0x800F) /* deprecated */
#define GEV_STATUS_PACKET_NOT_YET_AVAILABLE (0x8010) 
#define GEV_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY (0x8011) 
#define GEV_STATUS_PACKET_REMOVED_FROM_MEMORY (0x8012) 
#define GEV_STATUS_ERROR (0x8FFF) 


/*
   Pixel type color
 */

#define GVSP_PIX_MONO             (0x01000000)
#define GVSP_PIX_RGB              (0x02000000)
#define GVSP_PIX_COLOR            (0x02000000)
#define GVSP_PIX_CUSTOM           (0x80000000)
#define GVSP_PIX_COLOR_MASK       (0xFF000000)


/*
   Pixel type size
 */

#define GVSP_PIX_OCCUPY1BIT       (0x00010000)
#define GVSP_PIX_OCCUPY2BIT       (0x00020000)
#define GVSP_PIX_OCCUPY4BIT       (0x00040000)
#define GVSP_PIX_OCCUPY8BIT       (0x00080000)
#define GVSP_PIX_OCCUPY12BIT      (0x000C0000)
#define GVSP_PIX_OCCUPY16BIT      (0x00100000)
#define GVSP_PIX_OCCUPY24BIT      (0x00180000)
#define GVSP_PIX_OCCUPY32BIT      (0x00200000)
#define GVSP_PIX_OCCUPY36BIT      (0x00240000)
#define GVSP_PIX_OCCUPY48BIT      (0x00300000)


/*
   Pxiel type masks, shifts 
 */
#define GVSP_PIX_EFFECTIVE_PIXEL_SIZE_MASK (0x00FF0000)
#define GVSP_PIX_EFFECTIVE_PIXEL_SIZE_SHIFT (16)

#define GVSP_PIX_ID_MASK (0x0000FFFF)


/*
   Pixel types
 */

#define GVSP_PIX_COUNT (0x46)

#define GVSP_PIX_MONO1P                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY1BIT  | 0x0037)
#define GVSP_PIX_MONO2P                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY2BIT  | 0x0038)
#define GVSP_PIX_MONO4P                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY4BIT  | 0x0039)
#define GVSP_PIX_MONO8                   (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0001)
#define GVSP_PIX_MONO8S                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0002)
#define GVSP_PIX_MONO10                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0003)
#define GVSP_PIX_MONO10_PACKED           (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0004)
#define GVSP_PIX_MONO12                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0005)
#define GVSP_PIX_MONO12_PACKED           (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0006)
#define GVSP_PIX_MONO14                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0025)
#define GVSP_PIX_MONO16                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0007)
											    
#define GVSP_PIX_BAYGR8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0008)
#define GVSP_PIX_BAYRG8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0009)
#define GVSP_PIX_BAYGB8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x000A)
#define GVSP_PIX_BAYBG8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x000B)
#define GVSP_PIX_BAYGR10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000C)
#define GVSP_PIX_BAYRG10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000D)
#define GVSP_PIX_BAYGB10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000E)
#define GVSP_PIX_BAYBG10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000F)
#define GVSP_PIX_BAYGR12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0010)
#define GVSP_PIX_BAYRG12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0011)
#define GVSP_PIX_BAYGB12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0012)
#define GVSP_PIX_BAYBG12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0013)
#define GVSP_PIX_BAYGR10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0026)
#define GVSP_PIX_BAYRG10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0027)
#define GVSP_PIX_BAYGB10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0028)
#define GVSP_PIX_BAYBG10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0029)
#define GVSP_PIX_BAYGR12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002A)
#define GVSP_PIX_BAYRG12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002B)
#define GVSP_PIX_BAYGB12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002C)
#define GVSP_PIX_BAYBG12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002D)
#define GVSP_PIX_BAYGR16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x002E)
#define GVSP_PIX_BAYRG16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x002F)
#define GVSP_PIX_BAYGB16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0030)
#define GVSP_PIX_BAYBG16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0031)
								         
#define GVSP_PIX_RGB8                    (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0014)
#define GVSP_PIX_BGR8                    (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0015)
#define GVSP_PIX_RGBA8                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x0016)
#define GVSP_PIX_BGRA8                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x0017)
#define GVSP_PIX_RGB10                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0018)
#define GVSP_PIX_BGR10                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0019)
#define GVSP_PIX_RGB12                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x001A)
#define GVSP_PIX_BGR12                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x001B)
#define GVSP_PIX_RGB16                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0033)
#define GVSP_PIX_RGB10V1_PACKED          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x001C)
#define GVSP_PIX_RGB10P32                (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x001D)
#define GVSP_PIX_RGB12V1_PACKED          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY36BIT | 0x0034)
#define GVSP_PIX_RGB565P                 (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0035)
#define GVSP_PIX_BGR565P                 (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0036)

#define GVSP_PIX_YUV411_8_UYYVYY         (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x001E)
#define GVSP_PIX_YUV422_8_UYVY           (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x001F)
#define GVSP_PIX_YUV422_8                (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0032)
#define GVSP_PIX_YUV8_UYV                (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0020)
#define GVSP_PIX_YCBCR8_CBYCR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x003A)
#define GVSP_PIX_YCBCR422_8              (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x003B)
#define GVSP_PIX_YCBCR422_8_CBYCRY       (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0043)
#define GVSP_PIX_YCBCR422_8_CBYYCRYY     (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x003C)
#define GVSP_PIX_YCBCR601_8_CBYCR        (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x003D)
#define GVSP_PIX_YCBCR601_422_8          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x003E)
#define GVSP_PIX_YCBCR601_422_8_CBYCRY   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0044)
#define GVSP_PIX_YCBCR601_411_8_CBYYCRYY (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x003F)
#define GVSP_PIX_YCBCR709_411_8_CBYCR    (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0040)
#define GVSP_PIX_YCBCR709_422_8          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0041)
#define GVSP_PIX_YCBCR709_422_8_CBYCRY   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0045)
#define GVSP_PIX_YCBCR709_411_8_CBYYCRYY (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x0042)

#define GVSP_PIX_RGB8_PLANAR             (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0021)
#define GVSP_PIX_RGB10_PLANAR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0022)
#define GVSP_PIX_RGB12_PLANAR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0023)
#define GVSP_PIX_RGB16_PLANAR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0024)


typedef struct _display_string 
{
	int display;
	gchar* string_display;

} display_string;


/* Structure to hold GVSP packet information */
typedef struct _gvsp_packet_info
{
	gint chunk;
	guint8 format;
	guint16 payloadtype;
	guint64 blockid;
	guint32 packetid;
	gint enhanced;
	gint flag_resendrangeerror;
	gint flag_previousblockdropped;
	gint flag_packetresend;

	const char *formatstring;
	const char *payloadstring;

} gvsp_packet_info;


/*Define the gvsp proto */
static int proto_gvsp = -1;
static int global_gvsp_port = 20202;


/* Define the tree for gvsp */
static int ett_gvsp = -1;
static int ett_gvsp_flags = -1;
static int ett_gvsp_header = -1;
static int ett_gvsp_payload = -1;
static int ett_gvsp_trailer = -1;


typedef struct _ftenum_string {
	enum ftenum type;
	gchar* string_type;
} ftenum_string;

static const value_string statusnames[] = {
    { GEV_STATUS_SUCCESS, "GEV_STATUS_SUCCESS" },
    { GEV_STATUS_PACKET_RESEND, "GEV_STATUS_PACKET_RESEND" },
    { GEV_STATUS_NOT_IMPLEMENTED, "GEV_STATUS_NOT_IMPLEMENTED" },
    { GEV_STATUS_INVALID_PARAMETER, "GEV_STATUS_INVALID_PARAMETER" },
    { GEV_STATUS_INVALID_ADDRESS, "GEV_STATUS_INVALID_ADDRESS" },
    { GEV_STATUS_WRITE_PROTECT, "GEV_STATUS_WRITE_PROTECT" },
    { GEV_STATUS_BAD_ALIGNMENT, "GEV_STATUS_BAD_ALIGNMENT" },
    { GEV_STATUS_ACCESS_DENIED, "GEV_STATUS_ACCESS_DENIED" },
    { GEV_STATUS_BUSY, "GEV_STATUS_BUSY" },
    { GEV_STATUS_LOCAL_PROBLEM, "GEV_STATUS_LOCAL_PROBLEM (deprecated)" },
    { GEV_STATUS_MSG_MISMATCH, "GEV_STATUS_MSG_MISMATCH (deprecated)" },
    { GEV_STATUS_INVALID_PROTOCOL, "GEV_STATUS_INVALID_PROTOCOL (deprecated)" },
    { GEV_STATUS_NO_MSG, "GEV_STATUS_NO_MSG (deprecated)" },
    { GEV_STATUS_PACKET_UNAVAILABLE, "GEV_STATUS_PACKET_UNAVAILABLE" },
    { GEV_STATUS_DATA_OVERRUN, "GEV_STATUS_DATA_OVERRUN" },
    { GEV_STATUS_INVALID_HEADER, "GEV_STATUS_INVALID_HEADER" },
    { GEV_STATUS_WRONG_CONFIG, "GEV_STATUS_WRONG_CONFIG (deprecated)" },
    { GEV_STATUS_PACKET_NOT_YET_AVAILABLE, "GEV_STATUS_PACKET_NOT_YET_AVAILABLE" },
    { GEV_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY, "GEV_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY" },
    { GEV_STATUS_PACKET_REMOVED_FROM_MEMORY, "GEV_STATUS_PACKET_REMOVED_FROM_MEMORY" },
    { GEV_STATUS_ERROR, "GEV_STATUS_ERROR" },
    { 0, NULL },
};

static const value_string formatnames[] = {
    { GVSP_PACKET_LEADER, "LEADER" },
    { GVSP_PACKET_TRAILER, "TRAILER" },
    { GVSP_PACKET_PAYLOAD, "PAYLOAD" },
    { GVSP_PACKET_ALLIN, "ALLIN" },
    { GVSP_PACKET_PAYLOAD_H264, "H264" },
    { GVSP_PACKET_PAYLOAD_MULTIZONE, "MULTIZONE" },
    { 0x80 | GVSP_PACKET_LEADER, "LEADER (ext IDs)" },
    { 0x80 | GVSP_PACKET_TRAILER, "TRAILER (ext IDs)" },
    { 0x80 | GVSP_PACKET_PAYLOAD, "PAYLOAD (ext IDs)" },
    { 0x80 | GVSP_PACKET_ALLIN, "ALLIN (ext IDs)" },
    { 0x80 | GVSP_PACKET_PAYLOAD_H264, "H264 (ext IDs)" },
    { 0x80 | GVSP_PACKET_PAYLOAD_MULTIZONE, "MULTIZONE (ext IDs)" },
    { 0, NULL },
};

static const value_string payloadtypenames[] = {
    { GVSP_PAYLOAD_IMAGE, "IMAGE" },
    { GVSP_PAYLOAD_RAWDATA, "RAWDATA" },
    { GVSP_PAYLOAD_FILE, "FILE" },
    { GVSP_PAYLOAD_CHUNKDATA, "CHUNKDATA" },
    { GVSP_PAYLOAD_EXTENDEDCHUNKDATA, "EXTENDEDCHUNKDATA (obsolete with v2.0)" },
    { GVSP_PAYLOAD_JPEG, "JPEG" },
    { GVSP_PAYLOAD_JPEG2000, "JPEG2000" },
    { GVSP_PAYLOAD_H264, "H264" },
    { GVSP_PAYLOAD_MULTIZONEIMAGE, "MUTLIZONEIAMGE" },
    { 0x4000 | GVSP_PAYLOAD_IMAGE, "IMAGE (v2.0 chunks)" },
    { 0x4000 | GVSP_PAYLOAD_RAWDATA, "RAWDATA (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_FILE, "FILE (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_CHUNKDATA, "CHUNKDATA (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_EXTENDEDCHUNKDATA, "EXTENDEDCHUNKDATA (v2.0 chunks?)" },
    { 0x4000 | GVSP_PAYLOAD_JPEG, "JPEG (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_JPEG2000, "JPEG2000 (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_H264, "H264 (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_MULTIZONEIMAGE, "MULTIZONEIMAGE (v2.0 Chunks)" },
    { 0, NULL },
};

static const value_string pixeltypenames[] = {
	{ GVSP_PIX_MONO1P, "GVSP_PIX_MONO1P" },                  
    { GVSP_PIX_MONO2P, "GVSP_PIX_MONO2P" },                  
    { GVSP_PIX_MONO4P, "GVSP_PIX_MONO4P" },
    { GVSP_PIX_MONO8, "GVSP_PIX_MONO8" },                   
    { GVSP_PIX_MONO8S, "GVSP_PIX_MONO8S" },                  
    { GVSP_PIX_MONO10, "GVSP_PIX_MONO10" },                  
    { GVSP_PIX_MONO10_PACKED, "GVSP_PIX_MONO10_PACKED" },           
    { GVSP_PIX_MONO12, "GVSP_PIX_MONO12" },                  
    { GVSP_PIX_MONO12_PACKED, "GVSP_PIX_MONO12_PACKED" },           
    { GVSP_PIX_MONO14, "GVSP_PIX_MONO14" },                  
    { GVSP_PIX_MONO16, "GVSP_PIX_MONO16" },                  
    { GVSP_PIX_BAYGR8, "GVSP_PIX_BAYGR8" },                  
    { GVSP_PIX_BAYRG8, "GVSP_PIX_BAYRG8" },                  
    { GVSP_PIX_BAYGB8, "GVSP_PIX_BAYGB8" },                  
    { GVSP_PIX_BAYBG8, "GVSP_PIX_BAYBG8" },                  
    { GVSP_PIX_BAYGR10, "GVSP_PIX_BAYGR10" },                 
    { GVSP_PIX_BAYRG10, "GVSP_PIX_BAYRG10" },                 
    { GVSP_PIX_BAYGB10, "GVSP_PIX_BAYGB10" },                 
    { GVSP_PIX_BAYBG10, "GVSP_PIX_BAYBG10" },                 
    { GVSP_PIX_BAYGR12, "GVSP_PIX_BAYGR12" },                 
    { GVSP_PIX_BAYRG12, "GVSP_PIX_BAYRG12" },                 
    { GVSP_PIX_BAYGB12, "GVSP_PIX_BAYGB12" },                 
    { GVSP_PIX_BAYBG12, "GVSP_PIX_BAYBG12" },                 
    { GVSP_PIX_BAYGR10_PACKED, "GVSP_PIX_BAYGR10_PACKED" },          
    { GVSP_PIX_BAYRG10_PACKED, "GVSP_PIX_BAYRG10_PACKED" },          
    { GVSP_PIX_BAYGB10_PACKED, "GVSP_PIX_BAYGB10_PACKED" },          
    { GVSP_PIX_BAYBG10_PACKED, "GVSP_PIX_BAYBG10_PACKED" },          
    { GVSP_PIX_BAYGR12_PACKED, "GVSP_PIX_BAYGR12_PACKED" },          
    { GVSP_PIX_BAYRG12_PACKED, "GVSP_PIX_BAYRG12_PACKED" },          
    { GVSP_PIX_BAYGB12_PACKED, "GVSP_PIX_BAYGB12_PACKED" },          
    { GVSP_PIX_BAYBG12_PACKED, "GVSP_PIX_BAYBG12_PACKED" },          
    { GVSP_PIX_BAYGR16, "GVSP_PIX_BAYGR16" },                 
    { GVSP_PIX_BAYRG16, "GVSP_PIX_BAYRG16" },                 
    { GVSP_PIX_BAYGB16, "GVSP_PIX_BAYGB16" },                 
    { GVSP_PIX_BAYBG16, "GVSP_PIX_BAYBG16" },                 
    { GVSP_PIX_RGB8, "GVSP_PIX_RGB8" },                    
    { GVSP_PIX_BGR8, "GVSP_PIX_BGR8" },                    
    { GVSP_PIX_RGBA8, "GVSP_PIX_RGBA8" },                   
    { GVSP_PIX_BGRA8, "GVSP_PIX_BGRA8" },                   
    { GVSP_PIX_RGB10, "GVSP_PIX_RGB10" },                   
    { GVSP_PIX_BGR10, "GVSP_PIX_BGR10" },                   
    { GVSP_PIX_RGB12, "GVSP_PIX_RGB12" },                   
    { GVSP_PIX_BGR12, "GVSP_PIX_BGR12" },                   
    { GVSP_PIX_RGB16, "GVSP_PIX_RGB16" },                   
    { GVSP_PIX_RGB10V1_PACKED, "GVSP_PIX_RGB10V1_PACKED" },          
    { GVSP_PIX_RGB10P32, "GVSP_PIX_RGB10P32" },                
    { GVSP_PIX_RGB12V1_PACKED, "GVSP_PIX_RGB12V1_PACKED" },          
    { GVSP_PIX_RGB565P, "GVSP_PIX_RGB565P" },                 
    { GVSP_PIX_BGR565P, "GVSP_PIX_BGR565P" },                 
    { GVSP_PIX_YUV411_8_UYYVYY, "GVSP_PIX_YUV411_8_UYYVYY" },         
    { GVSP_PIX_YUV422_8_UYVY, "GVSP_PIX_YUV422_8_UYVY" },           
    { GVSP_PIX_YUV422_8, "GVSP_PIX_YUV422_8" },                
    { GVSP_PIX_YUV8_UYV, "GVSP_PIX_YUV8_UYV" },                
    { GVSP_PIX_YCBCR8_CBYCR, "GVSP_PIX_YCBCR8_CBYCR" },            
    { GVSP_PIX_YCBCR422_8, "GVSP_PIX_YCBCR422_8" },              
    { GVSP_PIX_YCBCR422_8_CBYCRY, "GVSP_PIX_YCBCR422_8_CBYCRY" },       
    { GVSP_PIX_YCBCR422_8_CBYYCRYY, "GVSP_PIX_YCBCR422_8_CBYYCRYY" },     
    { GVSP_PIX_YCBCR601_8_CBYCR, "GVSP_PIX_YCBCR601_8_CBYCR" },        
    { GVSP_PIX_YCBCR601_422_8, "GVSP_PIX_YCBCR601_422_8" },          
    { GVSP_PIX_YCBCR601_422_8_CBYCRY, "GVSP_PIX_YCBCR601_422_8_CBYCRY" },   
    { GVSP_PIX_YCBCR601_411_8_CBYYCRYY, "GVSP_PIX_YCBCR601_411_8_CBYYCRYY" }, 
    { GVSP_PIX_YCBCR709_411_8_CBYCR, "GVSP_PIX_YCBCR709_411_8_CBYCR" },    
    { GVSP_PIX_YCBCR709_422_8, "GVSP_PIX_YCBCR709_422_8" },         
    { GVSP_PIX_YCBCR709_422_8, "GVSP_PIX_YCBCR709_422_8" },          
    { GVSP_PIX_YCBCR709_411_8_CBYYCRYY, "GVSP_PIX_YCBCR709_411_8_CBYYCRYY" }, 
    { GVSP_PIX_RGB8_PLANAR, "GVSP_PIX_RGB8_PLANAR" },             
    { GVSP_PIX_RGB10_PLANAR, "GVSP_PIX_RGB10_PLANAR" },            
    { GVSP_PIX_RGB12_PLANAR, "GVSP_PIX_RGB12_PLANAR" },            
    { GVSP_PIX_RGB16_PLANAR, "GVSP_PIX_RGB16_PLANAR" },            
    { 0, NULL },
};

static const value_string colornames[] = {
    { GVSP_PIX_MONO >> 24, "Mono" },
    { GVSP_PIX_COLOR >> 24, "Color" },
    { GVSP_PIX_CUSTOM >> 24, "Custom" },
    { 0, NULL },
};

static const true_false_string directionnames = {
	"Receiver",
	"Transmitter"
};


static int hf_gvsp_status = -1;
static int hf_gvsp_blockid16 = -1;
static int hf_gvsp_flags = -1;
static int hf_gvsp_flagdevicespecific0 = -1;
static int hf_gvsp_flagdevicespecific1 = -1;
static int hf_gvsp_flagdevicespecific2 = -1;
static int hf_gvsp_flagdevicespecific3 = -1;
static int hf_gvsp_flagdevicespecific4 = -1;
static int hf_gvsp_flagdevicespecific5 = -1;
static int hf_gvsp_flagdevicespecific6 = -1;
static int hf_gvsp_flagdevicespecific7 = -1;
static int hf_gvsp_flagresendrangeerror = -1;
static int hf_gvsp_flagpreviousblockdropped = -1;
static int hf_gvsp_flagpacketresend = -1;
static int hf_gvsp_format = -1;
static int hf_gvsp_packetid24 = -1;
static int hf_gvsp_blockid64 = -1;
static int hf_gvsp_packetid32 = -1;
static int hf_gvsp_payloadtype = -1;
static int hf_gvsp_payloaddata = -1;
static int hf_gvsp_timestamp = -1;
static int hf_gvsp_pixelformat = -1;
static int hf_gvsp_sizex = -1;
static int hf_gvsp_sizey = -1;
static int hf_gvsp_offsetx = -1;
static int hf_gvsp_offsety = -1;
static int hf_gvsp_paddingx = -1;
static int hf_gvsp_paddingy = -1;
static int hf_gvsp_payloaddatasize = -1;
static int hf_gvsp_pixelcolor = -1;
static int hf_gvsp_pixeloccupy = -1;
static int hf_gvsp_pixelid = -1;
static int hf_gvsp_filename = -1;
static int hf_gvsp_payloadlength = -1;
static int hf_gvsp_fieldinfo = -1;
static int hf_gvsp_fieldid = -1;
static int hf_gvsp_fieldcount = -1;
static int hf_gvsp_genericflags = -1;
static int hf_gvsp_chunklayoutid = -1;
static int hf_gvsp_timestamptickfrequency = -1;
static int hf_gvsp_dataformat = -1;
static int hf_gvsp_packetizationmode = -1;
static int hf_gvsp_packetsize = -1;
static int hf_gvsp_profileidc = -1;
static int hf_gvsp_cs = -1;
static int hf_gvsp_cs0 = -1;
static int hf_gvsp_cs1 = -1;
static int hf_gvsp_cs2 = -1;
static int hf_gvsp_cs3 = -1;
static int hf_gvsp_levelidc = -1;
static int hf_gvsp_sropinterleavingdepth = -1;
static int hf_gvsp_sropmaxdondiff = -1;
static int hf_gvsp_sropdeintbufreq = -1;
static int hf_gvsp_sropinitbuftime = -1;
static int hf_gvsp_zoneinfo = -1;
static int hf_gvsp_zoneid = -1;
static int hf_gvsp_endofzone = -1;
static int hf_gvsp_addressoffsethigh = -1;
static int hf_gvsp_addressoffsetlow = -1;
static int hf_gvsp_sc_zone_direction = -1;
static int hf_gvsp_sc_zone0_direction = -1;
static int hf_gvsp_sc_zone1_direction = -1;
static int hf_gvsp_sc_zone2_direction = -1;
static int hf_gvsp_sc_zone3_direction = -1;
static int hf_gvsp_sc_zone4_direction = -1;
static int hf_gvsp_sc_zone5_direction = -1;
static int hf_gvsp_sc_zone6_direction = -1;
static int hf_gvsp_sc_zone7_direction = -1;
static int hf_gvsp_sc_zone8_direction = -1;
static int hf_gvsp_sc_zone9_direction = -1;
static int hf_gvsp_sc_zone10_direction = -1;
static int hf_gvsp_sc_zone11_direction = -1;
static int hf_gvsp_sc_zone12_direction = -1;
static int hf_gvsp_sc_zone13_direction = -1;
static int hf_gvsp_sc_zone14_direction = -1;
static int hf_gvsp_sc_zone15_direction = -1;
static int hf_gvsp_sc_zone16_direction = -1;
static int hf_gvsp_sc_zone17_direction = -1;
static int hf_gvsp_sc_zone18_direction = -1;
static int hf_gvsp_sc_zone19_direction = -1;
static int hf_gvsp_sc_zone20_direction = -1;
static int hf_gvsp_sc_zone21_direction = -1;
static int hf_gvsp_sc_zone22_direction = -1;
static int hf_gvsp_sc_zone23_direction = -1;
static int hf_gvsp_sc_zone24_direction = -1;
static int hf_gvsp_sc_zone25_direction = -1;
static int hf_gvsp_sc_zone26_direction = -1;
static int hf_gvsp_sc_zone27_direction = -1;
static int hf_gvsp_sc_zone28_direction = -1;
static int hf_gvsp_sc_zone29_direction = -1;
static int hf_gvsp_sc_zone30_direction = -1;
static int hf_gvsp_sc_zone31_direction = -1;
static int hf_gvsp_chunkdatapayloadlengthex = -1;
static int hf_gvsp_chunklayoutidex = -1;


/*
    \brief Dissects the image leader
 */

gint DissectImageLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{	
	proto_item *item = NULL;
	proto_item *ti = NULL;
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Pixel format */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_pixelformat, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_pixelcolor, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixeloccupy, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixelid, tvb, offset + 12, 4, ENC_BIG_ENDIAN);

		/* Size X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizex, tvb, offset + 16, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 20, 4, ENC_BIG_ENDIAN);

		/* Offset X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsetx, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* Offset Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsety, tvb, offset + 28, 4, ENC_BIG_ENDIAN);

		/* Padding X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingx, tvb, offset + 32, 2, ENC_BIG_ENDIAN);

		/* Padding Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingy, tvb, offset + 34, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 36;
}


/*
    \brief Dissects the image trailer
 */

gint DissectImageTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{	
	gint offset;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 8;
}


/*
    \brief Dissects the raw data leader
 */

gint DissectRawDataLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 12, 8, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 20;
}


/*
    \brief Dissects a file leader
 */

gint DissectFileLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 12, 8, ENC_BIG_ENDIAN);

		/* Filename */
		proto_tree_add_item(gvsp_tree, hf_gvsp_filename, tvb, offset + 20, -1, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 20; /* Problem with the spec? All-in does not work if variable header size... */
}


/*
    \brief Dissects a chunk data leader
 */

gint DissectChunkDataLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 12;
}


/*
    \brief Dissects a chunk data trailer
 */

gint DissectChunkDataTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Payload data length */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadlength, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 8;
}


/*
    \brief Dissects extended chunk data leader
 */

gint DissectExtendedChunkDataLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;	
    proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Generic flags */
		proto_tree_add_item(gvsp_tree, hf_gvsp_genericflags, tvb, offset + 1, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Pixel format */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_pixelformat, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_pixelcolor, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixeloccupy, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixelid, tvb, offset + 12, 4, ENC_BIG_ENDIAN);

		/* Size X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizex, tvb, offset + 16, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 20, 4, ENC_BIG_ENDIAN);

		/* Offset X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsetx, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* Offset Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsety, tvb, offset + 28, 4, ENC_BIG_ENDIAN);

		/* Padding X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingx, tvb, offset + 32, 2, ENC_BIG_ENDIAN);

		/* Padding Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingy, tvb, offset + 34, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 36;
}


/*
    \brief Dissects extended chunk data trailer
 */

gint DissectExtendedChunkDataTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Payload data length */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadlength, tvb, offset + 4, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 8, 4, ENC_BIG_ENDIAN);

		/* Chunk layout ID */
		proto_tree_add_item(gvsp_tree, hf_gvsp_chunklayoutid, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 16;
}


/*
    \brief Dissects a JPEG leader
 */

gint DissectJPEGLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Generic flags */
		proto_tree_add_item(gvsp_tree, hf_gvsp_genericflags, tvb, offset + 1, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 12, 8, ENC_BIG_ENDIAN);

		/* Timestamp tick frequency */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamptickfrequency, tvb, offset + 20, 8, ENC_BIG_ENDIAN);

		/* Data format */
		proto_tree_add_item(gvsp_tree, hf_gvsp_dataformat, tvb, offset + 24, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 28;
}


/*
 \brief Dissects a H264 leader
 */

gint DissectH264Leader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* packetization_mode */
		proto_tree_add_item(gvsp_tree, hf_gvsp_packetizationmode, tvb, offset + 13, 1, ENC_BIG_ENDIAN);

		/* packet_size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_packetsize, tvb, offset + 14, 2, ENC_BIG_ENDIAN);

		/* profile_idc */
		proto_tree_add_item(gvsp_tree, hf_gvsp_profileidc, tvb, offset + 17, 1, ENC_BIG_ENDIAN);

		/* cs0, 1, 2 ,3 */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_cs, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs0, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs1, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs2, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs3, tvb, offset + 18, 1, ENC_BIG_ENDIAN);

		/* level_idc */
		proto_tree_add_item(gvsp_tree, hf_gvsp_levelidc, tvb, offset + 19, 1, ENC_BIG_ENDIAN);

		/* srop_interleaving_depth */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropinterleavingdepth, tvb, offset + 20, 2, ENC_BIG_ENDIAN);

		/* srop_max_don_diff */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropmaxdondiff, tvb, offset + 22, 2, ENC_BIG_ENDIAN);

		/* srop_deint_buf_req */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropdeintbufreq, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* srop_init_buf_time */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropinitbuftime, tvb, offset + 28, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 32;
}


/*
    \brief Dissects the multizone image leader
 */

gint DissectMultizoneImageLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
    proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Zone direction */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_sc_zone_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_sc_zone0_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone1_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone2_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone3_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone4_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone5_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone6_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone7_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone8_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone9_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone10_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone11_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone12_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone13_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone14_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone15_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone16_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone17_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone18_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone19_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone20_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone21_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone22_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone23_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone24_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone25_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone26_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone27_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone28_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone29_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone30_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone31_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);

		/* Pixel format */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_pixelformat, tvb, offset + 16, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_pixelcolor, tvb, offset + 16, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixeloccupy, tvb, offset + 16, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixelid, tvb, offset + 16, 4, ENC_BIG_ENDIAN);

		/* Size X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizex, tvb, offset + 20, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* Offset X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsetx, tvb, offset + 28, 4, ENC_BIG_ENDIAN);

		/* Offset Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsety, tvb, offset + 32, 4, ENC_BIG_ENDIAN);

		/* Padding X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingx, tvb, offset + 36, 2, ENC_BIG_ENDIAN);

		/* Padding Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingy, tvb, offset + 38, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 40;
}


/*
    \brief Dissects a generic trailer (contains just the payload type)
 */

gint DissectGenericTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 4;
}


/*
    \brief Dissects a generic trailer (contains just the payload type)
 */

gint DissectExtraChunkInfo(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Chunk data payload length */
		proto_tree_add_item(gvsp_tree, hf_gvsp_chunkdatapayloadlengthex, tvb, offset, 4, ENC_BIG_ENDIAN);

		/* Chunk layoud id */
		proto_tree_add_item(gvsp_tree, hf_gvsp_chunklayoutidex, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 8;
}


/*
    \brief Dissects a packet payload
 */

void DissectPacketPayload(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddata, tvb, offset, -1, ENC_BIG_ENDIAN);
	}
}


/*
    \brief Dissects a packet payload for H264
 */

void DissectPacketPayloadH264(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 0, 8, ENC_BIG_ENDIAN);

		/* Data */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddata, tvb, offset + 8, -1, ENC_BIG_ENDIAN);
	}
}


/*
    \brief Dissects a packet payload for multizone
 */

void DissectPacketPayloadMultizone(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Zone information */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_zoneinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_zoneid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_endofzone, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Address offset high */
		proto_tree_add_item(gvsp_tree, hf_gvsp_addressoffsethigh, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Address offset low */
		proto_tree_add_item(gvsp_tree, hf_gvsp_addressoffsetlow, tvb, offset + 4, 4, ENC_BIG_ENDIAN);

		/* Data */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddata, tvb, offset + 8, -1, ENC_BIG_ENDIAN);
	}
}


/*
    \brief Dissects an all in packet
 */

void DissectPacketAllIn(proto_tree *gvsp_tree, tvbuff_t *tvb, gint startoffset, packet_info *pinfo, gvsp_packet_info *info)
{
	gint offset;
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		switch (info->payloadtype)
		{
		case GVSP_PAYLOAD_IMAGE:
			offset += DissectImageLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectImageTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_RAWDATA:
			offset += DissectRawDataLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_FILE:
			offset += DissectFileLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_CHUNKDATA:
			offset += DissectChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_EXTENDEDCHUNKDATA:
			offset += DissectExtendedChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectExtendedChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_JPEG:
		case GVSP_PAYLOAD_JPEG2000:
			offset += DissectJPEGLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_H264:
			offset += DissectH264Leader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayloadH264(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_MULTIZONEIMAGE:
			offset += DissectMultizoneImageLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectImageTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayloadMultizone(gvsp_tree, tvb, pinfo, offset);
			break;
		}
	}
}


/*
    \brief Dissects a leader packet
 */

void DissectPacketLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, gint startoffset, packet_info *pinfo, gvsp_packet_info *info)
{
	gint offset;
	offset = startoffset;

	switch (info->payloadtype)
	{
	case GVSP_PAYLOAD_IMAGE:
		DissectImageLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_RAWDATA:
		DissectRawDataLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_FILE:
		DissectFileLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_CHUNKDATA:
		DissectChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_EXTENDEDCHUNKDATA:
		DissectExtendedChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_JPEG:
	case GVSP_PAYLOAD_JPEG2000:
		DissectJPEGLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_H264:
		DissectH264Leader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_MULTIZONEIMAGE:
		DissectMultizoneImageLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	default:
		break;
	}
}


/*
    \brief Dissects a trailer packet
 */

void DissectPacketTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, gint startoffset, packet_info *pinfo, gvsp_packet_info *info)
{
	gint offset;
	offset = startoffset;

	switch (info->payloadtype)
	{
	case GVSP_PAYLOAD_IMAGE:
	case GVSP_PAYLOAD_MULTIZONEIMAGE:
		offset += DissectImageTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_CHUNKDATA:
		offset += DissectChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_EXTENDEDCHUNKDATA:
		offset += DissectExtendedChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_RAWDATA:
	case GVSP_PAYLOAD_FILE:
	case GVSP_PAYLOAD_JPEG:
	case GVSP_PAYLOAD_JPEG2000:
	case GVSP_PAYLOAD_H264:
		offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	default:
		break;
	}

	if (info->chunk != 0)
	{
		offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
	}
}


/*
    \brief Point of entry of all GVSP dissection
 */

static void dissect_gvsp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) 
{
    proto_item *ti = NULL;
    gint offset = 0;
	proto_tree *gvsp_tree = NULL;
	proto_item *item = NULL;
	gvsp_packet_info info;
	guint8 firstbyte = 0;

	memset(&info, 0x00, sizeof(info));

	/* Set the protocol column */
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "GVSP");

	/* Clear out stuff in the info column */
	col_clear(pinfo->cinfo, COL_INFO);

	/* Adds "Gigabit-Ethernet Streaming Protocol" heading to protocol tree */
	/* We will add fields to this using the gvsp_tree pointer */
	if (tree != NULL)
	{
		ti = proto_tree_add_item(tree, proto_gvsp, tvb, offset, -1, ENC_BIG_ENDIAN);
		gvsp_tree = proto_item_add_subtree(ti, ett_gvsp);
	}

	firstbyte = tvb_get_guint8(tvb, offset);
	if (firstbyte == 0x42)
	{
		/* Add packet format to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "GVCP Generic Packet (Firewall)");

		/* GVCP magic byte, must be a firewall opener packet */
		return;
	}

	/* Get packet format byte, look for enhanced flag and then clear it */
	info.format = tvb_get_guint8(tvb, offset + 4);
	info.formatstring = val_to_str(info.format, formatnames, "Unknown Format");
	info.enhanced = info.format & 0x80;
	info.format &= 0x7F;

	/* Add packet format to info string */
	col_append_fstr(pinfo->cinfo, COL_INFO, "%s ", info.formatstring);

	/* Dissect status */
	if (tree != NULL)
	{
		proto_tree_add_item(gvsp_tree, hf_gvsp_status, tvb, offset, 2, ENC_BIG_ENDIAN);
	}
	offset += 2;

	if (info.enhanced == 0)
	{
		info.blockid = tvb_get_ntohs(tvb, offset);

		/* Dissect block id 16 bits */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_blockid16, tvb, offset, 2, ENC_BIG_ENDIAN);
		}
		offset += 2;
	}
	else
	{
		/* Dissect flags */
		if (tree != NULL)
		{
			guint8 flags;
			flags = tvb_get_guint8(tvb, offset + 1);
			info.flag_resendrangeerror = flags & 0x04;
			info.flag_previousblockdropped = flags & 0x02;
			info.flag_packetresend = flags & 0x01;

			item = proto_tree_add_item(gvsp_tree, hf_gvsp_flags, tvb, offset, 2, ENC_BIG_ENDIAN);
			ti = proto_item_add_subtree(item, ett_gvsp_flags);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific0, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific1, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific2, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific3, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific4, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific5, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific6, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific7, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagresendrangeerror, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagpreviousblockdropped, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagpacketresend, tvb, offset, 2, ENC_BIG_ENDIAN);
		}
		offset += 2;
	}

	/* Dissect packet format */
	if (tree != NULL)
	{
		proto_tree_add_item(gvsp_tree, hf_gvsp_format, tvb, offset, 1, ENC_BIG_ENDIAN);
	}
	offset++;

	if (info.enhanced == 0)
	{
		info.packetid = tvb_get_ntohl(tvb, offset - 1);
		info.packetid &= 0x00FFFFFF;

		/* Dissect packet ID */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_packetid24, tvb, offset, 3, ENC_BIG_ENDIAN);
		}
		offset += 3;
	}
	else
	{
		/* Nothing... */
		offset += 3;
	}

	if (info.enhanced != 0)
	{
		guint64 high;
		guint64 low;
		high = tvb_get_ntohl(tvb, offset);
		low = tvb_get_ntohl(tvb, offset + 4);
		info.blockid = low | (high << 32);

		/* Dissect 64 bit block ID */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_blockid64, tvb, offset, 8, ENC_BIG_ENDIAN);
		}
		offset += 8;

		info.packetid = tvb_get_ntohl(tvb, offset);

		/* Dissect 32 bit packet ID */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_packetid32, tvb, offset, 4, ENC_BIG_ENDIAN);
		}
		offset += 4;
	}

	col_append_fstr(pinfo->cinfo, COL_INFO, "[Block ID: %" G_GINT64_MODIFIER "u Packet ID: %d] ", (guint64)info.blockid, info.packetid);

	if (info.flag_resendrangeerror != 0)
	{
		/* Add range error to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "[RANGE_ERROR] ");
	}

	if (info.flag_previousblockdropped != 0)
	{
		/* Add block dropped to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "[BLOCK_DROPPED] ");
	}

	if (info.flag_packetresend != 0)
	{
		/* Add packet resend to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "[PACKET_RESEND] ");
	}

	/* Process packet types that are payload agnostic */
	switch (info.format)
	{
	case GVSP_PACKET_PAYLOAD:
		DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
		return;

	case GVSP_PACKET_PAYLOAD_H264:
		DissectPacketPayloadH264(gvsp_tree, tvb, pinfo, offset);
		return;

	case GVSP_PACKET_PAYLOAD_MULTIZONE:
		DissectPacketPayloadMultizone(gvsp_tree, tvb, pinfo, offset);
		return;

	default:
		break;
	}

	/* Get payload type, clear chunk bit */
	info.payloadtype = tvb_get_ntohs(tvb, offset + 2);
	info.payloadstring = val_to_str(info.payloadtype, payloadtypenames, "Unknown Payload Type");
	info.chunk = info.payloadtype & 0x4000;
	info.payloadtype &= 0x3FFF;

	/* Add payload type to information string */
	col_append_fstr(pinfo->cinfo, COL_INFO, "%s ", info.payloadstring);

	/* Process packet types for specific payload types */
	switch (info.format)
	{
	case GVSP_PACKET_ALLIN:
		DissectPacketAllIn(gvsp_tree, tvb, offset, pinfo, &info);
		return;

	case GVSP_PACKET_LEADER:
		DissectPacketLeader(gvsp_tree, tvb, offset, pinfo, &info);
		return;

	case GVSP_PACKET_TRAILER:
		DissectPacketTrailer(gvsp_tree, tvb, offset, pinfo, &info);
		return;

	default:
		break;
	}
}


/*
    \brief Registers the dissector. Invoked when the pluging library is loaded.
 */

void proto_register_gvsp(void) 
{
    static hf_register_info hfgvsp[] = 
    {
        {& hf_gvsp_status,          
        { "Status", "gvsp.status",
        FT_UINT16, BASE_HEX, VALS(statusnames), 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_blockid16,           
        { "Block ID (16 bits)", "gvsp.blockid16",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_flags,           
        { "Flags", "gvsp.flags",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific0,         
        { "Flag Device Specific 0", "gvsp.flag.devicespecific0",
        FT_UINT16, BASE_HEX, NULL, 0x8000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific1,         
        { "Flag Device Specific 1", "gvsp.flag.devicespecific1",
        FT_UINT16, BASE_HEX, NULL, 0x4000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific2,         
        { "Flag Device Specific 2", "gvsp.flag.devicespecific2",
        FT_UINT16, BASE_HEX, NULL, 0x2000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific3,         
        { "Flag Device Specific 3", "gvsp.flag.devicespecific3",
        FT_UINT16, BASE_HEX, NULL, 0x1000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific4,         
        { "Flag Device Specific 4", "gvsp.flag.devicespecific4",
        FT_UINT16, BASE_HEX, NULL, 0x0800,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific5,         
        { "Flag Device Specific 5", "gvsp.flag.devicespecific5",
        FT_UINT16, BASE_HEX, NULL, 0x0400,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific6,         
        { "Flag Device Specific 6", "gvsp.flag.devicespecific6",
        FT_UINT16, BASE_HEX, NULL, 0x0200,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific7,         
        { "Flag Device Specific 7", "gvsp.flag.devicespecific7",
        FT_UINT16, BASE_HEX, NULL, 0x0100,
        NULL, HFILL
        }},

        {& hf_gvsp_flagresendrangeerror,            
        { "Flag Resend Range Error 7", "gvsp.flag.resendrangeerror",
        FT_UINT16, BASE_HEX, NULL, 0x0004,
        NULL, HFILL
        }},

        {& hf_gvsp_flagpreviousblockdropped,            
        { "Flag Previous Block Dropped", "gvsp.flag.previousblockdropped",
        FT_UINT16, BASE_HEX, NULL, 0x0002,
        NULL, HFILL
        }},

        {& hf_gvsp_flagpacketresend,            
        { "Flag Packet Resend", "gvsp.flag.packetresend",
        FT_UINT16, BASE_HEX, NULL, 0x0001,
        NULL, HFILL
        }},

        {& hf_gvsp_format,          
        { "Format", "gvsp.format",
        FT_UINT8, BASE_HEX, VALS(formatnames), 0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetid24,          
        { "Packet ID (24 bits)", "gvsp.packetid24",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_blockid64,           
        { "Block ID (64 bits v2.0)", "gvsp.blockid64",
        FT_UINT64, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetid32,          
        { "Packet ID (32 bits v2.0)", "gvsp.packetid32",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_payloadtype,         
        { "Payload Type", "gvsp.payloadtype",
        FT_UINT16, BASE_HEX, VALS(payloadtypenames), 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_payloaddata,         
        { "Payload Data", "gvsp.payloaddata",
        FT_BYTES, BASE_NONE, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_timestamp,           
        { "Timestamp", "gvsp.timestamp",
        FT_UINT64, BASE_HEX, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_pixelformat,         
        { "Pixel Format", "gvsp.pixel",
        FT_UINT32, BASE_HEX, VALS(pixeltypenames), 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_sizex,           
        { "Size X", "gvsp.sizex",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_sizey,           
        { "Size Y", "gvsp.sizey",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_offsetx,         
        { "Offset X", "gvsp.offsetx",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_offsety,         
        { "Offset Y", "gvsp.offsety",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_paddingx,            
        { "Padding X", "gvsp.paddingx",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_paddingy,            
        { "Padding Y", "gvsp.paddingy",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_payloaddatasize,         
        { "Payload Data Size", "gvsp.payloaddatasize",
        FT_UINT64, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_pixelcolor,          
        { "Monochrome or Color", "gvsp.pixel.color",
        FT_UINT32, BASE_HEX, VALS(colornames), 0xFF000000,
        NULL, HFILL
        }},

        {& hf_gvsp_pixeloccupy,         
        { "Occupy Bits", "gvsp.pixel.occupy",
        FT_UINT32, BASE_DEC, NULL, 0x00FF0000,
        NULL, HFILL
        }},

        {& hf_gvsp_pixelid,         
        { "ID", "gvsp.pixel.id",
        FT_UINT32, BASE_HEX, NULL, 0x0000FFFF,
        NULL, HFILL
        }},

        {& hf_gvsp_filename,            
        { "ID", "gvsp.filename",
        FT_STRINGZ, BASE_NONE, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_payloadlength,           
        { "Payload Length", "gvsp.payloadlength",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_fieldinfo,           
        { "Field Info", "gvsp.fieldinfo",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_fieldid,         
        { "Field ID", "gvsp.fieldid",
        FT_UINT8, BASE_HEX, NULL, 0xF0,
        NULL, HFILL
        }},

        {& hf_gvsp_fieldcount,          
        { "Field Count", "gvsp.fieldcount",
        FT_UINT8, BASE_HEX, NULL, 0x0F,
        NULL, HFILL
        }},

        {& hf_gvsp_genericflags,            
        { "Generic Flag", "gvsp.genericflag",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_timestamptickfrequency ,         
        { "Timestamp Tick Frequency", "gvsp.timestamptickfrequency",
        FT_UINT64, BASE_HEX, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_dataformat,          
        { "Data Format", "gvsp.dataformat",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetizationmode,           
        { "packetization_mode", "gvsp.packetizationmode",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetsize,          
        { "packet_size", "gvsp.packetsize",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_profileidc,          
        { "profile_idc", "gvsp.profileidc",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_cs,          
        { "cs", "gvsp.cs",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_cs0,         
        { "cs0", "gvsp.cs0",
        FT_UINT8, BASE_HEX, NULL, 0x80,
        NULL, HFILL
        }},

        {& hf_gvsp_cs1,         
        { "cs1", "gvsp.cs1",
        FT_UINT8, BASE_HEX, NULL, 0x40,
        NULL, HFILL
        }},

        {& hf_gvsp_cs2,         
        { "cs2", "gvsp.cs2",
        FT_UINT8, BASE_HEX, NULL, 0x20,
        NULL, HFILL
        }},

        {& hf_gvsp_cs3,         
        { "cs3", "gvsp.cs3",
        FT_UINT8, BASE_HEX, NULL, 0x10,
        NULL, HFILL
        }},

        {& hf_gvsp_levelidc,            
        { "level_idc", "gvsp.levelidc",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropinterleavingdepth,           
        { "srop_interlaving_depth", "gvsp.sropinterleavingdepth",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropmaxdondiff,          
        { "srop_max_don_diff", "gvsp.sropmaxdondiff",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropdeintbufreq,         
        { "srop_deint_buf_req", "gvsp.sropdeintbufreq",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropinitbuftime,         
        { "srop_init_buf_time", "gvsp.sropinitbuftime",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_zoneinfo,            
        { "Zone Info", "gvsp.zoneinfo",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_zoneid,          
        { "Zone ID", "gvsp.zoneid",
        FT_UINT8, BASE_HEX, NULL, 0x3E,
        NULL, HFILL
        }},

        {& hf_gvsp_endofzone,           
        { "End of Zone", "gvsp.endofzone",
        FT_UINT8, BASE_HEX, NULL, 0x01,
        NULL, HFILL
        }},

        {& hf_gvsp_addressoffsethigh,           
        { "Address Offset High", "gvsp.addressoffsethigh",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_addressoffsetlow,            
        { "Address Offset Low", "gvsp.addressoffsetlow",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sc_zone_direction,   
        { "Zone Directions Mask", "gvcpzonedirection",
        FT_BOOLEAN, 32, TFS(&directionnames), 0,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone0_direction,  
        { "Zone 0 Direction", "gvcp.zone0direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x80000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone1_direction,  
        { "Zone 1 Direction", "gvcp.zone1direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x40000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone2_direction,  
        { "Zone 2 Direction", "gvcp.zone2direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x20000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone3_direction,  
        { "Zone 3 Direction", "gvcp.zone3direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x10000000,
        NULL, HFILL
        }}, 
        
        {& hf_gvsp_sc_zone4_direction,  
        { "Zone 4 Direction", "gvcp.zone4direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x08000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone5_direction,  
        { "Zone 5 Direction", "gvcp.zone5direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x04000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone6_direction,  
        { "Zone 6 Direction", "gvcp.zone6direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x02000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone7_direction,  
        { "Zone 7 Direction", "gvcp.zone7direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x01000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone8_direction,  
        { "Zone 8 Direction", "gvcp.zone8direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00800000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone9_direction,  
        { "Zone 9 Direction", "gvcp.zone9direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00400000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone10_direction, 
        { "Zone 10 Direction", "gvcp.zone10direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00200000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone11_direction, 
        { "Zone 11 Direction", "gvcp.zone1direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00100000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone12_direction, 
        { "Zone 12 Direction", "gvcp.zone12direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00080000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone13_direction, 
        { "Zone 13 Direction", "gvcp.zone13direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00040000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone14_direction, 
        { "Zone 14 Direction", "gvcp.zone14direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00020000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone15_direction, 
        { "Zone 15 Direction", "gvcp.zone15direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00010000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone16_direction, 
        { "Zone 16 Direction", "gvcp.zone16direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00008000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone17_direction, 
        { "Zone 17 Direction", "gvcp.zone17direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00004000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone18_direction, 
        { "Zone 18 Direction", "gvcp.zone18direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00002000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone19_direction, 
        { "Zone 19 Direction", "gvcp.zone19direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00001000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone20_direction, 
        { "Zone 20 Direction", "gvcp.zone20direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000800,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone21_direction, 
        { "Zone 21 Direction", "gvcp.zone21direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000400,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone22_direction, 
        { "Zone 22 Direction", "gvcp.zone22direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000200,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone23_direction, 
        { "Zone 23 Direction", "gvcp.zone23direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000100,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone24_direction, 
        { "Zone 24 Direction", "gvcp.zone24direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000080,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone25_direction, 
        { "Zone 25 Direction", "gvcp.zone25direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000040,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone26_direction, 
        { "Zone 26 Direction", "gvcp.zone26direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000020,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone27_direction, 
        { "Zone 27 Direction", "gvcp.zone27direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000010,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone28_direction, 
        { "Zone 28 Direction", "gvcp.zone28direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000008,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone29_direction, 
        { "Zone 29 Direction", "gvcp.zone29direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000004,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone30_direction, 
        { "Zone 30 Direction", "gvcp.zone30direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000002,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone31_direction, 
        { "Zone 31 Direction", "gvcp.zone31direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000001,
        NULL, HFILL
        }}, 

        {& hf_gvsp_chunkdatapayloadlengthex,            
        { "Chunk Data Payload Length", "gvsp.chunkdatapayloadlengthex",
        FT_UINT32, BASE_HEX_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_chunklayoutidex,         
        { "Chunk Layout ID", "gvsp.chunklayoutidex",
        FT_UINT32, BASE_HEX_DEC, NULL, 0x0,
        NULL, HFILL
        }},
    };

    static gint *ett[] = {
        &ett_gvsp,
		&ett_gvsp_flags,
		&ett_gvsp_header,
		&ett_gvsp_payload,
		&ett_gvsp_trailer,
    };

    proto_gvsp = proto_register_protocol("GigE Vision Streaming Protocol", "GVSP", "gvsp");

    register_dissector("gvsp", dissect_gvsp, proto_gvsp);

    proto_register_field_array(proto_gvsp, hfgvsp, array_length(hfgvsp));
    proto_register_subtree_array(ett, array_length(ett));
}


/*
    \brief Rules for GVSP packets identification (?)
 */

void proto_reg_handoff_gvsp(void) 
{
    static int gvsp_initialized = FALSE;
    static dissector_handle_t gvsp_handle;

    if (!gvsp_initialized) 
	{
        gvsp_handle = new_create_dissector_handle((new_dissector_t)dissect_gvsp, proto_gvsp);
        dissector_add_uint("udp.port", global_gvsp_port, gvsp_handle);
        gvsp_initialized = TRUE;
    } 
	else 
	{
        dissector_delete_uint("udp.port", global_gvsp_port, gvsp_handle);
    }
}

/* packet-gvsp.c
 * Routines for AIA GigE Vision (TM) Streaming Protocol dissection
 * Copyright 2012, AIA <www.visiononline.org> All rights reserved
 *
 * GigE Vision (TM): GigE Vision a standard developed under the sponsorship of the AIA for 
 * the benefit of the machine vision industry. GVSP stands for GigE Vision (TM) Streaming
 * Protocol.
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*#ifdef HAVE_CONFIG_H */
#include "config.h"
/*#endif */

#ifdef PACKAGE
#undef PACKAGE
#endif

#define PACKAGE "gvsp"

#ifdef VERSION
#undef VERSION
#endif

#define VERSION "0.0.1"

#include <epan/packet.h>


/*
  Payload types
 */

#define GVSP_PAYLOAD_IMAGE ( 0x0001 )
#define GVSP_PAYLOAD_RAWDATA (0x0002 )
#define GVSP_PAYLOAD_FILE ( 0x0003 )
#define GVSP_PAYLOAD_CHUNKDATA ( 0x0004 )
#define GVSP_PAYLOAD_EXTENDEDCHUNKDATA ( 0x0005 ) /* Deprecated */
#define GVSP_PAYLOAD_JPEG ( 0x0006 )
#define GVSP_PAYLOAD_JPEG2000 ( 0x0007 )
#define GVSP_PAYLOAD_H264 ( 0x0008 )
#define GVSP_PAYLOAD_MULTIZONEIMAGE ( 0x0009 )
#define GVSP_PAYLOAD_DEVICEPSECIFICSTART ( 0x8000 )


/*
   GVSP packet types
 */

#define GVSP_PACKET_LEADER ( 1 )
#define GVSP_PACKET_TRAILER ( 2 )
#define GVSP_PACKET_PAYLOAD ( 3 )
#define GVSP_PACKET_ALLIN ( 4 )
#define GVSP_PACKET_PAYLOAD_H264 ( 5 )
#define GVSP_PACKET_PAYLOAD_MULTIZONE ( 6 )


/*
   GVSP statuses
 */

#define GEV_STATUS_SUCCESS (0x0000) 
#define GEV_STATUS_PACKET_RESEND (0x0100) 
#define GEV_STATUS_NOT_IMPLEMENTED (0x8001) 
#define GEV_STATUS_INVALID_PARAMETER (0x8002) 
#define GEV_STATUS_INVALID_ADDRESS (0x8003) 
#define GEV_STATUS_WRITE_PROTECT (0x8004) 
#define GEV_STATUS_BAD_ALIGNMENT (0x8005) 
#define GEV_STATUS_ACCESS_DENIED (0x8006) 
#define GEV_STATUS_BUSY (0x8007) 
#define GEV_STATUS_LOCAL_PROBLEM (0x8008)  /* deprecated */
#define GEV_STATUS_MSG_MISMATCH (0x8009) /* deprecated */
#define GEV_STATUS_INVALID_PROTOCOL (0x800A) /* deprecated */
#define GEV_STATUS_NO_MSG (0x800B) /* deprecated */
#define GEV_STATUS_PACKET_UNAVAILABLE (0x800C) 
#define GEV_STATUS_DATA_OVERRUN (0x800D) 
#define GEV_STATUS_INVALID_HEADER (0x800E) 
#define GEV_STATUS_WRONG_CONFIG (0x800F) /* deprecated */
#define GEV_STATUS_PACKET_NOT_YET_AVAILABLE (0x8010) 
#define GEV_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY (0x8011) 
#define GEV_STATUS_PACKET_REMOVED_FROM_MEMORY (0x8012) 
#define GEV_STATUS_ERROR (0x8FFF) 


/*
   Pixel type color
 */

#define GVSP_PIX_MONO             (0x01000000)
#define GVSP_PIX_RGB              (0x02000000)
#define GVSP_PIX_COLOR            (0x02000000)
#define GVSP_PIX_CUSTOM           (0x80000000)
#define GVSP_PIX_COLOR_MASK       (0xFF000000)


/*
   Pixel type size
 */

#define GVSP_PIX_OCCUPY1BIT       (0x00010000)
#define GVSP_PIX_OCCUPY2BIT       (0x00020000)
#define GVSP_PIX_OCCUPY4BIT       (0x00040000)
#define GVSP_PIX_OCCUPY8BIT       (0x00080000)
#define GVSP_PIX_OCCUPY12BIT      (0x000C0000)
#define GVSP_PIX_OCCUPY16BIT      (0x00100000)
#define GVSP_PIX_OCCUPY24BIT      (0x00180000)
#define GVSP_PIX_OCCUPY32BIT      (0x00200000)
#define GVSP_PIX_OCCUPY36BIT      (0x00240000)
#define GVSP_PIX_OCCUPY48BIT      (0x00300000)


/*
   Pxiel type masks, shifts 
 */
#define GVSP_PIX_EFFECTIVE_PIXEL_SIZE_MASK (0x00FF0000)
#define GVSP_PIX_EFFECTIVE_PIXEL_SIZE_SHIFT (16)

#define GVSP_PIX_ID_MASK (0x0000FFFF)


/*
   Pixel types
 */

#define GVSP_PIX_COUNT (0x46)

#define GVSP_PIX_MONO1P                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY1BIT  | 0x0037)
#define GVSP_PIX_MONO2P                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY2BIT  | 0x0038)
#define GVSP_PIX_MONO4P                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY4BIT  | 0x0039)
#define GVSP_PIX_MONO8                   (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0001)
#define GVSP_PIX_MONO8S                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0002)
#define GVSP_PIX_MONO10                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0003)
#define GVSP_PIX_MONO10_PACKED           (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0004)
#define GVSP_PIX_MONO12                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0005)
#define GVSP_PIX_MONO12_PACKED           (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0006)
#define GVSP_PIX_MONO14                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0025)
#define GVSP_PIX_MONO16                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0007)
											    
#define GVSP_PIX_BAYGR8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0008)
#define GVSP_PIX_BAYRG8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0009)
#define GVSP_PIX_BAYGB8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x000A)
#define GVSP_PIX_BAYBG8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x000B)
#define GVSP_PIX_BAYGR10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000C)
#define GVSP_PIX_BAYRG10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000D)
#define GVSP_PIX_BAYGB10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000E)
#define GVSP_PIX_BAYBG10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000F)
#define GVSP_PIX_BAYGR12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0010)
#define GVSP_PIX_BAYRG12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0011)
#define GVSP_PIX_BAYGB12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0012)
#define GVSP_PIX_BAYBG12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0013)
#define GVSP_PIX_BAYGR10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0026)
#define GVSP_PIX_BAYRG10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0027)
#define GVSP_PIX_BAYGB10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0028)
#define GVSP_PIX_BAYBG10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0029)
#define GVSP_PIX_BAYGR12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002A)
#define GVSP_PIX_BAYRG12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002B)
#define GVSP_PIX_BAYGB12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002C)
#define GVSP_PIX_BAYBG12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002D)
#define GVSP_PIX_BAYGR16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x002E)
#define GVSP_PIX_BAYRG16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x002F)
#define GVSP_PIX_BAYGB16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0030)
#define GVSP_PIX_BAYBG16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0031)
								         
#define GVSP_PIX_RGB8                    (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0014)
#define GVSP_PIX_BGR8                    (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0015)
#define GVSP_PIX_RGBA8                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x0016)
#define GVSP_PIX_BGRA8                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x0017)
#define GVSP_PIX_RGB10                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0018)
#define GVSP_PIX_BGR10                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0019)
#define GVSP_PIX_RGB12                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x001A)
#define GVSP_PIX_BGR12                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x001B)
#define GVSP_PIX_RGB16                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0033)
#define GVSP_PIX_RGB10V1_PACKED          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x001C)
#define GVSP_PIX_RGB10P32                (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x001D)
#define GVSP_PIX_RGB12V1_PACKED          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY36BIT | 0x0034)
#define GVSP_PIX_RGB565P                 (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0035)
#define GVSP_PIX_BGR565P                 (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0036)

#define GVSP_PIX_YUV411_8_UYYVYY         (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x001E)
#define GVSP_PIX_YUV422_8_UYVY           (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x001F)
#define GVSP_PIX_YUV422_8                (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0032)
#define GVSP_PIX_YUV8_UYV                (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0020)
#define GVSP_PIX_YCBCR8_CBYCR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x003A)
#define GVSP_PIX_YCBCR422_8              (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x003B)
#define GVSP_PIX_YCBCR422_8_CBYCRY       (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0043)
#define GVSP_PIX_YCBCR422_8_CBYYCRYY     (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x003C)
#define GVSP_PIX_YCBCR601_8_CBYCR        (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x003D)
#define GVSP_PIX_YCBCR601_422_8          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x003E)
#define GVSP_PIX_YCBCR601_422_8_CBYCRY   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0044)
#define GVSP_PIX_YCBCR601_411_8_CBYYCRYY (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x003F)
#define GVSP_PIX_YCBCR709_411_8_CBYCR    (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0040)
#define GVSP_PIX_YCBCR709_422_8          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0041)
#define GVSP_PIX_YCBCR709_422_8_CBYCRY   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0045)
#define GVSP_PIX_YCBCR709_411_8_CBYYCRYY (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x0042)

#define GVSP_PIX_RGB8_PLANAR             (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0021)
#define GVSP_PIX_RGB10_PLANAR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0022)
#define GVSP_PIX_RGB12_PLANAR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0023)
#define GVSP_PIX_RGB16_PLANAR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0024)


typedef struct _display_string 
{
	int display;
	gchar* string_display;

} display_string;


/* Structure to hold GVSP packet information */
typedef struct _gvsp_packet_info
{
	gint chunk;
	guint8 format;
	guint16 payloadtype;
	guint64 blockid;
	guint32 packetid;
	gint enhanced;
	gint flag_resendrangeerror;
	gint flag_previousblockdropped;
	gint flag_packetresend;

	const char *formatstring;
	const char *payloadstring;

} gvsp_packet_info;


/*Define the gvsp proto */
static int proto_gvsp = -1;
static int global_gvsp_port = 20202;


/* Define the tree for gvsp */
static int ett_gvsp = -1;
static int ett_gvsp_flags = -1;
static int ett_gvsp_header = -1;
static int ett_gvsp_payload = -1;
static int ett_gvsp_trailer = -1;


typedef struct _ftenum_string {
	enum ftenum type;
	gchar* string_type;
} ftenum_string;

static const value_string statusnames[] = {
    { GEV_STATUS_SUCCESS, "GEV_STATUS_SUCCESS" },
    { GEV_STATUS_PACKET_RESEND, "GEV_STATUS_PACKET_RESEND" },
    { GEV_STATUS_NOT_IMPLEMENTED, "GEV_STATUS_NOT_IMPLEMENTED" },
    { GEV_STATUS_INVALID_PARAMETER, "GEV_STATUS_INVALID_PARAMETER" },
    { GEV_STATUS_INVALID_ADDRESS, "GEV_STATUS_INVALID_ADDRESS" },
    { GEV_STATUS_WRITE_PROTECT, "GEV_STATUS_WRITE_PROTECT" },
    { GEV_STATUS_BAD_ALIGNMENT, "GEV_STATUS_BAD_ALIGNMENT" },
    { GEV_STATUS_ACCESS_DENIED, "GEV_STATUS_ACCESS_DENIED" },
    { GEV_STATUS_BUSY, "GEV_STATUS_BUSY" },
    { GEV_STATUS_LOCAL_PROBLEM, "GEV_STATUS_LOCAL_PROBLEM (deprecated)" },
    { GEV_STATUS_MSG_MISMATCH, "GEV_STATUS_MSG_MISMATCH (deprecated)" },
    { GEV_STATUS_INVALID_PROTOCOL, "GEV_STATUS_INVALID_PROTOCOL (deprecated)" },
    { GEV_STATUS_NO_MSG, "GEV_STATUS_NO_MSG (deprecated)" },
    { GEV_STATUS_PACKET_UNAVAILABLE, "GEV_STATUS_PACKET_UNAVAILABLE" },
    { GEV_STATUS_DATA_OVERRUN, "GEV_STATUS_DATA_OVERRUN" },
    { GEV_STATUS_INVALID_HEADER, "GEV_STATUS_INVALID_HEADER" },
    { GEV_STATUS_WRONG_CONFIG, "GEV_STATUS_WRONG_CONFIG (deprecated)" },
    { GEV_STATUS_PACKET_NOT_YET_AVAILABLE, "GEV_STATUS_PACKET_NOT_YET_AVAILABLE" },
    { GEV_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY, "GEV_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY" },
    { GEV_STATUS_PACKET_REMOVED_FROM_MEMORY, "GEV_STATUS_PACKET_REMOVED_FROM_MEMORY" },
    { GEV_STATUS_ERROR, "GEV_STATUS_ERROR" },
    { 0, NULL },
};

static const value_string formatnames[] = {
    { GVSP_PACKET_LEADER, "LEADER" },
    { GVSP_PACKET_TRAILER, "TRAILER" },
    { GVSP_PACKET_PAYLOAD, "PAYLOAD" },
    { GVSP_PACKET_ALLIN, "ALLIN" },
    { GVSP_PACKET_PAYLOAD_H264, "H264" },
    { GVSP_PACKET_PAYLOAD_MULTIZONE, "MULTIZONE" },
    { 0x80 | GVSP_PACKET_LEADER, "LEADER (ext IDs)" },
    { 0x80 | GVSP_PACKET_TRAILER, "TRAILER (ext IDs)" },
    { 0x80 | GVSP_PACKET_PAYLOAD, "PAYLOAD (ext IDs)" },
    { 0x80 | GVSP_PACKET_ALLIN, "ALLIN (ext IDs)" },
    { 0x80 | GVSP_PACKET_PAYLOAD_H264, "H264 (ext IDs)" },
    { 0x80 | GVSP_PACKET_PAYLOAD_MULTIZONE, "MULTIZONE (ext IDs)" },
    { 0, NULL },
};

static const value_string payloadtypenames[] = {
    { GVSP_PAYLOAD_IMAGE, "IMAGE" },
    { GVSP_PAYLOAD_RAWDATA, "RAWDATA" },
    { GVSP_PAYLOAD_FILE, "FILE" },
    { GVSP_PAYLOAD_CHUNKDATA, "CHUNKDATA" },
    { GVSP_PAYLOAD_EXTENDEDCHUNKDATA, "EXTENDEDCHUNKDATA (obsolete with v2.0)" },
    { GVSP_PAYLOAD_JPEG, "JPEG" },
    { GVSP_PAYLOAD_JPEG2000, "JPEG2000" },
    { GVSP_PAYLOAD_H264, "H264" },
    { GVSP_PAYLOAD_MULTIZONEIMAGE, "MUTLIZONEIAMGE" },
    { 0x4000 | GVSP_PAYLOAD_IMAGE, "IMAGE (v2.0 chunks)" },
    { 0x4000 | GVSP_PAYLOAD_RAWDATA, "RAWDATA (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_FILE, "FILE (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_CHUNKDATA, "CHUNKDATA (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_EXTENDEDCHUNKDATA, "EXTENDEDCHUNKDATA (v2.0 chunks?)" },
    { 0x4000 | GVSP_PAYLOAD_JPEG, "JPEG (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_JPEG2000, "JPEG2000 (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_H264, "H264 (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_MULTIZONEIMAGE, "MULTIZONEIMAGE (v2.0 Chunks)" },
    { 0, NULL },
};

static const value_string pixeltypenames[] = {
	{ GVSP_PIX_MONO1P, "GVSP_PIX_MONO1P" },                  
    { GVSP_PIX_MONO2P, "GVSP_PIX_MONO2P" },                  
    { GVSP_PIX_MONO4P, "GVSP_PIX_MONO4P" },
    { GVSP_PIX_MONO8, "GVSP_PIX_MONO8" },                   
    { GVSP_PIX_MONO8S, "GVSP_PIX_MONO8S" },                  
    { GVSP_PIX_MONO10, "GVSP_PIX_MONO10" },                  
    { GVSP_PIX_MONO10_PACKED, "GVSP_PIX_MONO10_PACKED" },           
    { GVSP_PIX_MONO12, "GVSP_PIX_MONO12" },                  
    { GVSP_PIX_MONO12_PACKED, "GVSP_PIX_MONO12_PACKED" },           
    { GVSP_PIX_MONO14, "GVSP_PIX_MONO14" },                  
    { GVSP_PIX_MONO16, "GVSP_PIX_MONO16" },                  
    { GVSP_PIX_BAYGR8, "GVSP_PIX_BAYGR8" },                  
    { GVSP_PIX_BAYRG8, "GVSP_PIX_BAYRG8" },                  
    { GVSP_PIX_BAYGB8, "GVSP_PIX_BAYGB8" },                  
    { GVSP_PIX_BAYBG8, "GVSP_PIX_BAYBG8" },                  
    { GVSP_PIX_BAYGR10, "GVSP_PIX_BAYGR10" },                 
    { GVSP_PIX_BAYRG10, "GVSP_PIX_BAYRG10" },                 
    { GVSP_PIX_BAYGB10, "GVSP_PIX_BAYGB10" },                 
    { GVSP_PIX_BAYBG10, "GVSP_PIX_BAYBG10" },                 
    { GVSP_PIX_BAYGR12, "GVSP_PIX_BAYGR12" },                 
    { GVSP_PIX_BAYRG12, "GVSP_PIX_BAYRG12" },                 
    { GVSP_PIX_BAYGB12, "GVSP_PIX_BAYGB12" },                 
    { GVSP_PIX_BAYBG12, "GVSP_PIX_BAYBG12" },                 
    { GVSP_PIX_BAYGR10_PACKED, "GVSP_PIX_BAYGR10_PACKED" },          
    { GVSP_PIX_BAYRG10_PACKED, "GVSP_PIX_BAYRG10_PACKED" },          
    { GVSP_PIX_BAYGB10_PACKED, "GVSP_PIX_BAYGB10_PACKED" },          
    { GVSP_PIX_BAYBG10_PACKED, "GVSP_PIX_BAYBG10_PACKED" },          
    { GVSP_PIX_BAYGR12_PACKED, "GVSP_PIX_BAYGR12_PACKED" },          
    { GVSP_PIX_BAYRG12_PACKED, "GVSP_PIX_BAYRG12_PACKED" },          
    { GVSP_PIX_BAYGB12_PACKED, "GVSP_PIX_BAYGB12_PACKED" },          
    { GVSP_PIX_BAYBG12_PACKED, "GVSP_PIX_BAYBG12_PACKED" },          
    { GVSP_PIX_BAYGR16, "GVSP_PIX_BAYGR16" },                 
    { GVSP_PIX_BAYRG16, "GVSP_PIX_BAYRG16" },                 
    { GVSP_PIX_BAYGB16, "GVSP_PIX_BAYGB16" },                 
    { GVSP_PIX_BAYBG16, "GVSP_PIX_BAYBG16" },                 
    { GVSP_PIX_RGB8, "GVSP_PIX_RGB8" },                    
    { GVSP_PIX_BGR8, "GVSP_PIX_BGR8" },                    
    { GVSP_PIX_RGBA8, "GVSP_PIX_RGBA8" },                   
    { GVSP_PIX_BGRA8, "GVSP_PIX_BGRA8" },                   
    { GVSP_PIX_RGB10, "GVSP_PIX_RGB10" },                   
    { GVSP_PIX_BGR10, "GVSP_PIX_BGR10" },                   
    { GVSP_PIX_RGB12, "GVSP_PIX_RGB12" },                   
    { GVSP_PIX_BGR12, "GVSP_PIX_BGR12" },                   
    { GVSP_PIX_RGB16, "GVSP_PIX_RGB16" },                   
    { GVSP_PIX_RGB10V1_PACKED, "GVSP_PIX_RGB10V1_PACKED" },          
    { GVSP_PIX_RGB10P32, "GVSP_PIX_RGB10P32" },                
    { GVSP_PIX_RGB12V1_PACKED, "GVSP_PIX_RGB12V1_PACKED" },          
    { GVSP_PIX_RGB565P, "GVSP_PIX_RGB565P" },                 
    { GVSP_PIX_BGR565P, "GVSP_PIX_BGR565P" },                 
    { GVSP_PIX_YUV411_8_UYYVYY, "GVSP_PIX_YUV411_8_UYYVYY" },         
    { GVSP_PIX_YUV422_8_UYVY, "GVSP_PIX_YUV422_8_UYVY" },           
    { GVSP_PIX_YUV422_8, "GVSP_PIX_YUV422_8" },                
    { GVSP_PIX_YUV8_UYV, "GVSP_PIX_YUV8_UYV" },                
    { GVSP_PIX_YCBCR8_CBYCR, "GVSP_PIX_YCBCR8_CBYCR" },            
    { GVSP_PIX_YCBCR422_8, "GVSP_PIX_YCBCR422_8" },              
    { GVSP_PIX_YCBCR422_8_CBYCRY, "GVSP_PIX_YCBCR422_8_CBYCRY" },       
    { GVSP_PIX_YCBCR422_8_CBYYCRYY, "GVSP_PIX_YCBCR422_8_CBYYCRYY" },     
    { GVSP_PIX_YCBCR601_8_CBYCR, "GVSP_PIX_YCBCR601_8_CBYCR" },        
    { GVSP_PIX_YCBCR601_422_8, "GVSP_PIX_YCBCR601_422_8" },          
    { GVSP_PIX_YCBCR601_422_8_CBYCRY, "GVSP_PIX_YCBCR601_422_8_CBYCRY" },   
    { GVSP_PIX_YCBCR601_411_8_CBYYCRYY, "GVSP_PIX_YCBCR601_411_8_CBYYCRYY" }, 
    { GVSP_PIX_YCBCR709_411_8_CBYCR, "GVSP_PIX_YCBCR709_411_8_CBYCR" },    
    { GVSP_PIX_YCBCR709_422_8, "GVSP_PIX_YCBCR709_422_8" },         
    { GVSP_PIX_YCBCR709_422_8, "GVSP_PIX_YCBCR709_422_8" },          
    { GVSP_PIX_YCBCR709_411_8_CBYYCRYY, "GVSP_PIX_YCBCR709_411_8_CBYYCRYY" }, 
    { GVSP_PIX_RGB8_PLANAR, "GVSP_PIX_RGB8_PLANAR" },             
    { GVSP_PIX_RGB10_PLANAR, "GVSP_PIX_RGB10_PLANAR" },            
    { GVSP_PIX_RGB12_PLANAR, "GVSP_PIX_RGB12_PLANAR" },            
    { GVSP_PIX_RGB16_PLANAR, "GVSP_PIX_RGB16_PLANAR" },            
    { 0, NULL },
};

static const value_string colornames[] = {
    { GVSP_PIX_MONO >> 24, "Mono" },
    { GVSP_PIX_COLOR >> 24, "Color" },
    { GVSP_PIX_CUSTOM >> 24, "Custom" },
    { 0, NULL },
};

static const true_false_string directionnames = {
	"Receiver",
	"Transmitter"
};


static int hf_gvsp_status = -1;
static int hf_gvsp_blockid16 = -1;
static int hf_gvsp_flags = -1;
static int hf_gvsp_flagdevicespecific0 = -1;
static int hf_gvsp_flagdevicespecific1 = -1;
static int hf_gvsp_flagdevicespecific2 = -1;
static int hf_gvsp_flagdevicespecific3 = -1;
static int hf_gvsp_flagdevicespecific4 = -1;
static int hf_gvsp_flagdevicespecific5 = -1;
static int hf_gvsp_flagdevicespecific6 = -1;
static int hf_gvsp_flagdevicespecific7 = -1;
static int hf_gvsp_flagresendrangeerror = -1;
static int hf_gvsp_flagpreviousblockdropped = -1;
static int hf_gvsp_flagpacketresend = -1;
static int hf_gvsp_format = -1;
static int hf_gvsp_packetid24 = -1;
static int hf_gvsp_blockid64 = -1;
static int hf_gvsp_packetid32 = -1;
static int hf_gvsp_payloadtype = -1;
static int hf_gvsp_payloaddata = -1;
static int hf_gvsp_timestamp = -1;
static int hf_gvsp_pixelformat = -1;
static int hf_gvsp_sizex = -1;
static int hf_gvsp_sizey = -1;
static int hf_gvsp_offsetx = -1;
static int hf_gvsp_offsety = -1;
static int hf_gvsp_paddingx = -1;
static int hf_gvsp_paddingy = -1;
static int hf_gvsp_payloaddatasize = -1;
static int hf_gvsp_pixelcolor = -1;
static int hf_gvsp_pixeloccupy = -1;
static int hf_gvsp_pixelid = -1;
static int hf_gvsp_filename = -1;
static int hf_gvsp_payloadlength = -1;
static int hf_gvsp_fieldinfo = -1;
static int hf_gvsp_fieldid = -1;
static int hf_gvsp_fieldcount = -1;
static int hf_gvsp_genericflags = -1;
static int hf_gvsp_chunklayoutid = -1;
static int hf_gvsp_timestamptickfrequency = -1;
static int hf_gvsp_dataformat = -1;
static int hf_gvsp_packetizationmode = -1;
static int hf_gvsp_packetsize = -1;
static int hf_gvsp_profileidc = -1;
static int hf_gvsp_cs = -1;
static int hf_gvsp_cs0 = -1;
static int hf_gvsp_cs1 = -1;
static int hf_gvsp_cs2 = -1;
static int hf_gvsp_cs3 = -1;
static int hf_gvsp_levelidc = -1;
static int hf_gvsp_sropinterleavingdepth = -1;
static int hf_gvsp_sropmaxdondiff = -1;
static int hf_gvsp_sropdeintbufreq = -1;
static int hf_gvsp_sropinitbuftime = -1;
static int hf_gvsp_zoneinfo = -1;
static int hf_gvsp_zoneid = -1;
static int hf_gvsp_endofzone = -1;
static int hf_gvsp_addressoffsethigh = -1;
static int hf_gvsp_addressoffsetlow = -1;
static int hf_gvsp_sc_zone_direction = -1;
static int hf_gvsp_sc_zone0_direction = -1;
static int hf_gvsp_sc_zone1_direction = -1;
static int hf_gvsp_sc_zone2_direction = -1;
static int hf_gvsp_sc_zone3_direction = -1;
static int hf_gvsp_sc_zone4_direction = -1;
static int hf_gvsp_sc_zone5_direction = -1;
static int hf_gvsp_sc_zone6_direction = -1;
static int hf_gvsp_sc_zone7_direction = -1;
static int hf_gvsp_sc_zone8_direction = -1;
static int hf_gvsp_sc_zone9_direction = -1;
static int hf_gvsp_sc_zone10_direction = -1;
static int hf_gvsp_sc_zone11_direction = -1;
static int hf_gvsp_sc_zone12_direction = -1;
static int hf_gvsp_sc_zone13_direction = -1;
static int hf_gvsp_sc_zone14_direction = -1;
static int hf_gvsp_sc_zone15_direction = -1;
static int hf_gvsp_sc_zone16_direction = -1;
static int hf_gvsp_sc_zone17_direction = -1;
static int hf_gvsp_sc_zone18_direction = -1;
static int hf_gvsp_sc_zone19_direction = -1;
static int hf_gvsp_sc_zone20_direction = -1;
static int hf_gvsp_sc_zone21_direction = -1;
static int hf_gvsp_sc_zone22_direction = -1;
static int hf_gvsp_sc_zone23_direction = -1;
static int hf_gvsp_sc_zone24_direction = -1;
static int hf_gvsp_sc_zone25_direction = -1;
static int hf_gvsp_sc_zone26_direction = -1;
static int hf_gvsp_sc_zone27_direction = -1;
static int hf_gvsp_sc_zone28_direction = -1;
static int hf_gvsp_sc_zone29_direction = -1;
static int hf_gvsp_sc_zone30_direction = -1;
static int hf_gvsp_sc_zone31_direction = -1;
static int hf_gvsp_chunkdatapayloadlengthex = -1;
static int hf_gvsp_chunklayoutidex = -1;


/*
    \brief Dissects the image leader
 */

gint DissectImageLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{	
	proto_item *item = NULL;
	proto_item *ti = NULL;
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Pixel format */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_pixelformat, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_pixelcolor, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixeloccupy, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixelid, tvb, offset + 12, 4, ENC_BIG_ENDIAN);

		/* Size X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizex, tvb, offset + 16, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 20, 4, ENC_BIG_ENDIAN);

		/* Offset X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsetx, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* Offset Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsety, tvb, offset + 28, 4, ENC_BIG_ENDIAN);

		/* Padding X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingx, tvb, offset + 32, 2, ENC_BIG_ENDIAN);

		/* Padding Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingy, tvb, offset + 34, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 36;
}


/*
    \brief Dissects the image trailer
 */

gint DissectImageTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{	
	gint offset;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 8;
}


/*
    \brief Dissects the raw data leader
 */

gint DissectRawDataLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 12, 8, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 20;
}


/*
    \brief Dissects a file leader
 */

gint DissectFileLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 12, 8, ENC_BIG_ENDIAN);

		/* Filename */
		proto_tree_add_item(gvsp_tree, hf_gvsp_filename, tvb, offset + 20, -1, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 20; /* Problem with the spec? All-in does not work if variable header size... */
}


/*
    \brief Dissects a chunk data leader
 */

gint DissectChunkDataLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 12;
}


/*
    \brief Dissects a chunk data trailer
 */

gint DissectChunkDataTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Payload data length */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadlength, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 8;
}


/*
    \brief Dissects extended chunk data leader
 */

gint DissectExtendedChunkDataLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;	
    proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Generic flags */
		proto_tree_add_item(gvsp_tree, hf_gvsp_genericflags, tvb, offset + 1, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Pixel format */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_pixelformat, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_pixelcolor, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixeloccupy, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixelid, tvb, offset + 12, 4, ENC_BIG_ENDIAN);

		/* Size X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizex, tvb, offset + 16, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 20, 4, ENC_BIG_ENDIAN);

		/* Offset X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsetx, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* Offset Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsety, tvb, offset + 28, 4, ENC_BIG_ENDIAN);

		/* Padding X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingx, tvb, offset + 32, 2, ENC_BIG_ENDIAN);

		/* Padding Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingy, tvb, offset + 34, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 36;
}


/*
    \brief Dissects extended chunk data trailer
 */

gint DissectExtendedChunkDataTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Payload data length */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadlength, tvb, offset + 4, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 8, 4, ENC_BIG_ENDIAN);

		/* Chunk layout ID */
		proto_tree_add_item(gvsp_tree, hf_gvsp_chunklayoutid, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 16;
}


/*
    \brief Dissects a JPEG leader
 */

gint DissectJPEGLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Generic flags */
		proto_tree_add_item(gvsp_tree, hf_gvsp_genericflags, tvb, offset + 1, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 12, 8, ENC_BIG_ENDIAN);

		/* Timestamp tick frequency */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamptickfrequency, tvb, offset + 20, 8, ENC_BIG_ENDIAN);

		/* Data format */
		proto_tree_add_item(gvsp_tree, hf_gvsp_dataformat, tvb, offset + 24, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 28;
}


/*
 \brief Dissects a H264 leader
 */

gint DissectH264Leader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* packetization_mode */
		proto_tree_add_item(gvsp_tree, hf_gvsp_packetizationmode, tvb, offset + 13, 1, ENC_BIG_ENDIAN);

		/* packet_size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_packetsize, tvb, offset + 14, 2, ENC_BIG_ENDIAN);

		/* profile_idc */
		proto_tree_add_item(gvsp_tree, hf_gvsp_profileidc, tvb, offset + 17, 1, ENC_BIG_ENDIAN);

		/* cs0, 1, 2 ,3 */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_cs, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs0, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs1, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs2, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs3, tvb, offset + 18, 1, ENC_BIG_ENDIAN);

		/* level_idc */
		proto_tree_add_item(gvsp_tree, hf_gvsp_levelidc, tvb, offset + 19, 1, ENC_BIG_ENDIAN);

		/* srop_interleaving_depth */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropinterleavingdepth, tvb, offset + 20, 2, ENC_BIG_ENDIAN);

		/* srop_max_don_diff */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropmaxdondiff, tvb, offset + 22, 2, ENC_BIG_ENDIAN);

		/* srop_deint_buf_req */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropdeintbufreq, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* srop_init_buf_time */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropinitbuftime, tvb, offset + 28, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 32;
}


/*
    \brief Dissects the multizone image leader
 */

gint DissectMultizoneImageLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
    proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Zone direction */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_sc_zone_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_sc_zone0_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone1_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone2_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone3_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone4_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone5_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone6_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone7_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone8_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone9_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone10_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone11_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone12_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone13_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone14_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone15_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone16_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone17_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone18_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone19_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone20_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone21_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone22_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone23_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone24_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone25_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone26_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone27_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone28_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone29_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone30_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone31_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);

		/* Pixel format */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_pixelformat, tvb, offset + 16, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_pixelcolor, tvb, offset + 16, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixeloccupy, tvb, offset + 16, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixelid, tvb, offset + 16, 4, ENC_BIG_ENDIAN);

		/* Size X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizex, tvb, offset + 20, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* Offset X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsetx, tvb, offset + 28, 4, ENC_BIG_ENDIAN);

		/* Offset Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsety, tvb, offset + 32, 4, ENC_BIG_ENDIAN);

		/* Padding X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingx, tvb, offset + 36, 2, ENC_BIG_ENDIAN);

		/* Padding Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingy, tvb, offset + 38, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 40;
}


/*
    \brief Dissects a generic trailer (contains just the payload type)
 */

gint DissectGenericTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 4;
}


/*
    \brief Dissects a generic trailer (contains just the payload type)
 */

gint DissectExtraChunkInfo(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Chunk data payload length */
		proto_tree_add_item(gvsp_tree, hf_gvsp_chunkdatapayloadlengthex, tvb, offset, 4, ENC_BIG_ENDIAN);

		/* Chunk layoud id */
		proto_tree_add_item(gvsp_tree, hf_gvsp_chunklayoutidex, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 8;
}


/*
    \brief Dissects a packet payload
 */

void DissectPacketPayload(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddata, tvb, offset, -1, ENC_BIG_ENDIAN);
	}
}


/*
    \brief Dissects a packet payload for H264
 */

void DissectPacketPayloadH264(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 0, 8, ENC_BIG_ENDIAN);

		/* Data */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddata, tvb, offset + 8, -1, ENC_BIG_ENDIAN);
	}
}


/*
    \brief Dissects a packet payload for multizone
 */

void DissectPacketPayloadMultizone(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Zone information */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_zoneinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_zoneid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_endofzone, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Address offset high */
		proto_tree_add_item(gvsp_tree, hf_gvsp_addressoffsethigh, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Address offset low */
		proto_tree_add_item(gvsp_tree, hf_gvsp_addressoffsetlow, tvb, offset + 4, 4, ENC_BIG_ENDIAN);

		/* Data */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddata, tvb, offset + 8, -1, ENC_BIG_ENDIAN);
	}
}


/*
    \brief Dissects an all in packet
 */

void DissectPacketAllIn(proto_tree *gvsp_tree, tvbuff_t *tvb, gint startoffset, packet_info *pinfo, gvsp_packet_info *info)
{
	gint offset;
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		switch (info->payloadtype)
		{
		case GVSP_PAYLOAD_IMAGE:
			offset += DissectImageLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectImageTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_RAWDATA:
			offset += DissectRawDataLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_FILE:
			offset += DissectFileLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_CHUNKDATA:
			offset += DissectChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_EXTENDEDCHUNKDATA:
			offset += DissectExtendedChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectExtendedChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_JPEG:
		case GVSP_PAYLOAD_JPEG2000:
			offset += DissectJPEGLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_H264:
			offset += DissectH264Leader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayloadH264(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_MULTIZONEIMAGE:
			offset += DissectMultizoneImageLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectImageTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayloadMultizone(gvsp_tree, tvb, pinfo, offset);
			break;
		}
	}
}


/*
    \brief Dissects a leader packet
 */

void DissectPacketLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, gint startoffset, packet_info *pinfo, gvsp_packet_info *info)
{
	gint offset;
	offset = startoffset;

	switch (info->payloadtype)
	{
	case GVSP_PAYLOAD_IMAGE:
		DissectImageLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_RAWDATA:
		DissectRawDataLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_FILE:
		DissectFileLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_CHUNKDATA:
		DissectChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_EXTENDEDCHUNKDATA:
		DissectExtendedChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_JPEG:
	case GVSP_PAYLOAD_JPEG2000:
		DissectJPEGLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_H264:
		DissectH264Leader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_MULTIZONEIMAGE:
		DissectMultizoneImageLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	default:
		break;
	}
}


/*
    \brief Dissects a trailer packet
 */

void DissectPacketTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, gint startoffset, packet_info *pinfo, gvsp_packet_info *info)
{
	gint offset;
	offset = startoffset;

	switch (info->payloadtype)
	{
	case GVSP_PAYLOAD_IMAGE:
	case GVSP_PAYLOAD_MULTIZONEIMAGE:
		offset += DissectImageTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_CHUNKDATA:
		offset += DissectChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_EXTENDEDCHUNKDATA:
		offset += DissectExtendedChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_RAWDATA:
	case GVSP_PAYLOAD_FILE:
	case GVSP_PAYLOAD_JPEG:
	case GVSP_PAYLOAD_JPEG2000:
	case GVSP_PAYLOAD_H264:
		offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	default:
		break;
	}

	if (info->chunk != 0)
	{
		offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
	}
}


/*
    \brief Point of entry of all GVSP dissection
 */

static void dissect_gvsp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) 
{
    proto_item *ti = NULL;
    gint offset = 0;
	proto_tree *gvsp_tree = NULL;
	proto_item *item = NULL;
	gvsp_packet_info info;
	guint8 firstbyte = 0;

	memset(&info, 0x00, sizeof(info));

	/* Set the protocol column */
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "GVSP");

	/* Clear out stuff in the info column */
	col_clear(pinfo->cinfo, COL_INFO);

	/* Adds "Gigabit-Ethernet Streaming Protocol" heading to protocol tree */
	/* We will add fields to this using the gvsp_tree pointer */
	if (tree != NULL)
	{
		ti = proto_tree_add_item(tree, proto_gvsp, tvb, offset, -1, ENC_BIG_ENDIAN);
		gvsp_tree = proto_item_add_subtree(ti, ett_gvsp);
	}

	firstbyte = tvb_get_guint8(tvb, offset);
	if (firstbyte == 0x42)
	{
		/* Add packet format to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "GVCP Generic Packet (Firewall)");

		/* GVCP magic byte, must be a firewall opener packet */
		return;
	}

	/* Get packet format byte, look for enhanced flag and then clear it */
	info.format = tvb_get_guint8(tvb, offset + 4);
	info.formatstring = val_to_str(info.format, formatnames, "Unknown Format");
	info.enhanced = info.format & 0x80;
	info.format &= 0x7F;

	/* Add packet format to info string */
	col_append_fstr(pinfo->cinfo, COL_INFO, "%s ", info.formatstring);

	/* Dissect status */
	if (tree != NULL)
	{
		proto_tree_add_item(gvsp_tree, hf_gvsp_status, tvb, offset, 2, ENC_BIG_ENDIAN);
	}
	offset += 2;

	if (info.enhanced == 0)
	{
		info.blockid = tvb_get_ntohs(tvb, offset);

		/* Dissect block id 16 bits */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_blockid16, tvb, offset, 2, ENC_BIG_ENDIAN);
		}
		offset += 2;
	}
	else
	{
		/* Dissect flags */
		if (tree != NULL)
		{
			guint8 flags;
			flags = tvb_get_guint8(tvb, offset + 1);
			info.flag_resendrangeerror = flags & 0x04;
			info.flag_previousblockdropped = flags & 0x02;
			info.flag_packetresend = flags & 0x01;

			item = proto_tree_add_item(gvsp_tree, hf_gvsp_flags, tvb, offset, 2, ENC_BIG_ENDIAN);
			ti = proto_item_add_subtree(item, ett_gvsp_flags);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific0, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific1, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific2, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific3, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific4, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific5, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific6, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific7, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagresendrangeerror, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagpreviousblockdropped, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagpacketresend, tvb, offset, 2, ENC_BIG_ENDIAN);
		}
		offset += 2;
	}

	/* Dissect packet format */
	if (tree != NULL)
	{
		proto_tree_add_item(gvsp_tree, hf_gvsp_format, tvb, offset, 1, ENC_BIG_ENDIAN);
	}
	offset++;

	if (info.enhanced == 0)
	{
		info.packetid = tvb_get_ntohl(tvb, offset - 1);
		info.packetid &= 0x00FFFFFF;

		/* Dissect packet ID */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_packetid24, tvb, offset, 3, ENC_BIG_ENDIAN);
		}
		offset += 3;
	}
	else
	{
		/* Nothing... */
		offset += 3;
	}

	if (info.enhanced != 0)
	{
		guint64 high;
		guint64 low;
		high = tvb_get_ntohl(tvb, offset);
		low = tvb_get_ntohl(tvb, offset + 4);
		info.blockid = low | (high << 32);

		/* Dissect 64 bit block ID */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_blockid64, tvb, offset, 8, ENC_BIG_ENDIAN);
		}
		offset += 8;

		info.packetid = tvb_get_ntohl(tvb, offset);

		/* Dissect 32 bit packet ID */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_packetid32, tvb, offset, 4, ENC_BIG_ENDIAN);
		}
		offset += 4;
	}

	col_append_fstr(pinfo->cinfo, COL_INFO, "[Block ID: %" G_GINT64_MODIFIER "u Packet ID: %d] ", (guint64)info.blockid, info.packetid);

	if (info.flag_resendrangeerror != 0)
	{
		/* Add range error to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "[RANGE_ERROR] ");
	}

	if (info.flag_previousblockdropped != 0)
	{
		/* Add block dropped to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "[BLOCK_DROPPED] ");
	}

	if (info.flag_packetresend != 0)
	{
		/* Add packet resend to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "[PACKET_RESEND] ");
	}

	/* Process packet types that are payload agnostic */
	switch (info.format)
	{
	case GVSP_PACKET_PAYLOAD:
		DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
		return;

	case GVSP_PACKET_PAYLOAD_H264:
		DissectPacketPayloadH264(gvsp_tree, tvb, pinfo, offset);
		return;

	case GVSP_PACKET_PAYLOAD_MULTIZONE:
		DissectPacketPayloadMultizone(gvsp_tree, tvb, pinfo, offset);
		return;

	default:
		break;
	}

	/* Get payload type, clear chunk bit */
	info.payloadtype = tvb_get_ntohs(tvb, offset + 2);
	info.payloadstring = val_to_str(info.payloadtype, payloadtypenames, "Unknown Payload Type");
	info.chunk = info.payloadtype & 0x4000;
	info.payloadtype &= 0x3FFF;

	/* Add payload type to information string */
	col_append_fstr(pinfo->cinfo, COL_INFO, "%s ", info.payloadstring);

	/* Process packet types for specific payload types */
	switch (info.format)
	{
	case GVSP_PACKET_ALLIN:
		DissectPacketAllIn(gvsp_tree, tvb, offset, pinfo, &info);
		return;

	case GVSP_PACKET_LEADER:
		DissectPacketLeader(gvsp_tree, tvb, offset, pinfo, &info);
		return;

	case GVSP_PACKET_TRAILER:
		DissectPacketTrailer(gvsp_tree, tvb, offset, pinfo, &info);
		return;

	default:
		break;
	}
}


/*
    \brief Registers the dissector. Invoked when the pluging library is loaded.
 */

void proto_register_gvsp(void) 
{
    static hf_register_info hfgvsp[] = 
    {
        {& hf_gvsp_status,          
        { "Status", "gvsp.status",
        FT_UINT16, BASE_HEX, VALS(statusnames), 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_blockid16,           
        { "Block ID (16 bits)", "gvsp.blockid16",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_flags,           
        { "Flags", "gvsp.flags",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific0,         
        { "Flag Device Specific 0", "gvsp.flag.devicespecific0",
        FT_UINT16, BASE_HEX, NULL, 0x8000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific1,         
        { "Flag Device Specific 1", "gvsp.flag.devicespecific1",
        FT_UINT16, BASE_HEX, NULL, 0x4000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific2,         
        { "Flag Device Specific 2", "gvsp.flag.devicespecific2",
        FT_UINT16, BASE_HEX, NULL, 0x2000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific3,         
        { "Flag Device Specific 3", "gvsp.flag.devicespecific3",
        FT_UINT16, BASE_HEX, NULL, 0x1000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific4,         
        { "Flag Device Specific 4", "gvsp.flag.devicespecific4",
        FT_UINT16, BASE_HEX, NULL, 0x0800,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific5,         
        { "Flag Device Specific 5", "gvsp.flag.devicespecific5",
        FT_UINT16, BASE_HEX, NULL, 0x0400,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific6,         
        { "Flag Device Specific 6", "gvsp.flag.devicespecific6",
        FT_UINT16, BASE_HEX, NULL, 0x0200,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific7,         
        { "Flag Device Specific 7", "gvsp.flag.devicespecific7",
        FT_UINT16, BASE_HEX, NULL, 0x0100,
        NULL, HFILL
        }},

        {& hf_gvsp_flagresendrangeerror,            
        { "Flag Resend Range Error 7", "gvsp.flag.resendrangeerror",
        FT_UINT16, BASE_HEX, NULL, 0x0004,
        NULL, HFILL
        }},

        {& hf_gvsp_flagpreviousblockdropped,            
        { "Flag Previous Block Dropped", "gvsp.flag.previousblockdropped",
        FT_UINT16, BASE_HEX, NULL, 0x0002,
        NULL, HFILL
        }},

        {& hf_gvsp_flagpacketresend,            
        { "Flag Packet Resend", "gvsp.flag.packetresend",
        FT_UINT16, BASE_HEX, NULL, 0x0001,
        NULL, HFILL
        }},

        {& hf_gvsp_format,          
        { "Format", "gvsp.format",
        FT_UINT8, BASE_HEX, VALS(formatnames), 0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetid24,          
        { "Packet ID (24 bits)", "gvsp.packetid24",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_blockid64,           
        { "Block ID (64 bits v2.0)", "gvsp.blockid64",
        FT_UINT64, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetid32,          
        { "Packet ID (32 bits v2.0)", "gvsp.packetid32",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_payloadtype,         
        { "Payload Type", "gvsp.payloadtype",
        FT_UINT16, BASE_HEX, VALS(payloadtypenames), 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_payloaddata,         
        { "Payload Data", "gvsp.payloaddata",
        FT_BYTES, BASE_NONE, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_timestamp,           
        { "Timestamp", "gvsp.timestamp",
        FT_UINT64, BASE_HEX, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_pixelformat,         
        { "Pixel Format", "gvsp.pixel",
        FT_UINT32, BASE_HEX, VALS(pixeltypenames), 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_sizex,           
        { "Size X", "gvsp.sizex",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_sizey,           
        { "Size Y", "gvsp.sizey",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_offsetx,         
        { "Offset X", "gvsp.offsetx",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_offsety,         
        { "Offset Y", "gvsp.offsety",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_paddingx,            
        { "Padding X", "gvsp.paddingx",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_paddingy,            
        { "Padding Y", "gvsp.paddingy",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_payloaddatasize,         
        { "Payload Data Size", "gvsp.payloaddatasize",
        FT_UINT64, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_pixelcolor,          
        { "Monochrome or Color", "gvsp.pixel.color",
        FT_UINT32, BASE_HEX, VALS(colornames), 0xFF000000,
        NULL, HFILL
        }},

        {& hf_gvsp_pixeloccupy,         
        { "Occupy Bits", "gvsp.pixel.occupy",
        FT_UINT32, BASE_DEC, NULL, 0x00FF0000,
        NULL, HFILL
        }},

        {& hf_gvsp_pixelid,         
        { "ID", "gvsp.pixel.id",
        FT_UINT32, BASE_HEX, NULL, 0x0000FFFF,
        NULL, HFILL
        }},

        {& hf_gvsp_filename,            
        { "ID", "gvsp.filename",
        FT_STRINGZ, BASE_NONE, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_payloadlength,           
        { "Payload Length", "gvsp.payloadlength",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_fieldinfo,           
        { "Field Info", "gvsp.fieldinfo",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_fieldid,         
        { "Field ID", "gvsp.fieldid",
        FT_UINT8, BASE_HEX, NULL, 0xF0,
        NULL, HFILL
        }},

        {& hf_gvsp_fieldcount,          
        { "Field Count", "gvsp.fieldcount",
        FT_UINT8, BASE_HEX, NULL, 0x0F,
        NULL, HFILL
        }},

        {& hf_gvsp_genericflags,            
        { "Generic Flag", "gvsp.genericflag",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_timestamptickfrequency ,         
        { "Timestamp Tick Frequency", "gvsp.timestamptickfrequency",
        FT_UINT64, BASE_HEX, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_dataformat,          
        { "Data Format", "gvsp.dataformat",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetizationmode,           
        { "packetization_mode", "gvsp.packetizationmode",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetsize,          
        { "packet_size", "gvsp.packetsize",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_profileidc,          
        { "profile_idc", "gvsp.profileidc",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_cs,          
        { "cs", "gvsp.cs",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_cs0,         
        { "cs0", "gvsp.cs0",
        FT_UINT8, BASE_HEX, NULL, 0x80,
        NULL, HFILL
        }},

        {& hf_gvsp_cs1,         
        { "cs1", "gvsp.cs1",
        FT_UINT8, BASE_HEX, NULL, 0x40,
        NULL, HFILL
        }},

        {& hf_gvsp_cs2,         
        { "cs2", "gvsp.cs2",
        FT_UINT8, BASE_HEX, NULL, 0x20,
        NULL, HFILL
        }},

        {& hf_gvsp_cs3,         
        { "cs3", "gvsp.cs3",
        FT_UINT8, BASE_HEX, NULL, 0x10,
        NULL, HFILL
        }},

        {& hf_gvsp_levelidc,            
        { "level_idc", "gvsp.levelidc",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropinterleavingdepth,           
        { "srop_interlaving_depth", "gvsp.sropinterleavingdepth",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropmaxdondiff,          
        { "srop_max_don_diff", "gvsp.sropmaxdondiff",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropdeintbufreq,         
        { "srop_deint_buf_req", "gvsp.sropdeintbufreq",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropinitbuftime,         
        { "srop_init_buf_time", "gvsp.sropinitbuftime",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_zoneinfo,            
        { "Zone Info", "gvsp.zoneinfo",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_zoneid,          
        { "Zone ID", "gvsp.zoneid",
        FT_UINT8, BASE_HEX, NULL, 0x3E,
        NULL, HFILL
        }},

        {& hf_gvsp_endofzone,           
        { "End of Zone", "gvsp.endofzone",
        FT_UINT8, BASE_HEX, NULL, 0x01,
        NULL, HFILL
        }},

        {& hf_gvsp_addressoffsethigh,           
        { "Address Offset High", "gvsp.addressoffsethigh",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_addressoffsetlow,            
        { "Address Offset Low", "gvsp.addressoffsetlow",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sc_zone_direction,   
        { "Zone Directions Mask", "gvcpzonedirection",
        FT_BOOLEAN, 32, TFS(&directionnames), 0,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone0_direction,  
        { "Zone 0 Direction", "gvcp.zone0direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x80000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone1_direction,  
        { "Zone 1 Direction", "gvcp.zone1direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x40000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone2_direction,  
        { "Zone 2 Direction", "gvcp.zone2direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x20000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone3_direction,  
        { "Zone 3 Direction", "gvcp.zone3direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x10000000,
        NULL, HFILL
        }}, 
        
        {& hf_gvsp_sc_zone4_direction,  
        { "Zone 4 Direction", "gvcp.zone4direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x08000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone5_direction,  
        { "Zone 5 Direction", "gvcp.zone5direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x04000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone6_direction,  
        { "Zone 6 Direction", "gvcp.zone6direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x02000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone7_direction,  
        { "Zone 7 Direction", "gvcp.zone7direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x01000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone8_direction,  
        { "Zone 8 Direction", "gvcp.zone8direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00800000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone9_direction,  
        { "Zone 9 Direction", "gvcp.zone9direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00400000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone10_direction, 
        { "Zone 10 Direction", "gvcp.zone10direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00200000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone11_direction, 
        { "Zone 11 Direction", "gvcp.zone1direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00100000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone12_direction, 
        { "Zone 12 Direction", "gvcp.zone12direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00080000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone13_direction, 
        { "Zone 13 Direction", "gvcp.zone13direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00040000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone14_direction, 
        { "Zone 14 Direction", "gvcp.zone14direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00020000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone15_direction, 
        { "Zone 15 Direction", "gvcp.zone15direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00010000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone16_direction, 
        { "Zone 16 Direction", "gvcp.zone16direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00008000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone17_direction, 
        { "Zone 17 Direction", "gvcp.zone17direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00004000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone18_direction, 
        { "Zone 18 Direction", "gvcp.zone18direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00002000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone19_direction, 
        { "Zone 19 Direction", "gvcp.zone19direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00001000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone20_direction, 
        { "Zone 20 Direction", "gvcp.zone20direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000800,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone21_direction, 
        { "Zone 21 Direction", "gvcp.zone21direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000400,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone22_direction, 
        { "Zone 22 Direction", "gvcp.zone22direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000200,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone23_direction, 
        { "Zone 23 Direction", "gvcp.zone23direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000100,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone24_direction, 
        { "Zone 24 Direction", "gvcp.zone24direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000080,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone25_direction, 
        { "Zone 25 Direction", "gvcp.zone25direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000040,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone26_direction, 
        { "Zone 26 Direction", "gvcp.zone26direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000020,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone27_direction, 
        { "Zone 27 Direction", "gvcp.zone27direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000010,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone28_direction, 
        { "Zone 28 Direction", "gvcp.zone28direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000008,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone29_direction, 
        { "Zone 29 Direction", "gvcp.zone29direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000004,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone30_direction, 
        { "Zone 30 Direction", "gvcp.zone30direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000002,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone31_direction, 
        { "Zone 31 Direction", "gvcp.zone31direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000001,
        NULL, HFILL
        }}, 

        {& hf_gvsp_chunkdatapayloadlengthex,            
        { "Chunk Data Payload Length", "gvsp.chunkdatapayloadlengthex",
        FT_UINT32, BASE_HEX_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_chunklayoutidex,         
        { "Chunk Layout ID", "gvsp.chunklayoutidex",
        FT_UINT32, BASE_HEX_DEC, NULL, 0x0,
        NULL, HFILL
        }},
    };

    static gint *ett[] = {
        &ett_gvsp,
		&ett_gvsp_flags,
		&ett_gvsp_header,
		&ett_gvsp_payload,
		&ett_gvsp_trailer,
    };

    proto_gvsp = proto_register_protocol("GigE Vision Streaming Protocol", "GVSP", "gvsp");

    register_dissector("gvsp", dissect_gvsp, proto_gvsp);

    proto_register_field_array(proto_gvsp, hfgvsp, array_length(hfgvsp));
    proto_register_subtree_array(ett, array_length(ett));
}


/*
    \brief Rules for GVSP packets identification (?)
 */

void proto_reg_handoff_gvsp(void) 
{
    static int gvsp_initialized = FALSE;
    static dissector_handle_t gvsp_handle;

    if (!gvsp_initialized) 
	{
        gvsp_handle = new_create_dissector_handle((new_dissector_t)dissect_gvsp, proto_gvsp);
        dissector_add_uint("udp.port", global_gvsp_port, gvsp_handle);
        gvsp_initialized = TRUE;
    } 
	else 
	{
        dissector_delete_uint("udp.port", global_gvsp_port, gvsp_handle);
    }
}

/* packet-gvsp.c
 * Routines for AIA GigE Vision (TM) Streaming Protocol dissection
 * Copyright 2012, AIA <www.visiononline.org> All rights reserved
 *
 * GigE Vision (TM): GigE Vision a standard developed under the sponsorship of the AIA for 
 * the benefit of the machine vision industry. GVSP stands for GigE Vision (TM) Streaming
 * Protocol.
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*#ifdef HAVE_CONFIG_H */
#include "config.h"
/*#endif */

#ifdef PACKAGE
#undef PACKAGE
#endif

#define PACKAGE "gvsp"

#ifdef VERSION
#undef VERSION
#endif

#define VERSION "0.0.1"

#include <epan/packet.h>


/*
  Payload types
 */

#define GVSP_PAYLOAD_IMAGE ( 0x0001 )
#define GVSP_PAYLOAD_RAWDATA (0x0002 )
#define GVSP_PAYLOAD_FILE ( 0x0003 )
#define GVSP_PAYLOAD_CHUNKDATA ( 0x0004 )
#define GVSP_PAYLOAD_EXTENDEDCHUNKDATA ( 0x0005 ) /* Deprecated */
#define GVSP_PAYLOAD_JPEG ( 0x0006 )
#define GVSP_PAYLOAD_JPEG2000 ( 0x0007 )
#define GVSP_PAYLOAD_H264 ( 0x0008 )
#define GVSP_PAYLOAD_MULTIZONEIMAGE ( 0x0009 )
#define GVSP_PAYLOAD_DEVICEPSECIFICSTART ( 0x8000 )


/*
   GVSP packet types
 */

#define GVSP_PACKET_LEADER ( 1 )
#define GVSP_PACKET_TRAILER ( 2 )
#define GVSP_PACKET_PAYLOAD ( 3 )
#define GVSP_PACKET_ALLIN ( 4 )
#define GVSP_PACKET_PAYLOAD_H264 ( 5 )
#define GVSP_PACKET_PAYLOAD_MULTIZONE ( 6 )


/*
   GVSP statuses
 */

#define GEV_STATUS_SUCCESS (0x0000) 
#define GEV_STATUS_PACKET_RESEND (0x0100) 
#define GEV_STATUS_NOT_IMPLEMENTED (0x8001) 
#define GEV_STATUS_INVALID_PARAMETER (0x8002) 
#define GEV_STATUS_INVALID_ADDRESS (0x8003) 
#define GEV_STATUS_WRITE_PROTECT (0x8004) 
#define GEV_STATUS_BAD_ALIGNMENT (0x8005) 
#define GEV_STATUS_ACCESS_DENIED (0x8006) 
#define GEV_STATUS_BUSY (0x8007) 
#define GEV_STATUS_LOCAL_PROBLEM (0x8008)  /* deprecated */
#define GEV_STATUS_MSG_MISMATCH (0x8009) /* deprecated */
#define GEV_STATUS_INVALID_PROTOCOL (0x800A) /* deprecated */
#define GEV_STATUS_NO_MSG (0x800B) /* deprecated */
#define GEV_STATUS_PACKET_UNAVAILABLE (0x800C) 
#define GEV_STATUS_DATA_OVERRUN (0x800D) 
#define GEV_STATUS_INVALID_HEADER (0x800E) 
#define GEV_STATUS_WRONG_CONFIG (0x800F) /* deprecated */
#define GEV_STATUS_PACKET_NOT_YET_AVAILABLE (0x8010) 
#define GEV_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY (0x8011) 
#define GEV_STATUS_PACKET_REMOVED_FROM_MEMORY (0x8012) 
#define GEV_STATUS_ERROR (0x8FFF) 


/*
   Pixel type color
 */

#define GVSP_PIX_MONO             (0x01000000)
#define GVSP_PIX_RGB              (0x02000000)
#define GVSP_PIX_COLOR            (0x02000000)
#define GVSP_PIX_CUSTOM           (0x80000000)
#define GVSP_PIX_COLOR_MASK       (0xFF000000)


/*
   Pixel type size
 */

#define GVSP_PIX_OCCUPY1BIT       (0x00010000)
#define GVSP_PIX_OCCUPY2BIT       (0x00020000)
#define GVSP_PIX_OCCUPY4BIT       (0x00040000)
#define GVSP_PIX_OCCUPY8BIT       (0x00080000)
#define GVSP_PIX_OCCUPY12BIT      (0x000C0000)
#define GVSP_PIX_OCCUPY16BIT      (0x00100000)
#define GVSP_PIX_OCCUPY24BIT      (0x00180000)
#define GVSP_PIX_OCCUPY32BIT      (0x00200000)
#define GVSP_PIX_OCCUPY36BIT      (0x00240000)
#define GVSP_PIX_OCCUPY48BIT      (0x00300000)


/*
   Pxiel type masks, shifts 
 */
#define GVSP_PIX_EFFECTIVE_PIXEL_SIZE_MASK (0x00FF0000)
#define GVSP_PIX_EFFECTIVE_PIXEL_SIZE_SHIFT (16)

#define GVSP_PIX_ID_MASK (0x0000FFFF)


/*
   Pixel types
 */

#define GVSP_PIX_COUNT (0x46)

#define GVSP_PIX_MONO1P                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY1BIT  | 0x0037)
#define GVSP_PIX_MONO2P                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY2BIT  | 0x0038)
#define GVSP_PIX_MONO4P                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY4BIT  | 0x0039)
#define GVSP_PIX_MONO8                   (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0001)
#define GVSP_PIX_MONO8S                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0002)
#define GVSP_PIX_MONO10                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0003)
#define GVSP_PIX_MONO10_PACKED           (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0004)
#define GVSP_PIX_MONO12                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0005)
#define GVSP_PIX_MONO12_PACKED           (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0006)
#define GVSP_PIX_MONO14                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0025)
#define GVSP_PIX_MONO16                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0007)
											    
#define GVSP_PIX_BAYGR8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0008)
#define GVSP_PIX_BAYRG8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x0009)
#define GVSP_PIX_BAYGB8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x000A)
#define GVSP_PIX_BAYBG8                  (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY8BIT  | 0x000B)
#define GVSP_PIX_BAYGR10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000C)
#define GVSP_PIX_BAYRG10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000D)
#define GVSP_PIX_BAYGB10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000E)
#define GVSP_PIX_BAYBG10                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x000F)
#define GVSP_PIX_BAYGR12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0010)
#define GVSP_PIX_BAYRG12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0011)
#define GVSP_PIX_BAYGB12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0012)
#define GVSP_PIX_BAYBG12                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0013)
#define GVSP_PIX_BAYGR10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0026)
#define GVSP_PIX_BAYRG10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0027)
#define GVSP_PIX_BAYGB10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0028)
#define GVSP_PIX_BAYBG10_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x0029)
#define GVSP_PIX_BAYGR12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002A)
#define GVSP_PIX_BAYRG12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002B)
#define GVSP_PIX_BAYGB12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002C)
#define GVSP_PIX_BAYBG12_PACKED          (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY12BIT | 0x002D)
#define GVSP_PIX_BAYGR16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x002E)
#define GVSP_PIX_BAYRG16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x002F)
#define GVSP_PIX_BAYGB16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0030)
#define GVSP_PIX_BAYBG16                 (GVSP_PIX_MONO  | GVSP_PIX_OCCUPY16BIT | 0x0031)
								         
#define GVSP_PIX_RGB8                    (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0014)
#define GVSP_PIX_BGR8                    (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0015)
#define GVSP_PIX_RGBA8                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x0016)
#define GVSP_PIX_BGRA8                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x0017)
#define GVSP_PIX_RGB10                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0018)
#define GVSP_PIX_BGR10                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0019)
#define GVSP_PIX_RGB12                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x001A)
#define GVSP_PIX_BGR12                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x001B)
#define GVSP_PIX_RGB16                   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0033)
#define GVSP_PIX_RGB10V1_PACKED          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x001C)
#define GVSP_PIX_RGB10P32                (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY32BIT | 0x001D)
#define GVSP_PIX_RGB12V1_PACKED          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY36BIT | 0x0034)
#define GVSP_PIX_RGB565P                 (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0035)
#define GVSP_PIX_BGR565P                 (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0036)

#define GVSP_PIX_YUV411_8_UYYVYY         (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x001E)
#define GVSP_PIX_YUV422_8_UYVY           (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x001F)
#define GVSP_PIX_YUV422_8                (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0032)
#define GVSP_PIX_YUV8_UYV                (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0020)
#define GVSP_PIX_YCBCR8_CBYCR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x003A)
#define GVSP_PIX_YCBCR422_8              (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x003B)
#define GVSP_PIX_YCBCR422_8_CBYCRY       (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0043)
#define GVSP_PIX_YCBCR422_8_CBYYCRYY     (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x003C)
#define GVSP_PIX_YCBCR601_8_CBYCR        (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x003D)
#define GVSP_PIX_YCBCR601_422_8          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x003E)
#define GVSP_PIX_YCBCR601_422_8_CBYCRY   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0044)
#define GVSP_PIX_YCBCR601_411_8_CBYYCRYY (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x003F)
#define GVSP_PIX_YCBCR709_411_8_CBYCR    (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0040)
#define GVSP_PIX_YCBCR709_422_8          (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0041)
#define GVSP_PIX_YCBCR709_422_8_CBYCRY   (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY16BIT | 0x0045)
#define GVSP_PIX_YCBCR709_411_8_CBYYCRYY (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY12BIT | 0x0042)

#define GVSP_PIX_RGB8_PLANAR             (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY24BIT | 0x0021)
#define GVSP_PIX_RGB10_PLANAR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0022)
#define GVSP_PIX_RGB12_PLANAR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0023)
#define GVSP_PIX_RGB16_PLANAR            (GVSP_PIX_COLOR | GVSP_PIX_OCCUPY48BIT | 0x0024)


typedef struct _display_string 
{
	int display;
	gchar* string_display;

} display_string;


/* Structure to hold GVSP packet information */
typedef struct _gvsp_packet_info
{
	gint chunk;
	guint8 format;
	guint16 payloadtype;
	guint64 blockid;
	guint32 packetid;
	gint enhanced;
	gint flag_resendrangeerror;
	gint flag_previousblockdropped;
	gint flag_packetresend;

	const char *formatstring;
	const char *payloadstring;

} gvsp_packet_info;


/*Define the gvsp proto */
static int proto_gvsp = -1;
static int global_gvsp_port = 20202;


/* Define the tree for gvsp */
static int ett_gvsp = -1;
static int ett_gvsp_flags = -1;
static int ett_gvsp_header = -1;
static int ett_gvsp_payload = -1;
static int ett_gvsp_trailer = -1;


typedef struct _ftenum_string {
	enum ftenum type;
	gchar* string_type;
} ftenum_string;

static const value_string statusnames[] = {
    { GEV_STATUS_SUCCESS, "GEV_STATUS_SUCCESS" },
    { GEV_STATUS_PACKET_RESEND, "GEV_STATUS_PACKET_RESEND" },
    { GEV_STATUS_NOT_IMPLEMENTED, "GEV_STATUS_NOT_IMPLEMENTED" },
    { GEV_STATUS_INVALID_PARAMETER, "GEV_STATUS_INVALID_PARAMETER" },
    { GEV_STATUS_INVALID_ADDRESS, "GEV_STATUS_INVALID_ADDRESS" },
    { GEV_STATUS_WRITE_PROTECT, "GEV_STATUS_WRITE_PROTECT" },
    { GEV_STATUS_BAD_ALIGNMENT, "GEV_STATUS_BAD_ALIGNMENT" },
    { GEV_STATUS_ACCESS_DENIED, "GEV_STATUS_ACCESS_DENIED" },
    { GEV_STATUS_BUSY, "GEV_STATUS_BUSY" },
    { GEV_STATUS_LOCAL_PROBLEM, "GEV_STATUS_LOCAL_PROBLEM (deprecated)" },
    { GEV_STATUS_MSG_MISMATCH, "GEV_STATUS_MSG_MISMATCH (deprecated)" },
    { GEV_STATUS_INVALID_PROTOCOL, "GEV_STATUS_INVALID_PROTOCOL (deprecated)" },
    { GEV_STATUS_NO_MSG, "GEV_STATUS_NO_MSG (deprecated)" },
    { GEV_STATUS_PACKET_UNAVAILABLE, "GEV_STATUS_PACKET_UNAVAILABLE" },
    { GEV_STATUS_DATA_OVERRUN, "GEV_STATUS_DATA_OVERRUN" },
    { GEV_STATUS_INVALID_HEADER, "GEV_STATUS_INVALID_HEADER" },
    { GEV_STATUS_WRONG_CONFIG, "GEV_STATUS_WRONG_CONFIG (deprecated)" },
    { GEV_STATUS_PACKET_NOT_YET_AVAILABLE, "GEV_STATUS_PACKET_NOT_YET_AVAILABLE" },
    { GEV_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY, "GEV_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY" },
    { GEV_STATUS_PACKET_REMOVED_FROM_MEMORY, "GEV_STATUS_PACKET_REMOVED_FROM_MEMORY" },
    { GEV_STATUS_ERROR, "GEV_STATUS_ERROR" },
    { 0, NULL },
};

static const value_string formatnames[] = {
    { GVSP_PACKET_LEADER, "LEADER" },
    { GVSP_PACKET_TRAILER, "TRAILER" },
    { GVSP_PACKET_PAYLOAD, "PAYLOAD" },
    { GVSP_PACKET_ALLIN, "ALLIN" },
    { GVSP_PACKET_PAYLOAD_H264, "H264" },
    { GVSP_PACKET_PAYLOAD_MULTIZONE, "MULTIZONE" },
    { 0x80 | GVSP_PACKET_LEADER, "LEADER (ext IDs)" },
    { 0x80 | GVSP_PACKET_TRAILER, "TRAILER (ext IDs)" },
    { 0x80 | GVSP_PACKET_PAYLOAD, "PAYLOAD (ext IDs)" },
    { 0x80 | GVSP_PACKET_ALLIN, "ALLIN (ext IDs)" },
    { 0x80 | GVSP_PACKET_PAYLOAD_H264, "H264 (ext IDs)" },
    { 0x80 | GVSP_PACKET_PAYLOAD_MULTIZONE, "MULTIZONE (ext IDs)" },
    { 0, NULL },
};

static const value_string payloadtypenames[] = {
    { GVSP_PAYLOAD_IMAGE, "IMAGE" },
    { GVSP_PAYLOAD_RAWDATA, "RAWDATA" },
    { GVSP_PAYLOAD_FILE, "FILE" },
    { GVSP_PAYLOAD_CHUNKDATA, "CHUNKDATA" },
    { GVSP_PAYLOAD_EXTENDEDCHUNKDATA, "EXTENDEDCHUNKDATA (obsolete with v2.0)" },
    { GVSP_PAYLOAD_JPEG, "JPEG" },
    { GVSP_PAYLOAD_JPEG2000, "JPEG2000" },
    { GVSP_PAYLOAD_H264, "H264" },
    { GVSP_PAYLOAD_MULTIZONEIMAGE, "MUTLIZONEIAMGE" },
    { 0x4000 | GVSP_PAYLOAD_IMAGE, "IMAGE (v2.0 chunks)" },
    { 0x4000 | GVSP_PAYLOAD_RAWDATA, "RAWDATA (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_FILE, "FILE (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_CHUNKDATA, "CHUNKDATA (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_EXTENDEDCHUNKDATA, "EXTENDEDCHUNKDATA (v2.0 chunks?)" },
    { 0x4000 | GVSP_PAYLOAD_JPEG, "JPEG (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_JPEG2000, "JPEG2000 (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_H264, "H264 (v2.0 Chunks)" },
    { 0x4000 | GVSP_PAYLOAD_MULTIZONEIMAGE, "MULTIZONEIMAGE (v2.0 Chunks)" },
    { 0, NULL },
};

static const value_string pixeltypenames[] = {
	{ GVSP_PIX_MONO1P, "GVSP_PIX_MONO1P" },                  
    { GVSP_PIX_MONO2P, "GVSP_PIX_MONO2P" },                  
    { GVSP_PIX_MONO4P, "GVSP_PIX_MONO4P" },
    { GVSP_PIX_MONO8, "GVSP_PIX_MONO8" },                   
    { GVSP_PIX_MONO8S, "GVSP_PIX_MONO8S" },                  
    { GVSP_PIX_MONO10, "GVSP_PIX_MONO10" },                  
    { GVSP_PIX_MONO10_PACKED, "GVSP_PIX_MONO10_PACKED" },           
    { GVSP_PIX_MONO12, "GVSP_PIX_MONO12" },                  
    { GVSP_PIX_MONO12_PACKED, "GVSP_PIX_MONO12_PACKED" },           
    { GVSP_PIX_MONO14, "GVSP_PIX_MONO14" },                  
    { GVSP_PIX_MONO16, "GVSP_PIX_MONO16" },                  
    { GVSP_PIX_BAYGR8, "GVSP_PIX_BAYGR8" },                  
    { GVSP_PIX_BAYRG8, "GVSP_PIX_BAYRG8" },                  
    { GVSP_PIX_BAYGB8, "GVSP_PIX_BAYGB8" },                  
    { GVSP_PIX_BAYBG8, "GVSP_PIX_BAYBG8" },                  
    { GVSP_PIX_BAYGR10, "GVSP_PIX_BAYGR10" },                 
    { GVSP_PIX_BAYRG10, "GVSP_PIX_BAYRG10" },                 
    { GVSP_PIX_BAYGB10, "GVSP_PIX_BAYGB10" },                 
    { GVSP_PIX_BAYBG10, "GVSP_PIX_BAYBG10" },                 
    { GVSP_PIX_BAYGR12, "GVSP_PIX_BAYGR12" },                 
    { GVSP_PIX_BAYRG12, "GVSP_PIX_BAYRG12" },                 
    { GVSP_PIX_BAYGB12, "GVSP_PIX_BAYGB12" },                 
    { GVSP_PIX_BAYBG12, "GVSP_PIX_BAYBG12" },                 
    { GVSP_PIX_BAYGR10_PACKED, "GVSP_PIX_BAYGR10_PACKED" },          
    { GVSP_PIX_BAYRG10_PACKED, "GVSP_PIX_BAYRG10_PACKED" },          
    { GVSP_PIX_BAYGB10_PACKED, "GVSP_PIX_BAYGB10_PACKED" },          
    { GVSP_PIX_BAYBG10_PACKED, "GVSP_PIX_BAYBG10_PACKED" },          
    { GVSP_PIX_BAYGR12_PACKED, "GVSP_PIX_BAYGR12_PACKED" },          
    { GVSP_PIX_BAYRG12_PACKED, "GVSP_PIX_BAYRG12_PACKED" },          
    { GVSP_PIX_BAYGB12_PACKED, "GVSP_PIX_BAYGB12_PACKED" },          
    { GVSP_PIX_BAYBG12_PACKED, "GVSP_PIX_BAYBG12_PACKED" },          
    { GVSP_PIX_BAYGR16, "GVSP_PIX_BAYGR16" },                 
    { GVSP_PIX_BAYRG16, "GVSP_PIX_BAYRG16" },                 
    { GVSP_PIX_BAYGB16, "GVSP_PIX_BAYGB16" },                 
    { GVSP_PIX_BAYBG16, "GVSP_PIX_BAYBG16" },                 
    { GVSP_PIX_RGB8, "GVSP_PIX_RGB8" },                    
    { GVSP_PIX_BGR8, "GVSP_PIX_BGR8" },                    
    { GVSP_PIX_RGBA8, "GVSP_PIX_RGBA8" },                   
    { GVSP_PIX_BGRA8, "GVSP_PIX_BGRA8" },                   
    { GVSP_PIX_RGB10, "GVSP_PIX_RGB10" },                   
    { GVSP_PIX_BGR10, "GVSP_PIX_BGR10" },                   
    { GVSP_PIX_RGB12, "GVSP_PIX_RGB12" },                   
    { GVSP_PIX_BGR12, "GVSP_PIX_BGR12" },                   
    { GVSP_PIX_RGB16, "GVSP_PIX_RGB16" },                   
    { GVSP_PIX_RGB10V1_PACKED, "GVSP_PIX_RGB10V1_PACKED" },          
    { GVSP_PIX_RGB10P32, "GVSP_PIX_RGB10P32" },                
    { GVSP_PIX_RGB12V1_PACKED, "GVSP_PIX_RGB12V1_PACKED" },          
    { GVSP_PIX_RGB565P, "GVSP_PIX_RGB565P" },                 
    { GVSP_PIX_BGR565P, "GVSP_PIX_BGR565P" },                 
    { GVSP_PIX_YUV411_8_UYYVYY, "GVSP_PIX_YUV411_8_UYYVYY" },         
    { GVSP_PIX_YUV422_8_UYVY, "GVSP_PIX_YUV422_8_UYVY" },           
    { GVSP_PIX_YUV422_8, "GVSP_PIX_YUV422_8" },                
    { GVSP_PIX_YUV8_UYV, "GVSP_PIX_YUV8_UYV" },                
    { GVSP_PIX_YCBCR8_CBYCR, "GVSP_PIX_YCBCR8_CBYCR" },            
    { GVSP_PIX_YCBCR422_8, "GVSP_PIX_YCBCR422_8" },              
    { GVSP_PIX_YCBCR422_8_CBYCRY, "GVSP_PIX_YCBCR422_8_CBYCRY" },       
    { GVSP_PIX_YCBCR422_8_CBYYCRYY, "GVSP_PIX_YCBCR422_8_CBYYCRYY" },     
    { GVSP_PIX_YCBCR601_8_CBYCR, "GVSP_PIX_YCBCR601_8_CBYCR" },        
    { GVSP_PIX_YCBCR601_422_8, "GVSP_PIX_YCBCR601_422_8" },          
    { GVSP_PIX_YCBCR601_422_8_CBYCRY, "GVSP_PIX_YCBCR601_422_8_CBYCRY" },   
    { GVSP_PIX_YCBCR601_411_8_CBYYCRYY, "GVSP_PIX_YCBCR601_411_8_CBYYCRYY" }, 
    { GVSP_PIX_YCBCR709_411_8_CBYCR, "GVSP_PIX_YCBCR709_411_8_CBYCR" },    
    { GVSP_PIX_YCBCR709_422_8, "GVSP_PIX_YCBCR709_422_8" },         
    { GVSP_PIX_YCBCR709_422_8, "GVSP_PIX_YCBCR709_422_8" },          
    { GVSP_PIX_YCBCR709_411_8_CBYYCRYY, "GVSP_PIX_YCBCR709_411_8_CBYYCRYY" }, 
    { GVSP_PIX_RGB8_PLANAR, "GVSP_PIX_RGB8_PLANAR" },             
    { GVSP_PIX_RGB10_PLANAR, "GVSP_PIX_RGB10_PLANAR" },            
    { GVSP_PIX_RGB12_PLANAR, "GVSP_PIX_RGB12_PLANAR" },            
    { GVSP_PIX_RGB16_PLANAR, "GVSP_PIX_RGB16_PLANAR" },            
    { 0, NULL },
};

static const value_string colornames[] = {
    { GVSP_PIX_MONO >> 24, "Mono" },
    { GVSP_PIX_COLOR >> 24, "Color" },
    { GVSP_PIX_CUSTOM >> 24, "Custom" },
    { 0, NULL },
};

static const true_false_string directionnames = {
	"Receiver",
	"Transmitter"
};


static int hf_gvsp_status = -1;
static int hf_gvsp_blockid16 = -1;
static int hf_gvsp_flags = -1;
static int hf_gvsp_flagdevicespecific0 = -1;
static int hf_gvsp_flagdevicespecific1 = -1;
static int hf_gvsp_flagdevicespecific2 = -1;
static int hf_gvsp_flagdevicespecific3 = -1;
static int hf_gvsp_flagdevicespecific4 = -1;
static int hf_gvsp_flagdevicespecific5 = -1;
static int hf_gvsp_flagdevicespecific6 = -1;
static int hf_gvsp_flagdevicespecific7 = -1;
static int hf_gvsp_flagresendrangeerror = -1;
static int hf_gvsp_flagpreviousblockdropped = -1;
static int hf_gvsp_flagpacketresend = -1;
static int hf_gvsp_format = -1;
static int hf_gvsp_packetid24 = -1;
static int hf_gvsp_blockid64 = -1;
static int hf_gvsp_packetid32 = -1;
static int hf_gvsp_payloadtype = -1;
static int hf_gvsp_payloaddata = -1;
static int hf_gvsp_timestamp = -1;
static int hf_gvsp_pixelformat = -1;
static int hf_gvsp_sizex = -1;
static int hf_gvsp_sizey = -1;
static int hf_gvsp_offsetx = -1;
static int hf_gvsp_offsety = -1;
static int hf_gvsp_paddingx = -1;
static int hf_gvsp_paddingy = -1;
static int hf_gvsp_payloaddatasize = -1;
static int hf_gvsp_pixelcolor = -1;
static int hf_gvsp_pixeloccupy = -1;
static int hf_gvsp_pixelid = -1;
static int hf_gvsp_filename = -1;
static int hf_gvsp_payloadlength = -1;
static int hf_gvsp_fieldinfo = -1;
static int hf_gvsp_fieldid = -1;
static int hf_gvsp_fieldcount = -1;
static int hf_gvsp_genericflags = -1;
static int hf_gvsp_chunklayoutid = -1;
static int hf_gvsp_timestamptickfrequency = -1;
static int hf_gvsp_dataformat = -1;
static int hf_gvsp_packetizationmode = -1;
static int hf_gvsp_packetsize = -1;
static int hf_gvsp_profileidc = -1;
static int hf_gvsp_cs = -1;
static int hf_gvsp_cs0 = -1;
static int hf_gvsp_cs1 = -1;
static int hf_gvsp_cs2 = -1;
static int hf_gvsp_cs3 = -1;
static int hf_gvsp_levelidc = -1;
static int hf_gvsp_sropinterleavingdepth = -1;
static int hf_gvsp_sropmaxdondiff = -1;
static int hf_gvsp_sropdeintbufreq = -1;
static int hf_gvsp_sropinitbuftime = -1;
static int hf_gvsp_zoneinfo = -1;
static int hf_gvsp_zoneid = -1;
static int hf_gvsp_endofzone = -1;
static int hf_gvsp_addressoffsethigh = -1;
static int hf_gvsp_addressoffsetlow = -1;
static int hf_gvsp_sc_zone_direction = -1;
static int hf_gvsp_sc_zone0_direction = -1;
static int hf_gvsp_sc_zone1_direction = -1;
static int hf_gvsp_sc_zone2_direction = -1;
static int hf_gvsp_sc_zone3_direction = -1;
static int hf_gvsp_sc_zone4_direction = -1;
static int hf_gvsp_sc_zone5_direction = -1;
static int hf_gvsp_sc_zone6_direction = -1;
static int hf_gvsp_sc_zone7_direction = -1;
static int hf_gvsp_sc_zone8_direction = -1;
static int hf_gvsp_sc_zone9_direction = -1;
static int hf_gvsp_sc_zone10_direction = -1;
static int hf_gvsp_sc_zone11_direction = -1;
static int hf_gvsp_sc_zone12_direction = -1;
static int hf_gvsp_sc_zone13_direction = -1;
static int hf_gvsp_sc_zone14_direction = -1;
static int hf_gvsp_sc_zone15_direction = -1;
static int hf_gvsp_sc_zone16_direction = -1;
static int hf_gvsp_sc_zone17_direction = -1;
static int hf_gvsp_sc_zone18_direction = -1;
static int hf_gvsp_sc_zone19_direction = -1;
static int hf_gvsp_sc_zone20_direction = -1;
static int hf_gvsp_sc_zone21_direction = -1;
static int hf_gvsp_sc_zone22_direction = -1;
static int hf_gvsp_sc_zone23_direction = -1;
static int hf_gvsp_sc_zone24_direction = -1;
static int hf_gvsp_sc_zone25_direction = -1;
static int hf_gvsp_sc_zone26_direction = -1;
static int hf_gvsp_sc_zone27_direction = -1;
static int hf_gvsp_sc_zone28_direction = -1;
static int hf_gvsp_sc_zone29_direction = -1;
static int hf_gvsp_sc_zone30_direction = -1;
static int hf_gvsp_sc_zone31_direction = -1;
static int hf_gvsp_chunkdatapayloadlengthex = -1;
static int hf_gvsp_chunklayoutidex = -1;


/*
    \brief Dissects the image leader
 */

gint DissectImageLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{	
	proto_item *item = NULL;
	proto_item *ti = NULL;
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Pixel format */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_pixelformat, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_pixelcolor, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixeloccupy, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixelid, tvb, offset + 12, 4, ENC_BIG_ENDIAN);

		/* Size X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizex, tvb, offset + 16, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 20, 4, ENC_BIG_ENDIAN);

		/* Offset X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsetx, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* Offset Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsety, tvb, offset + 28, 4, ENC_BIG_ENDIAN);

		/* Padding X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingx, tvb, offset + 32, 2, ENC_BIG_ENDIAN);

		/* Padding Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingy, tvb, offset + 34, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 36;
}


/*
    \brief Dissects the image trailer
 */

gint DissectImageTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{	
	gint offset;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 8;
}


/*
    \brief Dissects the raw data leader
 */

gint DissectRawDataLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 12, 8, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 20;
}


/*
    \brief Dissects a file leader
 */

gint DissectFileLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 12, 8, ENC_BIG_ENDIAN);

		/* Filename */
		proto_tree_add_item(gvsp_tree, hf_gvsp_filename, tvb, offset + 20, -1, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 20; /* Problem with the spec? All-in does not work if variable header size... */
}


/*
    \brief Dissects a chunk data leader
 */

gint DissectChunkDataLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 12;
}


/*
    \brief Dissects a chunk data trailer
 */

gint DissectChunkDataTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Payload data length */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadlength, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 8;
}


/*
    \brief Dissects extended chunk data leader
 */

gint DissectExtendedChunkDataLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;	
    proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Generic flags */
		proto_tree_add_item(gvsp_tree, hf_gvsp_genericflags, tvb, offset + 1, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Pixel format */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_pixelformat, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_pixelcolor, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixeloccupy, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixelid, tvb, offset + 12, 4, ENC_BIG_ENDIAN);

		/* Size X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizex, tvb, offset + 16, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 20, 4, ENC_BIG_ENDIAN);

		/* Offset X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsetx, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* Offset Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsety, tvb, offset + 28, 4, ENC_BIG_ENDIAN);

		/* Padding X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingx, tvb, offset + 32, 2, ENC_BIG_ENDIAN);

		/* Padding Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingy, tvb, offset + 34, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 36;
}


/*
    \brief Dissects extended chunk data trailer
 */

gint DissectExtendedChunkDataTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Payload data length */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadlength, tvb, offset + 4, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 8, 4, ENC_BIG_ENDIAN);

		/* Chunk layout ID */
		proto_tree_add_item(gvsp_tree, hf_gvsp_chunklayoutid, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 16;
}


/*
    \brief Dissects a JPEG leader
 */

gint DissectJPEGLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Generic flags */
		proto_tree_add_item(gvsp_tree, hf_gvsp_genericflags, tvb, offset + 1, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 12, 8, ENC_BIG_ENDIAN);

		/* Timestamp tick frequency */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamptickfrequency, tvb, offset + 20, 8, ENC_BIG_ENDIAN);

		/* Data format */
		proto_tree_add_item(gvsp_tree, hf_gvsp_dataformat, tvb, offset + 24, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 28;
}


/*
 \brief Dissects a H264 leader
 */

gint DissectH264Leader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Payload data size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddatasize, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* packetization_mode */
		proto_tree_add_item(gvsp_tree, hf_gvsp_packetizationmode, tvb, offset + 13, 1, ENC_BIG_ENDIAN);

		/* packet_size */
		proto_tree_add_item(gvsp_tree, hf_gvsp_packetsize, tvb, offset + 14, 2, ENC_BIG_ENDIAN);

		/* profile_idc */
		proto_tree_add_item(gvsp_tree, hf_gvsp_profileidc, tvb, offset + 17, 1, ENC_BIG_ENDIAN);

		/* cs0, 1, 2 ,3 */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_cs, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs0, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs1, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs2, tvb, offset + 18, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(gvsp_tree, hf_gvsp_cs3, tvb, offset + 18, 1, ENC_BIG_ENDIAN);

		/* level_idc */
		proto_tree_add_item(gvsp_tree, hf_gvsp_levelidc, tvb, offset + 19, 1, ENC_BIG_ENDIAN);

		/* srop_interleaving_depth */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropinterleavingdepth, tvb, offset + 20, 2, ENC_BIG_ENDIAN);

		/* srop_max_don_diff */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropmaxdondiff, tvb, offset + 22, 2, ENC_BIG_ENDIAN);

		/* srop_deint_buf_req */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropdeintbufreq, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* srop_init_buf_time */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sropinitbuftime, tvb, offset + 28, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 32;
}


/*
    \brief Dissects the multizone image leader
 */

gint DissectMultizoneImageLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
    proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Field info */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_fieldinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_fieldid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_fieldcount, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 4, 8, ENC_BIG_ENDIAN);

		/* Zone direction */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_sc_zone_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_sc_zone0_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone1_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone2_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone3_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone4_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone5_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone6_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone7_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone8_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone9_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone10_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone11_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone12_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone13_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone14_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone15_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone16_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone17_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone18_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone19_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone20_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone21_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone22_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone23_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone24_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone25_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone26_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone27_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone28_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone29_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone30_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_sc_zone31_direction, tvb, offset + 12, 4, ENC_BIG_ENDIAN);

		/* Pixel format */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_pixelformat, tvb, offset + 16, 4, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_pixelcolor, tvb, offset + 16, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixeloccupy, tvb, offset + 16, 4, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_pixelid, tvb, offset + 16, 4, ENC_BIG_ENDIAN);

		/* Size X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizex, tvb, offset + 20, 4, ENC_BIG_ENDIAN);

		/* Size Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_sizey, tvb, offset + 24, 4, ENC_BIG_ENDIAN);

		/* Offset X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsetx, tvb, offset + 28, 4, ENC_BIG_ENDIAN);

		/* Offset Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_offsety, tvb, offset + 32, 4, ENC_BIG_ENDIAN);

		/* Padding X */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingx, tvb, offset + 36, 2, ENC_BIG_ENDIAN);

		/* Padding Y */
		proto_tree_add_item(gvsp_tree, hf_gvsp_paddingy, tvb, offset + 38, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 40;
}


/*
    \brief Dissects a generic trailer (contains just the payload type)
 */

gint DissectGenericTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Payload type */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloadtype, tvb, offset + 2, 2, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 4;
}


/*
    \brief Dissects a generic trailer (contains just the payload type)
 */

gint DissectExtraChunkInfo(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Chunk data payload length */
		proto_tree_add_item(gvsp_tree, hf_gvsp_chunkdatapayloadlengthex, tvb, offset, 4, ENC_BIG_ENDIAN);

		/* Chunk layoud id */
		proto_tree_add_item(gvsp_tree, hf_gvsp_chunklayoutidex, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
	}

	/* Return dissected byte count (for all-in dissection) */
	return 8;
}


/*
    \brief Dissects a packet payload
 */

void DissectPacketPayload(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddata, tvb, offset, -1, ENC_BIG_ENDIAN);
	}
}


/*
    \brief Dissects a packet payload for H264
 */

void DissectPacketPayloadH264(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Timestamp */
		proto_tree_add_item(gvsp_tree, hf_gvsp_timestamp, tvb, offset + 0, 8, ENC_BIG_ENDIAN);

		/* Data */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddata, tvb, offset + 8, -1, ENC_BIG_ENDIAN);
	}
}


/*
    \brief Dissects a packet payload for multizone
 */

void DissectPacketPayloadMultizone(proto_tree *gvsp_tree, tvbuff_t *tvb, packet_info *pinfo _U_, gint startoffset)
{
	gint offset;
	proto_item *item = NULL;
    proto_item *ti = NULL;

	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		/* Zone information */
		item = proto_tree_add_item(gvsp_tree, hf_gvsp_zoneinfo, tvb, offset, 1, ENC_BIG_ENDIAN);
		ti = proto_item_add_subtree(item, ett_gvsp_flags);
		proto_tree_add_item(ti, hf_gvsp_zoneid, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(ti, hf_gvsp_endofzone, tvb, offset, 1, ENC_BIG_ENDIAN);

		/* Address offset high */
		proto_tree_add_item(gvsp_tree, hf_gvsp_addressoffsethigh, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

		/* Address offset low */
		proto_tree_add_item(gvsp_tree, hf_gvsp_addressoffsetlow, tvb, offset + 4, 4, ENC_BIG_ENDIAN);

		/* Data */
		proto_tree_add_item(gvsp_tree, hf_gvsp_payloaddata, tvb, offset + 8, -1, ENC_BIG_ENDIAN);
	}
}


/*
    \brief Dissects an all in packet
 */

void DissectPacketAllIn(proto_tree *gvsp_tree, tvbuff_t *tvb, gint startoffset, packet_info *pinfo, gvsp_packet_info *info)
{
	gint offset;
	offset = startoffset;

	if (gvsp_tree != NULL)
	{
		switch (info->payloadtype)
		{
		case GVSP_PAYLOAD_IMAGE:
			offset += DissectImageLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectImageTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_RAWDATA:
			offset += DissectRawDataLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_FILE:
			offset += DissectFileLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_CHUNKDATA:
			offset += DissectChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_EXTENDEDCHUNKDATA:
			offset += DissectExtendedChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectExtendedChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_JPEG:
		case GVSP_PAYLOAD_JPEG2000:
			offset += DissectJPEGLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_H264:
			offset += DissectH264Leader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayloadH264(gvsp_tree, tvb, pinfo, offset);
			break;

		case GVSP_PAYLOAD_MULTIZONEIMAGE:
			offset += DissectMultizoneImageLeader(gvsp_tree, tvb, pinfo, offset);
			offset += DissectImageTrailer(gvsp_tree, tvb, pinfo, offset);
			if (info->chunk != 0)
			{
				offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
			}
			DissectPacketPayloadMultizone(gvsp_tree, tvb, pinfo, offset);
			break;
		}
	}
}


/*
    \brief Dissects a leader packet
 */

void DissectPacketLeader(proto_tree *gvsp_tree, tvbuff_t *tvb, gint startoffset, packet_info *pinfo, gvsp_packet_info *info)
{
	gint offset;
	offset = startoffset;

	switch (info->payloadtype)
	{
	case GVSP_PAYLOAD_IMAGE:
		DissectImageLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_RAWDATA:
		DissectRawDataLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_FILE:
		DissectFileLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_CHUNKDATA:
		DissectChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_EXTENDEDCHUNKDATA:
		DissectExtendedChunkDataLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_JPEG:
	case GVSP_PAYLOAD_JPEG2000:
		DissectJPEGLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_H264:
		DissectH264Leader(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_MULTIZONEIMAGE:
		DissectMultizoneImageLeader(gvsp_tree, tvb, pinfo, offset);
		break;

	default:
		break;
	}
}


/*
    \brief Dissects a trailer packet
 */

void DissectPacketTrailer(proto_tree *gvsp_tree, tvbuff_t *tvb, gint startoffset, packet_info *pinfo, gvsp_packet_info *info)
{
	gint offset;
	offset = startoffset;

	switch (info->payloadtype)
	{
	case GVSP_PAYLOAD_IMAGE:
	case GVSP_PAYLOAD_MULTIZONEIMAGE:
		offset += DissectImageTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_CHUNKDATA:
		offset += DissectChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_EXTENDEDCHUNKDATA:
		offset += DissectExtendedChunkDataTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	case GVSP_PAYLOAD_RAWDATA:
	case GVSP_PAYLOAD_FILE:
	case GVSP_PAYLOAD_JPEG:
	case GVSP_PAYLOAD_JPEG2000:
	case GVSP_PAYLOAD_H264:
		offset += DissectGenericTrailer(gvsp_tree, tvb, pinfo, offset);
		break;

	default:
		break;
	}

	if (info->chunk != 0)
	{
		offset += DissectExtraChunkInfo(gvsp_tree, tvb, pinfo, offset);
	}
}


/*
    \brief Point of entry of all GVSP dissection
 */

static void dissect_gvsp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) 
{
    proto_item *ti = NULL;
    gint offset = 0;
	proto_tree *gvsp_tree = NULL;
	proto_item *item = NULL;
	gvsp_packet_info info;
	guint8 firstbyte = 0;

	memset(&info, 0x00, sizeof(info));

	/* Set the protocol column */
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "GVSP");

	/* Clear out stuff in the info column */
	col_clear(pinfo->cinfo, COL_INFO);

	/* Adds "Gigabit-Ethernet Streaming Protocol" heading to protocol tree */
	/* We will add fields to this using the gvsp_tree pointer */
	if (tree != NULL)
	{
		ti = proto_tree_add_item(tree, proto_gvsp, tvb, offset, -1, ENC_BIG_ENDIAN);
		gvsp_tree = proto_item_add_subtree(ti, ett_gvsp);
	}

	firstbyte = tvb_get_guint8(tvb, offset);
	if (firstbyte == 0x42)
	{
		/* Add packet format to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "GVCP Generic Packet (Firewall)");

		/* GVCP magic byte, must be a firewall opener packet */
		return;
	}

	/* Get packet format byte, look for enhanced flag and then clear it */
	info.format = tvb_get_guint8(tvb, offset + 4);
	info.formatstring = val_to_str(info.format, formatnames, "Unknown Format");
	info.enhanced = info.format & 0x80;
	info.format &= 0x7F;

	/* Add packet format to info string */
	col_append_fstr(pinfo->cinfo, COL_INFO, "%s ", info.formatstring);

	/* Dissect status */
	if (tree != NULL)
	{
		proto_tree_add_item(gvsp_tree, hf_gvsp_status, tvb, offset, 2, ENC_BIG_ENDIAN);
	}
	offset += 2;

	if (info.enhanced == 0)
	{
		info.blockid = tvb_get_ntohs(tvb, offset);

		/* Dissect block id 16 bits */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_blockid16, tvb, offset, 2, ENC_BIG_ENDIAN);
		}
		offset += 2;
	}
	else
	{
		/* Dissect flags */
		if (tree != NULL)
		{
			guint8 flags;
			flags = tvb_get_guint8(tvb, offset + 1);
			info.flag_resendrangeerror = flags & 0x04;
			info.flag_previousblockdropped = flags & 0x02;
			info.flag_packetresend = flags & 0x01;

			item = proto_tree_add_item(gvsp_tree, hf_gvsp_flags, tvb, offset, 2, ENC_BIG_ENDIAN);
			ti = proto_item_add_subtree(item, ett_gvsp_flags);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific0, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific1, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific2, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific3, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific4, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific5, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific6, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagdevicespecific7, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagresendrangeerror, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagpreviousblockdropped, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ti, hf_gvsp_flagpacketresend, tvb, offset, 2, ENC_BIG_ENDIAN);
		}
		offset += 2;
	}

	/* Dissect packet format */
	if (tree != NULL)
	{
		proto_tree_add_item(gvsp_tree, hf_gvsp_format, tvb, offset, 1, ENC_BIG_ENDIAN);
	}
	offset++;

	if (info.enhanced == 0)
	{
		info.packetid = tvb_get_ntohl(tvb, offset - 1);
		info.packetid &= 0x00FFFFFF;

		/* Dissect packet ID */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_packetid24, tvb, offset, 3, ENC_BIG_ENDIAN);
		}
		offset += 3;
	}
	else
	{
		/* Nothing... */
		offset += 3;
	}

	if (info.enhanced != 0)
	{
		guint64 high;
		guint64 low;
		high = tvb_get_ntohl(tvb, offset);
		low = tvb_get_ntohl(tvb, offset + 4);
		info.blockid = low | (high << 32);

		/* Dissect 64 bit block ID */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_blockid64, tvb, offset, 8, ENC_BIG_ENDIAN);
		}
		offset += 8;

		info.packetid = tvb_get_ntohl(tvb, offset);

		/* Dissect 32 bit packet ID */
		if (tree != NULL)
		{
			proto_tree_add_item(gvsp_tree, hf_gvsp_packetid32, tvb, offset, 4, ENC_BIG_ENDIAN);
		}
		offset += 4;
	}

	col_append_fstr(pinfo->cinfo, COL_INFO, "[Block ID: %" G_GINT64_MODIFIER "u Packet ID: %d] ", (guint64)info.blockid, info.packetid);

	if (info.flag_resendrangeerror != 0)
	{
		/* Add range error to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "[RANGE_ERROR] ");
	}

	if (info.flag_previousblockdropped != 0)
	{
		/* Add block dropped to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "[BLOCK_DROPPED] ");
	}

	if (info.flag_packetresend != 0)
	{
		/* Add packet resend to info string */
		col_append_fstr(pinfo->cinfo, COL_INFO, "[PACKET_RESEND] ");
	}

	/* Process packet types that are payload agnostic */
	switch (info.format)
	{
	case GVSP_PACKET_PAYLOAD:
		DissectPacketPayload(gvsp_tree, tvb, pinfo, offset);
		return;

	case GVSP_PACKET_PAYLOAD_H264:
		DissectPacketPayloadH264(gvsp_tree, tvb, pinfo, offset);
		return;

	case GVSP_PACKET_PAYLOAD_MULTIZONE:
		DissectPacketPayloadMultizone(gvsp_tree, tvb, pinfo, offset);
		return;

	default:
		break;
	}

	/* Get payload type, clear chunk bit */
	info.payloadtype = tvb_get_ntohs(tvb, offset + 2);
	info.payloadstring = val_to_str(info.payloadtype, payloadtypenames, "Unknown Payload Type");
	info.chunk = info.payloadtype & 0x4000;
	info.payloadtype &= 0x3FFF;

	/* Add payload type to information string */
	col_append_fstr(pinfo->cinfo, COL_INFO, "%s ", info.payloadstring);

	/* Process packet types for specific payload types */
	switch (info.format)
	{
	case GVSP_PACKET_ALLIN:
		DissectPacketAllIn(gvsp_tree, tvb, offset, pinfo, &info);
		return;

	case GVSP_PACKET_LEADER:
		DissectPacketLeader(gvsp_tree, tvb, offset, pinfo, &info);
		return;

	case GVSP_PACKET_TRAILER:
		DissectPacketTrailer(gvsp_tree, tvb, offset, pinfo, &info);
		return;

	default:
		break;
	}
}


/*
    \brief Registers the dissector. Invoked when the pluging library is loaded.
 */

void proto_register_gvsp(void) 
{
    static hf_register_info hfgvsp[] = 
    {
        {& hf_gvsp_status,          
        { "Status", "gvsp.status",
        FT_UINT16, BASE_HEX, VALS(statusnames), 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_blockid16,           
        { "Block ID (16 bits)", "gvsp.blockid16",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_flags,           
        { "Flags", "gvsp.flags",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific0,         
        { "Flag Device Specific 0", "gvsp.flag.devicespecific0",
        FT_UINT16, BASE_HEX, NULL, 0x8000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific1,         
        { "Flag Device Specific 1", "gvsp.flag.devicespecific1",
        FT_UINT16, BASE_HEX, NULL, 0x4000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific2,         
        { "Flag Device Specific 2", "gvsp.flag.devicespecific2",
        FT_UINT16, BASE_HEX, NULL, 0x2000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific3,         
        { "Flag Device Specific 3", "gvsp.flag.devicespecific3",
        FT_UINT16, BASE_HEX, NULL, 0x1000,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific4,         
        { "Flag Device Specific 4", "gvsp.flag.devicespecific4",
        FT_UINT16, BASE_HEX, NULL, 0x0800,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific5,         
        { "Flag Device Specific 5", "gvsp.flag.devicespecific5",
        FT_UINT16, BASE_HEX, NULL, 0x0400,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific6,         
        { "Flag Device Specific 6", "gvsp.flag.devicespecific6",
        FT_UINT16, BASE_HEX, NULL, 0x0200,
        NULL, HFILL
        }},

        {& hf_gvsp_flagdevicespecific7,         
        { "Flag Device Specific 7", "gvsp.flag.devicespecific7",
        FT_UINT16, BASE_HEX, NULL, 0x0100,
        NULL, HFILL
        }},

        {& hf_gvsp_flagresendrangeerror,            
        { "Flag Resend Range Error 7", "gvsp.flag.resendrangeerror",
        FT_UINT16, BASE_HEX, NULL, 0x0004,
        NULL, HFILL
        }},

        {& hf_gvsp_flagpreviousblockdropped,            
        { "Flag Previous Block Dropped", "gvsp.flag.previousblockdropped",
        FT_UINT16, BASE_HEX, NULL, 0x0002,
        NULL, HFILL
        }},

        {& hf_gvsp_flagpacketresend,            
        { "Flag Packet Resend", "gvsp.flag.packetresend",
        FT_UINT16, BASE_HEX, NULL, 0x0001,
        NULL, HFILL
        }},

        {& hf_gvsp_format,          
        { "Format", "gvsp.format",
        FT_UINT8, BASE_HEX, VALS(formatnames), 0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetid24,          
        { "Packet ID (24 bits)", "gvsp.packetid24",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_blockid64,           
        { "Block ID (64 bits v2.0)", "gvsp.blockid64",
        FT_UINT64, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetid32,          
        { "Packet ID (32 bits v2.0)", "gvsp.packetid32",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_payloadtype,         
        { "Payload Type", "gvsp.payloadtype",
        FT_UINT16, BASE_HEX, VALS(payloadtypenames), 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_payloaddata,         
        { "Payload Data", "gvsp.payloaddata",
        FT_BYTES, BASE_NONE, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_timestamp,           
        { "Timestamp", "gvsp.timestamp",
        FT_UINT64, BASE_HEX, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_pixelformat,         
        { "Pixel Format", "gvsp.pixel",
        FT_UINT32, BASE_HEX, VALS(pixeltypenames), 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_sizex,           
        { "Size X", "gvsp.sizex",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_sizey,           
        { "Size Y", "gvsp.sizey",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_offsetx,         
        { "Offset X", "gvsp.offsetx",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_offsety,         
        { "Offset Y", "gvsp.offsety",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_paddingx,            
        { "Padding X", "gvsp.paddingx",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},
        
        {& hf_gvsp_paddingy,            
        { "Padding Y", "gvsp.paddingy",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_payloaddatasize,         
        { "Payload Data Size", "gvsp.payloaddatasize",
        FT_UINT64, BASE_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_pixelcolor,          
        { "Monochrome or Color", "gvsp.pixel.color",
        FT_UINT32, BASE_HEX, VALS(colornames), 0xFF000000,
        NULL, HFILL
        }},

        {& hf_gvsp_pixeloccupy,         
        { "Occupy Bits", "gvsp.pixel.occupy",
        FT_UINT32, BASE_DEC, NULL, 0x00FF0000,
        NULL, HFILL
        }},

        {& hf_gvsp_pixelid,         
        { "ID", "gvsp.pixel.id",
        FT_UINT32, BASE_HEX, NULL, 0x0000FFFF,
        NULL, HFILL
        }},

        {& hf_gvsp_filename,            
        { "ID", "gvsp.filename",
        FT_STRINGZ, BASE_NONE, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_payloadlength,           
        { "Payload Length", "gvsp.payloadlength",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_fieldinfo,           
        { "Field Info", "gvsp.fieldinfo",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_fieldid,         
        { "Field ID", "gvsp.fieldid",
        FT_UINT8, BASE_HEX, NULL, 0xF0,
        NULL, HFILL
        }},

        {& hf_gvsp_fieldcount,          
        { "Field Count", "gvsp.fieldcount",
        FT_UINT8, BASE_HEX, NULL, 0x0F,
        NULL, HFILL
        }},

        {& hf_gvsp_genericflags,            
        { "Generic Flag", "gvsp.genericflag",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_timestamptickfrequency ,         
        { "Timestamp Tick Frequency", "gvsp.timestamptickfrequency",
        FT_UINT64, BASE_HEX, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_dataformat,          
        { "Data Format", "gvsp.dataformat",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetizationmode,           
        { "packetization_mode", "gvsp.packetizationmode",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_packetsize,          
        { "packet_size", "gvsp.packetsize",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_profileidc,          
        { "profile_idc", "gvsp.profileidc",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_cs,          
        { "cs", "gvsp.cs",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_cs0,         
        { "cs0", "gvsp.cs0",
        FT_UINT8, BASE_HEX, NULL, 0x80,
        NULL, HFILL
        }},

        {& hf_gvsp_cs1,         
        { "cs1", "gvsp.cs1",
        FT_UINT8, BASE_HEX, NULL, 0x40,
        NULL, HFILL
        }},

        {& hf_gvsp_cs2,         
        { "cs2", "gvsp.cs2",
        FT_UINT8, BASE_HEX, NULL, 0x20,
        NULL, HFILL
        }},

        {& hf_gvsp_cs3,         
        { "cs3", "gvsp.cs3",
        FT_UINT8, BASE_HEX, NULL, 0x10,
        NULL, HFILL
        }},

        {& hf_gvsp_levelidc,            
        { "level_idc", "gvsp.levelidc",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropinterleavingdepth,           
        { "srop_interlaving_depth", "gvsp.sropinterleavingdepth",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropmaxdondiff,          
        { "srop_max_don_diff", "gvsp.sropmaxdondiff",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropdeintbufreq,         
        { "srop_deint_buf_req", "gvsp.sropdeintbufreq",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sropinitbuftime,         
        { "srop_init_buf_time", "gvsp.sropinitbuftime",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_zoneinfo,            
        { "Zone Info", "gvsp.zoneinfo",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_zoneid,          
        { "Zone ID", "gvsp.zoneid",
        FT_UINT8, BASE_HEX, NULL, 0x3E,
        NULL, HFILL
        }},

        {& hf_gvsp_endofzone,           
        { "End of Zone", "gvsp.endofzone",
        FT_UINT8, BASE_HEX, NULL, 0x01,
        NULL, HFILL
        }},

        {& hf_gvsp_addressoffsethigh,           
        { "Address Offset High", "gvsp.addressoffsethigh",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_addressoffsetlow,            
        { "Address Offset Low", "gvsp.addressoffsetlow",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL
        }},

        {& hf_gvsp_sc_zone_direction,   
        { "Zone Directions Mask", "gvcpzonedirection",
        FT_BOOLEAN, 32, TFS(&directionnames), 0,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone0_direction,  
        { "Zone 0 Direction", "gvcp.zone0direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x80000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone1_direction,  
        { "Zone 1 Direction", "gvcp.zone1direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x40000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone2_direction,  
        { "Zone 2 Direction", "gvcp.zone2direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x20000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone3_direction,  
        { "Zone 3 Direction", "gvcp.zone3direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x10000000,
        NULL, HFILL
        }}, 
        
        {& hf_gvsp_sc_zone4_direction,  
        { "Zone 4 Direction", "gvcp.zone4direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x08000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone5_direction,  
        { "Zone 5 Direction", "gvcp.zone5direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x04000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone6_direction,  
        { "Zone 6 Direction", "gvcp.zone6direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x02000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone7_direction,  
        { "Zone 7 Direction", "gvcp.zone7direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x01000000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone8_direction,  
        { "Zone 8 Direction", "gvcp.zone8direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00800000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone9_direction,  
        { "Zone 9 Direction", "gvcp.zone9direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00400000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone10_direction, 
        { "Zone 10 Direction", "gvcp.zone10direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00200000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone11_direction, 
        { "Zone 11 Direction", "gvcp.zone1direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00100000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone12_direction, 
        { "Zone 12 Direction", "gvcp.zone12direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00080000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone13_direction, 
        { "Zone 13 Direction", "gvcp.zone13direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00040000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone14_direction, 
        { "Zone 14 Direction", "gvcp.zone14direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00020000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone15_direction, 
        { "Zone 15 Direction", "gvcp.zone15direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00010000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone16_direction, 
        { "Zone 16 Direction", "gvcp.zone16direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00008000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone17_direction, 
        { "Zone 17 Direction", "gvcp.zone17direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00004000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone18_direction, 
        { "Zone 18 Direction", "gvcp.zone18direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00002000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone19_direction, 
        { "Zone 19 Direction", "gvcp.zone19direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00001000,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone20_direction, 
        { "Zone 20 Direction", "gvcp.zone20direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000800,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone21_direction, 
        { "Zone 21 Direction", "gvcp.zone21direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000400,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone22_direction, 
        { "Zone 22 Direction", "gvcp.zone22direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000200,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone23_direction, 
        { "Zone 23 Direction", "gvcp.zone23direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000100,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone24_direction, 
        { "Zone 24 Direction", "gvcp.zone24direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000080,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone25_direction, 
        { "Zone 25 Direction", "gvcp.zone25direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000040,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone26_direction, 
        { "Zone 26 Direction", "gvcp.zone26direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000020,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone27_direction, 
        { "Zone 27 Direction", "gvcp.zone27direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000010,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone28_direction, 
        { "Zone 28 Direction", "gvcp.zone28direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000008,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone29_direction, 
        { "Zone 29 Direction", "gvcp.zone29direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000004,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone30_direction, 
        { "Zone 30 Direction", "gvcp.zone30direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000002,
        NULL, HFILL
        }}, 

        {& hf_gvsp_sc_zone31_direction, 
        { "Zone 31 Direction", "gvcp.zone31direction",
        FT_BOOLEAN, 32, TFS(&directionnames), 0x00000001,
        NULL, HFILL
        }}, 

        {& hf_gvsp_chunkdatapayloadlengthex,            
        { "Chunk Data Payload Length", "gvsp.chunkdatapayloadlengthex",
        FT_UINT32, BASE_HEX_DEC, NULL, 0x0,
        NULL, HFILL
        }},

        {& hf_gvsp_chunklayoutidex,         
        { "Chunk Layout ID", "gvsp.chunklayoutidex",
        FT_UINT32, BASE_HEX_DEC, NULL, 0x0,
        NULL, HFILL
        }},
    };

    static gint *ett[] = {
        &ett_gvsp,
		&ett_gvsp_flags,
		&ett_gvsp_header,
		&ett_gvsp_payload,
		&ett_gvsp_trailer,
    };

    proto_gvsp = proto_register_protocol("GigE Vision Streaming Protocol", "GVSP", "gvsp");

    register_dissector("gvsp", dissect_gvsp, proto_gvsp);

    proto_register_field_array(proto_gvsp, hfgvsp, array_length(hfgvsp));
    proto_register_subtree_array(ett, array_length(ett));
}


/*
    \brief Rules for GVSP packets identification (?)
 */

void proto_reg_handoff_gvsp(void) 
{
    static int gvsp_initialized = FALSE;
    static dissector_handle_t gvsp_handle;

    if (!gvsp_initialized) 
	{
        gvsp_handle = new_create_dissector_handle((new_dissector_t)dissect_gvsp, proto_gvsp);
        dissector_add_uint("udp.port", global_gvsp_port, gvsp_handle);
        gvsp_initialized = TRUE;
    } 
	else 
	{
        dissector_delete_uint("udp.port", global_gvsp_port, gvsp_handle);
    }
}

