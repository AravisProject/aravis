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

#ifndef ARV_VIDEO_SINK_H
#define ARV_VIDEO_SINK_H

#include <gtk/gtk.h>
#include <gst/gst.h>

G_BEGIN_DECLS

typedef struct _ArvVideoSink ArvVideoSink;

ArvVideoSink *	arv_video_sink_new		(GtkWidget *frame);
GstElement *	arv_video_sink_get_element	(ArvVideoSink *sink);
GstSample *	arv_video_sink_get_last_sample	(ArvVideoSink *sink);
gboolean	arv_video_sink_is_gpu		(ArvVideoSink *sink);
void		arv_video_sink_free		(ArvVideoSink *sink);

G_END_DECLS

#endif
