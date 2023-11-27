/* Aravis - Digital camera library
 *
 * Copyright © 2023 Xiaoqiang Wang
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
 * Author: Xiaoqiang Wang <xiaoqiang.wang@psi.ch>
 */

#ifndef ARV_GENTL_SYSTEM_PRIVATE_H
#define ARV_GENTL_SYSTEM_PRIVATE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvgentlsystem.h>
#include <GenTL_v1_5.h>

G_BEGIN_DECLS
#ifdef G_CXX_STD_VERSION
namespace GenTL {
#endif

typedef struct {
    /* Global functions */
    PGCInitLib                 GCInitLib;
    PGCCloseLib                GCCloseLib;
    PGCGetInfo                 GCGetInfo;
    PGCGetLastError            GCGetLastError;
    PGCReadPort                GCReadPort;
    PGCWritePort               GCWritePort;
    PGCReadPortStacked         GCReadPortStacked;      /* GenTL v1.1 */
    PGCWritePortStacked        GCWritePortStacked;     /* GenTL v1.1 */
    PGCGetPortURL              GCGetPortURL;           /* legacy function */
    PGCGetPortInfo             GCGetPortInfo;
    PGCGetNumPortURLs          GCGetNumPortURLs;       /* GenTL v1.1 */
    PGCGetPortURLInfo          GCGetPortURLInfo;       /* GenTL v1.1 */
    PGCRegisterEvent           GCRegisterEvent;
    PGCUnregisterEvent         GCUnregisterEvent;

    /* TL functions */
    PTLOpen                    TLOpen;
    PTLClose                   TLClose;
    PTLGetInfo                 TLGetInfo;
    PTLGetNumInterfaces        TLGetNumInterfaces;
    PTLGetInterfaceID          TLGetInterfaceID;
    PTLGetInterfaceInfo        TLGetInterfaceInfo;
    PTLOpenInterface           TLOpenInterface;
    PTLUpdateInterfaceList     TLUpdateInterfaceList;

    /* Interface functions */
    PIFClose                   IFClose;
    PIFGetInfo                 IFGetInfo;
    PIFGetNumDevices           IFGetNumDevices;
    PIFGetDeviceID             IFGetDeviceID;
    PIFUpdateDeviceList        IFUpdateDeviceList;
    PIFGetDeviceInfo           IFGetDeviceInfo;
    PIFOpenDevice              IFOpenDevice;
    PIFGetParentTL             IFGetParentTL;          /* GenTL v1.4 */

    /* Device functions */
    PDevGetPort                DevGetPort;
    PDevGetNumDataStreams      DevGetNumDataStreams;
    PDevGetDataStreamID        DevGetDataStreamID;
    PDevOpenDataStream         DevOpenDataStream;
    PDevGetInfo                DevGetInfo;
    PDevClose                  DevClose;
    PDevGetParentIF            DevGetParentIF;         /* GenTL v1.4 */

    /* Data stream functions */
    PDSAnnounceBuffer          DSAnnounceBuffer;
    PDSAllocAndAnnounceBuffer  DSAllocAndAnnounceBuffer;
    PDSFlushQueue              DSFlushQueue;
    PDSStartAcquisition        DSStartAcquisition;
    PDSStopAcquisition         DSStopAcquisition;
    PDSGetInfo                 DSGetInfo;
    PDSGetBufferID             DSGetBufferID;
    PDSClose                   DSClose;
    PDSRevokeBuffer            DSRevokeBuffer;
    PDSQueueBuffer             DSQueueBuffer;
    PDSGetBufferInfo           DSGetBufferInfo;
    PDSGetBufferChunkData      DSGetBufferChunkData;   /* GenTL v1.3 */
    PDSGetParentDev            DSGetParentDev;         /* GenTL v1.4 */
    PDSGetNumBufferParts       DSGetNumBufferParts;    /* GenTL v1.5 */
    PDSGetBufferPartInfo       DSGetBufferPartInfo;    /* GenTL v1.5 */

    /* Event functions */
    PEventGetData              EventGetData;
    PEventGetDataInfo          EventGetDataInfo;
    PEventGetInfo              EventGetInfo;
    PEventFlush                EventFlush;
    PEventKill                 EventKill;
} ArvGenTLModule;

GList *			arv_gentl_get_systems                   (void);

ArvGenTLModule *	arv_gentl_system_get_gentl              (ArvGenTLSystem *system);

TL_HANDLE 		arv_gentl_system_open_system_handle     (ArvGenTLSystem *system);
void 			arv_gentl_system_close_system_handle    (ArvGenTLSystem *system);

IF_HANDLE		arv_gentl_system_open_interface_handle  (ArvGenTLSystem *system,
                                                                 const char *interface_id);
void 			arv_gentl_system_close_interface_handle (ArvGenTLSystem *system,
                                                                 const char *interface_id);

DEV_HANDLE 		arv_gentl_system_open_device_handle     (ArvGenTLSystem *system,
                                                                 const char *interface_id,
                                                                 const char *device_id);
void 			arv_gentl_system_close_device_handle    (ArvGenTLSystem *system,
                                                                 const char *interface_id,
                                                                 DEV_HANDLE *device_handle);

#ifdef G_CXX_STD_VERSION
} /* end of namespace GenTL */
#endif
G_END_DECLS

#endif
