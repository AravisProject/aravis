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

/**
 * SECTION: arvgcport
 * @short_description: Class for Port nodes
 */

#include <arvgcport.h>
#include <arvgcregisterdescriptionnode.h>
#include <arvdevice.h>
#include <arvgc.h>

static GObjectClass *parent_class = NULL;

/* ArvDomNode implementation */

static const char *
arv_gc_port_get_node_name (ArvDomNode *node)
{
	return "Port";
}

/* ArvGcPort implementation */

static gboolean
_register_workaround_check (ArvGcPort *port, guint64 length) 
{
	ArvDomDocument *document;
	ArvGcRegisterDescriptionNode *register_description;

	document = arv_dom_node_get_owner_document (ARV_DOM_NODE (port));
	register_description = ARV_GC_REGISTER_DESCRIPTION_NODE (arv_dom_document_get_document_element (document));

	return length == 4 && !arv_gc_register_description_node_check_schema_version (register_description, 1, 1, 0);
}

void
arv_gc_port_read (ArvGcPort *port, void *buffer, guint64 address, guint64 length, GError **error)
{
	ArvGc *genicam;
	ArvDevice *device;

	g_return_if_fail (ARV_IS_GC_PORT (port));
	g_return_if_fail (error == NULL || *error == NULL);
	g_return_if_fail (buffer != NULL);

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (port));
	device = arv_gc_get_device (genicam);

	if (_register_workaround_check (port, length)) {
		guint32 value;

		value = *((guint32 *) buffer);
		value = GUINT32_FROM_BE (value);

		arv_device_read_register (device, address, &value, error);
		*((guint32 *) buffer) = GUINT32_TO_BE (value);
	} else
		arv_device_read_memory (device, address, length, buffer, error);
}

void
arv_gc_port_write (ArvGcPort *port, void *buffer, guint64 address, guint64 length, GError **error)
{
	ArvGc *genicam;
	ArvDevice *device;

	g_return_if_fail (ARV_IS_GC_PORT (port));
	g_return_if_fail (error == NULL || *error == NULL);
	g_return_if_fail (buffer != NULL);

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (port));
	device = arv_gc_get_device (genicam);

	if (_register_workaround_check (port, length)) {
		guint32 value;

		value = *((guint32 *) buffer);
		value = GUINT32_FROM_BE (value);

		arv_device_write_register (device, address, value, error);
	} else
		arv_device_write_memory (device, address, length, buffer, error);
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
arv_gc_port_class_init (ArvGcPortClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = arv_gc_port_finalize;
	dom_node_class->get_node_name = arv_gc_port_get_node_name;
}

G_DEFINE_TYPE (ArvGcPort, arv_gc_port, ARV_TYPE_GC_FEATURE_NODE)
