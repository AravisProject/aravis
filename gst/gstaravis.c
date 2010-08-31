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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-aravissrc
 *
 * Source using the Aravis vision library
 * 
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v aravissrc ! ffmpegcolorspace ! autovideosink
 * ]|
 * </refsect2>
 */

#include <gstaravis.h>
#include <time.h>
#include <string.h>

#define GST_ARAVIS_N_BUFFERS	50

GST_DEBUG_CATEGORY_STATIC (aravis_debug);
#define GST_CAT_DEFAULT aravis_debug

static GstElementDetails aravis_details =
GST_ELEMENT_DETAILS ("Aravis Video Source",
		     "Source/Video",
		     "Aravis based source",
		     "Emmanuel Pacaud <emmanuel@gnome.org>");

enum
{
  PROP_0,
  PROP_CAMERA_NAME,
  PROP_FRAME_RATE,
  PROP_GAIN,
  PROP_EXPOSURE,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_H_BINNING,
  PROP_V_BINNING
};

GST_BOILERPLATE (GstAravis, gst_aravis, GstPushSrc, GST_TYPE_PUSH_SRC);

static GstStaticPadTemplate aravis_src_template = GST_STATIC_PAD_TEMPLATE ("src",
									   GST_PAD_SRC,
									   GST_PAD_ALWAYS,
									   GST_STATIC_CAPS ("ANY"));

GstCaps *
gst_aravis_get_camera_caps (GstAravis *gst_aravis)
{
	GstCaps *gcaps = NULL;
	GstStructure *gs;
	double frame_rate;
	int height, width;

	GST_LOG_OBJECT (gst_aravis, "Get camera caps");

	gcaps = gst_caps_new_empty ();

	arv_camera_get_region (gst_aravis->camera, NULL, NULL, &width, &height);

	gs = gst_structure_empty_new ("video");
	gst_structure_set_name (gs, "video/x-raw-gray");
	gst_structure_set (gs,
			   "bpp", G_TYPE_INT, 8,
			   "depth", G_TYPE_INT, 8,
			   "width", G_TYPE_INT, width,
			   "height", G_TYPE_INT, height,
			   NULL);

	frame_rate = arv_camera_get_frame_rate (gst_aravis->camera);

	gst_structure_set(gs, "framerate", GST_TYPE_FRACTION, (gint) (100.0 * frame_rate), 100, NULL);
	gst_caps_append_structure (gcaps, gs);

	return gcaps;
}

static GstCaps *
gst_aravis_get_caps (GstBaseSrc * src)
{
	GstAravis* gst_aravis = GST_ARAVIS(src);

	if (gst_aravis->caps != NULL)
		return gst_caps_copy (gst_aravis->caps);

	return gst_caps_new_any ();
}

static gboolean
gst_aravis_start (GstBaseSrc *src)
{
	GstAravis* gst_aravis = GST_ARAVIS(src);
	int i;

	GST_LOG_OBJECT (gst_aravis, "Opening first available camera");

	if (gst_aravis->camera != NULL)
		g_object_unref (gst_aravis->camera);
	if (gst_aravis->stream != NULL)
		g_object_unref (gst_aravis->stream);

	gst_aravis->camera = arv_camera_new (gst_aravis->camera_name);
	gst_aravis->stream = arv_camera_create_stream (gst_aravis->camera, NULL, NULL);

	arv_camera_set_region (gst_aravis->camera, 0, 0, gst_aravis->width, gst_aravis->height);
	arv_camera_set_binning (gst_aravis->camera, gst_aravis->h_binning, gst_aravis->v_binning);
	arv_camera_set_pixel_format (gst_aravis->camera, ARV_PIXEL_FORMAT_MONO_8);

	GST_DEBUG_OBJECT (gst_aravis, "Frame rate = %g Hz", gst_aravis->frame_rate);
	arv_camera_set_frame_rate (gst_aravis->camera, gst_aravis->frame_rate);
	GST_DEBUG_OBJECT (gst_aravis, "Actual frame rate = %g Hz", arv_camera_get_frame_rate (gst_aravis->camera));

	GST_DEBUG_OBJECT (gst_aravis, "Gain       = %d", gst_aravis->gain);
	arv_camera_set_gain (gst_aravis->camera, gst_aravis->gain);
	GST_DEBUG_OBJECT (gst_aravis, "Actual gain       = %d", arv_camera_get_gain (gst_aravis->camera));

	GST_DEBUG_OBJECT (gst_aravis, "Exposure   = %g µs", gst_aravis->exposure_time_us);
	arv_camera_set_exposure_time (gst_aravis->camera, gst_aravis->exposure_time_us);
	GST_DEBUG_OBJECT (gst_aravis, "Actual exposure   = %g µs", arv_camera_get_exposure_time (gst_aravis->camera));

	gst_aravis->payload = arv_camera_get_payload (gst_aravis->camera);
	gst_aravis->caps = gst_aravis_get_camera_caps (gst_aravis);

	for (i = 0; i < GST_ARAVIS_N_BUFFERS; i++)
		arv_stream_push_buffer (gst_aravis->stream,
					arv_buffer_new (gst_aravis->payload, NULL));

	GST_LOG_OBJECT (gst_aravis, "Starting acquisition");

	arv_camera_start_acquisition (gst_aravis->camera);

	gst_aravis->timestamp_offset = 0;
	gst_aravis->last_timestamp = 0;

	return TRUE;
}


