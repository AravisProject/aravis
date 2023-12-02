/* Aravis - Digital camera library
 *
 * Copyright © 2009-2021 Emmanuel Pacaud
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
 * SECTION: arvv4l2interface
 * @short_description: V4l2 interface
 */

#include <arvv4l2interfaceprivate.h>
#include <arvv4l2deviceprivate.h>
#include <arvinterfaceprivate.h>
#include <arvv4l2device.h>
#include <arvdebug.h>
#include <gudev/gudev.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <arvmisc.h>

struct _ArvV4l2Interface {
	ArvInterface	interface;

	GHashTable *devices;
	GUdevClient *udev;
};

struct _ArvV4l2InterfaceClass {
	ArvInterfaceClass parent_class;
};

G_DEFINE_TYPE (ArvV4l2Interface, arv_v4l2_interface, ARV_TYPE_INTERFACE)

typedef struct {
	char *id;
	char *bus;
	char *device_file;
	char *version;

	volatile gint ref_count;
} ArvV4l2InterfaceDeviceInfos;

static ArvV4l2InterfaceDeviceInfos *
arv_v4l2_interface_device_infos_new (const char *device_file, const char *name)
{
	ArvV4l2InterfaceDeviceInfos *infos;

	g_return_val_if_fail (device_file != NULL, NULL);

	if (strncmp ("/dev/vbi", device_file,  8) != 0) {
		int fd;

		fd = v4l2_open (device_file, O_RDWR);
		if (fd != -1) {
			struct v4l2_capability cap;

			if (v4l2_ioctl (fd, VIDIOC_QUERYCAP, &cap) != -1 &&
			    ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) != 0) &&
			    ((cap.capabilities & V4L2_CAP_STREAMING) != 0)) {
				infos = g_new0 (ArvV4l2InterfaceDeviceInfos, 1);

				infos->ref_count = 1;
				infos->id = g_strdup_printf ("%s-%s", (char *) cap.card, name);
				infos->bus = g_strdup ((char *) cap.bus_info);
				infos->device_file = g_strdup (device_file);
				infos->version = g_strdup_printf ("%d.%d.%d",
								  (cap.version >> 16) & 0xff,
								  (cap.version >>  8) & 0xff,
								  (cap.version >>  0) & 0xff);

				return infos;
			}
			v4l2_close (fd);
		}
	}

	return NULL;
}

static ArvV4l2InterfaceDeviceInfos *
arv_v4l2_interface_device_infos_ref (ArvV4l2InterfaceDeviceInfos *infos)
{
	g_return_val_if_fail (infos != NULL, NULL);
	g_return_val_if_fail (g_atomic_int_get (&infos->ref_count) > 0, NULL);

	g_atomic_int_inc (&infos->ref_count);

	return infos;
}

static void
arv_v4l2_interface_device_infos_unref (ArvV4l2InterfaceDeviceInfos *infos)
{
	g_return_if_fail (infos != NULL);
	g_return_if_fail (g_atomic_int_get (&infos->ref_count) > 0);

	if (g_atomic_int_dec_and_test (&infos->ref_count)) {
		g_free (infos->id);
		g_free (infos->bus);
		g_free (infos->device_file);
		g_free (infos->version);
		g_free (infos);
	}
}

static void
_discover (ArvV4l2Interface *v4l2_interface, GArray *device_ids)
{
	GList *devices, *elem;

	g_hash_table_remove_all (v4l2_interface->devices);

	devices = g_udev_client_query_by_subsystem (v4l2_interface->udev, "video4linux");

	for (elem = g_list_first (devices); elem; elem = g_list_next (elem)) {
		ArvV4l2InterfaceDeviceInfos *device_infos;

		device_infos = arv_v4l2_interface_device_infos_new (g_udev_device_get_device_file (elem->data),
                                                                    g_udev_device_get_name(elem->data));
		if (device_infos != NULL) {
			ArvInterfaceDeviceIds *ids;

			g_hash_table_replace (v4l2_interface->devices,
					      device_infos->id,
					      arv_v4l2_interface_device_infos_ref (device_infos));
			g_hash_table_replace (v4l2_interface->devices,
					      device_infos->bus,
					      arv_v4l2_interface_device_infos_ref (device_infos));
			g_hash_table_replace (v4l2_interface->devices,
					      device_infos->device_file,
					      arv_v4l2_interface_device_infos_ref (device_infos));

			if (device_ids != NULL) {
				ids = g_new0 (ArvInterfaceDeviceIds, 1);

				ids->device = g_strdup (device_infos->id);
				ids->physical = g_strdup (device_infos->bus);
				ids->address = g_strdup (device_infos->device_file);
				ids->vendor = g_strdup ("Aravis");
				ids->model = g_strdup (device_infos->id);
				ids->serial_nbr = g_strdup_printf ("%s", g_udev_device_get_number(elem->data));
                                ids->protocol = "V4L2";

				g_array_append_val (device_ids, ids);
			}

			arv_v4l2_interface_device_infos_unref (device_infos);
		}

		g_object_unref (elem->data);
	}

	g_list_free (devices);
}

