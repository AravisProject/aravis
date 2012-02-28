/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
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
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcstring.h>
#include <arvgcport.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <arvdebug.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char *node;
	gint modification_count;
} ArvGcInvalidator;

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

/* ArvGcNode implementation */

static void
arv_gc_register_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (node);

	if (strcmp (name, "Address") == 0) {
		gc_register->addresses = g_slist_prepend (gc_register->addresses,
							  arv_create_int64_g_value (g_ascii_strtoull (content,
												   NULL, 0)));
	} else if (strcmp (name, "pAddress") == 0) {
		gc_register->addresses = g_slist_prepend (gc_register->addresses,
							  arv_create_string_g_value (content));
	} else if (strcmp (name, "pIndex") == 0) {
		int i;

		g_free (gc_register->index);
		gc_register->index = g_strdup (content);

		for (i = 0; attributes[i] != NULL && attributes[i+1] != NULL; i += 2)
			if (g_strcmp0 (attributes[i], "Offset") == 0) {
				arv_force_g_value_to_int64 (&gc_register->index_offset,
							    g_ascii_strtoull (attributes[i+1], NULL, 0));
				break;
			} else if (g_strcmp0 (attributes[i], "pOffset") == 0) {
				arv_force_g_value_to_string (&gc_register->index_offset, attributes[i+1]);
				break;
			}
	} else if (strcmp (name, "Length") == 0) {
		arv_force_g_value_to_int64 (&gc_register->length, g_ascii_strtoull (content, NULL, 0));
	} else if (strcmp (name, "pLength") == 0) {
		arv_force_g_value_to_string (&gc_register->length, content);
	} else if (strcmp (name, "AccessMode") == 0) {
		if (g_strcmp0 (content, "RW") == 0)
			gc_register->access_mode = ARV_GC_ACCESS_MODE_RW;
		else if (g_strcmp0 (content, "RO") == 0)
			gc_register->access_mode = ARV_GC_ACCESS_MODE_RO;
		else if (g_strcmp0 (content, "WO") == 0)
			gc_register->access_mode = ARV_GC_ACCESS_MODE_WO;
	} else if (strcmp (name, "Cachable") == 0) {
		if (g_strcmp0 (content, "NoCache") == 0)
			gc_register->cachable = ARV_GC_CACHABLE_NO_CACHE;
		else if (g_strcmp0 (content, "WriteAround") == 0)
			gc_register->cachable = ARV_GC_CACHABLE_WRITE_AROUND;
		else if (g_strcmp0 (content, "WriteThrough") == 0)
			gc_register->cachable = ARV_GC_CACHABLE_WRITE_TRHOUGH;
	} else if (strcmp (name, "pInvalidator") == 0) {
		ArvGcInvalidator *invalidator = g_new (ArvGcInvalidator, 1);

		invalidator->node = g_strdup (content);
		invalidator->modification_count = 0;

		gc_register->invalidators = g_slist_prepend (gc_register->invalidators, invalidator);
	} else if (strcmp (name, "pPort") == 0) {
		g_free (gc_register->port_name);
		gc_register->port_name = g_strdup (content);
	} else if (strcmp (name, "PollingTime") == 0) {
		gc_register->polling_time = g_ascii_strtoull (content, NULL, 0);
	} else if (strcmp (name, "Endianess") == 0) {
		if (g_strcmp0 (content, "BigEndian") == 0)
			gc_register->endianess = G_BIG_ENDIAN;
		else
			gc_register->endianess = G_LITTLE_ENDIAN;
	} else if (strcmp (name, "Sign") == 0) {
		if (g_strcmp0 (content, "Unsigned") == 0)
			gc_register->sign = ARV_GC_SIGN_UNSIGNED;
		else
			gc_register->sign = ARV_GC_SIGN_SIGNED;
	} else if (strcmp (name, "LSB") == 0) {
		gc_register->lsb = content != NULL ? atoi (content) : 0;
	} else if (strcmp (name, "MSB") == 0) {
		gc_register->msb = content != NULL ? atoi (content) : 0;
	} else if (strcmp (name, "Bit") == 0) {
		gc_register->msb = content != NULL ? atoi (content) : 0;
		gc_register->lsb = content != NULL ? atoi (content) : 0;
	} else
		ARV_GC_NODE_CLASS (parent_class)->add_element (node, name, content, attributes);
}

static GType
arv_gc_register_get_value_type (ArvGcNode *node)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (node);

	return gc_register->value_type;
}

