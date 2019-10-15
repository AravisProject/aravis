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

#include <arvgcintregnode.h>
#include <arvgcregisternodeprivate.h>
#include <arvgcinteger.h>
#include <arvgcselector.h>
#include <arvgcregister.h>

typedef struct {
	ArvGcPropertyNode *sign;
	ArvGcPropertyNode *endianess;
	ArvGcPropertyNode *unit;

	GSList *selecteds;
	GSList *selected_features;
} ArvGcIntRegNodePrivate;

static void arv_gc_int_reg_node_init (ArvGcIntRegNode *self);
static void arv_gc_int_reg_node_integer_interface_init (ArvGcIntegerInterface *interface);
static void arv_gc_int_reg_node_selector_interface_init (ArvGcSelectorInterface *interface);

G_DEFINE_TYPE_WITH_CODE (ArvGcIntRegNode, arv_gc_int_reg_node, ARV_TYPE_GC_REGISTER_NODE,
			 G_ADD_PRIVATE (ArvGcIntRegNode)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_int_reg_node_integer_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_SELECTOR, arv_gc_int_reg_node_selector_interface_init))

static const char *
arv_gc_int_reg_node_get_node_name (ArvDomNode *node)
{
	return "IntReg";
}

static void
arv_gc_int_reg_node_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcIntRegNodePrivate *priv = arv_gc_int_reg_node_get_instance_private (ARV_GC_INT_REG_NODE (self));

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_SIGN:
				priv->sign = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_ENDIANESS:
				priv->endianess = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_UNIT:
				priv->unit = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_SELECTED:
				priv->selecteds = g_slist_prepend (priv->selecteds, property_node);
				break;

				/* TODO Representation */

			default:
				ARV_DOM_NODE_CLASS (arv_gc_int_reg_node_parent_class)->post_new_child (self, child);
				break;
		}
	} else {
		ARV_DOM_NODE_CLASS (arv_gc_int_reg_node_parent_class)->post_new_child (self, child);
	}
}

static gint64
arv_gc_int_reg_node_get_integer_value (ArvGcInteger *self, GError **error)
{
	ArvGcIntRegNodePrivate *priv = arv_gc_int_reg_node_get_instance_private (ARV_GC_INT_REG_NODE (self));

	return arv_gc_register_node_get_masked_integer_value (ARV_GC_REGISTER_NODE (self),
							      0, 31,
							      arv_gc_property_node_get_sign (priv->sign, ARV_GC_SIGNEDNESS_UNSIGNED),
							      arv_gc_property_node_get_endianess (priv->endianess, G_LITTLE_ENDIAN),
							      ARV_GC_CACHABLE_UNDEFINED,
							      FALSE, error);
}

static void
arv_gc_int_reg_node_set_integer_value (ArvGcInteger *self, gint64 value, GError **error)
{
	ArvGcIntRegNodePrivate *priv = arv_gc_int_reg_node_get_instance_private (ARV_GC_INT_REG_NODE (self));

	arv_gc_register_node_set_masked_integer_value (ARV_GC_REGISTER_NODE (self),
						       0, 31,
						       arv_gc_property_node_get_sign (priv->sign, ARV_GC_SIGNEDNESS_UNSIGNED),
						       arv_gc_property_node_get_endianess (priv->endianess, G_LITTLE_ENDIAN),
						       ARV_GC_CACHABLE_UNDEFINED,
						       FALSE, value, error);
}

static gint64
arv_gc_int_reg_node_get_min (ArvGcInteger *self, GError **error)
{
#if 0
	return G_MININT64;
#else
	ArvGcIntRegNodePrivate *priv = arv_gc_int_reg_node_get_instance_private (ARV_GC_INT_REG_NODE (self));
	GError *local_error = NULL;
	gint64 length;
	ArvGcSignedness signedness;
	gint64 minimum;

	signedness = arv_gc_property_node_get_sign (priv->sign, ARV_GC_SIGNEDNESS_UNSIGNED);

	length = arv_gc_register_get_length (ARV_GC_REGISTER (self), &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return signedness == ARV_GC_SIGNEDNESS_UNSIGNED ? 0 : G_MININT64;
	}

	if (length > 8)
		minimum = signedness == ARV_GC_SIGNEDNESS_UNSIGNED ? 0 : G_MININT64;
	else
		minimum = signedness == ARV_GC_SIGNEDNESS_UNSIGNED ? 0 :- ((gint64) 1 << (8 * length - 1));

/*        printf ("minimum = %" G_GINT64_MODIFIER "x %" G_GINT64_FORMAT "\n", minimum, length);*/

	return minimum;
#endif
}

