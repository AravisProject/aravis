/* Aravis - Digital camera library
 *
 * Copyright © 2023 Václav Šmilauer
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
 * Authors: Václav Šmilauer <eu@doxos.eu>
 */

#include"private.h"

G_DEFINE_TYPE (ArvGentlEvent,arv_gentl_event,G_TYPE_OBJECT);

static void
arv_gentl_event_class_init(ArvGentlEventClass* self)
{
}

static void
arv_gentl_event_init(ArvGentlEvent* self)
{
}


G_DEFINE_TYPE (ArvTransportLayer,arv_transport_layer,G_TYPE_OBJECT);

static void
arv_transport_layer_class_init(ArvTransportLayerClass* self)
{
}

static void arv_transport_layer_init(ArvTransportLayer* self)
{
}

/* global variables */
int gentl_GCInitLib = 0;

/* singleton for the transport layer */
ArvTransportLayer* gentl_transport_layer = NULL;

/* error data must be thread-local as per GenTL spec */
GENTL_THREAD_LOCAL_STORAGE GError* gentl_err = NULL;

/* Map handle (pointer) to ArvGentlHandleEvents containing all events associated to this handle. This is not yet
 * implemented, someone knowledgeable of both Aravis and GenTL could propose a clean way of handling this. */
GHashTable* gentl_events=NULL;

GQuark
gentl_error_quark (void)
{
  return g_quark_from_static_string ("gentl-error-quark");
}

GC_API
gentl_init()
{
	if (gentl_GCInitLib != 0)
                return GC_ERR_RESOURCE_IN_USE;

	gentl_GCInitLib = 1;
	gentl_events = g_hash_table_new(g_direct_hash,g_direct_equal);

	return GC_ERR_SUCCESS;
}

GC_API
gentl_fini()
{
	GENTL_ENSURE_INIT;
	gentl_GCInitLib = 0;

	g_hash_table_destroy(gentl_events);

	gentl_events = NULL;

	return GC_ERR_SUCCESS;
}

gboolean
gentl_is_initialized (void)
{
	return gentl_GCInitLib == 1;
}

/* All events associated to one handle. Each handle can have at most one event of a given type as per GenTL spec. */
struct ArvGentlHandleEvents{
	/* EVENT_MODULE is the last one in EVENT_TYPE_LIST */
	ArvGentlEvent* ev[EVENT_MODULE+1];
};


