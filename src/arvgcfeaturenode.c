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
 * SECTION: arvgcfeaturenode
 * @short_description: Base class for Genicam feature nodes
 *
 * #ArvGcFeatureNode provides a base class for the implementation of the different
 * types of Genicam feature node (Group, Integer, Float, Enumeration...).
 */

#include <arvgcfeaturenodeprivate.h>
#include <arvgcpropertynode.h>
#include <arvgc.h>
#include <arvgcboolean.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcstring.h>
#include <arvgcenumeration.h>
#include <arvgcenums.h>
#include <arvmisc.h>
#include <arvdebugprivate.h>
#include <string.h>

typedef struct {

	char *name;
	ArvGcNameSpace name_space;
        char *comment;

	ArvGcPropertyNode *tooltip;
	ArvGcPropertyNode *description;
	ArvGcPropertyNode *visibility;
	ArvGcPropertyNode *display_name;
	ArvGcPropertyNode *is_implemented;
	ArvGcPropertyNode *is_available;
	ArvGcPropertyNode *is_locked;
	ArvGcPropertyNode *imposed_access_mode;
	ArvGcPropertyNode *streamable;
        ArvGcPropertyNode *is_deprecated;
        ArvGcPropertyNode *alias;
        ArvGcPropertyNode *cast_alias;

	guint64 change_count;

	char *string_buffer;
} ArvGcFeatureNodePrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ArvGcFeatureNode, arv_gc_feature_node, ARV_TYPE_GC_NODE, G_ADD_PRIVATE (ArvGcFeatureNode))

/* ArvDomNode implementation */

static gboolean
arv_gc_feature_node_can_append_child (ArvDomNode *self, ArvDomNode *child)
{
	return ARV_IS_GC_NODE (child);
}

static void
arv_gc_feature_node_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (ARV_GC_FEATURE_NODE (self));

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP:
				priv->tooltip = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION:
				priv->description = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_VISIBILITY:
				priv->visibility = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME:
				priv->display_name = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_IS_AVAILABLE:
				priv->is_available = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_IS_IMPLEMENTED:
				priv->is_implemented = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_IS_LOCKED:
				priv->is_locked = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_IMPOSED_ACCESS_MODE:
				priv->imposed_access_mode = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_STREAMABLE:
				priv->streamable = property_node;       	/* TODO */
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_IS_DEPRECATED:
				priv->is_deprecated = property_node;       	/* TODO */
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_ALIAS:
				priv->alias = property_node;		        /* TODO */
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_CAST_ALIAS:
				priv->alias = property_node;		        /* TODO */
				break;
			default:
				break;
		}
	}
}

static void
arv_gc_feature_node_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (ARV_GC_FEATURE_NODE (self));

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_TOOLTIP:
				priv->tooltip = NULL;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_DESCRIPTION:
				priv->description = NULL;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_VISIBILITY:
				priv->visibility = NULL;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NAME:
				priv->description = NULL;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_IS_AVAILABLE:
				priv->is_available = NULL;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_IS_IMPLEMENTED:
				priv->is_implemented = NULL;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_IS_LOCKED:
				priv->is_locked = NULL;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_IMPOSED_ACCESS_MODE:
				priv->imposed_access_mode =  NULL;
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
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (ARV_GC_FEATURE_NODE (self));

	if (strcmp (name, "Name") == 0) {
		ArvGc *genicam;

		g_free (priv->name);
		priv->name = g_strdup (value);

		genicam = arv_gc_node_get_genicam (ARV_GC_NODE (self));
		/* Kludge around ugly Genicam specification (Really, pre-parsing for EnumEntry Name substitution ?) */
		if (strcmp (arv_dom_node_get_node_name (ARV_DOM_NODE (self)), "EnumEntry") != 0)
			arv_gc_register_feature_node (genicam, ARV_GC_FEATURE_NODE (self));
	} else if (strcmp (name, "NameSpace") == 0) {
		if (g_strcmp0 (value, "Standard") == 0)
			priv->name_space = ARV_GC_NAME_SPACE_STANDARD;
		else
			priv->name_space = ARV_GC_NAME_SPACE_CUSTOM;
	} else if (strcmp (name, "Comment") == 0) {
                g_free (priv->comment);
                priv->comment = g_strdup (value);
	} else
		arv_info_dom ("[GcFeature::set_attribute] Unknown attribute '%s'", name);
}

