/* Aravis - Digital camera library
 *
 * Copyright © 2009-2019 Emmanuel Pacaud
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

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvdomnode.h>

G_BEGIN_DECLS

#define ARV_TYPE_DOM_ELEMENT (arv_dom_element_get_type ())
G_DECLARE_DERIVABLE_TYPE (ArvDomElement, arv_dom_element, ARV, DOM_ELEMENT, ArvDomNode)

struct _ArvDomElementClass {
	ArvDomNodeClass parent_class;

	const char* 	(*get_attribute) (ArvDomElement *self, const char *name);
	void 		(*set_attribute) (ArvDomElement *self, const char *name, const char *attribute_value);
};

const char * 	arv_dom_element_get_tag_name 	(ArvDomElement *self);
const char* 	arv_dom_element_get_attribute 	(ArvDomElement* self, const char* name);
void 		arv_dom_element_set_attribute 	(ArvDomElement* self, const char* name, const char* attribute_value);

G_END_DECLS

#endif

