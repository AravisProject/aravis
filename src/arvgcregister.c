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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvgcregister
 * @short_description: Class for Register, IntReg, MaskedIntReg, FloatReg and StringReg nodes
 */

#include <arvgcregister.h>
#include <arvgcindexnode.h>
#include <arvgcinvalidatornode.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcstring.h>
#include <arvgcport.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <arvdebug.h>
#include <stdlib.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_gc_register_get_node_name (ArvDomNode *node)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (node);

	switch (gc_register->type) {
		case ARV_GC_REGISTER_TYPE_REGISTER:
			return "Register";
		case ARV_GC_REGISTER_TYPE_INTEGER:
			return "IntReg";
		case ARV_GC_REGISTER_TYPE_MASKED_INTEGER:
			return "MaskedIntReg";
		case ARV_GC_REGISTER_TYPE_FLOAT:
			return "FloatReg";
		case ARV_GC_REGISTER_TYPE_STRING:
			return "StringReg";
	}

	return NULL;
}

static void
arv_gc_register_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcRegister *node = ARV_GC_REGISTER (self);

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_ADDRESS:
			case ARV_GC_PROPERTY_NODE_TYPE_P_ADDRESS:
				node->addresses = g_slist_prepend (node->addresses, child);
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_INDEX:
				node->index = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_LENGTH:
			case ARV_GC_PROPERTY_NODE_TYPE_P_LENGTH:
				node->length = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_PORT:
				node->port = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_ACCESS_MODE:
				/* TODO */
				node->access_mode = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_CACHABLE:
				node->cachable = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_POLLING_TIME:
				/* TODO */
				node->polling_time = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_ENDIANESS:
				node->endianess = property_node;
				break;
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
			case ARV_GC_PROPERTY_NODE_TYPE_P_INVALIDATOR:
				node->invalidators = g_slist_prepend (node->invalidators, property_node);
				break;
			default:
				ARV_DOM_NODE_CLASS (parent_class)->post_new_child (self, child);
				break;
		}
	}
}

static void
arv_gc_register_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcFeatureNode implementation */

static GType
arv_gc_register_get_value_type (ArvGcFeatureNode *node)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (node);

	return gc_register->value_type;
}

static void
arv_gc_register_set_value_from_string (ArvGcFeatureNode *node, const char *string)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (node);

	switch (gc_register->value_type) {
		case G_TYPE_INT64:
			arv_gc_integer_set_value (ARV_GC_INTEGER (node), g_ascii_strtoll (string, NULL, 0));
			break;
		case G_TYPE_DOUBLE:
			arv_gc_float_set_value (ARV_GC_FLOAT (node), g_ascii_strtod (string, NULL));
			break;
		case G_TYPE_STRING:
			arv_gc_string_set_value (ARV_GC_STRING (node), string);
			break;
		default:
			break;
	}
}

static const char *
arv_gc_register_get_value_as_string (ArvGcFeatureNode *node)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (node);

	switch (gc_register->value_type) {
		case G_TYPE_INT64:
			g_snprintf (gc_register->v_string, G_ASCII_DTOSTR_BUF_SIZE,
				    "0x%08" G_GINT64_MODIFIER "x", arv_gc_integer_get_value (ARV_GC_INTEGER (node)));
			return gc_register->v_string;
		case G_TYPE_DOUBLE:
			g_ascii_dtostr (gc_register->v_string, G_ASCII_DTOSTR_BUF_SIZE,
					arv_gc_float_get_value (ARV_GC_FLOAT (node)));
			return gc_register->v_string;
		case G_TYPE_STRING:
			return arv_gc_string_get_value (ARV_GC_STRING (node));
		default:
			break;
	}

	return NULL;
}

/* ArvGcRegister implementation */

