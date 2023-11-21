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
 * SECTION: arvgentldevice
 * @short_description: GenTL device
 */

#include <arvdeviceprivate.h>
#include <arvgentldeviceprivate.h>
#include <arvgentlsystemprivate.h>
#include <arvgentlinterfaceprivate.h>
#include <arvgentlstreamprivate.h>
#include <arvmiscprivate.h>
#include <arvgc.h>
#include <arvdebugprivate.h>
#include <arvzip.h>
#include <arvstr.h>

enum
{
	PROP_0,
	PROP_GENTL_SYSTEM,
	PROP_GENTL_INTERFACE_ID,
	PROP_GENTL_DEVICE_ID
};

typedef struct {
	ArvGc *genicam;

	char *genicam_xml;
	size_t genicam_xml_size;

	char *interface_id;
	char *device_id;

	ArvGenTLSystem *gentl_system;
	GList *gentl_streams;
	void *device_handle;
	void *port_handle;

#if ARAVIS_HAS_EVENT
	GHashTable *event_data;
	int event_thread_run;
	GThread* event_thread;
#endif
} ArvGenTLDevicePrivate;

struct _ArvGenTLDevice {
	ArvDevice device;
};

struct _ArvGenTLDeviceClass {
	ArvDeviceClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvGenTLDevice, arv_gentl_device, ARV_TYPE_DEVICE, G_ADD_PRIVATE (ArvGenTLDevice))

uint64_t
arv_gentl_device_get_timestamp_tick_frequency(ArvGenTLDevice *device)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (device);
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(priv->gentl_system);

	uint64_t timestamp_tick_frequency = 0;
	INFO_DATATYPE type;
	size_t size;
	GC_ERROR error;

	error = gentl->DevGetInfo(priv->device_handle, DEVICE_INFO_TIMESTAMP_FREQUENCY, &type, &timestamp_tick_frequency, &size);

	return error == GC_ERR_SUCCESS ? timestamp_tick_frequency : 0;
}

DS_HANDLE
arv_gentl_device_open_stream_handle(ArvGenTLDevice *device)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (device);
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(priv->gentl_system);

	uint32_t num_streams = 0;
	char stream_id[1024] = "\0";
	size_t size = 1024;
	GC_ERROR error;
	DS_HANDLE stream_handle = NULL;

	error = gentl->DevGetNumDataStreams(priv->device_handle, &num_streams);
	if (error != GC_ERR_SUCCESS || num_streams < 1) {
		arv_warning_device("DevGetNumDataStreams: %d", error);
		return NULL;
	}

	error = gentl->DevGetDataStreamID(priv->device_handle, 0, stream_id, &size);
	if (error != GC_ERR_SUCCESS) {
		arv_warning_device("DevGetDataStreamID: %d", error);
		return NULL;
	}

	error = gentl->DevOpenDataStream(priv->device_handle, stream_id, &stream_handle);
	if (error != GC_ERR_SUCCESS) {
		arv_warning_device("DevOpenDataStream['%s']: %d", stream_id, error);
		return NULL;
	}

	return stream_handle;
}

ArvGenTLSystem *
arv_gentl_device_get_system(ArvGenTLDevice *device)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (device);
	return priv->gentl_system;
}

void
arv_gentl_device_start_acquisition(ArvGenTLDevice *device)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (device);
	GList *iter = priv->gentl_streams;

	iter = priv->gentl_streams;
	while (iter != NULL) {
		GWeakRef *stream_weak_ref = iter->data;
		ArvGenTLStream *gentl_stream = g_weak_ref_get(stream_weak_ref);

		iter = g_list_next(iter);

		if (gentl_stream) {
			arv_gentl_stream_start_acquisition(gentl_stream);
			g_object_unref(gentl_stream);
		} else {
			priv->gentl_streams = g_list_remove(priv->gentl_streams, stream_weak_ref);
			g_weak_ref_clear(stream_weak_ref);
			g_free(stream_weak_ref);
		}
	}
}

void
arv_gentl_device_stop_acquisition(ArvGenTLDevice *device)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (device);
	GList *iter = priv->gentl_streams;

	while (iter != NULL) {
		GWeakRef *stream_weak_ref = iter->data;
		ArvGenTLStream *gentl_stream = g_weak_ref_get(stream_weak_ref);

		iter = g_list_next(iter);

		if (gentl_stream) {
			arv_gentl_stream_stop_acquisition(gentl_stream);
			g_object_unref(gentl_stream);
		} else {
			priv->gentl_streams = g_list_remove(priv->gentl_streams, stream_weak_ref);
			g_weak_ref_clear(stream_weak_ref);
			g_free(stream_weak_ref);
		}
	}
}

