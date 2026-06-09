/* Aravis - Digital camera library
 *
 * Copyright © 2023 Václav Šmilauer
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
 * Authors: Václav Šmilauer <eu@doxos.eu>
 */

#include "private.h"
#include <arvinterface.h>
#include <arvcamera.h>

#define _IF_CHECK_HANDLE GENTL_ENSURE_INIT; if(hIface==NULL || !(ARV_IS_INTERFACE(hIface))) return GC_ERR_INVALID_HANDLE;

static char **
gentl_copy_device_ids (ArvInterface *iface)
{
	unsigned int n_devices;
	char **ids;
	unsigned int i;

	n_devices = arv_interface_get_n_devices (iface);
	ids = g_new0 (char *, n_devices + 1);
	for (i = 0; i < n_devices; i++)
		ids[i] = g_strdup (arv_interface_get_device_id (iface, i));

	return ids;
}

static gboolean
gentl_device_ids_equal (char **first, char **second)
{
	unsigned int i;

	for (i = 0; first[i] != NULL && second[i] != NULL; i++) {
		if (g_strcmp0 (first[i], second[i]) != 0)
			return FALSE;
	}

	return first[i] == NULL && second[i] == NULL;
}

GC_ERROR
IFClose (IF_HANDLE hIface)
{
	_IF_CHECK_HANDLE;
	arv_trace_gentl("%s (hIface=%s[%p])",__FUNCTION__,G_OBJECT_TYPE_NAME(hIface),hIface);

	/* ArvInterface* is singleton, no need to free it */
	return GC_ERR_SUCCESS;
}

GC_ERROR
IFGetInfo (IF_HANDLE hIface, INTERFACE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize)
{
	_IF_CHECK_HANDLE;
	arv_trace_gentl("%s (hIface=%s[%p], iInfoCmd=%d)",__FUNCTION__,G_OBJECT_TYPE_NAME(hIface),hIface,iInfoCmd);

	switch (iInfoCmd) {
		case INTERFACE_INFO_ID:
		case INTERFACE_INFO_DISPLAYNAME:
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer,
					     arv_gentl_interface_get_id (hIface), piSize, piType);
		case INTERFACE_INFO_TLTYPE:
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer,
					     arv_gentl_tltype_from_protocol (arv_gentl_interface_get_id (hIface)),
					     piSize, piType);
		default:
			return GC_ERR_INVALID_PARAMETER;
	}
}

GC_ERROR
IFGetNumDevices (IF_HANDLE hIface, uint32_t *piNumDevices)
{
	_IF_CHECK_HANDLE;
	arv_trace_gentl("%s (hIface=%s[%p], piNumDevices=%p)",
                        __FUNCTION__,G_OBJECT_TYPE_NAME(hIface),hIface,piNumDevices);

	if(piNumDevices==NULL)
                return GC_ERR_INVALID_PARAMETER;

	*piNumDevices=arv_interface_get_n_devices(hIface);
	arv_trace_gentl("    (returning %d)",*piNumDevices);

	return GC_ERR_SUCCESS;
}

GC_ERROR
IFGetDeviceID (IF_HANDLE hIface, uint32_t iIndex, char *sIDeviceID, size_t *piSize)
{
	_IF_CHECK_HANDLE;
	arv_trace_gentl("%s (hIface=%s[%p], iIndex=%d, sIDeviceId=%p, piSize=%p)",
                        __FUNCTION__,G_OBJECT_TYPE_NAME(hIface),hIface,iIndex,sIDeviceID,piSize);

	if (iIndex >= arv_interface_get_n_devices (hIface))
		return GC_ERR_INVALID_INDEX;

	return gentl_to_buf(INFO_DATATYPE_STRING,sIDeviceID,arv_interface_get_device_id(hIface,iIndex),piSize,NULL);
}

GC_ERROR
IFUpdateDeviceList (IF_HANDLE hIface, bool8_t *pbChanged, uint64_t iTimeout)
{
	char **previous_ids = NULL;
	char **current_ids = NULL;

	_IF_CHECK_HANDLE;
	arv_trace_gentl ("%s (hIface=%s[%p], pbChanged=%p, iTimeout=%" G_GUINT64_FORMAT ")",
			 __FUNCTION__, G_OBJECT_TYPE_NAME (hIface), hIface, pbChanged, (guint64) iTimeout);

	previous_ids = gentl_copy_device_ids (hIface);
	arv_interface_update_device_list (hIface);
	current_ids = gentl_copy_device_ids (hIface);

	if (pbChanged != NULL)
		*pbChanged = gentl_device_ids_equal (previous_ids, current_ids) ? 0 : 1;

        g_strfreev (previous_ids);
        g_strfreev (current_ids);

	return GC_ERR_SUCCESS;
}

