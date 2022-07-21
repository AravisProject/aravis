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
 * SECTION: arvgccommand
 * @short_description: Class for Command nodes
 */

#include <arvgccommand.h>
#include <arvgcinteger.h>
#include <arvgcfeaturenodeprivate.h>
#include <arvgcport.h>
#include <arvgc.h>
#include <arvmisc.h>
#include <arvdebugprivate.h>
#include <stdlib.h>
#include <string.h>

struct _ArvGcCommand {
	ArvGcFeatureNode	node;

	ArvGcPropertyNode *command_value;
	ArvGcPropertyNode *value;
};

struct _ArvGcCommandClass {
	ArvGcFeatureNodeClass parent_class;
};

G_DEFINE_TYPE (ArvGcCommand, arv_gc_command, ARV_TYPE_GC_FEATURE_NODE)

/* ArvGcFeatureNode implementation */

static const char *
arv_gc_command_get_node_name (ArvDomNode *node)
{
	return "Command";
}

static void
arv_gc_command_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcCommand *node = ARV_GC_COMMAND (self);

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_VALUE:
			case ARV_GC_PROPERTY_NODE_TYPE_P_VALUE:
				node->value = property_node;
				break;
			case ARV_GC_PROPERTY_NODE_TYPE_COMMAND_VALUE:
			case ARV_GC_PROPERTY_NODE_TYPE_P_COMMAND_VALUE:
				node->command_value = property_node;
				break;
			default:
				ARV_DOM_NODE_CLASS (arv_gc_command_parent_class)->post_new_child (self, child);
				break;
		}
	}
}

static void
arv_gc_command_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcFeatureNode implementation */

/* ArvGcCommand implementation */

void
arv_gc_command_execute (ArvGcCommand *gc_command, GError **error)
{
	ArvGc *genicam;
	GError *local_error = NULL;
	gint64 command_value;

	g_return_if_fail (ARV_IS_GC_COMMAND (gc_command));
	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_command));
	g_return_if_fail (ARV_IS_GC (genicam));

	if (gc_command->value == NULL)
		return;

        if (!arv_gc_feature_node_check_write_access (ARV_GC_FEATURE_NODE (gc_command), error))
                return;

	command_value = arv_gc_property_node_get_int64 (gc_command->command_value, &local_error);

	if (local_error != NULL) {
		g_propagate_prefixed_error (error, local_error, "[%s] ",
                                            arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_command)));
		return;
	}

	arv_gc_feature_node_increment_change_count (ARV_GC_FEATURE_NODE (gc_command));
	arv_gc_property_node_set_int64 (gc_command->value, command_value, &local_error);

	if (local_error != NULL) {
		g_propagate_prefixed_error (error, local_error, "[%s] ",
                                            arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_command)));
		return;
	}

	arv_debug_genicam ("[GcCommand::execute] %s (0x%" G_GINT64_MODIFIER "x)",
			 arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (gc_command)),
			 command_value);
}

static ArvGcFeatureNode *
arv_gc_command_get_linked_feature (ArvGcFeatureNode *gc_feature_node)
{
	ArvGcCommand *gc_command = ARV_GC_COMMAND (gc_feature_node);
	ArvGcNode *pvalue_node = NULL;

	if (gc_command->value == NULL)
		return NULL;

	pvalue_node = arv_gc_property_node_get_linked_node (gc_command->value);
	if (ARV_IS_GC_FEATURE_NODE (pvalue_node))
		return ARV_GC_FEATURE_NODE (pvalue_node);

	return NULL;
}

ArvGcNode *
arv_gc_command_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_COMMAND, NULL);

	return node;
}

static void
arv_gc_command_init (ArvGcCommand *gc_command)
{
}

static void
arv_gc_command_finalize (GObject *object)
{
	G_OBJECT_CLASS (arv_gc_command_parent_class)->finalize (object);
}

static void
arv_gc_command_class_init (ArvGcCommandClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);
	ArvGcFeatureNodeClass *gc_feature_node_class = ARV_GC_FEATURE_NODE_CLASS (this_class);

	object_class->finalize = arv_gc_command_finalize;
	dom_node_class->get_node_name = arv_gc_command_get_node_name;
	dom_node_class->post_new_child = arv_gc_command_post_new_child;
	dom_node_class->pre_remove_child = arv_gc_command_pre_remove_child;
	gc_feature_node_class->get_linked_feature = arv_gc_command_get_linked_feature;
}
