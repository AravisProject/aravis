/* Aravis
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author:
 * 	Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION:arvdomdocumentfragment
 * @short_description: Base class for DOM document fragments
 */

#include <arvdomdocumentfragment.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_dom_document_fragment_get_node_name (ArvDomNode *node)
{
	return "#document-fragment";
}

static const char *
arv_dom_document_fragment_get_node_value (ArvDomNode *node)
{
	return NULL;
}

static ArvDomNodeType
arv_dom_document_fragment_get_node_type (ArvDomNode *node)
{
	return ARV_DOM_NODE_TYPE_DOCUMENT_FRAGMENT_NODE;
}

/* ArvDomDocumentFragment implementation */

ArvDomDocumentFragment *
arv_dom_document_fragment_new (void)
{
	return g_object_new (ARV_TYPE_DOM_DOCUMENT_FRAGMENT, NULL);
}

static void
arv_dom_document_fragment_init (ArvDomDocumentFragment *document_fragment)
{
}

/* ArvDomDocumentFragment class */

static void
arv_dom_document_fragment_class_init (ArvDomDocumentFragmentClass *klass)
{
	ArvDomNodeClass *node_class = ARV_DOM_NODE_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	node_class->get_node_name = arv_dom_document_fragment_get_node_name;
	node_class->get_node_value = arv_dom_document_fragment_get_node_value;
	node_class->get_node_type = arv_dom_document_fragment_get_node_type;
}

G_DEFINE_ABSTRACT_TYPE (ArvDomDocumentFragment, arv_dom_document_fragment, ARV_TYPE_DOM_NODE)
