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
 * SECTION: arvgcenumeration
 * @short_description: Class for Enumeration nodes
 */

#include <arvgcenumeration.h>
#include <arvgcenumentry.h>
#include <arvgcinteger.h>
#include <arvgcstring.h>
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
arv_gc_enumeration_set_value_from_string (ArvGcFeatureNode *node, const char *string, GError **error)
{
	GError *local_error = NULL;

	arv_gc_enumeration_set_string_value (ARV_GC_ENUMERATION (node), string, &local_error);

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

static const char *
arv_gc_enumeration_get_value_as_string (ArvGcFeatureNode *node, GError **error)
{
	const char *string;
	GError *local_error = NULL;

	string = arv_gc_enumeration_get_string_value (ARV_GC_ENUMERATION (node), &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return NULL;
	}

	return string;
}

GType
arv_gc_enumeration_get_value_type (ArvGcFeatureNode *node)
{
	return G_TYPE_STRING;
}


/* ArvGcEnumeration implementation */

const char *
arv_gc_enumeration_get_string_value (ArvGcEnumeration *enumeration, GError **error)
{
	const GSList *iter;
	GError *local_error = NULL;
	gint64 value;

	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	value = arv_gc_enumeration_get_int_value (enumeration, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return NULL;
	}

	for (iter = enumeration->entries; iter != NULL; iter = iter->next) {
		gint64 enum_value;

		enum_value = arv_gc_enum_entry_get_value (iter->data, &local_error);

		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return NULL;
		}

		if (enum_value == value) {
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
arv_gc_enumeration_set_string_value (ArvGcEnumeration *enumeration, const char *value, GError **error)
{
	const GSList *iter;

	g_return_if_fail (ARV_IS_GC_ENUMERATION (enumeration));
	g_return_if_fail (error == NULL || *error == NULL);

	for (iter = enumeration->entries; iter != NULL; iter = iter->next)
		if (g_strcmp0 (arv_gc_feature_node_get_name (iter->data), value) == 0) {
			GError *local_error = NULL;
			gint64 enum_value;

			arv_log_genicam ("[GcEnumeration::set_string_value] value = %d - string = %s",
					 &enumeration->value, value);

			enum_value = arv_gc_enum_entry_get_value (iter->data, &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}

			arv_gc_enumeration_set_int_value (enumeration, enum_value, &local_error);

			if (local_error != NULL)
				g_propagate_error (error, local_error);

			return;
		}

	arv_warning_genicam ("[GcEnumeration::set_string_value] entry %s not found", value);
}

gint64
arv_gc_enumeration_get_int_value (ArvGcEnumeration *enumeration, GError **error)
{
	GError *local_error = NULL;
	gint64 value;

	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	if (enumeration->value == NULL) 
		return 0;

	value = arv_gc_property_node_get_int64 (enumeration->value, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	return value;
}

gint64 *
arv_gc_enumeration_get_available_int_values (ArvGcEnumeration *enumeration, guint *n_values, GError **error)
{
	gint64 *values;
	const GSList *entries, *iter;
	GSList *available_entries = NULL;
	unsigned int i;
	GError *local_error = NULL;

	g_return_val_if_fail (n_values != NULL, NULL);

	*n_values = 0;

	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	entries = arv_gc_enumeration_get_entries (enumeration);

	*n_values = 0;
	for (iter = entries; iter != NULL; iter = iter->next) {
		gboolean is_available;

		is_available = arv_gc_feature_node_is_available (iter->data, &local_error);

		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			*n_values = 0;
			g_slist_free (available_entries);

			return NULL;
		}

		if (is_available) {
			gboolean is_implemented;

			is_implemented = arv_gc_feature_node_is_implemented (iter->data, &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				*n_values = 0;
				g_slist_free (available_entries);

				return NULL;
			}

			if (is_implemented) {
				(*n_values)++;
				available_entries = g_slist_prepend (available_entries, iter->data);
			}
		}
	}

	if (*n_values == 0) {
		g_slist_free (available_entries);
		return NULL;
	}

	values = g_new (gint64, *n_values);
	for (iter = available_entries, i = 0; iter != NULL; iter = iter->next) {

		values[i] = arv_gc_enum_entry_get_value (iter->data, &local_error);

		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			*n_values = 0;
			g_slist_free (available_entries);
			g_free (values);

			return NULL;
		}
		i++;
	}

	g_slist_free (available_entries);

	return values;
}

/**
 * arv_gc_enumeration_get_available_string_values:
 * @enumeration: an #ArvGcEnumeration
 * @n_values: (out): placeholder for the number of values
 * @error: placeholder for error, may be NULL
 *
 * Create an array of all available values of @enumeration, as strings.
 *
 * Returns: (array length=n_values) (transfer container): an newly created array of const strings, which must freed after use using g_free.
 */

const char **
arv_gc_enumeration_get_available_string_values (ArvGcEnumeration *enumeration, guint *n_values, GError **error)
{
	const char ** strings;
	const GSList *entries, *iter;
	GSList *available_entries = NULL;
	unsigned int i;
	GError *local_error = NULL;

	g_return_val_if_fail (n_values != NULL, NULL);

	*n_values = 0;

	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	entries = arv_gc_enumeration_get_entries (enumeration);

	*n_values = 0;
	for (iter = entries; iter != NULL; iter = iter->next) {
		gboolean is_available;

		is_available = arv_gc_feature_node_is_available (iter->data, &local_error);

		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			*n_values = 0;
			g_slist_free (available_entries);

			return NULL;
		}

		if (is_available) {
			gboolean is_implemented;

			is_implemented = arv_gc_feature_node_is_implemented (iter->data, &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				*n_values = 0;
				g_slist_free (available_entries);

				return NULL;
			}

			if (is_implemented) {
				(*n_values)++;
				available_entries = g_slist_prepend (available_entries, iter->data);
			}
		}
	}

	if (*n_values == 0) {
		g_slist_free (available_entries);
		return NULL;
	}

	strings = g_new (const char*, *n_values);
	for (iter = available_entries, i = 0; iter != NULL; iter = iter->next, i++)
		strings[i] = arv_gc_feature_node_get_name (iter->data);

	g_slist_free (available_entries);

	return strings;
}

void
arv_gc_enumeration_set_int_value (ArvGcEnumeration *enumeration, gint64 value, GError **error)
{
	g_return_if_fail (ARV_IS_GC_ENUMERATION (enumeration));
	g_return_if_fail (error == NULL || *error == NULL);

	if (enumeration->value) {
		GError *local_error = NULL;

		arv_gc_property_node_set_int64 (enumeration->value, value, &local_error);

		if (local_error != NULL)
			g_propagate_error (error, local_error);
	}
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
	gc_feature_node_class->get_value_type = arv_gc_enumeration_get_value_type;
}

/* ArvGcInteger interface implementation */

static gint64
arv_gc_enumeration_get_integer_value (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcEnumeration *gc_enumeration = ARV_GC_ENUMERATION (gc_integer);

	return arv_gc_enumeration_get_int_value (gc_enumeration, error);
}

static void
arv_gc_enumeration_set_integer_value (ArvGcInteger *gc_integer, gint64 value, GError **error)
{
	ArvGcEnumeration *gc_enumeration = ARV_GC_ENUMERATION (gc_integer);

	return arv_gc_enumeration_set_int_value (gc_enumeration, value, error);
}

static void
arv_gc_enumeration_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_enumeration_get_integer_value;
	interface->set_value = arv_gc_enumeration_set_integer_value;
}

