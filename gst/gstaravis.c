/*
 * Copyright © 2006 Eric Jonas <jonas@mit.edu>
 * Copyright © 2006 Antoine Tremblay <hexa00@gmail.com>
 * Copyright © 2010 United States Government, Joshua M. Doe <joshua.doe@us.army.mil>
 * Copyright © 2010-2019 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
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

#include <gstaravis.h>
#include <arvgvspprivate.h>
#include <time.h>
#include <string.h>

/* TODO: Add l10n */
#define _(x) (x)

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
  PROP_PACKET_DELAY,
  PROP_PACKET_SIZE,
  PROP_AUTO_PACKET_SIZE,
  PROP_PACKET_RESEND,
  PROP_FEATURES,
  PROP_NUM_ARV_BUFFERS,
  PROP_USB_MODE
};

#define GST_TYPE_ARV_AUTO (gst_arv_auto_get_type())
static GType
gst_arv_auto_get_type (void)
{
	static GType arv_auto_type = 0;

	static const GEnumValue arv_autos[] = {
		{ARV_AUTO_OFF, "Off", "off"},
		{ARV_AUTO_ONCE, "Once", "once"},
		{ARV_AUTO_CONTINUOUS, "Continuous", "on"},
		{0, NULL, NULL},
	};

	if (!arv_auto_type)
	{
		arv_auto_type = g_enum_register_static("GstArvAuto", arv_autos);
	}
	return arv_auto_type;
}

#define GST_TYPE_ARV_USB_MODE (gst_arv_usb_mode_get_type())
static GType
gst_arv_usb_mode_get_type (void)
{
	static GType arv_usb_mode_type = 0;

	static const GEnumValue arv_usb_modes[] = {
		{ARV_UV_USB_MODE_SYNC, "Synchronous", "sync"},
		{ARV_UV_USB_MODE_ASYNC, "Asynchronous", "async"},
		{ARV_UV_USB_MODE_DEFAULT, "Default", "default"},
		{0, NULL, NULL},
	};

	if (!arv_usb_mode_type)
	{
		arv_usb_mode_type = g_enum_register_static("GstArvUsbMode", arv_usb_modes);
	}
	return arv_usb_mode_type;
}

G_DEFINE_TYPE (GstAravis, gst_aravis, GST_TYPE_PUSH_SRC);

static GstStaticPadTemplate aravis_src_template = GST_STATIC_PAD_TEMPLATE ("src",
									   GST_PAD_SRC,
									   GST_PAD_ALWAYS,
									   GST_STATIC_CAPS ("ANY"));

static GstCaps *
gst_aravis_get_all_camera_caps (GstAravis *gst_aravis, GError **error)
{
	GError *local_error = NULL;
	GstCaps *caps;
	gint64 *pixel_formats = NULL;
	double min_frame_rate, max_frame_rate;
	int min_height, min_width;
	int max_height, max_width;
	unsigned int n_pixel_formats, i;
	int min_frame_rate_numerator;
	int min_frame_rate_denominator;
	int max_frame_rate_numerator;
	int max_frame_rate_denominator;
	gboolean is_frame_rate_available;

	g_return_val_if_fail (GST_IS_ARAVIS (gst_aravis), NULL);

	if (!ARV_IS_CAMERA (gst_aravis->camera))
		return NULL;

	GST_LOG_OBJECT (gst_aravis, "Get all camera caps");

	arv_camera_get_width_bounds (gst_aravis->camera, &min_width, &max_width, &local_error);
	if (!local_error) arv_camera_get_height_bounds (gst_aravis->camera, &min_height, &max_height, &local_error);
	if (!local_error) pixel_formats = arv_camera_dup_available_pixel_formats (gst_aravis->camera, &n_pixel_formats,
										  &local_error);
	is_frame_rate_available = arv_camera_is_frame_rate_available (gst_aravis->camera, NULL);
	if (is_frame_rate_available) {
		if (!local_error) arv_camera_get_frame_rate_bounds (gst_aravis->camera,
								    &min_frame_rate, &max_frame_rate, &local_error);
		if (!local_error) {
			gst_util_double_to_fraction (min_frame_rate, &min_frame_rate_numerator, &min_frame_rate_denominator);
			gst_util_double_to_fraction (max_frame_rate, &max_frame_rate_numerator, &max_frame_rate_denominator);
		}
	}
	if (local_error) {
		g_propagate_error (error, local_error);
		return NULL;
	}

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
					   NULL);
			if (is_frame_rate_available)
				gst_structure_set (structure,
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

	GST_OBJECT_LOCK (gst_aravis);
	if (gst_aravis->all_caps != NULL)
		caps = gst_caps_copy (gst_aravis->all_caps);
	else
		caps = gst_caps_new_any ();
	GST_OBJECT_UNLOCK (gst_aravis);

	GST_LOG_OBJECT (gst_aravis, "Available caps = %" GST_PTR_FORMAT, caps);

	return caps;
}

