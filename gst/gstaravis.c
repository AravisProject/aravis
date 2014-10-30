/*
 * Copyright © 2006 Eric Jonas <jonas@mit.edu>
 * Copyright © 2006 Antoine Tremblay <hexa00@gmail.com>
 * Copyright © 2010 United States Government, Joshua M. Doe <joshua.doe@us.army.mil>
 * Copyright © 2010-2011 Emmanuel Pacaud <emmanuel@gnome.org>
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

/**
 * SECTION:element-aravissrc
 *
 * Source using the Aravis vision library
 * 
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v aravissrc ! video/x-raw,format=UYVY,width=512,height=512,framerate=25/1 ! autovideosink
 * ]|
 * </refsect2>
 */

#include <arvconfig.h>
#include <gstaravis.h>
#include <time.h>
#include <string.h>

#define GST_ARAVIS_DEFAULT_N_BUFFERS		50
#define GST_ARAVIS_BUFFER_TIMEOUT_DEFAULT	2000000

GST_DEBUG_CATEGORY_STATIC (aravis_debug);
#define GST_CAT_DEFAULT aravis_debug

enum
{
  PROP_0,
  PROP_CAMERA_NAME,
  PROP_CAMERA,
  PROP_GAIN,
  PROP_GAIN_AUTO,
  PROP_EXPOSURE,
  PROP_EXPOSURE_AUTO,
  PROP_H_BINNING,
  PROP_V_BINNING,
  PROP_OFFSET_X,
  PROP_OFFSET_Y,
  PROP_PACKET_RESEND,
  PROP_NUM_BUFFERS
};

G_DEFINE_TYPE (GstAravis, gst_aravis, GST_TYPE_PUSH_SRC);

static GstStaticPadTemplate aravis_src_template = GST_STATIC_PAD_TEMPLATE ("src",
									   GST_PAD_SRC,
									   GST_PAD_ALWAYS,
									   GST_STATIC_CAPS ("ANY"));

static GstCaps *
gst_aravis_get_all_camera_caps (GstAravis *gst_aravis)
{
	GstCaps *caps;
	gint64 *pixel_formats;
	double min_frame_rate, max_frame_rate;
	int min_height, min_width;
	int max_height, max_width;
	unsigned int n_pixel_formats, i;

	g_return_val_if_fail (GST_IS_ARAVIS (gst_aravis), NULL);

	if (!ARV_IS_CAMERA (gst_aravis->camera))
		return NULL;

	GST_LOG_OBJECT (gst_aravis, "Get all camera caps");

	arv_camera_get_width_bounds (gst_aravis->camera, &min_width, &max_width);
	arv_camera_get_height_bounds (gst_aravis->camera, &min_height, &max_height);
	pixel_formats = arv_camera_get_available_pixel_formats (gst_aravis->camera, &n_pixel_formats);
	arv_camera_get_frame_rate_bounds (gst_aravis->camera, &min_frame_rate, &max_frame_rate);

	int min_frame_rate_numerator;
	int min_frame_rate_denominator;
	gst_util_double_to_fraction (min_frame_rate, &min_frame_rate_numerator, &min_frame_rate_denominator);

	int max_frame_rate_numerator;
	int max_frame_rate_denominator;
	gst_util_double_to_fraction (max_frame_rate, &max_frame_rate_numerator, &max_frame_rate_denominator);

	caps = gst_caps_new_empty ();
	for (i = 0; i < n_pixel_formats; i++) {
		const char *caps_string;

		caps_string = arv_pixel_format_to_gst_caps_string (pixel_formats[i]);

		if (caps_string != NULL) {
			GstStructure *structure;

			structure = gst_structure_from_string (caps_string, NULL);
			gst_structure_set (structure,
					   "width", GST_TYPE_INT_RANGE, min_width, max_width,
					   "height", GST_TYPE_INT_RANGE, min_height, max_height,
					   "framerate", GST_TYPE_FRACTION_RANGE,
							   min_frame_rate_numerator, min_frame_rate_denominator,
							   max_frame_rate_numerator, max_frame_rate_denominator,
					   NULL);
			gst_caps_append_structure (caps, structure);
		}
	}

	g_free (pixel_formats);

	return caps;
}

