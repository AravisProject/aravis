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
 * @short_description: Class for Register nodes
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

typedef struct {
	GSList *addresses;
	GSList *swiss_knives;
	ArvGcPropertyNode *index;
	ArvGcPropertyNode *length;
	ArvGcPropertyNode *port;
	ArvGcPropertyNode *cachable;
	ArvGcPropertyNode *polling_time;
	ArvGcPropertyNode *access_mode;
	ArvGcPropertyNode *endianness;

	GSList *invalidators;		/* #ArvGcPropertyNode */

	gboolean cached;
	GHashTable *caches;
	guint n_cache_hits;
	guint n_cache_misses;

	char v_string[G_ASCII_DTOSTR_BUF_SIZE];
} ArvGcRegisterNodePrivate;

static void arv_gc_register_node_register_interface_init (ArvGcRegisterInterface *interface);

G_DEFINE_TYPE_WITH_CODE (ArvGcRegisterNode, arv_gc_register_node, ARV_TYPE_GC_FEATURE_NODE,
			 G_ADD_PRIVATE (ArvGcRegisterNode)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_REGISTER, arv_gc_register_node_register_interface_init))

/* ArvDomNode implementation */

static const char *
arv_gc_register_node_get_node_name (ArvDomNode *node)
{
			return "Register";
}

static void
arv_gc_register_node_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcRegisterNodePrivate *priv = arv_gc_register_node_get_instance_private (ARV_GC_REGISTER_NODE (self));

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_ADDRESS:
			case ARV_GC_PROPERTY_NODE_TYPE_P_ADDRESS:
				priv->addresses = g_slist_prepend (priv->addresses, child);
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_INDEX:
				priv->index = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_LENGTH:
			case ARV_GC_PROPERTY_NODE_TYPE_P_LENGTH:
				priv->length = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_PORT:
				priv->port = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_CACHABLE:
				priv->cachable = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_POLLING_TIME:
				/* TODO */
				priv->polling_time = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_ENDIANNESS:
				priv->endianness = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_ACCESS_MODE:
				priv->access_mode = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_P_INVALIDATOR:
				priv->invalidators = g_slist_prepend (priv->invalidators, property_node);
				break;
			default:
				ARV_DOM_NODE_CLASS (arv_gc_register_node_parent_class)->post_new_child (self, child);
				break;
		}
	} else if (ARV_IS_GC_SWISS_KNIFE (child))
		priv->swiss_knives = g_slist_prepend (priv->swiss_knives, child);
	else
		ARV_DOM_NODE_CLASS (arv_gc_register_node_parent_class)->post_new_child (self, child);
}

static void
arv_gc_register_node_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcRegisterNode implementation */

static gint64
_get_length (ArvGcRegisterNode *self, GError **error)
{
	ArvGcRegisterNodePrivate *priv = arv_gc_register_node_get_instance_private (ARV_GC_REGISTER_NODE (self));

	if (priv->length == NULL)
		return 4;

	return arv_gc_property_node_get_int64 (priv->length, error);
}

static guint64
_get_address (ArvGcRegisterNode *self, GError **error)
{
	ArvGcRegisterNodePrivate *priv = arv_gc_register_node_get_instance_private (ARV_GC_REGISTER_NODE (self));
	ArvGc *genicam;
	GError *local_error = NULL;
	GSList *iter;
	guint64 value = 0;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (self));
	g_return_val_if_fail (ARV_IS_GC (genicam), 0);

	for (iter = priv->addresses; iter != NULL; iter = iter->next) {
		value += arv_gc_property_node_get_int64 (iter->data, &local_error);

		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return 0;
		}
	}

	for (iter = priv->swiss_knives; iter != NULL; iter = iter->next) {
		value += arv_gc_integer_get_value (iter->data, &local_error);

		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return 0;
		}
	}

	if (priv->index != NULL) {
		gint64 length;

		length = _get_length (self, &local_error);

		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return 0;
		}

		value += arv_gc_index_node_get_index (ARV_GC_INDEX_NODE (priv->index), length, &local_error);

		if (local_error != NULL) {
			g_propagate_error (error, local_error);
			return 0;
		}
	}

	return value;
}

static ArvGcCachable
_get_cachable (ArvGcRegisterNode *self)
{
	ArvGcRegisterNodePrivate *priv = arv_gc_register_node_get_instance_private (ARV_GC_REGISTER_NODE (self));

	return arv_gc_property_node_get_cachable (priv->cachable, ARV_GC_REGISTER_NODE_GET_CLASS (self)->default_cachable);
}

