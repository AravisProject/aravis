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

#include <stdio.h>
#include <string.h>

#include <arv.h>
#include <arvgvcpprivate.h>
#include <arvmiscprivate.h>
#include <arvversion.h>

#define ARV_GENTL_PRODUCER_NAME "ProducerAravis-" ARAVIS_API_VERSION ".cti"

/* minimal validating GenICam description used for System and Interface modules respectively */

const char _XML_TL[]=
"<RegisterDescription"
"  xmlns=\"http://www.genicam.org/GenApi/Version_1_1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
"  ModelName=\"GenTL_SystemModule\" VendorName=\"AravisProject\" StandardNameSpace=\"GEV\""
"  SchemaMajorVersion=\"1\" SchemaMinorVersion=\"1\" SchemaSubMinorVersion=\"0\" MajorVersion=\"1\" MinorVersion=\"0\" SubMinorVersion=\"0\""
"  ProductGuid=\"47374761-6ba9-4e90-a088-1f31da726d85\" VersionGuid=\"0f95f943-b926-4a26-83dc-dbb284dc8231\""
"  xsi:schemaLocation=\"http://www.genicam.org/GenApi/Version_1_1 http://www.genicam.org/GenApi/GenApiSchema_Version_1_1.xsd\""
"/>";

const char _XML_IF[]=
"<RegisterDescription"
"  xmlns=\"http://www.genicam.org/GenApi/Version_1_1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
"  ModelName=\"GenTL_InterfaceModule\" VendorName=\"AravisProject\" StandardNameSpace=\"GEV\""
"  SchemaMajorVersion=\"1\" SchemaMinorVersion=\"1\" SchemaSubMinorVersion=\"0\" MajorVersion=\"1\" MinorVersion=\"0\" SubMinorVersion=\"0\""
"  ProductGuid=\"2d44e0b5-40f2-4c0a-ae7d-6d463d6e516b\" VersionGuid=\"672223ba-b3a6-4bec-a381-b799f0bb566c\""
"  xsi:schemaLocation=\"http://www.genicam.org/GenApi/Version_1_1 http://www.genicam.org/GenApi/GenApiSchema_Version_1_1.xsd\""
"/>";

const char _XML_DS[] =
"<RegisterDescription"
"  xmlns=\"http://www.genicam.org/GenApi/Version_1_1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
"  ModelName=\"GenTL_DataStreamModule\" VendorName=\"AravisProject\" StandardNameSpace=\"GEV\""
"  SchemaMajorVersion=\"1\" SchemaMinorVersion=\"1\" SchemaSubMinorVersion=\"0\" MajorVersion=\"1\" MinorVersion=\"0\" SubMinorVersion=\"0\""
"  ProductGuid=\"8c0f7b2e-1d4a-4f1b-9e2c-7a6b5c4d3e2f\" VersionGuid=\"3f2e1d0c-9b8a-4756-8463-2d1c0b9a8f7e\""
"  xsi:schemaLocation=\"http://www.genicam.org/GenApi/Version_1_1 http://www.genicam.org/GenApi/GenApiSchema_Version_1_1.xsd\""
"/>";

#define _GC_CHECK_HANDLE { GENTL_ENSURE_INIT; if(hPort==NULL) return GC_ERR_INVALID_HANDLE; }

static GC_ERROR
gentl_read_local_xml (const char *xml, uint64_t address, void *buffer, size_t *size)
{
	size_t xml_size;
	size_t available;

	if (buffer == NULL || size == NULL)
		return GC_ERR_INVALID_PARAMETER;

	xml_size = strlen (xml) + 1;
	if (address >= xml_size)
		return GC_ERR_INVALID_ADDRESS;

	available = xml_size - (size_t) address;
	if (*size > available)
		*size = available;
	memcpy (buffer, xml + address, *size);

	return GC_ERR_SUCCESS;
}

static GC_ERROR
gentl_to_raw_buffer (const void *source, size_t source_size, void *buffer, size_t *size, INFO_DATATYPE *data_type)
{
	if (size == NULL)
		return GC_ERR_INVALID_PARAMETER;

	if (data_type != NULL)
		*data_type = INFO_DATATYPE_BUFFER;

	if (buffer == NULL) {
		*size = source_size;
		return GC_ERR_SUCCESS;
	}

	if (*size < source_size) {
		*size = source_size;
		return GC_ERR_BUFFER_TOO_SMALL;
	}

	memcpy (buffer, source, source_size);
	*size = source_size;

	return GC_ERR_SUCCESS;
}

