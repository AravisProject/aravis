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
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_V4L2_MISC_PRIVATE_H
#define ARV_V4L2_MISC_PRIVATE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvtypes.h>

G_BEGIN_DECLS

ArvPixelFormat  arv_pixel_format_from_v4l2      (guint32 v4l2_pixel_format);
guint32         arv_pixel_format_to_v4l2        (ArvPixelFormat pixel_format);

int             arv_v4l2_ioctl                  (int fd, int request, void *arg);
int             arv_v4l2_get_media_fd           (int fd, const char *bus_info);

gboolean        arv_v4l2_set_ctrl               (int fd, int ctrl_id, gint32 value);
gint32          arv_v4l2_get_ctrl               (int fd, int ctrl_id);

gint64          arv_v4l2_get_int64_ext_ctrl     (int fd, int ext_ctrl_class, int ext_ctrl_id);
gboolean        arv_v4l2_set_int64_ext_ctrl     (int fd, int ext_ctrl_class, int ext_ctrl_id, gint64 value);

gint32          arv_v4l2_get_int32_ext_ctrl     (int fd, int ext_ctrl_class, int ext_ctrl_id);
gboolean        arv_v4l2_set_int32_ext_ctrl     (int fd, int ext_ctrl_class, int ext_ctrl_id, gint32 value);

G_END_DECLS

#endif
