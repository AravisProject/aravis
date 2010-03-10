#include <arvgcinteger.h>

static void
arv_gc_integer_default_init (ArvGcIntegerInterface *gc_integer_iface)
{
}

G_DEFINE_INTERFACE (ArvGcInteger, arv_gc_integer, G_TYPE_OBJECT)

guint64
arv_gc_integer_get_value (ArvGcInteger *gc_integer)
{
	return 0;
}

void
arv_gc_integer_set_value (ArvGcInteger *gc_integer, guint64 value)
{
}

guint64
arv_gc_integer_get_min (ArvGcInteger *gc_integer)
{
	return 0;
}

guint64
arv_gc_integer_get_max (ArvGcInteger *gc_integer)
{
	return 0;
}

const char *
arv_gc_integer_get_unit	(ArvGcInteger *gc_integer)
{
	return NULL;
}

void arv_gc_integer_impose_min (ArvGcInteger *gc_integer, guint64 minimum)
{
}

void arv_gc_integer_impose_max (ArvGcInteger *gc_integer, guint64 maximum)
{
}
