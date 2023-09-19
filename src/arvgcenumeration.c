/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

/**
 * SECTION: arvgcenumeration
 * @short_description: Class for Enumeration nodes
 */

#include <arvgcenumeration.h>
#include <arvgcenumentry.h>
#include <arvgcinteger.h>
#include <arvgcselector.h>
#include <arvgcstring.h>
#include <arvgcfeaturenodeprivate.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <arvdebugprivate.h>
#include <string.h>

struct _ArvGcEnumeration {
	ArvGcFeatureNode base;

	ArvGcPropertyNode *value;
	GSList *entries;

	GSList *selecteds;		/* #ArvGcPropertyNode */
	GSList *selected_features;	/* #ArvGcFeatureNode */
};

struct _ArvGcEnumerationClass {
	ArvGcFeatureNodeClass parent_class;
};

static void arv_gc_enumeration_integer_interface_init (ArvGcIntegerInterface *interface);
static void arv_gc_enumeration_string_interface_init (ArvGcStringInterface *interface);
static void arv_gc_enumeration_selector_interface_init (ArvGcSelectorInterface *interface);

G_DEFINE_TYPE_WITH_CODE (ArvGcEnumeration, arv_gc_enumeration, ARV_TYPE_GC_FEATURE_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_enumeration_integer_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_STRING, arv_gc_enumeration_string_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_SELECTOR, arv_gc_enumeration_selector_interface_init))

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
			case ARV_GC_PROPERTY_NODE_TYPE_P_SELECTED:
				node->selecteds = g_slist_prepend (node->selecteds, property_node);
				break;
			default:
				ARV_DOM_NODE_CLASS (arv_gc_enumeration_parent_class)->post_new_child (self, child);
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

/* ArvGcEnumeration implementation */

/**
 * arv_gc_enumeration_dup_available_int_values:
 * @enumeration: a #ArvGcEnumeration
 * @n_values: (out): the number of values
 * @error: (out): the error that occured, or NULL
 *
 * Return value: (transfer full) (array length=n_values): a newly allocated array of 64 bit integers, to be freed after
 * use using g_free().
 *
 * Since: 0.8.0
 */

gint64 *
arv_gc_enumeration_dup_available_int_values (ArvGcEnumeration *enumeration, guint *n_values, GError **error)
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
			g_propagate_prefixed_error (error, local_error, "[%s] ",
                                                    arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));
			*n_values = 0;
			g_slist_free (available_entries);

			return NULL;
		}

		if (is_available) {
			gboolean is_implemented;

			is_implemented = arv_gc_feature_node_is_implemented (iter->data, &local_error);

			if (local_error != NULL) {
                                g_propagate_prefixed_error (error, local_error, "[%s] ",
                                                            arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));
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
                        g_propagate_prefixed_error (error, local_error, "[%s] ",
                                                    arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));
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

static const char **
_dup_available_string_values (ArvGcEnumeration *enumeration, gboolean display_name ,guint *n_values, GError **error)
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
                        g_propagate_prefixed_error (error, local_error, "[%s] ",
                                                    arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));
			*n_values = 0;
			g_slist_free (available_entries);

			return NULL;
		}

		if (is_available) {
			gboolean is_implemented;

			is_implemented = arv_gc_feature_node_is_implemented (iter->data, &local_error);

			if (local_error != NULL) {
                                g_propagate_prefixed_error (error, local_error, "[%s] ",
                                                            arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));
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
	for (iter = available_entries, i = 0; iter != NULL; iter = iter->next, i++) {
		const char *string = NULL;
		if (display_name)
			string = arv_gc_feature_node_get_display_name (iter->data);
		if (string == NULL)
			string = arv_gc_feature_node_get_name (iter->data);
		strings[i] = string;
	}

	g_slist_free (available_entries);

	return strings;
}

/**
 * arv_gc_enumeration_dup_available_string_values:
 * @enumeration: an #ArvGcEnumeration
 * @n_values: (out): placeholder for the number of values
 * @error: placeholder for error, may be NULL
 *
 * Create an array of all available values of @enumeration, as strings.
 *
 * Returns: (array length=n_values) (transfer container): an newly created array of const strings, which must freed after use using g_free,
 * %NULL on error.
 *
 * Since: 0.8.0
 */

const char **
arv_gc_enumeration_dup_available_string_values (ArvGcEnumeration *enumeration, guint *n_values, GError **error)
{
	return _dup_available_string_values (enumeration, FALSE, n_values, error);
}

/**
 * arv_gc_enumeration_dup_available_display_names:
 * @enumeration: an #ArvGcEnumeration
 * @n_values: (out): placeholder for the number of values
 * @error: placeholder for error, may be NULL
 *
 * Create an array of display names of all available entries.
 *
 * Returns: (array length=n_values) (transfer container): an newly created array of const strings, which must freed after use using g_free,
 * %NULL on error.
 *
 * Since: 0.8.0
 */

