/* Aravis - Digital camera library
 *
 * Copyright © 2009-2016 Emmanuel Pacaud
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
 * SECTION: arvuvdevice
 * @short_description: Uv camera device
 */

#include <arvdeviceprivate.h>
#include <arvuvdevice.h>
#include <arvuvstream.h>
#include <arvgc.h>
#include <arvdebug.h>
#include <arvuvcp.h>
#include <libusb.h>
#include <string.h>
#include <arvstr.h>
#include <arvzip.h>

static GObjectClass *parent_class = NULL;

struct _ArvUvDevicePrivate {
	char *vendor;
	char *product;
	char *serial_nbr;

	libusb_context *usb;
	libusb_device_handle *usb_device;

	ArvGc *genicam;

	const char *genicam_xml;
	size_t genicam_xml_size;

	guint16 packet_id;

	guint data_size_max;
};

/* ArvUvDevice implemenation */

/* ArvDevice implemenation */

static ArvStream *
arv_uv_device_create_stream (ArvDevice *device, ArvStreamCallback callback, void *user_data)
{
	ArvUvDevice *uv_device = ARV_UV_DEVICE (device);
	ArvStream *stream;

	stream = arv_uv_stream_new (uv_device->priv->usb, uv_device->priv->usb_device, callback, user_data);

	return stream;
}

static gboolean
_read_memory (ArvUvDevice *uv_device, guint32 address, guint32 size, void *buffer, GError **error)
{
	ArvUvcpPacket *packet;
	size_t packet_size;
	size_t answer_size;
	gboolean success = FALSE;

	answer_size = arv_uvcp_packet_get_read_memory_ack_size (size);

	g_return_val_if_fail (answer_size <= 1024, FALSE);

	packet = arv_uvcp_packet_new_read_memory_cmd (address, size, 0, &packet_size);

	do {
		int transferred;
		void *read_packet;
		size_t read_packet_size;

		read_packet_size = arv_uvcp_packet_get_read_memory_ack_size (size);
		read_packet = g_malloc0 (read_packet_size);

		uv_device->priv->packet_id = arv_uvcp_next_packet_id (uv_device->priv->packet_id);
		arv_uvcp_packet_set_packet_id (packet, uv_device->priv->packet_id);

		arv_uvcp_packet_debug (packet, ARV_DEBUG_LEVEL_LOG);

		g_assert (libusb_bulk_transfer (uv_device->priv->usb_device, (0x04 | LIBUSB_ENDPOINT_OUT),
						(guchar *) packet, packet_size, &transferred, 0) >= 0);
		g_assert (libusb_bulk_transfer (uv_device->priv->usb_device, (0x84 | LIBUSB_ENDPOINT_IN),
						(guchar *) read_packet, read_packet_size, &transferred, 0) >= 0);
		success = TRUE;

		memcpy (buffer, arv_uvcp_packet_get_read_memory_ack_data (read_packet), size);

		arv_uvcp_packet_debug (read_packet, ARV_DEBUG_LEVEL_LOG);

		g_free (read_packet);

	} while (!success);

	arv_uvcp_packet_free (packet);

	if (!success) {
		if (error != NULL && *error == NULL)
			*error = g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_STATUS_TIMEOUT,
					      "[ArvDevice::read_memory] Timeout");
	}

	return success;
}

static gboolean
arv_uv_device_read_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer, GError **error)
{
	ArvUvDevice *uv_device = ARV_UV_DEVICE (device);
	int i;
	gint32 block_size;
	guint data_size_max;

	data_size_max = uv_device->priv->data_size_max;

	for (i = 0; i < (size + data_size_max - 1) / data_size_max; i++) {
		block_size = MIN (data_size_max, size - i * data_size_max);
		if (!_read_memory (uv_device,
				   address + i * data_size_max,
				   block_size, ((char *) buffer) + i * data_size_max, error))
			return FALSE;
	}

	return TRUE;
}

