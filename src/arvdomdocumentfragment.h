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

#ifndef ARV_DOM_DOCUMENT_FRAGMENT_H
#define ARV_DOM_DOCUMENT_FRAGMENT_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvdomnode.h>

G_BEGIN_DECLS

#define ARV_TYPE_DOM_DOCUMENT_FRAGMENT             (arv_dom_document_fragment_get_type ())
ARV_API G_DECLARE_DERIVABLE_TYPE (ArvDomDocumentFragment, arv_dom_document_fragment, ARV, DOM_DOCUMENT_FRAGMENT, ArvDomNode)

struct _ArvDomDocumentFragmentClass {
	ArvDomNodeClass parent_class;

        /* Padding for future expansion */
        gpointer padding[10];
};

ARV_API ArvDomDocumentFragment *	arv_dom_document_fragment_new		(void);

G_END_DECLS

#endif
