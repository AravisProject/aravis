/* Aravis
 *
 * Copyright © 2007-2009 Emmanuel Pacaud
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
 * Author:
 * 	Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_DOM_PARSER_H
#define ARV_DOM_PARSER_H

#include <arvdomdocument.h>
#include <gio/gio.h>

G_BEGIN_DECLS

void 			arv_dom_document_append_from_memory 	(ArvDomDocument *document, ArvDomNode *node,
								 const void *buffer, int size, GError **error);

ArvDomDocument * 	arv_dom_document_new_from_memory 	(const void *buffer, int size, GError **error);
ArvDomDocument * 	arv_dom_document_new_from_path 		(const char *path, GError **error);
ArvDomDocument * 	arv_dom_document_new_from_url 		(const char *url, GError **error);

void			arv_dom_document_save_to_stream		(ArvDomDocument *document,
								 GOutputStream *stream,
								 GError **error);
void			arv_dom_document_save_to_memory		(ArvDomDocument *documennt,
								 void **buffer,
								 int *size,
								 GError **error);
void			arv_dom_document_save_to_path		(ArvDomDocument *documennt,
								 const char *path,
								 GError **error);
void			arv_dom_document_save_to_url		(ArvDomDocument *documennt,
								 const char *path,
								 GError **error);

G_END_DECLS

#endif
