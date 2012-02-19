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

/**
 * SECTION:lsmdomelement
 * @short_description: Base class for DOM element nodes
 */

#include <lsmdomelement.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* LsmDomNode implementation */

static const char *
lsm_dom_element_get_node_value (LsmDomNode *node)
{
	return NULL;
}

static LsmDomNodeType
lsm_dom_element_get_node_type (LsmDomNode *node)
{
	return LSM_DOM_NODE_TYPE_ELEMENT_NODE;
}

static void
lsm_dom_element_write_to_stream (LsmDomNode *self, GOutputStream *stream, GError **error)
{
	LsmDomElementClass *element_class;
	char *string;
	char *attributes = NULL;

	element_class = LSM_DOM_ELEMENT_GET_CLASS (self);
	if (element_class->get_serialized_attributes != NULL)
		attributes = element_class->get_serialized_attributes (LSM_DOM_ELEMENT (self));

	if (attributes != NULL)
		string = g_strdup_printf ("<%s %s>", lsm_dom_node_get_node_name (self), attributes);
	else
		string = g_strdup_printf ("<%s>", lsm_dom_node_get_node_name (self));

	g_output_stream_write (stream, string, strlen (string), NULL, error);
	g_free (string);
	g_free (attributes);

	LSM_DOM_NODE_CLASS (parent_class)->write_to_stream (self, stream, error);

	string = g_strdup_printf ("</\%s>\n", lsm_dom_node_get_node_name (self));
	g_output_stream_write (stream, string, strlen (string), NULL, error);
	g_free (string);
}

/* LsmDomElement implementation */

const char *
lsm_dom_element_get_attribute (LsmDomElement* self, const char* name)
{
	g_return_val_if_fail (LSM_IS_DOM_ELEMENT (self), NULL);
	g_return_val_if_fail (name != NULL, NULL);

	return LSM_DOM_ELEMENT_GET_CLASS (self)->get_attribute (self, name);
}

void
lsm_dom_element_set_attribute (LsmDomElement* self, const char* name, const char* attribute_value)
{
	g_return_if_fail (LSM_IS_DOM_ELEMENT (self));
	g_return_if_fail (name != NULL);

	LSM_DOM_ELEMENT_GET_CLASS (self)->set_attribute (self, name, attribute_value);

	lsm_dom_node_changed (LSM_DOM_NODE (self));
}

const char *
lsm_dom_element_get_tag_name (LsmDomElement *self)
{
	g_return_val_if_fail (LSM_IS_DOM_ELEMENT (self), NULL);

	return lsm_dom_node_get_node_name (LSM_DOM_NODE (self));
}

static void
lsm_dom_element_init (LsmDomElement *element)
{
}

/* LsmDomElement class */

static void
lsm_dom_element_class_init (LsmDomElementClass *klass)
{
	LsmDomNodeClass *node_class = LSM_DOM_NODE_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	node_class->get_node_value = lsm_dom_element_get_node_value;
	node_class->get_node_type = lsm_dom_element_get_node_type;
	node_class->write_to_stream = lsm_dom_element_write_to_stream;
}

G_DEFINE_ABSTRACT_TYPE (LsmDomElement, lsm_dom_element, LSM_TYPE_DOM_NODE)
