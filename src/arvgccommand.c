/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <arvgccommand.h>
#include <arvgcinteger.h>
#include <arvgcport.h>
#include <arvtools.h>
#include <arvdebug.h>
#include <stdlib.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvGcNode implementation */

static void
arv_gc_command_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	ArvGcCommand *gc_command = ARV_GC_COMMAND (node);

	if (strcmp (name, "Value") == 0) {
		arv_force_g_value_to_int64 (&gc_command->value, g_ascii_strtoull (content, NULL, 0));
	} else if (strcmp (name, "pValue") == 0) {
		arv_force_g_value_to_string (&gc_command->value, content);
	} else if (strcmp (name, "CommandValue") == 0) {
		arv_force_g_value_to_int64 (&gc_command->command_value, g_ascii_strtoull (content, NULL, 0));
	} else if (strcmp (name, "pCommandValue") == 0) {
		arv_force_g_value_to_string (&gc_command->command_value, content);
	} else
		ARV_GC_NODE_CLASS (parent_class)->add_element (node, name, content, attributes);
}

/* ArvGcCommand implementation */

ArvGcNode *
arv_gc_command_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_COMMAND, NULL);

	return node;
}

static void
arv_gc_command_init (ArvGcCommand *gc_command)
{
	/* Set default to read only 32 bits little endian integer command */
	g_value_init (&gc_command->value, G_TYPE_INT64);
	g_value_init (&gc_command->command_value, G_TYPE_INT64);
	g_value_set_int64 (&gc_command->value, 0);
	g_value_set_int64 (&gc_command->command_value, 0);
}

static void
arv_gc_command_finalize (GObject *object)
{
	ArvGcCommand *gc_command = ARV_GC_COMMAND (object);

	g_value_unset (&gc_command->value);
	g_value_unset (&gc_command->command_value);

	parent_class->finalize (object);
}

static void
arv_gc_command_class_init (ArvGcCommandClass *command_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (command_class);
	ArvGcNodeClass *node_class = ARV_GC_NODE_CLASS (command_class);

	parent_class = g_type_class_peek_parent (command_class);

	object_class->finalize = arv_gc_command_finalize;

	node_class->add_element = arv_gc_command_add_element;
}

G_DEFINE_TYPE (ArvGcCommand, arv_gc_command, ARV_TYPE_GC_NODE)