static void
arv_gc_register_set_value_from_string (ArvGcNode *node, const char *string)
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
arv_gc_register_get_value_as_string (ArvGcNode *node)
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
	ArvGc *genicam;
	GSList *iter;
	gint modification_count;
	gboolean is_cache_valid = gc_register->is_cache_valid;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_register));

	for (iter = gc_register->invalidators; iter != NULL; iter = iter->next) {
		ArvGcInvalidator *invalidator = iter->data;

		modification_count = invalidator->modification_count;
		invalidator->modification_count = arv_gc_node_get_modification_count
			(arv_gc_get_node (genicam, invalidator->node));
		if (modification_count != invalidator->modification_count)
			is_cache_valid = FALSE;
	}

	return is_cache_valid;
}

static void
_update_cache_size (ArvGcRegister *gc_register, ArvGc *genicam)
{
	gint64 length;

	length = arv_gc_get_int64_from_value (genicam, &gc_register->length);
	if (length != gc_register->cache_size) {
		g_free (gc_register->cache);
		gc_register->cache = g_malloc (length);
		gc_register->cache_size = length;
	}

}

static void
_read_cache (ArvGcRegister *gc_register)
{
	ArvGc *genicam;
	ArvGcNode *port;

	if (gc_register->is_cache_valid == TRUE) {
		arv_log_genicam ("[GcRegister::read_cache] Cache is valid");
		return;
	}

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_register));
	g_return_if_fail (ARV_IS_GC (genicam));

	port = arv_gc_get_node (genicam, gc_register->port_name);
	if (!ARV_IS_GC_PORT (port))
		return;

	_update_cache_size (gc_register, genicam);

	arv_gc_port_read (ARV_GC_PORT (port),
			  gc_register->cache,
			  arv_gc_register_get_address (gc_register),
			  gc_register->cache_size);

	if (gc_register->cachable != ARV_GC_CACHABLE_NO_CACHE)
		gc_register->is_cache_valid = TRUE;
	else
		gc_register->is_cache_valid = FALSE;
}

static void
_write_cache (ArvGcRegister *gc_register)
{
	ArvGc *genicam;
	ArvGcNode *port;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_register));
	g_return_if_fail (ARV_IS_GC (genicam));

	arv_gc_node_inc_modification_count (ARV_GC_NODE (gc_register));

	port = arv_gc_get_node (genicam, gc_register->port_name);
	if (!ARV_IS_GC_PORT (port))
		return;

	_update_cache_size (gc_register, genicam);

	arv_gc_port_write (ARV_GC_PORT (port),
			   gc_register->cache,
			   arv_gc_register_get_address (gc_register),
			   gc_register->cache_size);

	if (gc_register->cachable == ARV_GC_CACHABLE_WRITE_TRHOUGH)
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
		value += arv_gc_get_int64_from_value (genicam, iter->data);

	if (gc_register->index != NULL) {
		ArvGcNode *node;

		node = arv_gc_get_node (genicam, gc_register->index);
		if (ARV_IS_GC_INTEGER (node)) {
			guint64 index;
			guint64 index_offset;

			index = arv_gc_integer_get_value (ARV_GC_INTEGER (node));

			if (index != 0) {
				if (G_VALUE_HOLDS_BOOLEAN (&gc_register->index_offset))
					index_offset = arv_gc_register_get_length (gc_register);
				else
					index_offset = arv_gc_get_int64_from_value (genicam,
										    &gc_register->index_offset);
				value += index * index_offset;
			}
		}
	}

	return value;
}

guint64
arv_gc_register_get_length (ArvGcRegister *gc_register)
{
	ArvGc *genicam;

	g_return_val_if_fail (ARV_IS_GC_REGISTER (gc_register), 0);
	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_register));
	g_return_val_if_fail (ARV_IS_GC (genicam), 0);

	return arv_gc_get_int64_from_value (genicam, &gc_register->length);
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
	/* Set default to read only 32 bits little endian integer register */
	g_value_init (&gc_register->index_offset, G_TYPE_BOOLEAN);
	g_value_set_boolean (&gc_register->index_offset, FALSE);
	g_value_init (&gc_register->length, G_TYPE_INT64);
	g_value_set_int64 (&gc_register->length, 4);
	gc_register->access_mode = ARV_GC_ACCESS_MODE_RO;
	gc_register->cachable = ARV_GC_CACHABLE_NO_CACHE;
	gc_register->cache = g_malloc0(4);
	gc_register->cache_size = 4;
	gc_register->endianess = G_LITTLE_ENDIAN;
	gc_register->msb = 31;
	gc_register->lsb = 0;
	gc_register->invalidators = NULL;
	gc_register->is_cache_valid = FALSE;
}

