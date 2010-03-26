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

#include <arvgcregister.h>
#include <arvgcinteger.h>
#include <arvgcport.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvGcNode implementation */

ArvGcUint64 *
arv_gc_uint64_new (guint64 value)
{
	ArvGcUint64 *address;

	address = g_new (ArvGcUint64, 1);
	address->value = value;
	address->node_name = NULL;

	return address;
}

ArvGcUint64 *
arv_gc_uint64_new_with_node_name (const char *node_name)
{
	ArvGcUint64 *address;

	address = g_new (ArvGcUint64, 1);
	address->value = 0;
	address->node_name = g_strdup (node_name);

	return address;
}

guint64
arv_gc_get_uint64_value (ArvGc *genicam, ArvGcUint64 *gc_uint64)
{
	if (gc_uint64 == NULL)
		return 0;

	if (gc_uint64->node_name == NULL)
		return gc_uint64->value;
	else {
		ArvGcNode *node;

		node = arv_gc_get_node (genicam, gc_uint64->node_name);
		if (ARV_IS_GC_INTEGER (node))
			return arv_gc_integer_get_value (ARV_GC_INTEGER (node));
	}

	return 0;
}

void
arv_gc_uint64_free (ArvGcUint64 *value)
{
	if (value == NULL)
		return;

	g_free (value->node_name);
	g_free (value);
}

static void
arv_gc_register_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (node);
	ArvGcUint64 *address;

	if (strcmp (name, "Address") == 0) {
		address = arv_gc_uint64_new (g_ascii_strtoull (content, NULL, 0));
		gc_register->addresses = g_list_prepend (gc_register->addresses, address);
	} else if (strcmp (name, "pAddress") == 0) {
		address = arv_gc_uint64_new_with_node_name (content);
		gc_register->addresses = g_list_prepend (gc_register->addresses, address);
	} else if (strcmp (name, "Length") == 0) {
		arv_gc_uint64_free (gc_register->length);
		gc_register->length = arv_gc_uint64_new (g_ascii_strtoull (content, NULL, 0));
	} else if (strcmp (name, "pLength") == 0) {
		arv_gc_uint64_free (gc_register->length);
		gc_register->length = arv_gc_uint64_new_with_node_name (content);
	} else if (strcmp (name, "AccessMode") == 0) {
		if (g_strcmp0 (content, "RW") == 0)
			gc_register->access_mode = ARV_GC_ACCESS_MODE_RW;
		else if (g_strcmp0 (content, "RO") == 0)
			gc_register->access_mode = ARV_GC_ACCESS_MODE_RO;
		else if (g_strcmp0 (content, "WO") == 0)
			gc_register->access_mode = ARV_GC_ACCESS_MODE_WO;
	} else if (strcmp (name, "Cacheable") == 0) {
		if (g_strcmp0 (content, "NoCache") == 0)
			gc_register->cacheable = ARV_GC_CACHEABLE_NO_CACHE;
		else if (g_strcmp0 (content, "WriteAround") == 0)
			gc_register->cacheable = ARV_GC_CACHEABLE_WRITE_AROUND;
		else if (g_strcmp0 (content, "WriteThrough") == 0)
			gc_register->cacheable = ARV_GC_CACHEABLE_WRITE_TRHOUGH;
	} else if (strcmp (name, "pPort") == 0) {
		g_free (gc_register->port_name);
		gc_register->port_name = g_strdup (content);
	} else if (strcmp (name, "PollingTime") == 0) {
		gc_register->polling_time = g_ascii_strtoull (content, NULL, 0);
	} else
		ARV_GC_NODE_CLASS (parent_class)->add_element (node, name, content, attributes);
}

/* ArvGcRegister implementation */

void
arv_gc_register_get (ArvGcRegister *gc_register, guint8 *buffer, guint64 length)
{
	ArvGc *genicam;
	ArvGcNode *port;

	g_return_if_fail (ARV_IS_GC_REGISTER (gc_register));
	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_register));
	g_return_if_fail (ARV_IS_GC (genicam));

	port = arv_gc_get_node (genicam, gc_register->port_name);
	if (!ARV_IS_GC_PORT (port))
		return;

	arv_gc_port_read (ARV_GC_PORT (port), buffer, arv_gc_register_get_address (gc_register), length);
}