static const char *
arv_gc_feature_node_get_attribute (ArvDomElement *self, const char *name)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (ARV_GC_FEATURE_NODE (self));

	if (strcmp (name, "Name") == 0)
		return priv->name;
	else if (strcmp (name, "NameSpace") == 0) {
		switch (priv->name_space) {
			case ARV_GC_NAME_SPACE_STANDARD:
				return "Standard";
			default:
				return "Custom";
                }
        } else if (strcmp (name, "Comment") == 0) {
                return priv->comment;
        }

	arv_info_dom ("[GcFeature::set_attribute] Unknown attribute '%s'", name);

	return NULL;
}

/* ArvGcFeatureNode implementation */

static ArvGcFeatureNode *
arv_gc_feature_node_get_linked_feature (ArvGcFeatureNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (node), NULL);

	return ARV_GC_FEATURE_NODE_GET_CLASS (node)->get_linked_feature (node);
}

static ArvGcAccessMode
arv_gc_feature_node_get_access_mode (ArvGcFeatureNode *node)
{
	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (node), ARV_GC_ACCESS_MODE_UNDEFINED);

	return ARV_GC_FEATURE_NODE_GET_CLASS (node)->get_access_mode (node);
}

const char *
arv_gc_feature_node_get_name (ArvGcFeatureNode *node)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (node);

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (node), NULL);

	return priv->name;
}

/**
 * arv_gc_feature_node_get_name_space:
 * @gc_feature_node: a #ArvGcFeatureNode
 *
 * Get feature name space.
 *
 * Returns: Name space value as #ArvGcNameSpace.
 *
 * Since: 0.8.0
 */

ArvGcNameSpace
arv_gc_feature_node_get_name_space (ArvGcFeatureNode *node)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (node);

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (node), ARV_GC_NAME_SPACE_CUSTOM);

	return priv->name_space;
}

const char *
arv_gc_feature_node_get_tooltip (ArvGcFeatureNode *node)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (node);

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (node), NULL);

	if (priv->tooltip == NULL)
		return NULL;

	return arv_gc_property_node_get_string (priv->tooltip, NULL);
}

const char *
arv_gc_feature_node_get_description (ArvGcFeatureNode *node)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (node);

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (node), NULL);

	if (priv->description == NULL)
	       return NULL;

	return arv_gc_property_node_get_string (priv->description, NULL);
}

ArvGcVisibility
arv_gc_feature_node_get_visibility (ArvGcFeatureNode *node)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (node);

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (node), ARV_GC_VISIBILITY_UNDEFINED);

	return arv_gc_property_node_get_visibility (priv->visibility, ARV_GC_VISIBILITY_BEGINNER);
}

const char *
arv_gc_feature_node_get_display_name (ArvGcFeatureNode *node)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (node);

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (node), NULL);

	if (priv->display_name == NULL)
	       return NULL;

	return arv_gc_property_node_get_string (priv->display_name, NULL);
}

gboolean
arv_gc_feature_node_is_implemented (ArvGcFeatureNode *gc_feature_node, GError **error)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (gc_feature_node);
	gboolean value;
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	if (priv->is_implemented == NULL)
		return TRUE;


	value = arv_gc_property_node_get_int64 (priv->is_implemented, &local_error) != 0;

	if (local_error != NULL) {
                g_propagate_prefixed_error (error, local_error, "[%s] ",
                                            arv_gc_feature_node_get_name (gc_feature_node));
                return FALSE;
        }

	return value;
}

gboolean
arv_gc_feature_node_is_available (ArvGcFeatureNode *gc_feature_node, GError **error)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (gc_feature_node);
	gboolean value;
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	if (priv->is_available == NULL)
		return TRUE;

	value = arv_gc_property_node_get_int64 (priv->is_available, &local_error) != 0;

	if (local_error != NULL) {
                g_propagate_prefixed_error (error, local_error, "[%s] ",
                                            arv_gc_feature_node_get_name (gc_feature_node));
		return FALSE;
	}

	return value;
}

gboolean
arv_gc_feature_node_is_locked (ArvGcFeatureNode *gc_feature_node, GError **error)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (gc_feature_node);
	gboolean locked;
	GError *local_error = NULL;

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node), FALSE);

	if (priv->is_locked == NULL)
		return FALSE;

	locked = arv_gc_property_node_get_int64 (priv->is_locked, &local_error) != 0;

	if (local_error != NULL) {
                g_propagate_prefixed_error (error, local_error, "[%s] ",
                                            arv_gc_feature_node_get_name (gc_feature_node));
		return FALSE;
	}

	return locked;
}

