/***
  Copyright 2009 Lennart Poettering
  Copyright 2010 David Henningsson <diwic@ubuntu.com>
  Copyright 2014 Emmanuel Pacaud <emmanuel@gnome.org>

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
***/

#include <errno.h>
#include <sys/types.h>

#define RTKIT_SERVICE_NAME "org.freedesktop.RealtimeKit1"
#define RTKIT_OBJECT_PATH "/org/freedesktop/RealtimeKit1"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <gio/gio.h>
#include <stdlib.h>

#define ARV_RTKIT_ERROR arv_rtkit_error_quark ()

typedef enum {
	ARV_RTKIT_ERROR_PERMISSION_DENIED,
	ARV_RTKIT_ERROR_WRONG_REPLY
} ArvRtkitError;

GQuark
arv_rtkit_error_quark (void)
{
  return g_quark_from_static_string ("arv-rtkit-error-quark");
}

gint64
arv_rtkit_get_int_property (GDBusConnection *connection, const char* propname, GError **error) {

	GDBusMessage *message;
	GDBusMessage *reply;
	GError *local_error = NULL;
	GVariant *body;
	GVariant *parameter;
	GVariant *variant;
	const GVariantType *variant_type;
	gint64 value;

	message = g_dbus_message_new_method_call (RTKIT_SERVICE_NAME,
						  RTKIT_OBJECT_PATH,
						  "org.freedesktop.DBus.Properties",
						  "Get");
	g_dbus_message_set_body (message, g_variant_new ("(ss)", "org.freedesktop.RealtimeKit1", propname));

	reply = g_dbus_connection_send_message_with_reply_sync (connection, message,
								G_DBUS_SEND_MESSAGE_FLAGS_NONE, 1000, NULL, NULL,
								&local_error);
	g_object_unref (message);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return 0;
	}

	if (g_dbus_message_get_message_type (reply) != G_DBUS_MESSAGE_TYPE_METHOD_RETURN) {
		local_error = g_error_new (ARV_RTKIT_ERROR, ARV_RTKIT_ERROR_PERMISSION_DENIED,
					   "%s", g_dbus_message_get_error_name (reply));
		g_propagate_error (error, local_error);
		g_object_unref (reply);
		return 0;
	}

	if (!g_variant_type_equal ("v", g_dbus_message_get_signature (reply))) {
		local_error = g_error_new (ARV_RTKIT_ERROR, ARV_RTKIT_ERROR_WRONG_REPLY,
					   "Invalid reply signature");
		g_propagate_error (error, local_error);
		g_object_unref (reply);
		return 0;
	}

	body = g_dbus_message_get_body (reply);
	parameter = g_variant_get_child_value (body, 0);
	variant = g_variant_get_variant (parameter);

	variant_type = g_variant_get_type (variant);

	if (g_variant_type_equal (variant_type, G_VARIANT_TYPE_INT32))
		value = g_variant_get_int32 (variant);
	else if (g_variant_type_equal (variant_type, G_VARIANT_TYPE_INT64))
		value = g_variant_get_int64 (variant);
	else
		value = 0;

	g_variant_unref (parameter);
	g_variant_unref (variant);
	g_object_unref (reply);

	return value;
}

int
arv_rtkit_get_max_realtime_priority (GDBusConnection *connection, GError **error)
{
	return arv_rtkit_get_int_property (connection, "MaxRealtimePriority", error);
}

int
arv_rtkit_get_min_nice_level (GDBusConnection *connection, GError **error)
{
	return arv_rtkit_get_int_property (connection, "MinNiceLevel", error);
}

gint64
arv_rtkit_get_rttime_usec_max (GDBusConnection *connection, GError **error)
{
	GError *local_error = NULL;
	gint64 rttime_usec_max;

	rttime_usec_max = arv_rtkit_get_int_property (connection, "RTTimeUSecMax", &local_error);
	if (error == NULL)
		return rttime_usec_max;

	g_error_free (local_error);

	return arv_rtkit_get_int_property (connection, "RTTimeNSecMax", error) / 1000;
}

void
arv_rtkit_make_realtime (GDBusConnection *connection, pid_t thread, int priority, GError **error) {

	GDBusMessage *message;
	GDBusMessage *reply;
	GError *local_error = NULL;

	message = g_dbus_message_new_method_call (RTKIT_SERVICE_NAME,
						  RTKIT_OBJECT_PATH,
						  "org.freedesktop.RealtimeKit1",
						  "MakeThreadRealtime");
	g_dbus_message_set_body (message, g_variant_new ("(tu)", thread, priority));

	reply = g_dbus_connection_send_message_with_reply_sync (connection, message,
								G_DBUS_SEND_MESSAGE_FLAGS_NONE, 1000, NULL, NULL,
								&local_error);
	g_object_unref (message);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	if (g_dbus_message_get_message_type (reply) != G_DBUS_MESSAGE_TYPE_METHOD_RETURN) {
		local_error = g_error_new (ARV_RTKIT_ERROR, 0, "%s", g_dbus_message_get_error_name (reply));
		g_propagate_error (error, local_error);
		g_object_unref (reply);
		return;
	}

	g_object_unref (reply);
}

