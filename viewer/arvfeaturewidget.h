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

#ifndef ARV_FEATURE_WIDGET_H
#define ARV_FEATURE_WIDGET_H

#include <gtk/gtk.h>
#include <arv.h>

G_BEGIN_DECLS

#define ARV_TYPE_FEATURE_WIDGET             (arv_feature_widget_get_type ())
#define ARV_FEATURE_WIDGET(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_FEATURE_WIDGET, ArvFeatureWidget))
#define ARV_FEATURE_WIDGET_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_FEATURE_WIDGET, ArvFeatureWidgetClass))
#define ARV_IS_FEATURE_WIDGET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_FEATURE_WIDGET))
#define ARV_IS_FEATURE_WIDGET_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_FEATURE_WIDGET))
#define ARV_FEATURE_WIDGET_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_FEATURE_WIDGET, ArvFeatureWidgetClass))

typedef struct _ArvFeatureWidget ArvFeatureWidget;
typedef struct _ArvFeatureWidgetPrivate ArvFeatureWidgetPrivate;
typedef struct _ArvFeatureWidgetClass ArvFeatureWidgetClass;

struct _ArvFeatureWidget {
	GtkVBox parent;

	ArvFeatureWidgetPrivate *priv;
};

struct _ArvFeatureWidgetClass {
	GtkVBoxClass parent_class;
};

GType arv_feature_widget_get_type (void);

ArvFeatureWidget * 	arv_feature_widget_new 			(ArvGcFeatureNode *node);

G_END_DECLS

#endif

