/*-*- Mode: C; c-basic-offset: 8 -*-*/

/***
  Copyright 2009 Lennart Poettering
  Copyright 2010 David Henningsson <diwic@ubuntu.com>

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
#include <dbus/dbus.h>

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

static pid_t _gettid(void) {
        return (pid_t) syscall(SYS_gettid);
}

static int translate_error(const char *name) {
        if (strcmp(name, DBUS_ERROR_NO_MEMORY) == 0)
                return -ENOMEM;
        if (strcmp(name, DBUS_ERROR_SERVICE_UNKNOWN) == 0 ||
            strcmp(name, DBUS_ERROR_NAME_HAS_NO_OWNER) == 0)
                return -ENOENT;
        if (strcmp(name, DBUS_ERROR_ACCESS_DENIED) == 0 ||
            strcmp(name, DBUS_ERROR_AUTH_FAILED) == 0)
                return -EACCES;

        return -EIO;
}

static long long rtkit_get_int_property(DBusConnection *connection, const char* propname, long long* propval) {
        DBusMessage *m = NULL, *r = NULL;
        DBusMessageIter iter, subiter;
        dbus_int64_t i64;
        dbus_int32_t i32;
        DBusError error;
        int current_type;
        long long ret;
        const char * interfacestr = "org.freedesktop.RealtimeKit1";

        dbus_error_init(&error);

        if (!(m = dbus_message_new_method_call(
                              RTKIT_SERVICE_NAME,
                              RTKIT_OBJECT_PATH,
                              "org.freedesktop.DBus.Properties",
                              "Get"))) {
                ret = -ENOMEM;
                goto finish;
        }

        if (!dbus_message_append_args(
                            m,
                            DBUS_TYPE_STRING, &interfacestr,
                            DBUS_TYPE_STRING, &propname,
                            DBUS_TYPE_INVALID)) {
                ret = -ENOMEM;
                goto finish;
        }

        if (!(r = dbus_connection_send_with_reply_and_block(connection, m, -1, &error))) {
                ret = translate_error(error.name);
                goto finish;
        }

        if (dbus_set_error_from_message(&error, r)) {
                ret = translate_error(error.name);
                goto finish;
        }

        ret = -EBADMSG;
        dbus_message_iter_init(r, &iter);
        while ((current_type = dbus_message_iter_get_arg_type (&iter)) != DBUS_TYPE_INVALID) {

                if (current_type == DBUS_TYPE_VARIANT) {
                        dbus_message_iter_recurse(&iter, &subiter);

                        while ((current_type = dbus_message_iter_get_arg_type (&subiter)) != DBUS_TYPE_INVALID) {

                                if (current_type == DBUS_TYPE_INT32) {
                                        dbus_message_iter_get_basic(&subiter, &i32);
                                        *propval = i32;
                                        ret = 0;
                                }

                                if (current_type == DBUS_TYPE_INT64) {
                                        dbus_message_iter_get_basic(&subiter, &i64);
                                        *propval = i64;
                                        ret = 0;
                                }

                                dbus_message_iter_next (&subiter);
                         }
                }
                dbus_message_iter_next (&iter);
        }

finish:

        if (m)
                dbus_message_unref(m);

        if (r)
                dbus_message_unref(r);

        dbus_error_free(&error);

        return ret;
}

int rtkit_get_max_realtime_priority(DBusConnection *connection) {
        long long retval;
        int err;

        err = rtkit_get_int_property(connection, "MaxRealtimePriority", &retval);
        return err < 0 ? err : retval;
}

int rtkit_get_min_nice_level(DBusConnection *connection, int* min_nice_level) {
        long long retval;
        int err;

        err = rtkit_get_int_property(connection, "MinNiceLevel", &retval);
        if (err >= 0)
                *min_nice_level = retval;
        return err;
}

long long rtkit_get_rttime_usec_max(DBusConnection *connection) {
        long long retval;
        int err;

        err = rtkit_get_int_property(connection, "RTTimeUSecMax", &retval);
        return err < 0 ? err : retval;
}

int rtkit_make_realtime(DBusConnection *connection, pid_t thread, int priority) {
        DBusMessage *m = NULL, *r = NULL;
        dbus_uint64_t u64;
        dbus_uint32_t u32;
        DBusError error;
        int ret;

        dbus_error_init(&error);

        if (thread == 0)
                thread = _gettid();

        if (!(m = dbus_message_new_method_call(
                              RTKIT_SERVICE_NAME,
                              RTKIT_OBJECT_PATH,
                              "org.freedesktop.RealtimeKit1",
                              "MakeThreadRealtime"))) {
                ret = -ENOMEM;
                goto finish;
        }

        u64 = (dbus_uint64_t) thread;
        u32 = (dbus_uint32_t) priority;

        if (!dbus_message_append_args(
                            m,
                            DBUS_TYPE_UINT64, &u64,
                            DBUS_TYPE_UINT32, &u32,
                            DBUS_TYPE_INVALID)) {
                ret = -ENOMEM;
                goto finish;
        }

        if (!(r = dbus_connection_send_with_reply_and_block(connection, m, -1, &error))) {
                ret = translate_error(error.name);
                goto finish;
        }


        if (dbus_set_error_from_message(&error, r)) {
                ret = translate_error(error.name);
                goto finish;
        }

        ret = 0;

finish:

        if (m)
                dbus_message_unref(m);

        if (r)
                dbus_message_unref(r);

        dbus_error_free(&error);

        return ret;
}

int rtkit_make_high_priority(DBusConnection *connection, pid_t thread, int nice_level) {
        DBusMessage *m = NULL, *r = NULL;
        dbus_uint64_t u64;
        dbus_int32_t s32;
        DBusError error;
        int ret;

        dbus_error_init(&error);

        if (thread == 0)
                thread = _gettid();

        if (!(m = dbus_message_new_method_call(
                              RTKIT_SERVICE_NAME,
                              RTKIT_OBJECT_PATH,
                              "org.freedesktop.RealtimeKit1",
                              "MakeThreadHighPriority"))) {
                ret = -ENOMEM;
                goto finish;
        }

        u64 = (dbus_uint64_t) thread;
        s32 = (dbus_int32_t) nice_level;

        if (!dbus_message_append_args(
                            m,
                            DBUS_TYPE_UINT64, &u64,
                            DBUS_TYPE_INT32, &s32,
                            DBUS_TYPE_INVALID)) {
                ret = -ENOMEM;
                goto finish;
        }



        if (!(r = dbus_connection_send_with_reply_and_block(connection, m, -1, &error))) {
                ret = translate_error(error.name);
                goto finish;
        }


        if (dbus_set_error_from_message(&error, r)) {
                ret = translate_error(error.name);
                goto finish;
        }

        ret = 0;

finish:

        if (m)
                dbus_message_unref(m);

        if (r)
                dbus_message_unref(r);

        dbus_error_free(&error);

        return ret;
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
        DBusError error;
        DBusConnection *bus;
        int r, max_realtime_priority, min_nice_level;
        long long rttime_usec_max;
        struct rlimit rlim;

        dbus_error_init(&error);

        if (!(bus = dbus_bus_get(DBUS_BUS_SYSTEM, &error))) {
                fprintf(stderr, "Failed to connect to system bus: %s\n", error.message);
                return 1;
        }

        if ((max_realtime_priority = rtkit_get_max_realtime_priority(bus)) < 0)
                fprintf(stderr, "Failed to retrieve max realtime priority: %s\n", strerror(-max_realtime_priority));
        else
                printf("Max realtime priority is: %d\n", max_realtime_priority);

        if ((r = rtkit_get_min_nice_level(bus, &min_nice_level)))
                fprintf(stderr, "Failed to retrieve min nice level: %s\n", strerror(-r));
        else
                printf("Min nice level is: %d\n", min_nice_level);

        if ((rttime_usec_max = rtkit_get_rttime_usec_max(bus)) < 0)
                fprintf(stderr, "Failed to retrieve rttime limit: %s\n", strerror(-rttime_usec_max));
        else
                printf("Rttime limit is: %lld ns\n", rttime_usec_max);

        memset(&rlim, 0, sizeof(rlim));
        rlim.rlim_cur = rlim.rlim_max = 100000000ULL; /* 100ms */
        if ((setrlimit(RLIMIT_RTTIME, &rlim) < 0))
                fprintf(stderr, "Failed to set RLIMIT_RTTIME: %s\n", strerror(errno));

        print_status("before");

        if ((r = rtkit_make_high_priority(bus, 0, -10)) < 0)
                fprintf(stderr, "Failed to become high priority: %s\n", strerror(-r));
        else
                printf("Successfully became high priority.\n");

        print_status("after high priority");

        if ((r = rtkit_make_realtime(bus, 0, 10)) < 0)
                fprintf(stderr, "Failed to become realtime: %s\n", strerror(-r));
        else
                printf("Successfully became realtime.\n");

        print_status("after realtime");

        dbus_connection_unref(bus);

        return 0;
}
