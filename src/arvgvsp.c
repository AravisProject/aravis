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
arv_gvsp_packet_to_string (const ArvGvspPacket *packet)
{
	ArvGvspStartData *start_data;
	GString *string;
	char *c_string;

	string = g_string_new ("");

	g_string_append_printf (string, "packet_type  = %s\n",
				arv_gvsp_packet_type_to_string (g_ntohs (packet->header.packet_type)));

	switch (g_ntohs (packet->header.packet_type)) {
		case ARV_GVSP_PACKET_TYPE_START:
			start_data = (ArvGvspStartData *) &packet->data;
			g_string_append_printf (string, "width        = %d\n", g_ntohl (start_data->width));
			g_string_append_printf (string, "height       = %d\n", g_ntohl (start_data->height));
			break;
		case ARV_GVSP_PACKET_TYPE_STOP:
			break;
		case ARV_GVSP_PACKET_TYPE_DATA:
			break;
	}

	c_string = string->str;

	g_string_free (string, FALSE);

	return c_string;
}

void
arv_gvsp_packet_debug (const ArvGvspPacket *packet)
{
	char *string;

	string = arv_gvsp_packet_to_string (packet);
	arv_debug ("%s", string);
	g_free (string);
}
