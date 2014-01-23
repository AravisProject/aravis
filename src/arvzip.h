/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_ZIP_H
#define ARV_ZIP_H

#include <arvtypes.h>

G_BEGIN_DECLS

ArvZip * 	arv_zip_new 		(const void *buffer, size_t size);
void 		arv_zip_free		(ArvZip *zip);
void *		arv_zip_get_file	(ArvZip *zip, const char *name, size_t *size);
const GSList *	arv_zip_get_file_list	(ArvZip *zip);

const char *	arv_zip_file_get_name			(ArvZipFile *zip_file);
size_t		arv_zip_file_get_uncompressed_size	(ArvZipFile *zip_file);

#define ARV_GUINT32_FROM_LE_PTR(ptr,offset) arv_guint32_from_unaligned_le_ptr (ptr, offset)
#define ARV_GUINT16_FROM_LE_PTR(ptr,offset) arv_guint16_from_unaligned_le_ptr (ptr, offset)

/**
 * arv_guint32_from_unaligned_le_ptr:
 * @ptr: pointer to a little endian 32 bit usigned integer
 * @offset: an offset to add to @ptr
 *
 * Here's an excerpt of the ARM documentation:
 *
 * "Unaligned data access in C and C++ code
 *
 * It can be necessary to access unaligned data in memory, for example,
 * when porting legacy code from a CISC architecture where instructions are
 * available to directly access unaligned data in memory.
 * 
 * On ARMv4 and ARMv5 architectures, and on the ARMv6 architecture
 * depending on how it is configured, care is required when accessing
 * unaligned data in memory, to avoid unexpected results. For example, when
 * a conventional pointer is used to read a word in C or C++ source code,
 * the ARM compiler generates assembly language code that reads the word
 * using an LDR instruction. This works as expected when the address is a
 * multiple of four, for example if it lies on a word boundary. However, if
 * the address is not a multiple of four, the LDR instruction returns a
 * rotated result rather than performing a true unaligned word load.
 * Generally, this rotation is not what the programmer expects.
 * 
 * On ARMv6 and later architectures, unaligned access is fully supported."
 *
 * Returns: a guint32 in machine endianess
 */

static inline guint32
arv_guint32_from_unaligned_le_ptr (const char *ptr, gint32 offset)
{
	guint32 val;

	g_return_val_if_fail (ptr != NULL, 0);

	ptr += offset;

	*((char*)(&val)) = *((char*)ptr);
	*(((char*)(&val))+1) = *(((char*)ptr)+1);
	*(((char*)(&val))+2) = *(((char*)ptr)+2);
	*(((char*)(&val))+3) = *(((char*)ptr)+3);

	return GUINT32_FROM_LE (val);
}

/**
 * arv_guint16_from_unaligned_le_ptr:
 * @ptr: pointer to a little endian 16 bit usigned integer
 * @offset: an offset to add to @ptr
 *
 * See @arv_guint32_from_unaligned_le_ptr.
 *
 * Returns: a guint16 in machine endianess
 */

static inline guint16
arv_guint16_from_unaligned_le_ptr (const char *ptr, gint16 offset)
{
	guint16 val;

	g_return_val_if_fail (ptr != NULL, 0);

	ptr += offset;

	*((char*)(&val)) = *((char*)ptr);
	*(((char*)(&val))+1) = *(((char*)ptr)+1);

	return GUINT16_FROM_LE (val);
}

G_END_DECLS

#endif
