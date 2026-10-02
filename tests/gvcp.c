/* SPDX-License-Identifier:Unlicense */

#include <arv.h>
#include <string.h>
#include "../src/arvgvcpprivate.h"
#include "../src/arvgvdeviceprivate.h"

#define TEST_REGISTER 0x1f0
#define TEST_VALUE 0x12345678

typedef enum {
        REPLY_NORMAL,
        REPLY_PENDING,
        REPLY_STALE_PENDING,
        REPLY_WRONG_TYPE_PENDING,
        REPLY_SHORT_PENDING
} ReplyMode;

typedef struct {
        ArvFakeCamera *camera;
        GSocket *socket;
        GThread *thread;
        gint stopping;
        gint mode;
        gint target_requests;
        GSocketAddress *delayed_peer;
        guint8 delayed_ack[12];
        gint64 delayed_ack_time;
} GvcpServer;

static guint32
read_uint32 (const guint8 *data)
{
        guint32 value;

        memcpy (&value, data, sizeof (value));
        return GUINT32_FROM_BE (value);
}

static void
write_uint32 (guint8 *data, guint32 value)
{
        value = GUINT32_TO_BE (value);
        memcpy (data, &value, sizeof (value));
}

static void
write_header (guint8 *data, guint8 type, guint16 command, guint16 size, guint16 id)
{
        ArvGvcpHeader header = {0};

        header.packet_type = type;
        header.command = GUINT16_TO_BE (command);
        header.size = GUINT16_TO_BE (size);
        header.id = GUINT16_TO_BE (id);
        memcpy (data, &header, sizeof (header));
}

static void
send_reply (GvcpServer *server, GSocketAddress *peer, const guint8 *data, gsize size)
{
        GError *error = NULL;
        gssize sent;

        sent = g_socket_send_to (server->socket, peer, (const char *) data, size, NULL, &error);
        g_assert_no_error (error);
        g_assert_cmpint (sent, ==, size);
}

static gpointer
server_thread (gpointer data)
{
        GvcpServer *server = data;

        while (!g_atomic_int_get (&server->stopping)) {
                guint8 request[2048];
                guint8 reply[2048];
                ArvGvcpHeader header;
                GSocketAddress *peer = NULL;
                GError *error = NULL;
                gssize count;
                guint32 address, value, size;
                guint16 command, id;
                gsize reply_size;

                if (server->delayed_peer != NULL &&
                    g_get_monotonic_time () >= server->delayed_ack_time) {
                        send_reply (server, server->delayed_peer, server->delayed_ack,
                                    sizeof (server->delayed_ack));
                        g_clear_object (&server->delayed_peer);
                }

                if (!g_socket_condition_timed_wait (server->socket, G_IO_IN, 2000, NULL, &error)) {
                        g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
                        g_clear_error (&error);
                        continue;
                }

                count = g_socket_receive_from (server->socket, &peer, (char *) request,
                                               sizeof (request), NULL, &error);
                g_assert_no_error (error);
                g_assert_cmpint (count, >=, sizeof (header) + sizeof (guint32));
                memcpy (&header, request, sizeof (header));
                command = GUINT16_FROM_BE (header.command);
                id = GUINT16_FROM_BE (header.id);
                address = read_uint32 (request + sizeof (header));

                switch (command) {
                        case ARV_GVCP_COMMAND_READ_REGISTER_CMD:
                                g_assert_true (arv_fake_camera_read_register (server->camera, address, &value));
                                write_uint32 (reply + sizeof (header), value);
                                reply_size = sizeof (header) + sizeof (guint32);
                                break;
                        case ARV_GVCP_COMMAND_WRITE_REGISTER_CMD:
                                g_assert_cmpint (count, >=, sizeof (header) + 2 * sizeof (guint32));
                                value = read_uint32 (request + sizeof (header) + sizeof (guint32));
                                g_assert_true (arv_fake_camera_write_register (server->camera, address, value));
                                write_uint32 (reply + sizeof (header), 1);
                                reply_size = sizeof (header) + sizeof (guint32);
                                break;
                        case ARV_GVCP_COMMAND_READ_MEMORY_CMD:
                                g_assert_cmpint (count, >=, sizeof (header) + 2 * sizeof (guint32));
                                size = read_uint32 (request + sizeof (header) + sizeof (guint32)) & 0xffff;
                                g_assert_cmpuint (size, <=, sizeof (reply) - sizeof (header) - sizeof (guint32));
                                write_uint32 (reply + sizeof (header), address);
                                g_assert_true (arv_fake_camera_read_memory (server->camera, address, size,
                                                                          reply + sizeof (header) + sizeof (guint32)));
                                reply_size = sizeof (header) + sizeof (guint32) + size;
                                break;
                        default:
                                g_assert_not_reached ();
                }
                write_header (reply, ARV_GVCP_PACKET_TYPE_ACK, command + 1,
                              reply_size - sizeof (header), id);

                if (command == ARV_GVCP_COMMAND_READ_REGISTER_CMD && address == TEST_REGISTER &&
                    g_atomic_int_get (&server->mode) != REPLY_NORMAL &&
                    g_atomic_int_add (&server->target_requests, 1) == 0) {
                        guint8 pending[12];
                        gint mode = g_atomic_int_get (&server->mode);
                        guint32 delay_ms = MAX (2 * ARV_GV_DEVICE_GVCP_TIMEOUT_MS_DEFAULT, 200);

                        memcpy (server->delayed_ack, reply, sizeof (server->delayed_ack));
                        server->delayed_peer = g_object_ref (peer);
                        server->delayed_ack_time = g_get_monotonic_time () + delay_ms * 1000;
                        write_header (pending,
                                      mode == REPLY_WRONG_TYPE_PENDING ? ARV_GVCP_PACKET_TYPE_CMD :
                                                                         ARV_GVCP_PACKET_TYPE_ACK,
                                      ARV_GVCP_COMMAND_PENDING_ACK, sizeof (guint32),
                                      mode == REPLY_STALE_PENDING ? arv_gvcp_next_packet_id (id) : id);
                        write_uint32 (pending + sizeof (header), delay_ms + 500);
                        send_reply (server, peer, pending,
                                    mode == REPLY_SHORT_PENDING ? sizeof (header) : sizeof (pending));
                } else {
                        send_reply (server, peer, reply, reply_size);
                }
                g_object_unref (peer);
        }

        return NULL;
}

