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
#include <arvgc.h>
#include <arvmisc.h>

static void arv_gc_swiss_knife_node_init (ArvGcSwissKnifeNode *self);
static void arv_gc_swiss_knife_node_float_interface_init (ArvGcFloatInterface *interface);

G_DEFINE_TYPE_WITH_CODE (ArvGcSwissKnifeNode, arv_gc_swiss_knife_node, ARV_TYPE_GC_SWISS_KNIFE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_FLOAT, arv_gc_swiss_knife_node_float_interface_init))

static const char *
arv_gc_swiss_knife_node_get_node_name (ArvDomNode *node)
{
	return "SwissKnife";
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
arv_gc_swiss_knife_node_get_float_unit (ArvGcFloat *self, GError **error)
{
	return arv_gc_swiss_knife_get_unit (ARV_GC_SWISS_KNIFE (self), error);
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
}
