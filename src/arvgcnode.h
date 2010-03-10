#ifndef ARV_GC_NODE_H
#define ARV_GC_NODE_H

#include <arv.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_NODE             (arv_gc_node_get_type ())
#define ARV_GC_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_NODE, ArvGcNode))
#define ARV_GC_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_NODE, ArvGcNodeClass))
#define ARV_IS_GC_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_NODE))
#define ARV_IS_GC_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_NODE))
#define ARV_GC_NODE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_NODE, ArvGcNodeClass))

typedef struct _ArvGcNodeClass ArvGcNodeClass;

struct _ArvGcNode {
	GObject	object;

	char *name;
};

struct _ArvGcNodeClass {
	GObjectClass parent_class;

	void		(*set_attribute)		(ArvGcNode *node, const char *name, const char *value);
};

GType arv_gc_node_get_type (void);

ArvGcNode * 	arv_gc_node_new 			(void);
const char *	arv_gc_node_get_name			(ArvGcNode *node);
void		arv_gc_node_set_attribute 		(ArvGcNode *node, const char *name, const char *value);

G_END_DECLS

#endif
