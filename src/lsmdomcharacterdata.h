/* Lasem
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author:
 * 	Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef LSM_DOM_CHARACTER_DATA_H
#define LSM_DOM_CHARACTER_DATA_H

#include <lsmdomnode.h>

G_BEGIN_DECLS

#define LSM_TYPE_DOM_CHARACTER_DATA             (lsm_dom_character_data_get_type ())
#define LSM_DOM_CHARACTER_DATA(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), LSM_TYPE_DOM_CHARACTER_DATA, LsmDomCharacterData))
#define LSM_DOM_CHARACTER_DATA_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), LSM_TYPE_DOM_CHARACTER_DATA, LsmDomNodeClass))
#define LSM_IS_DOM_CHARACTER_DATA(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LSM_TYPE_DOM_CHARACTER_DATA))
#define LSM_IS_DOM_CHARACTER_DATA_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), LSM_TYPE_DOM_CHARACTER_DATA))
#define LSM_DOM_CHARACTER_DATA_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), LSM_TYPE_DOM_CHARACTER_DATA, LsmDomCharacterDataClass))

typedef struct _LsmDomCharacterDataClass LsmDomCharacterDataClass;

struct _LsmDomCharacterData
{
	LsmDomNode node;

	char *data;
};

struct _LsmDomCharacterDataClass {
	LsmDomNodeClass parent_class;
};

GType lsm_dom_character_data_get_type (void);

const char * 	lsm_dom_character_data_get_data 	(LsmDomCharacterData* self);
void 		lsm_dom_character_data_set_data 	(LsmDomCharacterData* self, const char* value);

G_END_DECLS

#endif