static gboolean
_write_memory (ArvUvDevice *uv_device, guint32 address, guint32 size, void *buffer, GError **error)
{
	ArvUvcpPacket *packet;
	size_t packet_size;
	size_t answer_size;
	gboolean success = FALSE;

	answer_size = arv_uvcp_packet_get_write_memory_ack_size ();

	g_return_val_if_fail (answer_size <= 1024, FALSE);

	packet = arv_uvcp_packet_new_write_memory_cmd (address, size, 0, &packet_size);
	memcpy (arv_uvcp_packet_get_write_memory_cmd_data (packet), buffer, size);

	do {
		int transferred;
		void *read_packet;
		size_t read_packet_size;

		read_packet_size = arv_uvcp_packet_get_read_memory_ack_size (size);
		read_packet = g_malloc0 (read_packet_size);

		uv_device->priv->packet_id = arv_uvcp_next_packet_id (uv_device->priv->packet_id);
		arv_uvcp_packet_set_packet_id (packet, uv_device->priv->packet_id);

		arv_uvcp_packet_debug (packet, ARV_DEBUG_LEVEL_LOG);

		g_assert (libusb_bulk_transfer (uv_device->priv->usb_device, (0x04 | LIBUSB_ENDPOINT_OUT),
						(guchar *) packet, packet_size, &transferred, 0) >= 0);
		g_assert (libusb_bulk_transfer (uv_device->priv->usb_device, (0x84 | LIBUSB_ENDPOINT_IN),
						(guchar *) read_packet, read_packet_size, &transferred, 0) >= 0);
		success = TRUE;

		arv_uvcp_packet_debug (read_packet, ARV_DEBUG_LEVEL_LOG);

		g_free (read_packet);

	} while (!success);

	arv_uvcp_packet_free (packet);

	if (!success) {
		if (error != NULL && *error == NULL)
			*error = g_error_new (ARV_DEVICE_ERROR, ARV_DEVICE_STATUS_TIMEOUT,
					      "[ArvDevice::write_memory] Timeout");
	}

	return success;
}

static gboolean
arv_uv_device_write_memory (ArvDevice *device, guint32 address, guint32 size, void *buffer, GError **error)
{
	ArvUvDevice *uv_device = ARV_UV_DEVICE (device);
	int i;
	gint32 block_size;
	guint data_size_max;

	data_size_max = uv_device->priv->data_size_max;

	for (i = 0; i < (size + data_size_max - 1) / data_size_max; i++) {
		block_size = MIN (data_size_max, size - i * data_size_max);
		if (!_write_memory (uv_device,
				   address + i * data_size_max,
				   block_size, ((char *) buffer) + i * data_size_max, error))
			return FALSE;
	}

	return TRUE;
}

static gboolean
arv_uv_device_read_register (ArvDevice *device, guint32 address, guint32 *value, GError **error)
{
	return arv_uv_device_read_memory (device, address, sizeof (guint32), value, error);
}

static gboolean
arv_uv_device_write_register (ArvDevice *device, guint32 address, guint32 value, GError **error)
{
	return arv_uv_device_write_memory (device, address, sizeof (guint32), &value, error);
}

