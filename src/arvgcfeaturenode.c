/* Aravis - Digital camera library
 *
 * Copyright © 2009-2019 Emmanuel Pacaud
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
 * SECTION: arvgcfeaturenode
 * @short_description: Base class for Genicam feature nodes
 *
 * #ArvGcFeatureNode provides a base class for the implementation of the different
 * types of Genicam feature node (Group, Integer, Float, Enumeration...).
 */

#include <arvgcfeaturenode.h>
#include <arvgcpropertynode.h>
#include <arvgc.h>
#include <arvgcenums.h>
#include <arvmisc.h>
#include <arvdebug.h>
#include <string.h>

struct _ArvGcFeatureNodePrivate {

	char *name;
	ArvGcNameSpace name_space;

	ArvGcPropertyNode *tooltip;
	ArvGcPropertyNode *description;
	ArvGcPropertyNode *display_name;
	ArvGcPropertyNode *is_implemented;
	ArvGcPropertyNode *is_available;
	ArvGcPropertyNode *is_locked;

	guint64 change_count;
};

/* ArvDomNode implementation */

static gboolean
arv_gc_feature_node_can_append_child (ArvDomNode *self, ArvDomNode *child)
{
	return ARV_IS_GC_NODE (child);
}

static void
arv_gc_feature_node_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcFeatureNode *node = ARV_GC_FEATURE_NODE (self);

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP:
				node->priv->tooltip = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION:
				node->priv->description = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME:
				node->priv->display_name = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_IS_AVAILABLE:
				node->priv->is_available = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_IS_IMPLEMENTED:
				node->priv->is_implemented = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_IS_LOCKED:
				node->priv->is_locked = property_node;
				break;
			default:
				break;
		}
	}
}

static void
arv_gc_feature_node_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcFeatureNode *node = ARV_GC_FEATURE_NODE (self);

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP:
				node->priv->tooltip = NULL;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION:
				node->priv->description = NULL;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME:
				node->priv->description = NULL;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_IS_AVAILABLE:
				node->priv->is_available = NULL;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_IS_IMPLEMENTED:
				node->priv->is_implemented = NULL;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_IS_LOCKED:
				node->priv->is_locked = NULL;
				break;
			default:
				break;
		}
	}
}

/* ArvDomNode implementation */

static void
arv_gc_feature_node_set_attribute (ArvDomElement *self, const char *name, const char *value)
{
	ArvGcFeatureNode *node = ARV_GC_FEATURE_NODE (self);

	if (strcmp (name, "Name") == 0) {
		ArvGc *genicam;

		g_free (node->priv->name);
		node->priv->name = g_strdup (value);

		genicam = arv_gc_node_get_genicam (ARV_GC_NODE (self));
		/* Kludge around ugly Genicam specification (Really, pre-parsing for EnumEntry Name substitution ?) */
		if (strcmp (arv_dom_node_get_node_name (ARV_DOM_NODE (node)), "EnumEntry") != 0)
			arv_gc_register_feature_node (genicam, node);
	} else if (strcmp (name, "NameSpace") == 0) {
		if (g_strcmp0 (value, "Standard") == 0)
			node->priv->name_space = ARV_GC_NAME_SPACE_STANDARD;
		else
			node->priv->name_space = ARV_GC_NAME_SPACE_CUSTOM;
	} else
		arv_debug_dom ("[GcFeature::set_attribute] Unknown attribute '%s'", name);
}

static const char *
arv_gc_feature_node_get_attribute (ArvDomElement *self, const char *name)
{
	ArvGcFeatureNode *node = ARV_GC_FEATURE_NODE (self);

	if (strcmp (name, "Name") == 0)
		return node->priv->name;
	else if (strcmp (name, "NameSpace") == 0)
		switch (node->priv->name_space) {
			case ARV_GC_NAME_SPACE_STANDARD:
				return "Standard";
			default:
				return "Custom";
		}

	arv_debug_dom ("[GcFeature::set_attribute] Unknown attribute '%s'", name);

	return NULL;
}

/* ArvGcFeatureNode implementation */

const char *
arv_gc_feature_node_get_name (ArvGcFeatureNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (node), NULL);

	return node->priv->name;
}

const char *
arv_gc_feature_node_get_tooltip (ArvGcFeatureNode *node, GError **error)
{
	const char *tooltip;
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (node), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (node->priv->tooltip == NULL)
		return NULL;

	tooltip = arv_gc_property_node_get_string (node->priv->tooltip, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return NULL;
	}

	return tooltip;
}

const char *
arv_gc_feature_node_get_description (ArvGcFeatureNode *node, GError **error)
{
	const char *description;
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (node), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (node->priv->description == NULL)
	       return NULL;

	description = arv_gc_property_node_get_string (node->priv->description, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return NULL;
	}

	return description;
}

