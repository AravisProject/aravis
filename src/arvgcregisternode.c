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
 * SECTION: arvgcregisternode
 * @short_description: Class for Register, IntReg, MaskedIntReg, FloatReg, StringReg and StructReg nodes
 */

#include <arvgcregisternode.h>
#include <arvgcindexnode.h>
#include <arvgcinvalidatornode.h>
#include <arvgcfeaturenodeprivate.h>
#include <arvgcswissknife.h>
#include <arvgcregister.h>
#include <arvgcinteger.h>
#include <arvgcselector.h>
#include <arvgcfloat.h>
#include <arvgcstring.h>
#include <arvgcport.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <arvdebug.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
	gint64 address;
	gint64 length;
} ArvGcCacheKey;

static gboolean
arv_gc_cache_key_equal (gconstpointer v1, gconstpointer v2)
{
	return ((((ArvGcCacheKey *) v1)->address == ((ArvGcCacheKey *) v2)->address) &&
		(((ArvGcCacheKey *) v1)->length == ((ArvGcCacheKey *) v2)->length));
}

static guint
arv_gc_cache_key_hash (gconstpointer v)
{
	return g_int64_hash (&((ArvGcCacheKey *) v)->address);
}

ArvGcCacheKey *
arv_gc_cache_key_new (gint64 address, gint64 length)
{
	ArvGcCacheKey *key =  g_new (ArvGcCacheKey, 1);

	key->address = address;
	key->length = length;

	return key;
}

static void arv_gc_register_node_register_interface_init (ArvGcRegisterInterface *interface);
static void arv_gc_register_node_integer_interface_init (ArvGcIntegerInterface *interface);
static void arv_gc_register_node_float_interface_init (ArvGcFloatInterface *interface);
static void arv_gc_register_node_string_interface_init (ArvGcStringInterface *interface);
static void arv_gc_register_node_selector_interface_init (ArvGcSelectorInterface *interface);

G_DEFINE_TYPE_WITH_CODE (ArvGcRegisterNode, arv_gc_register_node, ARV_TYPE_GC_FEATURE_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_REGISTER, arv_gc_register_node_register_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_register_node_integer_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_FLOAT, arv_gc_register_node_float_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_STRING, arv_gc_register_node_string_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_SELECTOR, arv_gc_register_node_selector_interface_init))

/* ArvDomNode implementation */

static const char *
arv_gc_register_node_get_node_name (ArvDomNode *node)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (node);

	switch (gc_register_node->type) {
		case ARV_GC_REGISTER_NODE_TYPE_REGISTER:
			return "Register";
		case ARV_GC_REGISTER_NODE_TYPE_INTEGER:
			return "IntReg";
		case ARV_GC_REGISTER_NODE_TYPE_MASKED_INTEGER:
			return "MaskedIntReg";
		case ARV_GC_REGISTER_NODE_TYPE_FLOAT:
			return "FloatReg";
		case ARV_GC_REGISTER_NODE_TYPE_STRING:
			return "StringReg";
		case ARV_GC_REGISTER_NODE_TYPE_STRUCT_REGISTER:
			return "StuctReg";
	}

	return NULL;
}

static void
arv_gc_register_node_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcRegisterNode *node = ARV_GC_REGISTER_NODE (self);

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
			case ARV_GC_PROPERTY_NODE_TYPE_P_SELECTED:
				node->selecteds = g_slist_prepend (node->selecteds, property_node);
				break;
			default:
				ARV_DOM_NODE_CLASS (arv_gc_register_node_parent_class)->post_new_child (self, child);
				break;
		}
	} else if (ARV_IS_GC_SWISS_KNIFE (child))
		node->swiss_knives = g_slist_prepend (node->swiss_knives, child);
	else
		ARV_DOM_NODE_CLASS (arv_gc_register_node_parent_class)->post_new_child (self, child);
}

static void
arv_gc_register_node_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcFeatureNode implementation */

static GType
arv_gc_register_node_get_value_type (ArvGcFeatureNode *node)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (node);

	return gc_register_node->value_type;
}

