#ifndef ARV_STREAM_H
#define ARV_STREAM_H

#include <arv.h>
#include <arvbuffer.h>

G_BEGIN_DECLS

#define ARV_TYPE_STREAM             (arv_stream_get_type ())
#define ARV_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_STREAM, ArvStream))
#define ARV_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_STREAM, ArvStreamClass))
#define ARV_IS_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_STREAM))
#define ARV_IS_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_STREAM))
#define ARV_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_STREAM, ArvStreamClass))

typedef struct _ArvStreamClass ArvStreamClass;

struct _ArvStream {
	GObject	object;

	GAsyncQueue *input_queue;
	GAsyncQueue *output_queue;
};

struct _ArvStreamClass {
	GObjectClass parent_class;
};

GType arv_stream_get_type (void);

void			arv_stream_push_buffer 			(ArvStream *stream, ArvBuffer *buffer);
ArvBuffer *		arv_stream_pop_buffer			(ArvStream *stream);
int			arv_stream_get_n_available_buffers	(ArvStream *stream);

G_END_DECLS

#endif
