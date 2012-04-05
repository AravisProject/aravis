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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvgcenumeration
 * @short_description: Class for Enumeration nodes
 */

#include <arvgcenumeration.h>
#include <arvgcenumentry.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <arvdebug.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvGcDomNode implementation */

static const char *
arv_gc_enumeration_get_node_name (ArvDomNode *node)
{
	return "Enumeration";
}

static gboolean
arv_gc_enumeration_can_append_child (ArvDomNode *node, ArvDomNode *child)
{
	return (ARV_IS_GC_ENUM_ENTRY (child) || ARV_IS_GC_PROPERTY_NODE (child));
}

static void
arv_gc_enumeration_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcEnumeration *node = ARV_GC_ENUMERATION (self);

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
	} else if (ARV_IS_GC_ENUM_ENTRY (child))
		node->entries = g_slist_prepend (node->entries, child);
}

static void
arv_gc_enumeration_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcFeatureNode implementation */

static void
arv_gc_enumeration_set_value_from_string (ArvGcFeatureNode *node, const char *string)
{
	arv_gc_enumeration_set_string_value (ARV_GC_ENUMERATION (node), string);
}

static const char *
arv_gc_enumeration_get_value_as_string (ArvGcFeatureNode *node)
{
	return arv_gc_enumeration_get_string_value (ARV_GC_ENUMERATION (node));
}


/* ArvGcEnumeration implementation */

const char *
arv_gc_enumeration_get_string_value (ArvGcEnumeration *enumeration)
{
	const GSList *iter;
	gint64 value;

	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), NULL);

	value = arv_gc_enumeration_get_int_value (enumeration);

	for (iter = enumeration->entries; iter != NULL; iter = iter->next) {
		if (arv_gc_enum_entry_get_value (iter->data) == value) {
			const char *string;

			string = arv_gc_feature_node_get_name (iter->data);
			arv_log_genicam ("[GcEnumeration::get_string_value] value = %Ld - string = %s",
					 value, string);
			return string;
		}
	}

	arv_warning_genicam ("[GcEnumeration::get_string_value] value = %Ld not found for node %s",
			     value, arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));

	return NULL;
}

void
arv_gc_enumeration_set_string_value (ArvGcEnumeration *enumeration, const char *value)
{
	const GSList *iter;

	g_return_if_fail (ARV_IS_GC_ENUMERATION (enumeration));

	for (iter = enumeration->entries; iter != NULL; iter = iter->next)
		if (g_strcmp0 (arv_gc_feature_node_get_name (iter->data), value) == 0) {
			arv_log_genicam ("[GcEnumeration::set_string_value] value = %d - string = %s",
					 &enumeration->value, value);
			arv_gc_enumeration_set_int_value (enumeration, arv_gc_enum_entry_get_value (iter->data));
			return;
		}

	arv_warning_genicam ("[GcEnumeration::set_string_value] entry %s not found", value);
}

gint64
arv_gc_enumeration_get_int_value (ArvGcEnumeration *enumeration)
{
	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), 0);

	if (enumeration->value != NULL)
		return arv_gc_property_node_get_int64 (enumeration->value);

	return 0;
}

gint64 *
arv_gc_enumeration_get_available_int_values (ArvGcEnumeration *enumeration, guint *n_values)
{
	gint64 *values;
	const GSList *entries, *iter;
	unsigned int i;

	g_return_val_if_fail (n_values != NULL, NULL);

	*n_values = 0;

	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), NULL);

	entries = arv_gc_enumeration_get_entries (enumeration);

	*n_values = 0;
	for (iter = entries; iter != NULL; iter = iter->next)
		if (arv_gc_feature_node_is_available (iter->data) &&
		    arv_gc_feature_node_is_implemented (iter->data))
		    (*n_values)++;

	if (*n_values == 0)
		return NULL;

	values = g_new (gint64, *n_values);
	for (iter = entries, i = 0; iter != NULL; iter = iter->next)
		if (arv_gc_feature_node_is_available (iter->data) &&
		    arv_gc_feature_node_is_implemented (iter->data)) {
			values[i] = arv_gc_enum_entry_get_value (iter->data);
			i++;
		}

	return values;
}

void
arv_gc_enumeration_set_int_value (ArvGcEnumeration *enumeration, gint64 value)
{
	g_return_if_fail (ARV_IS_GC_ENUMERATION (enumeration));

	if (enumeration->value)
		arv_gc_property_node_set_int64 (enumeration->value, value);
}

/**
 * arv_gc_enumeration_get_entries:
 * @enumeration: a #ArvGcEnumeration
 *
 * Returns: (element-type ArvGcFeatureNode) (transfer none): the list of enumeration entry nodes.
 */

const GSList *
arv_gc_enumeration_get_entries (ArvGcEnumeration *enumeration)
{
	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), NULL);

	return enumeration->entries;
}

ArvGcNode *
arv_gc_enumeration_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_ENUMERATION, NULL);

	return node;
}

static void
arv_gc_enumeration_init (ArvGcEnumeration *gc_enumeration)
{
}

static void
arv_gc_enumeration_finalize (GObject *object)
{
	ArvGcEnumeration *enumeration = ARV_GC_ENUMERATION (object);

	g_slist_free (enumeration->entries);
	enumeration->entries = NULL;

	parent_class->finalize (object);
}

static void
arv_gc_enumeration_class_init (ArvGcEnumerationClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_enumeration_finalize;

	dom_node_class->get_node_name = arv_gc_enumeration_get_node_name;
	dom_node_class->can_append_child = arv_gc_enumeration_can_append_child;
	dom_node_class->post_new_child = arv_gc_enumeration_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_enumeration_pre_remove_child;
	gc_feature_node_class->set_value_from_string = arv_gc_enumeration_set_value_from_string;
	gc_feature_node_class->get_value_as_string = arv_gc_enumeration_get_value_as_string;
}

G_DEFINE_TYPE (ArvGcEnumeration, arv_gc_enumeration, ARV_TYPE_GC_FEATURE_NODE)
