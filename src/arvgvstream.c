#include <arvgvstream.h>
#include <arvgvsp.h>

static GObjectClass *parent_class = NULL;

/* Acquisition thread */

typedef struct {
	GSocket *socket;
	gboolean cancel;
} ArvGvStreamThreadData;

static void *
arv_gv_stream_thread (void *data)
{
	ArvGvStreamThreadData *thread_data = data;
	GPollFD poll_fd;
	int n_events;
	char buffer[1024];

	poll_fd.fd = g_socket_get_fd (thread_data->socket);
	poll_fd.events =  G_IO_IN;
	poll_fd.revents = 0;

	do {
		n_events = g_poll (&poll_fd, 1, 50);

		if (n_events > 0) {
			g_socket_receive (thread_data->socket, buffer, 1024, NULL, NULL);
			arv_gvsp_packet_debug ((ArvGvspPacket *)buffer);
		}
	} while (!thread_data->cancel);

	return NULL;
}

/* ArvGvStream implemenation */

guint16
arv_gv_stream_get_port (ArvGvStream *gv_stream)
{
	GInetSocketAddress *local_address;

	g_return_val_if_fail (ARV_IS_GV_STREAM (gv_stream), 0);

	local_address = G_INET_SOCKET_ADDRESS (g_socket_get_local_address (gv_stream->socket, NULL));

	return g_inet_socket_address_get_port (local_address);
}

ArvStream *
arv_gv_stream_new (guint16 port)
{
	ArvGvStream *gv_stream;
	GInetAddress *incoming_inet_address;
	ArvGvStreamThreadData *thread_data;

	gv_stream = g_object_new (ARV_TYPE_GV_STREAM, NULL);

	gv_stream->socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
					  G_SOCKET_TYPE_DATAGRAM,
					  G_SOCKET_PROTOCOL_UDP, NULL);

	incoming_inet_address = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
	gv_stream->incoming_address = g_inet_socket_address_new (incoming_inet_address, port);
	g_object_unref (incoming_inet_address);

	g_socket_bind (gv_stream->socket, gv_stream->incoming_address, TRUE, NULL);

	thread_data = g_new (ArvGvStreamThreadData, 1);
	thread_data->socket = gv_stream->socket;
	thread_data->cancel = FALSE;

	gv_stream->thread_data = thread_data;

	gv_stream->thread = g_thread_create (arv_gv_stream_thread, gv_stream->thread_data, TRUE, NULL);

	return ARV_STREAM (gv_stream);
}

/* ArvStream implementation */

static void
arv_gv_stream_init (ArvGvStream *gv_stream)
{
}

static void
arv_gv_stream_finalize (GObject *object)
{
	ArvGvStream *gv_stream = ARV_GV_STREAM (object);

	if (gv_stream->thread != NULL) {
		ArvGvStreamThreadData *thread_data;

		thread_data = gv_stream->thread_data;

		thread_data->cancel = TRUE;
		g_thread_join (gv_stream->thread);
		g_free (thread_data);

		gv_stream->thread_data = NULL;
		gv_stream->thread = NULL;
	}

	g_object_unref (gv_stream->incoming_address);
	g_object_unref (gv_stream->socket);

	parent_class->finalize (object);
}

static void
arv_gv_stream_class_init (ArvGvStreamClass *gv_stream_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (gv_stream_class);

	parent_class = g_type_class_peek_parent (gv_stream_class);

	object_class->finalize = arv_gv_stream_finalize;
}

G_DEFINE_TYPE (ArvGvStream, arv_gv_stream, ARV_TYPE_STREAM)