#if ARAVIS_HAS_EVENT
static char *
_get_event_data(ArvGenTLModule *gentl, EVENT_HANDLE event_handle, void *event_data, size_t event_data_size, EVENT_DATA_INFO_CMD cmd)
{
	INFO_DATATYPE type;
	size_t size;
	char *value = NULL;
	GC_ERROR error;
	error = gentl->EventGetDataInfo(event_handle, event_data, event_data_size, cmd, &type, NULL, &size);
	if (error == GC_ERR_SUCCESS) {
		value = g_malloc(size);
		error = gentl->EventGetDataInfo(event_handle, event_data, event_data_size, cmd, &type, value, &size);
		if (error != GC_ERR_SUCCESS)
			g_clear_pointer(&value, g_free);
	}

	return value;
}

static gpointer
event_thread_func(void *p)
{
	ArvGenTLDevice *gentl_device = (ArvGenTLDevice*) p;
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private(gentl_device);
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(priv->gentl_system);
	EVENT_HANDLE event_handle;
	INFO_DATATYPE type;
	size_t size, event_data_size_max;
	char *event_data;
	GC_ERROR error;

	error = gentl->GCRegisterEvent(priv->device_handle, EVENT_REMOTE_DEVICE, &event_handle);
	if (error != GC_ERR_SUCCESS) {
		arv_warning_device("GCRegisterEvent[REMOTE_DEVICE]: %d\n", error);
	}

	size = sizeof(event_data_size_max);
	error = gentl->EventGetInfo(event_handle, EVENT_SIZE_MAX, &type, &event_data_size_max, &size);
	event_data = g_malloc0(event_data_size_max);

        while (priv->event_thread_run)
        {
		char *event_data_id, *event_data_value;
		int event_data_id_number = 0;
		size_t event_data_size = event_data_size_max;
		error = gentl->EventGetData(event_handle, event_data, &event_data_size, 100);
		if (error != GC_ERR_SUCCESS) {
			if (error != GC_ERR_ABORT && error != GC_ERR_TIMEOUT)
				arv_warning_device("EventGetData[REMOTE_DEVICE]: %d\n", error);
			continue;
		}
		/* Get event id and value */
		event_data_id = _get_event_data(gentl, event_handle, event_data, event_data_size, EVENT_DATA_ID);
		event_data_value = _get_event_data(gentl, event_handle, event_data, event_data_size, EVENT_DATA_VALUE);
		if (event_data_id && event_data_value) {
			/* Cache the event data */
			event_data_id_number = (int) g_ascii_strtoll(event_data_id, NULL, 16);
			g_hash_table_replace(priv->event_data, GINT_TO_POINTER(event_data_id_number), event_data_value);
			/* Emit the device-event signal */
			arv_device_emit_device_event_signal( ARV_DEVICE(gentl_device), event_data_id_number);
		}
        }

	error = gentl->GCUnregisterEvent(priv->device_handle, EVENT_REMOTE_DEVICE);
	if (error != GC_ERR_SUCCESS)
		arv_warning_device("GCUnregisterEvent[REMOTE_DEVICE]: %d\n", error);

	g_free(event_data);

        return NULL;
}
#endif /* ARAVIS_HAS_EVENT */

/* ArvGenTLDevice implementation */

/* ArvDevice implementation */

ArvDevice *
arv_gentl_device_new (ArvGenTLSystem *gentl_system, const char *interface_id, const char *device_id, GError **error)
{
	return g_initable_new (ARV_TYPE_GENTL_DEVICE, NULL, error,
				"gentl-system", gentl_system,
				"interface-id", interface_id,
				"device-id", device_id,
				NULL);
}

static ArvStream *
arv_gentl_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data,
                                GDestroyNotify destroy, GError **error)
{
	ArvGenTLDevice *gentl_device = ARV_GENTL_DEVICE (device);
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (gentl_device);
	ArvStream *stream;
	GWeakRef *stream_weak_ref;

	stream = arv_gentl_stream_new (gentl_device, callback, user_data, destroy, error);
	if (!ARV_IS_STREAM (stream))
		return NULL;

	stream_weak_ref = g_new(GWeakRef, 1);
	g_weak_ref_init(stream_weak_ref, ARV_GENTL_STREAM(stream));
	priv->gentl_streams = g_list_append(priv->gentl_streams, stream_weak_ref);

	return stream;
}

