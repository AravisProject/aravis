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
 * SECTION: arvgcselector
 * @short_description: Selector interface
 */

#include <arvgcselector.h>
#include <arvgc.h>
#include <arvmisc.h>

static void
arv_gc_selector_default_init (ArvGcSelectorInterface *gc_selector_iface)
{
}

G_DEFINE_INTERFACE (ArvGcSelector, arv_gc_selector, G_TYPE_OBJECT)

/**
 * arv_gc_selector_is_selector:
 * @gc_selector: a #ArvGcSelector
 *
 * Returns: %TRUE if this node is a selector, i.e. it has pSelected childs.
 *
 * Since: 0.8.0
 */

gboolean
arv_gc_selector_is_selector (ArvGcSelector *gc_selector)
{
	g_return_val_if_fail (ARV_IS_GC_SELECTOR (gc_selector), FALSE);

	return arv_gc_selector_get_selected_features (gc_selector) != NULL;
}

/**
 * arv_gc_selector_get_selected_features:
 * @gc_selector: a #ArvGcSelector
 *
 * Returns: (element-type ArvGcFeatureNode) (transfer none): a list of selected #ArvGcFeatureNode
 *
 * Since: 0.8.0
 */

const GSList *
arv_gc_selector_get_selected_features (ArvGcSelector *gc_selector)
{
	ArvGcSelectorInterface *selector_interface;

	g_return_val_if_fail (ARV_IS_GC_SELECTOR (gc_selector), 0);

	selector_interface = ARV_GC_SELECTOR_GET_INTERFACE (gc_selector);

	if (selector_interface->get_selected_features != NULL)
		return selector_interface->get_selected_features (gc_selector);

	return NULL;
}
