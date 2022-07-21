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

#ifndef ARV_MISC_H
#define ARV_MISC_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>

G_BEGIN_DECLS

ARV_API guint                   arv_get_major_version           (void);
ARV_API guint                   arv_get_minor_version           (void);
ARV_API guint                   arv_get_micro_version           (void);

ARV_API const char *		arv_pixel_format_to_gst_caps_string		(ArvPixelFormat pixel_format);
ARV_API ArvPixelFormat		arv_pixel_format_from_gst_caps			(const char *name, const char *format,
                                                                                 int bpp, int depth);
ARV_API const char *		arv_pixel_format_to_gst_0_10_caps_string	(ArvPixelFormat pixel_format);
ARV_API ArvPixelFormat		arv_pixel_format_from_gst_0_10_caps		(const char *name, int bpp,
                                                                                 int depth, guint32 fourcc);

G_END_DECLS

#endif