GC_ERROR
GCGetInfo (TL_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize)
{
	arv_trace_gentl("%s (iInfoCmd=%d)",__FUNCTION__,iInfoCmd);
	GENTL_ENSURE_INIT;

	switch(iInfoCmd){
		case TL_INFO_ID:
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, "Aravis", piSize, piType);
		case TL_INFO_VENDOR:
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, "Aravis Project", piSize, piType);
		case TL_INFO_MODEL:
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, "Aravis GenTL Producer", piSize, piType);
		case TL_INFO_VERSION:
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, ARAVIS_VERSION, piSize, piType);
		case TL_INFO_TLTYPE:
                        return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,"Mixed",piSize,piType);
		case TL_INFO_NAME:
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, ARV_GENTL_PRODUCER_NAME, piSize, piType);
		case TL_INFO_PATHNAME:
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, ARV_GENTL_PRODUCER_NAME, piSize, piType);
		case TL_INFO_DISPLAYNAME:
			return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, "Aravis GenTL Producer", piSize, piType);
		case TL_INFO_CHAR_ENCODING:
                        {
                                int32_t e=TL_CHAR_ENCODING_UTF8;
                                return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&e,piSize,piType);
                        }
			case TL_INFO_GENTL_VER_MAJOR: {
				uint32_t v = 1;
				return gentl_to_buf (INFO_DATATYPE_UINT32, pBuffer, &v, piSize, piType);
			}
			case TL_INFO_GENTL_VER_MINOR: {
				uint32_t v = 6;
				return gentl_to_buf (INFO_DATATYPE_UINT32, pBuffer, &v, piSize, piType);
			}
		case TL_INFO_CUSTOM_ID:
		default:
			return GC_ERR_INVALID_PARAMETER;
	}
}

GC_ERROR
GCGetLastError (GC_ERROR *piErrorCode, char *sErrText, size_t *piSize)
{
	if (piErrorCode == NULL || piSize == NULL)
		return GC_ERR_INVALID_PARAMETER;
	*piErrorCode = gentl_err == NULL ? GC_ERR_SUCCESS :
					   (gentl_err->domain == GENTL_ERROR ? gentl_err->code : GC_ERR_IO);
	if (gentl_err == NULL) {
		arv_trace_gentl("%s: No error.",__FUNCTION__);
		return gentl_to_buf(INFO_DATATYPE_STRING,sErrText,"No error.",piSize,NULL);
	}
	if(sErrText == NULL) {
		*piSize = strlen(gentl_err->message) + 1;
		arv_trace_gentl ("   (%s: message length is %" G_GSIZE_FORMAT ")", __FUNCTION__, *piSize);
		return GC_ERR_SUCCESS;
	}

	arv_trace_gentl("%s: %s",__FUNCTION__,gentl_err->message);

	return gentl_to_buf(INFO_DATATYPE_STRING,sErrText,gentl_err->message,piSize,NULL);
}

GC_ERROR
GCInitLib (void)
{
	arv_trace_gentl(__FUNCTION__);
	return gentl_init();
}

GC_ERROR
GCCloseLib (void)
{
	arv_trace_gentl(__FUNCTION__);
	return gentl_fini();
}


GC_ERROR
GCReadPort (PORT_HANDLE hPort, uint64_t iAddress, void *pBuffer, size_t *piSize )
{
	if (hPort == NULL)
		return GC_ERR_INVALID_HANDLE;
	if (pBuffer == NULL || piSize == NULL)
		return GC_ERR_INVALID_PARAMETER;
	arv_trace_gentl ("%s (hPort=%s[%p],iAddress=%#" G_GINT64_MODIFIER "x,pBuffer=%p,piSize=%" G_GSIZE_FORMAT ")",
			 __FUNCTION__, G_OBJECT_TYPE_NAME (hPort), hPort, iAddress, pBuffer, *piSize);
	_GC_CHECK_HANDLE;

	if (ARV_IS_TRANSPORT_LAYER (hPort) || ARV_IS_INTERFACE (hPort) || ARV_IS_GENTL_DATA_STREAM (hPort)) {
		const char *xml = ARV_IS_TRANSPORT_LAYER (hPort) ? _XML_TL :
								   (ARV_IS_INTERFACE (hPort) ? _XML_IF : _XML_DS);
		return gentl_read_local_xml (xml, iAddress, pBuffer, piSize);
	}

	if (ARV_IS_CAMERA (hPort) || ARV_IS_DEVICE (hPort)) {
		ArvDevice *dev = ARV_IS_CAMERA (hPort) ? arv_camera_get_device (ARV_CAMERA (hPort)) : ARV_DEVICE (hPort);
		/* Aravis requires a clean GError**; clear the stored GenTL last-error first. */
		g_clear_error (&gentl_err);
		if (ARV_IS_GV_DEVICE (dev) && *piSize == sizeof (guint32) &&
		    (iAddress % sizeof (guint32)) == 0) {
			guint32 value;
			guint32 network_value;

			/* GVCP has dedicated register commands. A number of cameras reject
			 * READ_MEMORY on register addresses even though they expose one
			 * flat GenApi port. Preserve the port's big-endian byte order. */
			if (!arv_device_read_register (dev, iAddress, &value, &gentl_err))
				return GC_ERR_IO;
			network_value = GUINT32_TO_BE (value);
			memcpy (pBuffer, &network_value, sizeof (network_value));
		} else {
			if (*piSize > G_MAXUINT32)
				return GC_ERR_INVALID_PARAMETER;
			if (!arv_device_read_memory (dev, iAddress, *piSize, pBuffer, &gentl_err))
				return GC_ERR_IO;
		}
		return GC_ERR_SUCCESS;
	}
	GENTL_NYI_DETAIL ("only TL/IF/DEV/PORT ports implemented (hPort=%s[%p])",G_OBJECT_TYPE_NAME(hPort),hPort);
	#if 0
		// something like this for device port, in the future
	#endif
}