static void
arv_gc_register_node_set_value_from_string (ArvGcFeatureNode *node, const char *string, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (node);

	switch (gc_register_node->value_type) {
		case G_TYPE_INT64:
			arv_gc_integer_set_value (ARV_GC_INTEGER (node), g_ascii_strtoll (string, NULL, 0), error);
			break;
		case G_TYPE_DOUBLE:
			arv_gc_float_set_value (ARV_GC_FLOAT (node), g_ascii_strtod (string, NULL), error);
			break;
		case G_TYPE_STRING:
			arv_gc_string_set_value (ARV_GC_STRING (node), string, error);
			break;
		default:
			break;
	}

	arv_warning_genicam ("[GcRegisterNode::set_value_from_string] Invalid value type");
}

static const char *
arv_gc_register_node_get_value_as_string (ArvGcFeatureNode *node, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (node);
	GError *local_error = NULL;
	const char *string;

	switch (gc_register_node->value_type) {
		case G_TYPE_INT64:
			g_snprintf (gc_register_node->v_string, G_ASCII_DTOSTR_BUF_SIZE,
				    "0x%08" G_GINT64_MODIFIER "x",
				    arv_gc_integer_get_value (ARV_GC_INTEGER (node), &local_error));

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return NULL;
			}

			return gc_register_node->v_string;
		case G_TYPE_DOUBLE:
			g_ascii_dtostr (gc_register_node->v_string, G_ASCII_DTOSTR_BUF_SIZE,
					arv_gc_float_get_value (ARV_GC_FLOAT (node), &local_error));

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return NULL;
			}

			return gc_register_node->v_string;
		case G_TYPE_STRING:
			string = arv_gc_string_get_value (ARV_GC_STRING (node), &local_error);

			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return NULL;
			}

			return string;
		default:
			break;
	}

	arv_warning_genicam ("[GcRegisterNode::get_value_as_string] Invalid value type");

	return NULL;
}

/* ArvGcRegisterNode implementation */

static gint64
_get_length (ArvGcRegisterNode *gc_register_node, GError **error)
{
	if (gc_register_node->length == NULL)
		return 4;

	return arv_gc_property_node_get_int64 (gc_register_node->length, error);
}

static guint64
_get_address (ArvGcRegisterNode *gc_register_node, GError **error)
{
	ArvGc *genicam;
	GError *local_error = NULL;
	GSList *iter;
	guint64 value = 0;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_register_node));
	g_return_val_if_fail (ARV_IS_GC (genicam), 0);

	for (iter = gc_register_node->addresses; iter != NULL; iter = iter->next) {
		value += arv_gc_property_node_get_int64 (iter->data, &local_error);

		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return 0;
		}
	}

	for (iter = gc_register_node->swiss_knives; iter != NULL; iter = iter->next) {
		value += arv_gc_integer_get_value (iter->data, &local_error);

		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return 0;
		}
	}

	if (gc_register_node->index != NULL) {
		gint64 length;

		length = _get_length (gc_register_node, &local_error);

		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return 0;
		}

		value += arv_gc_index_node_get_index (ARV_GC_INDEX_NODE (gc_register_node->index), length, &local_error);

		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return 0;
		}
	}

	return value;
}

static ArvGcCachable
_get_cachable (ArvGcRegisterNode *gc_register_node, GError **error)
{
	GError *local_error = NULL;
	const char *cachable;

	if (gc_register_node->cachable == NULL)
		return gc_register_node->default_cachable;

	cachable = arv_gc_property_node_get_string (gc_register_node->cachable, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return ARV_GC_CACHABLE_NO_CACHE;
	}

	if (g_strcmp0 (cachable, "WriteThrough") == 0)
		return ARV_GC_CACHABLE_WRITE_THROUGH;
	else if (strcmp (cachable, "WriteAround") == 0)
		return ARV_GC_CACHABLE_WRITE_AROUND;

	return ARV_GC_CACHABLE_NO_CACHE;
}

/* Set default to read only 32 bits little endian integer register */

static guint
_get_endianess (ArvGcRegisterNode *gc_register_node, GError **error)
{
	GError *local_error = NULL;
	const char *endianess;

	if (gc_register_node->endianess == NULL)
		return G_LITTLE_ENDIAN;

	endianess = arv_gc_property_node_get_string (gc_register_node->endianess, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_LITTLE_ENDIAN;
	}

	if (g_strcmp0 (endianess, "BigEndian") == 0)
		return G_BIG_ENDIAN;

	return G_LITTLE_ENDIAN;
}

