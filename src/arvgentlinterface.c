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

static char *
_gentl_get_info_str(GC_ERROR(*func)(void*, const char *, int32_t, int32_t*, void*, size_t*),
                    void *interface, const char *device, BUFFER_INFO_CMD info_cmd)
{
	GC_ERROR error;
	INFO_DATATYPE type;
	size_t size;
	char *value = NULL;
	error = func(interface, device, info_cmd, &type, NULL, &size);
	if (error == GC_ERR_SUCCESS) {
		value = g_malloc0(size);
		func(interface, device, info_cmd, &type, value, &size);
	}
	return value;
}

static char *
_gentl_get_id(GC_ERROR(*func)(void*, uint32_t, char*, size_t*), void *handle, uint32_t index)
{
	GC_ERROR error;
	size_t size;
	char *value = NULL;
	error = func(handle, index, NULL, &size);
	if (error == GC_ERR_SUCCESS) {
		value = g_malloc0(size);
		func(handle, index, value, &size);
	}
	return value;
}

static uint32_t
_gentl_refresh(GC_ERROR(*update)(void*, bool8_t *, uint64_t), GC_ERROR(*get)(void*, uint32_t*), void *handle)
{
	GC_ERROR error;
	uint32_t num = 0;

	error = update(handle, NULL, 100);
	if (error != GC_ERR_SUCCESS) {
		arv_warning_interface("_gentl_refresh: update %d", error);
		return 0;
	}

	error = get(handle, &num);
	if (error != GC_ERR_SUCCESS) {
		arv_warning_interface("_gentl_refresh: get %d", error);
		return 0;
	}

	return num;
}

static void
_discover (ArvGenTLInterface *gentl_interface, GArray *device_ids)
{
	ArvGenTLInterfacePrivate *priv = arv_gentl_interface_get_instance_private(gentl_interface);
	GList *gentl_systems = arv_gentl_get_systems();
	GList *gentl_systems_iter;

	g_hash_table_remove_all (priv->devices);

	/* Iterate over all discovered GenTL systems */
	for ( gentl_systems_iter = gentl_systems; gentl_systems_iter; gentl_systems_iter = g_list_next(gentl_systems_iter) ) {
		ArvGenTLSystem *gentl_system = gentl_systems_iter->data;
		ArvGenTLModule * gentl = arv_gentl_system_get_gentl(gentl_system);
		TL_HANDLE system_handle = arv_gentl_system_open_system_handle(gentl_system);
		uint32_t num_interfaces = _gentl_refresh(gentl->TLUpdateInterfaceList,
                                                         gentl->TLGetNumInterfaces, system_handle);

		/* Iterate over all interfaces of a GenTL system */
		for (uint32_t i=0; i<num_interfaces; i++) {
			char *interface_id, *interface_type;
			uint32_t num_devices = 0;
			IF_HANDLE interface_handle;

			interface_id = _gentl_get_id(gentl->TLGetInterfaceID, system_handle, i);
			if (interface_id == NULL)
				continue;

			interface_type = _gentl_get_info_str(gentl->TLGetInterfaceInfo,
                                                             system_handle, interface_id, INTERFACE_INFO_TLTYPE);

			interface_handle = arv_gentl_system_open_interface_handle(gentl_system, interface_id);
			if (interface_handle == NULL) {
				g_clear_pointer(&interface_id, g_free);
				g_clear_pointer(&interface_type, g_free);
				continue;
			}

			num_devices = _gentl_refresh(gentl->IFUpdateDeviceList, gentl->IFGetNumDevices, interface_handle);

			/* Iterate over all devices of a GenTL interface */
			for (uint32_t j=0; j<num_devices; j++) {
				char *device_id, *id, *model, *vendor, *serial_nbr;
				ArvGenTLInterfaceDeviceInfos *device_info;
				ArvInterfaceDeviceIds *ids;

				device_id = _gentl_get_id(gentl->IFGetDeviceID, interface_handle, j);

				id = _gentl_get_info_str(gentl->IFGetDeviceInfo,
                                                         interface_handle, device_id, DEVICE_INFO_ID);
				model = _gentl_get_info_str(gentl->IFGetDeviceInfo,
                                                            interface_handle, device_id, DEVICE_INFO_MODEL);
				vendor = _gentl_get_info_str(gentl->IFGetDeviceInfo,
                                                             interface_handle, device_id, DEVICE_INFO_VENDOR);
				serial_nbr =  _gentl_get_info_str(gentl->IFGetDeviceInfo,
                                                                  interface_handle, device_id, DEVICE_INFO_SERIAL_NUMBER);

				arv_info_interface ("Device: %s", device_id);
				arv_info_interface ("  ID: %s", id);
				arv_info_interface ("  VENDOR: %s", vendor);
				arv_info_interface ("  MODEL: %s", model);
				arv_info_interface ("  S/N: %s", serial_nbr);

				device_info =  arv_gentl_interface_device_infos_new(gentl_system, interface_id, id,
                                                                                    vendor, model, serial_nbr);
				g_hash_table_replace(priv->devices, device_info->id,
                                                     arv_gentl_interface_device_infos_ref(device_info));

				if (device_ids) {
					ids = g_new0 (ArvInterfaceDeviceIds, 1);
					ids->device = id;
					ids->model = model;
					ids->vendor = vendor;
					ids->serial_nbr = serial_nbr;
					ids->address = g_strdup(interface_type);
                                        ids->protocol = arv_protocol_from_transport_layer_type (interface_type);
					g_array_append_val (device_ids, ids);
				} else {
					g_free(id);
					g_free(model);
					g_free(vendor);
					g_free(serial_nbr);
				}
				g_clear_pointer(&device_id, g_free);
				arv_gentl_interface_device_infos_unref(device_info);
			}
			arv_gentl_system_close_interface_handle(gentl_system, interface_id);

			g_clear_pointer(&interface_id, g_free);
			g_clear_pointer(&interface_type, g_free);
		}
		arv_gentl_system_close_system_handle(gentl_system);
	}
}

static void
arv_gentl_interface_update_device_list (ArvInterface *interface, GArray *device_ids)
{
	ArvGenTLInterface *gentl_interface = ARV_GENTL_INTERFACE (interface);

	g_assert (device_ids->len == 0);

	_discover(gentl_interface, device_ids);
}

static ArvDevice *
arv_gentl_interface_open_device (ArvInterface *interface, const char *device_id, GError **error)
{
	ArvGenTLInterfacePrivate *priv = arv_gentl_interface_get_instance_private(ARV_GENTL_INTERFACE (interface));
	ArvDevice *device = NULL;
	ArvGenTLInterfaceDeviceInfos *device_info = g_hash_table_lookup(priv->devices, device_id);

	/* Refresh devices if the requested device is in the cache. */
	if (device_info == NULL) {
		_discover(ARV_GENTL_INTERFACE(interface), NULL);
		device_info = g_hash_table_lookup(priv->devices, device_id);
	}

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
}