GC_ERROR
GCWritePort (PORT_HANDLE hPort, uint64_t iAddress, const void *pBuffer, size_t *piSize )
{
	ArvDevice *dev = NULL;

	if (hPort == NULL)
		return GC_ERR_INVALID_HANDLE;
	if (pBuffer == NULL || piSize == NULL)
		return GC_ERR_INVALID_PARAMETER;
	arv_trace_gentl ("%s (hPort=%s[%p],iAddress=%#" G_GINT64_MODIFIER "x,pBuffer=%p,piSize=%" G_GSIZE_FORMAT ")",
			 __FUNCTION__, G_OBJECT_TYPE_NAME (hPort), hPort, iAddress, pBuffer, *piSize);
	_GC_CHECK_HANDLE;

	/* The TL / Interface / DataStream module XMLs are static and read-only. */
	if (ARV_IS_TRANSPORT_LAYER (hPort) || ARV_IS_INTERFACE (hPort) || ARV_IS_GENTL_DATA_STREAM (hPort)) {
		arv_warning_gentl ("%s: write to read-only module port ignored", __FUNCTION__);
		return GC_ERR_ACCESS_DENIED;
	}

	if (ARV_IS_CAMERA (hPort))
		dev = arv_camera_get_device (ARV_CAMERA (hPort));
	else if (ARV_IS_DEVICE (hPort))
		dev = ARV_DEVICE (hPort);

	if (dev == NULL)
		GENTL_NYI_DETAIL ("only DEV ports implemented (hPort=%s[%p])", G_OBJECT_TYPE_NAME (hPort), hPort);

	/* Aravis requires a clean GError**; the GenTL last-error is kept until the
	 * consumer queries it, so clear it before issuing a new device call. */
	g_clear_error (&gentl_err);
	if (ARV_IS_GV_DEVICE (dev) && *piSize == sizeof (guint32) &&
	    (iAddress % sizeof (guint32)) == 0) {
		guint32 network_value;
		guint32 value;

		/* A GenTL port exposes the device byte order. GigE Vision registers
		 * are big-endian on the wire, while arv_device_write_register()
		 * expects a host-endian value. Some cameras, including Hikrobot
		 * MV-CS models, do not acknowledge WRITE_MEMORY for registers. */
		memcpy (&network_value, pBuffer, sizeof (network_value));
		value = GUINT32_FROM_BE (network_value);
		if (!arv_device_write_register (dev, iAddress, value, &gentl_err))
			return GC_ERR_IO;
	} else {
		if (*piSize > G_MAXUINT32)
			return GC_ERR_INVALID_PARAMETER;
		if (!arv_device_write_memory (dev, iAddress, *piSize, pBuffer, &gentl_err))
			return GC_ERR_IO;
	}

	return GC_ERR_SUCCESS;
}

GC_ERROR
GCGetPortURL ( PORT_HANDLE hPort, char *sURL, size_t *piSize )
{
	return GCGetPortURLInfo (hPort, 0, URL_INFO_URL, NULL, sURL, piSize);
}

