/* Lasem
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author:
 * 	Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef LSM_DOM_DOCUMENT_FRAGMENT_H
#define LSM_DOM_DOCUMENT_FRAGMENT_H

#include <lsmdom.h>
#include <lsmdomnode.h>

G_BEGIN_DECLS

#define LSM_TYPE_DOM_DOCUMENT_FRAGMENT             (lsm_dom_document_fragment_get_type ())
#define LSM_DOM_DOCUMENT_FRAGMENT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), LSM_TYPE_DOM_DOCUMENT_FRAGMENT, LsmDomDocumentFragment))
#define LSM_DOM_DOCUMENT_FRAGMENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), LSM_TYPE_DOM_DOCUMENT_FRAGMENT, LsmDomDocumentFragmentClass))
#define LSM_IS_DOM_DOCUMENT_FRAGMENT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LSM_TYPE_DOM_DOCUMENT_FRAGMENT))
#define LSM_IS_DOM_DOCUMENT_FRAGMENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), LSM_TYPE_DOM_DOCUMENT_FRAGMENT))
#define LSM_DOM_DOCUMENT_FRAGMENT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), LSM_TYPE_DOM_DOCUMENT_FRAGMENT, LsmDomDocumentFragmentClass))

typedef struct _LsmDomDocumentFragmentClass LsmDomDocumentFragmentClass;

struct _LsmDomDocumentFragment {
	LsmDomNode node;
};

struct _LsmDomDocumentFragmentClass {
	LsmDomNodeClass parent_class;
};

GType lsm_dom_document_fragment_get_type (void);

LsmDomDocumentFragment * 	lsm_dom_document_fragment_new 		(void);

G_END_DECLS

#endif
