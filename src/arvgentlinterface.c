/* Aravis - Digital camera library
 *
 * Copyright © 2023 Xiaoqiang Wang
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
 * Author: Xiaoqiang Wang <xiaoqiang.wang@psi.ch>
 */

/**
 * SECTION: arvgentlinterface
 * @short_description: GenTL interface
 */
#include <GenTL_v1_5.h>
#include <arvgentlsystemprivate.h>
#include <arvgentlinterfaceprivate.h>
#include <arvgentldeviceprivate.h>
#include <arvinterfaceprivate.h>
#include <arvdebugprivate.h>
#include <arvmiscprivate.h>
#include <arvstr.h>
#include <stdlib.h>
#include <stdio.h>

/* ArvGenTLnterface implementation */

typedef struct {
	char *id;
	char *name;
	char *full_name;
	char *vendor;
	char *model;
	char *serial_nbr;

	ArvGenTLSystem *system;
	char *interface;
	volatile gint ref_count;
} ArvGenTLInterfaceDeviceInfos;

static ArvGenTLInterfaceDeviceInfos *
arv_gentl_interface_device_infos_new (ArvGenTLSystem *system,
				const char *interface,
				const char *id,
				const char *vendor,
				const char *model,
				const char *serial_nbr)
{
	ArvGenTLInterfaceDeviceInfos *infos;

	g_return_val_if_fail (system != NULL, NULL);
	g_return_val_if_fail (interface != NULL, NULL);
	g_return_val_if_fail (id != NULL, NULL);
	g_return_val_if_fail (vendor != NULL, NULL);
	g_return_val_if_fail (model != NULL, NULL);
	g_return_val_if_fail (serial_nbr != NULL, NULL);

	infos = g_new (ArvGenTLInterfaceDeviceInfos, 1);
	infos->system  = system;
	infos->interface = g_strdup(interface);
	infos->id = g_strdup(id);
	infos->name = g_strdup_printf ("%s-%s", arv_vendor_alias_lookup (vendor), serial_nbr);
	infos->full_name = g_strdup_printf ("%s-%s", vendor, serial_nbr);
	infos->vendor = g_strdup (vendor);
	infos->model = g_strdup (model);
	infos->serial_nbr = g_strdup (serial_nbr);
	infos->ref_count = 1;

	arv_str_strip (infos->id, ARV_DEVICE_NAME_ILLEGAL_CHARACTERS, ARV_DEVICE_NAME_REPLACEMENT_CHARACTER);
	arv_str_strip (infos->name, ARV_DEVICE_NAME_ILLEGAL_CHARACTERS, ARV_DEVICE_NAME_REPLACEMENT_CHARACTER);
	arv_str_strip (infos->full_name, ARV_DEVICE_NAME_ILLEGAL_CHARACTERS, ARV_DEVICE_NAME_REPLACEMENT_CHARACTER);

	return infos;
}

static ArvGenTLInterfaceDeviceInfos *
arv_gentl_interface_device_infos_ref (ArvGenTLInterfaceDeviceInfos *infos)
{
	g_return_val_if_fail (infos != NULL, NULL);
	g_return_val_if_fail (g_atomic_int_get (&infos->ref_count) > 0, NULL);

	g_atomic_int_inc (&infos->ref_count);

	return infos;
}

static void
arv_gentl_interface_device_infos_unref (ArvGenTLInterfaceDeviceInfos *infos)
{
	g_return_if_fail (infos != NULL);
	g_return_if_fail (g_atomic_int_get (&infos->ref_count) > 0);

	if (g_atomic_int_dec_and_test (&infos->ref_count)) {
		infos->system = NULL;
		g_clear_pointer (&infos->interface, g_free);
		g_clear_pointer (&infos->id, g_free);
		g_clear_pointer (&infos->name, g_free);
		g_clear_pointer (&infos->full_name, g_free);
		g_clear_pointer (&infos->vendor, g_free);
		g_clear_pointer (&infos->model, g_free);
		g_clear_pointer (&infos->serial_nbr, g_free);
		g_clear_pointer (&infos, g_free);
	}
}