static gboolean
gst_aravis_set_caps (GstBaseSrc *src, GstCaps *caps)
{
	GError *error = NULL;
	GstAravis* gst_aravis = GST_ARAVIS(src);
	GstStructure *structure;
	ArvPixelFormat pixel_format;
	gint height, width;
	gint current_height, current_width;
	int depth = 0, bpp = 0;
	const GValue *frame_rate = NULL;
	const char *caps_string;
	const char *format_string;
	unsigned int i;
	ArvStream *orig_stream = NULL;
	GstCaps *orig_fixed_caps = NULL;
	gboolean result = FALSE;
	gboolean is_frame_rate_available;
	gboolean is_gain_available;
	gboolean is_gain_auto_available;
	gboolean is_exposure_time_available;
	gboolean is_exposure_auto_available;

	GST_LOG_OBJECT (gst_aravis, "Requested caps = %" GST_PTR_FORMAT, caps);

	structure = gst_caps_get_structure (caps, 0);

	GST_OBJECT_LOCK (gst_aravis);
	arv_camera_get_region (gst_aravis->camera, NULL, NULL, &current_width, &current_height, &error);
	if (error)
		goto errored;

        width = current_width;
        height = current_height;

	is_frame_rate_available = arv_camera_is_frame_rate_available (gst_aravis->camera, NULL);
	is_gain_available = arv_camera_is_gain_available (gst_aravis->camera, NULL);
	is_gain_auto_available = arv_camera_is_gain_auto_available (gst_aravis->camera, NULL);
	is_exposure_time_available = arv_camera_is_exposure_time_available (gst_aravis->camera, NULL);
	is_exposure_auto_available = arv_camera_is_exposure_auto_available (gst_aravis->camera, NULL);

	gst_structure_get_int (structure, "width", &width);
	gst_structure_get_int (structure, "height", &height);
	gst_structure_get_int (structure, "depth", &depth);
	gst_structure_get_int (structure, "bpp", &bpp);
	if (is_frame_rate_available)
		frame_rate = gst_structure_get_value (structure, "framerate");
	format_string = gst_structure_get_string (structure, "format");

	pixel_format = arv_pixel_format_from_gst_caps (gst_structure_get_name (structure), format_string, bpp, depth);

	if (!pixel_format) {
		GST_ERROR_OBJECT (src, "did not find matching pixel_format");
		goto failed;
	}

	arv_camera_stop_acquisition (gst_aravis->camera, &error);

	orig_stream = g_steal_pointer (&gst_aravis->stream);

	if (!error) arv_camera_set_pixel_format (gst_aravis->camera, pixel_format, &error);
	if (!error) arv_camera_set_binning (gst_aravis->camera, gst_aravis->h_binning, gst_aravis->v_binning, &error);
	if (!error) {
                if (width != current_width || height != current_height)
                        arv_camera_set_region (gst_aravis->camera,
                                               gst_aravis->offset_x, gst_aravis->offset_y,
                                               width, height, &error);
                else
                        arv_camera_set_region (gst_aravis->camera,
                                               gst_aravis->offset_x, gst_aravis->offset_y,
                                               -1, -1, &error);
        }

	if (!error && arv_camera_is_gv_device (gst_aravis->camera)) {
		if (gst_aravis->packet_delay >= 0) {
			gint64 delay = 0;
			arv_camera_gv_set_packet_delay (gst_aravis->camera, gst_aravis->packet_delay, &error);
			if (!error) delay = arv_camera_gv_get_packet_delay (gst_aravis->camera, &error);
			if (!error && delay != gst_aravis->packet_delay)
				GST_WARNING_OBJECT (gst_aravis, "Packet delay is %" G_GINT64_FORMAT " ns instead of %" G_GINT64_FORMAT,
					delay, gst_aravis->packet_delay);
		}
		if (!error && gst_aravis->packet_size > 0)
			arv_camera_gv_set_packet_size (gst_aravis->camera, gst_aravis->packet_size, &error);
		if (!error && gst_aravis->auto_packet_size)
			arv_camera_gv_auto_packet_size (gst_aravis->camera, &error);
	}

	if (!error && frame_rate != NULL) {

		gst_aravis->frame_rate = (double) gst_value_get_fraction_numerator (frame_rate) /
			(double) gst_value_get_fraction_denominator (frame_rate);

		GST_DEBUG_OBJECT (gst_aravis, "Frame rate = %g Hz", gst_aravis->frame_rate);
		arv_camera_set_frame_rate (gst_aravis->camera, gst_aravis->frame_rate, &error);

		if (gst_aravis->frame_rate > 0.0)
			gst_aravis->buffer_timeout_us = MAX (GST_ARAVIS_BUFFER_TIMEOUT_DEFAULT,
                                                 3e6 / gst_aravis->frame_rate);
		else
			gst_aravis->buffer_timeout_us = GST_ARAVIS_BUFFER_TIMEOUT_DEFAULT;
	} else
		gst_aravis->buffer_timeout_us = GST_ARAVIS_BUFFER_TIMEOUT_DEFAULT;

	GST_DEBUG_OBJECT (gst_aravis, "Buffer timeout = %" G_GUINT64_FORMAT " µs", gst_aravis->buffer_timeout_us);

	if (is_frame_rate_available)
		GST_DEBUG_OBJECT (gst_aravis, "Actual frame rate = %g Hz",
				  arv_camera_get_frame_rate (gst_aravis->camera, NULL));

	if (is_gain_auto_available && !error && gst_aravis->gain_auto_set) {
		arv_camera_set_gain_auto (gst_aravis->camera, gst_aravis->gain_auto, &error);
		GST_DEBUG_OBJECT (gst_aravis, "Auto Gain = %s", arv_auto_to_string(gst_aravis->gain_auto));
	}
	if (is_gain_available && gst_aravis->gain_auto == ARV_AUTO_OFF) {
		if (gst_aravis->gain >= 0) {
			GST_DEBUG_OBJECT (gst_aravis, "Gain = %g", gst_aravis->gain);
			if (is_gain_auto_available && !error && !gst_aravis->gain_auto_set)
				arv_camera_set_gain_auto (gst_aravis->camera, ARV_AUTO_OFF, &error);
			if (!error) arv_camera_set_gain (gst_aravis->camera, gst_aravis->gain, &error);
		}
		GST_DEBUG_OBJECT (gst_aravis, "Actual gain = %g", arv_camera_get_gain (gst_aravis->camera, NULL));
	}

	if (is_exposure_auto_available && !error && gst_aravis->exposure_auto_set) {
		arv_camera_set_exposure_time_auto (gst_aravis->camera, gst_aravis->exposure_auto, &error);
		GST_DEBUG_OBJECT (gst_aravis, "Auto Exposure = %s", arv_auto_to_string(gst_aravis->exposure_auto));
	}
	if (is_exposure_time_available && gst_aravis->exposure_auto == ARV_AUTO_OFF) {
		if (gst_aravis->exposure_time_us > 0.0) {
			GST_DEBUG_OBJECT (gst_aravis, "Exposure = %g µs", gst_aravis->exposure_time_us);
			if (is_exposure_auto_available && !error && !gst_aravis->exposure_auto_set)
				arv_camera_set_exposure_time_auto (gst_aravis->camera, ARV_AUTO_OFF, &error);
			if (!error) arv_camera_set_exposure_time (gst_aravis->camera, gst_aravis->exposure_time_us, &error);
		}
		GST_DEBUG_OBJECT (gst_aravis, "Actual exposure = %g µs", arv_camera_get_exposure_time (gst_aravis->camera, NULL));
	}

	orig_fixed_caps = g_steal_pointer (&gst_aravis->fixed_caps);

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

	if (!error) arv_device_set_features_from_string (arv_camera_get_device (gst_aravis->camera), gst_aravis->features, &error);

	if (!error) gst_aravis->payload = arv_camera_get_payload (gst_aravis->camera, &error);
	if (!error) gst_aravis->stream = arv_camera_create_stream (gst_aravis->camera, NULL, NULL, &error);
	if (error)
		goto errored;

	if (ARV_IS_GV_STREAM (gst_aravis->stream)) {
		if (gst_aravis->packet_resend)
			g_object_set (gst_aravis->stream, "packet-resend", ARV_GV_STREAM_PACKET_RESEND_ALWAYS, NULL);
		else
			g_object_set (gst_aravis->stream, "packet-resend", ARV_GV_STREAM_PACKET_RESEND_NEVER, NULL);
	}

	for (i = 0; i < gst_aravis->num_arv_buffers; i++)
		arv_stream_push_buffer (gst_aravis->stream,
					arv_buffer_new (gst_aravis->payload, NULL));

	GST_LOG_OBJECT (gst_aravis, "Start acquisition");
	arv_camera_start_acquisition (gst_aravis->camera, &error);

	gst_aravis->timestamp_offset = 0;
	gst_aravis->last_timestamp = 0;

	if (error)
		goto errored;

	GST_OBJECT_UNLOCK (gst_aravis);

	result = TRUE;
	goto unref;

errored:
	GST_OBJECT_UNLOCK (gst_aravis);
	GST_ELEMENT_ERROR (gst_aravis, RESOURCE, WRITE,
		(_("Could not set caps on camera \"%s\": %s"),
			gst_aravis->camera_name ? gst_aravis->camera_name : "",
			error->message),
		(NULL));
	g_error_free (error);
	goto unref;

failed:
	GST_OBJECT_UNLOCK (gst_aravis);
unref:
	if (orig_stream != NULL)
		g_object_unref (orig_stream);
	if (orig_fixed_caps != NULL)
		gst_caps_unref (orig_fixed_caps);
	return result;
}