/**
 * arv_gc_feature_node_get_imposed_access_mode:
 * @gc_feature_node: a #ArvGcFeatureNode
 *
 * Gets feature node imposed access mode property.
 *
 * <warning><para>Note that this function will not give the actual access mode. Please use
 * #arv_gc_feature_node_get_actual_access_mode to get an access mode combined from imposed access
 * mode and underlying register access mode properties.</para></warning>
 *
 * Returns: Access mode as #ArvGcAccessMode
 *
 * Since: 0.8.0
 */

ArvGcAccessMode
arv_gc_feature_node_get_imposed_access_mode (ArvGcFeatureNode *gc_feature_node)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (gc_feature_node);

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node), ARV_GC_ACCESS_MODE_UNDEFINED);

	if (priv->imposed_access_mode == NULL)
		return ARV_GC_ACCESS_MODE_RW;
	return arv_gc_property_node_get_access_mode (priv->imposed_access_mode, ARV_GC_ACCESS_MODE_RW);
}

/**
 * arv_gc_feature_node_get_actual_access_mode:
 * @gc_feature_node: a #ArvGcFeatureNode
 *
 * Gets feature node allowed access mode. This is a combination of Genicam ImposedAccessMode and
 * AccessMode properties of underlying features and registers.
 *
 * Returns: Access mode as #ArvGcAccessMode
 *
 * Since: 0.8.0
 */

ArvGcAccessMode
arv_gc_feature_node_get_actual_access_mode (ArvGcFeatureNode *gc_feature_node)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (gc_feature_node);
	ArvGcAccessMode access_mode = ARV_GC_ACCESS_MODE_RO;
	ArvGcAccessMode imposed_access_mode = ARV_GC_ACCESS_MODE_RW;

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (gc_feature_node), ARV_GC_ACCESS_MODE_UNDEFINED);

	if (ARV_IS_GC_PROPERTY_NODE (priv->imposed_access_mode))
		imposed_access_mode = arv_gc_property_node_get_access_mode (priv->imposed_access_mode, imposed_access_mode);

	access_mode = arv_gc_feature_node_get_access_mode (gc_feature_node);
	if (access_mode == ARV_GC_ACCESS_MODE_RO && imposed_access_mode == ARV_GC_ACCESS_MODE_RW) {
		imposed_access_mode = ARV_GC_ACCESS_MODE_RO;
	} else if (access_mode == ARV_GC_ACCESS_MODE_WO && imposed_access_mode == ARV_GC_ACCESS_MODE_RW) {
		imposed_access_mode = ARV_GC_ACCESS_MODE_WO;
	}
	return imposed_access_mode;
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
arv_gc_feature_node_set_value_from_string (ArvGcFeatureNode *self, const char *string, GError **error)
{
        GError *local_error = NULL;

	g_return_if_fail (ARV_IS_GC_FEATURE_NODE (self));
	g_return_if_fail (string != NULL);

	if (ARV_IS_GC_ENUMERATION (self)) {
		arv_gc_enumeration_set_string_value (ARV_GC_ENUMERATION (self), string, &local_error);
	} else if (ARV_IS_GC_INTEGER (self)) {
                gint64 value;
                char *end = NULL;

                value = g_ascii_strtoll (string, &end, 0);

                if (end == NULL || end[0] != '\0') {
                        g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_INVALID_SYNTAX,
                                     "Invalid string for an integer feature (%s)", string);
                        return;
                }

		arv_gc_integer_set_value (ARV_GC_INTEGER (self), value, &local_error);
	} else if (ARV_IS_GC_FLOAT (self)) {
                double value;
                char *end = NULL;

                value = g_ascii_strtod (string, &end);

                if (end == NULL || end[0] != '\0') {
                        g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_INVALID_SYNTAX,
                                     "Invalid string for a float feature (%s)", string);
                        return;
                }

		arv_gc_float_set_value (ARV_GC_FLOAT (self), value, &local_error);
	} else if (ARV_IS_GC_STRING (self)) {
		arv_gc_string_set_value (ARV_GC_STRING (self), string, &local_error);
	} else if (ARV_IS_GC_BOOLEAN (self)) {
                gboolean value;

                if (g_ascii_strcasecmp (string, "false") == 0 || g_ascii_strcasecmp (string, "0") == 0)
                        value = FALSE;
                else if (g_ascii_strcasecmp (string, "true") == 0 || g_ascii_strcasecmp (string, "1") == 0)
                        value = TRUE;
                else {
                        g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_INVALID_SYNTAX,
                                     "Invalid string for a boolean feature (%s)", string);
                        return;
                }

		arv_gc_boolean_set_value (ARV_GC_BOOLEAN (self), value, &local_error);
	} else {
		g_set_error (&local_error, ARV_GC_ERROR, ARV_GC_ERROR_SET_FROM_STRING_UNDEFINED,
			     "Don't know how to set value from string");
	}

	if (local_error != NULL)
                g_propagate_error (error, local_error);
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
arv_gc_feature_node_get_value_as_string (ArvGcFeatureNode *self, GError **error)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (self);
        GError *local_error = NULL;
        const char *value = NULL;

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (self), NULL);

	if (ARV_IS_GC_ENUMERATION (self)) {
                value = arv_gc_enumeration_get_string_value (ARV_GC_ENUMERATION (self), &local_error);
	} else if (ARV_IS_GC_INTEGER (self)) {
		g_free (priv->string_buffer);
		priv->string_buffer = g_strdup_printf ("%" G_GINT64_FORMAT,
                                                       arv_gc_integer_get_value (ARV_GC_INTEGER (self), &local_error));
		value = priv->string_buffer;
	} else if (ARV_IS_GC_FLOAT (self)) {
		g_free (priv->string_buffer);
		priv->string_buffer = g_strdup_printf ("%g", arv_gc_float_get_value (ARV_GC_FLOAT (self), &local_error));
		value = priv->string_buffer;
	} else if (ARV_IS_GC_STRING (self)) {
		value =  arv_gc_string_get_value (ARV_GC_STRING (self), &local_error);
	} else if (ARV_IS_GC_BOOLEAN (self)) {
		value = arv_gc_boolean_get_value (ARV_GC_BOOLEAN (self), &local_error) ? "true" : "false";
	} else {
                g_set_error (&local_error, ARV_GC_ERROR, ARV_GC_ERROR_SET_FROM_STRING_UNDEFINED,
                             "Don't know how to set value from string");
        }

        if (local_error != NULL)
                g_propagate_error (error, local_error);

	return value;
}

