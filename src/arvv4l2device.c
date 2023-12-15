/* Aravis - Digital camera library
 *
 * Copyright © 2009-2023 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvv4l2device
 * @short_description: V4l2 device
 */

#include <arvdeviceprivate.h>
#include <arvv4l2deviceprivate.h>
#include <arvv4l2streamprivate.h>
#include <arvdebugprivate.h>
#include <arvmisc.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <sys/mman.h>

#define ARV_V4L2_ADDRESS_DEVICE_VENDOR_NAME		0x0048
#define ARV_V4L2_ADDRESS_DEVICE_MODEL_NAME		0x0068
#define ARV_V4L2_ADDRESS_DEVICE_VERSION			0x0088
#define ARV_V4L2_ADDRESS_DEVICE_MANUFACTURER_INFO	0x00a8
#define ARV_V4L2_ADDRESS_DEVICE_ID			0x00d8

#define ARV_V4L2_ADDRESS_WIDTH                          0x0100
#define ARV_V4L2_ADDRESS_HEIGHT                         0x0104
#define ARV_V4L2_ADDRESS_PAYLOAD_SIZE                   0x0118
#define ARV_V4L2_ADDRESS_ACQUISITION_COMMAND            0x0124
#define ARV_V4L2_ADDRESS_PIXEL_FORMAT                   0x0128

typedef struct {
        guint32 v4l2;
        guint32 genicam;
} ArvV4l2GenicamPixelFormat;

static ArvV4l2GenicamPixelFormat pixel_format_map[] = {
        {V4L2_PIX_FMT_YUYV,             ARV_PIXEL_FORMAT_YUV_422_YUYV_PACKED},
/* Disable these formats for now, makes gstreamer crash:
        {V4L2_PIX_FMT_RGB24,            ARV_PIXEL_FORMAT_RGB_8_PACKED},
        {V4L2_PIX_FMT_BGR24,            ARV_PIXEL_FORMAT_BGR_8_PACKED},
*/
};

enum
{
	PROP_0,
	PROP_V4L2_DEVICE_DEVICE_FILE
};

typedef struct {
	int device_fd;

	char *device_file;
	char *device_card;
	char *device_version;
	char *device_driver;

        guint sensor_width;
        guint sensor_height;

        gint pixel_format_idx;
        GArray *pixel_formats;
        GArray *frame_sizes;

	char *genicam_xml;
	size_t genicam_xml_size;

	ArvGc *genicam;
} ArvV4l2DevicePrivate;

struct _ArvV4l2Device {
	ArvDevice device;
};

struct _ArvV4l2DeviceClass {
	ArvDeviceClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvV4l2Device, arv_v4l2_device, ARV_TYPE_DEVICE, G_ADD_PRIVATE (ArvV4l2Device))

/* ArvV4l2Device implemenation */

ArvPixelFormat
arv_pixel_format_from_v4l2 (guint32 v4l2_pixel_format)
{
        unsigned int i;

        for (i = 0; i < G_N_ELEMENTS(pixel_format_map); i++) {
                if (v4l2_pixel_format == pixel_format_map[i].v4l2)
                        return pixel_format_map[i].genicam;
        }

        return 0;
}

/* ArvDevice implemenation */

static ArvStream *
arv_v4l2_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data, GDestroyNotify destroy,
                               GError **error)
{
	return arv_v4l2_stream_new (ARV_V4L2_DEVICE (device), callback, user_data, destroy, error);
}

static const char *
arv_v4l2_device_get_genicam_xml (ArvDevice *device, size_t *size)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));

	return priv->genicam_xml;
}

static ArvGc *
arv_v4l2_device_get_genicam (ArvDevice *device)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));

	return priv->genicam;
}

