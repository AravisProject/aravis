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

#include <arvgcintswissknifenode.h>
#include <arvgcswissknifeprivate.h>
#include <arvgcinteger.h>
#include <arvgcregister.h>
#include <arvgc.h>
#include <arvmisc.h>

static void arv_gc_int_swiss_knife_node_init (ArvGcIntSwissKnifeNode *self);
static void arv_gc_int_swiss_knife_node_integer_interface_init (ArvGcIntegerInterface *interface);

G_DEFINE_TYPE_WITH_CODE (ArvGcIntSwissKnifeNode, arv_gc_int_swiss_knife_node, ARV_TYPE_GC_SWISS_KNIFE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_int_swiss_knife_node_integer_interface_init))

static const char *
arv_gc_int_swiss_knife_node_get_node_name (ArvDomNode *node)
{
	return "IntSwissKnife";
}

static gint64
arv_gc_int_swiss_knife_node_get_integer_value (ArvGcInteger *self, GError **error)
{
	return arv_gc_swiss_knife_get_integer_value (ARV_GC_SWISS_KNIFE (self), error);
}

static void
arv_gc_int_swiss_knife_node_set_integer_value (ArvGcInteger *self, gint64 value, GError **error)
{
	g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_READ_ONLY, "IntSwissKnife is read only");
}

static ArvGcRepresentation
arv_gc_swiss_knife_node_get_integer_representation (ArvGcInteger *self, GError **error)
{
	return arv_gc_swiss_knife_get_representation (ARV_GC_SWISS_KNIFE (self), error);
}

static const char *
arv_gc_swiss_knife_node_get_integer_unit (ArvGcInteger *self, GError **error)
{
	return arv_gc_swiss_knife_get_unit (ARV_GC_SWISS_KNIFE (self), error);
}

ArvGcNode *
arv_gc_int_swiss_knife_node_new	(void)
{
	return g_object_new (ARV_TYPE_GC_INT_SWISS_KNIFE_NODE, NULL);
}

static void
arv_gc_int_swiss_knife_node_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_int_swiss_knife_node_get_integer_value;
	interface->set_value = arv_gc_int_swiss_knife_node_set_integer_value;
	interface->get_representation = arv_gc_swiss_knife_node_get_integer_representation;
	interface->get_unit = arv_gc_swiss_knife_node_get_integer_unit;
}

static void
arv_gc_int_swiss_knife_node_init (ArvGcIntSwissKnifeNode *self)
{
}

static void
arv_gc_int_swiss_knife_node_finalize (GObject *self)
{
	G_OBJECT_CLASS (arv_gc_int_swiss_knife_node_parent_class)->finalize (self);
}

static void
arv_gc_int_swiss_knife_node_class_init (ArvGcIntSwissKnifeNodeClass *this_class)
{
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);

	object_class->finalize = arv_gc_int_swiss_knife_node_finalize;
	dom_node_class->get_node_name = arv_gc_int_swiss_knife_node_get_node_name;
}
