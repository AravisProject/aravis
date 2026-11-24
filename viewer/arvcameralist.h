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

#ifndef ARV_CAMERA_LIST_H
#define ARV_CAMERA_LIST_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* One row: what the list shows and the device id to open. */
typedef struct _ArvCameraItem ArvCameraItem;

GType			arv_camera_item_get_type	(void);

#define ARV_TYPE_CAMERA_ITEM	(arv_camera_item_get_type ())

/* The list, its model, and the selection the caller watches. */
typedef struct {
	GtkWidget *		view;
	GListStore *		store;
	GtkSingleSelection *	selection;
} ArvCameraList;

ArvCameraList *		arv_camera_list_new		(void);
void			arv_camera_list_clear		(ArvCameraList *list);
void			arv_camera_list_append		(ArvCameraList *list,
							 const char *id,
							 const char *icon,
							 const char *vendor,
							 const char *model,
							 const char *serial);
char *			arv_camera_list_get_selected_id	(ArvCameraList *list);

G_END_DECLS

#endif