gboolean
_get_cache_validity (ArvGcRegister *gc_register)
{
	GSList *iter;
	gint modification_count;
	gint feature_modification_count;
	gboolean is_cache_valid = gc_register->is_cache_valid;

	for (iter = gc_register->invalidators; iter != NULL; iter = iter->next) {
		ArvGcInvalidatorNode *invalidator = iter->data;
		ArvGcNode *node;

		modification_count = arv_gc_invalidator_node_get_modification_count (invalidator);
		node = arv_gc_property_node_get_linked_node (ARV_GC_PROPERTY_NODE (invalidator));
		feature_modification_count = arv_gc_feature_node_get_modification_count (ARV_GC_FEATURE_NODE (node));
		arv_gc_invalidator_node_set_modification_count (invalidator, feature_modification_count);
		if (modification_count != feature_modification_count)
			is_cache_valid = FALSE;
	}

	return is_cache_valid;
}

static gint64
_get_length (ArvGcRegister *gc_register)
{
	if (gc_register->length == NULL)
		return 4;

	return arv_gc_property_node_get_int64 (gc_register->length);
}

static void
_update_cache_size (ArvGcRegister *gc_register)
{
	gint64 length;

	length = _get_length (gc_register);
	if (length != gc_register->cache_size) {
		g_free (gc_register->cache);
		gc_register->cache = g_malloc (length);
		gc_register->cache_size = length;
	}

}

static ArvGcCachable
_get_cachable (ArvGcRegister *gc_register)
{
	const char *cachable;

	if (gc_register->cachable == NULL)
		return ARV_GC_CACHABLE_NO_CACHE;

	cachable = arv_gc_property_node_get_string (gc_register->cachable);
	if (g_strcmp0 (cachable, "WriteThrough") == 0)
		return ARV_GC_CACHABLE_WRITE_TRHOUGH;
	else if (strcmp (cachable, "WriteAround") == 0)
		return ARV_GC_CACHABLE_WRITE_AROUND;

	return ARV_GC_CACHABLE_NO_CACHE;
}

/* Set default to read only 32 bits little endian integer register */

static ArvGcCachable
_get_endianess (ArvGcRegister *gc_register)
{
	const char *endianess;

	if (gc_register->endianess == NULL)
		return G_LITTLE_ENDIAN;

	endianess = arv_gc_property_node_get_string (gc_register->endianess);
	if (g_strcmp0 (endianess, "BigEndian") == 0)
		return G_BIG_ENDIAN;

	return G_LITTLE_ENDIAN;
}

static ArvGcCachable
_get_lsb (ArvGcRegister *gc_register)
{
	if (gc_register->lsb == NULL)
		return 0;

	return arv_gc_property_node_get_int64 (gc_register->lsb);
}

static ArvGcCachable
_get_msb (ArvGcRegister *gc_register)
{
	if (gc_register->msb == NULL)
		return 31;

	return arv_gc_property_node_get_int64 (gc_register->msb);
}

static void
_read_cache (ArvGcRegister *gc_register)
{
	ArvGcNode *port;

	if (gc_register->is_cache_valid == TRUE) {
		arv_log_genicam ("[GcRegister::read_cache] Cache is valid");
		return;
	}

	port = arv_gc_property_node_get_linked_node (gc_register->port);
	if (!ARV_IS_GC_PORT (port))
		return;

	_update_cache_size (gc_register);

	arv_gc_port_read (ARV_GC_PORT (port),
			  gc_register->cache,
			  arv_gc_register_get_address (gc_register),
			  gc_register->cache_size);

	if (_get_cachable (gc_register) != ARV_GC_CACHABLE_NO_CACHE)
		gc_register->is_cache_valid = TRUE;
	else
		gc_register->is_cache_valid = FALSE;
}

