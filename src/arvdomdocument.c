/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author:
 * 	Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

/**
 * SECTION:arvdomdocument
 * @short_description: Base class for DOM document nodes
 */

#include <arvdomdocument.h>
#include <arvdomnodeprivate.h>
#include <arvdomelement.h>
#include <arvstr.h>
#include <arvdebug.h>
#include <arvdomtext.h>
#include <gio/gio.h>
#include <string.h>

enum {
	PROP_0,
	PROP_WRITE_LOCK
};

typedef struct {
	char *		url;

	gint		is_write_locked;
	GRecMutex	lazy_mutex;
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
	g_return_if_fail (!arv_dom_document_get_write_lock (self));

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
	g_return_if_fail (!arv_dom_document_get_write_lock (self));
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
_write_lock_released (ArvDomNode *node)
{
	ArvDomNodeClass *node_class;
	ArvDomNode *iter;

	/* Children first, as the lazy data of a node may depend on the one of its children */
	for (iter = arv_dom_node_get_first_child (node); iter != NULL; iter = arv_dom_node_get_next_sibling (iter))
		_write_lock_released (iter);

	node_class = ARV_DOM_NODE_GET_CLASS (node);
	if (node_class->write_lock_released != NULL)
		node_class->write_lock_released (node);
}

/**
 * arv_dom_document_set_write_lock:
 * @self: a #ArvDomDocument
 * @write_lock: whether to write lock the document
 *
 * Sets the write lock of the document. When the document is write locked, any modification of the tree is refused,
 * and the read accessors of the tree become thread safe: the strings and lists they return are built once, at the
 * first call, and stay valid until the write lock is released.
 *
 * Without the write lock, the document behaves as a regular, not thread safe, DOM tree. The Genicam data of the
 * devices are write locked at the end of the device creation.
 *
 * Changing the write lock is not thread safe, and must be done while the document is not used by another thread.
 *
 * Since: 0.10.0
 */

void
arv_dom_document_set_write_lock (ArvDomDocument *self, gboolean write_lock)
{
	ArvDomDocumentPrivate *priv = arv_dom_document_get_instance_private (ARV_DOM_DOCUMENT (self));

	g_return_if_fail (ARV_IS_DOM_DOCUMENT (self));

	write_lock = write_lock ? TRUE : FALSE;

	if (g_atomic_int_get (&priv->is_write_locked) == write_lock)
		return;

	if (write_lock) {
		g_atomic_int_set (&priv->is_write_locked, TRUE);
	} else {
		g_atomic_int_set (&priv->is_write_locked, FALSE);

		/* The data built under the write lock can not be trusted anymore */
		_write_lock_released (ARV_DOM_NODE (self));
	}

	g_object_notify (G_OBJECT (self), "write-lock");
}

/**
 * arv_dom_document_get_write_lock:
 * @self: a #ArvDomDocument
 *
 * Returns: %TRUE if the document is write locked, see [method@ArvDomDocument.set_write_lock].
 *
 * Since: 0.10.0
 */

gboolean
arv_dom_document_get_write_lock (ArvDomDocument *self)
{
	ArvDomDocumentPrivate *priv = arv_dom_document_get_instance_private (ARV_DOM_DOCUMENT (self));

	g_return_val_if_fail (ARV_IS_DOM_DOCUMENT (self), FALSE);

	return g_atomic_int_get (&priv->is_write_locked);
}

/*
 * arv_dom_node_is_write_locked:
 * @self: a #ArvDomNode
 *
 * Returns: %TRUE if @self belongs to a write locked document.
 */

gboolean
arv_dom_node_is_write_locked (ArvDomNode *self)
{
	ArvDomDocument *document;

	document = arv_dom_node_get_owner_document (self);
	if (document == NULL)
		return FALSE;

	return arv_dom_document_get_write_lock (document);
}

/*
 * arv_dom_node_lazy_lock:
 * @self: a #ArvDomNode
 *
 * Locks the mutex protecting the lazy build of node data, while the owner document is write locked. The mutex is
 * recursive.
 */

void
arv_dom_node_lazy_lock (ArvDomNode *self)
{
	ArvDomDocument *document;
	ArvDomDocumentPrivate *priv;

	document = arv_dom_node_get_owner_document (self);
	if (document == NULL)
		return;

	priv = arv_dom_document_get_instance_private (document);
	g_rec_mutex_lock (&priv->lazy_mutex);
}

void
arv_dom_node_lazy_unlock (ArvDomNode *self)
{
	ArvDomDocument *document;
	ArvDomDocumentPrivate *priv;

	document = arv_dom_node_get_owner_document (self);
	if (document == NULL)
		return;

	priv = arv_dom_document_get_instance_private (document);
	g_rec_mutex_unlock (&priv->lazy_mutex);
}

/*
 * arv_dom_node_lazy_build:
 * @self: a #ArvDomNode
 * @is_frozen: the companion flag of the lazily built data
 * @build: the function (re)building the data
 *
 * Builds data depending on the DOM tree. When the owner document is not write locked, the data are rebuilt at each
 * call. When it is write locked, they are built once, at the first call, in a thread safe way, and kept until the
 * write lock is released. The node write_lock_released implementation must clear @is_frozen.
 */

void
arv_dom_node_lazy_build (ArvDomNode *self, gint *is_frozen, ArvDomNodeLazyBuildFunc build)
{
	if (!arv_dom_node_is_write_locked (self)) {
		build (self);
		return;
	}

	if (g_atomic_int_get (is_frozen))
		return;

	arv_dom_node_lazy_lock (self);
	if (!*is_frozen) {
		build (self);
		g_atomic_int_set (is_frozen, TRUE);
	}
	arv_dom_node_lazy_unlock (self);
}

static void
arv_dom_document_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch (prop_id) {
		case PROP_WRITE_LOCK:
			arv_dom_document_set_write_lock (ARV_DOM_DOCUMENT (object), g_value_get_boolean (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_dom_document_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch (prop_id) {
		case PROP_WRITE_LOCK:
			g_value_set_boolean (value, arv_dom_document_get_write_lock (ARV_DOM_DOCUMENT (object)));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_dom_document_init (ArvDomDocument *document)
{
	ArvDomDocumentPrivate *priv = arv_dom_document_get_instance_private (document);

	g_rec_mutex_init (&priv->lazy_mutex);
}

static void
arv_dom_document_finalize (GObject *self)
{
	ArvDomDocumentPrivate *priv = arv_dom_document_get_instance_private (ARV_DOM_DOCUMENT (self));

	g_free (priv->url);
	g_rec_mutex_clear (&priv->lazy_mutex);

	G_OBJECT_CLASS (arv_dom_document_parent_class)->finalize (self);
}

/* ArvDomDocument class */

static void
arv_dom_document_class_init (ArvDomDocumentClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	ArvDomNodeClass *node_class = ARV_DOM_NODE_CLASS (klass);

	object_class->finalize = arv_dom_document_finalize;
	object_class->set_property = arv_dom_document_set_property;
	object_class->get_property = arv_dom_document_get_property;

	node_class->get_node_name = arv_dom_document_get_node_name;
	node_class->get_node_type = arv_dom_document_get_node_type;

	klass->create_text_node = arv_dom_document_create_text_node_base;

	/**
	 * ArvDomDocument:write-lock:
	 *
	 * Whether the document is write locked, see [method@ArvDomDocument.set_write_lock].
	 *
	 * Since: 0.10.0
	 */

	g_object_class_install_property
		(object_class, PROP_WRITE_LOCK,
		 g_param_spec_boolean ("write-lock", "Write lock", "Refuse tree modifications, for thread safe reads",
				       FALSE,
				       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));
}