static GstCaps *
gst_aravis_get_caps (GstBaseSrc * src, GstCaps * filter)
{
	GstAravis* gst_aravis = GST_ARAVIS(src);
	GstCaps *caps;

	if (gst_aravis->all_caps != NULL)
		caps = gst_caps_copy (gst_aravis->all_caps);
	else
		caps = gst_caps_new_any ();

	GST_LOG_OBJECT (gst_aravis, "Available caps = %" GST_PTR_FORMAT, caps);

	return caps;
}

static gboolean
gst_aravis_set_caps (GstBaseSrc *src, GstCaps *caps)
{
	GstAravis* gst_aravis = GST_ARAVIS(src);
	GstStructure *structure;
	ArvPixelFormat pixel_format;
	int height, width;
	const GValue *frame_rate;
	const char *caps_string;
	const char *format_string;
	unsigned int i;

	GST_LOG_OBJECT (gst_aravis, "Requested caps = %" GST_PTR_FORMAT, caps);

	arv_camera_stop_acquisition (gst_aravis->camera);

	if (gst_aravis->stream != NULL)
		g_object_unref (gst_aravis->stream);

	structure = gst_caps_get_structure (caps, 0);

	gst_structure_get_int (structure, "width", &width);
	gst_structure_get_int (structure, "height", &height);
	frame_rate = gst_structure_get_value (structure, "framerate");
	format_string = gst_structure_get_string (structure, "format");

	pixel_format = arv_pixel_format_from_gst_caps (gst_structure_get_name (structure), format_string);

	arv_camera_set_region (gst_aravis->camera, gst_aravis->offset_x, gst_aravis->offset_y, width, height);
	arv_camera_set_binning (gst_aravis->camera, gst_aravis->h_binning, gst_aravis->v_binning);
	arv_camera_set_pixel_format (gst_aravis->camera, pixel_format);

	if (frame_rate != NULL) {
		double dbl_frame_rate;

		dbl_frame_rate = (double) gst_value_get_fraction_numerator (frame_rate) /
			(double) gst_value_get_fraction_denominator (frame_rate);

		GST_DEBUG_OBJECT (gst_aravis, "Frame rate = %g Hz", dbl_frame_rate);
		arv_camera_set_frame_rate (gst_aravis->camera, dbl_frame_rate);

		if (dbl_frame_rate > 0.0)
			gst_aravis->buffer_timeout_us = MAX (GST_ARAVIS_BUFFER_TIMEOUT_DEFAULT,
							     3e6 / dbl_frame_rate);
		else
			gst_aravis->buffer_timeout_us = GST_ARAVIS_BUFFER_TIMEOUT_DEFAULT;
	} else
		gst_aravis->buffer_timeout_us = GST_ARAVIS_BUFFER_TIMEOUT_DEFAULT;

	GST_DEBUG_OBJECT (gst_aravis, "Buffer timeout = %" G_GUINT64_FORMAT " µs", gst_aravis->buffer_timeout_us);

	GST_DEBUG_OBJECT (gst_aravis, "Actual frame rate = %g Hz", arv_camera_get_frame_rate (gst_aravis->camera));

	if(gst_aravis->gain_auto) {
		arv_camera_set_gain_auto (gst_aravis->camera, ARV_AUTO_CONTINUOUS);
		GST_DEBUG_OBJECT (gst_aravis, "Auto Gain = continuous");
	} else {
		if (gst_aravis->gain >= 0) {
			GST_DEBUG_OBJECT (gst_aravis, "Gain = %g", gst_aravis->gain);
			arv_camera_set_gain_auto (gst_aravis->camera, ARV_AUTO_OFF);
			arv_camera_set_gain (gst_aravis->camera, gst_aravis->gain);
		}
		GST_DEBUG_OBJECT (gst_aravis, "Actual gain = %g", arv_camera_get_gain (gst_aravis->camera));
	}

	if(gst_aravis->exposure_auto) {
		arv_camera_set_exposure_time_auto (gst_aravis->camera, ARV_AUTO_CONTINUOUS);
		GST_DEBUG_OBJECT (gst_aravis, "Auto Exposure = continuous");
	} else {
		if (gst_aravis->exposure_time_us > 0.0) {
			GST_DEBUG_OBJECT (gst_aravis, "Exposure = %g µs", gst_aravis->exposure_time_us);
			arv_camera_set_exposure_time_auto (gst_aravis->camera, ARV_AUTO_OFF);
			arv_camera_set_exposure_time (gst_aravis->camera, gst_aravis->exposure_time_us);
		}
		GST_DEBUG_OBJECT (gst_aravis, "Actual exposure = %g µs", arv_camera_get_exposure_time (gst_aravis->camera));
	}

	if (gst_aravis->fixed_caps != NULL)
		gst_caps_unref (gst_aravis->fixed_caps);

	caps_string = arv_pixel_format_to_gst_caps_string (pixel_format);
	if (caps_string != NULL) {
		GstStructure *structure;
		GstCaps *caps;

		caps = gst_caps_new_empty ();
		structure = gst_structure_from_string (caps_string, NULL);
		gst_structure_set (structure,
				   "width", G_TYPE_INT, width,
				   "height", G_TYPE_INT, height,
				   NULL);

		if (frame_rate != NULL)
			gst_structure_set_value (structure, "framerate", frame_rate);

		gst_caps_append_structure (caps, structure);

		gst_aravis->fixed_caps = caps;
	} else
		gst_aravis->fixed_caps = NULL;

	gst_aravis->payload = arv_camera_get_payload (gst_aravis->camera);
	gst_aravis->stream = arv_camera_create_stream (gst_aravis->camera, NULL, NULL);

	if (ARV_IS_GV_STREAM (gst_aravis->stream) && gst_aravis->packet_resend)
                g_object_set (gst_aravis->stream, "packet-resend", ARV_GV_STREAM_PACKET_RESEND_ALWAYS, NULL);
        else
                g_object_set (gst_aravis->stream, "packet-resend", ARV_GV_STREAM_PACKET_RESEND_NEVER, NULL);

	for (i = 0; i < gst_aravis->num_buffers; i++)
		arv_stream_push_buffer (gst_aravis->stream,
					arv_buffer_new (gst_aravis->payload, NULL));

	GST_LOG_OBJECT (gst_aravis, "Start acquisition");
	arv_camera_start_acquisition (gst_aravis->camera);

	gst_aravis->timestamp_offset = 0;
	gst_aravis->last_timestamp = 0;

	return TRUE;
}

