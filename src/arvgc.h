/* Aravis - Digital camera library
 *
 * Copyright © 2009-2019 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_GC_H
#define ARV_GC_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvbuffer.h>
#include <arvdomdocument.h>

G_BEGIN_DECLS

#define ARV_GC_ERROR arv_gc_error_quark()

GQuark arv_gc_error_quark (void);

typedef enum {
	ARV_GC_ERROR_PROPERTY_NOT_DEFINED,
	ARV_GC_ERROR_PVALUE_NOT_DEFINED,
	ARV_GC_ERROR_INVALID_PVALUE,
	ARV_GC_ERROR_EMPTY_ENUMERATION,
	ARV_GC_ERROR_OUT_OF_RANGE,
	ARV_GC_ERROR_NO_DEVICE_SET,
	ARV_GC_ERROR_NO_EVENT_IMPLEMENTATION,
	ARV_GC_ERROR_NODE_NOT_FOUND,
	ARV_GC_ERROR_INVALID_LENGTH,
	ARV_GC_ERROR_READ_ONLY,
	ARV_GC_ERROR_SET_FROM_STRING_UNDEFINED,
	ARV_GC_ERROR_GET_AS_STRING_UNDEFINED
} ArvGcError;

/**
 * ArvRegisterCachePolicy:
 * @ARV_REGISTER_CACHE_POLICY_DISABLE: disable register caching
 * @ARV_REGISTER_CACHE_POLICY_ENABLE: enable register caching
 * @ARV_REGISTER_CACHE_POLICY_DEBUG: enable register caching, but read the acual register value for comparison
 * @ARV_REGISTER_CACHE_POLICY_DEFAULT: default cache policy
 *
 * Since: 0.8.0
 */

typedef enum {
	ARV_REGISTER_CACHE_POLICY_DISABLE,
	ARV_REGISTER_CACHE_POLICY_ENABLE,
	ARV_REGISTER_CACHE_POLICY_DEBUG,
	ARV_REGISTER_CACHE_POLICY_DEFAULT = ARV_REGISTER_CACHE_POLICY_DISABLE
} ArvRegisterCachePolicy;

#define ARV_TYPE_GC             (arv_gc_get_type ())
G_DECLARE_FINAL_TYPE (ArvGc, arv_gc, ARV, GC, ArvDomDocument)

ArvGc * 		arv_gc_new 				(ArvDevice *device, const void *xml, size_t size);
void 			arv_gc_register_feature_node 		(ArvGc *genicam, ArvGcFeatureNode *node);
void			arv_gc_set_register_cache_policy	(ArvGc *genicam, ArvRegisterCachePolicy policy);
ArvRegisterCachePolicy 	arv_gc_get_register_cache_policy 	(ArvGc *genicam);
void 			arv_gc_set_default_node_data 		(ArvGc *genicam, const char *node_name, ...) G_GNUC_NULL_TERMINATED;
ArvGcNode *		arv_gc_get_node				(ArvGc *genicam, const char *name);
ArvDevice *		arv_gc_get_device			(ArvGc *genicam);
void			arv_gc_set_buffer			(ArvGc *genicam, ArvBuffer *buffer);
ArvBuffer *		arv_gc_get_buffer			(ArvGc *genicam);

G_END_DECLS

#endif
