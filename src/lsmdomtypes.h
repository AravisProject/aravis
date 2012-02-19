/* Lasem - SVG and Mathml library
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

#ifndef LSM_DOM_TYPES_H
#define LSM_DOM_TYPES_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _LsmDomNode LsmDomNode;
typedef struct _LsmDomNodeList LsmDomNodeList;
typedef struct _LsmDomNamedNodeMap LsmDomNamedNodeMap;
typedef struct _LsmDomElement LsmDomElement;
typedef struct _LsmDomDocument LsmDomDocument;
typedef struct _LsmDomDocumentFragment LsmDomDocumentFragment;
typedef struct _LsmDomCharacterData LsmDomCharacterData;
typedef struct _LsmDomText LsmDomText;

typedef struct _LsmDomView LsmDomView;

G_END_DECLS

#endif
