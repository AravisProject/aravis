#include <arvgvsp.h>
#include <arvdebug.h>
#include <arvenumtypes.h>

static const char *
arv_enum_to_string (GType type,
		    guint enum_value)
{
	GEnumClass *enum_class;
	GEnumValue *value;
	const char *retval = NULL;

	enum_class = g_type_class_ref (type);

	value = g_enum_get_value (enum_class, enum_value);
	if (value)
		retval = value->value_nick;

	g_type_class_unref (enum_class);

	return retval;
}

static const char *
arv_gvsp_packet_type_to_string (ArvGvspPacketType value)
{
	return arv_enum_to_string (ARV_TYPE_GVSP_PACKET_TYPE, value);
}

char *
arv_gvsp_packet_to_string (const ArvGvspPacket *packet, size_t packet_size)
{
	ArvGvspDataLeader *leader;
	GString *string;
	char *c_string;
	int i, j, index;

	string = g_string_new ("");

	g_string_append_printf (string, "packet_type  = %s\n",
				arv_gvsp_packet_type_to_string (g_ntohs (packet->header.packet_type)));

	switch (g_ntohs (packet->header.packet_type)) {
		case ARV_GVSP_PACKET_TYPE_DATA_LEADER:
			leader = (ArvGvspDataLeader *) &packet->data;
			g_string_append_printf (string, "width        = %d\n", g_ntohl (leader->width));
			g_string_append_printf (string, "height       = %d\n", g_ntohl (leader->height));
			break;
		case ARV_GVSP_PACKET_TYPE_DATA_TRAILER:
			break;
		case ARV_GVSP_PACKET_TYPE_DATA_BLOCK:
			break;
	}

	for (i = 0; i < (packet_size + 15) / 16; i++) {
		for (j = 0; j < 16; j++) {
			index = i * 16 + j;
			if (j == 0)
				g_string_append_printf (string, "%04x", i * 16);
			if (index < packet_size)
				g_string_append_printf (string, " %02x", *((guint8 *) ((void *) packet) + index));
			else
				g_string_append (string, "   ");
		}
		for (j = 0; j < 16; j++) {
			index = i * 16 + j;
			if (j == 0)
				g_string_append (string, "  ");
			if (index < packet_size)
				if (*((char *) ((void *) packet) + index) >= ' ')
					g_string_append_c (string, *((char *) ((void *) packet) + index));
				else g_string_append_c (string, '.');
			else
				g_string_append_c (string, ' ');
		}
		if (index < packet_size)
			g_string_append (string, "\n");
	}

	c_string = string->str;

	g_string_free (string, FALSE);

	return c_string;
}

void
arv_gvsp_packet_debug (const ArvGvspPacket *packet, size_t packet_size)
{
	char *string;

	string = arv_gvsp_packet_to_string (packet, packet_size);
	arv_debug (ARV_DEBUG_LEVEL_GVSP, "%s", string);
	g_free (string);
}
