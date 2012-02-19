/* Lasem
 *
 * Copyright © 2010 Emmanuel Pacaud
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
 * SECTION:lsmdomdocumentfragment
 * @short_description: Base class for DOM document fragments
 */

#include <lsmdomdocumentfragment.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* LsmDomNode implementation */

static const char *
lsm_dom_document_fragment_get_node_name (LsmDomNode *node)
{
	return "#document-fragment";
}

static const char *
lsm_dom_document_fragment_get_node_value (LsmDomNode *node)
{
	return NULL;
}

static LsmDomNodeType
lsm_dom_document_fragment_get_node_type (LsmDomNode *node)
{
	return LSM_DOM_NODE_TYPE_DOCUMENT_FRAGMENT_NODE;
}

/* LsmDomDocumentFragment implementation */

LsmDomDocumentFragment *
lsm_dom_document_fragment_new (void)
{
	return g_object_new (LSM_TYPE_DOM_DOCUMENT_FRAGMENT, NULL);
}

static void
lsm_dom_document_fragment_init (LsmDomDocumentFragment *document_fragment)
{
}

/* LsmDomDocumentFragment class */

static void
lsm_dom_document_fragment_class_init (LsmDomDocumentFragmentClass *klass)
{
	LsmDomNodeClass *node_class = LSM_DOM_NODE_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	node_class->get_node_name = lsm_dom_document_fragment_get_node_name;
	node_class->get_node_value = lsm_dom_document_fragment_get_node_value;
	node_class->get_node_type = lsm_dom_document_fragment_get_node_type;
}

G_DEFINE_ABSTRACT_TYPE (LsmDomDocumentFragment, lsm_dom_document_fragment, LSM_TYPE_DOM_NODE)