static ArvGcCachable
_get_endianness (ArvGcRegisterNode *self)
{
	ArvGcRegisterNodePrivate *priv = arv_gc_register_node_get_instance_private (ARV_GC_REGISTER_NODE (self));

	return arv_gc_property_node_get_endianness (priv->endianness, G_LITTLE_ENDIAN);
}

static gboolean
_get_cached (ArvGcRegisterNode *self, ArvRegisterCachePolicy *cache_policy)
{
	ArvGcRegisterNodePrivate *priv = arv_gc_register_node_get_instance_private (ARV_GC_REGISTER_NODE (self));
	ArvGc *genicam;
	GSList *iter;
	gboolean cached = priv->cached;

	*cache_policy = ARV_REGISTER_CACHE_POLICY_DISABLE;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (self));
	g_return_val_if_fail (ARV_IS_GC (genicam), FALSE);

	*cache_policy = arv_gc_get_register_cache_policy (genicam);

	if (*cache_policy == ARV_REGISTER_CACHE_POLICY_DISABLE)
		return FALSE;

	for (iter = priv->invalidators; iter != NULL; iter = iter->next) {
		if (arv_gc_invalidator_has_changed (iter->data))
			cached = FALSE;
	}

	if (cached)
		priv->n_cache_hits++;
	else
		priv->n_cache_misses++;

	return cached;
}

static void *
_get_cache (ArvGcRegisterNode *self, gint64 *address, gint64 *length, GError **error)
{
	ArvGcRegisterNodePrivate *priv = arv_gc_register_node_get_instance_private (ARV_GC_REGISTER_NODE (self));
	GError *local_error = NULL;
	ArvGcCacheKey key;
	void *cache;

	key.address = _get_address (self, &local_error);
	if (local_error == NULL)
		key.length = _get_length (self, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return NULL;
	}

	cache = g_hash_table_lookup (priv->caches, &key);

	if (cache == NULL) {
		cache = g_malloc0 (key.length);
		g_hash_table_replace (priv->caches, arv_gc_cache_key_new (key.address, key.length), cache);
	}

	if (address != NULL)
		*address = key.address;
	if (length != NULL)
		*length = key.length;

	return cache;
}

static ArvGcAccessMode
arv_gc_register_node_get_access_mode (ArvGcFeatureNode *gc_feature_node)
{
	ArvGcRegisterNodePrivate *priv = arv_gc_register_node_get_instance_private (ARV_GC_REGISTER_NODE (gc_feature_node));

	if (priv->access_mode == NULL)
		return ARV_GC_ACCESS_MODE_RO;

	return arv_gc_property_node_get_access_mode (priv->access_mode, ARV_GC_ACCESS_MODE_RO);
}

static void
_read_from_port (ArvGcRegisterNode *self, gint64 address, gint64 length, void *buffer, ArvGcCachable cachable, GError **error)
{
	ArvGcRegisterNodePrivate *priv = arv_gc_register_node_get_instance_private (ARV_GC_REGISTER_NODE (self));
	GError *local_error = NULL;
	ArvGcNode *port;
	ArvRegisterCachePolicy cache_policy;
	void *cache = NULL;
	gboolean cached;

	cached = _get_cached (self, &cache_policy);

	port = arv_gc_property_node_get_linked_node (priv->port);
	if (!ARV_IS_GC_PORT (port)) {
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_NODE_NOT_FOUND,
			     "Port not found for node '%s'",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (self)));
		priv->cached = FALSE;
		return;
	}

	if (cached && cache_policy == ARV_REGISTER_CACHE_POLICY_DEBUG) {
		cache = g_malloc (length);
		memcpy (cache, buffer, length);
	}

	if (!cached || cache_policy == ARV_REGISTER_CACHE_POLICY_DEBUG)
		arv_gc_port_read (ARV_GC_PORT (port), buffer, address, length, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		priv->cached = FALSE;
		g_free (cache);
		return;
	}

	if (cached && cache_policy == ARV_REGISTER_CACHE_POLICY_DEBUG) {
		if (memcmp (cache, buffer, length) != 0)
			printf ("Incorrect cache value for %s\n",
				arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (self)));
		g_free (cache);
	}

	if (cachable != ARV_GC_CACHABLE_NO_CACHE)
		priv->cached = TRUE;
	else
		priv->cached = FALSE;
}

static void
_write_to_port (ArvGcRegisterNode *self, gint64 address, gint64 length, void *buffer, ArvGcCachable cachable, GError **error)
{
	ArvGcRegisterNodePrivate *priv = arv_gc_register_node_get_instance_private (ARV_GC_REGISTER_NODE (self));
	GError *local_error = NULL;
	ArvGcNode *port;

	port = arv_gc_property_node_get_linked_node (priv->port);
	if (!ARV_IS_GC_PORT (port)) {
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_NODE_NOT_FOUND,
			     "Port not found for node '%s'",
			     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (self)));
		priv->cached = FALSE;
		return;
	}

	arv_gc_feature_node_increment_change_count (ARV_GC_FEATURE_NODE (self));
	arv_gc_port_write (ARV_GC_PORT (port), buffer, address, length, &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		priv->cached = FALSE;
		return;
	}

	if (cachable == ARV_GC_CACHABLE_WRITE_THROUGH)
		priv->cached = TRUE;
	else
		priv->cached = FALSE;
}

