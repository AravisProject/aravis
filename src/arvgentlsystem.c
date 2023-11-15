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
 * SECTION: arvgentlsystem
 * @short_description: GenTL system module.
 *
 * #ArvGenTLSystem represents a GenTL module.
 */

#include <gmodule.h>

#include <arvgentlsystemprivate.h>
#include <arvgentlinterfaceprivate.h>
#include <arvdebugprivate.h>

enum
{
	PROP_0,
	PROP_GENTL_SYSTEM_FILENAME
};

typedef struct {
	char *id;
	char *name;
	IF_HANDLE handle;
	grefcount ref_count;
	ArvGenTLSystem *system;
} Interface;

typedef struct {
	char *filename;
	gboolean is_valid;
	ArvGenTLModule gentl;
	TL_HANDLE system_handle;
	GModule *module;
	GHashTable *interfaces;
	volatile grefcount ref_count;
} ArvGenTLSystemPrivate;

struct _ArvGenTLSystem {
	GObject object;
};

struct _ArvGenTLSystemClass {
	GObjectClass parent_class;
};


G_DEFINE_TYPE_WITH_CODE (ArvGenTLSystem, arv_gentl_system, G_TYPE_OBJECT, G_ADD_PRIVATE (ArvGenTLSystem))

static char *
_gentl_get_info_str (GC_ERROR(*func)(void*, const char *, int32_t, int32_t*, void*, size_t*), void *handle, const char
                     *id, BUFFER_INFO_CMD info_cmd)
{
	GC_ERROR error;
	INFO_DATATYPE type;
	size_t size;
	char *value = NULL;
	error = func(handle, id, info_cmd, &type, NULL, &size);
	if (error == GC_ERR_SUCCESS) {
		value = g_malloc0(size);
		func(handle, id, info_cmd, &type, value, &size);
	}
	return value;
}

static Interface *
interface_new(ArvGenTLSystem *system, const char *id)
{
	ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (system);
	IF_HANDLE handle = NULL;
	Interface *interface = NULL;
	GC_ERROR error;

	g_return_val_if_fail (system != NULL, NULL);
	g_return_val_if_fail (id != NULL, NULL);

	arv_gentl_system_open_system_handle(system);

	arv_info_interface("TLOpenInterface '%s'", id);

	error = priv->gentl.TLOpenInterface(priv->system_handle, id, &handle);
	if (error != GC_ERR_SUCCESS) {
		arv_warning_interface("TLOpenInterface('%s'): %d", id, error);
		return NULL;
	}

	interface = g_new (Interface, 1);
	interface->id = g_strdup(id);
	interface->name = _gentl_get_info_str(priv->gentl.TLGetInterfaceInfo, priv->system_handle, id, INTERFACE_INFO_DISPLAYNAME);
	interface->handle = handle;
	interface->system = system;
	interface->ref_count = 1;

	return interface;
}

static Interface *
interface_ref(Interface *interface)
{
	g_return_val_if_fail (interface != NULL, NULL);
	g_return_val_if_fail (g_atomic_int_get (&interface->ref_count) > 0, NULL);

	g_atomic_int_inc (&interface->ref_count);

	return interface;
}

static void
interface_unref (Interface *interface)
{
	GC_ERROR error;

	g_return_if_fail (interface != NULL);
	g_return_if_fail (g_atomic_int_get (&interface->ref_count) > 0);

	if (g_atomic_int_dec_and_test (&interface->ref_count)) {
		ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (interface->system);
		arv_info_interface("IFClose '%s'", interface->id);
		error = priv->gentl.IFClose(interface->handle);
		if (error != GC_ERR_SUCCESS)
			arv_warning_interface("IFClose: %d", error);
		arv_gentl_system_close_system_handle(interface->system);
		g_clear_pointer (&interface->id, g_free);
		g_clear_pointer (&interface->name, g_free);
		g_clear_pointer (&interface, g_free);
	}
}

