/* Aravis
 *
 * Copyright © 2007-2009 Emmanuel Pacaud
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

#include <lsmdomimplementation.h>
#include <lsmmathmldocument.h>
#include <lsmsvgdocument.h>
#include <lsmdebug.h>
#include <string.h>

static GHashTable *document_types = NULL;

void
arv_dom_implementation_add_create_function (const char *qualified_name,
					    ArvDomDocumentCreateFunction create_function)
{
	if (document_types == NULL)
		document_types = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	g_hash_table_insert (document_types, g_strdup (qualified_name), create_function);
}

ArvDomDocument *
arv_dom_implementation_create_document (const char *namespace_uri,
					const char *qualified_name)
{
	ArvDomDocumentCreateFunction create_function;

	g_return_val_if_fail (qualified_name != NULL, NULL);

	if (document_types == NULL) {
		arv_dom_implementation_add_create_function ("math", lsm_mathml_document_new);
		arv_dom_implementation_add_create_function ("svg", lsm_svg_document_new);
	}

	create_function = g_hash_table_lookup (document_types, qualified_name);
	if (create_function == NULL) {
		lsm_debug_dom ("[ArvDomImplementation::create_document] Unknow document type (%s)",
			   qualified_name);
		return NULL;
	}

	return create_function ();
}

void
arv_dom_implementation_cleanup (void)
{
	if (document_types == NULL)
		return;

	g_hash_table_unref (document_types);
	document_types = NULL;
}
