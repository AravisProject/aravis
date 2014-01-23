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
 * SECTION: arvgcstructentrynode
 * @short_description: Class for StructEntry nodes
 */

#include <arvgcstructentrynode.h>
#include <arvgcregisternode.h>
#include <arvgcregister.h>
#include <arvgcinteger.h>
#include <arvgcport.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <arvdebug.h>
#include <stdlib.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_gc_struct_entry_node_get_node_name (ArvDomNode *node)
{
	return "StructEntry";
}

static void
arv_gc_struct_entry_node_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcStructEntryNode *node = ARV_GC_STRUCT_ENTRY_NODE (self);

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_SIGN:
				/* TODO */
				node->sign = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_LSB:
				node->lsb = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_MSB:
				node->msb = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_BIT:
				node->msb = property_node;
				node->lsb = property_node;
				break;
			default:
				ARV_DOM_NODE_CLASS (parent_class)->post_new_child (self, child);
				break;
		}
	}
}

static void
arv_gc_struct_entry_node_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcFeatureNode implementation */

static GType
arv_gc_struct_entry_node_get_value_type (ArvGcFeatureNode *node)
{
	return G_TYPE_INT64;
}

static void
arv_gc_struct_entry_node_set_value_from_string (ArvGcFeatureNode *node, const char *string, GError **error)
{
	arv_gc_integer_set_value (ARV_GC_INTEGER (node), g_ascii_strtoll (string, NULL, 0), error);
}

static const char *
arv_gc_struct_entry_node_get_value_as_string (ArvGcFeatureNode *node, GError **error)
{
	ArvGcStructEntryNode *gc_struct_entry_node = ARV_GC_STRUCT_ENTRY_NODE (node);
	GError *local_error = NULL;
	gint64 value;

	value = arv_gc_integer_get_value (ARV_GC_INTEGER (node), &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return NULL;
	}

	g_snprintf (gc_struct_entry_node->v_string, G_ASCII_DTOSTR_BUF_SIZE,
		    "0x%08" G_GINT64_MODIFIER "x", value);

	return gc_struct_entry_node->v_string;
}

/* ArvGcStructEntryNode implementation */

/* Set default to read only 32 bits little endian integer struct_entry_node */

static gint64
_get_lsb (ArvGcStructEntryNode *gc_struct_entry_node, GError **error)
{
	if (gc_struct_entry_node->lsb == NULL)
		return 0;

	return arv_gc_property_node_get_int64 (gc_struct_entry_node->lsb, error);
}

static gint64
_get_msb (ArvGcStructEntryNode *gc_struct_entry_node, GError **error)
{
	if (gc_struct_entry_node->msb == NULL)
		return 31;

	return arv_gc_property_node_get_int64 (gc_struct_entry_node->msb, error);
}

/**
 * arv_gc_struct_entry_node_new:
 *
 * Returns: (transfer full): a newly created #ArvGcStructEntryNode.
 */

ArvGcNode *
arv_gc_struct_entry_node_new (void)
{
	ArvGcStructEntryNode *gc_struct_entry_node;

	gc_struct_entry_node = g_object_new (ARV_TYPE_GC_STRUCT_ENTRY_NODE, NULL);

	return ARV_GC_NODE (gc_struct_entry_node);
}

static void
arv_gc_struct_entry_node_init (ArvGcStructEntryNode *gc_struct_entry_node)
{
}

static void
arv_gc_struct_entry_node_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_gc_struct_entry_node_class_init (ArvGcStructEntryNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_struct_entry_node_finalize;
	dom_node_class->get_node_name = arv_gc_struct_entry_node_get_node_name;
	dom_node_class->post_new_child = arv_gc_struct_entry_node_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_struct_entry_node_pre_remove_child;
	gc_feature_node_class->get_value_type = arv_gc_struct_entry_node_get_value_type;
	gc_feature_node_class->set_value_from_string = arv_gc_struct_entry_node_set_value_from_string;
	gc_feature_node_class->get_value_as_string = arv_gc_struct_entry_node_get_value_as_string;
}

/* ArvGcRegister interface implementation */

