/* Aravis
 *
 * Copyright © 2007-2008 Emmanuel Pacaud
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

#ifndef ARV_DOM_TEXT_H
#define ARV_DOM_TEXT_H

#include <arvdomcharacterdata.h>

G_BEGIN_DECLS

#define ARV_TYPE_DOM_TEXT             (arv_dom_text_get_type ())
#define ARV_DOM_TEXT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_DOM_TEXT, ArvDomText))
#define ARV_DOM_TEXT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_DOM_TEXT, ArvDomTextClass))
#define ARV_IS_DOM_TEXT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_DOM_TEXT))
#define ARV_IS_DOM_TEXT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_DOM_TEXT))
#define ARV_DOM_TEXT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_DOM_TEXT, ArvDomTextClass))

typedef struct _ArvDomTextClass ArvDomTextClass;

struct _ArvDomText {
	ArvDomCharacterData	character_data;
};

struct _ArvDomTextClass {
	ArvDomCharacterDataClass  parent_class;
};

GType arv_dom_text_get_type (void);

ArvDomNode 	*arv_dom_text_new 		(const char *data);

G_END_DECLS

#endif

