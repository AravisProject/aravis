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

#include"private.h"

#include<arvsystem.h>
#include<arvinterface.h>

#define _TL_CHECK_HANDLE { GENTL_ENSURE_INIT; if(hTL==NULL || !ARV_IS_TRANSPORT_LAYER(hTL)) return GC_ERR_INVALID_HANDLE; if(gentl_transport_layer==NULL) return GC_ERR_NOT_INITIALIZED; }

GC_API
TLOpen (TL_HANDLE *phTL)
{
	arv_trace_gentl(__FUNCTION__);
	GENTL_ENSURE_INIT;

	if(phTL==NULL)
                return GC_ERR_INVALID_PARAMETER;
	if (gentl_transport_layer!=NULL)
                return GC_ERR_RESOURCE_IN_USE;

        arv_disable_interface ("GenTL");

	gentl_transport_layer= g_object_new (ARV_TYPE_TRANSPORT_LAYER,NULL);
	*phTL=(void*)gentl_transport_layer;

	return GC_ERR_SUCCESS;
}

GC_API
TLClose (TL_HANDLE hTL)
{
	arv_trace_gentl("%s (phTL=%s[%p])",__FUNCTION__,G_OBJECT_TYPE_NAME(hTL),hTL);
	_TL_CHECK_HANDLE;

	g_object_unref(gentl_transport_layer);
	gentl_transport_layer=NULL;

	return GC_ERR_SUCCESS;
}

GC_API
TLGetInfo (TL_HANDLE hTL, TL_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize)
{
        GENTL_NYI;
}

GC_API
TLGetNumInterfaces (TL_HANDLE hTL, uint32_t *piNumIfaces)
{
	arv_trace_gentl("%s (hTL=%s[%p])",__FUNCTION__,G_OBJECT_TYPE_NAME(hTL),hTL);
	_TL_CHECK_HANDLE;

	if(!piNumIfaces)
                return GC_ERR_INVALID_PARAMETER;

	*piNumIfaces=arv_get_n_interfaces();
	arv_trace_gentl("   → %d interfaces",*piNumIfaces);

	return GC_ERR_SUCCESS;
}

GC_API
TLGetInterfaceID (TL_HANDLE hTL, uint32_t iIndex,  char *sID, size_t *piSize)
{
	arv_trace_gentl("%s (hTL=%s[%p],iIndex=%d,sID=%p,piSize=%p)",__FUNCTION__,G_OBJECT_TYPE_NAME(hTL),hTL,iIndex,sID,piSize);
	_TL_CHECK_HANDLE;

	if(iIndex>=arv_get_n_interfaces())
                return GC_ERR_INVALID_INDEX;

	return gentl_to_buf(INFO_DATATYPE_STRING,sID,arv_get_interface_id(iIndex),piSize,NULL);
}

GC_API
TLGetInterfaceInfo (TL_HANDLE hTL, const char *sIfaceID, INTERFACE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType,
                    void *pBuffer, size_t *piSize)
{
	ArvInterface *iface;

	arv_trace_gentl("%s (hTL=%s[%p], sIfaceID=%s, iInfoCmd=%d)",__FUNCTION__,G_OBJECT_TYPE_NAME(hTL),hTL,sIfaceID,iInfoCmd);
	_TL_CHECK_HANDLE;

	iface=arv_get_interface_by_id(sIfaceID);
	if (!iface)
                return GC_ERR_INVALID_ID;

	switch (iInfoCmd) {
		case INTERFACE_INFO_ID:
		case INTERFACE_INFO_DISPLAYNAME:
		case INTERFACE_INFO_TLTYPE:
			return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,
                                            arv_interface_get_device_protocol(iface,/*unused*/0),piSize,piType);
		default:
			return GC_ERR_INVALID_PARAMETER;
	}
}

GC_API
TLOpenInterface (TL_HANDLE hTL, const char *sIfaceID, IF_HANDLE *phIface)
{
	ArvInterface* iface;

	/* TODO: return GC_ERR_RESOURCE_IN_USE if the iface was already open via TLOpenInterface */
	arv_trace_gentl("%s (hTL=%s[%p], sIfaceID=%p, phIface=%p)",__FUNCTION__,G_OBJECT_TYPE_NAME(hTL),hTL,sIfaceID,phIface);
	_TL_CHECK_HANDLE;

	if (sIfaceID==NULL || phIface==NULL)
                return GC_ERR_INVALID_PARAMETER;

	iface=arv_get_interface_by_id(sIfaceID);
	if (!iface)
                return GC_ERR_INVALID_ID;

	*phIface=(void*)iface;
	arv_interface_update_device_list(iface);

	return GC_ERR_SUCCESS;
}

GC_API
TLUpdateInterfaceList (TL_HANDLE hTL, bool8_t *pbChanged, uint64_t iTimeout)
{
	arv_trace_gentl("%s (hTL=%s[%p], pbChanged=%p, iTimeout=%ld)",__FUNCTION__,G_OBJECT_TYPE_NAME(hTL),hTL,pbChanged,iTimeout);
	_TL_CHECK_HANDLE;

	if(pbChanged)
                pbChanged=0;

	return GC_ERR_SUCCESS;
}
