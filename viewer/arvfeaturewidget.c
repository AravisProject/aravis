/* Aravis - Digital camera library
 *
 * Copyright © 2011 Emmanuel Pacaud
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

#include <arvfeaturewidget.h>

static GObjectClass *parent_class = NULL;

struct _ArvFeatureWidgetPrivate {
	ArvGcNode *node;

	GtkWidget *entry;
	GtkWidget *combo_box;
	GtkWidget *spin_button;
};

ArvFeatureWidget *
arv_feature_widget_new (ArvGcNode *node)
{
	ArvFeatureWidget *feature;

	g_object_ref (node);

	feature = g_object_new (ARV_TYPE_FEATURE_WIDGET, NULL);
	feature->priv->node = node;

	return feature;
}

static void
arv_feature_widget_init (ArvFeatureWidget *feature)
{
	feature->priv = G_TYPE_INSTANCE_GET_PRIVATE (feature, ARV_TYPE_FEATURE_WIDGET, ArvFeatureWidgetPrivate);
}

static void
arv_feature_widget_finalize (GObject *object)
{
	ArvFeatureWidget *feature = ARV_FEATURE_WIDGET (object);

	if (feature->priv->node != NULL)
		g_object_unref (feature->priv->node);

	parent_class->finalize (object);
}

static void
arv_feature_widget_class_init (ArvFeatureWidgetClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = arv_feature_widget_finalize;
}

G_DEFINE_TYPE (ArvFeatureWidget, arv_feature_widget, GTK_TYPE_VBOX)
