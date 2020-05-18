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

#include <arvgcfloatregnode.h>
#include <arvgcpropertynode.h>
#include <arvgcfloat.h>
#include <arvgcregister.h>
#include <arvgc.h>
#include <arvmisc.h>

typedef struct {
	ArvGcPropertyNode *endianess;
	ArvGcPropertyNode *unit;

	GSList *selecteds;
} ArvGcFloatRegNodePrivate;

static void arv_gc_float_reg_node_init (ArvGcFloatRegNode *self);
static void arv_gc_float_reg_node_float_interface_init (ArvGcFloatInterface *interface);

G_DEFINE_TYPE_WITH_CODE (ArvGcFloatRegNode, arv_gc_float_reg_node, ARV_TYPE_GC_REGISTER_NODE,
			 G_ADD_PRIVATE (ArvGcFloatRegNode)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_FLOAT, arv_gc_float_reg_node_float_interface_init))

static const char *
arv_gc_float_reg_node_get_node_name (ArvDomNode *node)
{
	return "FloatReg";
}

static void
arv_gc_float_reg_node_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcFloatRegNodePrivate *priv = arv_gc_float_reg_node_get_instance_private (ARV_GC_FLOAT_REG_NODE (self));

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
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
				/* TODO DisplayNotation */
				/* TODO DisplayPrecision */

			default:
				ARV_DOM_NODE_CLASS (arv_gc_float_reg_node_parent_class)->post_new_child (self, child);
				break;
		}
	} else {
		ARV_DOM_NODE_CLASS (arv_gc_float_reg_node_parent_class)->post_new_child (self, child);
	}
}

static gdouble
arv_gc_float_reg_node_get_float_value (ArvGcFloat *self, GError **error)
{
	ArvGcFloatRegNodePrivate *priv = arv_gc_float_reg_node_get_instance_private (ARV_GC_FLOAT_REG_NODE (self));
	GError *local_error = NULL;
	guint endianess;
	gint64 length;
	double v_double = 0.0;

	endianess = arv_gc_property_node_get_endianess (priv->endianess, G_LITTLE_ENDIAN);
	length = arv_gc_register_get_length (ARV_GC_REGISTER (self), &local_error);
	if (local_error == NULL) {
		char *buffer;

		buffer = g_malloc (length);
		arv_gc_register_get (ARV_GC_REGISTER (self), buffer, length, &local_error);
		if (local_error == NULL) {
			if (length == 4) {
				float v_float = 0.0;

				arv_copy_memory_with_endianess (&v_float, sizeof (v_float), G_BYTE_ORDER,
								buffer, length, endianess);

				v_double = v_float;
			} else if (length == 8) {
				arv_copy_memory_with_endianess (&v_double, sizeof (v_double), G_BYTE_ORDER,
								buffer, length, endianess);
			} else {
				g_set_error (&local_error, ARV_GC_ERROR, ARV_GC_ERROR_INVALID_LENGTH,
					     "Invalid register length for FloatReg node");
			}
		}
		g_free (buffer);
	}

	if (local_error != NULL)
		g_propagate_error (error, local_error);

	return v_double;
}

static void
arv_gc_float_reg_node_set_float_value (ArvGcFloat *self, gdouble value, GError **error)
{
	ArvGcFloatRegNodePrivate *priv = arv_gc_float_reg_node_get_instance_private (ARV_GC_FLOAT_REG_NODE (self));
	GError *local_error = NULL;
	guint endianess;
	gint64 length;

	endianess = arv_gc_property_node_get_endianess (priv->endianess, G_LITTLE_ENDIAN);
	length = arv_gc_register_get_length (ARV_GC_REGISTER (self), &local_error);
	if (local_error == NULL) {
		char *buffer;

		buffer = g_malloc (length);
		if (local_error == NULL) {
			if (length == 4) {
				float v_float = value;

				arv_copy_memory_with_endianess (buffer, length, endianess, &v_float, sizeof (v_float), G_BYTE_ORDER);
			} else if (length == 8) {
				arv_copy_memory_with_endianess (buffer, length, endianess, &value, sizeof (value), G_BYTE_ORDER);
			} else {
				g_set_error (&local_error, ARV_GC_ERROR, ARV_GC_ERROR_INVALID_LENGTH,
					     "Invalid register length for FloatReg node");
			}
		}

		if (local_error == NULL)
			arv_gc_register_set (ARV_GC_REGISTER (self), buffer, length, &local_error);

		g_free (buffer);
	}

	if (local_error != NULL)
		g_propagate_error (error, local_error);
}

