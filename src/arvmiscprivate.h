/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_MISC_PRIVATE_H
#define ARV_MISC_PRIVATE_H

#include <arvapi.h>
#include <arvmisc.h>

typedef struct _ArvHistogram ArvHistogram;

#define ARV_TYPE_HISTOGRAM (arv_histogram_get_type())

GType arv_histogram_get_type (void);

ArvHistogram *		arv_histogram_new 		(guint n_variables, guint n_bins, double bin_step, double offset);
ArvHistogram *          arv_histogram_ref               (ArvHistogram *histogram);
void                    arv_histogram_unref             (ArvHistogram *histogram);
void 			arv_histogram_reset 		(ArvHistogram *histogram);
gboolean 		arv_histogram_fill 		(ArvHistogram *histogram, guint histogram_id, int value);
void 			arv_histogram_set_variable_name	(ArvHistogram *histogram, guint histogram_id, char const *name);

char *			arv_histogram_to_string 	(const ArvHistogram *histogram);

struct _ArvValue {
	GType type;
	union {
		gint64 v_int64;
		double v_double;
	} data;
};

#define ARV_TYPE_VALUE (arv_value_get_type())

GType arv_value_get_type (void);

typedef struct _ArvValue ArvValue;

ArvValue * 	arv_value_new_double 		(double v_double);
ArvValue * 	arv_value_new_int64 		(gint64 v_int64);
void 		arv_value_free 			(ArvValue *value);
void 		arv_value_copy 			(ArvValue *to, const ArvValue *from);
void 		arv_value_set_int64 		(ArvValue *value, gint64 v_int64);
void 		arv_value_set_double 		(ArvValue *value, double v_double);
gint64 		arv_value_get_int64 		(ArvValue *value);
double 		arv_value_get_double 		(ArvValue *value);
gboolean 	arv_value_holds_int64 		(ArvValue *value);
double 		arv_value_holds_double 		(ArvValue *value);

/* Compatibility functions */

char *          arv_g_string_free_and_steal     (GString *string) G_GNUC_WARN_UNUSED_RESULT;

/* private, but used by tests */
ARV_API gboolean	arv_parse_genicam_url	(const char *url, gssize url_length,
						 char **scheme, char **authority, char **path,
						 char **query, char **fragment,
						 guint64 *address, guint64 *size);

void 		arv_copy_memory_with_endianness	(void *to, size_t to_size, guint to_endianness,
						 void *from, size_t from_size, guint from_endianness);

void * 		arv_decompress 			(void *input_buffer, size_t input_size, size_t *output_size);

/* private, but used by tests */
ARV_API const char *	arv_vendor_alias_lookup	(const char *vendor);

/* this only wraps g_get_monotonic_time on non-windows platforms */
gint64 arv_monotonic_time_us (void);

#if GLIB_CHECK_VERSION(2,68,0)
#define arv_memdup(p,s) g_memdup2(p,s)
#else
#define arv_memdup(p,s) g_memdup(p,(size_t) s)
#endif

ARV_API GRegex *        arv_regex_new_from_glob_pattern (const char *glob, gboolean caseless);

/* See 'glib/gconstrutor.h' for some extra details on how the following constructor/destructor macros work */
#if  __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)

#define ARV_DEFINE_CONSTRUCTOR(_func) static void __attribute__((constructor)) _func (void);
#define ARV_DEFINE_DESTRUCTOR(_func) static void __attribute__((destructor)) _func (void);

#elif defined (_MSC_VER) && (_MSC_VER >= 1500)

#include <stdlib.h>

#ifdef _M_IX86
#define ARV_MSVC_SYMBOL_PREFIX "_"
#else
#define ARV_MSVC_SYMBOL_PREFIX ""
#endif

#define ARV_MSVC_CTOR(_func,_sym_prefix) \
    static void _func(void); \
    extern int (* _array ## _func)(void);              \
    int _func ## _wrapper(void) { _func(); g_slist_find (NULL,  _array ## _func); return 0; } \
    __pragma(comment(linker,"/include:" _sym_prefix # _func "_wrapper")) \
    __pragma(section(".CRT$XCU",read)) \
    __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _wrapper;

#define ARV_MSVC_DTOR(_func,_sym_prefix) \
    static void _func(void); \
    extern int (* _array ## _func)(void);              \
    int _func ## _constructor(void) { atexit (_func); g_slist_find (NULL,  _array ## _func); return 0; } \
    __pragma(comment(linker,"/include:" _sym_prefix # _func "_constructor")) \
    __pragma(section(".CRT$XCU",read)) \
    __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _constructor;

#define ARV_DEFINE_CONSTRUCTOR(_func) ARV_MSVC_CTOR (_func, ARV_MSVC_SYMBOL_PREFIX)
#define ARV_DEFINE_DESTRUCTOR(_func) ARV_MSVC_DTOR (_func, ARV_MSVC_SYMBOL_PREFIX)

#else
#error "Constructors/destructors are not supported on this compiler!"
#endif

#endif
