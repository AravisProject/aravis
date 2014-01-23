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

/**
 * SECTION:arvdomcharacterdata
 * @short_description: Base class for DOM character data nodes
 */

#include <arvdomcharacterdata.h>
#include <arvdebug.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static void
arv_dom_character_data_write_to_stream (ArvDomNode *self, GOutputStream *stream, GError **error)
{
	ArvDomCharacterData *character_data = ARV_DOM_CHARACTER_DATA (self);

	if (character_data->data != NULL)
		g_output_stream_write (stream, character_data->data, strlen (character_data->data), NULL, error);
}

static const char *
arv_dom_character_data_get_node_value (ArvDomNode* self)
{
	return arv_dom_character_data_get_data (ARV_DOM_CHARACTER_DATA (self));
}

static void
arv_dom_character_data_set_node_value (ArvDomNode* self, const char *value)
{
	arv_dom_character_data_set_data (ARV_DOM_CHARACTER_DATA (self), value);
}

/* ArvDomCharacterData implementation */

const char *
arv_dom_character_data_get_data (ArvDomCharacterData* self)
{
	g_return_val_if_fail (ARV_IS_DOM_CHARACTER_DATA (self), NULL);

	return self->data;
}

void
arv_dom_character_data_set_data (ArvDomCharacterData* self, const char * value)
{
	g_return_if_fail (ARV_IS_DOM_CHARACTER_DATA (self));
	g_return_if_fail (value != NULL);

	g_free (self->data);
	self->data = g_strdup (value);

	arv_log_dom ("[ArvDomCharacterData::set_data] Value = '%s'", value);

	arv_dom_node_changed (ARV_DOM_NODE (self));
}

static void
arv_dom_character_data_init (ArvDomCharacterData *character_data)
{
}

static void
arv_dom_character_data_finalize (GObject *object)
{
	ArvDomCharacterData *self = ARV_DOM_CHARACTER_DATA (object);

	g_free (self->data);

	parent_class->finalize (object);
}

/* ArvDomCharacterData class */

static void
arv_dom_character_data_class_init (ArvDomCharacterDataClass *character_data_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (character_data_class);
	ArvDomNodeClass *node_class = ARV_DOM_NODE_CLASS (character_data_class);

	parent_class = g_type_class_peek_parent (character_data_class);

	object_class->finalize = arv_dom_character_data_finalize;

	node_class->write_to_stream = arv_dom_character_data_write_to_stream;
	node_class->set_node_value = arv_dom_character_data_set_node_value;
	node_class->get_node_value = arv_dom_character_data_get_node_value;
}

G_DEFINE_ABSTRACT_TYPE (ArvDomCharacterData, arv_dom_character_data, ARV_TYPE_DOM_NODE)
