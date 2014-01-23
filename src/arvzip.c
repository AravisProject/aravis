/* Aravis - Digital camera library
 *
 * Copyright © 2009-2013 Emmanuel Pacaud
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
 *
 * http://www.pkware.com/documents/casestudies/APPNOTE.TXT
 *
 * Heavily inspired by the Imposter code from Gürer Özen <madcat@e-kolay.net>. 
 *
 */

/**
 * SECTION: arvzip
 * @short_description: A simple zip extractor
 */

#include <arvzip.h>
#include <arvdebug.h>
#include <string.h>
#include <zlib.h>

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
		if (ARV_GUINT32_FROM_LE_PTR (ptr, 0) != 0x02014b50) {
			arv_debug_misc ("[Zip::build_file_list] Magic number of central directory not found (0x02014b50)");
			arv_debug_misc ("[Zip::build_file_list] Expected at 0x%08x - found 0x%08x instead",
					zip->header_size + offset, ARV_GUINT32_FROM_LE_PTR (ptr, 0));
		       	return;
		}

		zip_file = g_new0 (ArvZipFile, 1);
                zip_file->compressed_size = ARV_GUINT32_FROM_LE_PTR (ptr, 20);
                zip_file->uncompressed_size = ARV_GUINT32_FROM_LE_PTR (ptr, 24);
                zip_file->offset = ARV_GUINT32_FROM_LE_PTR (ptr, 42);
		zip_file->name = g_strndup (((char *) ptr) + 46, ARV_GUINT16_FROM_LE_PTR (ptr, 28));

		arv_log_misc ("[Zip::list_files] %s", zip_file->name);

		zip->files = g_slist_prepend (zip->files, zip_file);

                offset += 0x2e +
			ARV_GUINT16_FROM_LE_PTR (ptr, 28) + /* filename size */
			ARV_GUINT16_FROM_LE_PTR (ptr, 30) + /* extra field */
			ARV_GUINT16_FROM_LE_PTR (ptr, 32);  /* file comment */
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

        if (ARV_GUINT32_FROM_LE_PTR (ptr, 0) != 0x04034b50) {
		arv_debug_misc ("[Zip::get_file_data] Magic number for file header not found (0x04034b50)");
	       	return -1;
	}

	return zip_file->offset + zip->header_size +
		ARV_GUINT16_FROM_LE_PTR (ptr, 26) +
		ARV_GUINT16_FROM_LE_PTR (ptr, 28) + 30;
}

/**
 * arv_zip_new: (skip)
 * @buffer: zipped data
 * @size: size of the zipped data
 * Return value: a new #ArvZip instance
 */

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
	if (!directory_found) {
		arv_debug_misc ("[Zip::new] Magic number for end of central directory not found (0x06054b50)");
		return zip;
	}

	ptr = zip->buffer + zip->directory_position;
        zip->n_files = ARV_GUINT16_FROM_LE_PTR (ptr, 10);
        if (ARV_GUINT16_FROM_LE_PTR (ptr, 8) != zip->n_files) {
		arv_debug_misc ("[Zip::new] Mismatch in number of files");
		zip->n_files = 0;
		return zip;
        }

        zip->directory_size = ARV_GUINT32_FROM_LE_PTR (ptr, 12);
        zip->directory_offset = ARV_GUINT32_FROM_LE_PTR (ptr, 16);
        zip->header_size = zip->directory_position - (zip->directory_offset + zip->directory_size);

	arv_log_misc ("[Zip::new] number of files = %d", zip->n_files);
	arv_log_misc ("[Zip::new] directory position = 0x%08x", zip->directory_position);
	arv_log_misc ("[Zip::new] directory size = %d", zip->directory_size);
	arv_log_misc ("[Zip::new] directory offset = 0x%08x", zip->directory_offset);
	arv_log_misc ("[Zip::new] header size = %d", zip->header_size);

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