static void
arv_gc_register_finalize (GObject *object)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (object);
	GSList *iter;

	for (iter = gc_register->addresses; iter != NULL; iter = iter->next)
		arv_free_g_value (iter->data);
	g_slist_free (gc_register->addresses);
	g_value_unset (&gc_register->index_offset);
	g_value_unset (&gc_register->length);
	g_free (gc_register->port_name);
	g_free (gc_register->cache);

	for (iter = gc_register->invalidators; iter != NULL; iter = iter->next) {
		ArvGcInvalidator *invalidator = iter->data;

		g_free (invalidator->node);
		g_free (invalidator);
	}
	g_slist_free (gc_register->invalidators);

	parent_class->finalize (object);
}

static void
arv_gc_register_class_init (ArvGcRegisterClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcNodeClass *gc_node_class = ARV_GC_NODE_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_register_finalize;
	dom_node_class->get_node_name = arv_gc_register_get_node_name;
	gc_node_class->add_element = arv_gc_register_add_element;
	gc_node_class->get_value_type = arv_gc_register_get_value_type;
	gc_node_class->set_value_from_string = arv_gc_register_set_value_from_string;
	gc_node_class->get_value_as_string = arv_gc_register_get_value_as_string;
}

/* ArvGcInteger interface implementation */

static gint64
arv_gc_register_get_integer_value (ArvGcInteger *gc_integer)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (gc_integer);
	gint64 value;
	guint lsb;
	guint msb;

	_read_cache (gc_register);

	arv_copy_memory_with_endianess (&value, sizeof (value), G_BYTE_ORDER,
					gc_register->cache, gc_register->cache_size, gc_register->endianess);

	if (gc_register->type == ARV_GC_REGISTER_TYPE_MASKED_INTEGER) {
		guint64 mask;

		if (gc_register->endianess == G_BYTE_ORDER) {
			lsb = gc_register->lsb;
			msb = gc_register->msb;
		} else {
			lsb = 8 * gc_register->cache_size - gc_register->lsb - 1;
			msb = 8 * gc_register->cache_size - gc_register->msb - 1;
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

	if (gc_register->type == ARV_GC_REGISTER_TYPE_MASKED_INTEGER) {
		gint64 current_value;
		guint64 mask;

		_read_cache (gc_register);

		arv_copy_memory_with_endianess (&current_value, sizeof (current_value), G_BYTE_ORDER,
						gc_register->cache, gc_register->cache_size, gc_register->endianess);

		if (gc_register->endianess == G_BYTE_ORDER) {
			lsb = gc_register->lsb;
			msb = gc_register->msb;
		} else {
			lsb = 8 * gc_register->cache_size - gc_register->lsb - 1;
			msb = 8 * gc_register->cache_size - gc_register->msb - 1;
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

	arv_copy_memory_with_endianess (gc_register->cache, gc_register->cache_size, gc_register->endianess,
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

	_read_cache (gc_register);

	if (gc_register->cache_size == 4) {
		float v_float;
		arv_copy_memory_with_endianess (&v_float, sizeof (v_float), G_BYTE_ORDER,
						gc_register->cache, gc_register->cache_size, gc_register->endianess);

		return v_float;
	} else if (gc_register->cache_size == 8) {
		double v_double;
		arv_copy_memory_with_endianess (&v_double, sizeof (v_double), G_BYTE_ORDER,
						gc_register->cache, gc_register->cache_size, gc_register->endianess);

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
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_register));
	g_return_if_fail (ARV_IS_GC (genicam));

	_update_cache_size (gc_register, genicam);

	if (gc_register->cache_size == 4) {
		float v_float = v_double;
		arv_copy_memory_with_endianess (gc_register->cache, gc_register->cache_size, gc_register->endianess,
						&v_float, sizeof (v_float), G_BYTE_ORDER);
	} else if (gc_register->cache_size == 8) {
		arv_copy_memory_with_endianess (gc_register->cache, gc_register->cache_size, gc_register->endianess,
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
	ArvGc *genicam;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_register));
	g_return_if_fail (ARV_IS_GC (genicam));

	_update_cache_size (gc_register, genicam);

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

G_DEFINE_TYPE_WITH_CODE (ArvGcRegister, arv_gc_register, ARV_TYPE_GC_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_register_integer_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_FLOAT, arv_gc_register_float_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_STRING, arv_gc_register_string_interface_init))
