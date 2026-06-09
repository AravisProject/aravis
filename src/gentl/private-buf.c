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

size_t
gentl_buf_size (INFO_DATATYPE type, const void *data)
{
	switch (type) {
		case INFO_DATATYPE_UNKNOWN:
			return 0;
		case INFO_DATATYPE_STRING:
			return (data == NULL ? 0 : strlen (data)) + 1;
		case INFO_DATATYPE_STRINGLIST: {
			const char *string = data;

			if (string == NULL)
				return 1;

			while (*string != '\0')
				string += strlen (string) + 1;

			return (size_t) (string - (const char *) data) + 1;
		}
		case INFO_DATATYPE_INT16:
		case INFO_DATATYPE_UINT16:
			return sizeof (uint16_t);
		case INFO_DATATYPE_INT32:
		case INFO_DATATYPE_UINT32:
			return sizeof (uint32_t);
		case INFO_DATATYPE_INT64:
		case INFO_DATATYPE_UINT64:
			return sizeof (uint64_t);
		case INFO_DATATYPE_FLOAT64:
			return sizeof (double);
		case INFO_DATATYPE_PTR:
			return sizeof (void *);
		case INFO_DATATYPE_BOOL8:
			return sizeof (bool8_t);
		case INFO_DATATYPE_SIZET:
			return sizeof (size_t);
		case INFO_DATATYPE_PTRDIFF:
			return sizeof (ptrdiff_t);
		case INFO_DATATYPE_BUFFER:
		default:
			return 0;
	}
}

GC_ERROR
gentl_to_buf (INFO_DATATYPE type, void *dst, const void *src, size_t *size, INFO_DATATYPE *data_type)
{
	const void *copy_source = src;
	size_t source_size;

	if (size == NULL)
		return GC_ERR_INVALID_PARAMETER;

	if (data_type != NULL)
		*data_type = type;

	source_size = gentl_buf_size (type, src);
	if (source_size == 0)
		arv_warning_gentl ("Datatype %d: zero size", type);

	if (dst == NULL) {
		*size = source_size;
		arv_trace_gentl ("   (returning required buffer size %" G_GSIZE_FORMAT ")", source_size);
		return GC_ERR_SUCCESS;
	}

	if (source_size > *size) {
		*size = source_size;
		return GC_ERR_BUFFER_TOO_SMALL;
	}

	*size = source_size;
	switch (type) {
		case INFO_DATATYPE_UNKNOWN:
			g_clear_error (&gentl_err);
			gentl_err = g_error_new (GENTL_ERROR, GC_ERR_INVALID_PARAMETER,
						 "%s: INFO_DATATYPE_UNKNOWN is not allowed", __FUNCTION__);
			return GC_ERR_INVALID_PARAMETER;
		case INFO_DATATYPE_STRING:
			copy_source = src != NULL ? src : "";
			arv_trace_gentl ("   (returning %" G_GSIZE_FORMAT "-byte string: '%s')",
					 source_size, (const char *) copy_source);
			memcpy (dst, copy_source, source_size);
			return GC_ERR_SUCCESS;
		case INFO_DATATYPE_STRINGLIST:
			copy_source = src != NULL ? src : "";
			memcpy (dst, copy_source, source_size);
			return GC_ERR_SUCCESS;
		case INFO_DATATYPE_INT16:
		case INFO_DATATYPE_UINT16:
		case INFO_DATATYPE_INT32:
		case INFO_DATATYPE_UINT32:
		case INFO_DATATYPE_INT64:
		case INFO_DATATYPE_UINT64:
		case INFO_DATATYPE_FLOAT64:
		case INFO_DATATYPE_PTR:
		case INFO_DATATYPE_BOOL8:
		case INFO_DATATYPE_SIZET:
		case INFO_DATATYPE_PTRDIFF:
			if (src == NULL)
				return GC_ERR_INVALID_PARAMETER;
			/* Consumers are allowed to provide byte-aligned storage. */
			memcpy (dst, src, source_size);
			return GC_ERR_SUCCESS;
		case INFO_DATATYPE_BUFFER:
			GENTL_NYI_DETAIL ("%s", "INFO_DATATYPE_BUFFER");
		default:
			arv_warning_gentl ("Datatype %d: buffer copy not implemented", type);
			return GC_ERR_NOT_IMPLEMENTED;
	}
}
