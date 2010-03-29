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

#include <arvgcconverter.h>
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
} ArvGcConverterVariableInfos;

static void
arv_gc_converter_add_element (ArvGcNode *node, const char *name, const char *content, const char **attributes)
{
	ArvGcConverter *gc_converter = ARV_GC_CONVERTER (node);

	if (strcmp (name, "pVariable") == 0) {
		const char *variable_name = NULL;
		int i;

		for (i = 0; attributes[i] != NULL && attributes[i+1] != NULL; i += 2)
			if (g_strcmp0 (attributes[i], "Name") == 0) {
				variable_name = attributes[i+1];
				break;
			}

		if (variable_name != NULL) {
			ArvGcConverterVariableInfos *variable_infos;

			variable_infos = g_new (ArvGcConverterVariableInfos, 1);
			variable_infos->name = g_strdup (variable_name);
			variable_infos->node_name = g_strdup (content);
			gc_converter->variables = g_slist_prepend (gc_converter->variables,
								     variable_infos);

			arv_debug (ARV_DEBUG_LEVEL_STANDARD,
				   "[GcConverter::add_element] Add pVariable '%s' named '%s'",
				   content, variable_name);
		}
	} else if (strcmp (name, "FormulaTo") == 0) {
		arv_evaluator_set_expression (gc_converter->formula_to, content);
	} else if (strcmp (name, "FormulaFrom") == 0) {
		arv_evaluator_set_expression (gc_converter->formula_from, content);
	} else if (strcmp (name, "pValue") == 0) {
		g_free (gc_converter->value);
		gc_converter->value = g_strdup (content);
	} else if (strcmp (name, "Expression") == 0) {
		g_assert_not_reached ();
	} else if (strcmp (name, "Constant") == 0) {
		g_assert_not_reached ();
	} else
		ARV_GC_NODE_CLASS (parent_class)->add_element (node, name, content, attributes);
}

/* ArvGcConverter implementation */

ArvGcNode *
arv_gc_converter_new (void)
{
	ArvGcConverter *converter;

	converter = g_object_new (ARV_TYPE_GC_CONVERTER, NULL);
	converter->is_integer = FALSE;

	return ARV_GC_NODE (converter);
}

ArvGcNode *
arv_gc_int_converter_new (void)
{
	ArvGcConverter *converter;

	converter = g_object_new (ARV_TYPE_GC_CONVERTER, NULL);
	converter->is_integer = TRUE;

	return ARV_GC_NODE (converter);
}

static void
arv_gc_converter_init (ArvGcConverter *gc_converter)
{
	gc_converter->formula_to = arv_evaluator_new ("");
	gc_converter->formula_from = arv_evaluator_new ("");
	gc_converter->value = NULL;
}

static void
arv_gc_converter_finalize (GObject *object)
{
	ArvGcConverter *gc_converter = ARV_GC_CONVERTER (object);
	GSList *iter;

	for (iter = gc_converter->variables; iter != NULL; iter = iter->next) {
		ArvGcConverterVariableInfos *variable_infos = iter->data;

		g_free (variable_infos->name);
		g_free (variable_infos->node_name);
		g_free (variable_infos);
	}
	g_slist_free (gc_converter->variables);

	g_object_unref (gc_converter->formula_to);
	g_object_unref (gc_converter->formula_from);

	g_free (gc_converter->value);

	parent_class->finalize (object);
}

static void
arv_gc_converter_class_init (ArvGcConverterClass *converter_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (converter_class);
	ArvGcNodeClass *node_class = ARV_GC_NODE_CLASS (converter_class);

	parent_class = g_type_class_peek_parent (converter_class);

	object_class->finalize = arv_gc_converter_finalize;

	node_class->add_element = arv_gc_converter_add_element;
}

/* ArvGcInteger interface implementation */

