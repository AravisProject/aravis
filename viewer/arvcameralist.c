/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* The camera list: a sortable GtkColumnView over a GListStore of devices.
 *
 * The list owns its model chain, and GtkSingleSelection frees the model it is
 * given when it is disposed (as the sort model does with the store), so the
 * caller only ever holds `store` to populate it and `selection` to watch.
 */

#include "arvcameralist.h"

/* One row of the camera list. A GObject because GListStore holds objects; the
 * fields are the columns the old GtkListStore carried, in the same order. */
struct _ArvCameraItem {
	GObject parent_instance;

	char *id;
	char *icon;
	char *vendor;
	char *model;
	char *serial;
};

typedef struct {
	GObjectClass parent_class;
} ArvCameraItemClass;

G_DEFINE_TYPE (ArvCameraItem, arv_camera_item, G_TYPE_OBJECT)

enum {
	ARV_CAMERA_ITEM_PROP_ID = 1,
	ARV_CAMERA_ITEM_PROP_ICON,
	ARV_CAMERA_ITEM_PROP_VENDOR,
	ARV_CAMERA_ITEM_PROP_MODEL,
	ARV_CAMERA_ITEM_PROP_SERIAL,
	ARV_CAMERA_ITEM_N_PROPERTIES
};

static GParamSpec *arv_camera_item_properties[ARV_CAMERA_ITEM_N_PROPERTIES];

static void
arv_camera_item_set_property (GObject *object, guint property_id,
			      const GValue *value, GParamSpec *pspec)
{
	ArvCameraItem *item = (ArvCameraItem *) object;
	char **field;

	switch (property_id) {
		case ARV_CAMERA_ITEM_PROP_ID:     field = &item->id;     break;
		case ARV_CAMERA_ITEM_PROP_ICON:   field = &item->icon;   break;
		case ARV_CAMERA_ITEM_PROP_VENDOR: field = &item->vendor; break;
		case ARV_CAMERA_ITEM_PROP_MODEL:  field = &item->model;  break;
		case ARV_CAMERA_ITEM_PROP_SERIAL: field = &item->serial; break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			return;
	}

	g_free (*field);
	*field = g_value_dup_string (value);
}