static ArvGcSignedness
_get_signedness (ArvGcRegisterNode *gc_register_node, GError **error)
{
	GError *local_error = NULL;
	const char *sign;

	if (gc_register_node->sign == NULL)
		return gc_register_node->type == ARV_GC_REGISTER_NODE_TYPE_REGISTER ?
			ARV_GC_SIGNEDNESS_SIGNED :
			ARV_GC_SIGNEDNESS_UNSIGNED;

	sign = arv_gc_property_node_get_string (gc_register_node->sign, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return ARV_GC_SIGNEDNESS_SIGNED;
	}

	if (g_strcmp0 (sign, "Unsigned") == 0)
		return ARV_GC_SIGNEDNESS_UNSIGNED;

	return ARV_GC_SIGNEDNESS_SIGNED;
}

static gint64
_get_lsb (ArvGcRegisterNode *gc_register_node, GError **error)
{
	if (gc_register_node->lsb == NULL)
		return 0;

	return arv_gc_property_node_get_int64 (gc_register_node->lsb, error);
}

static gint64
_get_msb (ArvGcRegisterNode *gc_register_node, GError **error)
{
	if (gc_register_node->msb == NULL)
		return 31;

	return arv_gc_property_node_get_int64 (gc_register_node->msb, error);
}

static gboolean
_get_cached (ArvGcRegisterNode *gc_register_node, ArvRegisterCachePolicy *cache_policy)
{
	ArvGc *genicam;
	GSList *iter;
	gboolean cached = gc_register_node->cached;

	*cache_policy = ARV_REGISTER_CACHE_POLICY_DISABLE;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_register_node));
	g_return_val_if_fail (ARV_IS_GC (genicam), FALSE);

	*cache_policy = arv_gc_get_register_cache_policy (genicam);

	if (*cache_policy == ARV_REGISTER_CACHE_POLICY_DISABLE)
		return FALSE;

	for (iter = gc_register_node->invalidators; iter != NULL; iter = iter->next) {
		if (arv_gc_invalidator_has_changed (iter->data))
			cached = FALSE;
	}

	if (cached)
		gc_register_node->n_cache_hits++;
	else
		gc_register_node->n_cache_misses++;

	return cached;
}

static void *
_get_cache (ArvGcRegisterNode *gc_register_node, gint64 *address, gint64 *length, GError **error)
{
	GError *local_error = NULL;
	ArvGcCacheKey key;
	void *cache;

	key.address = _get_address (gc_register_node, &local_error);
	if (local_error == NULL)
		key.length = _get_length (gc_register_node, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return NULL;
	}

	cache = g_hash_table_lookup (gc_register_node->caches, &key);

	if (cache == NULL) {
		cache = g_malloc0 (key.length);
		g_hash_table_replace (gc_register_node->caches, arv_gc_cache_key_new (key.address, key.length), cache);
	}

	if (address != NULL)
		*address = key.address;
	if (length != NULL)
		*length = key.length;

	return cache;
}

static void
_read_from_port (ArvGcRegisterNode *gc_register_node, gint64 address, gint64 length, void *buffer, GError **error)
{
	GError *local_error = NULL;
	ArvGcNode *port;
	ArvGcCachable cachable;
	ArvRegisterCachePolicy cache_policy;
	void *cache = NULL;
	gboolean cached;

	cached = _get_cached (gc_register_node, &cache_policy);

	port = arv_gc_property_node_get_linked_node (gc_register_node->port);
	if (!ARV_IS_GC_PORT (port)) {
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_NODE_NOT_FOUND,
			     "Port not found for node '%s'",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_register_node)));
		gc_register_node->cached = FALSE;
		return;
	}

	if (cached && cache_policy == ARV_REGISTER_CACHE_POLICY_DEBUG) {
		cache = g_malloc (length);
		memcpy (cache, buffer, length);
	}

	if (!cached || cache_policy == ARV_REGISTER_CACHE_POLICY_DEBUG)
		arv_gc_port_read (ARV_GC_PORT (port), buffer, address, length, &local_error);

	if (local_error == NULL)
		cachable = _get_cachable (gc_register_node, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		gc_register_node->cached = FALSE;
		g_free (cache);
		return;
	}

	if (cached && cache_policy == ARV_REGISTER_CACHE_POLICY_DEBUG) {
		if (memcmp (cache, buffer, length) != 0)
			printf ("Incorrect cache value for %s\n",
				arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_register_node)));
		g_free (cache);
	}

	if (cachable != ARV_GC_CACHABLE_NO_CACHE)
		gc_register_node->cached = TRUE;
	else
		gc_register_node->cached = FALSE;
}

