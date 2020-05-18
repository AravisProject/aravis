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

/**
 * SECTION:arvdomelement
 * @short_description: Base class for DOM element nodes
 */

#include <arvdomelement.h>
#include <string.h>

G_DEFINE_ABSTRACT_TYPE (ArvDomElement, arv_dom_element, ARV_TYPE_DOM_NODE)

/* ArvDomNode implementation */

static const char *
arv_dom_element_get_node_value (ArvDomNode *node)
{
	return NULL;
}

static ArvDomNodeType
arv_dom_element_get_node_type (ArvDomNode *node)
{
	return ARV_DOM_NODE_TYPE_ELEMENT_NODE;
}

/* ArvDomElement implementation */

const char *
arv_dom_element_get_attribute (ArvDomElement* self, const char* name)
{
	g_return_val_if_fail (ARV_IS_DOM_ELEMENT (self), NULL);
	g_return_val_if_fail (name != NULL, NULL);

	return ARV_DOM_ELEMENT_GET_CLASS (self)->get_attribute (self, name);
}

void
arv_dom_element_set_attribute (ArvDomElement* self, const char* name, const char* attribute_value)
{
	g_return_if_fail (ARV_IS_DOM_ELEMENT (self));
	g_return_if_fail (name != NULL);

	ARV_DOM_ELEMENT_GET_CLASS (self)->set_attribute (self, name, attribute_value);

	arv_dom_node_changed (ARV_DOM_NODE (self));
}

const char *
arv_dom_element_get_tag_name (ArvDomElement *self)
{
	g_return_val_if_fail (ARV_IS_DOM_ELEMENT (self), NULL);

	return arv_dom_node_get_node_name (ARV_DOM_NODE (self));
}

static void
arv_dom_element_init (ArvDomElement *element)
{
}

/* ArvDomElement class */

static void
arv_dom_element_class_init (ArvDomElementClass *klass)
{
	ArvDomNodeClass *node_class = ARV_DOM_NODE_CLASS (klass);

	node_class->get_node_value = arv_dom_element_get_node_value;
	node_class->get_node_type = arv_dom_element_get_node_type;
}