static GFile *
_read_link(GFile* file)
{
#ifdef G_OS_UNIX
	char *resolved_path = realpath(g_file_peek_path(file), NULL);
	GFile *resolved_file = NULL;
	if (resolved_path) {
		resolved_file = g_file_new_for_path(resolved_path);
		free(resolved_path);
	}
	return resolved_file;
#else
	return g_object_ref(file);
#endif
}

static GHashTable *
_get_gentl_files(void)
{
	GHashTable *gentl_files = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	const gchar *genicam_gentl_path_env = (sizeof(void *) == 8 ? "GENICAM_GENTL64_PATH" : "GENICAM_GENTL32_PATH");
	gchar **paths = NULL;
	const gchar *genicam_gentl_path;
	GFileEnumerator *file_enumerator;
	GFileInfo *file_info = NULL;

	genicam_gentl_path = g_getenv(genicam_gentl_path_env);
	if (!genicam_gentl_path) {
		arv_warning_interface("No \"%s\" defined", genicam_gentl_path_env);
		return gentl_files;
	}
	arv_info_interface("%s=%s", genicam_gentl_path_env, genicam_gentl_path);

	paths = g_strsplit(genicam_gentl_path, G_SEARCHPATH_SEPARATOR_S, -1);

	for (guint i=0; i<g_strv_length(paths); i++) {
		GFile *path = g_file_new_for_path(paths[i]);
		GFileType file_type = g_file_query_file_type(path, G_FILE_QUERY_INFO_NONE, NULL);

		switch (file_type) {
			case G_FILE_TYPE_REGULAR:
			{
				/* If a file is given, assume it is a CTI file. */
				GFile *resolved_file = _read_link(path);
				if (resolved_file) {
					arv_info_interface("Add %s", g_file_peek_path(resolved_file));
					g_hash_table_replace(gentl_files, g_file_get_path(resolved_file), NULL);
					g_object_unref(resolved_file);
				}
			}
			break;
			case G_FILE_TYPE_DIRECTORY:
			{
				/* Search path for "*.cti" file. */
				file_enumerator = g_file_enumerate_children(path, "standard::*",
					G_FILE_QUERY_INFO_NONE, NULL, NULL);
				while (file_enumerator && (file_info = g_file_enumerator_next_file (file_enumerator,
                                                                                                    NULL, NULL))) {
					/* Only files with ".cti" suffix. */
					if ( g_file_info_get_file_type(file_info) == G_FILE_TYPE_REGULAR &&
					     g_str_has_suffix(g_file_info_get_name(file_info), ".cti")) {
						GFile *file = g_file_enumerator_get_child(file_enumerator, file_info);
						GFile *resolved_file = _read_link(file);
						if (resolved_file) {
							arv_info_interface("Add file \"%s\"",
                                                                           g_file_peek_path(resolved_file));
							g_hash_table_replace(gentl_files, g_file_get_path(resolved_file),
                                                                             NULL);
							g_object_unref(resolved_file);
						}
						g_object_unref(file);
					}
					g_object_unref(file_info);
				}
				if (file_enumerator)
					g_object_unref(file_enumerator);
			}
			break;
			default:
				arv_warning_interface("Skip invalid path \"%s\"", paths[i]);
			break;
			}
		g_object_unref(path);
	}
	g_strfreev(paths);

	return gentl_files;
}

static GList *arv_gentl_systems = NULL;

static ArvGenTLSystem *
arv_gentl_system_new (const char *gentl_filename, GError **error)
{
	return g_object_new (ARV_TYPE_GENTL_SYSTEM,
				"filename", gentl_filename,
				NULL);
}

GList *
arv_gentl_get_systems(void)
{
	GHashTable *gentl_files;
	GHashTableIter iter;
	gpointer key, value;

	if (arv_gentl_systems)
		return arv_gentl_systems;

	gentl_files = _get_gentl_files();
	g_hash_table_iter_init(&iter, gentl_files);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		GError *error;
		ArvGenTLSystem *system = arv_gentl_system_new((const char *)key, &error);
		ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (system);
		if (priv->is_valid)
			arv_gentl_systems = g_list_prepend(arv_gentl_systems, system);
		else
			g_object_unref(system);
	}
	g_hash_table_unref(gentl_files);

	return arv_gentl_systems;
}