static void
_write_cache (ArvGcRegister *gc_register)
{
	ArvGcNode *port;

	arv_gc_feature_node_inc_modification_count (ARV_GC_FEATURE_NODE (gc_register));

	port = arv_gc_property_node_get_linked_node (gc_register->port);
	if (!ARV_IS_GC_PORT (port))
		return;

	_update_cache_size (gc_register);

	arv_gc_port_write (ARV_GC_PORT (port),
			   gc_register->cache,
			   arv_gc_register_get_address (gc_register),
			   gc_register->cache_size);

	if (_get_cachable (gc_register) == ARV_GC_CACHABLE_WRITE_TRHOUGH)
		gc_register->is_cache_valid = TRUE;
	else
		gc_register->is_cache_valid = FALSE;
}

void
arv_gc_register_get (ArvGcRegister *gc_register, void *buffer, guint64 length)
{
	g_return_if_fail (ARV_IS_GC_REGISTER (gc_register));

	_read_cache (gc_register);

	if (length > gc_register->cache_size) {
		memcpy (buffer, gc_register->cache, gc_register->cache_size);
		memset (buffer + gc_register->cache_size, 0, length - gc_register->cache_size);
	} else
		memcpy (buffer, gc_register->cache, length);

	arv_log_genicam ("[GcRegister::get] 0x%Lx,%Ld",
			 arv_gc_register_get_address (gc_register), length);
}

void
arv_gc_register_set (ArvGcRegister *gc_register, void *buffer, guint64 length)
{
	g_return_if_fail (ARV_IS_GC_REGISTER (gc_register));

	if (gc_register->cache_size > length) {
		memcpy (gc_register->cache, buffer, length);
		memset (gc_register->cache + length, 0, gc_register->cache_size - length);
	} else
		memcpy (gc_register->cache, buffer, gc_register->cache_size);

	_write_cache (gc_register);

	arv_log_genicam ("[GcRegister::set] 0x%Lx,%Ld", arv_gc_register_get_address (gc_register), length);
}

guint64
arv_gc_register_get_address (ArvGcRegister *gc_register)
{
	ArvGc *genicam;
	GSList *iter;
	guint64 value = 0;

	g_return_val_if_fail (ARV_IS_GC_REGISTER (gc_register), 0);
	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_register));
	g_return_val_if_fail (ARV_IS_GC (genicam), 0);

	for (iter = gc_register->addresses; iter != NULL; iter = iter->next)
		value += arv_gc_property_node_get_int64 (iter->data);

	if (gc_register->index != NULL)
		value += arv_gc_index_node_get_index (ARV_GC_INDEX_NODE (gc_register->index),
						      arv_gc_register_get_length (gc_register));

	return value;
}

guint64
arv_gc_register_get_length (ArvGcRegister *gc_register)
{
	g_return_val_if_fail (ARV_IS_GC_REGISTER (gc_register), 0);

	return _get_length (gc_register);
}

ArvGcNode *
arv_gc_register_new (void)
{
	ArvGcRegister *gc_register;

	gc_register = g_object_new (ARV_TYPE_GC_REGISTER, NULL);
	gc_register->type = ARV_GC_REGISTER_TYPE_REGISTER;
	gc_register->value_type = G_TYPE_BYTE_ARRAY;

	return ARV_GC_NODE (gc_register);
}

ArvGcNode *
arv_gc_register_new_integer (void)
{
	ArvGcRegister *gc_register;

	gc_register = g_object_new (ARV_TYPE_GC_REGISTER, NULL);
	gc_register->type = ARV_GC_REGISTER_TYPE_INTEGER;
	gc_register->value_type = G_TYPE_INT64;

	return ARV_GC_NODE (gc_register);
}

ArvGcNode *
arv_gc_register_new_masked_integer (void)
{
	ArvGcRegister *gc_register;

	gc_register = g_object_new (ARV_TYPE_GC_REGISTER, NULL);
	gc_register->type = ARV_GC_REGISTER_TYPE_MASKED_INTEGER;
	gc_register->value_type = G_TYPE_INT64;

	return ARV_GC_NODE (gc_register);
}

ArvGcNode *
arv_gc_register_new_float (void)
{
	ArvGcRegister *gc_register;

	gc_register = g_object_new (ARV_TYPE_GC_REGISTER, NULL);
	gc_register->type = ARV_GC_REGISTER_TYPE_FLOAT;
	gc_register->value_type = G_TYPE_DOUBLE;

	return ARV_GC_NODE (gc_register);
}

