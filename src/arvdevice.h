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

	char *genicam;
};

struct _ArvDeviceClass {
	GObjectClass parent_class;

	size_t 		(*read_memory)		(ArvDevice *device, guint32 address, guint32 size, void *buffer);
	size_t 		(*write_memory)		(ArvDevice *device, guint32 address, guint32 size, void *buffer);
	guint32		(*read_register)	(ArvDevice *device, guint32 address);
	void		(*write_register)	(ArvDevice *device, guint32 address, guint32 value);
};

GType arv_device_get_type (void);

size_t 		arv_device_read_memory 			(ArvDevice *device, guint32 address, guint32 size,
							 void *buffer);
size_t 		arv_device_write_memory	 		(ArvDevice *device, guint32 address, guint32 size,
							 void *buffer);
guint32 	arv_device_read_register		(ArvDevice *device, guint32 address);
void		arv_device_write_register 		(ArvDevice *device, guint32 address, guint32 value);

void 		arv_device_set_genicam 			(ArvDevice *device, char *genicam);

G_END_DECLS

#endif
