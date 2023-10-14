#include"private.h"
GC_API DSAnnounceBuffer        ( DS_HANDLE hDataStream, void *pBuffer, size_t iSize, void *pPrivate, BUFFER_HANDLE *phBuffer ){ GENTL_NYI; }
GC_API DSAllocAndAnnounceBuffer( DS_HANDLE hDataStream, size_t iSize, void *pPrivate, BUFFER_HANDLE *phBuffer ){ GENTL_NYI; }
GC_API DSFlushQueue            ( DS_HANDLE hDataStream, ACQ_QUEUE_TYPE iOperation ){ GENTL_NYI; }
GC_API DSStartAcquisition      ( DS_HANDLE hDataStream, ACQ_START_FLAGS iStartFlags, uint64_t iNumToAcquire ){ GENTL_NYI; }
GC_API DSStopAcquisition       ( DS_HANDLE hDataStream, ACQ_STOP_FLAGS iStopFlags ){ GENTL_NYI; }
GC_API DSGetInfo               ( DS_HANDLE hDataStream, STREAM_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize ){ GENTL_NYI; }
GC_API DSGetBufferID           ( DS_HANDLE hDataStream, uint32_t iIndex, BUFFER_HANDLE *phBuffer ){ GENTL_NYI; }
GC_API DSClose                 ( DS_HANDLE hDataStream ){ GENTL_NYI; }
GC_API DSRevokeBuffer          ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, void **pBuffer, void **pPrivate ){ GENTL_NYI; }
GC_API DSQueueBuffer           ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer ){ GENTL_NYI; }
GC_API DSGetBufferInfo         ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, BUFFER_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize ){ GENTL_NYI; }
/* GenTL v1.3 */
GC_API DSGetBufferChunkData    ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, SINGLE_CHUNK_DATA *pChunkData, size_t *piNumChunks ){ GENTL_NYI; }
/* GenTL v1.4 */
GC_API DSGetParentDev          ( DS_HANDLE hDataStream, DEV_HANDLE *phDevice ){ GENTL_NYI; }
/* GenTL v1.5 */
GC_API DSGetNumBufferParts     ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t *piNumParts ){ GENTL_NYI; }
GC_API DSGetBufferPartInfo     ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t iPartIndex, BUFFER_PART_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize ){ GENTL_NYI; }
/* GenTL v1.6 */
GC_API DSAnnounceCompositeBuffer ( DS_HANDLE hDataStream, size_t iNumSegments, void **ppSegments, size_t *piSizes, void *pPrivate, BUFFER_HANDLE *phBuffer ){ GENTL_NYI; }
GC_API DSGetBufferInfoStacked  ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, DS_BUFFER_INFO_STACKED *pInfoStacked, size_t iNumInfos ){ GENTL_NYI; }
GC_API DSGetBufferPartInfoStacked( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, DS_BUFFER_PART_INFO_STACKED *pInfoStacked, size_t iNumInfos ){ GENTL_NYI; }
GC_API DSGetNumFlows           ( DS_HANDLE hDataStream, uint32_t *piNumFlows ){ GENTL_NYI; }
GC_API DSGetFlowInfo           ( DS_HANDLE hDataStream, uint32_t iFlowIndex, FLOW_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize ){ GENTL_NYI; }
GC_API DSGetNumBufferSegments  ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t *piNumSegments ){ GENTL_NYI; }
GC_API DSGetBufferSegmentInfo  ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t iSegmentIndex, SEGMENT_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize ){ GENTL_NYI; }