void
gst_aravis_init_camera (GstAravis *gst_aravis)
{
	if (gst_aravis->camera != NULL)
		g_object_unref (gst_aravis->camera);

	gst_aravis->camera = arv_camera_new (gst_aravis->camera_name);

	gst_aravis->gain = arv_camera_get_gain(gst_aravis->camera);
	gst_aravis->gain_auto = arv_camera_is_gain_available(gst_aravis->camera);

	gst_aravis->exposure_time_us = arv_camera_get_exposure_time(gst_aravis->camera);
	if (arv_camera_get_exposure_time_auto(gst_aravis->camera) == ARV_AUTO_OFF)
		gst_aravis->exposure_auto = FALSE;
	else
		gst_aravis->exposure_auto = TRUE;

	arv_camera_get_region (gst_aravis->camera, &gst_aravis->offset_x, &gst_aravis->offset_y, NULL, NULL);
	arv_camera_get_binning (gst_aravis->camera, &gst_aravis->h_binning, &gst_aravis->v_binning);
	gst_aravis->payload = 0;
}

static gboolean
gst_aravis_start (GstBaseSrc *src)
{
	GstAravis* gst_aravis = GST_ARAVIS(src);

	GST_LOG_OBJECT (gst_aravis, "Open camera '%s'", gst_aravis->camera_name);

	if (gst_aravis->camera == NULL)
		gst_aravis_init_camera (gst_aravis);

	gst_aravis->all_caps = gst_aravis_get_all_camera_caps (gst_aravis);

	return TRUE;
}


gboolean gst_aravis_stop( GstBaseSrc * src )
{
        GstAravis* gst_aravis = GST_ARAVIS(src);

	arv_camera_stop_acquisition (gst_aravis->camera);

	if (gst_aravis->stream != NULL) {
		g_object_unref (gst_aravis->stream);
		gst_aravis->stream = NULL;

	}
	if (gst_aravis->all_caps != NULL) {
		gst_caps_unref (gst_aravis->all_caps);
		gst_aravis->all_caps = NULL;
	}

        GST_DEBUG_OBJECT (gst_aravis, "Stop acquisition");

        return TRUE;
}

