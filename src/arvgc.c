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
 * SECTION:arvgc
 * @short_description: Genicam root document class
 *
 * #ArvGc implements the root document for the storage of the Genicam feature
 * nodes. It builds the node tree by parsing an xml file in the Genicam
 * standard format. See http://www.genicam.org.
 */

#include <arvgc.h>
#include <arvgcnode.h>
#include <arvgcpropertynode.h>
#include <arvgcindexnode.h>
#include <arvgcvalueindexednode.h>
#include <arvgcinvalidatornode.h>
#include <arvgcregisterdescriptionnode.h>
#include <arvgcgroupnode.h>
#include <arvgccategory.h>
#include <arvgcenumeration.h>
#include <arvgcenumentry.h>
#include <arvgcintegernode.h>
#include <arvgcfloatnode.h>
#include <arvgcregisternode.h>
#include <arvgcintregnode.h>
#include <arvgcmaskedintregnode.h>
#include <arvgcfloatregnode.h>
#include <arvgcstringregnode.h>
#include <arvgcstructregnode.h>
#include <arvgcstructentrynode.h>
#include <arvgccommand.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcboolean.h>
#include <arvgcswissknifenode.h>
#include <arvgcintswissknifenode.h>
#include <arvgcconverternode.h>
#include <arvgcintconverternode.h>
#include <arvgcport.h>
#include <arvbuffer.h>
#include <arvdebugprivate.h>
#include <arvdomparser.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct {
	GHashTable *nodes;
	ArvDevice *device;
	ArvBuffer *buffer;

	ArvRegisterCachePolicy cache_policy;
	ArvRangeCheckPolicy range_check_policy;
} ArvGcPrivate;

struct _ArvGc {
	ArvDomDocument base;

	ArvGcPrivate *priv;
};

struct _ArvGcClass {
	ArvDomDocumentClass parent_class;
};

GQuark
arv_gc_error_quark (void)
{
	return g_quark_from_static_string ("arv-gc-error-quark");
}

/* ArvDomNode implementation */

static gboolean
arv_gc_can_append_child (ArvDomNode *self, ArvDomNode *child)
{
	return ARV_IS_GC_NODE (child);
}

/* ArvDomDocument implementation */

