/* Aravis
 *
 * Copyright © 2007-2010 Emmanuel Pacaud
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

#ifndef ARV_DOM_CHARACTER_DATA_H
#define ARV_DOM_CHARACTER_DATA_H

#include <arvdomnode.h>

G_BEGIN_DECLS

#define ARV_TYPE_DOM_CHARACTER_DATA             (arv_dom_character_data_get_type ())
#define ARV_DOM_CHARACTER_DATA(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_DOM_CHARACTER_DATA, ArvDomCharacterData))
#define ARV_DOM_CHARACTER_DATA_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_DOM_CHARACTER_DATA, ArvDomNodeClass))
#define ARV_IS_DOM_CHARACTER_DATA(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_DOM_CHARACTER_DATA))
#define ARV_IS_DOM_CHARACTER_DATA_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_DOM_CHARACTER_DATA))
#define ARV_DOM_CHARACTER_DATA_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_DOM_CHARACTER_DATA, ArvDomCharacterDataClass))

typedef struct _ArvDomCharacterDataClass ArvDomCharacterDataClass;

struct _ArvDomCharacterData
{
	ArvDomNode node;

	char *data;
};

struct _ArvDomCharacterDataClass {
	ArvDomNodeClass parent_class;
};

GType arv_dom_character_data_get_type (void);

const char * 	arv_dom_character_data_get_data 	(ArvDomCharacterData* self);
void 		arv_dom_character_data_set_data 	(ArvDomCharacterData* self, const char* value);

G_END_DECLS

#endif

