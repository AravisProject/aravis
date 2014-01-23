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

#ifndef ARV_DOM_IMPLEMENTATION_H
#define ARV_DOM_IMPLEMENTATION_H

#include <arvtypes.h>
#include <arvdomdocument.h>

G_BEGIN_DECLS

typedef ArvDomDocument * (*ArvDomDocumentCreateFunction) (void);

ArvDomDocument *	arv_dom_implementation_create_document 			(const char *namespace_uri,
										 const char *qualified_name);
void 			arv_dom_implementation_add_document_type 		(const char *qualified_name,
					  					GType document_type);

void			arv_dom_implementation_cleanup 				(void);

G_END_DECLS

#endif
