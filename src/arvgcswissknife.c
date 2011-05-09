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
 * SECTION: arvgcswissknife
 * @short_description: Class for SwissKnife and IntSwissKnife nodes
 */

#include <arvgcswissknife.h>
#include <arvevaluator.h>
#include <arvgcinteger.h>
#include <arvgcfloat.h>
#include <arvgcport.h>
#include <arvgc.h>
#include <arvdebug.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

/* ArvGcNode implementation */

typedef struct {
	char *name;
	char *node_name;
} ArvGcSwissKnifeVariableInfos;

static void
arv_gc_swiss_knife_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	ArvGcSwissKnife *gc_swiss_knife = ARV_GC_SWISS_KNIFE (node);

	if (strcmp (name, "pVariable") == 0) {
		const char *variable_name = NULL;
		int i;

		for (i = 0; attributes[i] != NULL && attributes[i+1] != NULL; i += 2)
			if (g_strcmp0 (attributes[i], "Name") == 0) {
				variable_name = attributes[i+1];
				break;
			}

		if (variable_name != NULL) {
			ArvGcSwissKnifeVariableInfos *variable_infos;

			variable_infos = g_new (ArvGcSwissKnifeVariableInfos, 1);
			variable_infos->name = g_strdup (variable_name);
			variable_infos->node_name = g_strdup (content);
			gc_swiss_knife->variables = g_slist_prepend (gc_swiss_knife->variables,
								     variable_infos);

			arv_log_genicam ("[GcSwissKnife::add_element] Add pVariable '%s' named '%s'",
					 content, variable_name);
		}
	} else if (strcmp (name, "Formula") == 0) {
		arv_evaluator_set_expression (gc_swiss_knife->formula, content);
	} else if (strcmp (name, "Expression") == 0) {
		g_assert_not_reached ();
	} else if (strcmp (name, "Constant") == 0) {
		g_assert_not_reached ();
	} else
		ARV_GC_NODE_CLASS (parent_class)->add_element (node, name, content, attributes);
}

/* ArvGcSwissKnife implementation */

static GType
arv_gc_swiss_knife_node_get_value_type (ArvGcNode *node)
{
	ArvGcSwissKnife *gc_swiss_knife = ARV_GC_SWISS_KNIFE (node);

	return gc_swiss_knife->value_type;
}

ArvGcNode *
arv_gc_swiss_knife_new (void)
{
	ArvGcSwissKnife *swiss_knife;

	swiss_knife = g_object_new (ARV_TYPE_GC_SWISS_KNIFE, NULL);
	swiss_knife->value_type = G_TYPE_DOUBLE;

	return ARV_GC_NODE (swiss_knife);
}

ArvGcNode *
arv_gc_swiss_knife_new_integer (void)
{
	ArvGcSwissKnife *swiss_knife;

	swiss_knife = g_object_new (ARV_TYPE_GC_SWISS_KNIFE, NULL);
	swiss_knife->value_type = G_TYPE_INT64;

	return ARV_GC_NODE (swiss_knife);
}

static void
arv_gc_swiss_knife_init (ArvGcSwissKnife *gc_swiss_knife)
{
	gc_swiss_knife->formula = arv_evaluator_new (NULL);
}

static void
arv_gc_swiss_knife_finalize (GObject *object)
{
	ArvGcSwissKnife *gc_swiss_knife = ARV_GC_SWISS_KNIFE (object);
	GSList *iter;

	for (iter = gc_swiss_knife->variables; iter != NULL; iter = iter->next) {
		ArvGcSwissKnifeVariableInfos *variable_infos = iter->data;

		g_free (variable_infos->name);
		g_free (variable_infos->node_name);
		g_free (variable_infos);
	}
	g_slist_free (gc_swiss_knife->variables);

	g_object_unref (gc_swiss_knife->formula);

	parent_class->finalize (object);
}

static void
arv_gc_swiss_knife_class_init (ArvGcSwissKnifeClass *swiss_knife_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (swiss_knife_class);
	ArvGcNodeClass *node_class = ARV_GC_NODE_CLASS (swiss_knife_class);

	parent_class = g_type_class_peek_parent (swiss_knife_class);

	object_class->finalize = arv_gc_swiss_knife_finalize;

	node_class->add_element = arv_gc_swiss_knife_add_element;
	node_class->get_value_type = arv_gc_swiss_knife_node_get_value_type;
}

/* ArvGcInteger interface implementation */

static void
_update_variables (ArvGcSwissKnife *gc_swiss_knife)
{
	ArvGc *genicam;
	ArvGcNode *node;
	GSList *iter;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_swiss_knife));

	for (iter = gc_swiss_knife->variables; iter != NULL; iter = iter->next) {
		ArvGcSwissKnifeVariableInfos *variable_infos = iter->data;

		node = arv_gc_get_node (genicam, variable_infos->node_name);
		if (arv_gc_node_get_value_type (node) == G_TYPE_INT64)
			arv_evaluator_set_int64_variable (gc_swiss_knife->formula,
							  variable_infos->name,
							  arv_gc_integer_get_value (ARV_GC_INTEGER (node)));
		else if (arv_gc_node_get_value_type (node) == G_TYPE_DOUBLE)
			arv_evaluator_set_double_variable (gc_swiss_knife->formula,
							   variable_infos->name,
							   arv_gc_float_get_value (ARV_GC_FLOAT (node)));
	}

}

static gint64
arv_gc_swiss_knife_get_integer_value (ArvGcInteger *gc_integer)
{
	ArvGcSwissKnife *gc_swiss_knife = ARV_GC_SWISS_KNIFE (gc_integer);

	_update_variables (gc_swiss_knife);

	return arv_evaluator_evaluate_as_int64 (gc_swiss_knife->formula, NULL);
}

static void
arv_gc_swiss_knife_set_integer_value (ArvGcInteger *gc_integer, gint64 value)
{
}

static void
arv_gc_swiss_knife_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_swiss_knife_get_integer_value;
	interface->set_value = arv_gc_swiss_knife_set_integer_value;
}

static double
arv_gc_swiss_knife_get_float_value (ArvGcFloat *gc_float)
{
	ArvGcSwissKnife *gc_swiss_knife = ARV_GC_SWISS_KNIFE (gc_float);

	_update_variables (gc_swiss_knife);

	return arv_evaluator_evaluate_as_double (gc_swiss_knife->formula, NULL);
}

static void
arv_gc_swiss_knife_set_float_value (ArvGcFloat *gc_float, double value)
{
}

static void
arv_gc_swiss_knife_float_interface_init (ArvGcFloatInterface *interface)
{
	interface->get_value = arv_gc_swiss_knife_get_float_value;
	interface->set_value = arv_gc_swiss_knife_set_float_value;
}

G_DEFINE_TYPE_WITH_CODE (ArvGcSwissKnife, arv_gc_swiss_knife, ARV_TYPE_GC_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_swiss_knife_integer_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_FLOAT,   arv_gc_swiss_knife_float_interface_init))
