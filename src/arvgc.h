#ifndef ARV_GC_H
#define ARV_GC_H

#include <arv.h>
#include <arvgcnode.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC             (arv_gc_get_type ())
#define ARV_GC(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC, ArvGc))
#define ARV_GC_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC, ArvGcClass))
#define ARV_IS_GC(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC))
#define ARV_IS_GC_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC))
#define ARV_GC_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC, ArvGcClass))

typedef struct _ArvGcClass ArvGcClass;

struct _ArvGc {
	GObject	object;

	GHashTable *nodes;
};

struct _ArvGcClass {
	GObjectClass parent_class;
};

GType arv_gc_get_type (void);

ArvGc * 			arv_gc_new 		(char *xml, size_t size);

G_END_DECLS

#endif
