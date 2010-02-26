#ifndef ARV_BUFFER_H
#define ARV_BUFFER_H

#include <arv.h>

G_BEGIN_DECLS

typedef enum {
	ARV_BUFFER_STATUS_SUCCESS,
	ARV_BUFFER_STATUS_CLEARED,
	ARV_BUFFER_STATUS_FILLING,
	ARV_BUFFER_STATUS_MISSING_BLOCK,
	ARV_BUFFER_STATUS_SIZE_MISMATCH,
	ARV_BUFFER_STATUS_ABORTED
} ArvBufferStatus;

#define ARV_TYPE_BUFFER             (arv_buffer_get_type ())
#define ARV_BUFFER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_BUFFER, ArvBuffer))
#define ARV_BUFFER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_BUFFER, ArvBufferClass))
#define ARV_IS_BUFFER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_BUFFER))
#define ARV_IS_BUFFER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_BUFFER))
#define ARV_BUFFER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_BUFFER, ArvBufferClass))

typedef struct _ArvBufferClass ArvBufferClass;

struct _ArvBuffer {
	GObject	object;

	size_t size;
	void *data;
	gboolean is_preallocated;

	ArvBufferStatus status;

	guint32 time_stamp;
	guint32 width;
	guint32 height;
};

struct _ArvBufferClass {
	GObjectClass parent_class;
};

GType arv_buffer_get_type (void);

ArvBuffer * 		arv_buffer_new 		(size_t size, void *preallocated);

G_END_DECLS

#endif
