#ifndef ARV_GC_INTEGER_NODE_H
#define ARV_GC_INTEGER_NODE_H

#include <arv.h>
#include <arvgcnode.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_INTEGER_NODE             (arv_gc_integer_node_get_type ())
#define ARV_GC_INTEGER_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_INTEGER_NODE, ArvGcIntegerNode))
#define ARV_GC_INTEGER_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_INTEGER_NODE, ArvGcIntegerNodeClass))
#define ARV_IS_GC_INTEGER_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_INTEGER_NODE))
#define ARV_IS_GC_INTEGER_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_INTEGER_NODE))
#define ARV_GC_INTEGER_NODE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_INTEGER_NODE, ArvGcIntegerNodeClass))

typedef struct _ArvGcIntegerNodeClass ArvGcIntegerNodeClass;

struct _ArvGcIntegerNode {
	ArvGcNode	node;
};

struct _ArvGcIntegerNodeClass {
	ArvGcNodeClass parent_class;
};

GType arv_gc_integer_node_get_type (void);

ArvGcNode * 		arv_gc_integer_node_new 		(void);

G_END_DECLS

#endif