static void
arv_gc_struct_entry_node_get (ArvGcRegister *gc_register, void *buffer, guint64 length, GError **error)
{
	ArvDomNode *struct_register = arv_dom_node_get_parent_node (ARV_DOM_NODE (gc_register));

	g_return_if_fail (ARV_IS_GC_REGISTER (struct_register));

	arv_gc_register_get (ARV_GC_REGISTER (struct_register), buffer, length, error);
}

static void
arv_gc_struct_entry_node_set (ArvGcRegister *gc_register, void *buffer, guint64 length, GError **error)
{
	ArvDomNode *struct_register = arv_dom_node_get_parent_node (ARV_DOM_NODE (gc_register));

	g_return_if_fail (ARV_IS_GC_REGISTER (struct_register));

	return arv_gc_register_set (ARV_GC_REGISTER (struct_register), buffer, length, error);
}

static guint64
arv_gc_struct_entry_node_get_address (ArvGcRegister *gc_register, GError **error)
{
	ArvDomNode *struct_register = arv_dom_node_get_parent_node (ARV_DOM_NODE (gc_register));
	GError *local_error = NULL;
	gint64 address;

	g_return_val_if_fail (ARV_IS_GC_REGISTER (struct_register), 0);

	address = arv_gc_register_get_address (ARV_GC_REGISTER (struct_register), &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	return address;
}

static guint64
arv_gc_struct_entry_node_get_length (ArvGcRegister *gc_register, GError **error)
{
	ArvDomNode *struct_register = arv_dom_node_get_parent_node (ARV_DOM_NODE (gc_register));
	GError *local_error = NULL;
	gint64 length;

	g_return_val_if_fail (ARV_IS_GC_REGISTER (struct_register), 0);

	length = arv_gc_register_get_length (ARV_GC_REGISTER (struct_register), &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	return length;
}

static void
arv_gc_struct_entry_node_register_interface_init (ArvGcRegisterInterface *interface)
{
	interface->get = arv_gc_struct_entry_node_get;
	interface->set = arv_gc_struct_entry_node_set;
	interface->get_address = arv_gc_struct_entry_node_get_address;
	interface->get_length = arv_gc_struct_entry_node_get_length;
}

/* ArvGcInteger interface implementation */

static gint64
arv_gc_struct_entry_node_get_integer_value (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcStructEntryNode *struct_entry = ARV_GC_STRUCT_ENTRY_NODE (gc_integer);
	ArvDomNode *struct_register;
	GError *local_error = NULL;
	gint64 value;
	guint lsb;
	guint msb;

	struct_register = arv_dom_node_get_parent_node (ARV_DOM_NODE (gc_integer));
	if (!ARV_IS_GC_REGISTER_NODE (struct_register))
		return 0;

	lsb = _get_lsb (struct_entry, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	msb = _get_msb (struct_entry, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	value = arv_gc_register_node_get_masked_integer_value (ARV_GC_REGISTER_NODE (struct_register), lsb, msb, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	return value;
}

static void
arv_gc_struct_entry_node_set_integer_value (ArvGcInteger *gc_integer, gint64 value, GError **error)
{
	ArvGcStructEntryNode *struct_entry = ARV_GC_STRUCT_ENTRY_NODE (gc_integer);
	ArvDomNode *struct_register;
	GError *local_error = NULL;
	guint lsb;
	guint msb;

	struct_register = arv_dom_node_get_parent_node (ARV_DOM_NODE (gc_integer));
	if (!ARV_IS_GC_REGISTER_NODE (struct_register))
		return;

	lsb = _get_lsb (struct_entry, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	msb = _get_msb (struct_entry, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	arv_gc_register_node_set_masked_integer_value (ARV_GC_REGISTER_NODE (struct_register), lsb, msb, value, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}
}

static void
arv_gc_struct_entry_node_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_struct_entry_node_get_integer_value;
	interface->set_value = arv_gc_struct_entry_node_set_integer_value;
}

G_DEFINE_TYPE_WITH_CODE (ArvGcStructEntryNode, arv_gc_struct_entry_node, ARV_TYPE_GC_FEATURE_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_REGISTER, arv_gc_struct_entry_node_register_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_struct_entry_node_integer_interface_init))
