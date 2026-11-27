/* SPDX-License-Identifier:Unlicense */

#include <glib.h>
#include <arv.h>
#include <arvstr.h>
#include <string.h>
#include "../src/arvgvcpprivate.h"
#include "../src/arvmiscprivate.h"
#include "../src/arveventprivate.h"

#if !ARAVIS_CHECK_VERSION (ARAVIS_MAJOR_VERSION, ARAVIS_MINOR_VERSION, ARAVIS_MICRO_VERSION)
#error
#endif

static void
unaligned_from_le_ptr_test (void)
{
	char test[]= {'\x11', '\x22', '\x33', '\x44','\x55', '\x66', '\x77', '\x88', '\x99', '\xaa', '\xbb', '\xcc'};
	guint32 v_uint32;
	guint16 v_uint16;

	v_uint32 = ARV_GUINT32_FROM_LE_PTR (&test[0], 0);
	g_assert_cmpuint (v_uint32, ==, 0x44332211);

	v_uint32 = ARV_GUINT32_FROM_LE_PTR (&test[1], 0);
	g_assert_cmpuint (v_uint32, ==, 0x55443322);

	v_uint32 = ARV_GUINT32_FROM_LE_PTR (&test[2], 0);
	g_assert_cmpuint (v_uint32, ==, 0x66554433);

	v_uint32 = ARV_GUINT32_FROM_LE_PTR (&test[3], 0);
	g_assert_cmpuint (v_uint32, ==, 0x77665544);

	v_uint16 = ARV_GUINT16_FROM_LE_PTR (&test[0], 0);
	g_assert_cmpuint (v_uint16, ==, 0x2211);

	v_uint16 = ARV_GUINT16_FROM_LE_PTR (&test[1], 0);
	g_assert_cmpuint (v_uint16, ==, 0x3322);

	v_uint16 = ARV_GUINT16_FROM_LE_PTR (&test[2], 0);
	g_assert_cmpuint (v_uint16, ==, 0x4433);

	v_uint16 = ARV_GUINT16_FROM_LE_PTR (&test[3], 0);
	g_assert_cmpuint (v_uint16, ==, 0x5544);
}

#define ILLEGAL_CHARACTERS 	"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f" \
				"\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f" \
				" _-"
#define REPLACEMENT_CHARACTER	'-'

struct {
	char *before;
	char *after;
} strip_strings[] = {
	{"\n\tHello\r\nworld!\n\t", 						"Hello-world!"},
	{"\n\tHello",								"Hello"},
	{"Hello\r\t",								"Hello"},
	{"Hello\rworld!",							"Hello-world!"},
	{"Hello\r- -_\rworld!",							"Hello-world!"},
	{"",									""},
	{"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",	""},
	{"\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f",	""},
	{"\r",									""},
	{"\n\n",								""},
	{"Hétéroclite",								"Hétéroclite"}
};

static void
arv_str_strip_test (void)
{
	unsigned i;
	char *string;

	for (i = 0; i < G_N_ELEMENTS (strip_strings); i++) {
		string = g_strdup (strip_strings[i].before);
		arv_str_strip (string, ILLEGAL_CHARACTERS, REPLACEMENT_CHARACTER);

		g_assert_cmpstr (string, ==, strip_strings[i].after);

		g_free (string);
	}

	string = g_strdup ("Hello\r\n world");
	arv_str_strip (string, ILLEGAL_CHARACTERS, '\0');
	g_assert_cmpstr (string, ==, "Helloworld");
	g_free (string);

	g_assert (arv_str_strip (NULL, ILLEGAL_CHARACTERS, REPLACEMENT_CHARACTER) == NULL);
	g_assert (arv_str_strip (NULL, NULL, REPLACEMENT_CHARACTER) == NULL);
}

struct {
	char *uri;
	gboolean is_valid;
} uris[] = {
	{"http://www.gnome/org",			TRUE},
	{"file:///file.txtx",				TRUE},
	{"",						FALSE}
};

static void
arv_str_uri_test (void)
{
	char *uri;
	unsigned i;
	gboolean success;

	for (i = 0; i < G_N_ELEMENTS (uris); i++) {
		success = arv_str_is_uri (uris[i].uri);

		g_assert (success == uris[i].is_valid);
	}

	uri = arv_str_to_uri ("http://www.gnome.org/test");
	g_assert_cmpstr (uri, ==, "http://www.gnome.org/test");
	g_free (uri);

	uri = arv_str_to_uri ("/test.txt");
	g_assert_cmpstr (uri, ==, "file:///test.txt");
	g_free (uri);

	uri = arv_str_to_uri ("test.txt");
	g_assert (g_str_has_suffix (uri, "test.txt"));
	g_assert (g_str_has_prefix (uri, "file:///"));
	g_free (uri);
}