gboolean gst_aravis_stop( GstBaseSrc * src )
{
        GstAravis* gst_aravis = GST_ARAVIS(src);

	arv_camera_stop_acquisition (gst_aravis->camera);

	g_object_unref (gst_aravis->stream);
	g_object_unref (gst_aravis->camera);
	gst_caps_unref (gst_aravis->caps);

	gst_aravis->camera = NULL;
	gst_aravis->stream = NULL;
	gst_aravis->caps = NULL;

        GST_DEBUG_OBJECT (gst_aravis, "Capture stoped");

        return TRUE;
}

static GstFlowReturn
gst_aravis_create (GstPushSrc * push_src, GstBuffer ** buffer)
{
	GstAravis *gst_aravis;
	ArvBuffer *arv_buffer;

	gst_aravis = GST_ARAVIS (push_src);

	*buffer = gst_buffer_new ();
	do {
		arv_buffer = arv_stream_pop_buffer (gst_aravis->stream);
		if (arv_buffer == NULL)
			g_usleep (1000);
		else if (arv_buffer->status != ARV_BUFFER_STATUS_SUCCESS) {
			arv_stream_push_buffer (gst_aravis->stream, arv_buffer);
			arv_buffer = NULL;
		}
	} while (arv_buffer == NULL);

	GST_BUFFER_DATA (*buffer) = arv_buffer->data;
	GST_BUFFER_MALLOCDATA (*buffer) = NULL;
	GST_BUFFER_SIZE (*buffer) = gst_aravis->payload;

	if (gst_aravis->timestamp_offset == 0) {
		gst_aravis->timestamp_offset = arv_buffer->timestamp_ns;
		gst_aravis->last_timestamp = arv_buffer->timestamp_ns;
	}

	GST_BUFFER_TIMESTAMP (*buffer) = arv_buffer->timestamp_ns - gst_aravis->timestamp_offset;
	GST_BUFFER_DURATION (*buffer) = arv_buffer->timestamp_ns - gst_aravis->last_timestamp;

	gst_aravis->last_timestamp = arv_buffer->timestamp_ns;

	arv_stream_push_buffer (gst_aravis->stream, arv_buffer);

	gst_buffer_set_caps (*buffer, gst_aravis->caps);

	return GST_FLOW_OK;
}


static void
gst_aravis_init (GstAravis *gst_aravis, GstAravisClass *g_class)
{
	gst_base_src_set_live (GST_BASE_SRC (gst_aravis), TRUE);

	gst_aravis->camera_name = NULL;

	gst_aravis->frame_rate = -1;
	gst_aravis->gain = -1;
	gst_aravis->exposure_time_us = -1;
	gst_aravis->width = -1;
	gst_aravis->height = -1;
	gst_aravis->h_binning = -1;
	gst_aravis->v_binning = -1;
	gst_aravis->payload = 0;

	gst_aravis->camera = NULL;
	gst_aravis->stream = NULL;

	gst_aravis->caps = NULL;
}

