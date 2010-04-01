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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 *
 * http://www.pkware.com/documents/casestudies/APPNOTE.TXT
 *
 * Heavily inspired by the Imposter code from Gürer Özen <madcat@e-kolay.net>. 
 *
 */

#include <arvzip.h>
#include <arvdebug.h>
#include <string.h>
#include <zlib.h>

#define GUINT32_FROM_LE_PTR(ptr) GUINT32_FROM_LE(*((guint32 *)(ptr)))
#define GUINT16_FROM_LE_PTR(ptr) GUINT16_FROM_LE(*((guint16 *)(ptr)))

struct _ArvZipFile {
        char *name;

        size_t compressed_size;
        size_t uncompressed_size;
        ptrdiff_t offset;
};

const char *
arv_zip_file_get_name (ArvZipFile *zip_file)
{
	g_return_val_if_fail (zip_file != NULL, NULL);

	return zip_file->name;
}

size_t
arv_zip_file_get_uncompressed_size (ArvZipFile *zip_file)
{
	g_return_val_if_fail (zip_file != NULL, 0);

	return zip_file->uncompressed_size;
}

struct _ArvZip {
	const unsigned char *buffer;
	size_t buffer_size;

	GSList *files;

        size_t header_size;
        ptrdiff_t directory_position;
        size_t directory_size;
        ptrdiff_t directory_offset;
        guint n_files;
};

static void
arv_zip_build_file_list (ArvZip *zip)
{
        ArvZipFile *zip_file;
        const void *ptr;
        ptrdiff_t offset;
        int i;

        offset = zip->directory_offset;

	for (i = 0; i < zip->n_files; i++) {
		ptr = zip->buffer + zip->header_size + offset;
		if (GUINT32_FROM_LE_PTR (ptr) != 0x02014b50) return;

		zip_file = g_new0 (ArvZipFile, 1);
                zip_file->compressed_size = GUINT32_FROM_LE_PTR (ptr + 20);
                zip_file->uncompressed_size = GUINT32_FROM_LE_PTR (ptr + 24);
                zip_file->offset = GUINT32_FROM_LE_PTR (ptr + 42);
		zip_file->name = g_strndup ((char *) (ptr + 46), GUINT16_FROM_LE_PTR (ptr + 28));

		arv_debug ("zip", "[Zip::list_files] %s", zip_file->name);

		zip->files = g_slist_prepend (zip->files, zip_file);

                offset += 0x2e +
			GUINT16_FROM_LE_PTR (ptr + 28) + /* filename size */
			GUINT16_FROM_LE_PTR (ptr + 30) + /* extra field */
			GUINT16_FROM_LE_PTR (ptr + 32);  /* file comment */
	}
}

static ArvZipFile *
arv_zip_find_file (ArvZip *zip, const char *name)
{
        ArvZipFile *zip_file;
	GSList *iter;

	for (iter = zip->files; iter != NULL; iter = iter->next) {
		zip_file = iter->data;
		if (g_strcmp0 (zip_file->name, name) == 0)
			return zip_file;
	}

	return NULL;
}

static ptrdiff_t
arv_zip_get_file_data (ArvZip *zip, ArvZipFile *zip_file)
{
        const void *ptr;

	ptr = zip->buffer + zip_file->offset + zip->header_size;

        if (GUINT32_FROM_LE_PTR (ptr) != 0x04034b50) return -1;

	return zip_file->offset + zip->header_size +
		GUINT16_FROM_LE_PTR (ptr + 26) +
		GUINT16_FROM_LE_PTR (ptr + 28) + 30;
}

ArvZip *
arv_zip_new (const void *buffer, size_t size)
{
        ArvZip *zip;
        const void *ptr;
        int i;
	gboolean directory_found;

	g_return_val_if_fail (buffer != NULL, NULL);
	g_return_val_if_fail (size > 0, NULL);

        zip = g_new0 (ArvZip, 1);
	zip->buffer = buffer;
	zip->buffer_size = size;

	directory_found = FALSE;
        for (i = zip->buffer_size - 4; i > 0; i--) {
                if (zip->buffer[i] == 0x50 &&
		    zip->buffer[i+1] == 0x4b &&
		    zip->buffer[i+2] == 0x05 &&
		    zip->buffer[i+3] == 0x06) {
			zip->directory_position = i;
			directory_found = TRUE;
                        break;
                }
        }
	if (!directory_found)
		return zip;

	ptr = zip->buffer + zip->directory_position;
        zip->n_files = GUINT16_FROM_LE_PTR (ptr + 10);
        if (GUINT16_FROM_LE_PTR (ptr + 8) != zip->n_files) {
		zip->n_files = 0;
		return zip;
        }

        zip->directory_size = GUINT32_FROM_LE_PTR (ptr + 12);
        zip->directory_offset = GUINT32_FROM_LE_PTR (ptr + 16);
        zip->header_size = zip->directory_position - (zip->directory_offset + zip->directory_size);

        arv_zip_build_file_list (zip);

        return zip;
}

void
arv_zip_free (ArvZip *zip)
{
        ArvZipFile *zip_file;
	GSList *iter;

	g_return_if_fail (zip != NULL);

	for (iter = zip->files; iter != NULL; iter = iter->next) {
		zip_file = iter->data;
		g_free (zip_file->name);
		g_free (zip_file);
	}
	g_slist_free (zip->files);
	g_free (zip);
}

const GSList *
arv_zip_get_file_list (ArvZip *zip)
{
	g_return_val_if_fail (zip != NULL, NULL);

	return zip->files;
}

void *
arv_zip_get_file (ArvZip *zip, const char *name, size_t *size)
{
        ArvZipFile *zip_file;
	void *output_buffer;
	ptrdiff_t offset;

	if (size != NULL)
		*size = 0;

	g_return_val_if_fail (zip != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

        zip_file = arv_zip_find_file (zip, name);
        if (!zip_file)
		return NULL;

        offset = arv_zip_get_file_data (zip, zip_file);
	if (offset < 0)
		return NULL;

	output_buffer = g_malloc (zip_file->uncompressed_size);
	if (output_buffer == NULL)
		return NULL;

        if (zip_file->compressed_size < zip_file->uncompressed_size) {
                z_stream zs;
                zs.zalloc = NULL;
                zs.zfree = NULL;
                zs.opaque = NULL;
                zs.next_in = (void *) &zip->buffer[offset];
                zs.avail_in = zip_file->compressed_size;
                zs.next_out = output_buffer;
                zs.avail_out = zip_file->uncompressed_size;
                inflateInit2 (&zs, -MAX_WBITS);
                inflate (&zs, Z_FINISH);
                inflateEnd (&zs);
        } else
		memcpy (output_buffer, zip->buffer + offset, zip_file->uncompressed_size);

	if (size != NULL)
		*size = zip_file->uncompressed_size;

        return output_buffer;
}