static void
gst_aravis_get_times (GstBaseSrc * basesrc, GstBuffer * buffer,
		      GstClockTime * start, GstClockTime * end)
{
	if (gst_base_src_is_live (basesrc)) {
		GstClockTime timestamp = GST_BUFFER_PTS (buffer);

		if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
			GstClockTime duration = GST_BUFFER_DURATION (buffer);

			if (GST_CLOCK_TIME_IS_VALID (duration)) {
				*end = timestamp + duration;
			}
			*start = timestamp;
		}
	} else {
		*start = -1;
		*end = -1;
	}
}

static GstFlowReturn
gst_aravis_create (GstPushSrc * push_src, GstBuffer ** buffer)
{
	GstAravis *gst_aravis;
	ArvBuffer *arv_buffer;
	int arv_row_stride;
	int width, height;
	char *buffer_data;
	size_t buffer_size;
	guint64 timestamp_ns;

	gst_aravis = GST_ARAVIS (push_src);

	do {
		arv_buffer = arv_stream_timeout_pop_buffer (gst_aravis->stream, gst_aravis->buffer_timeout_us);
		if (arv_buffer != NULL && arv_buffer_get_status (arv_buffer) != ARV_BUFFER_STATUS_SUCCESS)
			arv_stream_push_buffer (gst_aravis->stream, arv_buffer);
	} while (arv_buffer != NULL && arv_buffer_get_status (arv_buffer) != ARV_BUFFER_STATUS_SUCCESS);

	if (arv_buffer == NULL)
		return GST_FLOW_ERROR;

	buffer_data = (char *) arv_buffer_get_data (arv_buffer, &buffer_size);
	arv_buffer_get_image_region (arv_buffer, NULL, NULL, &width, &height);
	arv_row_stride = width * ARV_PIXEL_FORMAT_BIT_PER_PIXEL (arv_buffer_get_image_pixel_format (arv_buffer)) / 8;
	timestamp_ns = arv_buffer_get_timestamp (arv_buffer);

	/* Gstreamer requires row stride to be a multiple of 4 */
	if ((arv_row_stride & 0x3) != 0) {
		int gst_row_stride;
		size_t size;
		void *data;
		int i;

		gst_row_stride = (arv_row_stride & ~(0x3)) + 4;

		size = height * gst_row_stride;
		data = g_malloc (size);

		for (i = 0; i < height; i++)
			memcpy (((char *) data) + i * gst_row_stride, buffer_data + i * arv_row_stride, arv_row_stride);

		*buffer = gst_buffer_new_wrapped (data, size);
	} else {
		*buffer = gst_buffer_new_wrapped_full (0, buffer_data, buffer_size, 0, buffer_size, NULL, NULL);
	}

	if (!gst_base_src_get_do_timestamp(GST_BASE_SRC(push_src))) {
		if (gst_aravis->timestamp_offset == 0) {
			gst_aravis->timestamp_offset = timestamp_ns;
			gst_aravis->last_timestamp = timestamp_ns;
		}

		GST_BUFFER_PTS (*buffer) = timestamp_ns - gst_aravis->timestamp_offset;
		GST_BUFFER_DURATION (*buffer) = timestamp_ns - gst_aravis->last_timestamp;

		gst_aravis->last_timestamp = timestamp_ns;
	}

	arv_stream_push_buffer (gst_aravis->stream, arv_buffer);

	return GST_FLOW_OK;
}

static GstCaps *
gst_aravis_fixate_caps (GstBaseSrc * bsrc, GstCaps * caps)
{
	GstAravis *gst_aravis = GST_ARAVIS (bsrc);
	GstStructure *structure;
	gint width;
	gint height;
	double frame_rate;

	arv_camera_get_region (gst_aravis->camera, NULL, NULL, &width, &height);
	frame_rate = arv_camera_get_frame_rate (gst_aravis->camera);

	structure = gst_caps_get_structure (caps, 0);

	gst_structure_fixate_field_nearest_int (structure, "width", width);
	gst_structure_fixate_field_nearest_int (structure, "height", height);
	gst_structure_fixate_field_nearest_fraction (structure, "framerate", (double) (0.5 + frame_rate), 1);

	GST_LOG_OBJECT (gst_aravis, "Fixate caps");

	return GST_BASE_SRC_CLASS(gst_aravis_parent_class)->fixate(bsrc, caps);
}

