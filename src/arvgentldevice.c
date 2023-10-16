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
	void *device_handle;
	void *port_handle;
} ArvGenTLDevicePrivate;

struct _ArvGenTLDevice {
	ArvDevice device;
};

struct _ArvGenTLDeviceClass {
	ArvDeviceClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvGenTLDevice, arv_gentl_device, ARV_TYPE_DEVICE, G_ADD_PRIVATE (ArvGenTLDevice))

void *
arv_gentl_device_open_stream(ArvGenTLDevice *device)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (device);
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(priv->gentl_system);

	uint32_t num_streams = 0;
	char stream_id[1024];
	size_t size = 1024;
	DS_HANDLE hDataStream = NULL;

	gentl->DevGetNumDataStreams(priv->device_handle, &num_streams);
	if (num_streams < 1)
		return NULL;

	gentl->DevGetDataStreamID(priv->device_handle, 0, stream_id, &size);
	gentl->DevOpenDataStream(priv->device_handle, stream_id, &hDataStream);

	return hDataStream;
}

ArvGenTLSystem *
arv_gentl_device_get_system(ArvGenTLDevice *device)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (ARV_GENTL_DEVICE (device));
	return priv->gentl_system;
}

/* ArvGenTLDevice implemenation */

/* ArvDevice implemenation */

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
arv_gentl_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data, GDestroyNotify destroy, GError **error)
{
	ArvGenTLDevice *gentl_device = ARV_GENTL_DEVICE (device);
	ArvStream *stream;

	stream = arv_gentl_stream_new (gentl_device, callback, user_data, destroy, error);
	if (!ARV_IS_STREAM (stream))
		return NULL;

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
	size = (guint32) sizet;
	return error_code == GC_ERR_SUCCESS;
}

static gboolean
arv_gentl_device_write_memory (ArvDevice *device, guint64 address, guint32 size, void *buffer, GError **error)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (ARV_GENTL_DEVICE (device));
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(priv->gentl_system);

	size_t sizet = size;
	GC_ERROR error_code =  gentl->GCWritePort(priv->port_handle, address, buffer, &sizet);
	size = (guint32) sizet;
	return error_code == GC_ERR_SUCCESS;
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

static void
arv_gentl_device_init (ArvGenTLDevice *gentl_device)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (gentl_device);

	priv->genicam = NULL;
	priv->genicam_xml = NULL;
	priv->genicam_xml_size = 0;
	priv->gentl_system = NULL;
	priv->device_handle = NULL;
	priv->interface_id = NULL;
	priv->device_id = NULL;
	priv->port_handle = NULL;
}

static void
arv_gentl_device_finalize (GObject *object)
{
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (ARV_GENTL_DEVICE (object));

	arv_gentl_system_close_device_handle(priv->gentl_system, priv->interface_id, priv->device_handle);

	g_clear_object (&priv->genicam);
	g_clear_pointer (&priv->genicam_xml, g_free);
	
	g_clear_pointer(&priv->interface_id, g_free);
	g_clear_pointer(&priv->device_id, g_free);

	G_OBJECT_CLASS (arv_gentl_device_parent_class)->finalize (object);
}

static void
arv_gentl_device_constructed (GObject *self)
{
	ArvGenTLDevice *gentl_device = ARV_GENTL_DEVICE (self);
	ArvGenTLDevicePrivate *priv = arv_gentl_device_get_instance_private (gentl_device);
	ArvGenTLModule *gentl = arv_gentl_system_get_gentl(priv->gentl_system);

	char *scheme = NULL;
	char *path = NULL;
	guint64 file_address;
	guint64 file_size;
	char url[1024] = {'\0'};
	size_t size = 1024;
	INFO_DATATYPE type;

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
                                                         "Failed to open device '%s' of interface '%s'",
                                                         priv->device_id, priv->interface_id));
		return;
	}

	gentl->GCGetPortURLInfo(priv->port_handle, 0, URL_INFO_URL, &type, url, &size);
	arv_info_device ("[GenTLDevice::constructed] xml url = '%s'", url);

	arv_parse_genicam_url (url, -1, &scheme, NULL, &path, NULL, NULL, &file_address, &file_size);

	priv->genicam_xml = g_malloc(file_size);
	priv->genicam_xml_size = file_size;
	gentl->GCReadPort(priv->port_handle, file_address, priv->genicam_xml, &priv->genicam_xml_size);

	if (g_str_has_suffix (path, ".zip")) {
		ArvZip *zip;
		const GSList *zip_files;

		zip = arv_zip_new (priv->genicam_xml, priv->genicam_xml_size);
		zip_files = arv_zip_get_file_list (zip);

		if (zip_files != NULL) {
			const char *zip_filename;
			void *tmp_buffer;
			size_t tmp_buffer_size;

			zip_filename = arv_zip_file_get_name (zip_files->data);
			tmp_buffer = arv_zip_get_file (zip, zip_filename, &tmp_buffer_size);

			g_free (priv->genicam_xml);
			priv->genicam_xml_size = tmp_buffer_size;
			priv->genicam_xml = tmp_buffer;
		} else {
			arv_warning_device ("[GvDevice::load_genicam] Invalid format");
			arv_zip_free (zip);
		}
	}

	priv->genicam = arv_gc_new (ARV_DEVICE(self), priv->genicam_xml, priv->genicam_xml_size);
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