static void
arv_str_parse_double_test (void)
{
	gboolean success;
	char *test1 = "-10.0";
	char *test2 = "+10.0";
	char *test3 = "11.0a";
	double value;

	success = arv_str_parse_double (&test1, &value);
	g_assert (success);
	g_assert_cmpfloat (value, ==, -10.0);
	g_assert_cmpint (test1[0], ==, '\0');

	success = arv_str_parse_double (&test2, &value);
	g_assert (success);
	g_assert_cmpfloat (value, ==, 10.0);
	g_assert_cmpint (test2[0], ==, '\0');

	success = arv_str_parse_double (&test3, &value);
	g_assert (success);
	g_assert_cmpfloat (value, ==, 11.0);
	g_assert_cmpint (test3[0], ==, 'a');
}

static void
arv_str_parse_double_list_test (void)
{
	char *test1 = " -10.0 +20.0   ";
	double value[2];
	unsigned n_values;

	n_values = arv_str_parse_double_list (&test1, 2, value);
	g_assert_cmpint (n_values, ==, 2);
	g_assert_cmpfloat (value[0], ==, -10.0);
	g_assert_cmpfloat (value[1], ==, 20.0);
	g_assert_cmpint (test1[0], ==, '\0');
}

static void
arv_vendor_alias_lookup_test (void)
{
	const char *vendor_a = "The Imaging Source Europe GmbH";
	const char *vendor_b = "Unknown Vendor";
	const char *alias;

	alias = arv_vendor_alias_lookup (NULL);
	g_assert (alias == NULL);

	alias = arv_vendor_alias_lookup (vendor_a);
	g_assert_cmpstr (alias, == ,"TIS");

	alias = arv_vendor_alias_lookup (vendor_b);
	g_assert (alias == vendor_b);
}

struct {
        guint64 pixel_format;
} caps_data[] = {
        { ARV_PIXEL_FORMAT_MONO_8 },
        { ARV_PIXEL_FORMAT_MONO_10 },
        { ARV_PIXEL_FORMAT_MONO_12 },
        { ARV_PIXEL_FORMAT_MONO_12_PACKED },
        { ARV_PIXEL_FORMAT_MONO_14 },
        { ARV_PIXEL_FORMAT_MONO_16 },
        { ARV_PIXEL_FORMAT_BAYER_GB_8 },
        { ARV_PIXEL_FORMAT_BAYER_RG_8 },
        { ARV_PIXEL_FORMAT_BAYER_GR_8 },
        { ARV_PIXEL_FORMAT_BAYER_BG_8 },
        { ARV_PIXEL_FORMAT_YUV_422_PACKED },
        { ARV_PIXEL_FORMAT_YUV_422_YUYV_PACKED },
        { ARV_PIXEL_FORMAT_CUSTOM_YUV_422_YUYV_PACKED },
};

static void
caps_string_test (void)
{
        unsigned int i;

        for (i = 0; i < G_N_ELEMENTS (caps_data); i++) {
                const char *caps_string;

                caps_string = arv_pixel_format_to_gst_caps_string (caps_data[i].pixel_format);
                g_assert (caps_string != NULL);
        }

        for (i = 0; i < G_N_ELEMENTS (caps_data); i++) {
                const char *caps_string;

                caps_string = arv_pixel_format_to_gst_caps_string_full (caps_data[i].pixel_format, "Vendor", "Model");
                g_assert (caps_string != NULL);
        }
}

struct {
	const char *glob_pattern;
	const char *pattern;
} glob_patterns[] = {
	{	"*Foo*",				"^.*Foo.*$"},
	{	"Bar",					"^Bar$"},
	{	"http://www.gnome.org/index.html",	"^http://www\\.gnome\\.org/index\\.html$"},
	{	" Foo*  |  Bar*  ",			"^Foo.*$|^Bar.*$"}
};

static void
glob_test (void)
{
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (glob_patterns); i++) {
		GRegex *regex;
		const char *pattern;

		regex = arv_regex_new_from_glob_pattern (glob_patterns[i].glob_pattern, TRUE);
		g_assert (regex != NULL);
		pattern = g_regex_get_pattern (regex);
		g_assert_cmpstr (pattern, ==, glob_patterns[i].pattern);
		g_regex_unref (regex);
	}
}

