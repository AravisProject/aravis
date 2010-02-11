#ifndef ARV_INTERFACE_H
#define ARV_INTERFACE_H

#include <arv.h>

G_BEGIN_DECLS

#define ARV_TYPE_INTERFACE             (arv_interface_get_type ())
#define ARV_INTERFACE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_INTERFACE, ArvInterface))
#define ARV_INTERFACE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_INTERFACE, ArvInterfaceClass))
#define ARV_IS_INTERFACE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_INTERFACE))
#define ARV_IS_INTERFACE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_INTERFACE))
#define ARV_INTERFACE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_INTERFACE, ArvInterfaceClass))

typedef struct _ArvInterfaceClass ArvInterfaceClass;

struct _ArvInterface {
	GObject	object;
};

struct _ArvInterfaceClass {
	GObjectClass parent_class;

	void		(*get_devices)		(ArvInterface *interface);
};

GType arv_interface_get_type (void);

void 			arv_interface_get_devices 		(ArvInterface *interface);

G_END_DECLS

#endif