ArvGcNode *
arv_gc_register_node_new (void)
{
	return g_object_new (ARV_TYPE_GC_REGISTER_NODE, NULL);
}

static void
arv_gc_register_node_init (ArvGcRegisterNode *self)
{
	ArvGcRegisterNodePrivate *priv = arv_gc_register_node_get_instance_private (ARV_GC_REGISTER_NODE (self));

	priv->cached = FALSE;
	priv->caches = g_hash_table_new_full (arv_gc_cache_key_hash, arv_gc_cache_key_equal, g_free, g_free);
	priv->n_cache_hits = 0;
	priv->n_cache_misses = 0;
}

static void
arv_gc_register_node_finalize (GObject *self)
{
	ArvGcRegisterNodePrivate *priv = arv_gc_register_node_get_instance_private (ARV_GC_REGISTER_NODE (self));

	g_slist_free (priv->addresses);
	g_slist_free (priv->swiss_knives);
	g_slist_free (priv->invalidators);
	g_clear_pointer (&priv->caches, g_hash_table_unref);

	if (priv->n_cache_hits > 0 || priv->n_cache_misses > 0) {
		const char *name = arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (self));

		if (name == NULL)
			name = arv_dom_node_get_node_name (ARV_DOM_NODE (self));

		arv_debug_genicam ("Cache hits = %u / %u for %s",
				   priv->n_cache_hits,
				   priv->n_cache_hits + priv->n_cache_misses,
				   name);
	}

	G_OBJECT_CLASS (arv_gc_register_node_parent_class)->finalize (self);
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
	gc_feature_node_class->get_access_mode = arv_gc_register_node_get_access_mode;

	this_class->default_cachable = ARV_GC_CACHABLE_WRITE_THROUGH;
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
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	if (length < cache_length) {
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_INVALID_LENGTH,
			     "Register get failed due to data not fitting into buffer");
		return;
	}

	_read_from_port (gc_register_node, address, cache_length, cache, _get_cachable (gc_register_node), &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	if (length > cache_length) {
		memcpy (buffer, cache, cache_length);
		memset (((char *) buffer) + cache_length, 0, length - cache_length);
	} else
		memcpy (buffer, cache, length);

	arv_log_genicam ("[GcRegisterNode::get] 0x%" G_GINT64_MODIFIER "x,%" G_GUINT64_FORMAT, address, length);
}