struct {
	const char *glob;
	const char *check;
        const char *check_caseless;
} match_data[] = {
	{	"Foo*",		"Foobar",       "foobar"},
	{	"Foo*",		"Foo",          "foo"},
	{	"*Bar",		"fooBar",       "foobar"},
	{	"*Bar",		"Bar",          "bar"},
	{	"FooBar",	"FooBar",       "foobar"},
	{	"???{Foo}*r",	"bar{Foo}bar",  "bar{foo}bar"}
};

static void
match_test (void)
{
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (match_data); i++) {
		GRegex *regex;

		regex = arv_regex_new_from_glob_pattern (match_data[i].glob,FALSE);
		g_assert (regex != NULL);
		g_assert (g_regex_match (regex, match_data[i].check, 0, NULL));
		g_regex_unref (regex);

		regex = arv_regex_new_from_glob_pattern (match_data[i].glob, TRUE);
		g_assert (regex != NULL);
		g_assert (g_regex_match (regex, match_data[i].check_caseless, 0, NULL));
		g_regex_unref (regex);
	}
}

static void
gvcp_force_ip_packet_test (void)
{
	static const guint8 mac[] = { 0x00, 0x21, 0x49, 0x02, 0xa4, 0xd9 };
	static const guint8 ip[] = { 192, 168, 0, 11 };
	static const guint8 mask[] = { 255, 255, 255, 0 };
	static const guint8 gateway[] = { 0, 0, 0, 0 };
	static const guint8 expected[] = {
		0x42, 0x01, 0x00, 0x04, 0x00, 0x38, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x21, 0x49, 0x02, 0xa4, 0xd9,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 192, 168, 0, 11,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 255, 255, 255, 0,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	ArvGvcpPacket *packet;
	size_t packet_size;

	packet = arv_gvcp_packet_new_force_ip_cmd (mac, ip, mask, gateway, 1, &packet_size);

	g_assert_nonnull (packet);
	g_assert_cmpuint (packet_size, ==, sizeof expected);
	g_assert_cmpmem (packet, packet_size, expected, sizeof expected);

	g_free (packet);
}

static void
gvcp_event_packet_test (void)
{
	const guint8 payload[] = {0xde, 0xad, 0xbe, 0xef};
	ArvGvcpPacket *packet;
	ArvGvcpEventInfos infos;
	size_t packet_size;
	size_t offset;
	int i;

	for (i = 0; i < 2; i++) {
		gboolean extended_ids = i == 1;
		size_t header_size = extended_ids ?
			ARV_GVCP_EVENT_EXTENDED_HEADER_SIZE :
			ARV_GVCP_EVENT_HEADER_SIZE;

		packet = arv_gvcp_packet_new_eventdata_cmd (0x9001, 0, 0x123456789a, 0x1122334455667788,
							    payload, sizeof (payload), extended_ids, 42, &packet_size);
		g_assert_nonnull (packet);
		g_assert_cmpuint (packet_size, ==, sizeof (ArvGvcpHeader) + header_size + sizeof (payload));
		g_assert_cmpuint (arv_gvcp_packet_get_command (packet, packet_size), ==,
				  ARV_GVCP_COMMAND_EVENTDATA_CMD);

		offset = 0;
		g_assert_true (arv_gvcp_packet_get_next_event (packet, packet_size, &offset, &infos));
		g_assert_cmpuint (infos.event_id, ==, 0x9001);
		g_assert_cmpuint (infos.stream_channel, ==, 0);
		g_assert_cmpuint (infos.block_id, ==, extended_ids ? 0x123456789a : 0x789a);
		g_assert_cmpuint (infos.timestamp, ==, 0x1122334455667788);
		g_assert_cmpuint (infos.record_size, ==, header_size + sizeof (payload));
		g_assert_cmpmem ((const guint8 *) infos.record + header_size, sizeof (payload),
				 payload, sizeof (payload));
		g_assert_false (arv_gvcp_packet_get_next_event (packet, packet_size, &offset, &infos));

		/* Truncated packet */
		offset = 0;
		g_assert_false (arv_gvcp_packet_get_next_event (packet, sizeof (ArvGvcpHeader) + 4,
								&offset, &infos));

		g_free (packet);
	}

	/* EVENT_CMD with two records, using the reserved size field of devices not using extended ids */
	{
		/* guint64 array, for the alignment of the event record fields */
		guint64 buffer[(sizeof (ArvGvcpHeader) + 2 * ARV_GVCP_EVENT_HEADER_SIZE) / sizeof (guint64)] = {0};
		ArvGvcpPacket *event_packet = (ArvGvcpPacket *) buffer;

		event_packet->header.packet_type = ARV_GVCP_PACKET_TYPE_CMD;
		event_packet->header.packet_flags = ARV_GVCP_CMD_PACKET_FLAGS_ACK_REQUIRED;
		event_packet->header.command = g_htons (ARV_GVCP_COMMAND_EVENT_CMD);
		event_packet->header.size = g_htons (2 * ARV_GVCP_EVENT_HEADER_SIZE);
		event_packet->header.id = g_htons (7);
		event_packet->data[3] = 0x01;
		event_packet->data[7] = 0x05;
		event_packet->data[ARV_GVCP_EVENT_HEADER_SIZE + 3] = 0x02;
		event_packet->data[ARV_GVCP_EVENT_HEADER_SIZE + 15] = 0x10;

		offset = 0;
		g_assert_true (arv_gvcp_packet_get_next_event (event_packet, sizeof (buffer), &offset, &infos));
		g_assert_cmpuint (infos.event_id, ==, 0x0001);
		g_assert_cmpuint (infos.block_id, ==, 5);
		g_assert_true (arv_gvcp_packet_get_next_event (event_packet, sizeof (buffer), &offset, &infos));
		g_assert_cmpuint (infos.event_id, ==, 0x0002);
		g_assert_cmpuint (infos.timestamp, ==, 0x10);
		g_assert_false (arv_gvcp_packet_get_next_event (event_packet, sizeof (buffer), &offset, &infos));

		/* Record size not multiple of 8 */
		event_packet->data[1] = 20;
		offset = 0;
		g_assert_false (arv_gvcp_packet_get_next_event (event_packet, sizeof (buffer), &offset, &infos));
	}

	packet = arv_gvcp_packet_new_event_ack (ARV_GVCP_COMMAND_EVENTDATA_CMD, 42, &packet_size);
	g_assert_cmpuint (packet_size, ==, sizeof (ArvGvcpHeader));
	g_assert_cmpuint (arv_gvcp_packet_get_packet_type (packet, packet_size), ==, ARV_GVCP_PACKET_TYPE_ACK);
	g_assert_cmpuint (arv_gvcp_packet_get_command (packet, packet_size), ==, ARV_GVCP_COMMAND_EVENTDATA_ACK);
	g_assert_cmpuint (arv_gvcp_packet_get_packet_id (packet, packet_size), ==, 42);
	g_free (packet);
}

static void
event_test (void)
{
	const guint8 payload[] = {0x01, 0x02, 0x03, 0x04, 0x05};
	ArvEvent *event;
	GBytes *bytes;
	const guint8 *data;
	size_t size;

	event = arv_event_new (0x9001, 1234, 42, 0, payload, sizeof (payload));
	g_assert_true (ARV_IS_EVENT (event));

	g_assert_cmpuint (arv_event_get_id (event), ==, 0x9001);
	g_assert_cmpuint (arv_event_get_timestamp (event), ==, 1234);
	g_assert_cmpuint (arv_event_get_block_id (event), ==, 42);
	g_assert_cmpuint (arv_event_get_stream_channel (event), ==, 0);

	data = arv_event_peek_data (event, &size);
	g_assert_cmpmem (data, size, payload, sizeof (payload));

	bytes = arv_event_get_data (event);
	g_assert_cmpmem (g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes), payload, sizeof (payload));

	g_object_unref (event);

	/* Data stay valid after the event destruction */
	g_assert_cmpuint (g_bytes_get_size (bytes), ==, sizeof (payload));
	g_bytes_unref (bytes);

	event = arv_event_new (0x9002, 0, 0, ARV_EVENT_STREAM_CHANNEL_NONE, NULL, 0);
	data = arv_event_peek_data (event, &size);
	g_assert_cmpuint (size, ==, 0);
	g_object_unref (event);
}

int
main (int argc, char *argv[])
{
	int result;

	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/buffer/unaligned-from-le", unaligned_from_le_ptr_test);
	g_test_add_func ("/str/arv-str-strip", arv_str_strip_test);
	g_test_add_func ("/str/arv-str-uri", arv_str_uri_test);
	g_test_add_func ("/str/arv-str-parse-double", arv_str_parse_double_test);
	g_test_add_func ("/str/arv-str-parse-double-list", arv_str_parse_double_list_test);
	g_test_add_func ("/misc/arv-vendor-alias-lookup", arv_vendor_alias_lookup_test);
	g_test_add_func ("/gstreamer/caps-string", caps_string_test);
	g_test_add_func ("/misc/globs", glob_test);
	g_test_add_func ("/misc/matches", match_test);
	g_test_add_func ("/gvcp/force-ip-packet", gvcp_force_ip_packet_test);
	g_test_add_func ("/misc/event", event_test);
	g_test_add_func ("/gvcp/event-packet", gvcp_event_packet_test);


	result = g_test_run();

	arv_shutdown ();

	return result;
}