static void
_write_to_port (ArvGcRegisterNode *gc_register_node, gint64 address, gint64 length, void *buffer, GError **error)
{
	GError *local_error = NULL;
	ArvGcNode *port;
	ArvGcCachable cachable;

	port = arv_gc_property_node_get_linked_node (gc_register_node->port);
	if (!ARV_IS_GC_PORT (port)) {
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_NODE_NOT_FOUND,
			     "Port not found for node '%s'",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_register_node)));
		gc_register_node->cached = FALSE;
		return;
	}

	arv_gc_feature_node_increment_change_count (ARV_GC_FEATURE_NODE (gc_register_node));
	arv_gc_port_write (ARV_GC_PORT (port), buffer, address, length, &local_error);

	if (local_error == NULL)
		cachable = _get_cachable (gc_register_node, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		gc_register_node->cached = FALSE;
		return;
	}

	if (cachable == ARV_GC_CACHABLE_WRITE_THROUGH)
		gc_register_node->cached = TRUE;
	else
		gc_register_node->cached = FALSE;
}

ArvGcNode *
arv_gc_register_node_new (void)
{
	ArvGcRegisterNode *gc_register_node;

	gc_register_node = g_object_new (ARV_TYPE_GC_REGISTER_NODE, NULL);
	gc_register_node->type = ARV_GC_REGISTER_NODE_TYPE_REGISTER;
	gc_register_node->value_type = G_TYPE_BYTE_ARRAY;
	gc_register_node->default_cachable = ARV_GC_CACHABLE_WRITE_THROUGH;

	return ARV_GC_NODE (gc_register_node);
}

ArvGcNode *
arv_gc_register_node_new_integer (void)
{
	ArvGcRegisterNode *gc_register_node;

	gc_register_node = g_object_new (ARV_TYPE_GC_REGISTER_NODE, NULL);
	gc_register_node->type = ARV_GC_REGISTER_NODE_TYPE_INTEGER;
	gc_register_node->value_type = G_TYPE_INT64;
	gc_register_node->default_cachable = ARV_GC_CACHABLE_WRITE_THROUGH;

	return ARV_GC_NODE (gc_register_node);
}

ArvGcNode *
arv_gc_register_node_new_masked_integer (void)
{
	ArvGcRegisterNode *gc_register_node;

	gc_register_node = g_object_new (ARV_TYPE_GC_REGISTER_NODE, NULL);
	gc_register_node->type = ARV_GC_REGISTER_NODE_TYPE_MASKED_INTEGER;
	gc_register_node->value_type = G_TYPE_INT64;
	gc_register_node->default_cachable = ARV_GC_CACHABLE_WRITE_THROUGH;

	return ARV_GC_NODE (gc_register_node);
}

ArvGcNode *
arv_gc_register_node_new_float (void)
{
	ArvGcRegisterNode *gc_register_node;

	gc_register_node = g_object_new (ARV_TYPE_GC_REGISTER_NODE, NULL);
	gc_register_node->type = ARV_GC_REGISTER_NODE_TYPE_FLOAT;
	gc_register_node->value_type = G_TYPE_DOUBLE;
	gc_register_node->default_cachable = ARV_GC_CACHABLE_WRITE_AROUND;

	return ARV_GC_NODE (gc_register_node);
}

ArvGcNode *
arv_gc_register_node_new_string (void)
{
	ArvGcRegisterNode *gc_register_node;

	gc_register_node = g_object_new (ARV_TYPE_GC_REGISTER_NODE, NULL);
	gc_register_node->type = ARV_GC_REGISTER_NODE_TYPE_STRING;
	gc_register_node->value_type = G_TYPE_STRING;
	gc_register_node->default_cachable = ARV_GC_CACHABLE_WRITE_THROUGH;

	return ARV_GC_NODE (gc_register_node);
}

ArvGcNode *
arv_gc_register_node_new_struct_register (void)
{
	ArvGcRegisterNode *gc_register_node;

	gc_register_node = g_object_new (ARV_TYPE_GC_REGISTER_NODE, NULL);
	gc_register_node->type = ARV_GC_REGISTER_NODE_TYPE_STRUCT_REGISTER;
	gc_register_node->value_type = G_TYPE_INT64;
	gc_register_node->default_cachable = ARV_GC_CACHABLE_NO_CACHE; /* FIXME: <Cachable> is child of <StructEntry> */

	return ARV_GC_NODE (gc_register_node);
}

