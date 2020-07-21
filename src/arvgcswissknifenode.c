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

#include <arvgcswissknifenode.h>
#include <arvgcswissknifeprivate.h>
#include <arvgcfloat.h>
#include <arvgcregister.h>
#include <arvgcdefaultsprivate.h>
#include <arvgc.h>
#include <arvmisc.h>

typedef struct {
	ArvGcPropertyNode *display_notation;
	ArvGcPropertyNode *display_precision;
} ArvGcSwissKnifeNodePrivate;

static void arv_gc_swiss_knife_node_init (ArvGcSwissKnifeNode *self);
static void arv_gc_swiss_knife_node_float_interface_init (ArvGcFloatInterface *interface);

G_DEFINE_TYPE_WITH_CODE (ArvGcSwissKnifeNode, arv_gc_swiss_knife_node, ARV_TYPE_GC_SWISS_KNIFE,
			 G_ADD_PRIVATE (ArvGcSwissKnifeNode)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_FLOAT, arv_gc_swiss_knife_node_float_interface_init))

static const char *
arv_gc_swiss_knife_node_get_node_name (ArvDomNode *node)
{
	return "SwissKnife";
}

static void
arv_gc_swiss_knife_node_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcSwissKnifeNodePrivate *priv = arv_gc_swiss_knife_node_get_instance_private (ARV_GC_SWISS_KNIFE_NODE (self));

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_NOTATION:
				priv->display_notation = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_DISPLAY_PRECISION:
				priv->display_precision = property_node;
				break;
			default:
				ARV_DOM_NODE_CLASS (arv_gc_swiss_knife_node_parent_class)->post_new_child (self, child);
				break;
		}
	} else {
		ARV_DOM_NODE_CLASS (arv_gc_swiss_knife_node_parent_class)->post_new_child (self, child);
	}
}

static gdouble
arv_gc_swiss_knife_node_get_float_value (ArvGcFloat *self, GError **error)
{
	return arv_gc_swiss_knife_get_float_value (ARV_GC_SWISS_KNIFE (self), error);
}

static void
arv_gc_swiss_knife_node_set_float_value (ArvGcFloat *self, gdouble value, GError **error)
{
	g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_READ_ONLY, "SwissKnife is read only");
}

static ArvGcRepresentation
arv_gc_swiss_knife_node_get_float_representation (ArvGcFloat *self, GError **error)
{
	return arv_gc_swiss_knife_get_representation (ARV_GC_SWISS_KNIFE (self), error);
}

static const char *
arv_gc_swiss_knife_node_get_float_unit (ArvGcFloat *self)
{
	return arv_gc_swiss_knife_get_unit (ARV_GC_SWISS_KNIFE (self));
}

static ArvGcDisplayNotation
arv_gc_swiss_knife_node_get_display_notation (ArvGcFloat *self)
{
	ArvGcSwissKnifeNodePrivate *priv = arv_gc_swiss_knife_node_get_instance_private (ARV_GC_SWISS_KNIFE_NODE (self));

	if (priv->display_notation == NULL)
		return ARV_GC_DISPLAY_NOTATION_DEFAULT;

	return arv_gc_property_node_get_display_notation (ARV_GC_PROPERTY_NODE (priv->display_notation), ARV_GC_DISPLAY_NOTATION_DEFAULT);
}

static gint64
arv_gc_swiss_knife_node_get_display_precision (ArvGcFloat *self)
{
	ArvGcSwissKnifeNodePrivate *priv = arv_gc_swiss_knife_node_get_instance_private (ARV_GC_SWISS_KNIFE_NODE (self));

	if (priv->display_precision == NULL)
		return ARV_GC_DISPLAY_PRECISION_DEFAULT;

	return arv_gc_property_node_get_display_precision (ARV_GC_PROPERTY_NODE (priv->display_precision), ARV_GC_DISPLAY_PRECISION_DEFAULT);
}

ArvGcNode *
arv_gc_swiss_knife_node_new	(void)
{
	return g_object_new (ARV_TYPE_GC_SWISS_KNIFE_NODE, NULL);
}

static void
arv_gc_swiss_knife_node_float_interface_init (ArvGcFloatInterface *interface)
{
	interface->get_value = arv_gc_swiss_knife_node_get_float_value;
	interface->set_value = arv_gc_swiss_knife_node_set_float_value;
	interface->get_representation = arv_gc_swiss_knife_node_get_float_representation;
	interface->get_unit = arv_gc_swiss_knife_node_get_float_unit;
	interface->get_display_notation = arv_gc_swiss_knife_node_get_display_notation;
	interface->get_display_precision = arv_gc_swiss_knife_node_get_display_precision;
}

static void
arv_gc_swiss_knife_node_init (ArvGcSwissKnifeNode *self)
{
}

static void
arv_gc_swiss_knife_node_finalize (GObject *self)
{
	G_OBJECT_CLASS (arv_gc_swiss_knife_node_parent_class)->finalize (self);
}

static void
arv_gc_swiss_knife_node_class_init (ArvGcSwissKnifeNodeClass *this_class)
{
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);

	object_class->finalize = arv_gc_swiss_knife_node_finalize;
	dom_node_class->get_node_name = arv_gc_swiss_knife_node_get_node_name;
	dom_node_class->post_new_child = arv_gc_swiss_knife_node_post_new_child;
}