GC_ERROR
GCGetPortInfo (PORT_HANDLE hPort, PORT_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize)
{
	if (hPort == NULL)
		return GC_ERR_INVALID_HANDLE;
	arv_trace_gentl("%s (hPort=%s[%p],iInfoCmd=%d,pBuffer=%p)",__FUNCTION__,G_OBJECT_TYPE_NAME(hPort),hPort,iInfoCmd,pBuffer);
	_GC_CHECK_HANDLE;

        if (ARV_IS_TRANSPORT_LAYER (hPort) || ARV_IS_INTERFACE (hPort) || ARV_IS_GENTL_DATA_STREAM (hPort)) {
                const char *id = ARV_IS_TRANSPORT_LAYER (hPort) ?  "Aravis-System" :
                        (ARV_IS_INTERFACE (hPort) ? arv_gentl_interface_get_id (hPort) :
                         "Aravis-DataStream");
                const char *model = ARV_IS_TRANSPORT_LAYER (hPort) ? "GenTL System" :
                        (ARV_IS_INTERFACE (hPort) ? "GenTL Interface" : "GenTL DataStream");
                const char *module = ARV_IS_TRANSPORT_LAYER (hPort) ? "TLSystem" :
                        (ARV_IS_INTERFACE (hPort) ? "TLInterface" :
                         "TLDataStream");
                const char *tltype = ARV_IS_TRANSPORT_LAYER (hPort) ? "Mixed" :
                        (ARV_IS_INTERFACE (hPort) ?
                         arv_gentl_tltype_from_protocol (arv_gentl_interface_get_id (hPort)) :
                         (arv_camera_is_uv_device (ARV_GENTL_DATA_STREAM (hPort)->camera) ? "U3V" : "GEV"));
                switch(iInfoCmd)
                {
                        case PORT_INFO_ID:
                                return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, id, piSize, piType);
                        case PORT_INFO_VENDOR:
                                return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, "AravisProject", piSize, piType);
                        case PORT_INFO_MODEL:
                                return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, model, piSize, piType);
                        case PORT_INFO_TLTYPE:
                                return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, tltype, piSize, piType);
                        case PORT_INFO_MODULE:
                                return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, module, piSize, piType);
                        case PORT_INFO_LITTLE_ENDIAN:
                        case PORT_INFO_BIG_ENDIAN:
                                {
                                        bool8_t v = (iInfoCmd == PORT_INFO_LITTLE_ENDIAN) ==
                                                (G_BYTE_ORDER == G_LITTLE_ENDIAN);
                                        return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
                                }
                        case PORT_INFO_ACCESS_READ:
                        case PORT_INFO_ACCESS_WRITE:
                        case PORT_INFO_ACCESS_NA:
                        case PORT_INFO_ACCESS_NI:
                                {
                                        bool8_t v = iInfoCmd == PORT_INFO_ACCESS_READ ? 1 : 0;
                                        return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
                                }
                        case PORT_INFO_VERSION:
                                return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, "1.0.0", piSize, piType);
                        case PORT_INFO_PORTNAME:
                                return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,"Aravis-GenTL",piSize,piType);
                        default:
                                GENTL_NYI_DETAIL ("TL/IF/DS port: hPort=%s[%p], iInfoCmd=%d",
                                                  G_OBJECT_TYPE_NAME (hPort), hPort, iInfoCmd);
                }
        }

	if (ARV_IS_CAMERA (hPort) || ARV_IS_DEVICE (hPort)) {
		ArvDevice *device = ARV_IS_CAMERA (hPort) ?
					    arv_camera_get_device (ARV_CAMERA (hPort)) :
					    ARV_DEVICE (hPort);
		const char *module = ARV_IS_CAMERA (hPort) ? "TLDevice" : "Device";
		switch (iInfoCmd) {
			case PORT_INFO_ID:
				return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, "Device", piSize, piType);
			case PORT_INFO_VENDOR:
				return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, "AravisProject", piSize, piType);
			case PORT_INFO_MODEL:
				return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, G_OBJECT_TYPE_NAME (device), piSize, piType);
			case PORT_INFO_TLTYPE:
				return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer,
						     ARV_IS_GV_DEVICE (device) ? "GEV" : "U3V", piSize, piType);
			case PORT_INFO_MODULE:
				return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, module, piSize, piType);
			case PORT_INFO_LITTLE_ENDIAN:
			case PORT_INFO_BIG_ENDIAN:
                                {
#if ARAVIS_HAS_USB
                                        gboolean is_little_endian = ARV_IS_UV_DEVICE (device);
#else
                                        gboolean is_little_endian = FALSE;
#endif
                                        bool8_t v = (iInfoCmd == PORT_INFO_LITTLE_ENDIAN) == is_little_endian;
                                        return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
                                }
                        case PORT_INFO_ACCESS_READ:
                        case PORT_INFO_ACCESS_WRITE:
                        case PORT_INFO_ACCESS_NA:
                        case PORT_INFO_ACCESS_NI:
                                {
                                        bool8_t v = (iInfoCmd == PORT_INFO_ACCESS_READ ||
                                                     iInfoCmd == PORT_INFO_ACCESS_WRITE) ?
                                                1 :
                                                0;
                                        return gentl_to_buf (INFO_DATATYPE_BOOL8, pBuffer, &v, piSize, piType);
                                }
                        case PORT_INFO_VERSION:
                                return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, "1.0.0", piSize, piType);
                        case PORT_INFO_PORTNAME:
                                /* The port name must match the <Port Name="..."> element in the
                                 * device's GenApi description so that the consumer can connect
                                 * the node map to this port. The standard remote-device port
                                 * name is "Device". */
                                return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, "Device", piSize, piType);
                        default:
                                GENTL_NYI_DETAIL ("DEV port: hPort=%s[%p], iInfoCmd=%d",
                                                  G_OBJECT_TYPE_NAME (hPort), hPort, iInfoCmd);
                }
        }
        GENTL_NYI_DETAIL ("only TL/IF/DEV ports implemented (hPort=%s[%p])", G_OBJECT_TYPE_NAME (hPort), hPort);
}