static const char *
arv_gc_enumeration_get_str_value (ArvGcString *gc_string, GError **error)
{
	ArvGcEnumeration *gc_enumeration = ARV_GC_ENUMERATION (gc_string);

	return arv_gc_enumeration_get_string_value (gc_enumeration, error);
}

static void
arv_gc_enumeration_set_str_value (ArvGcString *gc_string, const char *value, GError **error)
{
	ArvGcEnumeration *gc_enumeration = ARV_GC_ENUMERATION (gc_string);

	arv_gc_enumeration_set_string_value (gc_enumeration, value, error);
}

static gint64
arv_gc_enumeration_get_max_string_length (ArvGcString *gc_string, GError **error)
{
	ArvGcEnumeration *gc_enumeration = ARV_GC_ENUMERATION (gc_string);
	const GSList *entries, *iter;
	gint64 length, max_length = 0;

	entries = arv_gc_enumeration_get_entries (gc_enumeration);
	for (iter = entries; iter != NULL; iter = iter->next) {
		const char *name;

		name = arv_gc_feature_node_get_name (iter->data);
		length = name != NULL ? strlen (name) : 0;
		if (length > max_length)
			max_length = length;
	}

	return max_length;
}

static void
arv_gc_enumeration_string_interface_init (ArvGcStringInterface *interface)
{
	interface->get_value = arv_gc_enumeration_get_str_value;
	interface->set_value = arv_gc_enumeration_set_str_value;
	interface->get_max_length = arv_gc_enumeration_get_max_string_length;
}

G_DEFINE_TYPE_WITH_CODE (ArvGcEnumeration, arv_gc_enumeration, ARV_TYPE_GC_FEATURE_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_enumeration_integer_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_STRING, arv_gc_enumeration_string_interface_init))