static void
arv_gc_register_node_init (ArvGcRegisterNode *gc_register_node)
{
	gc_register_node->cached = FALSE;
	gc_register_node->caches = g_hash_table_new_full (arv_gc_cache_key_hash, arv_gc_cache_key_equal, g_free, g_free);
	gc_register_node->n_cache_hits = 0;
	gc_register_node->n_cache_misses = 0;
	gc_register_node->default_cachable = ARV_GC_CACHABLE_NO_CACHE;
}

static void
arv_gc_register_node_finalize (GObject *object)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (object);

	g_slist_free (gc_register_node->addresses);
	g_slist_free (gc_register_node->swiss_knives);
	g_slist_free (gc_register_node->invalidators);
	g_slist_free (gc_register_node->selecteds);
	g_slist_free (gc_register_node->selected_features);
	g_clear_pointer (&gc_register_node->caches, g_hash_table_unref);

	if (gc_register_node->n_cache_hits > 0 || gc_register_node->n_cache_misses > 0) {
		const char *name = arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_register_node));

		if (name == NULL)
			name = arv_dom_node_get_node_name (ARV_DOM_NODE (gc_register_node));

		arv_debug_genicam ("Cache hits = %u / %u for %s",
				   gc_register_node->n_cache_hits,
				   gc_register_node->n_cache_hits + gc_register_node->n_cache_misses,
				   name);
	}

	G_OBJECT_CLASS (arv_gc_register_node_parent_class)->finalize (object);
}

static void
arv_gc_register_node_class_init (ArvGcRegisterNodeClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);

	object_class->finalize = arv_gc_register_node_finalize;
	dom_node_class->get_node_name = arv_gc_register_node_get_node_name;
	dom_node_class->post_new_child = arv_gc_register_node_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_register_node_pre_remove_child;
	gc_feature_node_class->get_value_type = arv_gc_register_node_get_value_type;
	gc_feature_node_class->set_value_from_string = arv_gc_register_node_set_value_from_string;
	gc_feature_node_class->get_value_as_string = arv_gc_register_node_get_value_as_string;
}

/* ArvGcRegister interface implementation */

