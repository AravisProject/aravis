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

/**
 * SECTION:lsmdomcharacterdata
 * @short_description: Base class for DOM character data nodes
 */

#include <lsmdomcharacterdata.h>
#include <lsmdebug.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* LsmDomNode implementation */

static void
lsm_dom_character_data_write_to_stream (LsmDomNode *self, GOutputStream *stream, GError **error)
{
	LsmDomCharacterData *character_data = LSM_DOM_CHARACTER_DATA (self);

	if (character_data->data != NULL)
		g_output_stream_write (stream, character_data->data, strlen (character_data->data), NULL, error);
}

static const char *
lsm_dom_character_data_get_node_value (LsmDomNode* self)
{
	return lsm_dom_character_data_get_data (LSM_DOM_CHARACTER_DATA (self));
}

static void
lsm_dom_character_data_set_node_value (LsmDomNode* self, const char *value)
{
	lsm_dom_character_data_set_data (LSM_DOM_CHARACTER_DATA (self), value);
}

/* LsmDomCharacterData implementation */

const char *
lsm_dom_character_data_get_data (LsmDomCharacterData* self)
{
	g_return_val_if_fail (LSM_IS_DOM_CHARACTER_DATA (self), NULL);

	return self->data;
}

void
lsm_dom_character_data_set_data (LsmDomCharacterData* self, const char * value)
{
	g_return_if_fail (LSM_IS_DOM_CHARACTER_DATA (self));
	g_return_if_fail (value != NULL);

	g_free (self->data);
	self->data = g_strdup (value);

	lsm_debug_dom ("[LsmDomCharacterData::set_data] Value = '%s'", value);

	lsm_dom_node_changed (LSM_DOM_NODE (self));
}

static void
lsm_dom_character_data_init (LsmDomCharacterData *character_data)
{
}

static void
lsm_dom_character_data_finalize (GObject *object)
{
	LsmDomCharacterData *self = LSM_DOM_CHARACTER_DATA (object);

	g_free (self->data);

	parent_class->finalize (object);
}

/* LsmDomCharacterData class */

static void
lsm_dom_character_data_class_init (LsmDomCharacterDataClass *character_data_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (character_data_class);
	LsmDomNodeClass *node_class = LSM_DOM_NODE_CLASS (character_data_class);

	parent_class = g_type_class_peek_parent (character_data_class);

	object_class->finalize = lsm_dom_character_data_finalize;

	node_class->write_to_stream = lsm_dom_character_data_write_to_stream;
	node_class->set_node_value = lsm_dom_character_data_set_node_value;
	node_class->get_node_value = lsm_dom_character_data_get_node_value;
}

G_DEFINE_ABSTRACT_TYPE (LsmDomCharacterData, lsm_dom_character_data, LSM_TYPE_DOM_NODE)