ArvGcNode *
arv_gc_register_new_string (void)
{
	ArvGcRegister *gc_register;

	gc_register = g_object_new (ARV_TYPE_GC_REGISTER, NULL);
	gc_register->type = ARV_GC_REGISTER_TYPE_STRING;
	gc_register->value_type = G_TYPE_STRING;

	return ARV_GC_NODE (gc_register);
}

static void
arv_gc_register_init (ArvGcRegister *gc_register)
{
	gc_register->cache = g_malloc0(4);
	gc_register->cache_size = 4;
	gc_register->is_cache_valid = FALSE;
}

static void
arv_gc_register_finalize (GObject *object)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (object);

	g_slist_free (gc_register->addresses);
	g_free (gc_register->cache);
	g_slist_free (gc_register->invalidators);

	parent_class->finalize (object);
}

static void
arv_gc_register_class_init (ArvGcRegisterClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_register_finalize;
	dom_node_class->get_node_name = arv_gc_register_get_node_name;
	dom_node_class->post_new_child = arv_gc_register_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_register_pre_remove_child;
	gc_feature_node_class->get_value_type = arv_gc_register_get_value_type;
	gc_feature_node_class->set_value_from_string = arv_gc_register_set_value_from_string;
	gc_feature_node_class->get_value_as_string = arv_gc_register_get_value_as_string;
}

/* ArvGcInteger interface implementation */

static gint64
arv_gc_register_get_integer_value (ArvGcInteger *gc_integer)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (gc_integer);
	gint64 value;
	guint lsb;
	guint msb;
	guint endianess;

	endianess = _get_endianess (gc_register);

	_read_cache (gc_register);

	arv_copy_memory_with_endianess (&value, sizeof (value), G_BYTE_ORDER,
					gc_register->cache, gc_register->cache_size, endianess);

	if (gc_register->type == ARV_GC_REGISTER_TYPE_MASKED_INTEGER) {
		guint64 mask;

		if (endianess == G_BYTE_ORDER) {
			msb = _get_msb (gc_register);
			lsb = _get_lsb (gc_register);
		} else {
			lsb = 8 * gc_register->cache_size - _get_lsb (gc_register) - 1;
			msb = 8 * gc_register->cache_size - _get_msb (gc_register) - 1;
		}

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
		if (msb - lsb < 63)
			mask = ((((guint64) 1) << (msb - lsb + 1)) - 1) << lsb;
		else
			mask = G_MAXUINT64;

		value = (value & mask) >> lsb;
#else
		g_assert_not_reached ();
#endif
	}

	arv_log_genicam ("[GcRegister::get_integer_value] address = 0x%Lx, value = 0x%Lx",
			 arv_gc_register_get_address (gc_register),
			 value);

	return value;
}

static void
arv_gc_register_set_integer_value (ArvGcInteger *gc_integer, gint64 value)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (gc_integer);
	guint lsb;
	guint msb;
	guint endianess;

	endianess = _get_endianess (gc_register);

	if (gc_register->type == ARV_GC_REGISTER_TYPE_MASKED_INTEGER) {
		gint64 current_value;
		guint64 mask;

		_read_cache (gc_register);

		arv_copy_memory_with_endianess (&current_value, sizeof (current_value), G_BYTE_ORDER,
						gc_register->cache, gc_register->cache_size, endianess);

		if (endianess == G_BYTE_ORDER) {
			msb = _get_msb (gc_register);
			lsb = _get_lsb (gc_register);
		} else {
			lsb = 8 * gc_register->cache_size - _get_lsb (gc_register) - 1;
			msb = 8 * gc_register->cache_size - _get_msb (gc_register) - 1;
		}

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
		if (msb - lsb < 63)
			mask = ((((guint64) 1) << (msb - lsb + 1)) - 1) << lsb;
		else
			mask = G_MAXUINT64;

		value = ((value << lsb) & mask) | (current_value & ~mask);
#else
		g_assert_not_reached ();
#endif
	}

	arv_log_genicam ("[GcRegister::set_integer_value] address = 0x%Lx, value = 0x%Lx",
			 arv_gc_register_get_address (gc_register),
			 value);

	arv_copy_memory_with_endianess (gc_register->cache, gc_register->cache_size, endianess,
					&value, sizeof (value), G_BYTE_ORDER);

	_write_cache (gc_register);
}

