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

/**
 * SECTION: arvv4l2device
 * @short_description: V4l2 device
 */

#include <arvdeviceprivate.h>
#include <arvv4l2deviceprivate.h>
#include <arvv4l2streamprivate.h>
#include <arvv4l2miscprivate.h>
#include <arvdebugprivate.h>
#include <arvmisc.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

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

#define ARV_V4L2_ADDRESS_GAIN                           0x0200
#define ARV_V4L2_ADDRESS_GAIN_MIN                       0x0204
#define ARV_V4L2_ADDRESS_GAIN_MAX                       0x0208

#define ARV_V4L2_ADDRESS_FRAME_RATE                     0x0300
#define ARV_V4L2_ADDRESS_FRAME_RATE_MIN                 0x0308
#define ARV_V4L2_ADDRESS_FRAME_RATE_MAX                 0x0310

#define ARV_V4L2_ADDRESS_EXPOSURE_TIME                  0x0400
#define ARV_V4L2_ADDRESS_EXPOSURE_MIN                   0x0404
#define ARV_V4L2_ADDRESS_EXPOSURE_MAX                   0x0408
#define ARV_V4L2_ADDRESS_EXPOSURE_AUTO                  0x040C

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

        gboolean gain_available;
        gint32 gain_min;
        gint32 gain_max;

        gboolean exposure_available;
        gint32 exposure_min;
        gint32 exposure_max;
        gint32 exposure_manual_index;
        gint32 exposure_auto_index;

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
        ArvPixelFormat arv_pixel_format;

        req.count = 0;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        if (arv_v4l2_ioctl(priv->device_fd, VIDIOC_REQBUFS, &req) == -1) {
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

        format.fmt.pix.pixelformat = arv_pixel_format_to_v4l2(arv_pixel_format);
        if (format.fmt.pix.pixelformat == 0) {
                arv_warning_device ("Unknown 0x%08x pixel format", arv_pixel_format);
                return FALSE;
        }

        format.fmt.pix.field = V4L2_FIELD_NONE;

        arv_info_device ("Set format to %d×%d %s",
                         format.fmt.pix.width,
                         format.fmt.pix.height,
                         arv_pixel_format_to_gst_caps_string(arv_pixel_format));

        if (arv_v4l2_ioctl(priv->device_fd, VIDIOC_S_FMT, &format) == -1) {
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
        ArvPixelFormat arv_pixel_format;

        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (arv_v4l2_ioctl (priv->device_fd, VIDIOC_G_FMT, &format) == -1) {
                arv_warning_device ("Failed to retrieve v4l2 format (%s)", strerror(errno));
                return FALSE;
        }

        arv_pixel_format = arv_pixel_format_from_v4l2(format.fmt.pix.pixelformat);
        if (arv_pixel_format == 0) {
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

static gboolean
arv_v4l2_device_get_frame_rate_bounds (ArvV4l2Device *device, double *framerate_min, double *framerate_max)
{
        ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));
        ArvPixelFormat arv_pixel_format;
        double fr_min = 0, fr_max = 0;
        struct v4l2_format format = {0};
        unsigned int i;

        if (framerate_min != NULL)
                *framerate_min = 0.0;
        if (framerate_max != NULL)
                *framerate_max = 0.0;

        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (arv_v4l2_ioctl (priv->device_fd, VIDIOC_G_FMT, &format) == -1) {
                arv_warning_device ("Failed to retrieve v4l2 format (%s)", strerror(errno));
                return FALSE;
        }

        arv_pixel_format = arv_pixel_format_from_v4l2(format.fmt.pix.pixelformat);
        if (arv_pixel_format == 0) {
                arv_warning_device ("Uknown v4l2 pixel format (%d)", format.fmt.pix.pixelformat);
                return FALSE;
        }

        for (i = 0; TRUE; i++) {
                struct v4l2_frmivalenum frmivalenum = {0};

                frmivalenum.index = i;
                frmivalenum.pixel_format = format.fmt.pix.pixelformat;
                frmivalenum.width = format.fmt.pix.width;
                frmivalenum.height = format.fmt.pix.height;
                if (arv_v4l2_ioctl (priv->device_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmivalenum) == -1) {
                        if (i == 0) {
                                arv_warning_device ("Can't find frame rate");
                                return FALSE;
                        }
                        break;
                }

                if (frmivalenum.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                        double value =
                                (double) frmivalenum.discrete.denominator /
                                (double) frmivalenum.discrete.numerator;

                        if (i == 0) {
                                fr_max = fr_min = value;
                        } else {
                                if (value < fr_min)
                                        fr_min = value;
                                if (value > fr_max)
                                        fr_max = value;
                        }
                } else if (frmivalenum.type == V4L2_FRMIVAL_TYPE_CONTINUOUS ||
                           frmivalenum.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
                        fr_min =
                                (double) frmivalenum.stepwise.min.denominator /
                                (double) frmivalenum.stepwise.min.numerator;
                        fr_min =
                                (double) frmivalenum.stepwise.min.denominator /
                                (double) frmivalenum.stepwise.min.numerator;
                        break;
                } else {
                        if (i == 0) {
                                arv_warning_device ("Can't find frame rate");
                                return FALSE;
                        }
                        break;
                }
        }

        if (framerate_min != NULL)
                *framerate_min = fr_min;
        if (framerate_max != NULL)
                *framerate_max = fr_max;

        return TRUE;
}

static double
_get_frame_rate (ArvV4l2Device *device)
{
        ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));
        struct v4l2_streamparm streamparm = {0};

        streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (ioctl (priv->device_fd, VIDIOC_G_PARM, &streamparm) == -1) {
                arv_warning_device ("Failed to set frame rate");
                return 0.0;
        }

        return
                (double) streamparm.parm.capture.timeperframe.denominator /
                (double) streamparm.parm.capture.timeperframe.numerator;
}

