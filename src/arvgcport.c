/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <arvgcport.h>

static GObjectClass *parent_class = NULL;

void
arv_gc_port_read (ArvGcPort *port, void *buffer, guint64 address, guint64 length)
{
	ArvGc *genicam;
	ArvDevice *device;

	g_return_if_fail (ARV_IS_GC_PORT (port));

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (port));
	device = arv_gc_get_device (genicam);

	arv_device_read_memory (device, address, length, buffer);
}

void
arv_gc_port_write (ArvGcPort *port, void *buffer, guint64 address, guint64 length)
{
	ArvGc *genicam;
	ArvDevice *device;

	g_return_if_fail (ARV_IS_GC_PORT (port));

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (port));
	device = arv_gc_get_device (genicam);

	arv_device_write_memory (device, address, length, buffer);
}

ArvGcNode *
arv_gc_port_new (void)
{
	ArvGcNode *node;

	node = g_object_new (ARV_TYPE_GC_PORT, NULL);

	return node;
}

static void
arv_gc_port_init (ArvGcPort *gc_port)
{
}

static void
arv_gc_port_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_gc_port_class_init (ArvGcPortClass *port_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (port_class);

	parent_class = g_type_class_peek_parent (port_class);

	object_class->finalize = arv_gc_port_finalize;
}

G_DEFINE_TYPE (ArvGcPort, arv_gc_port, ARV_TYPE_GC_NODE)
