#include"private.h"
GC_API DevGetPort              ( DEV_HANDLE hDevice, PORT_HANDLE *phRemoteDevice ){ GENTL_NYI; }
GC_API DevGetNumDataStreams    ( DEV_HANDLE hDevice, uint32_t *piNumDataStreams ){ GENTL_NYI; }
GC_API DevGetDataStreamID      ( DEV_HANDLE hDevice, uint32_t iIndex, char *sDataStreamID, size_t *piSize ){ GENTL_NYI; }
GC_API DevOpenDataStream       ( DEV_HANDLE hDevice, const char *sDataStreamID, DS_HANDLE *phDataStream ){ GENTL_NYI; }
GC_API DevGetInfo              ( DEV_HANDLE hDevice, DEVICE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize ){ GENTL_NYI; }
GC_API DevClose                ( DEV_HANDLE hDevice ){ GENTL_NYI; }
/* GenTL v1.4 */
GC_API DevGetParentIF          ( DEV_HANDLE hDevice, IF_HANDLE *phIface ){ GENTL_NYI; }