static void
arv_gc_register_node_set (ArvGcRegister *gc_register, const void *buffer, guint64 length, GError **error)
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

	if (cache_length < length) {
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_INVALID_LENGTH,
			     "Register set failed due to data not fitting into register");
		return;
	}

	if (cache_length > length) {
		memcpy (cache, buffer, length);
		memset (((char *) cache) + length, 0, cache_length - length);
	} else
		memcpy (cache, buffer, cache_length);

	_write_to_port (gc_register_node, address, cache_length, cache, _get_cachable (gc_register_node), &local_error);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	arv_log_genicam ("[GcRegisterNode::set] 0x%" G_GINT64_MODIFIER "x,%" G_GUINT64_FORMAT, address, length);
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
		    guint register_lsb, guint register_msb,
		    ArvGcSignedness signedness, guint endianness,
		    ArvGcCachable cachable,
		    gboolean is_masked, GError **error)
{
	GError *local_error = NULL;
	gint64 value;
	guint lsb;
	guint msb;
	void *cache;
	gint64 address;
	gint64 length;

	cache = _get_cache (gc_register_node, &address, &length, &local_error);
	if (local_error == NULL)
		_read_from_port (gc_register_node, address, length, cache, cachable, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	arv_copy_memory_with_endianness (&value, sizeof (value), G_BYTE_ORDER,
					cache, length, endianness);

	if (is_masked) {
		guint64 mask;

		if (endianness == G_LITTLE_ENDIAN) {
			msb = register_msb;
			lsb = register_lsb;
		} else {
			lsb = 8 * length - register_lsb - 1;
			msb = 8 * length - register_msb - 1;
		}

		arv_log_genicam ("[GcRegisterNode::_get_integer_value] reglsb = %d, regmsb, %d, lsb = %d, msb = %d",
				 register_lsb, register_msb, lsb, msb);
		arv_log_genicam ("[GcRegisterNode::_get_integer_value] value = 0x%08" G_GINT64_MODIFIER "x", value);

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

		arv_log_genicam ("[GcRegisterNode::_get_integer_value] mask  = 0x%08" G_GINT64_MODIFIER "x", mask);
	} else {
		if (length < 8 &&
		    ((value & (((guint64) 1) << (length * 8 - 1))) != 0) &&
		    signedness == ARV_GC_SIGNEDNESS_SIGNED)
			value |= G_MAXUINT64 ^ ((((guint64) 1) << (length * 8)) - 1);
	}

	arv_log_genicam ("[GcRegisterNode::_get_integer_value] address = 0x%" G_GINT64_MODIFIER "x, value = 0x%" G_GINT64_MODIFIER "x",
			 _get_address (gc_register_node, NULL), value);

	return value;
}

gint64
arv_gc_register_node_get_masked_integer_value (ArvGcRegisterNode *self,
					       guint lsb, guint msb,
					       ArvGcSignedness signedness, guint endianness,
					       ArvGcCachable cachable,
					       gboolean is_masked, GError **error)
{
	g_return_val_if_fail (ARV_IS_GC_REGISTER_NODE (self), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	if (cachable == ARV_GC_CACHABLE_UNDEFINED)
		cachable = _get_cachable (self);

	if (endianness == 0)
		endianness = _get_endianness (self);

	return _get_integer_value (self, lsb, msb, signedness, endianness, cachable, is_masked, error);
}

static void
_set_integer_value (ArvGcRegisterNode *gc_register_node,
		    guint register_lsb, guint register_msb,
		    ArvGcSignedness signedness, guint endianness,
		    ArvGcCachable cachable,
		    gboolean is_masked, gint64 value, GError **error)
{
	GError *local_error = NULL;
	guint lsb;
	guint msb;
	void *cache;
	gint64 address;
	gint64 length;

	cache = _get_cache (gc_register_node, &address, &length, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	if (is_masked) {
		gint64 current_value;
		guint64 mask;

		if (ARV_GC_ACCESS_MODE_WO != arv_gc_register_node_get_access_mode(ARV_GC_FEATURE_NODE(gc_register_node))) {
			_read_from_port (gc_register_node, address, length, cache, cachable, &local_error);
			if (local_error != NULL) {
				g_propagate_error (error, local_error);
				return;
			}
		}

		arv_copy_memory_with_endianness (&current_value, sizeof (current_value), G_BYTE_ORDER,
						cache, length, endianness);

		if (endianness == G_LITTLE_ENDIAN) {
			msb = register_msb;
			lsb = register_lsb;
		} else {
			lsb = 8 * length - register_lsb - 1;
			msb = 8 * length - register_msb - 1;
		}

		arv_log_genicam ("[GcRegisterNode::_set_integer_value] reglsb = %d, regmsb, %d, lsb = %d, msb = %d",
				 register_lsb, register_msb, lsb, msb);
		arv_log_genicam ("[GcRegisterNode::_set_integer_value] value = 0x%08" G_GINT64_MODIFIER "x", value);

		if (msb - lsb < 63)
			mask = ((((guint64) 1) << (msb - lsb + 1)) - 1) << lsb;
		else
			mask = G_MAXUINT64;

		value = ((value << lsb) & mask) | (current_value & ~mask);

		arv_log_genicam ("[GcRegisterNode::_set_integer_value] mask  = 0x%08" G_GINT64_MODIFIER "x", mask);
	}

	arv_log_genicam ("[GcRegisterNode::_set_integer_value] address = 0x%" G_GINT64_MODIFIER "x, value = 0x%" G_GINT64_MODIFIER "x",
			 _get_address (gc_register_node, NULL), value);

	arv_copy_memory_with_endianness (cache, length, endianness,
					&value, sizeof (value), G_BYTE_ORDER);

	_write_to_port (gc_register_node, address, length, cache, cachable, &local_error);
	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

}

void
arv_gc_register_node_set_masked_integer_value (ArvGcRegisterNode *self,
					       guint lsb, guint msb,
					       ArvGcSignedness signedness, guint endianness,
					       ArvGcCachable cachable,
					       gboolean is_masked,
					       gint64 value, GError **error)
{
	g_return_if_fail (ARV_IS_GC_REGISTER_NODE (self));
	g_return_if_fail (error == NULL || *error == NULL);

	if (cachable == ARV_GC_CACHABLE_UNDEFINED)
		cachable = _get_cachable (self);

	if (endianness == 0)
		endianness = _get_endianness (self);

	_set_integer_value (self, lsb, msb, signedness, endianness, cachable, is_masked, value, error);
}
