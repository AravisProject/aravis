#ifndef ARV_GV_DEVICE_H
#define ARV_GV_DEVICE_H

#include <arv.h>
#include <arvdevice.h>
#include <arvgvinterface.h>

G_BEGIN_DECLS

#define ARV_TYPE_GV_DEVICE             (arv_gv_device_get_type ())
#define ARV_GV_DEVICE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GV_DEVICE, ArvGvDevice))
#define ARV_GV_DEVICE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GV_DEVICE, ArvGvDeviceClass))
#define ARV_IS_GV_DEVICE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GV_DEVICE))
#define ARV_IS_GV_DEVICE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GV_DEVICE))
#define ARV_GV_DEVICE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GV_DEVICE, ArvGvDeviceClass))

typedef struct _ArvGvDeviceClass ArvGvDeviceClass;

struct _ArvGvDevice {
	ArvDevice device;

	guint32 packet_count;

	GSocket *socket;
	GSocketAddress	*control_address;
	GSocketAddress	*device_address;
};

struct _ArvGvDeviceClass {
	ArvDeviceClass parent_class;
};

GType arv_gv_device_get_type (void);

ArvDevice * 		arv_gv_device_new 		(GInetAddress *inet_address);

G_END_DECLS

#endif
