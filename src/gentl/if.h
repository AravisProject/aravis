#include"internal.h"
GC_API IFClose                 ( IF_HANDLE hIface );
GC_API IFGetInfo               ( IF_HANDLE hIface, INTERFACE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );

GC_API IFGetNumDevices         ( IF_HANDLE hIface, uint32_t *piNumDevices );
GC_API IFGetDeviceID           ( IF_HANDLE hIface, uint32_t iIndex, char *sIDeviceID, size_t *piSize );
GC_API IFUpdateDeviceList      ( IF_HANDLE hIface, bool8_t *pbChanged, uint64_t iTimeout );
GC_API IFGetDeviceInfo         ( IF_HANDLE hIface, const char *sDeviceID, DEVICE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );
GC_API IFOpenDevice            ( IF_HANDLE hIface, const char *sDeviceID, DEVICE_ACCESS_FLAGS iOpenFlag, DEV_HANDLE *phDevice );
/* GenTL v1.4 */
GC_API IFGetParentTL           ( IF_HANDLE hIface, TL_HANDLE *phSystem ); 

