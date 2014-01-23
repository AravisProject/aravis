/* Aravis
 *
 * Copyright © 2007-2012 Emmanuel Pacaud
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

#include <arvdomimplementation.h>
#include <arvdebug.h>
#include <arvgc.h>
#include <string.h>

static GHashTable *document_types = NULL;

void
arv_dom_implementation_add_document_type (const char *qualified_name,
					  GType document_type)
{
	GType *document_type_ptr;

	if (document_types == NULL)
		document_types = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	document_type_ptr = g_new (GType, 1);
	*document_type_ptr = document_type;

	g_hash_table_insert (document_types, g_strdup (qualified_name), document_type_ptr);
}

/**
 * arv_dom_implementation_create_document:
 * @namespace_uri: namespace URI
 * @qualified_name: qualified name
 *
 * Create a new DOM document. Currently, only @qualified_name is used.
 *
 * Returns: (transfer full): a new #ArvDomDocument, NULL on error:
 */

ArvDomDocument *
arv_dom_implementation_create_document (const char *namespace_uri,
					const char *qualified_name)
{
	GType *document_type;

	g_return_val_if_fail (qualified_name != NULL, NULL);

	if (document_types == NULL) {
		arv_dom_implementation_add_document_type ("RegisterDescription", ARV_TYPE_GC);
	}

	document_type = g_hash_table_lookup (document_types, qualified_name);
	if (document_type == NULL) {
		arv_debug_dom ("[ArvDomImplementation::create_document] Unknow document type (%s)",
			       qualified_name);
		return NULL;
	}

	return g_object_new (*document_type, NULL);
}

void
arv_dom_implementation_cleanup (void)
{
	if (document_types == NULL)
		return;

	g_hash_table_unref (document_types);
	document_types = NULL;
}