static double
arv_gc_float_reg_node_get_min (ArvGcFloat *self, GError **error)
{
	GError *local_error = NULL;
	gint64 length;

	length = arv_gc_register_get_length (ARV_GC_REGISTER (self), &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return -G_MAXDOUBLE;
	}

	if (length == 4)
		return -G_MAXFLOAT;
	else if (length == 8)
		return -G_MAXDOUBLE;

	g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_INVALID_LENGTH,
		     "Invalid register length for FloatReg node");

	return -G_MAXDOUBLE;
}

static double
arv_gc_float_reg_node_get_max (ArvGcFloat *self, GError **error)
{
	GError *local_error = NULL;
	gint64 length;

	length = arv_gc_register_get_length (ARV_GC_REGISTER (self), &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_MAXDOUBLE;
	}

	if (length == 4)
		return G_MAXFLOAT;
	else if (length == 8)
		return G_MAXDOUBLE;

	g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_INVALID_LENGTH,
		     "Invalid register length for FloatReg node");

	return G_MAXDOUBLE;
}

static const char *
arv_gc_float_reg_node_get_unit (ArvGcFloat *self, GError **error)
{
	ArvGcFloatRegNodePrivate *priv = arv_gc_float_reg_node_get_instance_private (ARV_GC_FLOAT_REG_NODE (self));

	if (priv->unit == NULL)
		return NULL;

	return arv_gc_property_node_get_string (priv->unit, error);
}

/**
 * arv_gc_float_reg_node_new:
 *
 * Returns: a new FloatReg node
 *
 * Since:0.8.0
 */

ArvGcNode *
arv_gc_float_reg_node_new	(void)
{
	return g_object_new (ARV_TYPE_GC_FLOAT_REG_NODE, NULL);
}

static void
arv_gc_float_reg_node_float_interface_init (ArvGcFloatInterface *interface)
{
	interface->get_value = arv_gc_float_reg_node_get_float_value;
	interface->set_value = arv_gc_float_reg_node_set_float_value;
	interface->get_min = arv_gc_float_reg_node_get_min;
	interface->get_max = arv_gc_float_reg_node_get_max;
	interface->get_unit = arv_gc_float_reg_node_get_unit;
}

static void
arv_gc_float_reg_node_init (ArvGcFloatRegNode *self)
{
}

static void
arv_gc_float_reg_node_finalize (GObject *self)
{
	ArvGcFloatRegNodePrivate *priv = arv_gc_float_reg_node_get_instance_private (ARV_GC_FLOAT_REG_NODE (self));

	g_clear_pointer (&priv->selecteds, g_slist_free);

	G_OBJECT_CLASS (arv_gc_float_reg_node_parent_class)->finalize (self);
}

static void
arv_gc_float_reg_node_class_init (ArvGcFloatRegNodeClass *this_class)
{
	ArvGcRegisterNodeClass *gc_register_node_class = ARV_GC_REGISTER_NODE_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);

	object_class->finalize = arv_gc_float_reg_node_finalize;
	dom_node_class->get_node_name = arv_gc_float_reg_node_get_node_name;
	dom_node_class->post_new_child = arv_gc_float_reg_node_post_new_child;
	gc_register_node_class->default_cachable = ARV_GC_CACHABLE_WRITE_AROUND;
}
