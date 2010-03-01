#ifndef ARV_GENICAM_NODE_H
#define ARV_GENICAM_NODE_H

#include <arv.h>

G_BEGIN_DECLS

#define ARV_TYPE_GENICAM_NODE             (arv_genicam_node_get_type ())
#define ARV_GENICAM_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GENICAM_NODE, ArvGenicamNode))
#define ARV_GENICAM_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GENICAM_NODE, ArvGenicamNodeClass))
#define ARV_IS_GENICAM_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GENICAM_NODE))
#define ARV_IS_GENICAM_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GENICAM_NODE))
#define ARV_GENICAM_NODE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GENICAM_NODE, ArvGenicamNodeClass))

typedef struct _ArvGenicamNodeClass ArvGenicamNodeClass;

struct _ArvGenicamNode {
	GObject	object;
};

struct _ArvGenicamNodeClass {
	GObjectClass parent_class;
};

GType arv_genicam_node_get_type (void);

G_END_DECLS

#endif