const char *
arv_gc_feature_node_get_display_name (ArvGcFeatureNode *node, GError **error)
{
	const char *display_name;
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (node), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (node->priv->display_name == NULL)
	       return NULL;

	display_name = arv_gc_property_node_get_string (node->priv->display_name, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return NULL;
	}

	return display_name;
}

gboolean
arv_gc_feature_node_is_implemented (ArvGcFeatureNode *gc_feature_node, GError **error)
{
	gboolean value;
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	if (gc_feature_node->priv->is_implemented == NULL)
		return TRUE;


	value = arv_gc_property_node_get_int64 (gc_feature_node->priv->is_implemented, &local_error) != 0;

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return FALSE;
	}

	return value;
}

gboolean
arv_gc_feature_node_is_available (ArvGcFeatureNode *gc_feature_node, GError **error)
{
	gboolean value;
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	if (gc_feature_node->priv->is_available == NULL)
		return TRUE;

	value = arv_gc_property_node_get_int64 (gc_feature_node->priv->is_available, &local_error) != 0;

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return FALSE;
	}

	return value;
}

gboolean
arv_gc_feature_node_is_locked (ArvGcFeatureNode *gc_feature_node, GError **error)
{
	gboolean value;
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node), FALSE);

	if (gc_feature_node->priv->is_locked == NULL)
		return FALSE;

	value = arv_gc_property_node_get_int64 (gc_feature_node->priv->is_locked, &local_error) != 0;

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return FALSE;
	}

	return value;
}

/**
 * arv_gc_feature_node_set_value_from_string:
 * @gc_feature_node: a #ArvGcFeatureNode
 * @string: new node value, as string
 * @error: return location for a GError, or NULL
 *
 * Set the node value using a string representation of the value. May not be applicable to every node type, but safe.
 */

void
arv_gc_feature_node_set_value_from_string (ArvGcFeatureNode *gc_feature_node, const char *string, GError **error)
{
	ArvGcFeatureNodeClass *node_class;

	g_return_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node));
	g_return_if_fail (error == NULL || *error == NULL);
	g_return_if_fail (string != NULL);

	node_class = ARV_GC_FEATURE_NODE_GET_CLASS (gc_feature_node);
	if (node_class->set_value_from_string != NULL)
		node_class->set_value_from_string (gc_feature_node, string, error);
}

/**
 * arv_gc_feature_node_get_value_as_string:
 * @gc_feature_node: a #ArvGcFeatureNode
 * @error: return location for a GError, or NULL
 *
 * Retrieve the node value a string.
 *
 * <warning><para>Please note the string content is still owned by the @node object, which means the returned pointer may not be still valid after a new call to this function.</para></warning>
 *
 * Returns: (transfer none): a string representation of the node value, %NULL if not applicable.
 */

const char *
arv_gc_feature_node_get_value_as_string (ArvGcFeatureNode *gc_feature_node, GError **error)
{
	ArvGcFeatureNodeClass *node_class;

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	node_class = ARV_GC_FEATURE_NODE_GET_CLASS (gc_feature_node);
	if (node_class->get_value_as_string != NULL)
		return node_class->get_value_as_string (gc_feature_node, error);

	return NULL;
}

void
arv_gc_feature_node_increment_change_count (ArvGcFeatureNode *gc_feature_node)
{
	g_return_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node));

	gc_feature_node->priv->change_count++;
}

guint64
arv_gc_feature_node_get_change_count (ArvGcFeatureNode *gc_feature_node)
{
	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node), 0);

	return gc_feature_node->priv->change_count;
}

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ArvGcFeatureNode, arv_gc_feature_node, ARV_TYPE_GC_NODE, G_ADD_PRIVATE (ArvGcFeatureNode))

static void
arv_gc_feature_node_init (ArvGcFeatureNode *gc_feature_node)
{
	gc_feature_node->priv = arv_gc_feature_node_get_instance_private (gc_feature_node);

	gc_feature_node->priv->change_count = 0;
}

static void
arv_gc_feature_node_finalize (GObject *object)
{
	ArvGcFeatureNode *node = ARV_GC_FEATURE_NODE(object);

	g_free (node->priv->name);

	G_OBJECT_CLASS (arv_gc_feature_node_parent_class)->finalize (object);
}

static void
arv_gc_feature_node_class_init (ArvGcFeatureNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvDomElementClass *dom_element_class = ARV_DOM_ELEMENT_CLASS (this_class);

	object_class->finalize = arv_gc_feature_node_finalize;
	dom_node_class->can_append_child = arv_gc_feature_node_can_append_child;
	dom_node_class->post_new_child = arv_gc_feature_node_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_feature_node_pre_remove_child;
	dom_element_class->set_attribute = arv_gc_feature_node_set_attribute;
	dom_element_class->get_attribute = arv_gc_feature_node_get_attribute;
}