static void
pending_ack_test (gconstpointer data)
{
        ReplyMode mode = GPOINTER_TO_INT (data);
        GvcpServer server = {0};
        GInetAddress *loopback;
        GSocketAddress *address;
        ArvDevice *device;
        GError *error = NULL;
        guint32 value = 0;
        gboolean success;
        gint requests;

        server.camera = arv_fake_camera_new ("GvcpTest");
        g_assert_nonnull (server.camera);
        g_assert_true (arv_fake_camera_write_register (server.camera, TEST_REGISTER, TEST_VALUE));
        server.socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM,
                                      G_SOCKET_PROTOCOL_UDP, &error);
        g_assert_no_error (error);
        loopback = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);
        address = g_inet_socket_address_new (loopback, ARV_GVCP_PORT);
        success = g_socket_bind (server.socket, address, FALSE, &error);
        g_assert_no_error (error);
        g_assert_true (success);
        g_object_unref (address);
        server.thread = g_thread_new ("gvcp-test", server_thread, &server);

        /* Direct construction only contacts loopback; no discovery broadcasts or real camera. */
        device = arv_gv_device_new (loopback, loopback, &error);
        g_assert_no_error (error);
        g_assert_nonnull (device);
        g_object_unref (loopback);
        g_atomic_int_set (&server.mode, mode);
        success = arv_device_read_register (device, TEST_REGISTER, &value, &error);
        requests = g_atomic_int_get (&server.target_requests);

        /* Restore ordinary responses before the heartbeat and leave-control teardown. */
        g_atomic_int_set (&server.mode, REPLY_NORMAL);
        g_object_unref (device);
        g_atomic_int_set (&server.stopping, TRUE);
        g_thread_join (server.thread);
        g_clear_object (&server.delayed_peer);
        g_object_unref (server.socket);
        g_object_unref (server.camera);

        g_assert_no_error (error);
        g_assert_true (success);
        g_assert_cmphex (value, ==, TEST_VALUE);
        /* Only a matching pending ACK should defer the retry until the delayed valid ACK. */
        g_test_message ("GVCP sends: %d", requests);
        g_assert_cmpint (requests, ==, mode == REPLY_PENDING ? 1 : 2);
}

int
main (int argc, char **argv)
{
        g_test_init (&argc, &argv, NULL);
        g_test_add_data_func ("/gvcp/pending/matching", GINT_TO_POINTER (REPLY_PENDING), pending_ack_test);
        g_test_add_data_func ("/gvcp/pending/stale-id", GINT_TO_POINTER (REPLY_STALE_PENDING), pending_ack_test);
        g_test_add_data_func ("/gvcp/pending/wrong-type", GINT_TO_POINTER (REPLY_WRONG_TYPE_PENDING), pending_ack_test);
        g_test_add_data_func ("/gvcp/pending/short", GINT_TO_POINTER (REPLY_SHORT_PENDING), pending_ack_test);

        return g_test_run ();
}
