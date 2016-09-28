#ifndef ARV_GV_STREAM_PRIVATE_H
#define ARV_GV_STREAM_PRIVATE_H

#include <arvgvstream.h>
#include <arvstream.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _ArvGvStreamPrivate ArvGvStreamPrivate;
typedef struct _ArvGvStreamClass ArvGvStreamClass;

struct _ArvGvStream {
	ArvStream	stream;

	ArvGvStreamPrivate *priv;
};

struct _ArvGvStreamClass {
	ArvStreamClass parent_class;
};

ArvStream * 	arv_gv_stream_new			(ArvGvDevice *gv_device,
							 GInetAddress *interface_address,
							 GInetAddress *device_address,
							 ArvStreamCallback callback, void *user_data);

G_END_DECLS

#endif