gboolean
arv_v4l2_device_set_image_format (ArvV4l2Device *device)
{
        ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));
        struct v4l2_format format = {0};
        struct v4l2_requestbuffers req = {0};
        struct v4l2_frmsizeenum *frame_size;
        int i;
        ArvPixelFormat arv_pixel_format;

        req.count = 0;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        if (v4l2_ioctl(priv->device_fd, VIDIOC_REQBUFS, &req) == -1) {
                arv_warning_device ("Failed to release all v4l2 buffers (%s)", strerror(errno));
                return FALSE;
        }

        frame_size = &g_array_index (priv->frame_sizes,
                                     struct v4l2_frmsizeenum,
                                     priv->pixel_format_idx);

        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.width = frame_size->type == V4L2_FRMSIZE_TYPE_DISCRETE ?
                frame_size->discrete.width :
                frame_size->stepwise.max_width;
        format.fmt.pix.height = frame_size->type == V4L2_FRMSIZE_TYPE_DISCRETE ?
                frame_size->discrete.height :
                frame_size->stepwise.max_height;

        arv_pixel_format = g_array_index (priv->pixel_formats, guint32, priv->pixel_format_idx);

        for (i = 0; i < G_N_ELEMENTS (pixel_format_map); i++) {
                if (pixel_format_map[i].genicam == arv_pixel_format) {
                        format.fmt.pix.pixelformat = pixel_format_map[i].v4l2;
                        break;
                }
        }
        if (i >= G_N_ELEMENTS(pixel_format_map)) {
                arv_warning_device ("Uknown v4l2 pixel format (%d)", format.fmt.pix.pixelformat);
                return FALSE;
        }

        format.fmt.pix.field = V4L2_FIELD_NONE;

        arv_info_device ("Set format to %d×%d %s",
                         format.fmt.pix.width,
                         format.fmt.pix.height,
                         arv_pixel_format_to_gst_caps_string(arv_pixel_format));

        if (v4l2_ioctl(priv->device_fd, VIDIOC_S_FMT, &format) == -1) {
                arv_warning_device ("Failed to select v4l2 format (%s)", strerror(errno));
                return FALSE;
        }

        return TRUE;
}

gboolean
arv_v4l2_device_get_image_format (ArvV4l2Device *device,
                                  guint32 *payload_size,
                                  ArvPixelFormat *pixel_format,
                                  guint32 *width,
                                  guint32 *height,
                                  guint32 *bytes_per_line)
{
        ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));
        struct v4l2_format format = {0};
        int i;
        ArvPixelFormat arv_pixel_format;

        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (v4l2_ioctl(priv->device_fd, VIDIOC_G_FMT, &format) == -1) {
                arv_warning_device ("Failed to retrieve v4l2 format (%s)", strerror(errno));
                return FALSE;
        }

        for (i = 0; i < G_N_ELEMENTS (pixel_format_map); i++) {
                if (pixel_format_map[i].v4l2 == format.fmt.pix.pixelformat) {
                        arv_pixel_format = pixel_format_map[i].genicam;
                        break;
                }
        }
        if (i >= G_N_ELEMENTS(pixel_format_map)) {
                arv_warning_device ("Uknown v4l2 pixel format (%d)", format.fmt.pix.pixelformat);
                return FALSE;
        }

        if (payload_size != NULL)
                *payload_size = format.fmt.pix.sizeimage;
        if (pixel_format != NULL)
                *pixel_format = arv_pixel_format;
        if (width != NULL)
                *width = format.fmt.pix.width;
        if (height != NULL)
                *height = format.fmt.pix.height;
        if (bytes_per_line != NULL)
                *bytes_per_line = format.fmt.pix.bytesperline;

        arv_info_device ("Current format %d×%d %s %d bytes, %d bytes per line",
                         format.fmt.pix.width,
                         format.fmt.pix.height,
                         arv_pixel_format_to_gst_caps_string(arv_pixel_format),
                         format.fmt.pix.sizeimage,
                         format.fmt.pix.bytesperline);

        return TRUE;
}

static void
_control_stream (ArvV4l2Device *device, gboolean enable)
{
        ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (v4l2_ioctl(priv->device_fd, enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF, &type) == -1) {
                arv_warning_device ("v4l2 stream %s failed (%s)",
                                    enable ? "start" : "stop",
                                    strerror (errno));
        } else {
                arv_info_device ("Stream %s for device '%s'", enable ? "started" : "stopped", priv->device_file);
        }
}

