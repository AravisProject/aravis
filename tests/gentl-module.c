/* Aravis - Digital camera library
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <arv.h>
#include <arvgentlprivate.h>
#include <glib.h>
#include <gmodule.h>

static GModule *producer;
static PGCInitLib gentl_init;
static PGCCloseLib gentl_close;
static PGCGetInfo gentl_get_info;
static PGCGetPortInfo gentl_get_port_info;
static PGCGetPortURLInfo gentl_get_port_url_info;
static PGCReadPort gentl_read_port;
static PGCReadPortStacked gentl_read_port_stacked;
static PGCWritePortStacked gentl_write_port_stacked;
static PGCRegisterEvent gentl_register_event;
static PGCUnregisterEvent gentl_unregister_event;
static PEventGetData gentl_event_get_data;
static PTLOpen gentl_tl_open;
static PTLClose gentl_tl_close;
static PTLOpenInterface gentl_tl_open_interface;
static PTLUpdateInterfaceList gentl_tl_update_interface_list;
static PIFClose gentl_if_close;
static PIFUpdateDeviceList gentl_if_update_device_list;
static PIFOpenDevice gentl_if_open_device;
static PDevClose gentl_dev_close;
static PDevGetPort gentl_dev_get_port;
static PDevOpenDataStream gentl_dev_open_data_stream;
static PDSClose gentl_ds_close;
static PDSAllocAndAnnounceBuffer gentl_ds_alloc_buffer;
static PDSQueueBuffer gentl_ds_queue_buffer;
static PDSRevokeBuffer gentl_ds_revoke_buffer;
static PDSGetInfo gentl_ds_get_info;
static PDSStartAcquisition gentl_ds_start_acquisition;
static PDSStopAcquisition gentl_ds_stop_acquisition;
static PDSGetBufferInfo gentl_ds_get_buffer_info;
static PDSGetBufferInfoStacked gentl_ds_get_buffer_info_stacked;
static PDSGetBufferPartInfo gentl_ds_get_buffer_part_info;
static PDSGetBufferPartInfoStacked gentl_ds_get_buffer_part_info_stacked;
static PDSGetBufferChunkData gentl_ds_get_buffer_chunk_data;
static PDSGetNumFlows gentl_ds_get_num_flows;
static PDSGetFlowInfo gentl_ds_get_flow_info;
static PDSGetNumBufferSegments gentl_ds_get_num_buffer_segments;
static PDSGetBufferSegmentInfo gentl_ds_get_buffer_segment_info;

static void
load_symbol (const char *name, gpointer *symbol)
{
	g_assert_true (g_module_symbol (producer, name, symbol));
	g_assert_nonnull (*symbol);
}

static char *
get_string_info (TL_INFO_CMD command)
{
	INFO_DATATYPE type = INFO_DATATYPE_UNKNOWN;
	size_t size = 0;
	char *value;

	g_assert_cmpint (gentl_get_info (command, &type, NULL, &size), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (type, ==, INFO_DATATYPE_STRING);
	g_assert_cmpuint (size, >, 1);

	value = g_malloc (size);
	g_assert_cmpint (gentl_get_info (command, &type, value, &size), ==, GC_ERR_SUCCESS);

	return value;
}

static guint32
get_uint32_info (TL_INFO_CMD command)
{
	INFO_DATATYPE type = INFO_DATATYPE_UNKNOWN;
	guint32 value = 0;
	size_t size = sizeof value;

	g_assert_cmpint (gentl_get_info (command, &type, &value, &size), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (type, ==, INFO_DATATYPE_UINT32);
	g_assert_cmpuint (size, ==, sizeof value);

	return value;
}

static void
assert_port_info_string (PORT_HANDLE port, PORT_INFO_CMD command, const char *expected)
{
	INFO_DATATYPE type = INFO_DATATYPE_UNKNOWN;
	size_t size = 0;
	char *value = NULL;

	g_assert_cmpint (gentl_get_port_info (port, command, &type, NULL, &size), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (type, ==, INFO_DATATYPE_STRING);
	value = g_malloc (size);
	g_assert_cmpint (gentl_get_port_info (port, command, &type, value, &size), ==, GC_ERR_SUCCESS);
	g_assert_cmpstr (value, ==, expected);

        g_free (value);
}

static void
assert_port_info_bool (PORT_HANDLE port, PORT_INFO_CMD command, bool8_t expected)
{
	INFO_DATATYPE type = INFO_DATATYPE_UNKNOWN;
	bool8_t value = !expected;
	size_t size = sizeof value;

	g_assert_cmpint (gentl_get_port_info (port, command, &type, &value, &size), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (type, ==, INFO_DATATYPE_BOOL8);
	g_assert_cmpuint (size, ==, sizeof value);
	g_assert_cmpint (value, ==, expected);
}

static void
assert_local_xml_sha1 (PORT_HANDLE port)
{
	INFO_DATATYPE type = INFO_DATATYPE_UNKNOWN;
	GChecksum *checksum;
	guint64 xml_size = 0;
	guint8 *xml = NULL;
	guint8 expected[20];
	guint8 actual[20];
	gsize expected_size = sizeof expected;
	size_t size;

	size = sizeof xml_size;
	g_assert_cmpint (gentl_get_port_url_info (port, 0, URL_INFO_FILE_SIZE, &type, &xml_size, &size),
			 ==, GC_ERR_SUCCESS);
	g_assert_cmpint (type, ==, INFO_DATATYPE_UINT64);
	g_assert_cmpuint (xml_size, >, 0);
	g_assert_cmpuint (xml_size, <=, G_MAXSIZE);

	xml = g_malloc (xml_size);
	size = xml_size;
	g_assert_cmpint (gentl_read_port (port, 0, xml, &size), ==, GC_ERR_SUCCESS);
	g_assert_cmpuint (size, ==, xml_size);

	checksum = g_checksum_new (G_CHECKSUM_SHA1);
	g_checksum_update (checksum, xml, xml_size);
	g_checksum_get_digest (checksum, expected, &expected_size);
	g_checksum_free (checksum);
	g_assert_cmpuint (expected_size, ==, sizeof expected);

        g_free (xml);

	size = 0;
	g_assert_cmpint (gentl_get_port_url_info (port, 0, URL_INFO_FILE_SHA1_HASH, &type, NULL, &size),
			 ==, GC_ERR_SUCCESS);
	g_assert_cmpint (type, ==, INFO_DATATYPE_BUFFER);
	g_assert_cmpuint (size, ==, sizeof actual);

	size = sizeof actual - 1;
	g_assert_cmpint (gentl_get_port_url_info (port, 0, URL_INFO_FILE_SHA1_HASH, &type, actual, &size),
			 ==, GC_ERR_BUFFER_TOO_SMALL);
	g_assert_cmpuint (size, ==, sizeof actual);

	size = sizeof actual;
	g_assert_cmpint (gentl_get_port_url_info (port, 0, URL_INFO_FILE_SHA1_HASH, &type, actual, &size),
			 ==, GC_ERR_SUCCESS);
	g_assert_cmpmem (actual, size, expected, expected_size);
}

static void
test_metadata (void)
{
	char *name = NULL;
	char *path = NULL;
	char *version = NULL;

	g_assert_cmpint (gentl_init (), ==, GC_ERR_SUCCESS);

	name = get_string_info (TL_INFO_NAME);
	path = get_string_info (TL_INFO_PATHNAME);
	version = get_string_info (TL_INFO_VERSION);

	g_assert_true (g_str_has_prefix (name, "ProducerAravis-"));
	g_assert_true (g_str_has_suffix (name, ".cti"));
	g_assert_nonnull (strstr (path, name));
	g_assert_cmpstr (version, !=, "Aravis-Version");
	g_assert_cmpuint (get_uint32_info (TL_INFO_GENTL_VER_MAJOR), ==, 1);
	g_assert_cmpuint (get_uint32_info (TL_INFO_GENTL_VER_MINOR), ==, 6);

	g_assert_cmpint (gentl_close (), ==, GC_ERR_SUCCESS);

        g_free (name);
        g_free (path);
        g_free (version);
}

static void
test_port_stacked (void)
{
	TL_HANDLE tl = NULL;
	char scalar[16] = { 0 };
	char first[8] = { 0 };
	char second[8] = { 0 };
	PORT_REGISTER_STACK_ENTRY entries[] = {
		{ .Address = 0, .Buffer = first, .Size = sizeof first },
		{ .Address = sizeof first, .Buffer = second, .Size = sizeof second },
	};
	PORT_REGISTER_STACK_ENTRY failing_entries[] = {
		{ .Address = 0, .Buffer = first, .Size = 1 },
		{ .Address = G_MAXUINT64, .Buffer = second, .Size = 1 },
	};
	size_t scalar_size = sizeof scalar;
	size_t num_entries = G_N_ELEMENTS (entries);
	bool8_t changed = 1;

	g_assert_cmpint (gentl_init (), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_tl_open (&tl), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_tl_update_interface_list (tl, &changed, 0), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (changed, ==, 0);
	assert_port_info_string (tl, PORT_INFO_MODULE, "TLSystem");
	assert_port_info_bool (tl, PORT_INFO_LITTLE_ENDIAN, G_BYTE_ORDER == G_LITTLE_ENDIAN);
	assert_port_info_bool (tl, PORT_INFO_BIG_ENDIAN, G_BYTE_ORDER == G_BIG_ENDIAN);
	assert_local_xml_sha1 (tl);

	g_assert_cmpint (gentl_read_port (tl, 0, scalar, &scalar_size), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_read_port_stacked (tl, entries, &num_entries), ==, GC_ERR_SUCCESS);
	g_assert_cmpuint (num_entries, ==, G_N_ELEMENTS (entries));
	g_assert_cmpmem (first, sizeof first, scalar, sizeof first);
	g_assert_cmpmem (second, sizeof second, scalar + sizeof first, sizeof second);

	num_entries = G_N_ELEMENTS (failing_entries);
	g_assert_cmpint (gentl_read_port_stacked (tl, failing_entries, &num_entries), ==,
			 GC_ERR_INVALID_ADDRESS);
	g_assert_cmpuint (num_entries, ==, 1);

	num_entries = 1;
	g_assert_cmpint (gentl_write_port_stacked (tl, entries, &num_entries), ==,
			 GC_ERR_ACCESS_DENIED);
	g_assert_cmpuint (num_entries, ==, 0);

	num_entries = 0;
	g_assert_cmpint (gentl_read_port_stacked (tl, NULL, &num_entries), ==, GC_ERR_SUCCESS);

	g_assert_cmpint (gentl_tl_close (tl), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_close (), ==, GC_ERR_SUCCESS);
}

static void
test_buffer_info_stacked (void)
{
	TL_HANDLE tl = NULL;
	IF_HANDLE iface = NULL;
	DEV_HANDLE device = NULL;
	PORT_HANDLE remote_device = NULL;
	DS_HANDLE data_stream = NULL;
	BUFFER_HANDLE buffer = NULL;
	size_t scalar_size_value = 0;
	size_t scalar_size = sizeof scalar_size_value;
	size_t stacked_size_value = 0;
	void *stacked_base = NULL;
	size_t part_data_size = G_MAXSIZE;
	size_t part_width = G_MAXSIZE;
	size_t segment_size = 0;
	size_t segment_size_filled = G_MAXSIZE;
	size_t segment_data_size = G_MAXSIZE;
	size_t info_size;
	size_t num_chunks = 1;
	uint32_t num_flows = 1;
	uint32_t num_segments = 0;
	void *segment_base = NULL;
	bool8_t segment_incomplete = 1;
	bool8_t changed = 0;
	DS_BUFFER_INFO_STACKED info_entries[] = {
		{
			.iInfoCmd = BUFFER_INFO_SIZE,
			.pBuffer = &stacked_size_value,
			.iSize = sizeof stacked_size_value,
		},
		{
			.iInfoCmd = BUFFER_INFO_CUSTOM_ID,
			.iSize = 0,
		},
		{
			.iInfoCmd = BUFFER_INFO_BASE,
			.pBuffer = &stacked_base,
			.iSize = sizeof stacked_base,
		},
	};
	DS_BUFFER_PART_INFO_STACKED part_entries[] = {
		{
			.iPartIndex = 0,
			.iInfoCmd = BUFFER_PART_INFO_DATA_SIZE,
			.pBuffer = &part_data_size,
			.iSize = sizeof part_data_size,
		},
		{
			.iPartIndex = 1,
			.iInfoCmd = BUFFER_PART_INFO_WIDTH,
			.pBuffer = &part_width,
			.iSize = sizeof part_width,
		},
		{
			.iPartIndex = 0,
			.iInfoCmd = BUFFER_PART_INFO_WIDTH,
			.pBuffer = &part_width,
			.iSize = sizeof part_width,
		},
	};

	g_assert_cmpint (gentl_init (), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_tl_open (&tl), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_tl_open_interface (tl, "Fake", &iface), ==, GC_ERR_SUCCESS);
	assert_port_info_string (iface, PORT_INFO_MODULE, "TLInterface");
	assert_local_xml_sha1 (iface);
	g_assert_cmpint (gentl_if_update_device_list (iface, &changed, 0), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (changed, ==, 0);
	changed = 1;
	g_assert_cmpint (gentl_if_update_device_list (iface, &changed, 0), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (changed, ==, 0);
	g_assert_cmpint (gentl_if_open_device (iface, "Fake_1", DEVICE_ACCESS_CONTROL, &device), ==,
			 GC_ERR_SUCCESS);
	assert_port_info_string (device, PORT_INFO_MODULE, "TLDevice");
	assert_port_info_bool (device, PORT_INFO_LITTLE_ENDIAN, 0);
	assert_port_info_bool (device, PORT_INFO_BIG_ENDIAN, 1);
	g_assert_cmpint (gentl_dev_get_port (device, &remote_device), ==, GC_ERR_SUCCESS);
	assert_port_info_string (remote_device, PORT_INFO_MODULE, "Device");
	assert_port_info_bool (remote_device, PORT_INFO_LITTLE_ENDIAN, 0);
	assert_port_info_bool (remote_device, PORT_INFO_BIG_ENDIAN, 1);
	g_assert_cmpint (gentl_dev_open_data_stream (device, "0", &data_stream), ==, GC_ERR_SUCCESS);
	assert_port_info_string (data_stream, PORT_INFO_MODULE, "TLDataStream");
	assert_local_xml_sha1 (data_stream);
	g_assert_cmpint (gentl_ds_alloc_buffer (data_stream, 4096, NULL, &buffer), ==, GC_ERR_SUCCESS);

	g_assert_cmpint (gentl_ds_get_buffer_info (data_stream, buffer, BUFFER_INFO_SIZE, NULL,
						   &scalar_size_value, &scalar_size),
			 ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_ds_get_buffer_info_stacked (data_stream, buffer, info_entries,
							   G_N_ELEMENTS (info_entries)),
			 ==, GC_ERR_SUCCESS);
	g_assert_cmpint (info_entries[0].iResult, ==, GC_ERR_SUCCESS);
	g_assert_cmpuint (stacked_size_value, ==, scalar_size_value);
	g_assert_cmpint (info_entries[0].iType, ==, INFO_DATATYPE_SIZET);
	g_assert_cmpint (info_entries[1].iResult, ==, GC_ERR_NOT_IMPLEMENTED);
	g_assert_cmpint (info_entries[2].iResult, ==, GC_ERR_SUCCESS);
	g_assert_nonnull (stacked_base);

	g_assert_cmpint (gentl_ds_get_buffer_part_info_stacked (data_stream, buffer, part_entries,
								G_N_ELEMENTS (part_entries)),
			 ==, GC_ERR_SUCCESS);
	g_assert_cmpint (part_entries[0].iResult, ==, GC_ERR_SUCCESS);
	g_assert_cmpuint (part_data_size, ==, 0);
	g_assert_cmpint (part_entries[1].iResult, ==, GC_ERR_INVALID_INDEX);
	g_assert_cmpint (part_entries[2].iResult, ==, GC_ERR_SUCCESS);
	g_assert_cmpuint (part_width, ==, 0);

	g_assert_cmpint (gentl_ds_get_buffer_chunk_data (data_stream, buffer, NULL, &num_chunks), ==,
			 GC_ERR_SUCCESS);
	g_assert_cmpuint (num_chunks, ==, 0);

	g_assert_cmpint (gentl_ds_get_num_flows (data_stream, &num_flows), ==, GC_ERR_NO_DATA);
	g_assert_cmpuint (num_flows, ==, 0);
	info_size = 0;
	g_assert_cmpint (gentl_ds_get_flow_info (data_stream, 0, FLOW_INFO_SIZE, NULL, NULL, &info_size),
			 ==, GC_ERR_NO_DATA);

	g_assert_cmpint (gentl_ds_get_num_buffer_segments (data_stream, buffer, &num_segments), ==,
			 GC_ERR_SUCCESS);
	g_assert_cmpuint (num_segments, ==, 1);

	info_size = sizeof segment_base;
	g_assert_cmpint (gentl_ds_get_buffer_segment_info (data_stream, buffer, 0, SEGMENT_INFO_BASE, NULL,
							   &segment_base, &info_size),
			 ==, GC_ERR_SUCCESS);
	g_assert_true (segment_base == stacked_base);

	info_size = sizeof segment_size;
	g_assert_cmpint (gentl_ds_get_buffer_segment_info (data_stream, buffer, 0, SEGMENT_INFO_SIZE, NULL,
							   &segment_size, &info_size),
			 ==, GC_ERR_SUCCESS);
	g_assert_cmpuint (segment_size, ==, 4096);

	info_size = sizeof segment_incomplete;
	g_assert_cmpint (gentl_ds_get_buffer_segment_info (data_stream, buffer, 0,
							   SEGMENT_INFO_IS_INCOMPLETE, NULL,
							   &segment_incomplete, &info_size),
			 ==, GC_ERR_SUCCESS);
	g_assert_cmpint (segment_incomplete, ==, 0);

	info_size = sizeof segment_size_filled;
	g_assert_cmpint (gentl_ds_get_buffer_segment_info (data_stream, buffer, 0,
							   SEGMENT_INFO_SIZE_FILLED, NULL,
							   &segment_size_filled, &info_size),
			 ==, GC_ERR_SUCCESS);
	g_assert_cmpuint (segment_size_filled, ==, 0);

	info_size = sizeof segment_data_size;
	g_assert_cmpint (gentl_ds_get_buffer_segment_info (data_stream, buffer, 0,
							   SEGMENT_INFO_DATA_SIZE, NULL,
							   &segment_data_size, &info_size),
			 ==, GC_ERR_SUCCESS);
	g_assert_cmpuint (segment_data_size, ==, 0);

	info_size = sizeof segment_size;
	g_assert_cmpint (gentl_ds_get_buffer_segment_info (data_stream, buffer, 1, SEGMENT_INFO_SIZE, NULL,
							   &segment_size, &info_size),
			 ==, GC_ERR_INVALID_INDEX);

	g_assert_cmpint (gentl_ds_revoke_buffer (data_stream, buffer, NULL, NULL), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_ds_close (data_stream), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_dev_close (device), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_if_close (iface), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_tl_close (tl), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_close (), ==, GC_ERR_SUCCESS);
}

static void
test_finite_acquisition (void)
{
	TL_HANDLE tl = NULL;
	IF_HANDLE iface = NULL;
	DEV_HANDLE device = NULL;
	DS_HANDLE data_stream = NULL;
	EVENT_HANDLE event = NULL;
	BUFFER_HANDLE buffers[8] = { NULL };
	EVENT_NEW_BUFFER_DATA event_data;
	INFO_DATATYPE type = INFO_DATATYPE_UNKNOWN;
	size_t payload_size = 0;
	size_t info_size = sizeof payload_size;
	bool8_t changed = 0;
	bool8_t is_grabbing = 1;
	unsigned int i;

	g_assert_cmpint (gentl_init (), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_tl_open (&tl), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_tl_open_interface (tl, "Fake", &iface), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_if_update_device_list (iface, &changed, 0), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_if_open_device (iface, "Fake_1", DEVICE_ACCESS_CONTROL, &device), ==,
			 GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_dev_open_data_stream (device, "0", &data_stream), ==, GC_ERR_SUCCESS);

	g_assert_cmpint (gentl_ds_get_info (data_stream, STREAM_INFO_PAYLOAD_SIZE, &type, &payload_size,
					    &info_size),
			 ==, GC_ERR_SUCCESS);
	g_assert_cmpuint (payload_size, >, 0);

	for (i = 0; i < G_N_ELEMENTS (buffers); i++) {
		g_assert_cmpint (gentl_ds_alloc_buffer (data_stream, payload_size, NULL, &buffers[i]), ==,
				 GC_ERR_SUCCESS);
		g_assert_cmpint (gentl_ds_queue_buffer (data_stream, buffers[i]), ==, GC_ERR_SUCCESS);
	}

	g_assert_cmpint (gentl_register_event (data_stream, EVENT_NEW_BUFFER, &event), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_ds_start_acquisition (data_stream, ACQ_START_FLAGS_DEFAULT, 0), ==,
			 GC_ERR_INVALID_PARAMETER);
	g_assert_cmpint (gentl_ds_start_acquisition (data_stream, ACQ_START_FLAGS_DEFAULT, 5), ==,
			 GC_ERR_SUCCESS);

	for (i = 0; i < 5; i++) {
		size_t event_size = sizeof event_data;

		g_assert_cmpint (gentl_event_get_data (event, &event_data, &event_size, 2000), ==,
				 GC_ERR_SUCCESS);
		g_assert_nonnull (event_data.BufferHandle);
		g_assert_cmpuint (event_size, ==, sizeof event_data);
	}

	info_size = sizeof event_data;
	g_assert_cmpint (gentl_event_get_data (event, &event_data, &info_size, 100), ==, GC_ERR_ABORT);

	info_size = sizeof is_grabbing;
	g_assert_cmpint (gentl_ds_get_info (data_stream, STREAM_INFO_IS_GRABBING, &type, &is_grabbing,
					    &info_size),
			 ==, GC_ERR_SUCCESS);
	g_assert_cmpint (is_grabbing, ==, 0);

	g_assert_cmpint (gentl_ds_stop_acquisition (data_stream, ACQ_STOP_FLAGS_DEFAULT), ==,
			 GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_unregister_event (data_stream, EVENT_NEW_BUFFER), ==, GC_ERR_SUCCESS);

	for (i = 0; i < G_N_ELEMENTS (buffers); i++)
		g_assert_cmpint (gentl_ds_revoke_buffer (data_stream, buffers[i], NULL, NULL), ==,
				 GC_ERR_SUCCESS);

	g_assert_cmpint (gentl_ds_close (data_stream), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_dev_close (device), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_if_close (iface), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_tl_close (tl), ==, GC_ERR_SUCCESS);
	g_assert_cmpint (gentl_close (), ==, GC_ERR_SUCCESS);
}

int
main (int argc, char **argv)
{
	const char *fake_genicam_path;
	const char *producer_path;
	int result;

	g_test_init (&argc, &argv, NULL);

	fake_genicam_path = g_getenv ("FAKE_GENICAM_PATH");
	g_assert_nonnull (fake_genicam_path);
	arv_set_fake_camera_genicam_filename (fake_genicam_path);
	arv_enable_interface ("Fake");

	producer_path = g_getenv ("ARV_PRODUCER_PATH");
	g_assert_nonnull (producer_path);

	producer = g_module_open (producer_path, G_MODULE_BIND_LOCAL | G_MODULE_BIND_LAZY);
	g_assert_nonnull (producer);

	load_symbol ("GCInitLib", (gpointer *) &gentl_init);
	load_symbol ("GCCloseLib", (gpointer *) &gentl_close);
	load_symbol ("GCGetInfo", (gpointer *) &gentl_get_info);
	load_symbol ("GCGetPortInfo", (gpointer *) &gentl_get_port_info);
	load_symbol ("GCGetPortURLInfo", (gpointer *) &gentl_get_port_url_info);
	load_symbol ("GCReadPort", (gpointer *) &gentl_read_port);
	load_symbol ("GCReadPortStacked", (gpointer *) &gentl_read_port_stacked);
	load_symbol ("GCWritePortStacked", (gpointer *) &gentl_write_port_stacked);
	load_symbol ("GCRegisterEvent", (gpointer *) &gentl_register_event);
	load_symbol ("GCUnregisterEvent", (gpointer *) &gentl_unregister_event);
	load_symbol ("EventGetData", (gpointer *) &gentl_event_get_data);
	load_symbol ("TLOpen", (gpointer *) &gentl_tl_open);
	load_symbol ("TLClose", (gpointer *) &gentl_tl_close);
	load_symbol ("TLOpenInterface", (gpointer *) &gentl_tl_open_interface);
	load_symbol ("TLUpdateInterfaceList", (gpointer *) &gentl_tl_update_interface_list);
	load_symbol ("IFClose", (gpointer *) &gentl_if_close);
	load_symbol ("IFUpdateDeviceList", (gpointer *) &gentl_if_update_device_list);
	load_symbol ("IFOpenDevice", (gpointer *) &gentl_if_open_device);
	load_symbol ("DevClose", (gpointer *) &gentl_dev_close);
	load_symbol ("DevGetPort", (gpointer *) &gentl_dev_get_port);
	load_symbol ("DevOpenDataStream", (gpointer *) &gentl_dev_open_data_stream);
	load_symbol ("DSClose", (gpointer *) &gentl_ds_close);
	load_symbol ("DSAllocAndAnnounceBuffer", (gpointer *) &gentl_ds_alloc_buffer);
	load_symbol ("DSQueueBuffer", (gpointer *) &gentl_ds_queue_buffer);
	load_symbol ("DSRevokeBuffer", (gpointer *) &gentl_ds_revoke_buffer);
	load_symbol ("DSGetInfo", (gpointer *) &gentl_ds_get_info);
	load_symbol ("DSStartAcquisition", (gpointer *) &gentl_ds_start_acquisition);
	load_symbol ("DSStopAcquisition", (gpointer *) &gentl_ds_stop_acquisition);
	load_symbol ("DSGetBufferInfo", (gpointer *) &gentl_ds_get_buffer_info);
	load_symbol ("DSGetBufferInfoStacked", (gpointer *) &gentl_ds_get_buffer_info_stacked);
	load_symbol ("DSGetBufferPartInfo", (gpointer *) &gentl_ds_get_buffer_part_info);
	load_symbol ("DSGetBufferPartInfoStacked", (gpointer *) &gentl_ds_get_buffer_part_info_stacked);
	load_symbol ("DSGetBufferChunkData", (gpointer *) &gentl_ds_get_buffer_chunk_data);
	load_symbol ("DSGetNumFlows", (gpointer *) &gentl_ds_get_num_flows);
	load_symbol ("DSGetFlowInfo", (gpointer *) &gentl_ds_get_flow_info);
	load_symbol ("DSGetNumBufferSegments", (gpointer *) &gentl_ds_get_num_buffer_segments);
	load_symbol ("DSGetBufferSegmentInfo", (gpointer *) &gentl_ds_get_buffer_segment_info);

	g_test_add_func ("/gentl/module/metadata", test_metadata);
	g_test_add_func ("/gentl/module/port-stacked", test_port_stacked);
	g_test_add_func ("/gentl/module/buffer-info-stacked", test_buffer_info_stacked);
	g_test_add_func ("/gentl/module/finite-acquisition", test_finite_acquisition);
	result = g_test_run ();

	g_assert_true (g_module_close (producer));

	return result;
}
