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

/**
 * SECTION:arvdomtext
 * @short_description: Base class for DOM text nodes
 */

#include <arvdomtext.h>

/* ArvDomNode implementation */

static const char *
arv_dom_text_get_node_name (ArvDomNode *node)
{
	return "#text";
}

static const char *
arv_dom_text_get_node_value (ArvDomNode *node)
{
	return ARV_DOM_CHARACTER_DATA (node)->data;
}

static ArvDomNodeType
arv_dom_text_get_node_type (ArvDomNode *node)
{
	return ARV_DOM_NODE_TYPE_TEXT_NODE;
}

/* ArvDomText implementation */

ArvDomNode *
arv_dom_text_new (const char *data)
{
	ArvDomNode *node;

	node = g_object_new (ARV_TYPE_DOM_TEXT, NULL);

	arv_dom_character_data_set_data (ARV_DOM_CHARACTER_DATA (node), data);

	return node;
}

static void
arv_dom_text_init (ArvDomText *text_node)
{
}

/* ArvDomText class */

static void
arv_dom_text_class_init (ArvDomTextClass *klass)
{
	ArvDomNodeClass *node_class = ARV_DOM_NODE_CLASS (klass);

	node_class->get_node_name = arv_dom_text_get_node_name;
	node_class->get_node_value = arv_dom_text_get_node_value;
	node_class->get_node_type = arv_dom_text_get_node_type;
}

G_DEFINE_TYPE (ArvDomText, arv_dom_text, ARV_TYPE_DOM_CHARACTER_DATA)
