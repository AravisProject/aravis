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
 * SECTION:arvdomdocument
 * @short_description: Base class for DOM document nodes
 */

#include <arvdomdocument.h>
#include <arvdomelement.h>
#include <arvstr.h>
#include <arvdebug.h>
#include <arvdomtext.h>
#include <gio/gio.h>
#include <string.h>

typedef struct {
	char *		url;

} ArvDomDocumentPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ArvDomDocument, arv_dom_document, ARV_TYPE_DOM_NODE, G_ADD_PRIVATE (ArvDomDocument))

/* ArvDomNode implementation */

static const char *
arv_dom_document_get_node_name (ArvDomNode *node)
{
	return "#document";
}

static ArvDomNodeType
arv_dom_document_get_node_type (ArvDomNode *node)
{
	return ARV_DOM_NODE_TYPE_DOCUMENT_NODE;
}

/* ArvDomDocument implementation */

/**
 * arv_dom_document_get_document_element:
 * @self: a #ArvDomDocument
 *
 * Returns: (transfer none): the top element of @self.
 */

ArvDomElement *
arv_dom_document_get_document_element (ArvDomDocument *self)
{
	g_return_val_if_fail (ARV_IS_DOM_DOCUMENT (self), NULL);

	return ARV_DOM_ELEMENT (arv_dom_node_get_first_child (ARV_DOM_NODE (self)));
}

/**
 * arv_dom_document_create_element:
 * @self: a #ArvDomDocument
 * @tag_name: node name of the element to create
 *
 * Create a new element.
 *
 * Returns: (transfer full): a new orphan #ArvDomElement, NULL on error.
 */

ArvDomElement *
arv_dom_document_create_element (ArvDomDocument *self, const char *tag_name)
{
	ArvDomDocumentClass *document_class;

	g_return_val_if_fail (ARV_IS_DOM_DOCUMENT (self), NULL);

	document_class = ARV_DOM_DOCUMENT_GET_CLASS (self);
	if (document_class->create_element != NULL)
		return document_class->create_element (self, tag_name);

	return NULL;
}

static ArvDomText *
arv_dom_document_create_text_node_base (ArvDomDocument *document, const char *data)
{
	return ARV_DOM_TEXT (arv_dom_text_new (data));
}

/**
 * arv_dom_document_create_text_node:
 * @self: a #ArvDomDocument
 * @data: initial content
 *
 * Create a new text element.
 *
 * Returns: (transfer full): a new orphan #ArvDomText, NULL on error.
 */

ArvDomText *
arv_dom_document_create_text_node (ArvDomDocument *self, const char *data)
{
	g_return_val_if_fail (ARV_IS_DOM_DOCUMENT (self), NULL);

	return ARV_DOM_DOCUMENT_GET_CLASS (self)->create_text_node (self, data);
}

const char *
arv_dom_document_get_url (ArvDomDocument *self)
{
	ArvDomDocumentPrivate *priv = arv_dom_document_get_instance_private (ARV_DOM_DOCUMENT (self));

	g_return_val_if_fail (ARV_IS_DOM_DOCUMENT (self), NULL);

	return priv->url;
}

void
arv_dom_document_set_path (ArvDomDocument *self, const char *path)
{
	ArvDomDocumentPrivate *priv = arv_dom_document_get_instance_private (ARV_DOM_DOCUMENT (self));

	g_return_if_fail (ARV_IS_DOM_DOCUMENT (self));

	g_free (priv->url);

	if (path == NULL) {
		priv->url = NULL;
		return;
	}

	priv->url = arv_str_to_uri (path);
}

void
arv_dom_document_set_url (ArvDomDocument *self, const char *url)
{
	ArvDomDocumentPrivate *priv = arv_dom_document_get_instance_private (ARV_DOM_DOCUMENT (self));

	g_return_if_fail (ARV_IS_DOM_DOCUMENT (self));
	g_return_if_fail (url == NULL || arv_str_is_uri (url));

	g_free (priv->url);
	priv->url = g_strdup (url);
}

/**
 * arv_dom_document_get_href_data:
 * @self: a #ArvDomDocument
 * @href: document reference
 * @size: data size placeholder
 *
 * Load the content referenced by @href.
 *
 * Returns: (transfer full): a newly allocated data buffer.
 */

void *
arv_dom_document_get_href_data (ArvDomDocument *self, const char *href, gsize *size)
{
	ArvDomDocumentPrivate *priv = arv_dom_document_get_instance_private (ARV_DOM_DOCUMENT (self));
	GFile *file;
	char *data = NULL;

	g_return_val_if_fail (ARV_IS_DOM_DOCUMENT (self), NULL);
	g_return_val_if_fail (href != NULL, NULL);

	if (strncmp (href, "data:", 5) == 0) {
		while (*href != '\0' && *href != ',')
			href++;
		return g_base64_decode (href, size);
	}

	file = g_file_new_for_uri (href);

	if (!g_file_load_contents (file, NULL, &data, size, NULL, NULL) && priv->url != NULL) {
		GFile *document_file;
		GFile *parent_file;

		g_object_unref (file);

		document_file = g_file_new_for_uri (priv->url);
		parent_file = g_file_get_parent (document_file);
		file = g_file_resolve_relative_path (parent_file, href);
		g_object_unref (document_file);
		g_object_unref (parent_file);

		g_file_load_contents (file, NULL, &data, size, NULL, NULL);
	}

	g_object_unref (file);

	return data;
}

static void
arv_dom_document_init (ArvDomDocument *document)
{
}

static void
arv_dom_document_finalize (GObject *self)
{
	ArvDomDocumentPrivate *priv = arv_dom_document_get_instance_private (ARV_DOM_DOCUMENT (self));

	g_free (priv->url);

	G_OBJECT_CLASS (arv_dom_document_parent_class)->finalize (self);
}

/* ArvDomDocument class */

static void
arv_dom_document_class_init (ArvDomDocumentClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	ArvDomNodeClass *node_class = ARV_DOM_NODE_CLASS (klass);

	object_class->finalize = arv_dom_document_finalize;

	node_class->get_node_name = arv_dom_document_get_node_name;
	node_class->get_node_type = arv_dom_document_get_node_type;

	klass->create_text_node = arv_dom_document_create_text_node_base;
}
