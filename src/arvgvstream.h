#ifndef ARV_GV_STREAM_H
#define ARV_GV_STREAM_H

#include <arvstream.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define ARV_TYPE_GV_STREAM             (arv_gv_stream_get_type ())
#define ARV_GV_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GV_STREAM, ArvGvStream))
#define ARV_GV_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GV_STREAM, ArvGvStreamClass))
#define ARV_IS_GV_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GV_STREAM))
#define ARV_IS_GV_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GV_STREAM))
#define ARV_GV_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GV_STREAM, ArvGvStreamClass))

typedef struct _ArvGvStreamClass ArvGvStreamClass;

struct _ArvGvStream {
	ArvStream	stream;

	GSocket *socket;
	GSocketAddress *incoming_address;
};

struct _ArvGvStreamClass {
	ArvStreamClass parent_class;
};

GType arv_gv_stream_get_type (void);

G_END_DECLS

#endif
