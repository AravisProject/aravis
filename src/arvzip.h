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

G_END_DECLS

#endif