GC_ERROR
GCRegisterEvent (EVENTSRC_HANDLE hEventSrc, EVENT_TYPE iEventID, EVENT_HANDLE *phEvent)
{
	ArvGentlEvent *ev;
	struct ArvGentlHandleEvents *hev;

	GENTL_ENSURE_INIT;

	if (hEventSrc == NULL || phEvent == NULL)
		return GC_ERR_INVALID_PARAMETER;
	*phEvent = NULL;

	if (iEventID < EVENT_ERROR || iEventID > EVENT_MODULE)
		return GC_ERR_INVALID_PARAMETER;

	arv_trace_gentl ("%s (hEventSrc=%s[%p],iEventID=%d,phEvent=%p)",
			 __FUNCTION__, G_OBJECT_TYPE_NAME (hEventSrc), hEventSrc, iEventID, phEvent);

	/* Only EVENT_NEW_BUFFER on a DataStream is currently delivered. Other
	 * event types (e.g. EVENT_REMOTE_DEVICE, EVENT_MODULE on the device) are
	 * still accepted so consumers like Harvester can register them; they
	 * simply never fire. Returning an error here would abort the consumer. */
	if (iEventID != EVENT_NEW_BUFFER || !ARV_IS_GENTL_DATA_STREAM (hEventSrc)) {
		arv_trace_gentl ("%s: accepting inert event (type=%d, src=%s[%p]); only "
				 "EVENT_NEW_BUFFER on a DataStream is actually delivered",
				 __FUNCTION__, iEventID, G_OBJECT_TYPE_NAME (hEventSrc), hEventSrc);
	}

	/* Get or create the handle events structure */
	g_mutex_lock (&gentl_mutex);
	if (gentl_events == NULL) {
		g_mutex_unlock (&gentl_mutex);
		return GC_ERR_NOT_INITIALIZED;
	}
	hev = g_hash_table_lookup (gentl_events, hEventSrc);
	if (hev == NULL) {
		hev = g_new0 (struct ArvGentlHandleEvents, 1);
		g_hash_table_insert (gentl_events, hEventSrc, hev);
	}

	/* Check if an event of this type already exists */
	if (hev->ev[iEventID] != NULL) {
		arv_warning_gentl ("%s: event type %d already registered for src %p",
				   __FUNCTION__, iEventID, hEventSrc);
		g_mutex_unlock (&gentl_mutex);
		return GC_ERR_RESOURCE_IN_USE;
	}

	/* Create event */
	ev = g_object_new (ARV_TYPE_GENTL_EVENT, NULL);
	ev->event_type = iEventID;
	ev->event_src = hEventSrc;

	/* For EVENT_NEW_BUFFER on DataStream, store reference */
	if (iEventID == EVENT_NEW_BUFFER && ARV_IS_GENTL_DATA_STREAM (hEventSrc)) {
		ev->data_stream = ARV_GENTL_DATA_STREAM (hEventSrc);
		g_object_ref (ev->data_stream);
	}

	hev->ev[iEventID] = ev;
	*phEvent = (EVENT_HANDLE) ev;
	g_mutex_unlock (&gentl_mutex);

	arv_trace_gentl ("   (registered event %p type=%d for src=%p)", ev, iEventID, hEventSrc);

	return GC_ERR_SUCCESS;
}

