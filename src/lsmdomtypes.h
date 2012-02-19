/* Aravis
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

#ifndef ARV_DOM_TYPES_H
#define ARV_DOM_TYPES_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _ArvDomNode ArvDomNode;
typedef struct _ArvDomNodeList ArvDomNodeList;
typedef struct _ArvDomNamedNodeMap ArvDomNamedNodeMap;
typedef struct _ArvDomElement ArvDomElement;
typedef struct _ArvDomDocument ArvDomDocument;
typedef struct _ArvDomDocumentFragment ArvDomDocumentFragment;
typedef struct _ArvDomCharacterData ArvDomCharacterData;
typedef struct _ArvDomText ArvDomText;

typedef struct _ArvDomView ArvDomView;

G_END_DECLS

#endif