static gboolean
_set_frame_rate (ArvV4l2Device *device, double frame_rate)
{
        ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));
        struct v4l2_streamparm streamparm = {0};

        streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        streamparm.parm.capture.timeperframe.numerator = 1000000.0;
        streamparm.parm.capture.timeperframe.denominator = 1000000.0 * frame_rate;

        if (ioctl (priv->device_fd, VIDIOC_S_PARM, &streamparm) == -1) {
                arv_warning_device ("Failed to set frame rate");
                return FALSE;
        }

        return TRUE;
}

static void
_control_stream (ArvV4l2Device *device, gboolean enable)
{
        ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (device));
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (arv_v4l2_ioctl (priv->device_fd, enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF, &type) == -1) {
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
                if (priv->pixel_format_idx < priv->frame_sizes->len &&
                    priv->pixel_format_idx < priv->pixel_formats->len) {
                        struct v4l2_frmsizeenum *frame_size;

                        frame_size = &g_array_index (priv->frame_sizes,
                                                     struct v4l2_frmsizeenum,
                                                     priv->pixel_format_idx);

                        if (size == 4) {
                                union {
                                        gint32 i32;
                                        float f;
                                } value;

                                g_assert (sizeof (value.i32) == 4);
                                g_assert (sizeof (value.f) == 4);

                                switch (address) {
                                        case ARV_V4L2_ADDRESS_WIDTH:
                                                value.i32 = frame_size->type == V4L2_FRMSIZE_TYPE_DISCRETE ?
                                                        frame_size->discrete.width :
                                                        frame_size->stepwise.max_width;
                                                break;
                                        case ARV_V4L2_ADDRESS_HEIGHT:
                                                value.i32 = frame_size->type == V4L2_FRMSIZE_TYPE_DISCRETE ?
                                                        frame_size->discrete.height :
                                                        frame_size->stepwise.max_height;
                                                break;
                                        case ARV_V4L2_ADDRESS_GAIN:
                                                value.i32 = arv_v4l2_get_ctrl (priv->device_fd, V4L2_CID_GAIN);
                                                break;
                                        case ARV_V4L2_ADDRESS_GAIN_MIN:
                                                value.i32 = priv->gain_min;
                                                break;
                                        case ARV_V4L2_ADDRESS_GAIN_MAX:
                                                value.i32 = priv->gain_max;
                                                break;
                                        case ARV_V4L2_ADDRESS_EXPOSURE_TIME:
                                                value.i32 = arv_v4l2_get_int32_ext_ctrl(priv->device_fd,
                                                                                        V4L2_CTRL_CLASS_CAMERA,
                                                                                        V4L2_CID_EXPOSURE_ABSOLUTE);
                                                break;
                                        case ARV_V4L2_ADDRESS_EXPOSURE_MIN:
                                                value.i32 = priv->exposure_min;
                                                break;
                                        case ARV_V4L2_ADDRESS_EXPOSURE_MAX:
                                                value.i32 = priv->exposure_max;
                                                break;
                                        case ARV_V4L2_ADDRESS_EXPOSURE_AUTO:
                                                value.i32 = arv_v4l2_get_int32_ext_ctrl
                                                        (priv->device_fd,
                                                         V4L2_CTRL_CLASS_CAMERA,
                                                         V4L2_CID_EXPOSURE_AUTO) == priv->exposure_auto_index ?
                                                        ARV_AUTO_CONTINUOUS : ARV_AUTO_OFF;
                                                break;
                                        case ARV_V4L2_ADDRESS_PAYLOAD_SIZE:
                                                arv_v4l2_device_set_image_format (v4l2_device);
                                                arv_v4l2_device_get_image_format (v4l2_device, (guint32 *) &value,
                                                                                  NULL, NULL, NULL, NULL);
                                                break;
                                        case ARV_V4L2_ADDRESS_PIXEL_FORMAT:
                                                value.i32 = g_array_index (priv->pixel_formats, guint32,
                                                                           priv->pixel_format_idx);
                                                break;
                                        default:
                                                found = FALSE;
                                }

                                if (found) {
                                        memcpy (buffer, &value, sizeof (value));
                                }
                        } else if (size == 8) {
                                union {
                                        gint64 i64;
                                        double d;
                                } value;

                                g_assert (sizeof (value.i64) == 8);
                                g_assert (sizeof (value.d) == 8);

                                switch (address) {
                                        case ARV_V4L2_ADDRESS_FRAME_RATE:
                                                value.d = _get_frame_rate(v4l2_device);
                                                break;
                                        case ARV_V4L2_ADDRESS_FRAME_RATE_MIN:
                                                arv_v4l2_device_get_frame_rate_bounds(v4l2_device, &value.d, NULL);
                                                break;
                                        case ARV_V4L2_ADDRESS_FRAME_RATE_MAX:
                                                arv_v4l2_device_get_frame_rate_bounds(v4l2_device, NULL, &value.d);
                                                break;
                                        default:
                                                found = FALSE;
                                }

                                if (found) {
                                        memcpy (buffer, &value, sizeof (value));
                                }
                        } else {
                                found = FALSE;
                        }
                } else {
                        found = FALSE;
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
        gboolean found = TRUE;
        gint i;

        if (size == 4) {
                union {
                        gint32 i32;
                        float f;
                } value;

                g_assert (sizeof (value.i32) == 4);
                g_assert (sizeof (value.f) == 4);

                memcpy (&value, buffer, sizeof (value));
                switch (address) {
                        case ARV_V4L2_ADDRESS_ACQUISITION_COMMAND:
                                _control_stream (v4l2_device, value.i32 != 0);
                                break;
                        case ARV_V4L2_ADDRESS_GAIN:
                                arv_v4l2_set_ctrl (priv->device_fd, V4L2_CID_GAIN, value.i32);
                                break;
                        case ARV_V4L2_ADDRESS_EXPOSURE_TIME:
                                arv_v4l2_set_int32_ext_ctrl (priv->device_fd,
                                                             V4L2_CTRL_CLASS_CAMERA,
                                                             V4L2_CID_EXPOSURE_ABSOLUTE,
                                                             value.i32);
                                break;
                        case ARV_V4L2_ADDRESS_EXPOSURE_AUTO:
                                arv_v4l2_set_int32_ext_ctrl (priv->device_fd, V4L2_CTRL_CLASS_CAMERA,
                                                             V4L2_CID_EXPOSURE_AUTO, value.i32 == ARV_AUTO_OFF ?
                                                             priv->exposure_manual_index :
                                                             priv->exposure_auto_index);
                                break;
                        case ARV_V4L2_ADDRESS_PIXEL_FORMAT:
                                for (i = 0; i < priv->pixel_formats->len; i++) {
                                        if (g_array_index(priv->pixel_formats, guint32, i) == value.i32)
                                                priv->pixel_format_idx = i;
                                }
                                if (i == priv->pixel_formats->len)
                                        found = FALSE;
                                break;
                        default:
                                found = FALSE;
                }
        } else if (size == 8) {
                union {
                        gint64 i64;
                        double d;
                } value;

                g_assert (sizeof (value.i64) == 8);
                g_assert (sizeof (value.d) == 8);

                memcpy (&value, buffer, sizeof (value));
                switch (address) {
                        case ARV_V4L2_ADDRESS_FRAME_RATE:
                                _set_frame_rate(v4l2_device, value.d);
                                break;
                        default:
                                found = FALSE;
                }
        } else {
                found = FALSE;
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
        struct stat st;
        int i;

        if (stat(priv->device_file, &st) == -1) {
                arv_device_take_init_error (ARV_DEVICE (self),
                                            g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
                                                         "Can't identify '%s' (%s)\n",
                         priv->device_file, strerror(errno)));
		return;
        }

        if (!S_ISCHR(st.st_mode)) {
                arv_device_take_init_error (ARV_DEVICE (self),
                                            g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
                                                         "'%s' is no device\n", priv->device_file));
                return;
        }

	priv->device_fd = open (priv->device_file, O_RDWR | O_NONBLOCK, 0);
	if (priv->device_fd == -1) {
		arv_device_take_init_error (ARV_DEVICE (self),
					    g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
							 "Can't open '%s'", priv->device_file));
		return;
	}

	if (arv_v4l2_ioctl (priv->device_fd, VIDIOC_QUERYCAP, &cap) == -1) {
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

        /* Reset cropping region */
        if (arv_v4l2_ioctl (priv->device_fd, VIDIOC_CROPCAP, &crop_cap) == 0) {
                struct v4l2_crop crop =  {0};

                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = crop_cap.defrect; /* reset to default */

                arv_v4l2_ioctl (priv->device_fd, VIDIOC_S_CROP, &crop);
        }

        priv->sensor_width = crop_cap.bounds.width;
        priv->sensor_height = crop_cap.bounds.height;

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

        /* Get gain infos */

        {
                struct v4l2_queryctrl queryctrl = {0};

                queryctrl.id = V4L2_CID_GAIN;
                if (ioctl (priv->device_fd, VIDIOC_QUERYCTRL, &queryctrl) != -1) {
                        priv->gain_available = TRUE;
                        priv->gain_min = queryctrl.minimum;
                        priv->gain_max = queryctrl.maximum;
                } else {
                        priv->gain_available = FALSE;
                }
        }

        /* Get Exposure infos */

        {
                struct v4l2_query_ext_ctrl query = {0};

                query.id = V4L2_CID_EXPOSURE_ABSOLUTE;
                if (ioctl (priv->device_fd, VIDIOC_QUERY_EXT_CTRL, &query) != -1) {
                        priv->exposure_available = TRUE;
                        priv->exposure_min = query.minimum * 100;
                        priv->exposure_max = query.maximum * 100;
                } else {
                        priv->exposure_available = FALSE;
                }

                query.id = V4L2_CID_EXPOSURE_AUTO;
                if (ioctl (priv->device_fd, VIDIOC_QUERY_EXT_CTRL, &query) != -1) {
                        priv->exposure_auto_index = -1;
                        priv->exposure_manual_index = -1;

                        for (i = query.minimum; i <= query.maximum; i++) {
                                struct v4l2_querymenu querymenu = {0};

                                querymenu.id = V4L2_CID_EXPOSURE_AUTO;
                                querymenu.index = i;

                                if (ioctl (priv->device_fd, VIDIOC_QUERYMENU, &querymenu) != -1) {
                                        if (i == V4L2_EXPOSURE_MANUAL)
                                                priv->exposure_manual_index = i;
                                        else
                                                priv->exposure_auto_index = i;
                                }
                        }
                }
        }

        /* Enumerate pixel formats */

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
                if (arv_v4l2_ioctl (priv->device_fd, VIDIOC_ENUM_FMT, &format) == -1)
                        break;

                genicam_pixel_format = arv_pixel_format_from_v4l2(format.pixelformat);

                g_array_insert_val (priv->pixel_formats, i, genicam_pixel_format);

                if (genicam_pixel_format == 0) {
                        arv_info_device ("Format %s ignored", format.description);
                        continue;
                } else {
                        arv_info_device ("Format %s found", format.description);
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

                        if (arv_v4l2_ioctl(priv->device_fd, VIDIOC_ENUM_FRAMESIZES, &frame_size) == -1)
                                break;

                        if (frame_size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                                arv_debug_device ("  %d×%d", frame_size.discrete.width, frame_size.discrete.height);
                        } else {
                                arv_debug_device ("  (%d to %d)×(%d to %d) ",
                                                  frame_size.stepwise.min_width,
                                                  frame_size.stepwise.max_width,
                                                  frame_size.stepwise.min_height,
                                                  frame_size.stepwise.max_height);
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

        feature = g_strdup_printf ("<Integer Name=\"GainAvailable\">\n"
                                   "  <Value>%d</Value>\n"
                                   "  <AccessMode>RO</AccessMode>\n"
                                   "</Integer>", priv->gain_available ? 1 : 0);
        arv_gc_set_default_node_data (priv->genicam, "GainAvailable", feature, NULL);
        g_free (feature);

        feature = g_strdup_printf ("<Integer Name=\"ExposureAvailable\">\n"
                                   "  <Value>%d</Value>\n"
                                   "  <AccessMode>RO</AccessMode>\n"
                                   "</Integer>", priv->exposure_available ? 1 : 0);
        arv_gc_set_default_node_data (priv->genicam, "ExposureAvailable", feature, NULL);
        g_free (feature);

        arv_gc_set_default_node_data (priv->genicam, "PixelFormat", format_feature->str, NULL);
        g_string_free (format_feature, TRUE);
}

static void
arv_v4l2_device_finalize (GObject *object)
{
	ArvV4l2DevicePrivate *priv = arv_v4l2_device_get_instance_private (ARV_V4L2_DEVICE (object));

	if (priv->device_fd != -1)
		close (priv->device_fd);

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