GC_ERROR
IFGetDeviceInfo (IF_HANDLE hIface, const char *sDeviceID, DEVICE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType,
                 void *pBuffer, size_t *piSize)
{
	gint i, iDev;

	_IF_CHECK_HANDLE;
	arv_trace_gentl("%s (hIface=%s[%p], sDeviceID=%s, iInfoCmd=%d, piSize=%p)",
                        __FUNCTION__,G_OBJECT_TYPE_NAME(hIface),hIface,sDeviceID,iInfoCmd,piSize);

	iDev=-1;
	for (i=0; i<arv_interface_get_n_devices(hIface); i++) {
		if (g_strcmp0(arv_interface_get_device_id(hIface,i),sDeviceID)==0) {
                        iDev=i;
                        break;
                }
	}
	if (iDev<0)
                return GC_ERR_INVALID_ID;

	#if 0
		/* do we need device objects for some of the data (like version, access status, timestamp frequency? must be freed again. */
		ArvDevice* dev=arv_interface_open_device(hIface,sDeviceID,&gentl_err);
		if(!dev) return GC_ERR_INVALID_ID;
	#endif

	switch(iInfoCmd) {
		case DEVICE_INFO_ID:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            arv_interface_get_device_id(hIface,iDev),piSize,piType);
		case DEVICE_INFO_VENDOR:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            arv_interface_get_device_vendor(hIface,iDev),piSize,piType);
		case DEVICE_INFO_MODEL:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            arv_interface_get_device_model(hIface,iDev),piSize,piType);
		case DEVICE_INFO_TLTYPE:
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer,
					     arv_gentl_tltype_from_protocol (
						     arv_interface_get_device_protocol (hIface, iDev)),
					     piSize, piType);
		case DEVICE_INFO_DISPLAYNAME:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            /* ?? */arv_interface_get_device_id(hIface,iDev),piSize,piType);
		case DEVICE_INFO_USER_DEFINED_NAME:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            /* ?? */arv_interface_get_device_id(hIface,iDev),piSize,piType);
		case DEVICE_INFO_SERIAL_NUMBER:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            arv_interface_get_device_serial_nbr(hIface,iDev),piSize,piType);
		/* TODO: where is this supposed to come from? */
		case DEVICE_INFO_VERSION:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,"1.0",piSize,piType);
		case DEVICE_INFO_ACCESS_STATUS:
                        {
                                int32_t rw=DEVICE_ACCESS_STATUS_READWRITE;
                                return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&rw,piSize,piType);
                        }
		case DEVICE_INFO_TIMESTAMP_FREQUENCY:
		default:
			GENTL_NYI_DETAIL("iInfoCmd=%d",iInfoCmd);
	}
}

GC_ERROR
IFOpenDevice (IF_HANDLE hIface, const char *sDeviceID, DEVICE_ACCESS_FLAGS iOpenFlag, DEV_HANDLE *phDevice)
{
	ArvCamera* cam;

	_IF_CHECK_HANDLE;
	if (sDeviceID == NULL || phDevice == NULL)
		return GC_ERR_INVALID_PARAMETER;
	*phDevice = NULL;
	if (iOpenFlag != DEVICE_ACCESS_CONTROL &&
	    iOpenFlag != DEVICE_ACCESS_EXCLUSIVE)
		return GC_ERR_ACCESS_DENIED;
	arv_trace_gentl("%s (hIface=%s[%p], sDeviceID=%s, iOpenFlag=%d)",
                        __FUNCTION__,G_OBJECT_TYPE_NAME(hIface),hIface,sDeviceID,iOpenFlag);

	g_clear_error (&gentl_err);
	cam=arv_camera_new(sDeviceID,&gentl_err);
	if(!cam)
                return GC_ERR_IO;

	g_object_set_data (G_OBJECT (cam), "arv-gentl-parent-interface", hIface);
	*phDevice=cam;
	return GC_ERR_SUCCESS;
}

/* GenTL v1.4 */
GC_ERROR
IFGetParentTL (IF_HANDLE hIface, TL_HANDLE *phSystem)
{
	_IF_CHECK_HANDLE;
	if (phSystem == NULL)
		return GC_ERR_INVALID_PARAMETER;
	if (gentl_transport_layer == NULL)
		return GC_ERR_NOT_INITIALIZED;

	*phSystem = (TL_HANDLE) gentl_transport_layer;
	return GC_ERR_SUCCESS;
}