static void
gst_aravis_finalize (GObject * object)
{
        GstAravis *gst_aravis = GST_ARAVIS (object);

	if (gst_aravis->camera != NULL) {
		g_object_unref (gst_aravis->camera);
		gst_aravis->camera = NULL;
	}
	if (gst_aravis->stream != NULL) {
		g_object_unref (gst_aravis->stream);
		gst_aravis->stream = NULL;
	}
	if (gst_aravis->caps != NULL) {
		gst_caps_unref (gst_aravis->caps);
		gst_aravis->caps = NULL;
	}

	g_free (gst_aravis->camera_name);
	gst_aravis->camera_name = NULL;

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_aravis_set_property (GObject * object, guint prop_id,
			 const GValue * value, GParamSpec * pspec)
{
	GstAravis *gst_aravis = GST_ARAVIS (object);

	switch (prop_id) {
		case PROP_CAMERA_NAME:
			g_free (gst_aravis->camera_name);
			gst_aravis->camera_name = g_strdup (g_value_get_string (value));
			break;
		case PROP_FRAME_RATE:
			gst_aravis->frame_rate = g_value_get_double (value);
			break;
		case PROP_GAIN:
			gst_aravis->gain = g_value_get_int (value);
			break;
		case PROP_EXPOSURE:
			gst_aravis->exposure_time_us = g_value_get_double (value);
			break;
		case PROP_WIDTH:
			gst_aravis->width = g_value_get_int (value);
			break;
		case PROP_HEIGHT:
			gst_aravis->height = g_value_get_int (value);
			break;
		case PROP_H_BINNING:
			gst_aravis->h_binning = g_value_get_int (value);
			break;
		case PROP_V_BINNING:
			gst_aravis->v_binning = g_value_get_int (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gst_aravis_get_property (GObject * object, guint prop_id, GValue * value,
			 GParamSpec * pspec)
{
	GstAravis *gst_aravis = GST_ARAVIS (object);

	switch (prop_id) {
		case PROP_CAMERA_NAME:
			g_value_set_string (value, gst_aravis->camera_name);
			break;
		case PROP_FRAME_RATE:
			g_value_set_double (value, gst_aravis->frame_rate);
			break;
		case PROP_GAIN:
			g_value_set_int (value, gst_aravis->gain);
			break;
		case PROP_EXPOSURE:
			g_value_set_double (value, gst_aravis->exposure_time_us);
			break;
		case PROP_WIDTH:
			g_value_set_int (value, gst_aravis->width);
			break;
		case PROP_HEIGHT:
			g_value_set_int (value, gst_aravis->height);
			break;
		case PROP_H_BINNING:
			g_value_set_int (value, gst_aravis->h_binning);
			break;
		case PROP_V_BINNING:
			g_value_set_int (value, gst_aravis->v_binning);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gst_aravis_base_init (gpointer g_class)
{
	GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

	gst_element_class_set_details (element_class, &aravis_details);

	gst_element_class_add_pad_template (element_class,
					    gst_static_pad_template_get (&aravis_src_template));
}

static void
gst_aravis_class_init (GstAravisClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GstBaseSrcClass *gstbasesrc_class = GST_BASE_SRC_CLASS (klass);
	GstPushSrcClass *gstpushsrc_class = GST_PUSH_SRC_CLASS (klass);

	gobject_class->finalize = gst_aravis_finalize;
	gobject_class->set_property = gst_aravis_set_property;
	gobject_class->get_property = gst_aravis_get_property;

	g_object_class_install_property
		(gobject_class,
		 PROP_CAMERA_NAME,
		 g_param_spec_string ("camera-name",
				      "Camera name",
				      "Name of the camera",
				      NULL,
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_FRAME_RATE,
		 g_param_spec_double ("frame-rate",
				      "Frame rate",
				      "Acquisition frame rate (in Hz)",
				      0.0, 500.0, 25.0,
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_GAIN,
		 g_param_spec_int ("gain",
				   "Gain",
				   "Gain (dB)",
				   0, 500, 0,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_EXPOSURE,
		 g_param_spec_double ("exposure",
				      "Exposure",
				      "Exposure time (µs)",
				      0.0, 100000000.0, 500.0,
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_WIDTH,
		 g_param_spec_int ("width",
				   "Image width",
				   "Image width (in pixels)",
				   1, G_MAXINT, 320,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_HEIGHT,
		 g_param_spec_int ("height",
				   "Image height",
				   "Image height (in pixels)",
				   1, G_MAXINT, 320,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_H_BINNING,
		 g_param_spec_int ("h-binning",
				   "Horizontal binning",
				   "CCD horizontal binning",
				   1, G_MAXINT, 1,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_V_BINNING,
		 g_param_spec_int ("v-binning",
				   "Vertical binning",
				   "CCD vertical binning",
				   1, G_MAXINT, 1,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

        GST_DEBUG_CATEGORY_INIT (aravis_debug, "aravis", 0, "Aravis interface");

	gstbasesrc_class->get_caps = gst_aravis_get_caps;
	gstbasesrc_class->start = gst_aravis_start;
	gstbasesrc_class->stop = gst_aravis_stop;

	gstpushsrc_class->create = gst_aravis_create;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
        return gst_element_register (plugin, "aravissrc", GST_RANK_NONE, GST_TYPE_ARAVIS);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
		   GST_VERSION_MINOR,
		   "aravissrc",
		   "Aravis Video Source",
		   plugin_init,
		   VERSION,
		   "LGPL",
		   PACKAGE_NAME,
		   "http://blogs.gnome.org/emmanuel")
