#ifndef ARV_GC_INTEGER_H
#define ARV_GC_INTEGER_H

#include <arv.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_INTEGER             	(arv_gc_integer_get_type ())
#define ARV_GC_INTEGER(obj)             	(G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_INTEGER, ArvGcInteger))
#define ARV_IS_GC_INTEGER(obj)          	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_INTEGER))
#define ARV_GC_INTEGER_GET_INTERFFACE(obj)   	(G_TYPE_INSTANCE_GET_INTERFACE((obj), ARV_TYPE_GC_INTEGER, ArvGcIntegerInterface))

typedef struct _ArvGcIntegerInterface ArvGcIntegerInterface;

struct _ArvGcIntegerInterface {
	GTypeInterface parent;
};

GType arv_gc_integer_get_type (void);

guint64		arv_gc_integer_get_value	(ArvGcInteger *gc_integer);
void		arv_gc_integer_set_value	(ArvGcInteger *gc_integer, guint64 value);
guint64		arv_gc_integer_get_min		(ArvGcInteger *gc_integer);
guint64		arv_gc_integer_get_max		(ArvGcInteger *gc_integer);
const char *	arv_gc_integer_get_unit		(ArvGcInteger *gc_integer);
void		arv_gc_integer_impose_min	(ArvGcInteger *gc_integer, guint64 minimum);
void		arv_gc_integer_impose_max	(ArvGcInteger *gc_integer, guint64 maximum);

/* FIXME get_representation is missing */

G_END_DECLS

#endif