ArvGenTLModule *
arv_gentl_system_get_gentl(ArvGenTLSystem *system)
{
	ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (system);
	return &priv->gentl;
}

TL_HANDLE
arv_gentl_system_open_system_handle(ArvGenTLSystem *system)
{
	ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (system);
	ArvGenTLModule *gentl = &priv->gentl;
	GC_ERROR error;

	if (!priv->system_handle) {
		TL_HANDLE handle = NULL;
		arv_info_interface("TLOpen: %s", priv->filename);
		error = gentl->TLOpen(&handle);
		if (error != GC_ERR_SUCCESS)
			arv_warning_interface("TLOpen: %d", error);
		priv->system_handle = handle;
	}
	g_atomic_int_inc(&priv->ref_count);
	return priv->system_handle;
}

void
arv_gentl_system_close_system_handle (ArvGenTLSystem *system)
{
	ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (system);
	GC_ERROR error;

	if (g_atomic_int_dec_and_test (&priv->ref_count)) {
		if (priv->system_handle) {
			arv_info_interface("TLClose: %s", priv->filename);
			error = priv->gentl.TLClose(priv->system_handle);
			if (error != GC_ERR_SUCCESS)
				arv_warning_interface("TLClose: %d", error);
			priv->system_handle = NULL;
		}
	}
}

IF_HANDLE
arv_gentl_system_open_interface_handle (ArvGenTLSystem *system, const char *interface_id)
{
	ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (system);
	Interface *interface = NULL;

	interface = g_hash_table_lookup(priv->interfaces, interface_id);
	if (interface == NULL) {
		interface = interface_new(system, interface_id);
		if (interface == NULL)
			return NULL;
		g_hash_table_replace(priv->interfaces, g_strdup(interface_id), interface);
	} else {
		interface_ref(interface);
	}

	return interface->handle;
}

void
arv_gentl_system_close_interface_handle(ArvGenTLSystem *system, const char *interface_id)
{
	ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (system);
	Interface *interface = NULL;

	interface = g_hash_table_lookup(priv->interfaces, interface_id);
	if (interface) {
		/* If this is the last reference to this interface,
		   removing it from hashtable triggers the destruction. */
		if (interface->ref_count == 1)
			g_hash_table_remove(priv->interfaces, interface_id);
		else
			interface_unref(interface);
	}
}

DEV_HANDLE
arv_gentl_system_open_device_handle(ArvGenTLSystem *system, const char *interface_id, const char *device_id)
{
	ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (system);
	ArvGenTLModule *gentl = &priv->gentl;
	IF_HANDLE interface_handle = NULL;
	DEV_HANDLE device_handle = NULL;
	GC_ERROR error;

	/* Get interface handle */
	interface_handle = arv_gentl_system_open_interface_handle(system, interface_id);

	arv_info_interface("IFOpenDevice: '%s'", device_id);
	error = gentl->IFOpenDevice(interface_handle, device_id, DEVICE_ACCESS_CONTROL, &device_handle);
	if (error != GC_ERR_SUCCESS) {
		arv_warning_interface("IFOpenDevice: error %d", error);
		return NULL;
	}
	return device_handle;
}

void
arv_gentl_system_close_device_handle(ArvGenTLSystem *system, const char *interface_id, DEV_HANDLE *device_handle)
{
	ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (system);
	ArvGenTLModule *gentl = &priv->gentl;
	GC_ERROR error;

	error = gentl->DevClose(device_handle);
	if (error != GC_ERR_SUCCESS)
		arv_warning_interface("DevClose error %d", error);

	arv_gentl_system_close_interface_handle(system, interface_id);
}

static void
arv_gentl_system_init (ArvGenTLSystem *system)
{
	ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (system);

	priv->ref_count = 0;
	priv->system_handle = NULL;

	priv->filename = NULL;
	priv->module = NULL;
	priv->is_valid = FALSE;
	priv->interfaces = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)interface_unref);
	memset(&priv->gentl, 0, sizeof(ArvGenTLModule));
}

