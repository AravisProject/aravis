/* Aravis - Digital camera library
 *
 * Copyright © 2009-2026 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_GENTL_H
#define ARV_GENTL_H

#include <arvtypes.h>
#include <arvapi.h>
#include <stdint.h>

G_BEGIN_DECLS

#define GENTL_INVALID_HANDLE NULL
#define GENTL_INFINITE 0xFFFFFFFFFFFFFFFFULL

typedef int8_t bool8_t;

typedef int32_t GC_ERROR;
enum {
        GC_ERR_SUCCESS                  = 0,
        GC_ERR_ERROR                    = -1001,
        GC_ERR_NOT_INITIALIZED          = -1002,
        GC_ERR_NOT_IMPLEMENTED          = -1003,
        GC_ERR_RESOURCE_IN_USE          = -1004,
        GC_ERR_ACCESS_DENIED            = -1005,
        GC_ERR_INVALID_HANDLE           = -1006,
        GC_ERR_INVALID_ID               = -1007,
        GC_ERR_NO_DATA                  = -1008,
        GC_ERR_INVALID_PARAMETER        = -1009,
        GC_ERR_IO                       = -1010,
        GC_ERR_TIMEOUT                  = -1011,
        GC_ERR_ABORT                    = -1012,
        GC_ERR_INVALID_BUFFER           = -1013,
        GC_ERR_NOT_AVAILABLE            = -1014,
        GC_ERR_INVALID_ADDRESS          = -1015,
        GC_ERR_BUFFER_TOO_SMALL         = -1016,
        GC_ERR_INVALID_INDEX            = -1017,
        GC_ERR_PARSING_CHUNK_DATA       = -1018,
        GC_ERR_INVALID_VALUE            = -1019,
        GC_ERR_RESOURCE_EXHAUSTED       = -1020,
        GC_ERR_OUT_OF_MEMORY            = -1021,
        GC_ERR_BUSY                     = -1022,
        GC_ERR_AMBIGUOUS                = -1023,
        GC_ERR_CUSTOM_ID                = -10000
};

typedef int32_t TL_CHAR_ENCODING;
enum
{
        TL_CHAR_ENCODING_ASCII          = 0,
        TL_CHAR_ENCODING_UTF8           = 1
};

typedef int32_t TL_INFO_CMD;
enum {
        TL_INFO_ID                      = 0,
        TL_INFO_VENDOR                  = 1,
        TL_INFO_MODEL                   = 2,
        TL_INFO_VERSION                 = 3,
        TL_INFO_TLTYPE                  = 4,
        TL_INFO_NAME                    = 5,
        TL_INFO_PATHNAME                = 6,
        TL_INFO_DISPLAYNAME             = 7,
        TL_INFO_CHAR_ENCODING           = 8,
        TL_INFO_GENTL_VER_MAJOR         = 9,
        TL_INFO_GENTL_VER_MINOR         = 10,
        TL_INFO_CUSTOM_ID               = 1000
} ;


typedef int32_t INFO_DATATYPE;
enum {
        INFO_DATATYPE_UNKNOWN           = 0,
        INFO_DATATYPE_STRING            = 1,
        INFO_DATATYPE_STRINGLIST        = 2,
        INFO_DATATYPE_INT16             = 3,
        INFO_DATATYPE_UINT16            = 4,
        INFO_DATATYPE_INT32             = 5,
        INFO_DATATYPE_UINT32            = 6,
        INFO_DATATYPE_INT64             = 7,
        INFO_DATATYPE_UINT64            = 8,
        INFO_DATATYPE_FLOAT64           = 9,
        INFO_DATATYPE_PTR               = 10,
        INFO_DATATYPE_BOOL8             = 11,
        INFO_DATATYPE_SIZET             = 12,
        INFO_DATATYPE_BUFFER            = 13,
        INFO_DATATYPE_PTRDIFF           = 14,
        INFO_DATATYPE_CUSTOM_ID         = 1000
};

typedef int32_t INTERFACE_INFO_CMD;
enum {
        INTERFACE_INFO_ID               = 0,
        INTERFACE_INFO_DISPLAYNAME      = 1,
        INTERFACE_INFO_TLTYPE           = 2,
        INTERFACE_INFO_CUSTOM_ID        = 1000
};

typedef int32_t DEVICE_INFO_CMD;
enum {
        DEVICE_INFO_ID                  = 0,
        DEVICE_INFO_VENDOR              = 1,
        DEVICE_INFO_MODEL               = 2,
        DEVICE_INFO_TLTYPE              = 3,
        DEVICE_INFO_DISPLAYNAME         = 4,
        DEVICE_INFO_ACCESS_STATUS       = 5,
        DEVICE_INFO_USER_DEFINED_NAME   = 6,
        DEVICE_INFO_SERIAL_NUMBER       = 7,
        DEVICE_INFO_VERSION             = 8,
        DEVICE_INFO_TIMESTAMP_FREQUENCY = 9,
        DEVICE_INFO_CUSTOM_ID           = 1000
};

typedef int32_t DEVICE_ACCESS_FLAGS;
enum {
        DEVICE_ACCESS_UNKNOWN           = 0,
        DEVICE_ACCESS_NONE              = 1,
        DEVICE_ACCESS_READONLY          = 2,
        DEVICE_ACCESS_CONTROL           = 3,
        DEVICE_ACCESS_EXCLUSIVE         = 4,
        DEVICE_ACCESS_CUSTOM_ID         = 1000
};

typedef int32_t DEVICE_ACCESS_STATUS;
enum {
        DEVICE_ACCESS_STATUS_UNKNOWN            = 0,
        DEVICE_ACCESS_STATUS_READWRITE          = 1,
        DEVICE_ACCESS_STATUS_READONLY           = 2,
        DEVICE_ACCESS_STATUS_NOACCESS           = 3,
        DEVICE_ACCESS_STATUS_BUSY               = 4,
        DEVICE_ACCESS_STATUS_OPEN_READWRITE     = 5,
        DEVICE_ACCESS_STATUS_OPEN_READONLY      = 6,
        DEVICE_ACCESS_STATUS_CUSTOM_ID          = 1000
};

typedef int32_t ACQ_QUEUE_TYPE;
enum {
        ACQ_QUEUE_INPUT_TO_OUTPUT       = 0,
        ACQ_QUEUE_OUTPUT_DISCARD        = 1,
        ACQ_QUEUE_ALL_TO_INPUT          = 2,
        ACQ_QUEUE_UNQUEUED_TO_INPUT     = 3,
        ACQ_QUEUE_ALL_DISCARD           = 4,
        ACQ_QUEUE_CUSTOM_ID             = 1000
};

typedef int32_t BUFFER_INFO_CMD;
enum {
        BUFFER_INFO_BASE                = 0,
        BUFFER_INFO_SIZE                = 1,
        BUFFER_INFO_USER_PTR            = 2,
        BUFFER_INFO_TIMESTAMP           = 3,
        BUFFER_INFO_NEW_DATA            = 4,
        BUFFER_INFO_IS_QUEUED           = 5,
        BUFFER_INFO_IS_ACQUIRING        = 6,
        BUFFER_INFO_IS_INCOMPLETE       = 7,
        BUFFER_INFO_TLTYPE              = 8,
        BUFFER_INFO_SIZE_FILLED         = 9,
        BUFFER_INFO_WIDTH               = 10,
        BUFFER_INFO_HEIGHT              = 11,
        BUFFER_INFO_XOFFSET             = 12,
        BUFFER_INFO_YOFFSET             = 13,
        BUFFER_INFO_XPADDING            = 14,
        BUFFER_INFO_YPADDING            = 15,
        BUFFER_INFO_FRAMEID             = 16,
        BUFFER_INFO_IMAGEPRESENT        = 17,
        BUFFER_INFO_IMAGEOFFSET         = 18,
        BUFFER_INFO_PAYLOADTYPE         = 19,
        BUFFER_INFO_PIXELFORMAT         = 20,
        BUFFER_INFO_PIXELFORMAT_NAMESPACE       = 21,
        BUFFER_INFO_DELIVERED_IMAGEHEIGHT       = 22,
        BUFFER_INFO_DELIVERED_CHUNKPAYLOADSIZE  = 23,
        BUFFER_INFO_CHUNKLAYOUTID               = 24,
        BUFFER_INFO_FILENAME                    = 25,
        BUFFER_INFO_PIXEL_ENDIANNESS            = 26,
        BUFFER_INFO_DATA_SIZE                   = 27,
        BUFFER_INFO_TIMESTAMP_NS                = 28,
        BUFFER_INFO_DATA_LARGER_THAN_BUFFER     = 29,
        BUFFER_INFO_CONTAINS_CHUNKDATA          = 30,
        BUFFER_INFO_IS_COMPOSITE                = 31,
        BUFFER_INFO_CUSTOM_ID                   = 1000
};

typedef int32_t PAYLOADTYPE_INFO_ID;
enum PAYLOADTYPE_INFO_IDS
{
        PAYLOAD_TYPE_UNKNOWN                    =  0,
        PAYLOAD_TYPE_IMAGE                      =  1,
        PAYLOAD_TYPE_RAW_DATA                   =  2,
        PAYLOAD_TYPE_FILE                       =  3,
        PAYLOAD_TYPE_CHUNK_DATA                 =  4,
        PAYLOAD_TYPE_JPEG                       =  5,
        PAYLOAD_TYPE_JPEG2000                   =  6,
        PAYLOAD_TYPE_H264                       =  7,
        PAYLOAD_TYPE_CHUNK_ONLY                 =  8,
        PAYLOAD_TYPE_DEVICE_SPECIFIC            =  9,
        PAYLOAD_TYPE_MULTI_PART                 =  10,
        PAYLOAD_TYPE_GENDC                      =  11,
        PAYLOAD_TYPE_CUSTOM_ID                  = 1000
};

typedef int32_t STREAM_INFO_CMD;
enum {
        STREAM_INFO_ID                          =  0,
        STREAM_INFO_NUM_DELIVERED               =  1,
        STREAM_INFO_NUM_UNDERRUN                =  2,
        STREAM_INFO_NUM_ANNOUNCED               =  3,
        STREAM_INFO_NUM_QUEUED                  =  4,
        STREAM_INFO_NUM_AWAIT_DELIVERY          =  5,
        STREAM_INFO_NUM_STARTED                 =  6,
        STREAM_INFO_PAYLOAD_SIZE                =  7,
        STREAM_INFO_IS_GRABBING                 =  8,
        STREAM_INFO_DEFINES_PAYLOADSIZE         =  9,
        STREAM_INFO_TLTYPE                      = 10,
        STREAM_INFO_NUM_CHUNKS_MAX              = 11,
        STREAM_INFO_BUF_ANNOUNCE_MIN            = 12,
        STREAM_INFO_BUF_ALIGNMENT               = 13,
        STREAM_INFO_FLOW_TABLE                  = 14,
        STREAM_INFO_GENDC_PREFETCH_DESCRIPTOR   = 15,
        STREAM_INFO_CUSTOM_ID                   = 1000
};

typedef int32_t ACQ_START_FLAGS;
enum {
        ACQ_START_FLAGS_DEFAULT                 = 0,
        ACQ_START_FLAGS_CUSTOM_ID               = 1000
};

typedef int32_t ACQ_STOP_FLAGS;
enum {
        ACQ_STOP_FLAGS_DEFAULT                  = 0,
        ACQ_STOP_FLAGS_KILL                     = 1,
        ACQ_STOP_FLAGS_CUSTOM_ID                = 1000
};

typedef int32_t BUFFER_PART_INFO_CMD;
enum {
        BUFFER_PART_INFO_BASE                   = 0,
        BUFFER_PART_INFO_DATA_SIZE              = 1,
        BUFFER_PART_INFO_DATA_TYPE              = 2,
        BUFFER_PART_INFO_DATA_FORMAT            = 3,
        BUFFER_PART_INFO_DATA_FORMAT_NAMESPACE  = 4,
        BUFFER_PART_INFO_WIDTH                  = 5,
        BUFFER_PART_INFO_HEIGHT                 = 6,
        BUFFER_PART_INFO_XOFFSET                = 7,
        BUFFER_PART_INFO_YOFFSET                = 8,
        BUFFER_PART_INFO_XPADDING               = 9,
        BUFFER_PART_INFO_SOURCE_ID              = 10,
        BUFFER_PART_INFO_DELIVERED_IMAGEHEIGHT  = 11,
        BUFFER_PART_INFO_REGION_ID              = 12,
        BUFFER_PART_INFO_DATA_PURPOSE_ID        = 13,
        BUFFER_PART_INFO_CUSTOM_ID              = 1000
};

typedef int32_t FLOW_INFO_CMD;
enum {
        FLOW_INFO_SIZE                          = 0,
        FLOW_INFO_CUSTOM_ID                     = 1000
};

typedef int32_t SEGMENT_INFO_CMD;
enum {
        SEGMENT_INFO_BASE                       = 0,
        SEGMENT_INFO_SIZE                       = 1,
        SEGMENT_INFO_IS_INCOMPLETE              = 2,
        SEGMENT_INFO_SIZE_FILLED                = 3,
        SEGMENT_INFO_DATA_SIZE                  = 4,
        SEGMENT_INFO_CUSTOM_ID                  = 1000
};

typedef int32_t PORT_INFO_CMD;
enum {
        PORT_INFO_ID                            = 0,
        PORT_INFO_VENDOR                        = 1,
        PORT_INFO_MODEL                         = 2,
        PORT_INFO_TLTYPE                        = 3,
        PORT_INFO_MODULE                        = 4,
        PORT_INFO_LITTLE_ENDIAN                 = 5,
        PORT_INFO_BIG_ENDIAN                    = 6,
        PORT_INFO_ACCESS_READ                   = 7,
        PORT_INFO_ACCESS_WRITE                  = 8,
        PORT_INFO_ACCESS_NA                     = 9,
        PORT_INFO_ACCESS_NI                     = 10,
        PORT_INFO_VERSION                       = 11,
        PORT_INFO_PORTNAME                      = 12,
        PORT_INFO_CUSTOM_ID                     = 1000
};

typedef int32_t URL_INFO_CMD;
enum {
        URL_INFO_URL                            = 0,
        URL_INFO_SCHEMA_VER_MAJOR               = 1,
        URL_INFO_SCHEMA_VER_MINOR               = 2,
        URL_INFO_FILE_VER_MAJOR                 = 3,
        URL_INFO_FILE_VER_MINOR                 = 4,
        URL_INFO_FILE_VER_SUBMINOR              = 5,
        URL_INFO_FILE_SHA1_HASH                 = 6,
        URL_INFO_FILE_REGISTER_ADDRESS          = 7,
        URL_INFO_FILE_SIZE                      = 8,
        URL_INFO_SCHEME                         = 9,
        URL_INFO_FILENAME                       = 10,
        URL_INFO_CUSTOM_ID                      = 1000
};

typedef int32_t EVENT_DATA_INFO_CMD;
enum {
        EVENT_DATA_ID                           = 0,
        EVENT_DATA_VALUE                        = 1,
        EVENT_DATA_NUMID                        = 2,
        EVENT_DATA_CUSTOM_ID                    = 1000
};

typedef int32_t EVENT_INFO_CMD;
enum {
        EVENT_EVENT_TYPE                        = 0,
        EVENT_NUM_IN_QUEUE                      = 1,
        EVENT_NUM_FIRED                         = 2,
        EVENT_SIZE_MAX                          = 3,
        EVENT_INFO_DATA_SIZE_MAX                = 4,
        EVENT_INFO_CUSTOM_ID                    = 1000
};

typedef int32_t EVENT_TYPE;
enum {
        EVENT_ERROR                             = 0,
        EVENT_NEW_BUFFER                        = 1,
        EVENT_FEATURE_INVALIDATE                = 2,
        EVENT_FEATURE_CHANGE                    = 3,
        EVENT_REMOTE_DEVICE                     = 4,
        EVENT_MODULE                            = 5,
        EVENT_CUSTOM_ID                         = 1000
};

typedef void * TL_HANDLE;
typedef void * IF_HANDLE;
typedef void * DEV_HANDLE;
typedef void * PORT_HANDLE;
typedef void * DS_HANDLE;
typedef void * BUFFER_HANDLE;
typedef void * EVENT_HANDLE;
typedef void * EVENTSRC_HANDLE;

#pragma pack(push,1)

typedef struct {
        uint64_t ChunkID;
        ptrdiff_t ChunkOffset;
        size_t ChunkLength;
} SINGLE_CHUNK_DATA;

typedef struct {
        BUFFER_INFO_CMD iInfoCmd;
        INFO_DATATYPE iType;
        void *puffer;
        size_t iSize;
        GC_ERROR iResult;
} DS_BUFFER_INFO_STACKED;

typedef struct {
        uint32_t iPartIndex;
        BUFFER_PART_INFO_CMD iInfoCmd;
        INFO_DATATYPE iType;
        GC_ERROR iResult;
        void *pBuffer;
        size_t iSize;
} DS_BUFFER_PART_INFO_STACKED;

typedef struct {
        BUFFER_HANDLE BufferHandle;
        void *UserPointer;
} EVENT_NEW_BUFFER_DATA;

typedef struct {
        uint64_t Address;
        void *Buffer;
        size_t Size;
} PORT_REGISTER_STACK_ENTRY;

#pragma pack(pop)

ARV_API GC_ERROR        GCCloseLib                      (void);
ARV_API GC_ERROR        GCGetInfo                       (TL_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );
ARV_API GC_ERROR        GCGetLastError                  (GC_ERROR *piErrorCode, char *sErrorText, size_t *piSize);
ARV_API GC_ERROR        GCInitLib                       (void);

ARV_API GC_ERROR        TLClose                         (TL_HANDLE hSystem);
ARV_API GC_ERROR        TLGetInfo                       (TL_HANDLE hSystem, TL_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
ARV_API GC_ERROR        TLGetInterfaceID                (TL_HANDLE hSystem, uint32_t iIndex,
                                                         char *sIfaceID, size_t *piSize );
ARV_API GC_ERROR        TLGetInterfaceInfo              (TL_HANDLE hSystem, const char *sIfaceID,
                                                         INTERFACE_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
ARV_API GC_ERROR        TLGetNumInterfaces              (TL_HANDLE hSystem, uint32_t *piNumIfaces);
ARV_API GC_ERROR        TLOpen                          (TL_HANDLE *phSystem);
ARV_API GC_ERROR        TLOpenInterface                 (TL_HANDLE hSystem, const char * sIfaceID, IF_HANDLE *phIface);
ARV_API GC_ERROR        TLUpdateInterfaceList           (TL_HANDLE hSystem, bool8_t *pbChanged, uint64_t iTimeout);

ARV_API GC_ERROR        IFClose                         (IF_HANDLE hIface);
ARV_API GC_ERROR        IFGetInfo                       (IF_HANDLE hIface, INTERFACE_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t * piSize);
ARV_API GC_ERROR        IFGetDeviceID                   (IF_HANDLE hIface, uint32_t iIndex,
                                                         char *sDeviceID, size_t *piSize );
ARV_API GC_ERROR        IFGetDeviceInfo                 (IF_HANDLE hIface, const char *sDeviceID,
                                                         DEVICE_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
ARV_API GC_ERROR        IFGetNumDevices                 (IF_HANDLE hIface, uint32_t *piNumDevices);
ARV_API GC_ERROR        IFOpenDevice                    (IF_HANDLE hIface, const char * sDeviceID,
                                                         DEVICE_ACCESS_FLAGS iOpenFlag, DEV_HANDLE *phDevice);
ARV_API GC_ERROR        IFUpdateDeviceList              (IF_HANDLE hIface, bool8_t *pbChanged, uint64_t iTimeout);
ARV_API GC_ERROR        IFGetParentTL                   (IF_HANDLE hIface, TL_HANDLE *phSystem);

ARV_API GC_ERROR        DevClose                        (DEV_HANDLE hDevice);
ARV_API GC_ERROR        DevGetInfo                      (DEV_HANDLE hDevice, DEVICE_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
ARV_API GC_ERROR        DevGetDataStreamID              (DEV_HANDLE hDevice, uint32_t iIndex,
                                                         char *sDataStreamID, size_t *piSize);
ARV_API GC_ERROR        DevGetNumDataStreams            (DEV_HANDLE hDevice, uint32_t *piNumDataStreams);
ARV_API GC_ERROR        DevGetPort                      (DEV_HANDLE hDevice, PORT_HANDLE *phRemoteDev);
ARV_API GC_ERROR        DevOpenDataStream               (DEV_HANDLE hDevice, const char *sDataStreamID,
                                                         DS_HANDLE *phDataStream );
ARV_API GC_ERROR        DevGetParentIF                  (DEV_HANDLE hDevice, IF_HANDLE *phIface);

ARV_API GC_ERROR        DSAllocAndAnnounceBuffer        (DS_HANDLE hDataStream, size_t iBufferSize,
                                                         void *pPrivate, BUFFER_HANDLE *phBuffer);
ARV_API GC_ERROR        DSAnnounceBuffer                (DS_HANDLE hDataStream, void *pBuffer,
                                                         size_t iSize, void *pPrivate, BUFFER_HANDLE *phBuffer);
ARV_API GC_ERROR        DSAnnounceCompositeBuffer       (DS_HANDLE hDataStream, size_t iNumSegments, void ** ppSegments,
                                                         size_t *piSizes, void *pPrivate, BUFFER_HANDLE *phBuffer);
ARV_API GC_ERROR        DSClose                         (DS_HANDLE hDataStream);
ARV_API GC_ERROR        DSFlushQueue                    (DS_HANDLE hDataStream, ACQ_QUEUE_TYPE iOperation);
ARV_API GC_ERROR        DSGetBufferID                   (DS_HANDLE hDataStream, uint32_t iIndex, BUFFER_HANDLE *phBuffer);
ARV_API GC_ERROR        DSGetBufferInfo                 (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                         BUFFER_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
ARV_API GC_ERROR        DSGetInfo                       (DS_HANDLE hDataStream, STREAM_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
ARV_API GC_ERROR        DSQueueBuffer                   (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer);
ARV_API GC_ERROR        DSRevokeBuffer                  (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                         void **ppBuffer, void **ppPrivate);
ARV_API GC_ERROR        DSStartAcquisition              (DS_HANDLE hDataStream, ACQ_START_FLAGS iStartFlags,
                                                         uint64_t iNumToAcquire);
ARV_API GC_ERROR        DSStopAcquisition               (DS_HANDLE hDataStream, ACQ_STOP_FLAGS iStopFlags);
ARV_API GC_ERROR        DSGetBufferChunkData            (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                         SINGLE_CHUNK_DATA *pChunkData, size_t *piNumChunks);
ARV_API GC_ERROR        DSGetParentDev                  (DS_HANDLE hDataStream, DEV_HANDLE *phDevice);
ARV_API GC_ERROR        DSGetNumBufferParts             (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t *piNumParts);
ARV_API GC_ERROR        DSGetBufferPartInfo             (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t iPartIndex,
                                                         BUFFER_PART_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
ARV_API GC_ERROR        DSGetBufferInfoStacked          (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                         DS_BUFFER_INFO_STACKED *pInfoStacked, size_t iNumInfos);
ARV_API GC_ERROR        DSGetBufferPartInfoStacked      (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                         DS_BUFFER_PART_INFO_STACKED *pInfoStacked, size_t iNumInfos);
ARV_API GC_ERROR        DSGetNumFlows                   (DS_HANDLE hDataStream, uint32_t *piNumFlows);
ARV_API GC_ERROR        DSGetFlowInfo                   (DS_HANDLE hDataStream, uint32_t iFlowIndex,
                                                         FLOW_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
ARV_API GC_ERROR        DSGetNumBufferSegments          (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                         uint32_t *piNumSegments);
ARV_API GC_ERROR        DSGetBufferSegmentInfo          (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                         uint32_t iSegmentIndex, SEGMENT_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);

ARV_API GC_ERROR        GCGetPortInfo                   (PORT_HANDLE hPort, PORT_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
ARV_API GC_ERROR        GCGetPortURL                    (PORT_HANDLE hPort, char *sURL, size_t *piSize);
ARV_API GC_ERROR        GCGetNumPortURLs                (PORT_HANDLE hPort, uint32_t *piNumURLs);
ARV_API GC_ERROR        GCGetPortURLInfo                (PORT_HANDLE hPort, uint32_t iURLIndex, URL_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
ARV_API GC_ERROR        GCReadPort                      (PORT_HANDLE hPort, uint64_t iAddress, void *pBuffer, size_t *piSize);
ARV_API GC_ERROR        GCWritePort                     (PORT_HANDLE hPort, uint64_t iAddress, const void *pBuffer, size_t *piSize);
ARV_API GC_ERROR        GCWritePortStacked              (PORT_HANDLE hPort, PORT_REGISTER_STACK_ENTRY *pEntries,
                                                         size_t * piNumEntries);
ARV_API GC_ERROR        GCReadPortStacked               (PORT_HANDLE hPort, PORT_REGISTER_STACK_ENTRY *pEntries,
                                                         size_t * piNumEntries);

ARV_API GC_ERROR        EventFlush                      (EVENT_HANDLE hEvent);
ARV_API GC_ERROR        EventGetData                    (EVENT_HANDLE hEvent, void *pBuffer, size_t *piSize, uint64_t iTimeout);
ARV_API GC_ERROR        EventGetDataInfo                (EVENT_HANDLE hEvent, const void *pInBuffer, size_t iInSize,
                                                         EVENT_DATA_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pOutBuffer, size_t *piOutSize);
ARV_API GC_ERROR        EventGetInfo                    (EVENT_HANDLE hEvent, EVENT_INFO_CMD iInfoCmd,
                                                         INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
ARV_API GC_ERROR        EventKill                       (EVENT_HANDLE hEvent);
ARV_API GC_ERROR        GCRegisterEvent                 (EVENTSRC_HANDLE hModule, EVENT_TYPE iEventID, EVENT_HANDLE *phEvent);
ARV_API GC_ERROR        GCUnregisterEvent               (EVENTSRC_HANDLE hModule, EVENT_TYPE iEventID);

typedef GC_ERROR        (*PGCCloseLib)                      (void);
typedef GC_ERROR        (*PGCGetInfo)                       (TL_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );
typedef GC_ERROR        (*PGCGetLastError)                  (GC_ERROR *piErrorCode, char *sErrorText, size_t *piSize);
typedef GC_ERROR        (*PGCInitLib)                       (void);

typedef GC_ERROR        (*PTLClose)                         (TL_HANDLE hSystem);
typedef GC_ERROR        (*PTLGetInfo)                       (TL_HANDLE hSystem, TL_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
typedef GC_ERROR        (*PTLGetInterfaceID)                (TL_HANDLE hSystem, uint32_t iIndex,
                                                             char *sIfaceID, size_t *piSize );
typedef GC_ERROR        (*PTLGetInterfaceInfo)              (TL_HANDLE hSystem, const char *sIfaceID,
                                                             INTERFACE_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
typedef GC_ERROR        (*PTLGetNumInterfaces)              (TL_HANDLE hSystem, uint32_t *piNumIfaces);
typedef GC_ERROR        (*PTLOpen)                          (TL_HANDLE *phSystem);
typedef GC_ERROR        (*PTLOpenInterface)                 (TL_HANDLE hSystem, const char * sIfaceID, IF_HANDLE *phIface);
typedef GC_ERROR        (*PTLUpdateInterfaceList)           (TL_HANDLE hSystem, bool8_t *pbChanged, uint64_t iTimeout);

typedef GC_ERROR        (*PIFClose)                         (IF_HANDLE hIface);
typedef GC_ERROR        (*PIFGetInfo)                       (IF_HANDLE hIface, INTERFACE_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t * piSize);
typedef GC_ERROR        (*PIFGetDeviceID)                   (IF_HANDLE hIface, uint32_t iIndex,
                                                             char *sDeviceID, size_t *piSize );
typedef GC_ERROR        (*PIFGetDeviceInfo)                 (IF_HANDLE hIface, const char *sDeviceID,
                                                             DEVICE_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
typedef GC_ERROR        (*PIFGetNumDevices)                 (IF_HANDLE hIface, uint32_t *piNumDevices);
typedef GC_ERROR        (*PIFOpenDevice)                    (IF_HANDLE hIface, const char * sDeviceID,
                                                             DEVICE_ACCESS_FLAGS iOpenFlag, DEV_HANDLE *phDevice);
typedef GC_ERROR        (*PIFUpdateDeviceList)              (IF_HANDLE hIface, bool8_t *pbChanged, uint64_t iTimeout);
typedef GC_ERROR        (*PIFGetParentTL)                   (IF_HANDLE hIface, TL_HANDLE *phSystem);

typedef GC_ERROR        (*PDevClose)                        (DEV_HANDLE hDevice);
typedef GC_ERROR        (*PDevGetInfo)                      (DEV_HANDLE hDevice, DEVICE_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
typedef GC_ERROR        (*PDevGetDataStreamID)              (DEV_HANDLE hDevice, uint32_t iIndex,
                                                             char *sDataStreamID, size_t *piSize);
typedef GC_ERROR        (*PDevGetNumDataStreams)            (DEV_HANDLE hDevice, uint32_t *piNumDataStreams);
typedef GC_ERROR        (*PDevGetPort)                      (DEV_HANDLE hDevice, PORT_HANDLE *phRemoteDev);
typedef GC_ERROR        (*PDevOpenDataStream)               (DEV_HANDLE hDevice, const char *sDataStreamID,
                                                             DS_HANDLE *phDataStream );
typedef GC_ERROR        (*PDevGetParentIF)                  (DEV_HANDLE hDevice, IF_HANDLE *phIface);

typedef GC_ERROR        (*PDSAllocAndAnnounceBuffer)        (DS_HANDLE hDataStream, size_t iBufferSize,
                                                             void *pPrivate, BUFFER_HANDLE *phBuffer);
typedef GC_ERROR        (*PDSAnnounceBuffer)                (DS_HANDLE hDataStream, void *pBuffer,
                                                             size_t iSize, void *pPrivate, BUFFER_HANDLE *phBuffer);
typedef GC_ERROR        (*PDSAnnounceCompositeBuffer)       (DS_HANDLE hDataStream, size_t iNumSegments, void ** ppSegments,
                                                             size_t *piSizes, void *pPrivate, BUFFER_HANDLE *phBuffer);
typedef GC_ERROR        (*PDSClose)                         (DS_HANDLE hDataStream);
typedef GC_ERROR        (*PDSFlushQueue)                    (DS_HANDLE hDataStream, ACQ_QUEUE_TYPE iOperation);
typedef GC_ERROR        (*PDSGetBufferID)                   (DS_HANDLE hDataStream, uint32_t iIndex, BUFFER_HANDLE *phBuffer);
typedef GC_ERROR        (*PDSGetBufferInfo)                 (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                             BUFFER_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
typedef GC_ERROR        (*PDSGetInfo)                       (DS_HANDLE hDataStream, STREAM_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
typedef GC_ERROR        (*PDSQueueBuffer)                   (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer);
typedef GC_ERROR        (*PDSRevokeBuffer)                  (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                             void **ppBuffer, void **ppPrivate);
typedef GC_ERROR        (*PDSStartAcquisition)              (DS_HANDLE hDataStream, ACQ_START_FLAGS iStartFlags,
                                                             uint64_t iNumToAcquire);
typedef GC_ERROR        (*PDSStopAcquisition)               (DS_HANDLE hDataStream, ACQ_STOP_FLAGS iStopFlags);
typedef GC_ERROR        (*PDSGetBufferChunkData)            (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                             SINGLE_CHUNK_DATA *pChunkData, size_t *piNumChunks);
typedef GC_ERROR        (*PDSGetParentDev)                  (DS_HANDLE hDataStream, DEV_HANDLE *phDevice);
typedef GC_ERROR        (*PDSGetNumBufferParts)             (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t *piNumParts);
typedef GC_ERROR        (*PDSGetBufferPartInfo)             (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t iPartIndex,
                                                             BUFFER_PART_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
typedef GC_ERROR        (*PDSGetBufferInfoStacked)          (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                             DS_BUFFER_INFO_STACKED *pInfoStacked, size_t iNumInfos);
typedef GC_ERROR        (*PDSGetBufferPartInfoStacked)      (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                             DS_BUFFER_PART_INFO_STACKED *pInfoStacked, size_t iNumInfos);
typedef GC_ERROR        (*PDSGetNumFlows)                   (DS_HANDLE hDataStream, uint32_t *piNumFlows);
typedef GC_ERROR        (*PDSGetFlowInfo)                   (DS_HANDLE hDataStream, uint32_t iFlowIndex,
                                                             FLOW_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
typedef GC_ERROR        (*PDSGetNumBufferSegments)          (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                             uint32_t *piNumSegments);
typedef GC_ERROR        (*PDSGetBufferSegmentInfo)          (DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
                                                             uint32_t iSegmentIndex, SEGMENT_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);

typedef GC_ERROR        (*PGCGetPortInfo)                   (PORT_HANDLE hPort, PORT_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
typedef GC_ERROR        (*PGCGetPortURL)                    (PORT_HANDLE hPort, char *sURL, size_t *piSize);
typedef GC_ERROR        (*PGCGetNumPortURLs)                (PORT_HANDLE hPort, uint32_t *piNumURLs);
typedef GC_ERROR        (*PGCGetPortURLInfo)                (PORT_HANDLE hPort, uint32_t iURLIndex, URL_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
typedef GC_ERROR        (*PGCReadPort)                      (PORT_HANDLE hPort, uint64_t iAddress, void *pBuffer, size_t *piSize);
typedef GC_ERROR        (*PGCWritePort)                     (PORT_HANDLE hPort, uint64_t iAddress, const void *pBuffer, size_t *piSize);
typedef GC_ERROR        (*PGCWritePortStacked)              (PORT_HANDLE hPort, PORT_REGISTER_STACK_ENTRY *pEntries,
                                                             size_t * piNumEntries);
typedef GC_ERROR        (*PGCReadPortStacked)               (PORT_HANDLE hPort, PORT_REGISTER_STACK_ENTRY *pEntries,
                                                             size_t * piNumEntries);

typedef GC_ERROR        (*PEventFlush)                      (EVENT_HANDLE hEvent);
typedef GC_ERROR        (*PEventGetData)                    (EVENT_HANDLE hEvent, void *pBuffer, size_t *piSize, uint64_t iTimeout);
typedef GC_ERROR        (*PEventGetDataInfo)                (EVENT_HANDLE hEvent, const void *pInBuffer, size_t iInSize,
                                                             EVENT_DATA_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pOutBuffer, size_t *piOutSize);
typedef GC_ERROR        (*PEventGetInfo)                    (EVENT_HANDLE hEvent, EVENT_INFO_CMD iInfoCmd,
                                                             INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
typedef GC_ERROR        (*PEventKill)                       (EVENT_HANDLE hEvent);
typedef GC_ERROR        (*PGCRegisterEvent)                 (EVENTSRC_HANDLE hModule, EVENT_TYPE iEventID, EVENT_HANDLE *phEvent);
typedef GC_ERROR        (*PGCUnregisterEvent)               (EVENTSRC_HANDLE hModule, EVENT_TYPE iEventID);

G_END_DECLS

#endif