static const char *
arv_gentl_device_get_genicam_xml (ArvDevice *device, size_t *size)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (ARV_GENTL_DEVICE (device));

	*size = priv->genicam_xml_size;
	return priv->genicam_xml;
}

static ArvGc *
arv_gentl_device_get_genicam (ArvDevice *device)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (ARV_GENTL_DEVICE (device));

	return priv->genicam;
}

static gboolean
arv_gentl_device_read_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (ARV_GENTL_DEVICE (device));
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(priv->gentl_system);

	size_t sizet = size;
	GC_ERROR error_code = gentl->GCReadPort(priv->port_handle, address, buffer, &sizet);

        if (error_code != GC_ERR_SUCCESS) {
                g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                             "Read memory error (%s)", arv_gentl_gc_error_to_string (error_code));
                return FALSE;
        }

	return TRUE;
}

static gboolean
arv_gentl_device_write_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (ARV_GENTL_DEVICE (device));
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(priv->gentl_system);

	size_t sizet = size;
	GC_ERROR error_code =  gentl->GCWritePort(priv->port_handle, address, buffer, &sizet);

        if (error_code != GC_ERR_SUCCESS) {
                g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                             "Write memory error (%s)", arv_gentl_gc_error_to_string (error_code));
                return FALSE;
        }

	return TRUE;
}

static gboolean
arv_gentl_device_read_register (ArvDevice *device, guint64 address, guint32 *value, GError **error)
{
	return arv_gentl_device_read_memory(device, address, sizeof(guint32), value, error);
}

static gboolean
arv_gentl_device_write_register (ArvDevice *device, guint64 address, guint32 value, GError **error)
{
	return arv_gentl_device_write_memory(device, address, sizeof(guint32), &value, error);
}

#if ARAVIS_HAS_EVENT
static gboolean
arv_gentl_device_read_event_data (ArvDevice *device, int event_id, guint64 address, guint32 size, void *buffer, GError **error)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private ( ARV_GENTL_DEVICE(device) );

	char *event_value = g_hash_table_lookup(priv->event_data, GINT_TO_POINTER(event_id));
	if (event_value == NULL) {
		g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_UNKNOWN,
			     "Event not found");
		return FALSE;
	}
	memcpy(buffer, event_value + address, size);
	return TRUE;
}
#endif /* ARAVIS_HAS_EVENT */

static void
arv_gentl_device_init (ArvGenTLDevice *gentl_device)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (gentl_device);

	priv->genicam = NULL;
	priv->genicam_xml = NULL;
	priv->genicam_xml_size = 0;
	priv->gentl_system = NULL;
	priv->gentl_streams = NULL;
	priv->device_handle = NULL;
	priv->interface_id = NULL;
	priv->device_id = NULL;
	priv->port_handle = NULL;
#if ARAVIS_HAS_EVENT
	priv->event_data = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
#endif
}