static gboolean
arv_v4l2_device_read_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
        ArvV4l2Device *v4l2_device = ARV_V4L2_DEVICE(device);
        ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));
        gint32 value;
        gboolean found = TRUE;

        if (size < 1 || buffer == NULL)
                return FALSE;

        if (address == ARV_V4L2_ADDRESS_DEVICE_VENDOR_NAME) {
                strncpy (buffer, priv->device_driver, size - 1);
                ((char *) buffer)[size - 1] = '\0';
        } else if (address == ARV_V4L2_ADDRESS_DEVICE_MODEL_NAME) {
                strncpy (buffer, priv->device_card, size - 1);
                ((char *) buffer)[size - 1] = '\0';
        } else if (address == ARV_V4L2_ADDRESS_DEVICE_VERSION) {
                strncpy (buffer, priv->device_version, size - 1);
                ((char *) buffer)[size - 1] = '\0';
        } else if (address == ARV_V4L2_ADDRESS_DEVICE_MANUFACTURER_INFO) {
                strncpy (buffer, "Aravis", size - 1);
                ((char *) buffer)[size - 1] = '\0';
        } else if (address == ARV_V4L2_ADDRESS_DEVICE_ID) {
                strncpy (buffer, priv->device_file, size - 1);
                ((char *) buffer)[size - 1] = '\0';
        } else {
                if (size == sizeof (value)) {
                        struct v4l2_frmsizeenum *frame_size;

                        if (priv->pixel_format_idx < priv->frame_sizes->len &&
                            priv->pixel_format_idx < priv->pixel_formats->len) {
                                frame_size = &g_array_index (priv->frame_sizes,
                                                             struct v4l2_frmsizeenum,
                                                             priv->pixel_format_idx);

                                switch (address) {
                                        case ARV_V4L2_ADDRESS_WIDTH:
                                                value = frame_size->type == V4L2_FRMSIZE_TYPE_DISCRETE ?
                                                        frame_size->discrete.width :
                                                        frame_size->stepwise.max_width;
                                                break;
                                        case ARV_V4L2_ADDRESS_HEIGHT:
                                                value = frame_size->type == V4L2_FRMSIZE_TYPE_DISCRETE ?
                                                        frame_size->discrete.height :
                                                        frame_size->stepwise.max_height;
                                                break;
                                        case ARV_V4L2_ADDRESS_PAYLOAD_SIZE:
                                                arv_v4l2_device_set_image_format (v4l2_device);
                                                arv_v4l2_device_get_image_format (v4l2_device, (guint32 *) &value,
                                                                                  NULL, NULL, NULL, NULL);
                                                break;
                                        case ARV_V4L2_ADDRESS_PIXEL_FORMAT:
                                                value = g_array_index (priv->pixel_formats, guint32,
                                                                       priv->pixel_format_idx);
                                                break;
                                        default:
                                                found = FALSE;
                                }
                        } else {
                                found = FALSE;
                        }

                        if (found) {
                                memcpy (buffer, &value, sizeof (value));
                        }
                }
        }

        if (!found) {
                g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR_INVALID_ADDRESS,
                             "Invalid address (0x%08" G_GINT64_MODIFIER "x)", address);
                return FALSE;
        }

	return TRUE;
}

static gboolean
arv_v4l2_device_write_memory (ArvDevice *device, guint64 address, guint32 size, const void *buffer, GError **error)
{
        ArvV4l2Device *v4l2_device = ARV_V4L2_DEVICE(device);
        ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));
        gint32 value;
        gboolean found = TRUE;
        gint i;

        if (size == sizeof (value)) {
                memcpy (&value, buffer, sizeof (value));
                switch (address) {
                        case ARV_V4L2_ADDRESS_ACQUISITION_COMMAND:
                                _control_stream (v4l2_device, value != 0);
                                break;
                        case ARV_V4L2_ADDRESS_PIXEL_FORMAT:
                                for (i = 0; i < priv->pixel_formats->len; i++) {
                                        if (g_array_index(priv->pixel_formats, guint32, i) == value)
                                                priv->pixel_format_idx = i;
                                }
                                if (i == priv->pixel_formats->len)
                                        found = FALSE;
                                break;
                        default:
                                found = FALSE;
                }

                if (found) {
                        memcpy (buffer, &value, sizeof (value));
                }
        }

        if (!found) {
                g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR_INVALID_ADDRESS,
                             "Invalid address (0x%08" G_GINT64_MODIFIER "x)", address);
                return FALSE;
        }

        return TRUE;
}

static gboolean
arv_v4l2_device_read_register (ArvDevice *device, guint64 address, guint32 *value, GError **error)
{
	return arv_v4l2_device_read_memory (device, address, sizeof (guint32), value, error);
}

static gboolean
arv_v4l2_device_write_register (ArvDevice *device, guint64 address, guint32 value, GError **error)
{
	return arv_v4l2_device_write_memory (device, address, sizeof (guint32), &value, error);
}

int
arv_v4l2_device_get_fd (ArvV4l2Device *v4l2_device)
{
        ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (v4l2_device);

        g_return_val_if_fail(ARV_IS_V4L2_DEVICE(v4l2_device), 0);

        return priv->device_fd;
}

/**
 * arv_v4l2_device_new:
 * @error: a #GError placeholder, %NULL to ignore
 *
 * Returns: a newly created #ArvDevice connected to a v4l2 device
 *
 * Since: 0.10.0
 */