static void
arv_gc_register_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_register_get_integer_value;
	interface->set_value = arv_gc_register_set_integer_value;
}

static double
arv_gc_register_get_float_value (ArvGcFloat *gc_float)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (gc_float);
	guint endianess;

	endianess = _get_endianess (gc_register);

	_read_cache (gc_register);

	if (gc_register->cache_size == 4) {
		float v_float;
		arv_copy_memory_with_endianess (&v_float, sizeof (v_float), G_BYTE_ORDER,
						gc_register->cache, gc_register->cache_size, endianess);

		return v_float;
	} else if (gc_register->cache_size == 8) {
		double v_double;
		arv_copy_memory_with_endianess (&v_double, sizeof (v_double), G_BYTE_ORDER,
						gc_register->cache, gc_register->cache_size, endianess);

		return v_double;
	} else {
		arv_warning_genicam ("[GcFloatReg::get_value] Invalid register size");
		return 0.0;
	}
}

static void
arv_gc_register_set_float_value (ArvGcFloat *gc_float, double v_double)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (gc_float);
	guint endianess;

	endianess = _get_endianess (gc_register);

	_update_cache_size (gc_register);

	if (gc_register->cache_size == 4) {
		float v_float = v_double;
		arv_copy_memory_with_endianess (gc_register->cache, gc_register->cache_size, endianess,
						&v_float, sizeof (v_float), G_BYTE_ORDER);
	} else if (gc_register->cache_size == 8) {
		arv_copy_memory_with_endianess (gc_register->cache, gc_register->cache_size, endianess,
						&v_double, sizeof (v_double), G_BYTE_ORDER);
	} else {
		arv_warning_genicam ("[GcFloatReg::set_value] Invalid register size");
		return;
	}

	_write_cache (gc_register);
}

static void
arv_gc_register_float_interface_init (ArvGcFloatInterface *interface)
{
	interface->get_value = arv_gc_register_get_float_value;
	interface->set_value = arv_gc_register_set_float_value;
}

static const char *
arv_gc_register_get_string_value (ArvGcString *gc_string)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (gc_string);

	_read_cache (gc_register);

	if (gc_register->cache_size > 0)
		((char *) gc_register->cache)[gc_register->cache_size - 1] = '\0';

	return gc_register->cache;
}

static void
arv_gc_register_set_string_value (ArvGcString *gc_string, const char *value)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (gc_string);

	_update_cache_size (gc_register);

	if (gc_register->cache_size > 0) {
		strncpy (gc_register->cache, value, gc_register->cache_size);
		((char *) gc_register->cache)[gc_register->cache_size - 1] = '\0';

		_write_cache (gc_register);
	}
}

static gint64
arv_gc_register_get_max_string_length (ArvGcString *gc_string)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (gc_string);

	return arv_gc_register_get_length (gc_register);
}

static void
arv_gc_register_string_interface_init (ArvGcStringInterface *interface)
{
	interface->get_value = arv_gc_register_get_string_value;
	interface->set_value = arv_gc_register_set_string_value;
	interface->get_max_length = arv_gc_register_get_max_string_length;
}

G_DEFINE_TYPE_WITH_CODE (ArvGcRegister, arv_gc_register, ARV_TYPE_GC_FEATURE_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_register_integer_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_FLOAT, arv_gc_register_float_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_STRING, arv_gc_register_string_interface_init))
