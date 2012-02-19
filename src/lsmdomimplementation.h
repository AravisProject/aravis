/* Lasem
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

#ifndef LSM_DOM_IMPLEMENTATION_H
#define LSM_DOM_IMPLEMENTATION_H

#include <lsmdomtypes.h>
#include <lsmdomdocument.h>

G_BEGIN_DECLS

typedef LsmDomDocument * (*LsmDomDocumentCreateFunction) (void);

LsmDomDocument *	lsm_dom_implementation_create_document 			(const char *namespace_uri,
										 const char *qualified_name);
void			lsm_dom_implementation_add_document_create_function	(const char *qualified_name,
										 LsmDomDocumentCreateFunction create_function);

void			lsm_dom_implementation_cleanup 				(void);

G_END_DECLS

#endif