void
arv_rtkit_make_high_priority (GDBusConnection *connection, pid_t thread, int nice_level, GError **error) {

	GDBusMessage *message;
	GDBusMessage *reply;
	GError *local_error = NULL;

	message = g_dbus_message_new_method_call (RTKIT_SERVICE_NAME,
						  RTKIT_OBJECT_PATH,
						  "org.freedesktop.RealtimeKit1",
						  "MakeThreadHighPriority");
	g_dbus_message_set_body (message, g_variant_new ("(ti)", thread, nice_level));

	reply = g_dbus_connection_send_message_with_reply_sync (connection, message,
								G_DBUS_SEND_MESSAGE_FLAGS_NONE, 1000, NULL, NULL,
								&local_error);
	g_object_unref (message);

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		return;
	}

	if (g_dbus_message_get_message_type (reply) != G_DBUS_MESSAGE_TYPE_METHOD_RETURN) {
		local_error = g_error_new (ARV_RTKIT_ERROR, 0, "%s", g_dbus_message_get_error_name (reply));
		g_propagate_error (error, local_error);
		g_object_unref (reply);
		return;
	}

	g_object_unref (reply);
}

#ifndef SCHED_RESET_ON_FORK
#define SCHED_RESET_ON_FORK 0x40000000
#endif

#ifndef RLIMIT_RTTIME
#define RLIMIT_RTTIME 15
#endif

static void print_status(const char *t) {
	int ret;

	if ((ret = sched_getscheduler(0)) < 0) {
		fprintf(stderr, "sched_getscheduler() failed: %s\n", strerror(errno));
		return;
	}

	printf("%s:\n"
	       "\tSCHED_RESET_ON_FORK: %s\n",
	       t,
	       (ret & SCHED_RESET_ON_FORK) ? "yes" : "no");

	if ((ret & ~SCHED_RESET_ON_FORK) == SCHED_RR) {
		struct sched_param param;

		if (sched_getparam(0, &param) < 0) {
			fprintf(stderr, "sched_getschedparam() failed: %s\n", strerror(errno));
			return;
		}

		printf("\tSCHED_RR with priority %i\n", param.sched_priority);

	} else if ((ret & ~SCHED_RESET_ON_FORK) == SCHED_OTHER) {
		errno = 0;
		ret = getpriority(PRIO_PROCESS, 0);
		if (errno != 0) {
			fprintf(stderr, "getpriority() failed: %s\n", strerror(errno));
			return;
		}

		printf("\tSCHED_OTHER with nice level: %i\n", ret);

	} else
		fprintf(stderr, "Neither SCHED_RR nor SCHED_OTHER.\n");
}

int main(int argc, char *argv[]) {
        GDBusConnection *bus;
	GError *error = NULL;
        int max_realtime_priority, min_nice_level;
	long long rttime_usec_max;
	struct rlimit rlim;

	bus = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
	if (!G_IS_DBUS_CONNECTION (bus)) {
		fprintf (stderr, "Failed to connect to system bus: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	max_realtime_priority = arv_rtkit_get_max_realtime_priority (bus, &error);
	if (error != NULL) {
		fprintf (stderr, "Failed to get MaxRealtimePriority: %s\n", error->message);
		g_error_free (error);
		error = NULL;
	} else
		printf ("MaxRealtimePriority = %d\n", max_realtime_priority);

	min_nice_level = arv_rtkit_get_min_nice_level (bus, &error);
	if (error != NULL) {
		fprintf (stderr, "Failed to get MinNiceLevel: %s\n", error->message);
		g_error_free (error);
		error = NULL;
	} else
		printf ("MinNiceLevel = %d\n", min_nice_level);

	rttime_usec_max = arv_rtkit_get_rttime_usec_max (bus, &error);
	if (error != NULL) {
		fprintf (stderr, "Failed to get RTTimeUSecMax: %s\n", error->message);
		g_error_free (error);
		error = NULL;
	} else
		printf ("RTTimeUSecMax = %Ld\n", rttime_usec_max);

	memset(&rlim, 0, sizeof(rlim));
	rlim.rlim_cur = rlim.rlim_max = 100000000ULL; /* 100ms */
	if ((setrlimit(RLIMIT_RTTIME, &rlim) < 0))
		fprintf(stderr, "Failed to set RLIMIT_RTTIME: %s\n", strerror(errno));

	print_status("before");

	arv_rtkit_make_high_priority (bus, 0, -10, &error);
	if (error != NULL) {
		fprintf (stderr, "Failed to become high priority: %s\n", error->message);
		g_error_free (error);
		error = NULL;
	} else
		printf ("Successfully became high priority\n");

	print_status("after high priority");

	arv_rtkit_make_realtime (bus, 0, 10, &error);
	if (error != NULL) {
		fprintf (stderr, "Failed to get become realtime: %s\n", error->message);
		g_error_free (error);
		error = NULL;
	} else
		printf ("Successfully became realtime\n");

	print_status("after realtime");

	g_object_unref (bus);

        return EXIT_SUCCESS;
}
