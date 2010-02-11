#include <arvdevice.h>

static GObjectClass *parent_class = NULL;

static void
arv_device_init (ArvDevice *device)
{
}

static void
arv_device_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_device_class_init (ArvDeviceClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_device_finalize;
}

G_DEFINE_ABSTRACT_TYPE (ArvDevice, arv_device, G_TYPE_OBJECT)