static void
arv_v4l2_interface_update_device_list (ArvInterface *interface, GArray *device_ids)
{
	ArvV4l2Interface *v4l2_interface = ARV_V4L2_INTERFACE (interface);

	g_assert (device_ids->len == 0);

	_discover (v4l2_interface, device_ids);
}

static ArvDevice *
_open_device (ArvInterface *interface, const char *device_id, GError **error)
{
	ArvV4l2Interface *v4l2_interface = ARV_V4L2_INTERFACE (interface);
	ArvV4l2InterfaceDeviceInfos *device_infos;

	if (device_id == NULL) {
		GList *device_list;

		device_list = g_hash_table_get_values (v4l2_interface->devices);
		device_infos = device_list != NULL ? device_list->data : NULL;
		g_list_free (device_list);
	} else
		device_infos = g_hash_table_lookup (v4l2_interface->devices, device_id);

	if (device_infos != NULL)
		return arv_v4l2_device_new (device_infos->device_file, error);

	return NULL;
}

static ArvDevice *
arv_v4l2_interface_open_device (ArvInterface *interface, const char *device_id, GError **error)
{
	ArvDevice *device;
	GError *local_error = NULL;

	device = _open_device (interface, device_id, error);
	if (ARV_IS_DEVICE (device) || local_error != NULL) {
		if (local_error != NULL)
			g_propagate_error (error, local_error);
		return device;
	}

	_discover (ARV_V4L2_INTERFACE (interface), NULL);

	return _open_device (interface, device_id, error);
}

static ArvInterface *arv_v4l2_interface = NULL;
static GMutex arv_v4l2_interface_mutex;

/**
 * arv_v4l2_interface_get_instance:
 *
 * Gets the unique instance of the v4l2 interface.
 *
 * Returns: (transfer none): a #ArvInterface singleton.
 */

ArvInterface *
arv_v4l2_interface_get_instance (void)
{
	g_mutex_lock (&arv_v4l2_interface_mutex);

	if (arv_v4l2_interface == NULL)
		arv_v4l2_interface = g_object_new (ARV_TYPE_V4L2_INTERFACE, NULL);

	g_mutex_unlock (&arv_v4l2_interface_mutex);

	return ARV_INTERFACE (arv_v4l2_interface);
}

void
arv_v4l2_interface_destroy_instance (void)
{
	g_mutex_lock (&arv_v4l2_interface_mutex);

	if (arv_v4l2_interface != NULL) {
		g_object_unref (arv_v4l2_interface);
		arv_v4l2_interface = NULL;
	}

	g_mutex_unlock (&arv_v4l2_interface_mutex);
}

static void
arv_v4l2_interface_init (ArvV4l2Interface *v4l2_interface)
{
	const gchar *const subsystems[] = {"video4linux", NULL};

	v4l2_interface->udev = g_udev_client_new (subsystems);

	v4l2_interface->devices = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
							 (GDestroyNotify) arv_v4l2_interface_device_infos_unref);
}

static void
arv_v4l2_interface_finalize (GObject *object)
{
	ArvV4l2Interface *v4l2_interface = ARV_V4L2_INTERFACE (object);

	g_hash_table_unref (v4l2_interface->devices);
	v4l2_interface->devices = NULL;

	G_OBJECT_CLASS (arv_v4l2_interface_parent_class)->finalize (object);
}

static void
arv_v4l2_interface_class_init (ArvV4l2InterfaceClass *v4l2_interface_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (v4l2_interface_class);
	ArvInterfaceClass *interface_class = ARV_INTERFACE_CLASS (v4l2_interface_class);

	object_class->finalize = arv_v4l2_interface_finalize;

	interface_class->update_device_list = arv_v4l2_interface_update_device_list;
	interface_class->open_device = arv_v4l2_interface_open_device;
}