static void 
_bootstrap (ArvUvDevice *uv_device)
{
	ArvDevice *device = ARV_DEVICE (uv_device);
	guint64 offset;
	guint32 response_time;
	guint64 manifest_table_address;
	guint64 device_capability;
	guint32 max_cmd_transfer;
	guint32 max_ack_transfer;
	guint32 u3vcp_capability;
	guint64 sirm_offset;
	guint32 si_info;
	guint32 si_control;
	guint64 si_req_payload_size;
	guint32 si_req_leader_size;
	guint32 si_req_trailer_size;
	guint32 si_max_leader_size;
	guint32 si_payload_size;
	guint32 si_payload_count;
	guint32 si_transfer1_size;
	guint32 si_transfer2_size;
	guint32 si_max_trailer_size;

	guint64 manifest_n_entries;
	ArvUvcpManifestEntry entry;
	GString *string;
	void *data;

	g_message ("Get genicam");

	arv_device_read_memory (device, ARV_ABRM_SBRM_ADDRESS, sizeof (guint64), &offset, NULL);
	arv_device_read_memory (device, ARV_ABRM_MAX_DEVICE_RESPONSE_TIME, sizeof (guint32), &response_time, NULL);
	arv_device_read_memory (device, ARV_ABRM_DEVICE_CAPABILITY, sizeof (guint64), &device_capability, NULL);
	arv_device_read_memory (device, ARV_ABRM_MANIFEST_TABLE_ADDRESS, sizeof (guint64), &manifest_table_address, NULL);

	g_message ("MAX_DEVICE_RESPONSE_TIME = 0x%08x", response_time);
	g_message ("DEVICE_CAPABILITY        = 0x%016lx", device_capability);
	g_message ("SRBM_ADDRESS =             0x%016lx", offset);
	g_message ("MANIFEST_TABLE_ADDRESS =   0x%016lx", manifest_table_address);

	arv_device_read_memory (device, offset + ARV_SBRM_U3VCP_CAPABILITY, sizeof (guint32), &u3vcp_capability, NULL);
	arv_device_read_memory (device, offset + ARV_SBRM_MAX_CMD_TRANSFER, sizeof (guint32), &max_cmd_transfer, NULL);
	arv_device_read_memory (device, offset + ARV_SBRM_MAX_ACK_TRANSFER, sizeof (guint32), &max_ack_transfer, NULL);
	arv_device_read_memory (device, offset + ARV_SBRM_SIRM_ADDRESS, sizeof (guint64), &sirm_offset, NULL);

	g_message ("U3VCP_CAPABILITY =         0x%08x", u3vcp_capability);
	g_message ("MAX_CMD_TRANSFER =         0x%08x", max_cmd_transfer);
	g_message ("MAX_ACK_TRANSFER =         0x%08x", max_ack_transfer);
	g_message ("SIRM_OFFSET =              0x%016lx", sirm_offset);

	arv_device_read_memory (device, sirm_offset + ARV_SI_INFO, sizeof (si_info), &si_info, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SI_CONTROL, sizeof (si_control), &si_control, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SI_REQ_PAYLOAD_SIZE, sizeof (si_req_payload_size), &si_req_payload_size, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SI_REQ_LEADER_SIZE, sizeof (si_req_leader_size), &si_req_leader_size, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SI_REQ_TRAILER_SIZE, sizeof (si_req_trailer_size), &si_req_trailer_size, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SI_MAX_LEADER_SIZE, sizeof (si_max_leader_size), &si_max_leader_size, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SI_PAYLOAD_SIZE, sizeof (si_payload_size), &si_payload_size, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SI_PAYLOAD_COUNT, sizeof (si_payload_count), &si_payload_count, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SI_TRANSFER1_SIZE, sizeof (si_transfer1_size), &si_transfer1_size, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SI_TRANSFER2_SIZE, sizeof (si_transfer2_size), &si_transfer2_size, NULL);
	arv_device_read_memory (device, sirm_offset + ARV_SI_MAX_TRAILER_SIZE, sizeof (si_max_trailer_size), &si_max_trailer_size, NULL);

	g_message ("SI_INFO =                  0x%08x", si_info);
	g_message ("SI_CONTROL =               0x%08x", si_control);
	g_message ("SI_REQ_PAYLOAD_SIZE =      0x%016lx", si_req_payload_size);
	g_message ("SI_REQ_LEADER_SIZE =       0x%08x", si_req_leader_size);
	g_message ("SI_REQ_TRAILER_SIZE =      0x%08x", si_req_trailer_size);
	g_message ("SI_MAX_LEADER_SIZE =       0x%08x", si_max_leader_size);
	g_message ("SI_PAYLOAD_SIZE =          0x%08x", si_payload_size);
	g_message ("SI_PAYLOAD_COUNT =         0x%08x", si_payload_count);
	g_message ("SI_TRANSFER1_SIZE =        0x%08x", si_transfer1_size);
	g_message ("SI_TRANSFER2_SIZE =        0x%08x", si_transfer2_size);
	g_message ("SI_MAX_TRAILER_SIZE =      0x%08x", si_max_trailer_size);

	arv_device_read_memory (device, manifest_table_address, sizeof (guint64), &manifest_n_entries, NULL);
	arv_device_read_memory (device, manifest_table_address + 0x08, sizeof (entry), &entry, NULL);

	g_message ("MANIFEST_N_ENTRIES =       0x%016lx", manifest_n_entries);

	string = g_string_new ("");
	arv_g_string_append_hex_dump (string, &entry, sizeof (entry));
	g_message ("MANIFEST ENTRY\n%s", string->str);
	g_string_free (string, TRUE);

	g_message ("genicam address =          0x%016lx", entry.address);
	g_message ("genicam address =          0x%016lx", entry.size);

	data = g_malloc0 (entry.size);
	arv_device_read_memory (device, entry.address, entry.size, data, NULL);

#if 0
	string = g_string_new ("");
	arv_g_string_append_hex_dump (string, data, entry.size);
	g_message ("GENICAM\n%s", string->str);
	g_string_free (string, TRUE);
#endif

	{
		ArvZip *zip;
		const GSList *zip_files;

		zip = arv_zip_new (data, entry.size);
		zip_files = arv_zip_get_file_list (zip);

		if (zip_files != NULL) {
			const char *zip_filename;

			zip_filename = arv_zip_file_get_name (zip_files->data);
			uv_device->priv->genicam_xml = arv_zip_get_file (zip, zip_filename, &uv_device->priv->genicam_xml_size);

			g_message ("file = %s", zip_filename);

#if 0
			string = g_string_new ("");
			arv_g_string_append_hex_dump (string, uv_device->priv->genicam_xml, uv_device->priv->genicam_xml_size);
			g_message ("GENICAM\n%s", string->str);
			g_string_free (string, TRUE);
#endif

			uv_device->priv->genicam = arv_gc_new (ARV_DEVICE (uv_device), uv_device->priv->genicam_xml,
							       uv_device->priv->genicam_xml_size);
		}

		arv_zip_free (zip);
	}

	g_free (data);
}

