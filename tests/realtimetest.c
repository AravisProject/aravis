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

#include <../src/arvrealtimeprivate.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>

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
