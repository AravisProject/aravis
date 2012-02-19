/* Lasem
 *
 * Copyright © 2007-2010 Emmanuel Pacaud
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
 * SECTION:lsmdomdocument
 * @short_description: Base class for DOM document nodes
 */

#include <lsmdomdocument.h>
#include <lsmdomelement.h>
#include <lsmstr.h>
#include <lsmdebug.h>
#include <lsmdomtext.h>
#include <gio/gio.h>
#include <string.h>

static GObjectClass *parent_class;

/* LsmDomNode implementation */

static const char *
lsm_dom_document_get_node_name (LsmDomNode *node)
{
	return "#document";
}

static LsmDomNodeType
lsm_dom_document_get_node_type (LsmDomNode *node)
{
	return LSM_DOM_NODE_TYPE_DOCUMENT_NODE;
}

/* LsmDomDocument implementation */

LsmDomElement *
lsm_dom_document_get_document_element (LsmDomDocument *self)
{
	g_return_val_if_fail (LSM_IS_DOM_DOCUMENT (self), NULL);

	return LSM_DOM_ELEMENT (lsm_dom_node_get_first_child (LSM_DOM_NODE (self)));
}

LsmDomElement *
lsm_dom_document_create_element (LsmDomDocument *document, const char *tag_name)
{
	LsmDomDocumentClass *document_class;

	g_return_val_if_fail (LSM_IS_DOM_DOCUMENT (document), NULL);

	document_class = LSM_DOM_DOCUMENT_GET_CLASS (document);
	if (document_class->create_element != NULL)
		return document_class->create_element (document, tag_name);

	return NULL;
}

LsmDomText *
lsm_dom_document_create_text_node_base (LsmDomDocument *document, const char *data)
{
	return LSM_DOM_TEXT (lsm_dom_text_new (data));
}

LsmDomText *
lsm_dom_document_create_text_node (LsmDomDocument *document, const char *data)
{
	g_return_val_if_fail (LSM_IS_DOM_DOCUMENT (document), NULL);

	return LSM_DOM_DOCUMENT_GET_CLASS (document)->create_text_node (document, data);
}

LsmDomView *
lsm_dom_document_create_view (LsmDomDocument *self)
{
	g_return_val_if_fail (LSM_IS_DOM_DOCUMENT (self), NULL);

	return LSM_DOM_DOCUMENT_GET_CLASS (self)->create_view (self);
}

LsmDomElement *
lsm_dom_document_get_element_by_id (LsmDomDocument *self, const char *id)
{
	g_return_val_if_fail (LSM_IS_DOM_DOCUMENT (self), NULL);
	g_return_val_if_fail (id != NULL, NULL);

	lsm_debug_dom ("[LsmDomDocument::get_element_by_id] Lookup '%s'", id);

	return g_hash_table_lookup (self->ids, id);
}

void
lsm_dom_document_register_element (LsmDomDocument *self, LsmDomElement *element, const char *id)
{
	char *old_id;

	g_return_if_fail (LSM_IS_DOM_DOCUMENT (self));

	old_id = g_hash_table_lookup (self->elements, element);
	if (old_id != NULL) {
		lsm_debug_dom ("[LsmDomDocument::register_element] Unregister '%s'", old_id);

		g_hash_table_remove (self->elements, element);
		g_hash_table_remove (self->ids, old_id);
	}

	if (id != NULL) {
		char *new_id = g_strdup (id);

		lsm_debug_dom ("[LsmDomDocument::register_element] Register '%s'", id);

		g_hash_table_replace (self->ids, new_id, element);
		g_hash_table_replace (self->elements, element, new_id);
	}
}

const char *
lsm_dom_document_get_url (LsmDomDocument *self)
{
	g_return_val_if_fail (LSM_IS_DOM_DOCUMENT (self), NULL);

	return self->url;
}

void
lsm_dom_document_set_path (LsmDomDocument *self, const char *path)
{
	g_return_if_fail (LSM_IS_DOM_DOCUMENT (self));

	g_free (self->url);

	if (path == NULL) {
		self->url = NULL;
		return;
	}

	self->url = lsm_str_to_uri (path);
}

void
lsm_dom_document_set_url (LsmDomDocument *self, const char *url)
{
	g_return_if_fail (LSM_IS_DOM_DOCUMENT (self));
	g_return_if_fail (url == NULL || lsm_str_is_uri (url));

	g_free (self->url);
	self->url = g_strdup (url);
}

void *
lsm_dom_document_get_href_data (LsmDomDocument *self, const char *href, gsize *size)
{
	GFile *file;
	char *data = NULL;

	g_return_val_if_fail (LSM_IS_DOM_DOCUMENT (self), NULL);
	g_return_val_if_fail (href != NULL, NULL);

	if (strncmp (href, "data:", 5) == 0) {
		while (*href != '\0' && *href != ',')
			href++;
		return g_base64_decode (href, size);
	}

	file = g_file_new_for_uri (href);

	if (!g_file_load_contents (file, NULL, &data, size, NULL, NULL) && self->url != NULL) {
		GFile *document_file;
		GFile *parent_file;

		g_object_unref (file);

		document_file = g_file_new_for_uri (self->url);
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
lsm_dom_document_init (LsmDomDocument *document)
{
	document->ids = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	document->elements = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
}

static void
lsm_dom_document_finalize (GObject *object)
{
	LsmDomDocument *document = LSM_DOM_DOCUMENT (object);

	g_hash_table_unref (document->elements);
	g_hash_table_unref (document->ids);

	g_free (document->url);

	parent_class->finalize (object);
}

/* LsmDomDocument class */

static void
lsm_dom_document_class_init (LsmDomDocumentClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	LsmDomNodeClass *node_class = LSM_DOM_NODE_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = lsm_dom_document_finalize;

	node_class->get_node_name = lsm_dom_document_get_node_name;
	node_class->get_node_type = lsm_dom_document_get_node_type;

	klass->create_text_node = lsm_dom_document_create_text_node_base;
}

G_DEFINE_ABSTRACT_TYPE (LsmDomDocument, lsm_dom_document, LSM_TYPE_DOM_NODE)