static void
gst_aravis_init (GstAravis *gst_aravis)
{
	gst_base_src_set_live (GST_BASE_SRC (gst_aravis), TRUE);
	gst_base_src_set_format (GST_BASE_SRC (gst_aravis), GST_FORMAT_TIME);

	gst_aravis->camera_name = NULL;

	gst_aravis->gain = -1;
	gst_aravis->gain_auto = FALSE;
	gst_aravis->exposure_time_us = -1;
	gst_aravis->exposure_auto = FALSE;
	gst_aravis->offset_x = 0;
	gst_aravis->offset_y = 0;
	gst_aravis->h_binning = -1;
	gst_aravis->v_binning = -1;
        gst_aravis->packet_resend = TRUE;
        gst_aravis->num_buffers = GST_ARAVIS_DEFAULT_N_BUFFERS;
	gst_aravis->payload = 0;

	gst_aravis->buffer_timeout_us = GST_ARAVIS_BUFFER_TIMEOUT_DEFAULT;

	gst_aravis->camera = NULL;
	gst_aravis->stream = NULL;

	gst_aravis->all_caps = NULL;
	gst_aravis->fixed_caps = NULL;
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
	if (gst_aravis->all_caps != NULL) {
		gst_caps_unref (gst_aravis->all_caps);
		gst_aravis->all_caps = NULL;
	}
	if (gst_aravis->fixed_caps != NULL) {
		gst_caps_unref (gst_aravis->fixed_caps);
		gst_aravis->fixed_caps = NULL;
	}

	g_free (gst_aravis->camera_name);
	gst_aravis->camera_name = NULL;

        G_OBJECT_CLASS (gst_aravis_parent_class)->finalize (object);
}