const char **
arv_gc_enumeration_dup_available_display_names (ArvGcEnumeration *enumeration, guint *n_values, GError **error)
{
	return _dup_available_string_values (enumeration, TRUE, n_values, error);
}

static gint64
_get_int_value (ArvGcEnumeration *enumeration, GError **error)
{
	GError *local_error = NULL;
	gint64 value;

	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	if (enumeration->value == NULL)
		return 0;

	value = arv_gc_property_node_get_int64 (enumeration->value, &local_error);

	if (local_error != NULL) {
		g_propagate_prefixed_error (error, local_error, "[%s] ",
                                            arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));
		return 0;
	}

	return value;
}

gint64
arv_gc_enumeration_get_int_value (ArvGcEnumeration *enumeration, GError **error)
{

        if (!arv_gc_feature_node_check_read_access (ARV_GC_FEATURE_NODE (enumeration), error))
                return 0;

        return _get_int_value (enumeration, error);
}

static gboolean
_set_int_value (ArvGcEnumeration *enumeration, gint64 value, GError **error)
{
	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	if (enumeration->value) {
		GError *local_error = NULL;

		{
			gint64 *available_values;
			unsigned n_values;
			unsigned i;
			gboolean found = FALSE;

			available_values = arv_gc_enumeration_dup_available_int_values (enumeration, &n_values, &local_error);
			if (local_error != NULL) {
                                g_propagate_prefixed_error (error, local_error, "[%s] ",
                                                            arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));
				return FALSE;
			}

			if (available_values == NULL) {
				g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_EMPTY_ENUMERATION,
					     "[%s] No available entry found",
					     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));
				return FALSE;
			}

			for (i = 0; i < n_values; i++)
				if (available_values[i] == value)
					found = TRUE;

			g_free (available_values);

			if (!found) {
				g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_OUT_OF_RANGE,
					     "[%s] Value %" G_GINT64_FORMAT " not found",
					     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)),
                                             value);
				return FALSE;
			}
		}

		arv_gc_feature_node_increment_change_count (ARV_GC_FEATURE_NODE (enumeration));
		arv_gc_property_node_set_int64 (enumeration->value, value, &local_error);

		if (local_error != NULL) {
                        g_propagate_prefixed_error (error, local_error, "[%s] ",
                                                    arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));
                        return FALSE;
                }

		return TRUE;
	}

	g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_PROPERTY_NOT_DEFINED,
		     "[%s] <Value> node not found", arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));

	return FALSE;
}

gboolean
arv_gc_enumeration_set_int_value (ArvGcEnumeration *enumeration, gint64 value, GError **error)
{
        if (!arv_gc_feature_node_check_write_access (ARV_GC_FEATURE_NODE (enumeration), error))
                return FALSE;

        return _set_int_value (enumeration, value, error);
}

static const char *
_get_string_value (ArvGcEnumeration *enumeration, GError **error)
{
	const GSList *iter;
	GError *local_error = NULL;
	gint64 value;

	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	value = _get_int_value (enumeration, &local_error);

	if (local_error != NULL) {
		g_propagate_prefixed_error (error, local_error, "[%s] ",
                                            arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));
		return NULL;
	}

	for (iter = enumeration->entries; iter != NULL; iter = iter->next) {
		gint64 enum_value;

		enum_value = arv_gc_enum_entry_get_value (iter->data, &local_error);

		if (local_error != NULL) {
                        g_propagate_prefixed_error (error, local_error, "[%s] ",
                                                    arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));
                        return NULL;
                }

		if (enum_value == value) {
			const char *string;

			string = arv_gc_feature_node_get_name (iter->data);
			arv_debug_genicam ("[GcEnumeration::get_string_value] value = %" G_GINT64_FORMAT " - string = %s",
					 value, string);
			return string;
		}
	}

	arv_warning_genicam ("[GcEnumeration::get_string_value] value = %" G_GINT64_FORMAT " not found for node %s",
			     value, arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));

	return NULL;
}

const char *
arv_gc_enumeration_get_string_value (ArvGcEnumeration *enumeration, GError **error)
{
        if (!arv_gc_feature_node_check_read_access (ARV_GC_FEATURE_NODE (enumeration), error))
                return NULL;

        return _get_string_value (enumeration, error);
}