static gboolean
gst_aravis_init_camera (GstAravis *gst_aravis, GError **error)
{
	GError *local_error = NULL;

	if (gst_aravis->camera != NULL)
		g_object_unref (gst_aravis->camera);

	gst_aravis->camera = arv_camera_new (gst_aravis->camera_name, &local_error);

	if (!local_error) arv_camera_get_region (gst_aravis->camera, &gst_aravis->offset_x,
                                                 &gst_aravis->offset_y, NULL, NULL, &local_error);
	if (!local_error) gst_aravis->payload = 0;
	if (!local_error && arv_camera_is_uv_device (gst_aravis->camera))
                arv_camera_uv_set_usb_mode (gst_aravis->camera, gst_aravis->usb_mode);

	if (local_error) {
		g_clear_object (&gst_aravis->camera);
		g_propagate_error (error, local_error);
		return FALSE;
	}

	return TRUE;
}

static void
gst_aravis_init_error (GstAravis *gst_aravis, GError *error)
{
	if (error->domain == ARV_DEVICE_ERROR && error->code == ARV_DEVICE_ERROR_NOT_FOUND) {
		GST_ELEMENT_ERROR (gst_aravis, RESOURCE, NOT_FOUND,
			(_("Could not find camera \"%s\": %s"),
			 gst_aravis->camera_name ? gst_aravis->camera_name : "",
			 error->message),
			(NULL));
	} else {
		GST_ELEMENT_ERROR (gst_aravis, RESOURCE, READ,
			(_("Could not read camera \"%s\": %s"),
			 gst_aravis->camera_name ? gst_aravis->camera_name : "",
			 error->message),
			(NULL));
	}

	g_error_free (error);
}