GC_ERROR
GCUnregisterEvent (EVENTSRC_HANDLE hEventSrc, EVENT_TYPE iEventID)
{
	struct ArvGentlHandleEvents *hev;

	GENTL_ENSURE_INIT;

	if (hEventSrc == NULL)
		return GC_ERR_INVALID_PARAMETER;
	if (iEventID < EVENT_ERROR || iEventID > EVENT_MODULE)
		return GC_ERR_INVALID_PARAMETER;

	arv_trace_gentl ("%s (hEventSrc=%s[%p], iEventID=%d)",
			 __FUNCTION__, G_OBJECT_TYPE_NAME (hEventSrc), hEventSrc, iEventID);

	g_mutex_lock (&gentl_mutex);
	if (gentl_events == NULL) {
		g_mutex_unlock (&gentl_mutex);
		return GC_ERR_NOT_INITIALIZED;
	}
	hev = g_hash_table_lookup (gentl_events, hEventSrc);
	if (hev == NULL || hev->ev[iEventID] == NULL) {
		arv_warning_gentl ("%s: event type %d not registered for src %p",
				   __FUNCTION__, iEventID, hEventSrc);
		g_mutex_unlock (&gentl_mutex);
		return GC_ERR_INVALID_ID;
	}

	/* Kill any pending wait */
	EventKill ((EVENT_HANDLE) hev->ev[iEventID]);

	/* Unref and clear */
	g_clear_object (&hev->ev[iEventID]);

	for (unsigned int i = EVENT_ERROR; i <= EVENT_MODULE; i++) {
		if (hev->ev[i] != NULL) {
			g_mutex_unlock (&gentl_mutex);
			return GC_ERR_SUCCESS;
		}
	}
	g_hash_table_remove (gentl_events, hEventSrc);
	g_mutex_unlock (&gentl_mutex);

	return GC_ERR_SUCCESS;
}

/* GenTL v1.1 */
GC_ERROR
GCGetNumPortURLs (PORT_HANDLE hPort, uint32_t *piNumURLs)
{
	if(hPort==NULL)
                return GC_ERR_INVALID_HANDLE; // this apparently sometimes happens?!

	arv_trace_gentl("%s (hPort=%s[%p])",__FUNCTION__,G_OBJECT_TYPE_NAME(hPort),hPort);
	_GC_CHECK_HANDLE;

	if(piNumURLs==NULL)
                return GC_ERR_INVALID_PARAMETER;

	if (ARV_IS_TRANSPORT_LAYER(hPort)) {
                *piNumURLs=1;
        } else if (ARV_IS_INTERFACE(hPort)) {
		/* The Interface port exposes exactly one URL: its own GenApi
		 * description XML. This is unrelated to the number of devices. */
		*piNumURLs = 1;
	} else if (ARV_IS_GENTL_DATA_STREAM (hPort)) {
		*piNumURLs = 1;
	} else if (ARV_IS_CAMERA (hPort)) {
		*piNumURLs = 1;
	} else if (ARV_IS_DEVICE (hPort)) {
		*piNumURLs = 1;
	} else {
		GENTL_NYI_DETAIL ("only TL/IF/DEV/PORT ports implemented (hPort=%s[%p])",
				  G_OBJECT_TYPE_NAME (hPort), hPort);
	}

	arv_trace_gentl("   (returning %d)",*piNumURLs);

	return GC_ERR_SUCCESS;
}