#define _ARV_GENTL_LOAD_SYMBOL( SYMBOL) \
	if (!g_module_symbol(module, #SYMBOL, (gpointer *)&priv->gentl.SYMBOL)) { \
		arv_warning_interface("%s:%s: %s", priv->filename, #SYMBOL,  g_module_error()); \
		break; \
	}


static void
arv_gentl_system_constructed (GObject *object)
{
	ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (ARV_GENTL_SYSTEM (object));
	GC_ERROR error;
	GModule *module;

	G_OBJECT_CLASS (arv_gentl_system_parent_class)->constructed (object);

	arv_info_interface("Open module %s", priv->filename);
	module = g_module_open(priv->filename, G_MODULE_BIND_LOCAL | G_MODULE_BIND_LAZY);
	if (module == NULL) {
		arv_warning_interface("Failed to load GenTL: %s\n", g_module_error());
		return;
	}

	do {
	_ARV_GENTL_LOAD_SYMBOL (GCInitLib);
	_ARV_GENTL_LOAD_SYMBOL (GCCloseLib);
	_ARV_GENTL_LOAD_SYMBOL (GCGetInfo);
	_ARV_GENTL_LOAD_SYMBOL (GCGetLastError);
	_ARV_GENTL_LOAD_SYMBOL (GCReadPort);
	_ARV_GENTL_LOAD_SYMBOL (GCWritePort);
	_ARV_GENTL_LOAD_SYMBOL (GCReadPortStacked);
	_ARV_GENTL_LOAD_SYMBOL (GCWritePortStacked);
	_ARV_GENTL_LOAD_SYMBOL (GCGetPortURL);
	_ARV_GENTL_LOAD_SYMBOL (GCGetPortInfo);
	_ARV_GENTL_LOAD_SYMBOL (GCGetNumPortURLs);
	_ARV_GENTL_LOAD_SYMBOL (GCGetPortURLInfo);
	_ARV_GENTL_LOAD_SYMBOL (GCRegisterEvent);
	_ARV_GENTL_LOAD_SYMBOL (GCUnregisterEvent);

	_ARV_GENTL_LOAD_SYMBOL (TLOpen);
	_ARV_GENTL_LOAD_SYMBOL (TLClose);
	_ARV_GENTL_LOAD_SYMBOL (TLGetInfo);
	_ARV_GENTL_LOAD_SYMBOL (TLGetNumInterfaces);
	_ARV_GENTL_LOAD_SYMBOL (TLGetInterfaceID);
	_ARV_GENTL_LOAD_SYMBOL (TLGetInterfaceInfo);
	_ARV_GENTL_LOAD_SYMBOL (TLOpenInterface);
	_ARV_GENTL_LOAD_SYMBOL (TLUpdateInterfaceList);

	_ARV_GENTL_LOAD_SYMBOL (IFClose);
	_ARV_GENTL_LOAD_SYMBOL (IFGetInfo);
	_ARV_GENTL_LOAD_SYMBOL (IFGetNumDevices);
	_ARV_GENTL_LOAD_SYMBOL (IFGetDeviceID);
	_ARV_GENTL_LOAD_SYMBOL (IFUpdateDeviceList);
	_ARV_GENTL_LOAD_SYMBOL (IFGetDeviceInfo);
	_ARV_GENTL_LOAD_SYMBOL (IFOpenDevice);
	_ARV_GENTL_LOAD_SYMBOL (IFGetParentTL);

	_ARV_GENTL_LOAD_SYMBOL (DevGetPort);
	_ARV_GENTL_LOAD_SYMBOL (DevGetNumDataStreams);
	_ARV_GENTL_LOAD_SYMBOL (DevGetDataStreamID);
	_ARV_GENTL_LOAD_SYMBOL (DevOpenDataStream);
	_ARV_GENTL_LOAD_SYMBOL (DevGetInfo);
	_ARV_GENTL_LOAD_SYMBOL (DevClose);
	_ARV_GENTL_LOAD_SYMBOL (DevGetParentIF);

	_ARV_GENTL_LOAD_SYMBOL (DSAnnounceBuffer);
	_ARV_GENTL_LOAD_SYMBOL (DSAllocAndAnnounceBuffer);
	_ARV_GENTL_LOAD_SYMBOL (DSFlushQueue);
	_ARV_GENTL_LOAD_SYMBOL (DSStartAcquisition);
	_ARV_GENTL_LOAD_SYMBOL (DSStopAcquisition);
	_ARV_GENTL_LOAD_SYMBOL (DSGetInfo);
	_ARV_GENTL_LOAD_SYMBOL (DSGetBufferID);
	_ARV_GENTL_LOAD_SYMBOL (DSClose);
	_ARV_GENTL_LOAD_SYMBOL (DSRevokeBuffer);
	_ARV_GENTL_LOAD_SYMBOL (DSQueueBuffer);
	_ARV_GENTL_LOAD_SYMBOL (DSGetBufferInfo);
	_ARV_GENTL_LOAD_SYMBOL (DSGetBufferChunkData);
	_ARV_GENTL_LOAD_SYMBOL (DSGetParentDev);
	_ARV_GENTL_LOAD_SYMBOL (DSGetNumBufferParts);
	_ARV_GENTL_LOAD_SYMBOL (DSGetBufferPartInfo);

	_ARV_GENTL_LOAD_SYMBOL (EventGetData);
	_ARV_GENTL_LOAD_SYMBOL (EventGetDataInfo);
	_ARV_GENTL_LOAD_SYMBOL (EventGetInfo);
	_ARV_GENTL_LOAD_SYMBOL (EventFlush);
	_ARV_GENTL_LOAD_SYMBOL (EventKill);

	error = priv->gentl.GCInitLib();
	if (error != GC_ERR_SUCCESS) {
		arv_warning_interface("GCInitLib: %d\n", error);
		break;
	}

	priv->module = module;
	priv->is_valid = TRUE;

	arv_debug_interface("ArvGenTLSystem::constructed[%p]: %s", ARV_GENTL_SYSTEM (object), priv->filename);
	return;
	} while (0);

	g_module_close(module);
	memset(&priv->gentl, 0, sizeof(ArvGenTLModule));
}

static void
arv_gentl_system_finalize (GObject *object)
{
	ArvGenTLSystem *system = ARV_GENTL_SYSTEM (object);
	ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (system);
	GC_ERROR error;

	arv_debug_interface("ArvGenTLSystem::finalize[%p]: %s", ARV_GENTL_SYSTEM (object), priv->filename);

	g_hash_table_unref (priv->interfaces);

	if (priv->is_valid) {
		error = priv->gentl.GCCloseLib();
		if (error)
			arv_warning_interface("GCCloseLib %d\n", error);

		priv->is_valid = FALSE;
		if (!g_module_close(priv->module))
			arv_warning_interface("Error in close modules \"%s\"", priv->filename);

	}
	g_free(priv->filename);

	G_OBJECT_CLASS (arv_gentl_system_parent_class)->finalize (object);
}

static void
arv_gentl_system_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (ARV_GENTL_SYSTEM (object));

	switch (prop_id)
	{
		case PROP_GENTL_SYSTEM_FILENAME:
			priv->filename = g_value_dup_string(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_gentl_system_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	ArvGenTLSystemPrivate *priv = arv_gentl_system_get_instance_private (ARV_GENTL_SYSTEM (object));

	switch (prop_id)
	{
		case PROP_GENTL_SYSTEM_FILENAME:
			g_value_set_string(value, priv->filename);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_gentl_system_class_init (ArvGenTLSystemClass *system_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (system_class);

	object_class->constructed = arv_gentl_system_constructed;
	object_class->finalize = arv_gentl_system_finalize;
	object_class->get_property = arv_gentl_system_get_property;
	object_class->set_property = arv_gentl_system_set_property;

	g_object_class_install_property
		(object_class,
		 PROP_GENTL_SYSTEM_FILENAME,
		 g_param_spec_string ("filename",
				      "GenTL filename",
				      "GenTL filename",
				      NULL,
				      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