static char *
_load_genicam (ArvGenTLDevice *gentl_device, size_t  *size, char **url, GError **error)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (gentl_device);
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(priv->gentl_system);
        GError *local_error = NULL;
	char filename[1024];
	size_t filename_size = 1024;
	char *genicam = NULL;
	char *scheme = NULL;
	char *path = NULL;
	guint64 file_address;
	guint64 file_size;
	INFO_DATATYPE info_data_type;
        GC_ERROR gc_error;

	g_return_val_if_fail (size != NULL, NULL);
        g_return_val_if_fail (url != NULL, NULL);

	*size = 0;
        *url = NULL;

	gc_error = gentl->GCGetPortURLInfo(priv->port_handle, 0, URL_INFO_URL, &info_data_type, filename, &filename_size);
	if (gc_error != GC_ERR_SUCCESS) {
		arv_warning_device("GCGetPortURLInfo: %d", gc_error);
                g_set_error (error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_NOT_FOUND,
                             "Failed to get PortURLInfo (%s)", arv_gentl_gc_error_to_string (gc_error));
		return NULL;
        }

	arv_info_device ("[GenTLDevice::load_genicam] xml url = '%s'", filename);

	arv_parse_genicam_url (filename, -1, &scheme, NULL, &path, NULL, NULL, &file_address, &file_size);

	if (g_ascii_strcasecmp (scheme, "file") == 0) {
		gsize len;

		g_file_get_contents (path, &genicam, &len, NULL);
		if (genicam) {
			*size = len;
                        *url = g_strdup (filename);
                }
	} else if (g_ascii_strcasecmp (scheme, "local") == 0) {
		arv_info_device ("[GenTLDevice::load_genicam] Xml address = 0x%" G_GINT64_MODIFIER "x - "
				  "size = 0x%" G_GINT64_MODIFIER "x - %s", file_address, file_size, path);

		if (file_size > 0) {
			genicam = g_malloc (file_size);
			if (arv_gentl_device_read_memory (ARV_DEVICE (gentl_device), file_address, file_size,
						    genicam, &local_error)) {

				if (arv_debug_check (ARV_DEBUG_CATEGORY_MISC, ARV_DEBUG_LEVEL_DEBUG)) {
					GString *string = g_string_new ("");

					g_string_append_printf (string,
								"[GenTLDevice::load_genicam] Raw data size = 0x%"
								G_GINT64_MODIFIER "x\n", file_size);
					arv_g_string_append_hex_dump (string, genicam, file_size);

					arv_debug_misc ("%s", string->str);

					g_string_free (string, TRUE);
				}

				if (g_str_has_suffix (path, ".zip")) {
					ArvZip *zip;
					const GSList *zip_files;

					arv_info_device ("[GenTLDevice::load_genicam] Zipped xml data");

					zip = arv_zip_new (genicam, file_size);
					zip_files = arv_zip_get_file_list (zip);

					if (zip_files != NULL) {
						const char *zip_filename;
						void *tmp_buffer;
						size_t tmp_buffer_size;

						zip_filename = arv_zip_file_get_name (zip_files->data);
						tmp_buffer = arv_zip_get_file (zip, zip_filename,
									       &tmp_buffer_size);

						g_free (genicam);
                                                *size = tmp_buffer_size;
						genicam = tmp_buffer;
					} else {
						arv_warning_device ("[GenTLDevice::load_genicam] Invalid format");
                                                g_clear_pointer (&genicam, g_free);
                                        }
					arv_zip_free (zip);
				} else {
                                        *size = file_size;
                                }

                                if (genicam != NULL)
                                        *url = g_strdup_printf ("%s:///%s;%lx;%lx", scheme, path,
                                                                file_address, file_size);
                        } else {
                                g_clear_pointer (&genicam, g_free);
                        }
                }
        } else if (g_ascii_strcasecmp (scheme, "http")) {
                GFile *file;
                GFileInputStream *stream;

		file = g_file_new_for_uri (filename);
		stream = g_file_read (file, NULL, NULL);
		if(stream) {
			GDataInputStream *data_stream;
			gsize len;

			data_stream = g_data_input_stream_new (G_INPUT_STREAM (stream));
			genicam = g_data_input_stream_read_upto (data_stream, "", 0, &len, NULL, NULL);

			if (genicam) {
				*size = len;
                                *url = g_strdup (filename);
                        }

			g_object_unref (data_stream);
			g_object_unref (stream);
		}
		g_object_unref (file);
	} else {
		g_critical ("Unkown GENICAM url scheme: '%s'", filename);
	}

        if (local_error != NULL) {
                arv_warning_device("Failed to load GENICAM data: %s", local_error->message);
                g_propagate_error (error, local_error);
        }

	g_free (scheme);
	g_free (path);

	return genicam;
}

static void
arv_gentl_device_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (ARV_GENTL_DEVICE (self));

	switch (prop_id)
	{
		case PROP_GENTL_SYSTEM:
			priv->gentl_system = g_value_get_object(value);
			break;
		case PROP_GENTL_INTERFACE_ID:
			g_clear_pointer (&priv->interface_id, g_free);
			priv->interface_id = g_value_dup_string(value);
			break;
		case PROP_GENTL_DEVICE_ID:
			g_clear_pointer (&priv->device_id, g_free);
			priv->device_id = g_value_dup_string(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
			break;
	}
}