static ArvDomElement *
arv_gc_create_element (ArvDomDocument *document, const char *tag_name)
{
	ArvGcNode *node = NULL;

	if (strcmp (tag_name, "Category") == 0)
		node = arv_gc_category_new ();
	else if (strcmp (tag_name, "Command") == 0)
		node = arv_gc_command_new ();
	else if (strcmp (tag_name, "Converter") == 0)
		node = arv_gc_converter_node_new ();
	else if (strcmp (tag_name, "IntConverter") == 0)
		node = arv_gc_int_converter_node_new ();
	else if (strcmp (tag_name, "Register") == 0)
		node = arv_gc_register_node_new ();
	else if (strcmp (tag_name, "IntReg") == 0)
		node = arv_gc_int_reg_node_new ();
	else if (strcmp (tag_name, "MaskedIntReg") == 0)
		node = arv_gc_masked_int_reg_node_new ();
	else if (strcmp (tag_name, "FloatReg") == 0)
		node = arv_gc_float_reg_node_new ();
	else if (strcmp (tag_name, "StringReg") == 0)
		node = arv_gc_string_reg_node_new ();
	else if (strcmp (tag_name, "StructReg") == 0)
		node = arv_gc_struct_reg_node_new ();
	else if (strcmp (tag_name, "StructEntry") == 0)
		node = arv_gc_struct_entry_node_new ();
	else if (strcmp (tag_name, "Integer") == 0)
		node = arv_gc_integer_node_new ();
	else if (strcmp (tag_name, "Float") == 0)
		node = arv_gc_float_node_new ();
	else if (strcmp (tag_name, "Boolean") == 0)
		node = arv_gc_boolean_new ();
	else if (strcmp (tag_name, "Enumeration") == 0)
		node = arv_gc_enumeration_new ();
	else if (strcmp (tag_name, "EnumEntry") == 0)
		node = arv_gc_enum_entry_new ();
	else if (strcmp (tag_name, "SwissKnife") == 0)
		node = arv_gc_swiss_knife_node_new ();
	else if (strcmp (tag_name, "IntSwissKnife") == 0)
		node = arv_gc_int_swiss_knife_node_new ();
	else if (strcmp (tag_name, "Port") == 0)
		node = arv_gc_port_new ();
	else if (strcmp (tag_name, "pIndex") == 0)
		node = arv_gc_index_node_new ();
	else if (strcmp (tag_name, "RegisterDescription") == 0)
		node = arv_gc_register_description_node_new ();
	else if (strcmp (tag_name, "pFeature") == 0)
		node = arv_gc_property_node_new_p_feature ();
	else if (strcmp (tag_name, "Value") == 0)
		node = arv_gc_property_node_new_value ();
	else if (strcmp (tag_name, "pValue") == 0)
		node = arv_gc_property_node_new_p_value ();
	else if (strcmp (tag_name, "Address") == 0)
		node = arv_gc_property_node_new_address ();
	else if (strcmp (tag_name, "pAddress") == 0)
		node = arv_gc_property_node_new_p_address ();
	else if (strcmp (tag_name, "Description") == 0)
		node = arv_gc_property_node_new_description ();
	else if (strcmp (tag_name, "Visibility") == 0)
		node = arv_gc_property_node_new_visibility ();
	else if (strcmp (tag_name, "ToolTip") == 0)
		node = arv_gc_property_node_new_tooltip ();
	else if (strcmp (tag_name, "DisplayName") == 0)
		node = arv_gc_property_node_new_display_name ();
	else if (strcmp (tag_name, "Min") == 0)
		node = arv_gc_property_node_new_minimum ();
	else if (strcmp (tag_name, "pMin") == 0)
		node = arv_gc_property_node_new_p_minimum ();
	else if (strcmp (tag_name, "Max") == 0)
		node = arv_gc_property_node_new_maximum ();
	else if (strcmp (tag_name, "pMax") == 0)
		node = arv_gc_property_node_new_p_maximum ();
	else if (strcmp (tag_name, "Inc") == 0)
		node = arv_gc_property_node_new_increment ();
	else if (strcmp (tag_name, "pInc") == 0)
		node = arv_gc_property_node_new_p_increment ();
	else if (strcmp (tag_name, "IsLinear") == 0)
		node = arv_gc_property_node_new_is_linear ();
	else if (strcmp (tag_name, "Slope") == 0)
		node = arv_gc_property_node_new_slope ();
	else if (strcmp (tag_name, "Unit") == 0)
		node = arv_gc_property_node_new_unit ();
	else if (strcmp (tag_name, "Representation") == 0)
		node = arv_gc_property_node_new_representation ();
	else if (strcmp (tag_name, "DisplayNotation") == 0)
		node = arv_gc_property_node_new_display_notation ();
	else if (strcmp (tag_name, "DisplayPrecision") == 0)
		node = arv_gc_property_node_new_display_precision ();
	else if (strcmp (tag_name, "OnValue") == 0)
		node = arv_gc_property_node_new_on_value ();
	else if (strcmp (tag_name, "OffValue") == 0)
		node = arv_gc_property_node_new_off_value ();
	else if (strcmp (tag_name, "pIsImplemented") == 0)
		node = arv_gc_property_node_new_p_is_implemented ();
	else if (strcmp (tag_name, "pIsAvailable") == 0)
		node = arv_gc_property_node_new_p_is_available ();
	else if (strcmp (tag_name, "pIsLocked") == 0)
		node = arv_gc_property_node_new_p_is_locked ();
	else if (strcmp (tag_name, "pSelected") == 0)
		node = arv_gc_property_node_new_p_selected ();
	else if (strcmp (tag_name, "Length") == 0)
		node = arv_gc_property_node_new_length ();
	else if (strcmp (tag_name, "pLength") == 0)
		node = arv_gc_property_node_new_p_length ();
	else if (strcmp (tag_name, "pPort") == 0)
		node = arv_gc_property_node_new_p_port ();
	else if (strcmp (tag_name, "pVariable") == 0)
		node = arv_gc_property_node_new_p_variable ();
	else if (strcmp (tag_name, "ValueIndexed") == 0)
		node = arv_gc_value_indexed_node_new ();
	else if (strcmp (tag_name, "pValueIndexed") == 0)
		node = arv_gc_p_value_indexed_node_new ();
	else if (strcmp (tag_name, "ValueDefault") == 0)
		node = arv_gc_property_node_new_value_default ();
	else if (strcmp (tag_name, "pValueDefault") == 0)
		node = arv_gc_property_node_new_p_value_default ();
	else if (strcmp (tag_name, "Formula") == 0)
		node = arv_gc_property_node_new_formula ();
	else if (strcmp (tag_name, "FormulaTo") == 0)
		node = arv_gc_property_node_new_formula_to ();
	else if (strcmp (tag_name, "FormulaFrom") == 0)
		node = arv_gc_property_node_new_formula_from ();
	else if (strcmp (tag_name, "Expression") == 0)
		node = arv_gc_property_node_new_expression ();
	else if (strcmp (tag_name, "Constant") == 0)
		node = arv_gc_property_node_new_constant ();

	else if (strcmp (tag_name, "AccessMode") == 0)
		node = arv_gc_property_node_new_access_mode ();
	else if (strcmp (tag_name, "ImposedAccessMode") == 0)
		node = arv_gc_property_node_new_imposed_access_mode ();
	else if (strcmp (tag_name, "Cachable") == 0)
		node = arv_gc_property_node_new_cachable ();
	else if (strcmp (tag_name, "PollingTime") == 0)
		node = arv_gc_property_node_new_polling_time ();
	else if (strcmp (tag_name, "Endianess") == 0)
		node = arv_gc_property_node_new_endianness ();
	else if (strcmp (tag_name, "Sign") == 0)
		node = arv_gc_property_node_new_sign ();
	else if (strcmp (tag_name, "LSB") == 0)
		node = arv_gc_property_node_new_lsb ();
	else if (strcmp (tag_name, "MSB") == 0)
		node = arv_gc_property_node_new_msb ();
	else if (strcmp (tag_name, "Bit") == 0)
		node = arv_gc_property_node_new_bit ();
	else if (strcmp (tag_name, "pInvalidator") == 0)
		node = arv_gc_invalidator_node_new ();

	else if (strcmp (tag_name, "CommandValue") == 0)
		node = arv_gc_property_node_new_command_value ();
	else if (strcmp (tag_name, "pCommandValue") == 0)
		node = arv_gc_property_node_new_p_command_value ();

	else if (strcmp (tag_name, "ChunkID") == 0)
		node = arv_gc_property_node_new_chunk_id ();
	else if (strcmp (tag_name, "EventID") == 0)
		node = arv_gc_property_node_new_event_id ();

	else if (strcmp (tag_name, "Group") == 0)
		node = arv_gc_group_node_new ();
	else
		arv_debug_dom ("[Genicam::create_element] Unknown tag (%s)", tag_name);

	return ARV_DOM_ELEMENT (node);
}

