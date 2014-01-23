/* Aravis - Digital camera library
 *
 * Copyright © 2009-2012 Emmanuel Pacaud
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

/* ArvDomNode implementation */

static const char *
arv_gc_enum_entry_get_node_name (ArvDomNode *node)
{
	return "EnumEntry";
}

static void
arv_gc_enum_entry_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcEnumEntry *node = ARV_GC_ENUM_ENTRY (self);

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_VALUE:
			case ARV_GC_PROPERTY_NODE_TYPE_P_VALUE:
				node->value = property_node;
				break;
			default:
				ARV_DOM_NODE_CLASS (parent_class)->post_new_child (self, child);
				break;
		}
	}
}

static void
arv_gc_integer_node_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcFeatureNode implementation */

/* ArvGcEnumEntry implementation */

gint64
arv_gc_enum_entry_get_value (ArvGcEnumEntry *entry, GError **error)
{
	gint64 value;
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_ENUM_ENTRY (entry), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	if (entry->value == NULL)
		return 0;

	value = arv_gc_property_node_get_int64 (entry->value, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	return value;
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
	parent_class->finalize (object);
}

static void
arv_gc_enum_entry_class_init (ArvGcEnumEntryClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_enum_entry_finalize;
	dom_node_class->get_node_name = arv_gc_enum_entry_get_node_name;
	dom_node_class->post_new_child = arv_gc_enum_entry_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_integer_node_pre_remove_child;
}

G_DEFINE_TYPE (ArvGcEnumEntry, arv_gc_enum_entry, ARV_TYPE_GC_FEATURE_NODE)