static void
arv_gc_register_node_get (ArvGcRegister *gc_register, void *buffer, guint64 length, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (gc_register);
	GError *local_error = NULL;
	void *cache;
	gint64 address;
	gint64 cache_length;

	cache = _get_cache (gc_register_node, &address, &cache_length, &local_error);
	if (local_error == NULL)
		_read_from_port (gc_register_node, address, cache_length, cache, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	if (length > cache_length) {
		memcpy (buffer, cache, cache_length);
		memset (((char *) buffer) + cache_length, 0, length - cache_length);
	} else
		memcpy (buffer, cache, length);

	arv_log_genicam ("[GcRegisterNode::get] 0x%Lx,%Ld", address, length);
}

static void
arv_gc_register_node_set (ArvGcRegister *gc_register, void *buffer, guint64 length, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (gc_register);
	GError *local_error = NULL;
	void *cache;
	gint64 address;
	gint64 cache_length;

	cache = _get_cache (gc_register_node, &address, &cache_length, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	if (cache_length > length) {
		memcpy (cache, buffer, length);
		memset (((char *) cache) + length, 0, cache_length - length);
	} else
		memcpy (cache, buffer, cache_length);

	_write_to_port (gc_register_node, address, cache_length, cache, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	arv_log_genicam ("[GcRegisterNode::set] 0x%Lx,%Ld", address, length);
}

static guint64
arv_gc_register_node_get_address (ArvGcRegister *gc_register, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (gc_register);

	return _get_address (gc_register_node, error);
}

static guint64
arv_gc_register_node_get_length (ArvGcRegister *gc_register, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (gc_register);

	return _get_length (gc_register_node, error);
}

static void
arv_gc_register_node_register_interface_init (ArvGcRegisterInterface *interface)
{
	interface->get = arv_gc_register_node_get;
	interface->set = arv_gc_register_node_set;
	interface->get_address = arv_gc_register_node_get_address;
	interface->get_length = arv_gc_register_node_get_length;
}

/* ArvGcInteger interface implementation */

static gint64
_get_integer_value (ArvGcRegisterNode *gc_register_node,
		    guint register_lsb, guint register_msb, ArvGcSignedness signedness,
		    GError **error)
{
	GError *local_error = NULL;
	gint64 value;
	guint lsb;
	guint msb;
	guint endianess;
	void *cache;
	gint64 address;
	gint64 length;

	endianess = _get_endianess (gc_register_node, &local_error);
	if (local_error == NULL)
		cache = _get_cache (gc_register_node, &address, &length, &local_error);
	if (local_error == NULL)
		_read_from_port (gc_register_node, address, length, cache, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	arv_copy_memory_with_endianess (&value, sizeof (value), G_BYTE_ORDER,
					cache, length, endianess);

	if (gc_register_node->type == ARV_GC_REGISTER_NODE_TYPE_MASKED_INTEGER ||
	    gc_register_node->type == ARV_GC_REGISTER_NODE_TYPE_STRUCT_REGISTER) {
		guint64 mask;

		if (endianess == G_LITTLE_ENDIAN) {
			msb = register_msb;
			lsb = register_lsb;
		} else {
			lsb = 8 * length - register_lsb - 1;
			msb = 8 * length - register_msb - 1;
		}

		arv_log_genicam ("[GcRegisterNode::_get_integer_value] reglsb = %d, regmsb, %d, lsb = %d, msb = %d",
				 register_lsb, register_msb, lsb, msb);
		arv_log_genicam ("[GcRegisterNode::_get_integer_value] value = 0x%08Lx", value);

		if (msb - lsb < 63)
			mask = ((((guint64) 1) << (msb - lsb + 1)) - 1) << lsb;
		else
			mask = G_MAXUINT64;

		value = (value & mask) >> lsb;

		if (msb-lsb < 63 &&
		    (value & (((guint64) 1) << (msb - lsb))) != 0 &&
		    signedness == ARV_GC_SIGNEDNESS_SIGNED) {
			value |= G_MAXUINT64 ^ (mask >> lsb);
		}

		arv_log_genicam ("[GcRegisterNode::_get_integer_value] mask  = 0x%08Lx", mask);
	} else {
		if (length < 8 &&
		    ((value & (((guint64) 1) << (length * 8 - 1))) != 0) &&
		    signedness == ARV_GC_SIGNEDNESS_SIGNED)
			value |= G_MAXUINT64 ^ ((((guint64) 1) << (length * 8)) - 1);
	}

	arv_log_genicam ("[GcRegisterNode::_get_integer_value] address = 0x%Lx, value = 0x%Lx",
			 _get_address (gc_register_node, NULL), value);

	return value;
}

gint64
arv_gc_register_node_get_masked_integer_value (ArvGcRegisterNode *gc_register_node,
					       guint lsb, guint msb, ArvGcSignedness signedness,
					       GError **error)
{
	g_return_val_if_fail (ARV_IS_GC_REGISTER_NODE (gc_register_node), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	return _get_integer_value (gc_register_node, lsb, msb, signedness, error);
}

static gint64
arv_gc_register_node_get_integer_value (ArvGcInteger *gc_integer, GError **error)
{
	GError *local_error = NULL;
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (gc_integer);
	ArvGcSignedness signedness;
	gint64 lsb, msb;

	lsb = _get_lsb (gc_register_node, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	msb = _get_msb (gc_register_node, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	signedness = _get_signedness (gc_register_node, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	return _get_integer_value (gc_register_node, lsb, msb, signedness, error);
}

static void
_set_integer_value (ArvGcRegisterNode *gc_register_node, guint register_lsb, guint register_msb, gint64 value, GError **error)
{
	GError *local_error = NULL;
	guint lsb;
	guint msb;
	guint endianess;
	void *cache;
	gint64 address;
	gint64 length;

	endianess = _get_endianess (gc_register_node, &local_error);
	if (local_error == NULL)
		cache = _get_cache (gc_register_node, &address, &length, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	if (gc_register_node->type == ARV_GC_REGISTER_NODE_TYPE_MASKED_INTEGER ||
	    gc_register_node->type == ARV_GC_REGISTER_NODE_TYPE_STRUCT_REGISTER) {
		gint64 current_value;
		guint64 mask;

		_read_from_port (gc_register_node, address, length, cache, &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return;
		}

		arv_copy_memory_with_endianess (&current_value, sizeof (current_value), G_BYTE_ORDER,
						cache, length, endianess);

		if (endianess == G_LITTLE_ENDIAN) {
			msb = register_msb;
			lsb = register_lsb;
		} else {
			lsb = 8 * length - register_lsb - 1;
			msb = 8 * length - register_msb - 1;
		}

		arv_log_genicam ("[GcRegisterNode::_set_integer_value] reglsb = %d, regmsb, %d, lsb = %d, msb = %d",
				 register_lsb, register_msb, lsb, msb);
		arv_log_genicam ("[GcRegisterNode::_set_integer_value] value = 0x%08Lx", value);

		if (msb - lsb < 63)
			mask = ((((guint64) 1) << (msb - lsb + 1)) - 1) << lsb;
		else
			mask = G_MAXUINT64;

		value = ((value << lsb) & mask) | (current_value & ~mask);

		arv_log_genicam ("[GcRegisterNode::_set_integer_value] mask  = 0x%08Lx", mask);
	}

	arv_log_genicam ("[GcRegisterNode::_set_integer_value] address = 0x%Lx, value = 0x%Lx",
			 _get_address (gc_register_node, NULL), value);

	arv_copy_memory_with_endianess (cache, length, endianess,
					&value, sizeof (value), G_BYTE_ORDER);

	_write_to_port (gc_register_node, address, length, cache, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

}

void
arv_gc_register_node_set_masked_integer_value (ArvGcRegisterNode *gc_register_node, guint lsb, guint msb, gint64 value, GError **error)
{
	g_return_if_fail (ARV_IS_GC_REGISTER_NODE (gc_register_node));
	g_return_if_fail (error == NULL || *error == NULL);

	_set_integer_value (gc_register_node, lsb, msb, value, error);
}

static void
arv_gc_register_node_set_integer_value (ArvGcInteger *gc_integer, gint64 value, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (gc_integer);
	GError *local_error = NULL;
	gint64 lsb, msb;

	lsb = _get_lsb (gc_register_node, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	msb = _get_msb (gc_register_node, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	_set_integer_value (gc_register_node, lsb, msb, value, error);
}

static gint64
arv_gc_register_node_get_min (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (gc_integer);
	GError *local_error = NULL;
	gint64 lsb, msb;
	ArvGcSignedness signedness;

	if (gc_register_node->type != ARV_GC_REGISTER_NODE_TYPE_MASKED_INTEGER &&
	    gc_register_node->type != ARV_GC_REGISTER_NODE_TYPE_STRUCT_REGISTER)
		return G_MININT64;

	lsb = _get_lsb (gc_register_node, &local_error);
	if (local_error == NULL)
		msb = _get_msb (gc_register_node, &local_error);
	if (local_error == NULL)
		signedness = _get_signedness (gc_register_node, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_MININT64;
	}

	if (signedness == ARV_GC_SIGNEDNESS_SIGNED) {
		return -((1 << (msb - lsb  - 1)));
	} else {
		return 0;
	}
}

static gint64
arv_gc_register_node_get_max (ArvGcInteger *gc_integer, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (gc_integer);
	GError *local_error = NULL;
	gint64 lsb, msb;
	ArvGcSignedness signedness;

	if (gc_register_node->type != ARV_GC_REGISTER_NODE_TYPE_MASKED_INTEGER &&
	    gc_register_node->type != ARV_GC_REGISTER_NODE_TYPE_STRUCT_REGISTER)
		return G_MAXINT64;

	lsb = _get_lsb (gc_register_node, &local_error);
	if (local_error == NULL)
		msb = _get_msb (gc_register_node, &local_error);
	if (local_error == NULL)
		signedness = _get_signedness (gc_register_node, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return G_MAXINT64;
	}

	if (signedness == ARV_GC_SIGNEDNESS_SIGNED) {
		return ((1 << (msb - lsb  - 1)) - 1);
	} else {
		return (1 << (msb - lsb)) - 1;
	}
}

static gint64
arv_gc_register_node_get_inc (ArvGcInteger *gc_integer, GError **error)
{
	return 1;
}

static void
arv_gc_register_node_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_register_node_get_integer_value;
	interface->set_value = arv_gc_register_node_set_integer_value;
	interface->get_min = arv_gc_register_node_get_min;
	interface->get_max = arv_gc_register_node_get_max;
	interface->get_inc = arv_gc_register_node_get_inc;
}

static double
arv_gc_register_node_get_float_value (ArvGcFloat *gc_float, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (gc_float);
	GError *local_error = NULL;
	guint endianess;
	void *cache;
	gint64 address;
	gint64 length;

	endianess = _get_endianess (gc_register_node, &local_error);
	if (local_error == NULL)
		cache = _get_cache (gc_register_node, &address, &length, &local_error);
	if (local_error == NULL)
		_read_from_port (gc_register_node, address, length, cache, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0.0;
	}

	if (length == 4) {
		float v_float;
		arv_copy_memory_with_endianess (&v_float, sizeof (v_float), G_BYTE_ORDER,
						cache, length, endianess);

		return v_float;
	} else if (length == 8) {
		double v_double;
		arv_copy_memory_with_endianess (&v_double, sizeof (v_double), G_BYTE_ORDER,
						cache, length, endianess);

		return v_double;
	} else {
		arv_warning_genicam ("[GcFloatReg::get_value] Invalid register size");
		return 0.0;
	}
}

static void
arv_gc_register_node_set_float_value (ArvGcFloat *gc_float, double v_double, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (gc_float);
	GError *local_error = NULL;
	guint endianess;
	void *cache;
	gint64 address;
	gint64 length;

	endianess = _get_endianess (gc_register_node, &local_error);
	if (local_error == NULL)
		cache = _get_cache (gc_register_node, &address, &length, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	if (length == 4) {
		float v_float = v_double;
		arv_copy_memory_with_endianess (cache, length, endianess,
						&v_float, sizeof (v_float), G_BYTE_ORDER);
	} else if (length == 8) {
		arv_copy_memory_with_endianess (cache, length, endianess,
						&v_double, sizeof (v_double), G_BYTE_ORDER);
	} else {
		arv_warning_genicam ("[GcFloatReg::set_value] Invalid register size");
		return;
	}

	_write_to_port (gc_register_node, address, length, cache, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}
}

static void
arv_gc_register_node_float_interface_init (ArvGcFloatInterface *interface)
{
	interface->get_value = arv_gc_register_node_get_float_value;
	interface->set_value = arv_gc_register_node_set_float_value;
}

static const char *
arv_gc_register_node_get_string_value (ArvGcString *gc_string, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (gc_string);
	GError *local_error = NULL;
	void *cache;
	gint64 address;
	gint64 length;

	cache = _get_cache (gc_register_node, &address, &length, &local_error);
	if (local_error == NULL)
		_read_from_port (gc_register_node, address, length, cache, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return NULL;
	}

	if (length > 0)
		((char *) cache)[length - 1] = '\0';

	return cache;
}

static void
arv_gc_register_node_set_string_value (ArvGcString *gc_string, const char *value, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (gc_string);
	GError *local_error = NULL;
	void *cache;
	gint64 address;
	gint64 length;

	cache = _get_cache (gc_register_node, &address, &length, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	if (length > 0) {
		strncpy (cache, value, length);
		((char *) cache)[length - 1] = '\0';

		_write_to_port (gc_register_node, address, length, cache, &local_error);
		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return;
		}
	}
}

static gint64
arv_gc_register_node_get_max_string_length (ArvGcString *gc_string, GError **error)
{
	ArvGcRegisterNode *gc_register_node = ARV_GC_REGISTER_NODE (gc_string);

	return _get_length (gc_register_node, error);
}

static void
arv_gc_register_node_string_interface_init (ArvGcStringInterface *interface)
{
	interface->get_value = arv_gc_register_node_get_string_value;
	interface->set_value = arv_gc_register_node_set_string_value;
	interface->get_max_length = arv_gc_register_node_get_max_string_length;
}

const GSList *
arv_gc_register_node_get_selected_features (ArvGcSelector *selector)
{
	ArvGcRegisterNode *register_node = ARV_GC_REGISTER_NODE (selector);

	if (register_node->type == ARV_GC_REGISTER_NODE_TYPE_INTEGER ||
	    register_node->type == ARV_GC_REGISTER_NODE_TYPE_MASKED_INTEGER) {
		GSList *iter;

		g_clear_pointer (&register_node->selected_features, g_slist_free);
		for (iter = register_node->selecteds; iter != NULL; iter = iter->next) {
			ArvGcFeatureNode *feature_node = ARV_GC_FEATURE_NODE (arv_gc_property_node_get_linked_node (iter->data));
			if (ARV_IS_GC_FEATURE_NODE (feature_node))
				register_node->selected_features = g_slist_prepend (register_node->selected_features, feature_node);
		}

		return register_node->selected_features;
	}

	return NULL;
}

static void
arv_gc_register_node_selector_interface_init (ArvGcSelectorInterface *interface)
{
	interface->get_selected_features = arv_gc_register_node_get_selected_features;
}