static void
arv_v4l2_device_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (self));

	switch (prop_id)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
			break;
		case PROP_V4L2_DEVICE_DEVICE_FILE:
			g_free (priv->device_file);
			priv->device_file = g_value_dup_string (value);
			break;
	}
}

ArvDevice *
arv_v4l2_device_new (const char *device_file, GError **error)
{
	g_return_val_if_fail (device_file != NULL, NULL);

	return g_initable_new (ARV_TYPE_V4L2_DEVICE, NULL, error, "device-file", device_file, NULL);
}

static void
arv_v4l2_device_init (ArvV4l2Device *v4l2_device)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (v4l2_device);

	priv->device_fd = -1;
}

static void
arv_v4l2_device_constructed (GObject *self)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (self));
        GString *format_feature;
        char *feature;
	struct v4l2_capability cap = {0};
        struct v4l2_cropcap crop_cap = {0};
	GBytes *bytes;
	GError *error = NULL;
        int i;

	priv->device_fd = v4l2_open (priv->device_file, O_RDWR);
	if (priv->device_fd == -1) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
							 "Can't open '%s'", priv->device_file));
		return;
	}

	if (v4l2_ioctl (priv->device_fd, VIDIOC_QUERYCAP, &cap) == -1) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
							 "Device '%s' is not a V4L2 device", priv->device_file));
		return;
	}

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
							 "Device '%s' is not video capture device",
                                                         priv->device_file));
		return;
        }

#if 0
        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) != 0) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
							 "Multiplanar capture of device '%s' is not supported",
                                                         priv->device_file));
		return;
        }
#endif

        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
							 "Device '%s' does not support streaming",
                                                         priv->device_file));
		return;
        }

#if 0
        if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
							 "Device '%s' does not support read",
                                                         priv->device_file));
		return;
        }
