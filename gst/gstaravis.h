/*
 * Copyright © 2006 Eric Jonas <jonas@mit.edu>
 * Copyright © 2006 Antoine Tremblay <hexa00@gmail.com>
 * Copyright © 2010 United States Government, Joshua M. Doe <joshua.doe@us.army.mil>
 * Copyright © 2010 Emmanuel Pacaud <emmanuel@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef ARV_GST_H
#define ARV_GST_H

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <arv.h>

G_BEGIN_DECLS

#define GST_TYPE_ARAVIS 		(gst_aravis_get_type())
#define GST_ARAVIS(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ARAVIS,GstAravis))
#define GST_ARAVIS_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ARAVIS,GstAravis))
#define GST_IS_ARAVIS(obj) 		(G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ARAVIS))
#define GST_IS_ARAVIS_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ARAVIS))

typedef struct _GstAravis GstAravis;
typedef struct _GstAravisClass GstAravisClass;

struct _GstAravis {
	GstPushSrc element;

	char *camera_name;

	double gain;
	gboolean gain_auto;
	double exposure_time_us;
	gboolean exposure_auto;

	gint offset_x;
	gint offset_y;

	gint h_binning;
	gint v_binning;
        gboolean packet_resend;
        gint num_buffers;

	gint payload;

	guint64 buffer_timeout_us;

	ArvCamera *camera;
	ArvStream *stream;

	GstCaps *all_caps;
	GstCaps *fixed_caps;

	guint64 timestamp_offset;
	guint64 last_timestamp;
};

struct _GstAravisClass {
	GstPushSrcClass parent_class;
};

GType gst_aravis_get_type (void);

G_END_DECLS

#endif