GC_ERROR
GCGetPortURLInfo (PORT_HANDLE hPort, uint32_t iURLIndex, URL_INFO_CMD iInfoCmd, INFO_DATATYPE *piType,
                  void *pBuffer, size_t *piSize)
{
	_GC_CHECK_HANDLE;
	if (iURLIndex != 0)
		return GC_ERR_INVALID_INDEX;
	arv_trace_gentl ("%s (hPort=%s[%p],iURLIndex=%d,iInfoCmd=%d)",
			 __FUNCTION__, G_OBJECT_TYPE_NAME (hPort), hPort, iURLIndex, iInfoCmd);

	if (ARV_IS_TRANSPORT_LAYER (hPort) || ARV_IS_INTERFACE (hPort) || ARV_IS_GENTL_DATA_STREAM (hPort)) {
		const char *name;
		const char *xml;
		size_t xml_size;

		if (ARV_IS_TRANSPORT_LAYER (hPort)) {
			name = "aravis-gentl-transport.xml";
			xml = _XML_TL;
			xml_size = sizeof (_XML_TL);
		} else if (ARV_IS_INTERFACE (hPort)) {
			name = "aravis-gentl-interface.xml";
			xml = _XML_IF;
			xml_size = sizeof (_XML_IF);
		} else {
			name = "aravis-gentl-datastream.xml";
			xml = _XML_DS;
			xml_size = sizeof (_XML_DS);
		}

		switch(iInfoCmd) {
                        case URL_INFO_URL:
                                {
				char buf[100];

				/* GenICam "Local" URL: address and size are hexadecimal */
				g_snprintf (buf, sizeof (buf), "Local:%s;0;%" G_GSIZE_MODIFIER "x", name, xml_size);

				return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, buf, piSize, piType);
			}
			case URL_INFO_SCHEMA_VER_MAJOR:
			case URL_INFO_SCHEMA_VER_MINOR:
			case URL_INFO_FILE_VER_MAJOR:
			case URL_INFO_FILE_VER_MINOR:
			case URL_INFO_FILE_VER_SUBMINOR: {
				int32_t v = (iInfoCmd == URL_INFO_SCHEMA_VER_MAJOR ||
					     iInfoCmd == URL_INFO_SCHEMA_VER_MINOR ||
					     iInfoCmd == URL_INFO_FILE_VER_MAJOR) ?
						    1 :
						    0;
				return gentl_to_buf (INFO_DATATYPE_INT32, pBuffer, &v, piSize, piType);
			}
			case URL_INFO_FILE_REGISTER_ADDRESS: {
				uint64_t v = 0;
				return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &v, piSize, piType);
			}
			case URL_INFO_FILE_SIZE: {
				uint64_t v = xml_size;
				return gentl_to_buf (INFO_DATATYPE_UINT64, pBuffer, &v, piSize, piType);
			}
			case URL_INFO_SCHEME: {
				int32_t v = 0; /* URL_SCHEME_LOCAL */
				return gentl_to_buf (INFO_DATATYPE_INT32, pBuffer, &v, piSize, piType);
			}
			case URL_INFO_FILENAME:
				return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, name, piSize, piType);
			case URL_INFO_FILE_SHA1_HASH: {
				guint8 digest[20];
				gsize digest_size = sizeof digest;
				GChecksum *checksum = g_checksum_new (G_CHECKSUM_SHA1);
				GC_ERROR result;

				g_checksum_update (checksum, (const guchar *) xml, xml_size);
				g_checksum_get_digest (checksum, digest, &digest_size);
				result = gentl_to_raw_buffer (digest, digest_size, pBuffer, piSize, piType);
				g_checksum_free (checksum);

				return result;
			}
			default:
				return GC_ERR_INVALID_PARAMETER;
		}
	}

	if (ARV_IS_CAMERA (hPort)) {
		switch (iInfoCmd) {
			case URL_INFO_URL: {
				ArvDevice *dev;
				const char *url;

				dev = arv_camera_get_device (ARV_CAMERA (hPort));
				url = arv_dom_document_get_url (ARV_DOM_DOCUMENT (arv_device_get_genicam (dev)));

				return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, url, piSize, piType);
			}
			default:
				GENTL_NYI_DETAIL ("DEV port, iInfoCmd=%d", iInfoCmd);
		}
	}

	if (ARV_IS_DEVICE (hPort)) {
		switch (iInfoCmd) {
			case URL_INFO_URL: {
				const char *url;

				url = arv_dom_document_get_url (ARV_DOM_DOCUMENT (arv_device_get_genicam (ARV_DEVICE (hPort))));

				return gentl_to_buf (INFO_DATATYPE_STRING, pBuffer, url, piSize, piType);
			}
			default:
				GENTL_NYI_DETAIL ("PORT port, iInfoCmd=%d", iInfoCmd);
		}
	}
	GENTL_NYI_DETAIL ("only TL/IF/DEV ports implemented (hPort=%s[%p])", G_OBJECT_TYPE_NAME (hPort), hPort);

#if 0
#if 0
        const char *url=NULL;
#else
        char url[ARV_GVBS_XML_URL_SIZE];
