#ifndef ARV_DEBUG_H
#define ARV_DEBUG_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	ARV_DEBUG_LEVEL_NONE,
	ARV_DEBUG_LEVEL_STANDARD,
	ARV_DEBUG_LEVEL_GVCP,
	ARV_DEBUG_LEVEL_GVSP
} ArvDebugLevel;

void 		arv_debug 			(ArvDebugLevel level, char const *format, ...);
gboolean 	arv_debug_check 		(ArvDebugLevel level);
void 		arv_debug_enable 		(ArvDebugLevel level);

G_END_DECLS

#endif
