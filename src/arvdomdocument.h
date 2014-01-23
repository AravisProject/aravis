/* Aravis
 *
 * Copyright © 2007-2008 Emmanuel Pacaud
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

#ifndef ARV_DOM_DOCUMENT_H
#define ARV_DOM_DOCUMENT_H

#include <arvtypes.h>
#include <arvdomnode.h>

G_BEGIN_DECLS

#define ARV_TYPE_DOM_DOCUMENT             (arv_dom_document_get_type ())
#define ARV_DOM_DOCUMENT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_DOM_DOCUMENT, ArvDomDocument))
#define ARV_DOM_DOCUMENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_DOM_DOCUMENT, ArvDomDocumentClass))
#define ARV_IS_DOM_DOCUMENT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_DOM_DOCUMENT))
#define ARV_IS_DOM_DOCUMENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_DOM_DOCUMENT))
#define ARV_DOM_DOCUMENT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_DOM_DOCUMENT, ArvDomDocumentClass))

typedef struct _ArvDomDocumentClass ArvDomDocumentClass;

struct _ArvDomDocument {
	ArvDomNode node;

	char *		url;

};

struct _ArvDomDocumentClass {
	ArvDomNodeClass parent_class;

	ArvDomElement *	(*get_document_element) (ArvDomDocument* self);
	ArvDomElement *	(*create_element) 	(ArvDomDocument* self, const char *tag_name);
	ArvDomText * 	(*create_text_node) 	(ArvDomDocument* self, const char *data);
};

GType arv_dom_document_get_type (void);

ArvDomElement* 	arv_dom_document_get_document_element 	(ArvDomDocument* self);
ArvDomElement* 	arv_dom_document_create_element 	(ArvDomDocument* self, const char *tag_name);
ArvDomText* 	arv_dom_document_create_text_node 	(ArvDomDocument* self, const char *data);

const char * 	arv_dom_document_get_url 		(ArvDomDocument *self);
void		arv_dom_document_set_url		(ArvDomDocument *self, const char *url);
void 		arv_dom_document_set_path 		(ArvDomDocument *self, const char *path);

void * 		arv_dom_document_get_href_data 		(ArvDomDocument *self, const char *href, gsize *size);

G_END_DECLS

#endif