static void
gst_aravis_set_property (GObject * object, guint prop_id,
			 const GValue * value, GParamSpec * pspec)
{
	GstAravis *gst_aravis = GST_ARAVIS (object);

	switch (prop_id) {
		case PROP_CAMERA_NAME:
			g_free (gst_aravis->camera_name);
			/* check if we are currently active
			   prevent setting camera and other values to something not representing the active camera */
			if (gst_aravis->stream == NULL) {
				gst_aravis->camera_name = g_strdup (g_value_get_string (value));
				gst_aravis_init_camera (gst_aravis);
			}

			GST_LOG_OBJECT (gst_aravis, "Set camera name to %s", gst_aravis->camera_name);

			break;
		case PROP_GAIN:
			gst_aravis->gain = g_value_get_double (value);
			if (gst_aravis->camera != NULL)
				arv_camera_set_gain (gst_aravis->camera, gst_aravis->gain);
			break;
		case PROP_GAIN_AUTO:
			gst_aravis->gain_auto = g_value_get_boolean (value);
			if (gst_aravis->camera != NULL)
				arv_camera_set_gain_auto (gst_aravis->camera, gst_aravis->gain_auto);
			break;
		case PROP_EXPOSURE:
			gst_aravis->exposure_time_us = g_value_get_double (value);
			if (gst_aravis->camera != NULL)
				arv_camera_set_exposure_time (gst_aravis->camera, gst_aravis->exposure_time_us);
			break;
		case PROP_EXPOSURE_AUTO:
			gst_aravis->exposure_auto = g_value_get_boolean (value);
			if (gst_aravis->camera != NULL)
				arv_camera_set_exposure_time_auto (gst_aravis->camera, gst_aravis->exposure_auto);
			break;
		case PROP_OFFSET_X:
			gst_aravis->offset_x = g_value_get_int (value);
			break;
		case PROP_OFFSET_Y:
			gst_aravis->offset_y = g_value_get_int (value);
			break;
		case PROP_H_BINNING:
			gst_aravis->h_binning = g_value_get_int (value);
			break;
		case PROP_V_BINNING:
			gst_aravis->v_binning = g_value_get_int (value);
			break;
                case PROP_PACKET_RESEND:
                        gst_aravis->packet_resend = g_value_get_boolean (value);
                        break;
                case PROP_NUM_BUFFERS:
                        gst_aravis->num_buffers = g_value_get_int (value);
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
		case PROP_CAMERA:
			g_value_set_object (value, gst_aravis->camera);
			break;
		case PROP_GAIN:
			g_value_set_double (value, gst_aravis->gain);
			break;
		case PROP_GAIN_AUTO:
			g_value_set_boolean (value, gst_aravis->gain_auto);
			break;
		case PROP_EXPOSURE:
			g_value_set_double (value, gst_aravis->exposure_time_us);
			break;
		case PROP_EXPOSURE_AUTO:
			g_value_set_boolean (value, gst_aravis->exposure_auto);
			break;
		case PROP_OFFSET_X:
			g_value_set_int (value, gst_aravis->offset_x);
			break;
		case PROP_OFFSET_Y:
			g_value_set_int (value, gst_aravis->offset_y);
			break;
		case PROP_H_BINNING:
			g_value_set_int (value, gst_aravis->h_binning);
			break;
		case PROP_V_BINNING:
			g_value_set_int (value, gst_aravis->v_binning);
			break;
        	case PROP_PACKET_RESEND:
                        g_value_set_boolean (value, gst_aravis->packet_resend);
                        break;
        	case PROP_NUM_BUFFERS:
                        g_value_set_int (value, gst_aravis->num_buffers);
                        break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gst_aravis_class_init (GstAravisClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
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
		 PROP_CAMERA,
		 g_param_spec_object ("camera",
				      "Camera Object",
				      "Camera instance to retrieve additional information",
				              ARV_TYPE_CAMERA,
				      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_GAIN,
		 g_param_spec_double ("gain",
				   "Gain",
				   "Gain (dB)",
				   -1.0, 500.0, 0.0,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_GAIN_AUTO,
		 g_param_spec_boolean ("gain-auto",
				       "Auto Gain",
				       "Auto Gain Mode",
				       TRUE,
				       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_EXPOSURE,
		 g_param_spec_double ("exposure",
				      "Exposure",
				      "Exposure time (µs)",
				      -1, 100000000.0, 500.0,
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_EXPOSURE_AUTO,
		 g_param_spec_boolean ("exposure-auto",
				       "Auto Exposure",
				       "Auto Exposure Mode",
				       TRUE,
				       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_OFFSET_X,
		 g_param_spec_int ("offset-x",
				   "x Offset",
				   "Offset in x direction",
				   0, G_MAXINT, 0,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_OFFSET_Y,
		 g_param_spec_int ("offset-y",
				   "y Offset",
				   "Offset in y direction",
				   0, G_MAXINT, 0,
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

	g_object_class_install_property
		(gobject_class,
		 PROP_PACKET_RESEND,
		 g_param_spec_boolean ("packet-resend",
				       "Packet Resend",
				       "Request dropped packets to be reissued by the camera",
				       TRUE,
				       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property
		(gobject_class,
		 PROP_NUM_BUFFERS,
		 g_param_spec_int ("num-buffers",
                                   "Number of Buffers",
                                   "Number of video buffers to allocate for video frames",
                                   1, G_MAXINT, GST_ARAVIS_DEFAULT_N_BUFFERS,
                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

        GST_DEBUG_CATEGORY_INIT (aravis_debug, "aravissrc", 0, "Aravis interface");

	gst_element_class_set_details_simple (element_class,
					      "Aravis Video Source",
					      "Source/Video",
					      "Aravis based source",
					      "Emmanuel Pacaud <emmanuel@gnome.org>");
	gst_element_class_add_pad_template (element_class,
					    gst_static_pad_template_get (&aravis_src_template));

	gstbasesrc_class->get_caps = gst_aravis_get_caps;
	gstbasesrc_class->set_caps = gst_aravis_set_caps;
	gstbasesrc_class->fixate = gst_aravis_fixate_caps;
	gstbasesrc_class->start = gst_aravis_start;
	gstbasesrc_class->stop = gst_aravis_stop;

	gstbasesrc_class->get_times = gst_aravis_get_times;

	gstpushsrc_class->create = gst_aravis_create;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
        return gst_element_register (plugin, "aravissrc", GST_RANK_NONE, GST_TYPE_ARAVIS);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
		   GST_VERSION_MINOR,
		   aravissrc,
		   "Aravis Video Source",
		   plugin_init,
		   VERSION,
		   "LGPL",
		   PACKAGE_NAME,
		   "http://blogs.gnome.org/emmanuel")