/* ArvGc implementation */

/**
 * arv_gc_get_node:
 * @genicam: a #ArvGc object
 * @name: node name
 *
 * Retrieves a genicam node by name.
 *
 * Return value: (transfer none): a #ArvGcNode, null if not found.
 */

ArvGcNode *
arv_gc_get_node	(ArvGc *genicam, const char *name)
{
	g_return_val_if_fail (ARV_IS_GC (genicam), NULL);
	g_return_val_if_fail (name != NULL, NULL);

	return g_hash_table_lookup (genicam->priv->nodes, name);
}

/**
 * arv_gc_get_device:
 * @genicam: a #ArvGc object
 *
 * Retrieves the device handled by this genicam interface. The device is used for register access.
 *
 * Return value: (transfer none): a #ArvDevice.
 */

ArvDevice *
arv_gc_get_device (ArvGc *genicam)
{
	g_return_val_if_fail (ARV_IS_GC (genicam), NULL);

	return genicam->priv->device;
}

void
arv_gc_register_feature_node (ArvGc *genicam, ArvGcFeatureNode *node)
{
	const char *name;

	g_return_if_fail (ARV_IS_GC (genicam));
	g_return_if_fail (ARV_IS_GC_FEATURE_NODE (node));


	name = arv_gc_feature_node_get_name (node);
	if (name == NULL)
		return;

	g_object_ref (node);

	g_hash_table_remove (genicam->priv->nodes, (char *) name);
	g_hash_table_insert (genicam->priv->nodes, (char *) name, node);

	arv_log_genicam ("[Gc::register_feature_node] Register node '%s' [%s]", name,
			 arv_dom_node_get_node_name (ARV_DOM_NODE (node)));
}