static void
_update_from_variables (ArvGcConverter *gc_converter)
{
	ArvGc *genicam;
	ArvGcNode *node;
	GSList *iter;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_converter));

	for (iter = gc_converter->variables; iter != NULL; iter = iter->next) {
		ArvGcConverterVariableInfos *variable_infos = iter->data;

		node = arv_gc_get_node (genicam, variable_infos->node_name);
		if (ARV_IS_GC_INTEGER (node))
			arv_evaluator_set_int64_variable (gc_converter->formula_from,
							  variable_infos->name,
							  arv_gc_integer_get_value (ARV_GC_INTEGER (node)));
		else if (ARV_IS_GC_FLOAT (node))
			arv_evaluator_set_double_variable (gc_converter->formula_from,
							   variable_infos->name,
							   arv_gc_float_get_value (ARV_GC_FLOAT (node)));
	}

	node = arv_gc_get_node (genicam, gc_converter->value);
	if (ARV_IS_GC_INTEGER (node))
		arv_evaluator_set_int64_variable (gc_converter->formula_from,
						  "TO",
						  arv_gc_integer_get_value (ARV_GC_INTEGER (node)));
	else if (ARV_IS_GC_FLOAT (node))
		arv_evaluator_set_double_variable (gc_converter->formula_from,
						   "TO",
						   arv_gc_float_get_value (ARV_GC_FLOAT (node)));
}

static void
_update_to_variables (ArvGcConverter *gc_converter)
{
	ArvGc *genicam;
	ArvGcNode *node;
	GSList *iter;

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (gc_converter));

	for (iter = gc_converter->variables; iter != NULL; iter = iter->next) {
		ArvGcConverterVariableInfos *variable_infos = iter->data;

		node = arv_gc_get_node (genicam, variable_infos->node_name);
		if (ARV_IS_GC_INTEGER (node))
			arv_evaluator_set_int64_variable (gc_converter->formula_to,
							  variable_infos->name,
							  arv_gc_integer_get_value (ARV_GC_INTEGER (node)));
		else if (ARV_IS_GC_FLOAT (node))
			arv_evaluator_set_double_variable (gc_converter->formula_from,
							   variable_infos->name,
							   arv_gc_float_get_value (ARV_GC_FLOAT (node)));
	}

	node = arv_gc_get_node (genicam, gc_converter->value);
	if (ARV_IS_GC_INTEGER (node))
		arv_gc_integer_set_value (ARV_GC_INTEGER (node),
					  arv_evaluator_evaluate_as_int64 (gc_converter->formula_to, NULL));
	else if (ARV_IS_GC_FLOAT (node))
		arv_gc_float_set_value (ARV_GC_FLOAT (node),
					arv_evaluator_evaluate_as_double (gc_converter->formula_to, NULL));
	else
		arv_debug (ARV_DEBUG_LEVEL_STANDARD,
			   "[GcConverter::set_value] Invalid pValue node '%s'",
			   gc_converter->value);
}

static gint64
arv_gc_converter_get_integer_value (ArvGcInteger *gc_integer)
{
	ArvGcConverter *gc_converter = ARV_GC_CONVERTER (gc_integer);

	_update_from_variables (gc_converter);

	return arv_evaluator_evaluate_as_int64 (gc_converter->formula_from, NULL);
}

static void
arv_gc_converter_set_integer_value (ArvGcInteger *gc_integer, gint64 value)
{
	ArvGcConverter *gc_converter = ARV_GC_CONVERTER (gc_integer);

	arv_evaluator_set_int64_variable (gc_converter->formula_to,
					  "FROM", value);

	_update_to_variables (gc_converter);
}

static void
arv_gc_converter_integer_interface_init (ArvGcIntegerInterface *interface)
{
	interface->get_value = arv_gc_converter_get_integer_value;
	interface->set_value = arv_gc_converter_set_integer_value;
}

static double
arv_gc_converter_get_float_value (ArvGcFloat *gc_float)
{
	ArvGcConverter *gc_converter = ARV_GC_CONVERTER (gc_float);

	_update_from_variables (gc_converter);

	return arv_evaluator_evaluate_as_double (gc_converter->formula_from, NULL);
}

static void
arv_gc_converter_set_float_value (ArvGcFloat *gc_float, double value)
{
	ArvGcConverter *gc_converter = ARV_GC_CONVERTER (gc_float);

	arv_evaluator_set_int64_variable (gc_converter->formula_to,
					  "FROM", value);

	_update_to_variables (gc_converter);
}

static void
arv_gc_converter_float_interface_init (ArvGcFloatInterface *interface)
{
	interface->get_value = arv_gc_converter_get_float_value;
	interface->set_value = arv_gc_converter_set_float_value;
}

G_DEFINE_TYPE_WITH_CODE (ArvGcConverter, arv_gc_converter, ARV_TYPE_GC_NODE,
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_INTEGER, arv_gc_converter_integer_interface_init)
			 G_IMPLEMENT_INTERFACE (ARV_TYPE_GC_FLOAT,   arv_gc_converter_float_interface_init))