static ArvGc *
arv_uv_device_get_genicam (ArvDevice *device)
{
	ArvUvDevice *uv_device = ARV_UV_DEVICE (device);

	return uv_device->priv->genicam;
}

static const char *
arv_uv_device_get_genicam_xml (ArvDevice *device, size_t *size)
{
	ArvUvDevice *uv_device = ARV_UV_DEVICE (device);

	if (size != NULL)
		*size = uv_device->priv->genicam_xml_size;

	return uv_device->priv->genicam_xml;
}

static void
_open_usb_device (ArvUvDevice *uv_device)
{
	libusb_device **devices;
	unsigned i, count;

	count = libusb_get_device_list (uv_device->priv->usb, &devices);
	if (count < 0)
		return;

	for (i = 0; i < count && uv_device->priv->usb_device == NULL; i++) {
		libusb_device_handle *usb_device;
		struct libusb_device_descriptor desc;

		if (libusb_get_device_descriptor (devices[i], &desc) >= 0 &&
		    libusb_open (devices[i], &usb_device) == LIBUSB_SUCCESS) {
			unsigned char *manufacturer;
			unsigned char *product;
			unsigned char *serial_nbr;
			int index;

			manufacturer = g_malloc0 (256);
			product = g_malloc0 (256);
			serial_nbr = g_malloc0 (256);

			index = desc.iManufacturer;
			if (index > 0)
				libusb_get_string_descriptor_ascii (usb_device, index, manufacturer, 256);
			index = desc.iProduct;
			if (index > 0)
				libusb_get_string_descriptor_ascii (usb_device, index, product, 256);
			index = desc.iSerialNumber;
			if (index > 0)
				libusb_get_string_descriptor_ascii (usb_device, index, serial_nbr, 256);

			if (g_strcmp0 ((char * ) manufacturer, uv_device->priv->vendor) == 0 &&
			    g_strcmp0 ((char * ) product, uv_device->priv->product) == 0 &&
			    g_strcmp0 ((char * ) serial_nbr, uv_device->priv->serial_nbr) == 0) {
				uv_device->priv->usb_device = usb_device;
			} else
				libusb_close (usb_device);

			g_free (manufacturer);
			g_free (product);
			g_free (serial_nbr);
		}
	}

	libusb_free_device_list (devices, 1);
}

