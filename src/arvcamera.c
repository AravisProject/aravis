#include <arvcamera.h>

static GObjectClass *parent_class = NULL;

static void
arv_camera_init (ArvCamera *camera)
{
}

static void
arv_camera_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_camera_class_init (ArvCameraClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_camera_finalize;
}

G_DEFINE_TYPE (ArvCamera, arv_camera, G_TYPE_OBJECT)
