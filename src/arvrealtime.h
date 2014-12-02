#ifndef ARV_REALTIME_H
#define ARV_REALTIME_H

#include <arvtypes.h>

G_BEGIN_DECLS

gboolean	arv_make_thread_realtime 		(int priority);
gboolean	arv_make_thread_high_priority 		(int nice_level);

G_END_DECLS

#endif