static gboolean
gst_aravis_start (GstBaseSrc *src)
{
	GError *error = NULL;
	gboolean result = TRUE;
	GstAravis* gst_aravis = GST_ARAVIS(src);

	GST_LOG_OBJECT (gst_aravis, "Open camera '%s'", gst_aravis->camera_name);

	GST_OBJECT_LOCK (gst_aravis);
	if (gst_aravis->camera == NULL)
		result = gst_aravis_init_camera (gst_aravis, &error);

	if (result) gst_aravis->all_caps = gst_aravis_get_all_camera_caps (gst_aravis, &error);
	GST_OBJECT_UNLOCK (gst_aravis);

	if (error) gst_aravis_init_error (gst_aravis, error);

	return result;
}


static gboolean
gst_aravis_stop( GstBaseSrc * src )
{
	GError *error = NULL;
	GstAravis* gst_aravis = GST_ARAVIS(src);
	ArvStream *stream;
	GstCaps *all_caps;

	GST_OBJECT_LOCK (gst_aravis);
	arv_camera_stop_acquisition (gst_aravis->camera, &error);
	stream = g_steal_pointer (&gst_aravis->stream);
	all_caps = g_steal_pointer (&gst_aravis->all_caps);
	GST_OBJECT_UNLOCK (gst_aravis);

	if (stream != NULL)
		g_object_unref (stream);
	if (all_caps != NULL)
		gst_caps_unref (all_caps);

	GST_DEBUG_OBJECT (gst_aravis, "Stop acquisition");
	if (error) {
		// GigEVision write_register timeout is common if
		// stopping due to lost camera
		GST_ERROR_OBJECT (src, "Acquisition stop error: %s", error->message);
		g_error_free (error);
	}

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
	int arv_row_stride;
	int width, height;
	char *buffer_data;
	size_t buffer_size;
	guint64 timestamp_ns;
	gboolean base_src_does_timestamp;
	ArvBuffer *arv_buffer = NULL;

	gst_aravis = GST_ARAVIS (push_src);
	base_src_does_timestamp = gst_base_src_get_do_timestamp(GST_BASE_SRC(push_src));

	GST_OBJECT_LOCK (gst_aravis);

	do {
		if (arv_buffer) arv_stream_push_buffer (gst_aravis->stream, arv_buffer);
		arv_buffer = arv_stream_timeout_pop_buffer (gst_aravis->stream, gst_aravis->buffer_timeout_us);
	} while (arv_buffer != NULL && arv_buffer_get_status (arv_buffer) != ARV_BUFFER_STATUS_SUCCESS);

	if (arv_buffer == NULL)
		goto error;

	buffer_data = (char *) arv_buffer_get_data (arv_buffer, &buffer_size);
	arv_buffer_get_image_region (arv_buffer, NULL, NULL, &width, &height);
	arv_row_stride = width * ARV_PIXEL_FORMAT_BIT_PER_PIXEL (arv_buffer_get_image_pixel_format (arv_buffer)) / 8;
	timestamp_ns = arv_buffer_get_timestamp (arv_buffer);

	/* Gstreamer requires row stride to be a multiple of 4 */
	if ((arv_row_stride & 0x3) != 0) {
		int gst_row_stride;
		size_t size;
		char *data;
		int i;

		gst_row_stride = (arv_row_stride & ~(0x3)) + 4;

		size = height * gst_row_stride;
		data = g_malloc (size);

		for (i = 0; i < height; i++)
			memcpy (data + i * gst_row_stride, buffer_data + i * arv_row_stride, arv_row_stride);

		*buffer = gst_buffer_new_wrapped (data, size);
	} else {
		// FIXME Should arv_stream_push_buffer when the GstBuffer is destroyed
		*buffer = gst_buffer_new_wrapped_full (0, buffer_data, buffer_size, 0, buffer_size, NULL, NULL);
	}

	if (!base_src_does_timestamp) {
		if (gst_aravis->timestamp_offset == 0) {
			gst_aravis->timestamp_offset = timestamp_ns;
			gst_aravis->last_timestamp = timestamp_ns;
		}

		GST_BUFFER_PTS (*buffer) = timestamp_ns - gst_aravis->timestamp_offset;
		GST_BUFFER_DURATION (*buffer) = timestamp_ns - gst_aravis->last_timestamp;

		gst_aravis->last_timestamp = timestamp_ns;
	}

	arv_stream_push_buffer (gst_aravis->stream, arv_buffer);
	GST_OBJECT_UNLOCK (gst_aravis);

	return GST_FLOW_OK;

error:
	GST_OBJECT_UNLOCK (gst_aravis);
	return GST_FLOW_ERROR;
}