void
arv_gc_register_set (ArvGcRegister *gc_register, guint8 *buffer, guint64 length)
{
	ArvGc *genicam;
	ArvGcNode *port;

	g_return_if_fail (ARV_IS_GC_REGISTER (gc_register));
	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_register));
	g_return_if_fail (ARV_IS_GC (genicam));

	port = arv_gc_get_node (genicam, gc_register->port_name);
	if (!ARV_IS_GC_PORT (port))
		return;

	arv_gc_port_write (ARV_GC_PORT (port), buffer, arv_gc_register_get_address (gc_register), length);
}

guint64
arv_gc_register_get_address (ArvGcRegister *gc_register)
{
	ArvGc *genicam;
	GList *iter;
	guint64 value = 0;

	g_return_val_if_fail (ARV_IS_GC_REGISTER (gc_register), 0);
	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_register));
	g_return_val_if_fail (ARV_IS_GC (genicam), 0);

	for (iter = gc_register->addresses; iter != NULL; iter = iter->next)
		value += arv_gc_get_uint64_value (genicam, iter->data);

	return value;
}

guint64
arv_gc_register_get_length (ArvGcRegister *gc_register)
{
	ArvGc *genicam;

	g_return_val_if_fail (ARV_IS_GC_REGISTER (gc_register), 0);
	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_register));
	g_return_val_if_fail (ARV_IS_GC (genicam), 0);

	return arv_gc_get_uint64_value (genicam, gc_register->length);
}

ArvGcNode *
arv_gc_register_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_REGISTER, NULL);

	return node;
}

static void
arv_gc_register_init (ArvGcRegister *gc_register)
{
	gc_register->access_mode = ARV_GC_ACCESS_MODE_RO;
	gc_register->cacheable = ARV_GC_CACHEABLE_NO_CACHE;
}

static void
arv_gc_register_finalize (GObject *object)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (object);
	GList *iter;

	for (iter = gc_register->addresses; iter != NULL; iter = iter->next)
		arv_gc_uint64_free (iter->data);
	g_list_free (gc_register->addresses);
	arv_gc_uint64_free (gc_register->length);
	g_free (gc_register->port_name);

	parent_class->finalize (object);
}

static void
arv_gc_register_class_init (ArvGcRegisterClass *register_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (register_class);
	ArvGcNodeClass *node_class = ARV_GC_NODE_CLASS (register_class);

	parent_class = g_type_class_peek_parent (register_class);

	object_class->finalize = arv_gc_register_finalize;

	node_class->add_element = arv_gc_register_add_element;
}

/* ArvGcInteger interface implementation */

static guint64
arv_gc_register_get_integer_value (ArvGcInteger *gc_integer)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (gc_integer);
	ArvGc *genicam;
	ArvGcNode *port_node;
	guint64 value = 0;
	guint64 address;
	size_t size;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_integer));

	size = MIN (sizeof (value), arv_gc_get_uint64_value (genicam, gc_register->length));
	address = arv_gc_get_uint64_value (genicam, gc_register->length);

	port_node = arv_gc_get_node (genicam, gc_register->port_name);
	arv_gc_port_read (ARV_GC_PORT (port_node), &value, address, size);

	return value;
}

static void
arv_gc_register_set_integer_value (ArvGcInteger *gc_integer, guint64 value)
{
	ArvGcRegister *gc_register = ARV_GC_REGISTER (gc_integer);
	ArvGc *genicam;
	ArvGcNode *port_node;
	guint64 address;
	size_t size;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_integer));

	size = MIN (sizeof (value), arv_gc_get_uint64_value (genicam, gc_register->length));
	address = arv_gc_get_uint64_value (genicam, gc_register->length);

	port_node = arv_gc_get_node (genicam, gc_register->port_name);
	arv_gc_port_write (ARV_GC_PORT (port_node), &value, address, size);
}


static void
arv_gc_register_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_register_get_integer_value;
	interface->set_value = arv_gc_register_set_integer_value;
}

G_DEFINE_TYPE_WITH_CODE (ArvGcRegister, arv_gc_register, ARV_TYPE_GC_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_register_integer_interface_init))