typedef struct {
	GHashTable *devices;
} ArvGenTLInterfacePrivate;

struct _ArvGenTLInterface {
	ArvInterface	interface;

	ArvGenTLInterfacePrivate *priv;
};

struct _ArvGenTLInterfaceClass {
	ArvInterfaceClass parent_class;
};


G_DEFINE_TYPE_WITH_CODE (ArvGenTLInterface, arv_gentl_interface, ARV_TYPE_INTERFACE, G_ADD_PRIVATE (ArvGenTLInterface))

static gboolean
_gentl_get_info_4(void(*func)(void *handle, int32_t, int32_t*, void*, size_t*), void* handle, int32_t cmd, void*value)
{
	int32_t type;
	size_t size;
	func(handle, cmd, &type, value, &size);
}

#define _gentl_cnt(_1,_2,_3,_4,_5,_6,_7,_8,_9,_N, ...) _N
#define _gentl_argn(...) _gentl_cnt(dummy, ## __VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define _gentl_get_info(FUNC, CMD, VALUE, ...) _gentl_get_info_##_gentl_argn(__VAR_ARGS__)
#define _gentl_get_info_1(FUNC, CMD, VALUE, HANDLE) FUNC(HANDLE, CMD, VALUE)
#define _gentl_get_info_2(FUNC, CMD, VALUE, HANDLE, INDEX) FUNC(HANDLE, INDEX, CMD, VALUE)

static void
arv_gentl_interface_update_device_list (ArvInterface *interface, GArray *device_ids)
{
	ArvGenTLInterfacePrivate *priv = arv_gentl_interface_get_instance_private(ARV_GENTL_INTERFACE (interface));
	GList *gentl_systems = arv_gentl_get_systems();
	GList *gentl_systems_iter;
	ArvGenTLInterfaceDeviceInfos *device;
	ArvInterfaceDeviceIds *ids;

	g_assert (device_ids->len == 0);

	g_hash_table_remove_all (priv->devices);

	for ( gentl_systems_iter = gentl_systems; gentl_systems_iter; gentl_systems_iter = g_list_next(gentl_systems_iter) ) {
		ArvGenTLSystem *gentl_system = gentl_systems_iter->data;

		ArvGenTLModule * gentl = arv_gentl_system_get_gentl(gentl_system);
	
		TL_HANDLE system_handle = arv_gentl_system_open_system_handle(gentl_system);

		IF_HANDLE interface_handle;
		uint32_t i, j, num_interfaces, num_devices;
		size_t size;

		gentl->TLUpdateInterfaceList(system_handle, NULL, 100);
		gentl->TLGetNumInterfaces(system_handle, &num_interfaces);

		for (i=0; i<num_interfaces; i++) {
			char interface_id[1024], interface_type[1024];
			INTERFACE_INFO_CMD info_datatype;
			size=1024;gentl->TLGetInterfaceID(system_handle, i, interface_id, &size);
			size=1024;gentl->TLGetInterfaceInfo(system_handle, interface_id, INTERFACE_INFO_TLTYPE, &info_datatype, interface_type, &size);

			gentl->TLOpenInterface(system_handle, interface_id, &interface_handle);
			gentl->IFUpdateDeviceList(interface_handle, NULL, 100);
			gentl->IFGetNumDevices(interface_handle, &num_devices);
			arv_info_interface ("Interface: %s (%s)", interface_id, interface_type);
		
			for (j=0; j<num_devices; j++) {
				char device_id[1024], id[1024], model[1024], vendor[1024], serial_nbr[1024];
				size_t size = 1024;
				INFO_DATATYPE info_datatype;
				gentl->IFGetDeviceID(interface_handle, j, device_id, &size);
				arv_info_interface ("  Device: %s", device_id);

				size=1024;gentl->IFGetDeviceInfo(interface_handle, device_id, DEVICE_INFO_ID, &info_datatype, id, &size);
				size=1024;gentl->IFGetDeviceInfo(interface_handle, device_id, DEVICE_INFO_VENDOR, &info_datatype, vendor, &size);
				size=1024;gentl->IFGetDeviceInfo(interface_handle, device_id, DEVICE_INFO_MODEL, &info_datatype, model, &size);
				size=1024;gentl->IFGetDeviceInfo(interface_handle, device_id, DEVICE_INFO_SERIAL_NUMBER, &info_datatype, serial_nbr, &size);

				device =  arv_gentl_interface_device_infos_new(gentl_system, interface_id, id, vendor, model, serial_nbr);
				g_hash_table_replace(priv->devices, device->id, arv_gentl_interface_device_infos_ref(device));

				ids = g_new0 (ArvInterfaceDeviceIds, 1);
				ids->device = g_strdup(id);
				ids->model = g_strdup (model);
				ids->vendor = g_strdup (vendor);
				ids->physical = g_strdup(interface_type);
				ids->serial_nbr = g_strdup (serial_nbr);
				g_array_append_val (device_ids, ids);

				arv_gentl_interface_device_infos_unref(device);
			}
			gentl->IFClose(interface_handle);
		}
		arv_gentl_system_close_system_handle(gentl_system);
	}
}

