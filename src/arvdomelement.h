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

#ifndef ARV_DOM_ELEMENT_H
#define ARV_DOM_ELEMENT_H

#include <arvdomnode.h>

G_BEGIN_DECLS

#define ARV_TYPE_DOM_ELEMENT             (arv_dom_element_get_type ())
#define ARV_DOM_ELEMENT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_DOM_ELEMENT, ArvDomElement))
#define ARV_DOM_ELEMENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_DOM_ELEMENT, ArvDomElementClass))
#define ARV_IS_DOM_ELEMENT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_DOM_ELEMENT))
#define ARV_IS_DOM_ELEMENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_DOM_ELEMENT))
#define ARV_DOM_ELEMENT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_DOM_ELEMENT, ArvDomElementClass))

typedef struct _ArvDomElementClass ArvDomElementClass;

struct _ArvDomElement {
	ArvDomNode node;
};

struct _ArvDomElementClass {
	ArvDomNodeClass parent_class;

	const char* 	(*get_attribute) (ArvDomElement *self, const char *name);
	void 		(*set_attribute) (ArvDomElement *self, const char *name, const char *attribute_value);
	char *		(*get_serialized_attributes)	(ArvDomElement *self);
};

GType arv_dom_element_get_type (void);

const char * 	arv_dom_element_get_tag_name 	(ArvDomElement *self);
const char* 	arv_dom_element_get_attribute 	(ArvDomElement* self, const char* name);
void 		arv_dom_element_set_attribute 	(ArvDomElement* self, const char* name, const char* attribute_value);

G_END_DECLS

#endif

