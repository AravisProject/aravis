#ifndef ARV_CAMERA_H
#define ARV_CAMERA_H

#include <arv.h>

G_BEGIN_DECLS

#define ARV_TYPE_CAMERA             (arv_camera_get_type ())
#define ARV_CAMERA(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_CAMERA, ArvCamera))
#define ARV_CAMERA_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_CAMERA, ArvCameraClass))
#define ARV_IS_CAMERA(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_CAMERA))
#define ARV_IS_CAMERA_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_CAMERA))
#define ARV_CAMERA_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_CAMERA, ArvCameraClass))

typedef struct _ArvCameraClass ArvCameraClass;

struct _ArvCamera {
	GObject	object;
};

struct _ArvCameraClass {
	GObjectClass parent_class;
};

GType arv_camera_get_type (void);

G_END_DECLS

#endif