static GstCaps *
gst_aravis_fixate_caps (GstBaseSrc * bsrc, GstCaps * caps)
{
	GError *error = NULL;
	GstAravis *gst_aravis = GST_ARAVIS (bsrc);
	GstStructure *structure;
	gint width;
	gint height;
	double frame_rate = 0.0;
	gboolean is_frame_rate_available;

	g_return_val_if_fail (GST_IS_ARAVIS (bsrc), NULL);

	GST_OBJECT_LOCK (gst_aravis);
	arv_camera_get_region (gst_aravis->camera, NULL, NULL, &width, &height, &error);
	is_frame_rate_available = arv_camera_is_frame_rate_available (gst_aravis->camera, NULL);
	if (is_frame_rate_available)
		if (!error) frame_rate = arv_camera_get_frame_rate (gst_aravis->camera, &error);
	GST_OBJECT_UNLOCK (gst_aravis);
	if (error) {
		GST_ELEMENT_ERROR (gst_aravis, RESOURCE, READ,
			(_("Could not read camera \"%s\": %s"),
			 gst_aravis->camera_name ? gst_aravis->camera_name : "",
			 error->message),
			(NULL));
		g_error_free (error);
	} else {
		structure = gst_caps_get_structure (caps, 0);

		gst_structure_fixate_field_nearest_int (structure, "width", width);
		gst_structure_fixate_field_nearest_int (structure, "height", height);
		if (is_frame_rate_available)
			gst_structure_fixate_field_nearest_fraction (structure, "framerate",
								     (double) (0.5 + frame_rate), 1);

		GST_LOG_OBJECT (gst_aravis, "Fixate caps");
	}

	return GST_BASE_SRC_CLASS(gst_aravis_parent_class)->fixate(bsrc, caps);
}

static void
gst_aravis_init (GstAravis *gst_aravis)
{
	gst_base_src_set_live (GST_BASE_SRC (gst_aravis), TRUE);
	gst_base_src_set_format (GST_BASE_SRC (gst_aravis), GST_FORMAT_TIME);

	gst_aravis->camera_name = NULL;

	gst_aravis->gain = -1;
	gst_aravis->gain_auto = ARV_AUTO_OFF;
	gst_aravis->gain_auto_set = FALSE;
	gst_aravis->exposure_time_us = -1;
	gst_aravis->exposure_auto = ARV_AUTO_OFF;
	gst_aravis->exposure_auto_set = FALSE;
	gst_aravis->offset_x = 0;
	gst_aravis->offset_y = 0;
	gst_aravis->h_binning = -1;
	gst_aravis->v_binning = -1;
	gst_aravis->packet_delay = -1;
	gst_aravis->packet_size = -1;
	gst_aravis->auto_packet_size = FALSE;
        gst_aravis->packet_resend = TRUE;
	gst_aravis->num_arv_buffers = GST_ARAVIS_DEFAULT_N_BUFFERS;
	gst_aravis->payload = 0;
	gst_aravis->usb_mode = ARV_UV_USB_MODE_DEFAULT;

	gst_aravis->buffer_timeout_us = GST_ARAVIS_BUFFER_TIMEOUT_DEFAULT;
	gst_aravis->frame_rate = 0.0;

	gst_aravis->camera = NULL;
	gst_aravis->stream = NULL;

	gst_aravis->all_caps = NULL;
	gst_aravis->fixed_caps = NULL;
}

