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
 * SECTION: arvgcstructentrynode
 * @short_description: Class for StructEntry nodes
 */

#include <arvgcstructentrynode.h>
#include <arvgcregisternode.h>
#include <arvgcregister.h>
#include <arvgcinteger.h>
#include <arvgcfeaturenodeprivate.h>
#include <arvgcregisternodeprivate.h>
#include <arvgcport.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <arvdebug.h>
#include <stdlib.h>
#include <string.h>

struct _ArvGcStructEntryNode {
	ArvGcFeatureNode node;

	ArvGcPropertyNode *sign;
	ArvGcPropertyNode *representation;
	ArvGcPropertyNode *lsb;
	ArvGcPropertyNode *msb;
	ArvGcPropertyNode *access_mode;
	ArvGcPropertyNode *cachable;

	char v_string[G_ASCII_DTOSTR_BUF_SIZE];
};

struct _ArvGcStructEntryNodeClass {
	ArvGcFeatureNodeClass parent_class;
};

static void arv_gc_struct_entry_node_register_interface_init (ArvGcRegisterInterface *interface);
static void arv_gc_struct_entry_node_integer_interface_init (ArvGcIntegerInterface *interface);

G_DEFINE_TYPE_WITH_CODE (ArvGcStructEntryNode, arv_gc_struct_entry_node, ARV_TYPE_GC_FEATURE_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_REGISTER, arv_gc_struct_entry_node_register_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_struct_entry_node_integer_interface_init))

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
				node->sign = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_REPRESENTATION:
				node->representation = property_node;
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
			case ARV_GC_PROPERTY_NODE_TYPE_ACCESS_MODE:
				node->access_mode = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_CACHABLE:
				node->cachable = property_node;
				break;
			default:
				ARV_DOM_NODE_CLASS (arv_gc_struct_entry_node_parent_class)->post_new_child (self, child);
				break;
		}
	}
}

static void
arv_gc_struct_entry_node_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcStructEntryNode implementation */

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

static ArvGcAccessMode
arv_gc_struct_entry_node_get_access_mode (ArvGcFeatureNode *gc_feature_node)
{
	ArvGcStructEntryNode *self = ARV_GC_STRUCT_ENTRY_NODE(gc_feature_node);
        ArvDomNode *struct_register;

        if (ARV_IS_GC_PROPERTY_NODE(self->access_mode))
                return arv_gc_property_node_get_access_mode (self->access_mode, ARV_GC_ACCESS_MODE_RO);

	struct_register = arv_dom_node_get_parent_node (ARV_DOM_NODE (gc_feature_node));
	if (ARV_IS_GC_REGISTER_NODE (struct_register))
                return arv_gc_feature_node_get_actual_access_mode (ARV_GC_FEATURE_NODE (struct_register));

        return ARV_GC_ACCESS_MODE_RO;
}

static void
arv_gc_struct_entry_node_finalize (GObject *object)
{
	G_OBJECT_CLASS (arv_gc_struct_entry_node_parent_class)->finalize (object);
}

static void
arv_gc_struct_entry_node_class_init (ArvGcStructEntryNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);

	object_class->finalize = arv_gc_struct_entry_node_finalize;
	dom_node_class->get_node_name = arv_gc_struct_entry_node_get_node_name;
	dom_node_class->post_new_child = arv_gc_struct_entry_node_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_struct_entry_node_pre_remove_child;
	gc_feature_node_class->get_access_mode = arv_gc_struct_entry_node_get_access_mode;
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
arv_gc_struct_entry_node_set (ArvGcRegister *gc_register, const void *buffer, guint64 length, GError **error)
{
	ArvDomNode *struct_register = arv_dom_node_get_parent_node (ARV_DOM_NODE (gc_register));

	g_return_if_fail (ARV_IS_GC_REGISTER (struct_register));

	arv_gc_feature_node_increment_change_count (ARV_GC_FEATURE_NODE (gc_register));
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

	struct_register = arv_dom_node_get_parent_node (ARV_DOM_NODE (gc_integer));
	if (!ARV_IS_GC_REGISTER_NODE (struct_register))
		return 0;

	return arv_gc_register_node_get_masked_integer_value
		(ARV_GC_REGISTER_NODE (struct_register),
		 arv_gc_property_node_get_lsb (struct_entry->lsb, 0),
		 arv_gc_property_node_get_msb (struct_entry->msb, 31),
		 arv_gc_property_node_get_sign (struct_entry->sign, ARV_GC_SIGNEDNESS_UNSIGNED),
		 0,
		 arv_gc_property_node_get_cachable (struct_entry->cachable, ARV_GC_CACHABLE_WRITE_AROUND),
		 TRUE, error);
}