static void
arv_gentl_device_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (ARV_GENTL_DEVICE (object));

	switch (prop_id)
	{
		case PROP_GENTL_SYSTEM:
			g_value_set_object(value, priv->gentl_system);
			break;
		case PROP_GENTL_INTERFACE_ID:
			g_value_set_string(value, priv->interface_id);
			break;
		case PROP_GENTL_DEVICE_ID:
			g_value_set_string(value, priv->device_id);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
arv_gentl_device_constructed (GObject *self)
{
	ArvGenTLDevice *gentl_device = ARV_GENTL_DEVICE (self);
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (gentl_device);
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(priv->gentl_system);
        char *url = NULL;

	G_OBJECT_CLASS (arv_gentl_device_parent_class)->constructed (self);

	priv->device_handle = arv_gentl_system_open_device_handle(priv->gentl_system, priv->interface_id, priv->device_id);
	if (priv->device_handle == NULL) {
		arv_device_take_init_error (ARV_DEVICE (gentl_device),
                                            g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                                                         "Failed to open device '%s' of interface '%s'",
                                                         priv->device_id, priv->interface_id));
		return;
	}

	gentl->DevGetPort(priv->device_handle, &priv->port_handle);
	if (priv->port_handle == NULL) {
		arv_device_take_init_error (ARV_DEVICE (gentl_device),
                                            g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_PROTOCOL_ERROR,
                                                         "Failed to open port to remove device '%s' of interface '%s'",
                                                         priv->device_id, priv->interface_id));
		return;
	}

	priv->genicam_xml = _load_genicam (gentl_device, &priv->genicam_xml_size, &url, NULL);
	if (priv->genicam_xml == NULL) {
		arv_device_take_init_error (ARV_DEVICE (gentl_device),
                                            g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_GENICAM_NOT_FOUND,
                                                         "Failed to load GenICam data for device '%s' of interface '%s'",
                                                         priv->device_id, priv->interface_id));
		return;
	}

	priv->genicam = arv_gc_new (ARV_DEVICE(self), priv->genicam_xml, priv->genicam_xml_size);
        arv_dom_document_set_url (ARV_DOM_DOCUMENT(priv->genicam), url);
        g_free (url);

#if ARAVIS_HAS_EVENT
	priv->event_thread_run = 1;
	priv->event_thread = g_thread_new ( "arv_gentl_device_event", event_thread_func, gentl_device);
#endif
}

static void
arv_gentl_device_finalize (GObject *object)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (ARV_GENTL_DEVICE (object));

#if ARAVIS_HAS_EVENT
	priv->event_thread_run = 0;
        if (priv->event_thread)
                g_thread_join (priv->event_thread);
#endif

	arv_gentl_system_close_device_handle(priv->gentl_system, priv->interface_id, priv->device_handle);

	for (GList *iter=priv->gentl_streams; iter; iter=g_list_next(iter))
		g_weak_ref_clear(iter->data);
	g_list_free(priv->gentl_streams);

	g_clear_object (&priv->genicam);
	g_clear_pointer (&priv->genicam_xml, g_free);

	g_clear_pointer(&priv->interface_id, g_free);
	g_clear_pointer(&priv->device_id, g_free);

#if ARAVIS_HAS_EVENT
	g_hash_table_unref(priv->event_data);
#endif

	G_OBJECT_CLASS (arv_gentl_device_parent_class)->finalize (object);
}

static void
arv_gentl_device_class_init (ArvGenTLDeviceClass *gentl_device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (gentl_device_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (gentl_device_class);

	object_class->finalize = arv_gentl_device_finalize;
	object_class->constructed = arv_gentl_device_constructed;
	object_class->set_property = arv_gentl_device_set_property;
	object_class->get_property = arv_gentl_device_get_property;

	device_class->create_stream = arv_gentl_device_create_stream;
	device_class->get_genicam_xml = arv_gentl_device_get_genicam_xml;
	device_class->get_genicam = arv_gentl_device_get_genicam;
	device_class->read_memory = arv_gentl_device_read_memory;
	device_class->write_memory = arv_gentl_device_write_memory;
	device_class->read_register = arv_gentl_device_read_register;
	device_class->write_register = arv_gentl_device_write_register;
#if ARAVIS_HAS_EVENT
	device_class->read_event_data = arv_gentl_device_read_event_data;
#endif

	g_object_class_install_property
		(object_class,
		 PROP_GENTL_SYSTEM,
		 g_param_spec_object ("gentl-system",
				      "GenTL system",
				      "GenTL system",
				      ARV_TYPE_GENTL_SYSTEM,
				      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(object_class,
		 PROP_GENTL_INTERFACE_ID,
		 g_param_spec_string ("interface-id",
				      "Interface ID",
				      "The ID of the interface",
				      NULL,
				      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(object_class,
		 PROP_GENTL_DEVICE_ID,
		 g_param_spec_string ("device-id",
				      "Device ID",
				      "The ID of the device",
				      NULL,
				      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
