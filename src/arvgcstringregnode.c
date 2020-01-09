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

#include <arvgcstringregnode.h>
#include <arvgcstring.h>
#include <arvgcregister.h>
#include <string.h>

typedef struct {
	char *string;
} ArvGcStringRegNodePrivate;

static void arv_gc_string_reg_node_init (ArvGcStringRegNode *self);
static void arv_gc_string_reg_node_string_interface_init (ArvGcStringInterface *interface);

G_DEFINE_TYPE_WITH_CODE (ArvGcStringRegNode, arv_gc_string_reg_node, ARV_TYPE_GC_REGISTER_NODE,
			 G_ADD_PRIVATE (ArvGcStringRegNode)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_STRING, arv_gc_string_reg_node_string_interface_init))

static const char *
arv_gc_string_reg_node_get_node_name (ArvDomNode *node)
{
	return "StringReg";
}

static const char *
arv_gc_string_reg_node_get_string_value (ArvGcString *self, GError **error)
{
	ArvGcStringRegNodePrivate *priv = arv_gc_string_reg_node_get_instance_private (ARV_GC_STRING_REG_NODE (self));
	GError *local_error = NULL;
	gint64 length;

	length = arv_gc_string_get_max_length (self, &local_error);
	if (local_error == NULL) {
		priv->string = g_realloc (priv->string, length + 1);

		if (priv->string != NULL) {
			arv_gc_register_get (ARV_GC_REGISTER (self), priv->string, length, &local_error);
			if (local_error == NULL)
				priv->string[length] = '\0';
		}
	} else {
		g_clear_pointer (&priv->string, g_free);
	}

	if (local_error != NULL)
		g_propagate_error (error, local_error);

	return priv->string;
}

static void
arv_gc_string_reg_node_set_string_value (ArvGcString *self, const char *value, GError **error)
{
	if (value != NULL)
		arv_gc_register_set (ARV_GC_REGISTER (self), value, strlen (value), error);
}

static gint64
arv_gc_string_reg_node_get_max_string_length (ArvGcString *self, GError **error)
{
	return arv_gc_register_get_length (ARV_GC_REGISTER (self), error);
}

/**
 * arv_gc_string_reg_node_new:
 *
 * Returns: a new StringReg node
 *
 * Since:0.8.0
 */

ArvGcNode *
arv_gc_string_reg_node_new	(void)
{
	return g_object_new (ARV_TYPE_GC_STRING_REG_NODE, NULL);
}

static void
arv_gc_string_reg_node_string_interface_init (ArvGcStringInterface *interface)
{
	interface->get_value = arv_gc_string_reg_node_get_string_value;
	interface->set_value = arv_gc_string_reg_node_set_string_value;
	interface->get_max_length = arv_gc_string_reg_node_get_max_string_length;
}

static void
arv_gc_string_reg_node_init (ArvGcStringRegNode *self)
{
}

static void
arv_gc_string_reg_node_finalize (GObject *self)
{
	ArvGcStringRegNodePrivate *priv = arv_gc_string_reg_node_get_instance_private (ARV_GC_STRING_REG_NODE (self));

	g_clear_pointer (&priv->string, g_free);

	G_OBJECT_CLASS (arv_gc_string_reg_node_parent_class)->finalize (self);
}

static void
arv_gc_string_reg_node_class_init (ArvGcStringRegNodeClass *this_class)
{
	ArvGcRegisterNodeClass *gc_register_node_class = ARV_GC_REGISTER_NODE_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);

	object_class->finalize = arv_gc_string_reg_node_finalize;
	dom_node_class->get_node_name = arv_gc_string_reg_node_get_node_name;
	gc_register_node_class->default_cachable = ARV_GC_CACHABLE_WRITE_THROUGH;
}