#endif
        char *scheme = NULL;
        char *path=NULL;
        guint64 file_address;
        guint64 file_size;
        ArvDevice* dev;
        ArvGc* genicam;
        ArvGcRegisterDescriptionNode *regs;

		arv_trace_gentl("%s (hPort=%p,iURLIndex=%d,iInfoCmd=%d,pBuffer=%p)",__FUNCTION__,hPort,iURLIndex,iInfoCmd,pBuffer);
		GENTL_ENSURE_INIT;
		dev=arv_open_device(arv_get_device_id(iURLIndex),&gentl_err);
		if(!dev) return GC_ERR_IO;
		genicam=arv_device_get_genicam(dev);
		#if 0
			url=arv_dom_document_get_url(ARV_DOM_DOCUMENT(genicam));
			if(!url) arv_warning_gentl("url is NULL?");
		#else
			if(!arv_device_read_memory(ARV_DEVICE(dev),ARV_GVBS_XML_URL_0_OFFSET,ARV_GVBS_XML_URL_SIZE,url,&gentl_err)) return GC_ERR_IO;
			url[ARV_GVBS_XML_URL_SIZE - 1] = '\0';
		#endif
		regs=ARV_GC_REGISTER_DESCRIPTION_NODE(arv_dom_document_get_document_element(ARV_DOM_DOCUMENT(genicam)));
		/* use this private function, to retrieve scheme, file_address, file_size from the URL */
		arv_parse_genicam_url(url, -1, &scheme, NULL, &path, NULL, NULL, &file_address, &file_size);
		switch(iInfoCmd){
			case URL_INFO_URL: return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,url,piSize,piType);
			case URL_INFO_SCHEMA_VER_MAJOR: {
				int32_t v=arv_gc_register_description_node_get_schema_major_version(regs);
				return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&v,piSize,piType);
			}
			case URL_INFO_SCHEMA_VER_MINOR: {
				int32_t v=arv_gc_register_description_node_get_schema_minor_version(regs);
				return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&v,piSize,piType);
			}
			case URL_INFO_FILE_VER_MAJOR: {
				int32_t v=arv_gc_register_description_node_get_major_version(regs);
				return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&v,piSize,piType);
			}
			case URL_INFO_FILE_VER_MINOR: {
				int32_t v=arv_gc_register_description_node_get_minor_version(regs);
				return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&v,piSize,piType);
			}
			case URL_INFO_FILE_VER_SUBMINOR: {
				int32_t v=arv_gc_register_description_node_get_subminor_version(regs);
				return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&v,piSize,piType);
			}
			case URL_INFO_FILE_SHA1_HASH:    return GC_ERR_NOT_IMPLEMENTED; /* gentl_to_buf(INFO_DATATYPE_BUFFER,pBuffer,...,piSize,piType); */
			case URL_INFO_FILE_REGISTER_ADDRESS: return gentl_to_buf(INFO_DATATYPE_UINT64,pBuffer,&file_address,piSize,piType);
			case URL_INFO_FILE_SIZE:         return gentl_to_buf(INFO_DATATYPE_UINT64,pBuffer,&file_size,piSize,piType);
			case URL_INFO_SCHEME:       return gentl_to_buf(INFO_DATATYPE_INT32,pBuffer,&scheme,piSize,piType);
			case URL_INFO_FILENAME:          return gentl_to_buf(INFO_DATATYPE_STRING,pBuffer,path,piSize,piType);
			default:
				return GC_ERR_INVALID_PARAMETER;
		}
#endif
}

GC_ERROR
GCReadPortStacked (PORT_HANDLE hPort, PORT_REGISTER_STACK_ENTRY *pEntries, size_t *piNumEntries)
{
	size_t requested;
	size_t completed;

	if (hPort == NULL)
		return GC_ERR_INVALID_HANDLE;
	if (piNumEntries == NULL)
		return GC_ERR_INVALID_PARAMETER;
	_GC_CHECK_HANDLE;

	requested = *piNumEntries;
	*piNumEntries = 0;

	if (requested == 0)
		return GC_ERR_SUCCESS;
	if (pEntries == NULL)
		return GC_ERR_INVALID_PARAMETER;

	for (completed = 0; completed < requested; completed++) {
		uint64_t address;
		void *buffer;
		size_t size;
		GC_ERROR error;

		memcpy (&address, &pEntries[completed].Address, sizeof address);
		memcpy (&buffer, &pEntries[completed].Buffer, sizeof buffer);
		memcpy (&size, &pEntries[completed].Size, sizeof size);
		error = GCReadPort (hPort, address, buffer, &size);
		memcpy (&pEntries[completed].Size, &size, sizeof size);
		if (error != GC_ERR_SUCCESS) {
			*piNumEntries = completed;
			return error;
		}
	}

	*piNumEntries = completed;
	return GC_ERR_SUCCESS;
}

GC_ERROR
GCWritePortStacked (PORT_HANDLE hPort, PORT_REGISTER_STACK_ENTRY *pEntries, size_t *piNumEntries)
{
	size_t requested;
	size_t completed;

	if (hPort == NULL)
		return GC_ERR_INVALID_HANDLE;
	if (piNumEntries == NULL)
		return GC_ERR_INVALID_PARAMETER;
	_GC_CHECK_HANDLE;

	requested = *piNumEntries;
	*piNumEntries = 0;

	if (requested == 0)
		return GC_ERR_SUCCESS;
	if (pEntries == NULL)
		return GC_ERR_INVALID_PARAMETER;

	for (completed = 0; completed < requested; completed++) {
		uint64_t address;
		void *buffer;
		size_t size;
		GC_ERROR error;

		memcpy (&address, &pEntries[completed].Address, sizeof address);
		memcpy (&buffer, &pEntries[completed].Buffer, sizeof buffer);
		memcpy (&size, &pEntries[completed].Size, sizeof size);
		error = GCWritePort (hPort, address, buffer, &size);
		memcpy (&pEntries[completed].Size, &size, sizeof size);
		if (error != GC_ERR_SUCCESS) {
			*piNumEntries = completed;
			return error;
		}
	}

	*piNumEntries = completed;
	return GC_ERR_SUCCESS;
}
