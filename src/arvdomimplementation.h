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

#ifndef ARV_DOM_IMPLEMENTATION_H
#define ARV_DOM_IMPLEMENTATION_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvdomdocument.h>

G_BEGIN_DECLS

typedef ArvDomDocument * (*ArvDomDocumentCreateFunction) (void);

ARV_API ArvDomDocument *	arv_dom_implementation_create_document		(const char *namespace_uri,
										 const char *qualified_name);
ARV_API void			arv_dom_implementation_add_document_type	(const char *qualified_name,
										 GType document_type);

ARV_API void			arv_dom_implementation_cleanup			(void);

G_END_DECLS

#endif
