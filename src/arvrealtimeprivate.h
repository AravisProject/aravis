#ifndef ARV_REALTIME_PRIVATE_H
#define ARV_REALTIME_PRIVATE_H

#include <arvrealtime.h>
#include <gio/gio.h>

int 		arv_rtkit_get_max_realtime_priority	(GDBusConnection *connection, GError **error);
int 		arv_rtkit_get_min_nice_level 		(GDBusConnection *connection, GError **error);
gint64		arv_rtkit_get_rttime_usec_max 		(GDBusConnection *connection, GError **error);
void		arv_rtkit_make_realtime 		(GDBusConnection *connection, pid_t thread, int priority, GError **error);
void		arv_rtkit_make_high_priority 		(GDBusConnection *connection, pid_t thread, int nice_level, GError **error);

#endif