static void
arv_gc_struct_entry_node_set_integer_value (ArvGcInteger *gc_integer, gint64 value, GError **error)
{
	ArvGcStructEntryNode *struct_entry = ARV_GC_STRUCT_ENTRY_NODE (gc_integer);
	ArvDomNode *struct_register;

	struct_register = arv_dom_node_get_parent_node (ARV_DOM_NODE (gc_integer));
	if (!ARV_IS_GC_REGISTER_NODE (struct_register))
		return;

	arv_gc_feature_node_increment_change_count (ARV_GC_FEATURE_NODE (gc_integer));
	arv_gc_register_node_set_masked_integer_value
		(ARV_GC_REGISTER_NODE (struct_register),
		 arv_gc_property_node_get_lsb (struct_entry->lsb, 0),
		 arv_gc_property_node_get_msb (struct_entry->msb, 31),
		 arv_gc_property_node_get_sign (struct_entry->sign, ARV_GC_SIGNEDNESS_UNSIGNED),
		 0,
		 arv_gc_property_node_get_cachable (struct_entry->cachable, ARV_GC_CACHABLE_WRITE_AROUND),
		 TRUE, value, error);
}

static gint64
arv_gc_struct_entry_node_get_min (ArvGcInteger *self, GError **error)
{
	ArvGcStructEntryNode *struct_entry = ARV_GC_STRUCT_ENTRY_NODE (self);
	ArvDomNode *struct_register = arv_dom_node_get_parent_node (ARV_DOM_NODE (self));
	gint64 lsb, msb, min;
	ArvGcSignedness signedness;
	guint endianness;

	signedness = arv_gc_property_node_get_sign (struct_entry->sign, ARV_GC_SIGNEDNESS_UNSIGNED);
	endianness = arv_gc_register_node_get_endianness (ARV_GC_REGISTER_NODE (struct_register));
	lsb = arv_gc_property_node_get_lsb (struct_entry->lsb, endianness == G_BIG_ENDIAN ? 31 : 0);
	msb = arv_gc_property_node_get_msb (struct_entry->msb, endianness == G_BIG_ENDIAN ? 0 : 31);

	if ((endianness == G_BIG_ENDIAN && (msb > lsb)) ||
	    (endianness != G_BIG_ENDIAN && (lsb > msb))) {
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_INVALID_BIT_RANGE,
			     "[%s] Invalid bit range for node",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (self)));
		return G_MAXINT64;
	}

	if (signedness == ARV_GC_SIGNEDNESS_SIGNED) {
		min = endianness == G_BIG_ENDIAN ?
			-(((gint64) 1 << (lsb - msb))) :
			-(((gint64) 1 << (msb - lsb)));
	} else {
		min = 0;
	}

	return min;
}

static gint64
arv_gc_struct_entry_node_get_max (ArvGcInteger *self, GError **error)
{
	ArvGcStructEntryNode *struct_entry = ARV_GC_STRUCT_ENTRY_NODE (self);
	ArvDomNode *struct_register = arv_dom_node_get_parent_node (ARV_DOM_NODE (self));
	gint64 lsb, msb, max;
	ArvGcSignedness signedness;
	guint endianness;

	signedness = arv_gc_property_node_get_sign (struct_entry->sign, ARV_GC_SIGNEDNESS_UNSIGNED);
	endianness = arv_gc_register_node_get_endianness (ARV_GC_REGISTER_NODE (struct_register));
	lsb = arv_gc_property_node_get_lsb (struct_entry->lsb, endianness == G_BIG_ENDIAN ? 31 : 0);
	msb = arv_gc_property_node_get_msb (struct_entry->msb, endianness == G_BIG_ENDIAN ? 0 : 31);

	if ((endianness == G_BIG_ENDIAN && (msb > lsb)) ||
	    (endianness != G_BIG_ENDIAN && (lsb > msb))) {
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_INVALID_BIT_RANGE,
			     "[%s] Invalid bit range for node",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (self)));
		return G_MAXINT64;
	}

	if (signedness == ARV_GC_SIGNEDNESS_SIGNED) {
		max = endianness == G_BIG_ENDIAN ?
			(((gint64) 1 << (lsb - msb)) - 1) :
			(((gint64) 1 << (msb - lsb)) - 1);
	} else {
		max = endianness == G_BIG_ENDIAN ?
			((gint64) 1 << (lsb - msb + 1)) - 1 :
			((gint64) 1 << (msb - lsb + 1)) - 1;
	}

	return max;
}

static ArvGcRepresentation
arv_gc_struct_entry_node_get_representation (ArvGcInteger *self)
{
	ArvGcStructEntryNode *struct_entry = ARV_GC_STRUCT_ENTRY_NODE (self);

	if (struct_entry->representation == NULL)
		return ARV_GC_REPRESENTATION_UNDEFINED;

	return arv_gc_property_node_get_representation (struct_entry->representation, ARV_GC_REPRESENTATION_UNDEFINED);
}

static void
arv_gc_struct_entry_node_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_struct_entry_node_get_integer_value;
	interface->set_value = arv_gc_struct_entry_node_set_integer_value;
	interface->get_min = arv_gc_struct_entry_node_get_min;
	interface->get_max = arv_gc_struct_entry_node_get_max;
	interface->get_representation = arv_gc_struct_entry_node_get_representation;
}
