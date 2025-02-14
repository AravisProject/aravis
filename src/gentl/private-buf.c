/* Aravis - Digital camera library
 *
 * Copyright © 2023 Václav Šmilauer
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors: Václav Šmilauer <eu@doxos.eu>
 */

#include"private.h"

size_t
gentl_buf_size (INFO_DATATYPE type, const void* data)
{
	switch(type) {
		case INFO_DATATYPE_UNKNOWN:
                        return 0;
		case INFO_DATATYPE_STRING:
                        return strlen(data)+1;
		case INFO_DATATYPE_STRINGLIST:
                        {
                                /* TODO: check for correctness */
                                char *s=(char*)data;
                                while(s[0]!='\0') s+=strlen(s)+1;
                                return s-(char*)data;
                        }
                case INFO_DATATYPE_INT16:
                case INFO_DATATYPE_UINT16:
                        return 2;
                case INFO_DATATYPE_INT32:
                case INFO_DATATYPE_UINT32:
                        return 4;
                case INFO_DATATYPE_INT64:
                case INFO_DATATYPE_UINT64:
                case INFO_DATATYPE_FLOAT64:
                        return 8;
                case INFO_DATATYPE_PTR:
                        return sizeof(void*);
                case INFO_DATATYPE_BOOL8:
                        return 1;
                case INFO_DATATYPE_SIZET:
                        return sizeof(size_t);
                case INFO_DATATYPE_BUFFER:
                        return 0;
                case INFO_DATATYPE_PTRDIFF:
                        return sizeof(ptrdiff_t);
                default:
                        return 0;
        }
}


GC_ERROR
gentl_to_buf (INFO_DATATYPE type, void* dst, const void* src, size_t* sz, INFO_DATATYPE *piType)
{
	size_t szSrc;

	if(sz==NULL)
                return GC_ERR_INVALID_PARAMETER;

	if(piType!=NULL)
                *piType=type;

	szSrc=gentl_buf_size(type,src);
	if(sz==0)
                arv_warning_gentl("Datatype %d: zero size?",type);

	/* the call only queries about necessary storage */
	if(dst==NULL) {
		*sz=szSrc;
		arv_trace_gentl ("   (returning required buffer size %ld)",szSrc);
		return GC_ERR_SUCCESS;
	}

	if(szSrc>*sz)
                return GC_ERR_BUFFER_TOO_SMALL;

	*sz=szSrc;
	switch(type) {
		case INFO_DATATYPE_UNKNOWN:
			gentl_err = g_error_new (GENTL_ERROR, GC_ERR_INVALID_PARAMETER,
                                                 "%s: INFO_DATATYPE_UNKNOWN is not allowed.", __FUNCTION__);
			return GC_ERR_INVALID_PARAMETER;
		case INFO_DATATYPE_STRING:
			arv_trace_gentl("   (returning %ld-byte string: '%s')",szSrc,(const char*)src);
			strcpy(dst,src);
			break;
		case INFO_DATATYPE_STRINGLIST:
			GENTL_NYI_DETAIL("%s","INFO_DATATYPE_STRINGLIST");
		case INFO_DATATYPE_INT16:
			*(int16_t*)dst=*(int16_t*)src;
			arv_trace_gentl("   (returning int16_t: %d",*(int16_t*)dst);
			break;
		case INFO_DATATYPE_UINT16:
			*(uint16_t*)dst=*(uint16_t*)src;
			arv_trace_gentl("   (returning uint16_t: %d",*(uint16_t*)dst);
			break;
		case INFO_DATATYPE_INT32:
			*(int32_t*)dst=*(int32_t*)src;
			arv_trace_gentl("   (returning int32_t: %d",*(int32_t*)dst);
			break;
		case INFO_DATATYPE_UINT32:
			*(uint32_t*)dst=*(uint32_t*)src;
			arv_trace_gentl("   (returning uint32_t: %d",*(uint32_t*)dst);
			break;
		case INFO_DATATYPE_INT64:
			*(int64_t*)dst=*(int64_t*)src;
			arv_trace_gentl("   (returning int64_t: %ld",*(int64_t*)dst);
			break;
		case INFO_DATATYPE_UINT64:
			*(uint64_t*)dst=*(uint64_t*)src; break;
			arv_trace_gentl("   (returning uint64_t: %ld",*(uint64_t*)dst);
			break;
		case INFO_DATATYPE_FLOAT64:
			*(double*)dst=*(double*)src; break;
			arv_trace_gentl("   (returning uint64_t: %lf",*(double*)dst);
			break;
		case INFO_DATATYPE_PTR:
			dst=(void*)src;
			arv_trace_gentl("   (returning pointer: %p",dst);
			break;
		case INFO_DATATYPE_BOOL8:
			arv_trace_gentl("   (returning bool8: %u",*(char*)dst);
			*(char*)dst=*(char*)src; break;
			break;
		case INFO_DATATYPE_SIZET:
			*(size_t*)dst=*(size_t*)src; break;
			arv_trace_gentl("   (returning size_t: %ld",*(size_t*)dst);
			break;
		case INFO_DATATYPE_BUFFER:
			GENTL_NYI_DETAIL("%s","INFO_DATATYPE_BUFFER");
			/* memcpy(dst,src,szSrc); break; */
		case INFO_DATATYPE_PTRDIFF:
			*(ptrdiff_t*)dst=*(ptrdiff_t*)src; break;
			arv_trace_gentl("   (returning ptrdiff_t: %td",*(ptrdiff_t*)dst);
			break;
		default:
			arv_warning_gentl("Datatype %d: buffer copy not implemented.",type); return GC_ERR_NOT_IMPLEMENTED;
	}
	return GC_ERR_SUCCESS;
}