static gint64
arv_gc_int_reg_node_get_max (ArvGcInteger *self, GError **error)
{
#if 0
	return G_MAXINT64;
#else
	ArvGcIntRegNodePrivate *priv = arv_gc_int_reg_node_get_instance_private (ARV_GC_INT_REG_NODE (self));
	GError *local_error = NULL;
	gint64 length;
	ArvGcSignedness signedness;
	gint64 maximum;

	signedness = arv_gc_property_node_get_sign (priv->sign, ARV_GC_SIGNEDNESS_UNSIGNED);

	length = arv_gc_register_get_length (ARV_GC_REGISTER (self), &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_MAXINT64;
	}

	if (length >= 8)
		maximum = G_MAXINT64;
	else
		maximum = signedness == ARV_GC_SIGNEDNESS_UNSIGNED ?
			((gint64) 1 << (8 * length)) - 1 :
			((gint64) 1 << (8 * length - 1)) - 1;

/*        printf ("maximum = %" G_GINT64_MODIFIER "x %" G_GINT64_FORMAT "\n", maximum, length);*/

	return maximum;
#endif
}

static gint64
arv_gc_int_reg_node_get_inc (ArvGcInteger *self, GError **error)
{
	return 1;
}

static const char *
arv_gc_int_reg_node_get_unit (ArvGcInteger *self, GError **error)
{
	ArvGcIntRegNodePrivate *priv = arv_gc_int_reg_node_get_instance_private (ARV_GC_INT_REG_NODE (self));

	if (priv->unit == NULL)
		return NULL;

	return arv_gc_property_node_get_string (priv->unit, error);
}

/**
 * arv_gc_int_reg_node_new:
 *
 * Returns: a new IntReg node
 *
 * Since:0.8.0
 */

ArvGcNode *
arv_gc_int_reg_node_new	(void)
{
	return g_object_new (ARV_TYPE_GC_INT_REG_NODE, NULL);
}

static void
arv_gc_int_reg_node_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_int_reg_node_get_integer_value;
	interface->set_value = arv_gc_int_reg_node_set_integer_value;
	interface->get_min = arv_gc_int_reg_node_get_min;
	interface->get_max = arv_gc_int_reg_node_get_max;
	interface->get_inc = arv_gc_int_reg_node_get_inc;
	interface->get_unit = arv_gc_int_reg_node_get_unit;
}

const GSList *
arv_gc_int_reg_node_get_selected_features (ArvGcSelector *self)
{
	ArvGcIntRegNodePrivate *priv = arv_gc_int_reg_node_get_instance_private (ARV_GC_INT_REG_NODE (self));
	GSList *iter;

	g_clear_pointer (&priv->selected_features, g_slist_free);
	for (iter = priv->selecteds; iter != NULL; iter = iter->next) {
		ArvGcFeatureNode *feature_node = ARV_GC_FEATURE_NODE (arv_gc_property_node_get_linked_node (iter->data));
		if (ARV_IS_GC_FEATURE_NODE (feature_node))
			priv->selected_features = g_slist_prepend (priv->selected_features, feature_node);
	}

	return priv->selected_features;
}

static void
arv_gc_int_reg_node_selector_interface_init (ArvGcSelectorInterface *interface)
{
	interface->get_selected_features = arv_gc_int_reg_node_get_selected_features;
}

static void
arv_gc_int_reg_node_init (ArvGcIntRegNode *self)
{
}

static void
arv_gc_int_reg_node_finalize (GObject *self)
{
	ArvGcIntRegNodePrivate *priv = arv_gc_int_reg_node_get_instance_private (ARV_GC_INT_REG_NODE (self));

	g_clear_pointer (&priv->selecteds, g_slist_free);
	g_slist_free (priv->selecteds);
	g_slist_free (priv->selected_features);

	G_OBJECT_CLASS (arv_gc_int_reg_node_parent_class)->finalize (self);
}

static void
arv_gc_int_reg_node_class_init (ArvGcIntRegNodeClass *this_class)
{
	ArvGcRegisterNodeClass *gc_register_node_class = ARV_GC_REGISTER_NODE_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);

	object_class->finalize = arv_gc_int_reg_node_finalize;
	dom_node_class->get_node_name = arv_gc_int_reg_node_get_node_name;
	dom_node_class->post_new_child = arv_gc_int_reg_node_post_new_child;
	gc_register_node_class->default_cachable = ARV_GC_CACHABLE_WRITE_THROUGH;
}