void
arv_gc_feature_node_increment_change_count (ArvGcFeatureNode *self)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (self);

	g_return_if_fail (ARV_IS_GC_FEATURE_NODE (self));

	priv->change_count++;
}

guint64
arv_gc_feature_node_get_change_count (ArvGcFeatureNode *self)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (self);

	g_return_val_if_fail (ARV_IS_GC_FEATURE_NODE (self), 0);

	return priv->change_count;
}

static void
arv_gc_feature_node_init (ArvGcFeatureNode *self)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (self);

	priv->change_count = 0;
}

static void
arv_gc_feature_node_finalize (GObject *object)
{
	ArvGcFeatureNodePrivate *priv = arv_gc_feature_node_get_instance_private (ARV_GC_FEATURE_NODE(object));

	g_clear_pointer (&priv->name, g_free);
        g_clear_pointer (&priv->comment, g_free);
	g_clear_pointer (&priv->string_buffer, g_free);

	G_OBJECT_CLASS (arv_gc_feature_node_parent_class)->finalize (object);
}

static ArvGcFeatureNode *
_get_linked_feature (ArvGcFeatureNode *gc_feature_node)
{
	return NULL;
}

static ArvGcAccessMode
_get_access_mode (ArvGcFeatureNode *gc_feature_node)
{
	ArvGcFeatureNode *pointed_node = arv_gc_feature_node_get_linked_feature (gc_feature_node);
	if (pointed_node)
		return arv_gc_feature_node_get_access_mode (pointed_node);

	return ARV_GC_FEATURE_NODE_GET_CLASS (gc_feature_node)->default_access_mode;
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
	this_class->get_linked_feature = _get_linked_feature;
	this_class->get_access_mode = _get_access_mode;
        this_class->default_access_mode = ARV_GC_ACCESS_MODE_RO;
}
