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

/* ArvGcNode implementation */

static const char *
arv_gc_enumeration_get_node_name (ArvGcNode *node)
{
	return "Enumeration";
}

static gboolean
arv_gc_enumeration_can_add_child (ArvGcNode *node, ArvGcNode *child)
{
	if (ARV_IS_GC_ENUM_ENTRY (child))
		return TRUE;

	return FALSE;
}

static void
arv_gc_enumeration_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	ArvGcEnumeration *gc_enumeration = ARV_GC_ENUMERATION (node);

	if (strcmp (name, "Value") == 0) {
		arv_force_g_value_to_int64 (&gc_enumeration->value,
					    g_ascii_strtoll (content, NULL, 0));
	} else if (strcmp (name, "pValue") == 0) {
		arv_force_g_value_to_string (&gc_enumeration->value, content);
	} else
		ARV_GC_NODE_CLASS (parent_class)->add_element (node, name, content, attributes);
}

static void
arv_gc_enumeration_set_value_from_string (ArvGcNode *node, const char *string)
{
	arv_gc_enumeration_set_string_value (ARV_GC_ENUMERATION (node), string);
}

static const char *
arv_gc_enumeration_get_value_as_string (ArvGcNode *node)
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

	value = arv_gc_get_int64_from_value (arv_gc_node_get_genicam (ARV_GC_NODE (enumeration)),
								      &enumeration->value);

	for (iter = arv_gc_node_get_childs (ARV_GC_NODE (enumeration)); iter != NULL; iter = iter->next) {
		if (arv_gc_enum_entry_get_value (iter->data) == value) {
			const char *string;

			string = arv_gc_node_get_name (iter->data);
			arv_log_genicam ("[GcEnumeration::get_string_value] value = %Ld - string = %s",
					 value, string);
			return string;
		}
	}

	arv_warning_genicam ("[GcEnumeration::get_string_value] value = %Ld not found for node %s",
			     value, arv_gc_node_get_name (ARV_GC_NODE (enumeration)));

	return NULL;
}

void
arv_gc_enumeration_set_string_value (ArvGcEnumeration *enumeration, const char *value)
{
	const GSList *iter;

	g_return_if_fail (ARV_IS_GC_ENUMERATION (enumeration));

	for (iter = arv_gc_node_get_childs (ARV_GC_NODE (enumeration)); iter != NULL; iter = iter->next)
		if (g_strcmp0 (arv_gc_node_get_name (iter->data), value) == 0) {
			arv_log_genicam ("[GcEnumeration::set_string_value] value = %d - string = %s",
					 &enumeration->value, value);
			arv_gc_set_int64_to_value (arv_gc_node_get_genicam (ARV_GC_NODE (enumeration)),
						   &enumeration->value,
						   arv_gc_enum_entry_get_value (iter->data));
			return;
		}

	arv_warning_genicam ("[GcEnumeration::set_string_value] entry %s not found", value);
}

gint64
arv_gc_enumeration_get_int_value (ArvGcEnumeration *enumeration)
{
	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), 0);

	return arv_gc_get_int64_from_value (arv_gc_node_get_genicam (ARV_GC_NODE (enumeration)),
					    &enumeration->value);
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
		if (arv_gc_node_is_available (iter->data))
		    (*n_values)++;

	if (*n_values == 0)
		return NULL;

	values = g_new (gint64, *n_values);
	for (iter = entries, i = 0; iter != NULL; iter = iter->next)
		if (arv_gc_node_is_available (iter->data)) {
			values[i] = arv_gc_enum_entry_get_value (iter->data);
			i++;
		}

	return values;
}

void
arv_gc_enumeration_set_int_value (ArvGcEnumeration *enumeration, gint64 value)
{
	g_return_if_fail (ARV_IS_GC_ENUMERATION (enumeration));

	arv_gc_set_int64_to_value (arv_gc_node_get_genicam (ARV_GC_NODE (enumeration)),
				   &enumeration->value, value);
}

/**
 * arv_gc_enumeration_get_entries:
 * enumeration: a #ArvGcEnumeration
 *
 * Returns: (element-type ArvGcNode) (transfer none): the list of enumeration entry nodes.
 */

const GSList *
arv_gc_enumeration_get_entries (ArvGcEnumeration *enumeration)
{
	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), NULL);

	return arv_gc_node_get_childs (ARV_GC_NODE (enumeration));
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
	g_value_init (&gc_enumeration->value, G_TYPE_INT64);
	g_value_set_int64 (&gc_enumeration->value, 0);
}

static void
arv_gc_enumeration_finalize (GObject *object)
{
	ArvGcEnumeration *gc_enumeration = ARV_GC_ENUMERATION (object);

	g_value_unset (&gc_enumeration->value);

	parent_class->finalize (object);
}

static void
arv_gc_enumeration_class_init (ArvGcEnumerationClass *enumeration_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (enumeration_class);
	ArvGcNodeClass *node_class = ARV_GC_NODE_CLASS (enumeration_class);

	parent_class = g_type_class_peek_parent (enumeration_class);

	object_class->finalize = arv_gc_enumeration_finalize;

	node_class->get_node_name = arv_gc_enumeration_get_node_name;
	node_class->add_element = arv_gc_enumeration_add_element;
	node_class->can_add_child = arv_gc_enumeration_can_add_child;
	node_class->set_value_from_string = arv_gc_enumeration_set_value_from_string;
	node_class->get_value_as_string = arv_gc_enumeration_get_value_as_string;
}

G_DEFINE_TYPE (ArvGcEnumeration, arv_gc_enumeration, ARV_TYPE_GC_NODE)