ArvDevice *
arv_uv_device_new (const char *vendor, const char *product, const char *serial_nbr)
{
	ArvUvDevice *uv_device;

	g_return_val_if_fail (vendor != NULL, NULL);
	g_return_val_if_fail (product != NULL, NULL);
	g_return_val_if_fail (serial_nbr != NULL, NULL);

	arv_debug_device ("[UvDevice::new] Vendor  = %s", vendor);
	arv_debug_device ("[UvDevice::new] Product = %s", product);
	arv_debug_device ("[UvDevice::new] S/N     = %s", serial_nbr);

	uv_device = g_object_new (ARV_TYPE_UV_DEVICE, NULL);

	libusb_init (&uv_device->priv->usb);
	uv_device->priv->vendor = g_strdup (vendor);
	uv_device->priv->product = g_strdup (product);
	uv_device->priv->serial_nbr = g_strdup (serial_nbr);
	uv_device->priv->packet_id = 65300; /* Start near the end of the circular counter */

	_open_usb_device (uv_device);

	g_assert (libusb_claim_interface (uv_device->priv->usb_device, 0) >= 0);

	_bootstrap (uv_device);

	if (!ARV_IS_GC (uv_device->priv->genicam)) {
		g_assert (libusb_release_interface (uv_device->priv->usb_device, 0) >= 0);

		arv_warning_device ("[UvDevice::new] Failed to load genicam data");
		g_object_unref (uv_device);
		return NULL;
	}

	return ARV_DEVICE (uv_device);
}

static void
arv_uv_device_init (ArvUvDevice *uv_device)
{
	uv_device->priv = G_TYPE_INSTANCE_GET_PRIVATE (uv_device, ARV_TYPE_UV_DEVICE, ArvUvDevicePrivate);
	uv_device->priv->data_size_max = 256;
}

static void
arv_uv_device_finalize (GObject *object)
{
	ArvUvDevice *uv_device = ARV_UV_DEVICE (object);

	g_assert (libusb_release_interface (uv_device->priv->usb_device, 0) >= 0);

	g_object_unref (uv_device->priv->genicam);

	g_clear_pointer (&uv_device->priv->vendor, g_free);
	g_clear_pointer (&uv_device->priv->product, g_free);
	g_clear_pointer (&uv_device->priv->serial_nbr, g_free);
	libusb_close (uv_device->priv->usb_device);
	libusb_exit (uv_device->priv->usb);

	parent_class->finalize (object);
}

static void
arv_uv_device_class_init (ArvUvDeviceClass *uv_device_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (uv_device_class);
	ArvDeviceClass *device_class = ARV_DEVICE_CLASS (uv_device_class);

	g_type_class_add_private (uv_device_class, sizeof (ArvUvDevicePrivate));

	parent_class = g_type_class_peek_parent (uv_device_class);

	object_class->finalize = arv_uv_device_finalize;

	device_class->create_stream = arv_uv_device_create_stream;
	device_class->get_genicam_xml = arv_uv_device_get_genicam_xml;
	device_class->get_genicam = arv_uv_device_get_genicam;
	device_class->read_memory = arv_uv_device_read_memory;
	device_class->write_memory = arv_uv_device_write_memory;
	device_class->read_register = arv_uv_device_read_register;
	device_class->write_register = arv_uv_device_write_register;
}

G_DEFINE_TYPE (ArvUvDevice, arv_uv_device, ARV_TYPE_DEVICE)