static void
arv_camera_item_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
	ArvCameraItem *item = (ArvCameraItem *) object;

	switch (property_id) {
		case ARV_CAMERA_ITEM_PROP_ID:     g_value_set_string (value, item->id);     break;
		case ARV_CAMERA_ITEM_PROP_ICON:   g_value_set_string (value, item->icon);   break;
		case ARV_CAMERA_ITEM_PROP_VENDOR: g_value_set_string (value, item->vendor); break;
		case ARV_CAMERA_ITEM_PROP_MODEL:  g_value_set_string (value, item->model);  break;
		case ARV_CAMERA_ITEM_PROP_SERIAL: g_value_set_string (value, item->serial); break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
arv_camera_item_finalize (GObject *object)
{
	ArvCameraItem *item = (ArvCameraItem *) object;

	g_free (item->id);
	g_free (item->icon);
	g_free (item->vendor);
	g_free (item->model);
	g_free (item->serial);

	G_OBJECT_CLASS (arv_camera_item_parent_class)->finalize (object);
}

static void
arv_camera_item_class_init (ArvCameraItemClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->set_property = arv_camera_item_set_property;
	object_class->get_property = arv_camera_item_get_property;
	object_class->finalize = arv_camera_item_finalize;

	arv_camera_item_properties[ARV_CAMERA_ITEM_PROP_ID] =
		g_param_spec_string ("id", "id", "Device id", NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	arv_camera_item_properties[ARV_CAMERA_ITEM_PROP_ICON] =
		g_param_spec_string ("icon", "icon", "Protocol icon name", NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	arv_camera_item_properties[ARV_CAMERA_ITEM_PROP_VENDOR] =
		g_param_spec_string ("vendor", "vendor", "Vendor", NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	arv_camera_item_properties[ARV_CAMERA_ITEM_PROP_MODEL] =
		g_param_spec_string ("model", "model", "Model", NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	arv_camera_item_properties[ARV_CAMERA_ITEM_PROP_SERIAL] =
		g_param_spec_string ("serial", "serial", "Serial", NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, ARV_CAMERA_ITEM_N_PROPERTIES,
					   arv_camera_item_properties);
}

static void
arv_camera_item_init (ArvCameraItem *item)
{
	item->id = NULL;
	item->icon = NULL;
	item->vendor = NULL;
	item->model = NULL;
	item->serial = NULL;
}

static void
camera_item_setup_label (GtkSignalListItemFactory *factory, GtkListItem *list_item,
			 gpointer user_data)
{
	gtk_list_item_set_child (list_item, gtk_label_new (NULL));
}

static void
camera_item_bind_label (GtkSignalListItemFactory *factory, GtkListItem *list_item,
			gpointer user_data)
{
	ArvCameraItem *item = gtk_list_item_get_item (list_item);
	GtkWidget *label = gtk_list_item_get_child (list_item);
	const char *field = g_object_get_data (G_OBJECT (factory), "field");
	char *text = NULL;

	if (field != NULL)
		g_object_get (item, field, &text, NULL);

	gtk_label_set_label (GTK_LABEL (label), text != NULL ? text : "");

	g_free (text);
}

/* The Model column: the protocol icon beside the model name. */
static void
camera_model_setup (GtkSignalListItemFactory *factory, GtkListItem *list_item,
		    gpointer user_data)
{
	GtkWidget *box;
	GtkWidget *image;
	GtkWidget *label;

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	image = gtk_image_new ();
	gtk_widget_set_margin_start (image, 4);
	gtk_image_set_pixel_size (GTK_IMAGE (image), 16);
	gtk_box_append (GTK_BOX (box), image);

	label = gtk_label_new (NULL);
	gtk_box_append (GTK_BOX (box), label);

	gtk_list_item_set_child (list_item, box);
}

static void
camera_model_bind (GtkSignalListItemFactory *factory, GtkListItem *list_item,
		   gpointer user_data)
{
	ArvCameraItem *item = gtk_list_item_get_item (list_item);
	GtkWidget *box = gtk_list_item_get_child (list_item);
	GtkWidget *image = gtk_widget_get_first_child (box);
	GtkWidget *label = gtk_widget_get_next_sibling (image);
	char *icon = NULL;
	char *model = NULL;

	g_object_get (item, "icon", &icon, "model", &model, NULL);

	gtk_image_set_from_icon_name (GTK_IMAGE (image), icon != NULL ? icon : "");
	gtk_label_set_label (GTK_LABEL (label), model != NULL ? model : "");

	g_free (icon);
	g_free (model);
}

/* A column of one text field, sortable by that field. */
static GtkColumnViewColumn *
camera_column_new (const char *title, const char *field, double expand,
		   GtkListItemFactory **factory_out)
{
	GtkListItemFactory *factory;
	GtkColumnViewColumn *column;
	GtkStringSorter *sorter;
	GtkExpression *expression;

	factory = gtk_signal_list_item_factory_new ();
	g_signal_connect (factory, "setup", G_CALLBACK (camera_item_setup_label), NULL);
	g_signal_connect (factory, "bind", G_CALLBACK (camera_item_bind_label), NULL);
	/* the field name rides on the factory, read back in bind */
	g_object_set_data (G_OBJECT (factory), "field", (gpointer) field);

	column = gtk_column_view_column_new (title, factory);

	/* clicking the header sorts by this field; the column view's own sorter,
	 * attached to the sort model, drives it */
	expression = gtk_property_expression_new (ARV_TYPE_CAMERA_ITEM, NULL, field);
	sorter = gtk_string_sorter_new (expression);
	gtk_column_view_column_set_sorter (column, GTK_SORTER (sorter));
	g_object_unref (sorter);
	if (expand > 0)
		gtk_column_view_column_set_expand (column, TRUE);

	*factory_out = factory;

	return column;
}

/* The Model column: icon plus name, sortable by the model name. */
static GtkColumnViewColumn *
camera_model_column_new (GtkListItemFactory **factory_out)
{
	GtkListItemFactory *factory;
	GtkColumnViewColumn *column;
	GtkExpression *expression;
	GtkStringSorter *sorter;

	factory = gtk_signal_list_item_factory_new ();
	g_signal_connect (factory, "setup", G_CALLBACK (camera_model_setup), NULL);
	g_signal_connect (factory, "bind", G_CALLBACK (camera_model_bind), NULL);

	column = gtk_column_view_column_new ("Model", factory);
	expression = gtk_property_expression_new (ARV_TYPE_CAMERA_ITEM, NULL, "model");
	sorter = gtk_string_sorter_new (expression);
	gtk_column_view_column_set_sorter (column, GTK_SORTER (sorter));
	gtk_column_view_column_set_expand (column, TRUE);

	g_object_unref (sorter);

	*factory_out = factory;

	return column;
}

static GtkColumnView *
camera_column_view_new (ArvCameraList *list)
{
	GtkColumnView *view;
	GtkColumnViewColumn *column;
	GtkListItemFactory *factory;
	GListStore *store;
	GtkSortListModel *sorted;
	GtkSingleSelection *selection;

	store = g_list_store_new (ARV_TYPE_CAMERA_ITEM);
	/* gtk_sort_list_model_new does not take its own reference on the model
	 * (measured: the count stays 1), so one is sunk here and handed to the
	 * caller; releasing `sorted` below would otherwise free the store */
	g_object_ref_sink (store);
	sorted = gtk_sort_list_model_new (G_LIST_MODEL (store), NULL);

	view = GTK_COLUMN_VIEW (gtk_column_view_new (NULL));

	/* The column does not own its factory (measured: dropping the last
	 * reference before the view draws leaves it dangling), and a column's
	 * destroy notify runs too late to release one. The factories are held on
	 * the view itself, which outlives its columns. */
	column = camera_column_new ("Vendor", "vendor", 0, &factory);
	gtk_column_view_append_column (view, column);
	g_object_unref (column);

	column = camera_model_column_new (&factory);
	gtk_column_view_append_column (view, column);
	g_object_unref (column);

	column = camera_column_new ("Serial", "serial", 0, &factory);
	gtk_column_view_append_column (view, column);
	g_object_unref (column);

	gtk_column_view_set_show_column_separators (view, TRUE);
	gtk_column_view_set_show_row_separators (view, TRUE);

	/* the view builds the sorter its columns feed; the sort model applies it */
	gtk_sort_list_model_set_sorter (sorted, gtk_column_view_get_sorter (view));

	/* Neither gtk_sort_list_model_new() nor gtk_single_selection_new() takes a
	 * reference on its model (measured), so the chain store -> sorted ->
	 * selection is held by these explicit references alone. The column view
	 * takes one on the selection only. */
	selection = gtk_single_selection_new (G_LIST_MODEL (sorted));
	/* GtkSingleSelection auto-selects the first row as the list fills, which
	 * emits no change when the user then clicks it: with one camera nothing
	 * would ever be opened. Start with nothing selected instead. */
	gtk_single_selection_set_autoselect (selection, FALSE);
	gtk_single_selection_set_can_unselect (selection, TRUE);
	gtk_column_view_set_model (view, GTK_SELECTION_MODEL (selection));

	/* The sort model takes ownership of the store's reference (measured: the
	 * count does not rise), so the viewer holds its own for the populate
	 * loop, which runs while the view is live. */
	list->store = g_object_ref (store);
	list->selection = selection;

	return view;
}

ArvCameraList *
arv_camera_list_new (void)
{
	ArvCameraList *list;

	list = g_new0 (ArvCameraList, 1);
	list->view = GTK_WIDGET (camera_column_view_new (list));

	return list;
}

void
arv_camera_list_clear (ArvCameraList *list)
{
	g_return_if_fail (list != NULL);

	g_list_store_remove_all (list->store);
}

/* The device id of the selected row, or NULL when nothing is selected; the
 * caller frees it. */
char *
arv_camera_list_get_selected_id (ArvCameraList *list)
{
	ArvCameraItem *item;
	char *id = NULL;

	g_return_val_if_fail (list != NULL, NULL);

	item = gtk_single_selection_get_selected_item (list->selection);
	if (item != NULL)
		g_object_get (item, "id", &id, NULL);

	return id;
}

void
arv_camera_list_append (ArvCameraList *list, const char *id, const char *icon,
			const char *vendor, const char *model, const char *serial)
{
	ArvCameraItem *item;

	g_return_if_fail (list != NULL);

	item = g_object_new (ARV_TYPE_CAMERA_ITEM,
			     "id", id,
			     "icon", icon,
			     "vendor", vendor,
			     "model", model,
			     "serial", serial,
			     NULL);
	g_list_store_append (list->store, item);
	g_object_unref (item);
}
