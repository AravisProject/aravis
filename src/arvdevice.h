#ifndef ARV_DEVICE_H
#define ARV_DEVICE_H

#include <arv.h>

G_BEGIN_DECLS

#define ARV_TYPE_DEVICE             (arv_device_get_type ())
#define ARV_DEVICE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_DEVICE, ArvDevice))
#define ARV_DEVICE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_DEVICE, ArvDeviceClass))
#define ARV_IS_DEVICE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_DEVICE))
#define ARV_IS_DEVICE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_DEVICE))
#define ARV_DEVICE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_DEVICE, ArvDeviceClass))

typedef struct _ArvDeviceClass ArvDeviceClass;

struct _ArvDevice {
	GObject	object;
};

struct _ArvDeviceClass {
	GObjectClass parent_class;

	size_t 		(*read_register)	(ArvDevice *device, guint64 address, size_t size, void *buffer);
	size_t 		(*write_register)	(ArvDevice *device, guint64 address, size_t size, void *buffer);
};

GType arv_device_get_type (void);

void 		arv_device_load_genicam 		(ArvDevice *device);
size_t 		arv_device_read_register 		(ArvDevice *device, guint64 address, size_t size, void *buffer);
size_t 		arv_device_write_register 		(ArvDevice *device, guint64 address, size_t size, void *buffer);

G_END_DECLS

#endif