void
arv_gc_set_default_node_data (ArvGc *genicam, const char *node_name, ...)
{
	va_list args;
	char *node_data;

	g_return_if_fail (ARV_IS_GC (genicam));
	g_return_if_fail (node_name != NULL);

	if (arv_gc_get_node (genicam, node_name) != NULL)
		return;

	arv_debug_genicam ("[Gc::set_default_node_data] Add '%s'", node_name);

	va_start (args, node_name);
	do {
		node_data = va_arg (args, char *);
		if (node_data != NULL)
			arv_dom_document_append_from_memory (ARV_DOM_DOCUMENT (genicam), NULL, node_data, -1, NULL);
	} while (node_data != NULL);
	va_end (args);
}

void
arv_gc_set_register_cache_policy (ArvGc *genicam, ArvRegisterCachePolicy policy)
{
	g_return_if_fail (ARV_IS_GC (genicam));

	genicam->priv->cache_policy = policy;
}

ArvRegisterCachePolicy
arv_gc_get_register_cache_policy (ArvGc *genicam)
{
	g_return_val_if_fail (ARV_IS_GC (genicam), ARV_REGISTER_CACHE_POLICY_DISABLE);

	return genicam->priv->cache_policy;
}

void
arv_gc_set_range_check_policy (ArvGc *genicam, ArvRangeCheckPolicy policy)
{
	g_return_if_fail (ARV_IS_GC (genicam));

	genicam->priv->range_check_policy = policy;
}

ArvRangeCheckPolicy
arv_gc_get_range_check_policy (ArvGc *genicam)
{
	g_return_val_if_fail (ARV_IS_GC (genicam), ARV_RANGE_CHECK_POLICY_DISABLE);

	return genicam->priv->range_check_policy;
}

static void
_weak_notify_cb (gpointer data, GObject *object)
{
	ArvGc *genicam = data;

	genicam->priv->buffer = NULL;
}

void
arv_gc_set_buffer (ArvGc *genicam, ArvBuffer *buffer)
{
	g_return_if_fail (ARV_IS_GC (genicam));
	g_return_if_fail (ARV_IS_BUFFER (buffer));

	if (genicam->priv->buffer != NULL)
		g_object_weak_unref (G_OBJECT (genicam->priv->buffer), _weak_notify_cb, genicam);

	g_object_weak_ref (G_OBJECT (buffer), _weak_notify_cb, genicam);

	genicam->priv->buffer = buffer;
}

/**
 * arv_gc_get_buffer:
 * @genicam: a #ArvGc object
 *
 * Retrieves the binded buffer.
 *
 * Return value: (transfer none): a #ArvBuffer.
 */

ArvBuffer *
arv_gc_get_buffer (ArvGc *genicam)
{
	g_return_val_if_fail (ARV_IS_GC (genicam), NULL);

	return genicam->priv->buffer;
}

ArvGc *
arv_gc_new (ArvDevice *device, const void *xml, size_t size)
{
	ArvDomDocument *document;
	ArvGc *genicam;

	document = arv_dom_document_new_from_memory (xml, size, NULL);
	if (!ARV_IS_GC (document)) {
		if (document != NULL)
			g_object_unref (document);
		return NULL;
	}

	genicam = ARV_GC (document);
	genicam->priv->device = device;

	return genicam;
}

G_DEFINE_TYPE_WITH_CODE (ArvGc, arv_gc, ARV_TYPE_DOM_DOCUMENT, G_ADD_PRIVATE (ArvGc))

static void
arv_gc_init (ArvGc *genicam)
{
	genicam->priv = arv_gc_get_instance_private (genicam);

	genicam->priv->nodes = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_object_unref);
	genicam->priv->cache_policy = ARV_REGISTER_CACHE_POLICY_DISABLE;
}

static void
arv_gc_finalize (GObject *object)
{
	ArvGc *genicam = ARV_GC (object);

	if (genicam->priv->buffer != NULL)
		g_object_weak_unref (G_OBJECT (genicam->priv->buffer), _weak_notify_cb, genicam);

	g_hash_table_unref (genicam->priv->nodes);

	G_OBJECT_CLASS (arv_gc_parent_class)->finalize (object);
}

static void
arv_gc_class_init (ArvGcClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);
	ArvDomNodeClass *d_node_class = ARV_DOM_NODE_CLASS (node_class);
	ArvDomDocumentClass *d_document_class = ARV_DOM_DOCUMENT_CLASS (node_class);

	object_class->finalize = arv_gc_finalize;
	d_node_class->can_append_child = arv_gc_can_append_child;
	d_document_class->create_element = arv_gc_create_element;
}
