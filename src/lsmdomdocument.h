/* Lasem
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author:
 * 	Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef LSM_DOM_DOCUMENT_H
#define LSM_DOM_DOCUMENT_H

#include <lsmtypes.h>
#include <lsmdomtypes.h>
#include <lsmdomnode.h>
#include <lsmdomview.h>

G_BEGIN_DECLS

#define LSM_TYPE_DOM_DOCUMENT             (lsm_dom_document_get_type ())
#define LSM_DOM_DOCUMENT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), LSM_TYPE_DOM_DOCUMENT, LsmDomDocument))
#define LSM_DOM_DOCUMENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), LSM_TYPE_DOM_DOCUMENT, LsmDomDocumentClass))
#define LSM_IS_DOM_DOCUMENT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LSM_TYPE_DOM_DOCUMENT))
#define LSM_IS_DOM_DOCUMENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), LSM_TYPE_DOM_DOCUMENT))
#define LSM_DOM_DOCUMENT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), LSM_TYPE_DOM_DOCUMENT, LsmDomDocumentClass))

typedef struct _LsmDomDocumentClass LsmDomDocumentClass;

struct _LsmDomDocument {
	LsmDomNode node;

	GHashTable *	ids;
	GHashTable *	elements;

	char *		url;

};

struct _LsmDomDocumentClass {
	LsmDomNodeClass parent_class;

	LsmDomElement *	(*get_document_element) (LsmDomDocument* self);
	LsmDomElement *	(*create_element) 	(LsmDomDocument* self, const char *tag_name);
	LsmDomText * 	(*create_text_node) 	(LsmDomDocument* self, const char *data);

	LsmDomView*	(*create_view) 		(LsmDomDocument *self);
};

GType lsm_dom_document_get_type (void);

LsmDomElement* 	lsm_dom_document_get_document_element 	(LsmDomDocument* self);
LsmDomElement* 	lsm_dom_document_create_element 	(LsmDomDocument* self, const char *tag_name);
LsmDomText* 	lsm_dom_document_create_text_node 	(LsmDomDocument* self, const char *data);
LsmDomElement *	lsm_dom_document_get_element_by_id 	(LsmDomDocument *self, const char *id);

void 		lsm_dom_document_register_element 	(LsmDomDocument *self, LsmDomElement *element, const char *id);

LsmDomView*	lsm_dom_document_create_view		(LsmDomDocument *self);

const char * 	lsm_dom_document_get_url 		(LsmDomDocument *self);
void		lsm_dom_document_set_url		(LsmDomDocument *self, const char *url);
void 		lsm_dom_document_set_path 		(LsmDomDocument *self, const char *path);

void * 		lsm_dom_document_get_href_data 		(LsmDomDocument *self, const char *href, gsize *size);

G_END_DECLS

#endif
