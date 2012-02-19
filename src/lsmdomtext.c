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
 * SECTION:lsmdomtext
 * @short_description: Base class for DOM text nodes
 */

#include <lsmdomtext.h>

/* LsmDomNode implementation */

static const char *
lsm_dom_text_get_node_name (LsmDomNode *node)
{
	return "#text";
}

static const char *
lsm_dom_text_get_node_value (LsmDomNode *node)
{
	return LSM_DOM_CHARACTER_DATA (node)->data;
}

static LsmDomNodeType
lsm_dom_text_get_node_type (LsmDomNode *node)
{
	return LSM_DOM_NODE_TYPE_TEXT_NODE;
}

/* LsmDomText implementation */

LsmDomNode *
lsm_dom_text_new (const char *data)
{
	LsmDomNode *node;

	node = g_object_new (LSM_TYPE_DOM_TEXT, NULL);

	lsm_dom_character_data_set_data (LSM_DOM_CHARACTER_DATA (node), data);

	return node;
}

static void
lsm_dom_text_init (LsmDomText *text_node)
{
}

/* LsmDomText class */

static void
lsm_dom_text_class_init (LsmDomTextClass *klass)
{
	LsmDomNodeClass *node_class = LSM_DOM_NODE_CLASS (klass);

	node_class->get_node_name = lsm_dom_text_get_node_name;
	node_class->get_node_value = lsm_dom_text_get_node_value;
	node_class->get_node_type = lsm_dom_text_get_node_type;
}

G_DEFINE_TYPE (LsmDomText, lsm_dom_text, LSM_TYPE_DOM_CHARACTER_DATA)