static void
gst_aravis_finalize (GObject * object)
{
	GstAravis *gst_aravis = GST_ARAVIS (object);
	ArvCamera *camera;
	ArvStream *stream;
	GstCaps *all_caps;
	GstCaps *fixed_caps;

	GST_OBJECT_LOCK (gst_aravis);
	camera = g_steal_pointer (&gst_aravis->camera);
	stream = g_steal_pointer (&gst_aravis->stream);
	all_caps = g_steal_pointer (&gst_aravis->all_caps);
	fixed_caps = g_steal_pointer (&gst_aravis->fixed_caps);
	g_clear_pointer (&gst_aravis->camera_name, g_free);
	g_clear_pointer (&gst_aravis->features, g_free);
	GST_OBJECT_UNLOCK (gst_aravis);

	if (camera != NULL)
		g_object_unref (camera);
	if (stream != NULL)
		g_object_unref (stream);
	if (all_caps != NULL)
		gst_caps_unref (all_caps);
	if (fixed_caps != NULL)
		gst_caps_unref (fixed_caps);

	G_OBJECT_CLASS (gst_aravis_parent_class)->finalize (object);
}

static void
gst_aravis_set_property (GObject * object, guint prop_id,
			 const GValue * value, GParamSpec * pspec)
{
	GError *error = NULL;
	GstAravis *gst_aravis = GST_ARAVIS (object);

	GST_DEBUG_OBJECT (gst_aravis, "setting property %s", pspec->name);

	switch (prop_id) {
		case PROP_CAMERA_NAME:
			GST_OBJECT_LOCK (gst_aravis);
			g_free (gst_aravis->camera_name);
			/* check if we are currently active
			   prevent setting camera and other values to something not representing the active camera */
			if (gst_aravis->stream == NULL) {
				gst_aravis->camera_name = g_strdup (g_value_get_string (value));
				gst_aravis_init_camera (gst_aravis, &error);
			}

			GST_LOG_OBJECT (gst_aravis, "Set camera name to %s", gst_aravis->camera_name);
			GST_OBJECT_UNLOCK (gst_aravis);
			if (error) gst_aravis_init_error (gst_aravis, error);
			break;
		case PROP_GAIN:
			GST_OBJECT_LOCK (gst_aravis);
			gst_aravis->gain = g_value_get_double (value);
			if (gst_aravis->camera != NULL && arv_camera_is_gain_available (gst_aravis->camera, NULL))
				arv_camera_set_gain (gst_aravis->camera, gst_aravis->gain, NULL);
			GST_OBJECT_UNLOCK (gst_aravis);
			break;
		case PROP_GAIN_AUTO:
			GST_OBJECT_LOCK (gst_aravis);
			gst_aravis->gain_auto = g_value_get_enum (value);
			gst_aravis->gain_auto_set = TRUE;
			if (gst_aravis->camera != NULL && arv_camera_is_gain_auto_available (gst_aravis->camera, NULL))
				arv_camera_set_gain_auto (gst_aravis->camera, gst_aravis->gain_auto, NULL);
			GST_OBJECT_UNLOCK (gst_aravis);
			break;
		case PROP_EXPOSURE:
			GST_OBJECT_LOCK (gst_aravis);
			gst_aravis->exposure_time_us = g_value_get_double (value);
			if (gst_aravis->camera != NULL &&
			    arv_camera_is_exposure_time_available (gst_aravis->camera, NULL))
				arv_camera_set_exposure_time (gst_aravis->camera, gst_aravis->exposure_time_us, NULL);
			GST_OBJECT_UNLOCK (gst_aravis);
			break;
		case PROP_EXPOSURE_AUTO:
			GST_OBJECT_LOCK (gst_aravis);
			gst_aravis->exposure_auto = g_value_get_enum (value);
			gst_aravis->exposure_auto_set = TRUE;
			if (gst_aravis->camera != NULL &&
			    arv_camera_is_exposure_auto_available (gst_aravis->camera, NULL))
				arv_camera_set_exposure_time_auto (gst_aravis->camera, gst_aravis->exposure_auto, NULL);
			GST_OBJECT_UNLOCK (gst_aravis);
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
		case PROP_PACKET_DELAY:
			GST_OBJECT_LOCK (gst_aravis);
			gst_aravis->packet_delay = g_value_get_int64 (value);
			GST_OBJECT_UNLOCK (gst_aravis);
			break;
                case PROP_PACKET_SIZE:
                        gst_aravis->packet_size = g_value_get_int (value);
                        break;
                case PROP_AUTO_PACKET_SIZE:
                        gst_aravis->auto_packet_size = g_value_get_boolean (value);
                        break;
                case PROP_PACKET_RESEND:
                        gst_aravis->packet_resend = g_value_get_boolean (value);
                        break;
                case PROP_FEATURES:
			GST_OBJECT_LOCK (gst_aravis);
			g_free (gst_aravis->features);
                        gst_aravis->features = g_value_dup_string (value);
			GST_OBJECT_UNLOCK (gst_aravis);
                        break;
		case PROP_NUM_ARV_BUFFERS:
			gst_aravis->num_arv_buffers = g_value_get_int (value);
			break;
		case PROP_USB_MODE:
			gst_aravis->usb_mode = g_value_get_enum (value);
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

	GST_DEBUG_OBJECT (gst_aravis, "getting property %s", pspec->name);

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
			GST_OBJECT_LOCK (gst_aravis);
			if (!gst_aravis->gain_auto_set && gst_aravis->camera != NULL &&
			    arv_camera_is_gain_auto_available (gst_aravis->camera, NULL)) {
				gst_aravis->gain_auto = arv_camera_get_gain_auto(gst_aravis->camera, NULL);
			}
			g_value_set_enum (value, gst_aravis->gain_auto);
			GST_OBJECT_UNLOCK (gst_aravis);
			break;
		case PROP_EXPOSURE:
			g_value_set_double (value, gst_aravis->exposure_time_us);
			break;
		case PROP_EXPOSURE_AUTO:
			GST_OBJECT_LOCK (gst_aravis);
			if (!gst_aravis->exposure_auto_set && gst_aravis->camera != NULL &&
			    arv_camera_is_exposure_auto_available (gst_aravis->camera, NULL)) {
				gst_aravis->exposure_auto = arv_camera_get_exposure_time_auto(gst_aravis->camera, NULL);
			}
			g_value_set_enum (value, gst_aravis->exposure_auto);
			GST_OBJECT_UNLOCK (gst_aravis);
			break;
		case PROP_OFFSET_X:
			g_value_set_int (value, gst_aravis->offset_x);
			break;
		case PROP_OFFSET_Y:
			g_value_set_int (value, gst_aravis->offset_y);
			break;
		case PROP_H_BINNING:
			GST_OBJECT_LOCK (gst_aravis);
			if (gst_aravis->h_binning < 0 && gst_aravis->camera) {
				arv_camera_get_binning (gst_aravis->camera, &gst_aravis->h_binning, NULL, NULL);
			}
			g_value_set_int (value, gst_aravis->h_binning);
			GST_OBJECT_UNLOCK (gst_aravis);
			break;
		case PROP_V_BINNING:
			GST_OBJECT_LOCK (gst_aravis);
			if (gst_aravis->v_binning < 0 && gst_aravis->camera) {
				arv_camera_get_binning (gst_aravis->camera, NULL, &gst_aravis->v_binning, NULL);
			}
			g_value_set_int (value, gst_aravis->v_binning);
			GST_OBJECT_UNLOCK (gst_aravis);
			break;
		case PROP_PACKET_DELAY:
			GST_OBJECT_LOCK (gst_aravis);
			g_value_set_int64 (value, gst_aravis->packet_delay);
			GST_OBJECT_UNLOCK (gst_aravis);
			break;
        	case PROP_PACKET_SIZE:
                        g_value_set_int (value, gst_aravis->packet_size);
                        break;
        	case PROP_AUTO_PACKET_SIZE:
                        g_value_set_boolean (value, gst_aravis->auto_packet_size);
                        break;
        	case PROP_PACKET_RESEND:
                        g_value_set_boolean (value, gst_aravis->packet_resend);
                        break;
		case PROP_FEATURES:
			g_value_set_string (value, gst_aravis->features);
			break;
		case PROP_NUM_ARV_BUFFERS:
			g_value_set_int (value, gst_aravis->num_arv_buffers);
			break;
		case PROP_USB_MODE:
			g_value_set_enum(value, gst_aravis->usb_mode);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static gboolean
gst_aravis_query (GstBaseSrc *bsrc, GstQuery *query)
{
	GstAravis *src = GST_ARAVIS (bsrc);
	gboolean res = FALSE;

	switch (GST_QUERY_TYPE (query))
	{
		case GST_QUERY_LATENCY:
		{
			GstClockTime min_latency;
			GstClockTime max_latency;

			/* device must be open */
			if (!src->stream)
			{
				GST_WARNING_OBJECT (src, "Can't give latency since device isn't open !");
				goto done;
			}

			/* we must have a framerate */
			if (src->frame_rate <= 0.0)
			{
				GST_WARNING_OBJECT (src, "Can't give latency since framerate isn't fixated !");
				goto done;
			}

			/* min latency is the time to capture one frame/field */
			min_latency = gst_util_gdouble_to_guint64 (src->frame_rate);

			/* max latency is set to NONE because cameras may enter trigger mode
			   and not deliver images for an unspecified amount of time */
			max_latency = GST_CLOCK_TIME_NONE;

			GST_DEBUG_OBJECT (bsrc, "report latency min %" GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
					  GST_TIME_ARGS (min_latency), GST_TIME_ARGS (max_latency));

			/* we are always live, the min latency is 1 frame and the max latency is
			 * the complete buffer of frames. */
			gst_query_set_latency (query, TRUE, min_latency, max_latency);

			res = TRUE;
			break;
		}
		default:
		{
			res = GST_BASE_SRC_CLASS (gst_aravis_parent_class)->query (bsrc, query);
			break;
		}
	}

done:

	return res;
}

static void
gst_aravis_class_init (GstAravisClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
	GstBaseSrcClass *gstbasesrc_class = GST_BASE_SRC_CLASS (klass);
	GstPushSrcClass *gstpushsrc_class = GST_PUSH_SRC_CLASS (klass);

	gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_aravis_finalize);
	gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_aravis_set_property);
	gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_aravis_get_property);

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
		 g_param_spec_enum ("gain-auto",
				    "Auto Gain",
				    "Auto Gain Mode",
				    GST_TYPE_ARV_AUTO, ARV_AUTO_OFF,
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
		 g_param_spec_enum ("exposure-auto",
				    "Auto Exposure",
				    "Auto Exposure Mode",
				    GST_TYPE_ARV_AUTO, ARV_AUTO_OFF,
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
		 PROP_PACKET_DELAY,
		 g_param_spec_int64 ("packet-delay",
				   "Packet delay",
				   "GigEVision streaming inter packet delay (in ns, -1 = default)",
				   0, G_MAXINT64/1000000000LL, 0,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_PACKET_SIZE,
		 g_param_spec_int ("packet-size",
				   "Packet size",
				   "GigEVision streaming packet size",
				   ARV_GVSP_PACKET_PROTOCOL_OVERHEAD(FALSE), 65500, 1500,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class,
		 PROP_AUTO_PACKET_SIZE,
		 g_param_spec_boolean ("auto-packet-size",
				       "Auto Packet Size",
				       "Negotiate GigEVision streaming packet size",
				       FALSE,
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
		 PROP_FEATURES,
		 g_param_spec_string ("features",
				      "String of feature values",
				      "Additional configuration parameters as a space separated list of feature assignations",
				      NULL,
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property
		(gobject_class,
		 PROP_NUM_ARV_BUFFERS,
		 g_param_spec_int ("num-arv-buffers",
				   "Number of Buffers allocated",
				   "Number of video buffers to allocate for video frames",
				   1, G_MAXINT, GST_ARAVIS_DEFAULT_N_BUFFERS,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property
	    (gobject_class, PROP_USB_MODE,
	     g_param_spec_enum("usb-mode",
			       "USB mode",
			       "USB mode (synchronous/asynchronous)",
			       GST_TYPE_ARV_USB_MODE, ARV_UV_USB_MODE_DEFAULT,
			       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

        GST_DEBUG_CATEGORY_INIT (aravis_debug, "aravissrc", 0, "Aravis interface");

	gst_element_class_set_details_simple (element_class,
					      "Aravis Video Source",
					      "Source/Video",
					      "Aravis based source",
					      "Emmanuel Pacaud <emmanuel.pacaud@free.fr>");
	gst_element_class_add_pad_template (element_class,
					    gst_static_pad_template_get (&aravis_src_template));

	gstbasesrc_class->get_caps = GST_DEBUG_FUNCPTR (gst_aravis_get_caps);
	gstbasesrc_class->set_caps = GST_DEBUG_FUNCPTR (gst_aravis_set_caps);
	gstbasesrc_class->fixate = GST_DEBUG_FUNCPTR (gst_aravis_fixate_caps);
	gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gst_aravis_start);
	gstbasesrc_class->stop = GST_DEBUG_FUNCPTR (gst_aravis_stop);
	gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_aravis_query);

	gstbasesrc_class->get_times = GST_DEBUG_FUNCPTR (gst_aravis_get_times);

	gstpushsrc_class->create = GST_DEBUG_FUNCPTR (gst_aravis_create);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
        return gst_element_register (plugin, "aravissrc", GST_RANK_NONE, GST_TYPE_ARAVIS);
}

#define PACKAGE "aravis"

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
		   GST_VERSION_MINOR,
		   aravis,
		   "Aravis Video Source",
		   plugin_init,
		   ARAVIS_VERSION,
		   "LGPL",
		   "aravis",
		   "http://blogs.gnome.org/emmanuel")