static ArvDevice *
arv_gentl_interface_open_device (ArvInterface *interface, const char *device_id, GError **error)
{
	ArvGenTLInterfacePrivate *priv = arv_gentl_interface_get_instance_private(ARV_GENTL_INTERFACE (interface));
	ArvDevice *device = NULL;
	ArvGenTLInterfaceDeviceInfos *device_info = g_hash_table_lookup(priv->devices, device_id);

	if (device_info) {
		device = arv_gentl_device_new(device_info->system, device_info->interface, device_id, error);
	}

	return device;
}

static ArvInterface *arv_gentl_interface = NULL;
static GMutex arv_gentl_interface_mutex;

/**
 * arv_gentl_interface_get_instance:
 *
 * Gets the unique instance of the GenTL interface.
 *
 * Returns: (transfer none): a #ArvInterface singleton.
 */

ArvInterface *
arv_gentl_interface_get_instance (void)
{
	g_mutex_lock (&arv_gentl_interface_mutex);

	if (arv_gentl_interface == NULL)
		arv_gentl_interface = g_object_new (ARV_TYPE_GENTL_INTERFACE, NULL);

	g_mutex_unlock (&arv_gentl_interface_mutex);

	return ARV_INTERFACE (arv_gentl_interface);
}

void
arv_gentl_interface_destroy_instance (void)
{
	g_mutex_lock (&arv_gentl_interface_mutex);

	g_clear_object (&arv_gentl_interface);

	g_mutex_unlock (&arv_gentl_interface_mutex);
}

static void
arv_gentl_interface_init (ArvGenTLInterface *gentl_interface)
{
	ArvGenTLInterfacePrivate *priv = arv_gentl_interface_get_instance_private (gentl_interface);

	priv->devices = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
arv_gentl_interface_finalize (GObject *object)
{
	ArvGenTLInterface *gentl_interface = ARV_GENTL_INTERFACE (object);
	ArvGenTLInterfacePrivate *priv = arv_gentl_interface_get_instance_private (gentl_interface);
	GList *gentl_systems = arv_gentl_get_systems();
	GList *gentl_systems_iter;

	g_hash_table_unref (priv->devices);

	for ( gentl_systems_iter = gentl_systems; gentl_systems_iter; gentl_systems_iter = g_list_next(gentl_systems_iter) ) {
		g_object_unref(gentl_systems_iter->data);
	}

	G_OBJECT_CLASS (arv_gentl_interface_parent_class)->finalize (object);
}

static void
arv_gentl_interface_class_init (ArvGenTLInterfaceClass *gentl_interface_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (gentl_interface_class);
	ArvInterfaceClass *interface_class = ARV_INTERFACE_CLASS (gentl_interface_class);

	object_class->finalize = arv_gentl_interface_finalize;

	interface_class->update_device_list = arv_gentl_interface_update_device_list;
	interface_class->open_device = arv_gentl_interface_open_device;

	interface_class->protocol = "GenTL";
}
