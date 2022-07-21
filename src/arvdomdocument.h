/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
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
 * 	Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_DOM_DOCUMENT_H
#define ARV_DOM_DOCUMENT_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvdomnode.h>

G_BEGIN_DECLS

#define ARV_TYPE_DOM_DOCUMENT             (arv_dom_document_get_type ())
ARV_API G_DECLARE_DERIVABLE_TYPE (ArvDomDocument, arv_dom_document, ARV, DOM_DOCUMENT, ArvDomNode)

struct _ArvDomDocumentClass {
	ArvDomNodeClass parent_class;

	ArvDomElement *	(*get_document_element) (ArvDomDocument* self);
	ArvDomElement *	(*create_element) 	(ArvDomDocument* self, const char *tag_name);
	ArvDomText * 	(*create_text_node) 	(ArvDomDocument* self, const char *data);
};

ARV_API ArvDomElement*		arv_dom_document_get_document_element	(ArvDomDocument *self);
ARV_API ArvDomElement*		arv_dom_document_create_element		(ArvDomDocument *self, const char *tag_name);
ARV_API ArvDomText*		arv_dom_document_create_text_node	(ArvDomDocument *self, const char *data);

ARV_API const char *		arv_dom_document_get_url		(ArvDomDocument *self);
ARV_API void			arv_dom_document_set_url		(ArvDomDocument *self, const char *url);
ARV_API void			arv_dom_document_set_path		(ArvDomDocument *self, const char *path);

ARV_API void *			arv_dom_document_get_href_data		(ArvDomDocument *self, const char *href, gsize *size);

G_END_DECLS

#endif