#endif

        crop_cap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (v4l2_ioctl (priv->device_fd, VIDIOC_CROPCAP, &crop_cap) == 0) {
                struct v4l2_crop crop =  {0};

                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = crop_cap.defrect; /* reset to default */

                v4l2_ioctl (priv->device_fd, VIDIOC_S_CROP, &crop);
        }


	priv->device_card = g_strdup ((char *) cap.card);
	priv->device_driver = g_strdup ((char *) cap.driver);
	priv->device_version = g_strdup_printf ("%d.%d.%d",
						(cap.version >> 16) & 0xff,
						(cap.version >>  8) & 0xff,
						(cap.version >>  0) & 0xff);

	bytes = g_resources_lookup_data("/org/aravis/arv-v4l2.xml",
					G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
	if (error != NULL) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_GENICAM_NOT_FOUND,
							 "%s", error->message));
		g_clear_error (&error);
		return;
	}

	priv->genicam_xml = g_strndup (g_bytes_get_data(bytes,NULL), g_bytes_get_size(bytes));
	priv->genicam_xml_size = g_bytes_get_size(bytes);

	g_bytes_unref (bytes);

	priv->genicam = arv_gc_new (ARV_DEVICE (self), priv->genicam_xml, priv->genicam_xml_size);
	if (!ARV_IS_GC (priv->genicam)) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_GENICAM_NOT_FOUND,
							 "Invalid Genicam data"));
		return;
	}

        priv->sensor_width = 0;
        priv->sensor_height = 0;

        priv->pixel_formats = g_array_new (FALSE, TRUE, sizeof (guint32));
        priv->frame_sizes = g_array_new(FALSE, TRUE, sizeof (struct v4l2_frmsizeenum));

        format_feature = g_string_new ("<Enumeration Name=\"PixelFormat\">\n"
                                       "  <DisplayName>Pixel format</DisplayName>\n");

        for (i = 0; TRUE; i++) {
                int j;
                struct v4l2_fmtdesc format = {0};
                guint32 genicam_pixel_format = 0;

                format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                format.index = i;
                if (v4l2_ioctl(priv->device_fd, VIDIOC_ENUM_FMT, &format) == -1)
                        break;

                arv_info_device ("Found format %s", format.description);

                genicam_pixel_format = arv_pixel_format_from_v4l2(format.pixelformat);

                g_array_insert_val (priv->pixel_formats, i, genicam_pixel_format);

                if (genicam_pixel_format == 0) {
                        arv_warning_device ("Genicam equivalent to v4l2 format %s not found", format.description);
                        continue;
                }

                g_string_append_printf (format_feature,
                                        "  <EnumEntry Name=\"%s\">\n"
                                        "    <Value>%d</Value>\n"
                                        "  </EnumEntry>\n",
                                        format.description,
                                        genicam_pixel_format);

                priv->pixel_format_idx = i;

                for (j = 0; TRUE; j++) {
                        struct v4l2_frmsizeenum frame_size = {0};

                        frame_size.index = j;
                        frame_size.pixel_format = format.pixelformat;

                        if (v4l2_ioctl(priv->device_fd, VIDIOC_ENUM_FRAMESIZES, &frame_size) == -1)
                                break;

                        if (frame_size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                                arv_debug_device ("  %d×%d", frame_size.discrete.width, frame_size.discrete.height);

                                priv->sensor_width = MAX (priv->sensor_width, frame_size.discrete.width);
                                priv->sensor_height = MAX (priv->sensor_height, frame_size.discrete.height);
                        } else {
                                arv_debug_device ("  (%d to %d)×(%d to %d) ",
                                                  frame_size.stepwise.min_width,
                                                  frame_size.stepwise.max_width,
                                                  frame_size.stepwise.min_height,
                                                  frame_size.stepwise.max_height);

                                priv->sensor_width = MAX (priv->sensor_width, frame_size.stepwise.max_width);
                                priv->sensor_height = MAX (priv->sensor_height, frame_size.stepwise.max_height);
                        }

                        if (j == 0)
                                g_array_insert_val (priv->frame_sizes, i, frame_size);
                }
        }

        g_array_set_size (priv->pixel_formats, i);
        g_array_set_size (priv->frame_sizes, i);

        g_string_append_printf (format_feature,
                                "  <pValue>PixelFormatRegister</pValue>\n"
                                "</Enumeration>");

        feature = g_strdup_printf ("<Integer Name=\"SensorHeight\">\n"
                                   "  <Description>Full height of image sensor.</Description>\n"
                                   "  <Value>%u</Value>\n"
                                   "  <AccessMode>RO</AccessMode>\n"
                                   "</Integer>", priv->sensor_height);
        arv_gc_set_default_node_data (priv->genicam, "SensorHeight", feature, NULL);
        g_free (feature);

        feature = g_strdup_printf ("<Integer Name=\"SensorWidth\">\n"
                                   "  <Description>Full width of image sensor.</Description>\n"
                                   "  <Value>%u</Value>\n"
                                   "  <AccessMode>RO</AccessMode>\n"
                                   "</Integer>", priv->sensor_width);
        arv_gc_set_default_node_data (priv->genicam, "SensorWidth", feature, NULL);
        g_free (feature);

        arv_gc_set_default_node_data (priv->genicam, "PixelFormat", format_feature->str, NULL);
        g_string_free (format_feature, TRUE);
}

static void
arv_v4l2_device_finalize (GObject *object)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (object));

	if (priv->device_fd != -1)
		v4l2_close (priv->device_fd);

	g_clear_object (&priv->genicam);
	g_clear_pointer (&priv->genicam_xml, g_free);
	g_clear_pointer (&priv->device_file, g_free);
	g_clear_pointer (&priv->device_version, g_free);
	g_clear_pointer (&priv->device_driver, g_free);
	g_clear_pointer (&priv->device_card, g_free);

        g_array_unref (priv->frame_sizes);
        g_array_unref (priv->pixel_formats);

	G_OBJECT_CLASS (arv_v4l2_device_parent_class)->finalize (object);
}

static void
arv_v4l2_device_class_init (ArvV4l2DeviceClass *v4l2_device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (v4l2_device_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (v4l2_device_class);

	object_class->finalize = arv_v4l2_device_finalize;
	object_class->constructed = arv_v4l2_device_constructed;
	object_class->set_property = arv_v4l2_device_set_property;

	device_class->create_stream = arv_v4l2_device_create_stream;
	device_class->get_genicam_xml = arv_v4l2_device_get_genicam_xml;
	device_class->get_genicam = arv_v4l2_device_get_genicam;
	device_class->read_memory = arv_v4l2_device_read_memory;
	device_class->write_memory = arv_v4l2_device_write_memory;
	device_class->read_register = arv_v4l2_device_read_register;
	device_class->write_register = arv_v4l2_device_write_register;

	g_object_class_install_property
		(object_class,
		 PROP_V4L2_DEVICE_DEVICE_FILE,
		 g_param_spec_string ("device-file",
				      "Device file",
				      "V4L2 device file",
				      NULL,
				      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}