static gboolean
_set_string_value (ArvGcEnumeration *enumeration, const char *value, GError **error)
{
	const GSList *iter;

	g_return_val_if_fail (ARV_IS_GC_ENUMERATION (enumeration), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	for (iter = enumeration->entries; iter != NULL; iter = iter->next)
		if (g_strcmp0 (arv_gc_feature_node_get_name (iter->data), value) == 0) {
			GError *local_error = NULL;
			gint64 enum_value;

			enum_value = arv_gc_enum_entry_get_value (iter->data, &local_error);

			arv_debug_genicam ("[GcEnumeration::set_string_value] value = %" G_GINT64_FORMAT " - string = %s",
					 enum_value, value);

			if (local_error != NULL) {
                                g_propagate_prefixed_error (error, local_error, "[%s] ",
                                                            arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));
                                return FALSE;
                        }

			_set_int_value (enumeration, enum_value, &local_error);

			if (local_error != NULL) {
                                g_propagate_prefixed_error (error, local_error, "[%s] ",
                                                            arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)));
				return FALSE;
			}

			return TRUE;
		}

	arv_warning_genicam ("[GcEnumeration::set_string_value] entry %s not found", value);

	g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_ENUM_ENTRY_NOT_FOUND, "[%s] '%s' not an entry",
		     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (enumeration)), value);

	return FALSE;
}

gboolean
arv_gc_enumeration_set_string_value (ArvGcEnumeration *enumeration, const char *value, GError **error)
{
        if (!arv_gc_feature_node_check_write_access (ARV_GC_FEATURE_NODE (enumeration), error))
                return FALSE;

        return _set_string_value (enumeration, value, error);
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

static ArvGcFeatureNode *
arv_gc_enumeration_get_linked_feature (ArvGcFeatureNode *gc_feature_node)
{
	ArvGcEnumeration *gc_enumeration = ARV_GC_ENUMERATION (gc_feature_node);
	ArvGcNode *pvalue_node = NULL;

	if (gc_enumeration->value == NULL)
		return NULL;

	pvalue_node = arv_gc_property_node_get_linked_node (gc_enumeration->value);
	if (ARV_IS_GC_FEATURE_NODE (pvalue_node))
		return ARV_GC_FEATURE_NODE (pvalue_node);

	return NULL;
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

	g_clear_pointer (&enumeration->entries, g_slist_free);
	g_clear_pointer (&enumeration->selecteds, g_slist_free);
	g_clear_pointer (&enumeration->selected_features, g_slist_free);

	G_OBJECT_CLASS (arv_gc_enumeration_parent_class)->finalize (object);
}

static void
arv_gc_enumeration_class_init (ArvGcEnumerationClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);

	object_class->finalize = arv_gc_enumeration_finalize;
	dom_node_class->get_node_name = arv_gc_enumeration_get_node_name;
	dom_node_class->can_append_child = arv_gc_enumeration_can_append_child;
	dom_node_class->post_new_child = arv_gc_enumeration_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_enumeration_pre_remove_child;
	gc_feature_node_class->get_linked_feature = arv_gc_enumeration_get_linked_feature;
        gc_feature_node_class->default_access_mode = ARV_GC_ACCESS_MODE_RW;
}

/* ArvGcInteger interface implementation */

static gint64
arv_gc_enumeration_get_integer_value (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcEnumeration *gc_enumeration = ARV_GC_ENUMERATION (gc_integer);

	return _get_int_value (gc_enumeration, error);
}

static void
arv_gc_enumeration_set_integer_value (ArvGcInteger *gc_integer, gint64 value, GError **error)
{
	ArvGcEnumeration *gc_enumeration = ARV_GC_ENUMERATION (gc_integer);

	_set_int_value (gc_enumeration, value, error);
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

	return _get_string_value (gc_enumeration, error);
}

static void
arv_gc_enumeration_set_str_value (ArvGcString *gc_string, const char *value, GError **error)
{
	ArvGcEnumeration *gc_enumeration = ARV_GC_ENUMERATION (gc_string);

	_set_string_value (gc_enumeration, value, error);
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

static const GSList *
arv_gc_enumeration_get_selected_features (ArvGcSelector *selector)
{
	ArvGcEnumeration *enumeration = ARV_GC_ENUMERATION (selector);
	GSList *iter;

	g_clear_pointer (&enumeration->selected_features, g_slist_free);
	for (iter = enumeration->selecteds; iter != NULL; iter = iter->next) {
		ArvGcFeatureNode *feature_node = ARV_GC_FEATURE_NODE (arv_gc_property_node_get_linked_node (iter->data));
		if (ARV_IS_GC_FEATURE_NODE (feature_node))
		    enumeration->selected_features = g_slist_prepend (enumeration->selected_features, feature_node);
	}

	return enumeration->selected_features;
}

static void
arv_gc_enumeration_selector_interface_init (ArvGcSelectorInterface *interface)
{
	interface->get_selected_features = arv_gc_enumeration_get_selected_features;
}
