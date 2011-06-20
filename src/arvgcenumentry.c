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

/**
 * SECTION: arvgcenumentry
 * @short_description: Class for EnumEntry nodes
 */

#include <arvgcenumentry.h>
#include <arvgc.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvGcNode implementation */

static const char *
arv_gc_enum_entry_get_node_name (ArvGcNode *node)
{
	return "EnumEntry";
}

static void
arv_gc_enum_entry_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	ArvGcEnumEntry *gc_enum_entry = ARV_GC_ENUM_ENTRY (node);

	if (strcmp (name, "Value") == 0) {
		gc_enum_entry->value = g_ascii_strtoll (content, NULL, 0);
	} else
		ARV_GC_NODE_CLASS (parent_class)->add_element (node, name, content, attributes);
}

/* ArvGcEnumEntry implementation */

gint64
arv_gc_enum_entry_get_value (ArvGcEnumEntry *entry)
{
	g_return_val_if_fail (ARV_IS_GC_ENUM_ENTRY (entry), 0);

	return entry->value;
}

ArvGcNode *
arv_gc_enum_entry_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_ENUM_ENTRY, NULL);

	return node;
}

static void
arv_gc_enum_entry_init (ArvGcEnumEntry *gc_enum_entry)
{
	gc_enum_entry->value = 0;
}

static void
arv_gc_enum_entry_finalize (GObject *object)
{
/*        ArvGcEnumEntry *gc_enum_entry = ARV_GC_ENUM_ENTRY (object);*/

	parent_class->finalize (object);
}

static void
arv_gc_enum_entry_class_init (ArvGcEnumEntryClass *enum_entry_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (enum_entry_class);
	ArvGcNodeClass *node_class = ARV_GC_NODE_CLASS (enum_entry_class);

	parent_class = g_type_class_peek_parent (enum_entry_class);

	object_class->finalize = arv_gc_enum_entry_finalize;

	node_class->get_node_name = arv_gc_enum_entry_get_node_name;
	node_class->add_element = arv_gc_enum_entry_add_element;
}

G_DEFINE_TYPE (ArvGcEnumEntry, arv_gc_enum_entry, ARV_TYPE_GC_NODE)
