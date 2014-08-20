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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
#include <arvchunkparserprivate.h>
#include <arvbuffer.h>
#include <arvgcpropertynode.h>
#include <arvgc.h>
#include <memory.h>

static GObjectClass *parent_class = NULL;

struct _ArvGcPortPrivate {
	ArvGcPropertyNode *chunk_id;
};

/* ArvDomNode implementation */

static const char *
_get_node_name (ArvDomNode *node)
{
	return "Port";
}

static void
_post_new_child (ArvDomNode *self, ArvDomNode *child)
{
	ArvGcPort *node = ARV_GC_PORT (self);

	if (ARV_IS_GC_PROPERTY_NODE (child)) {
		ArvGcPropertyNode *property_node = ARV_GC_PROPERTY_NODE (child);

		switch (arv_gc_property_node_get_node_type (property_node)) {
			case ARV_GC_PROPERTY_NODE_TYPE_CHUNK_ID:
				node->priv->chunk_id = property_node;
				break;
			default:
				ARV_DOM_NODE_CLASS (parent_class)->post_new_child (self, child);
				break;
		}
	} else
		ARV_DOM_NODE_CLASS (parent_class)->post_new_child (self, child);
}

static void
_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
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

	g_return_if_fail (ARV_IS_GC_PORT (port));
	g_return_if_fail (error == NULL || *error == NULL);
	g_return_if_fail (buffer != NULL);

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (port));

	if (port->priv->chunk_id == NULL) {
		ArvDevice *device;

		device = arv_gc_get_device (genicam);
		if (ARV_IS_DEVICE (device)) {
			/* For schema < 1.1.0 and length == 4, register read must be used instead of memory read.
			 * See Appendix 3 of Genicam 2.0 specification. */
			if (_register_workaround_check (port, length)) {
				guint32 value;

				/* For schema < 1.1.0, all registers are big endian. */
				value = *((guint32 *) buffer);
				value = GUINT32_FROM_BE (value);

				arv_device_read_register (device, address, &value, error);
				*((guint32 *) buffer) = GUINT32_TO_BE (value);
			} else
				arv_device_read_memory (device, address, length, buffer, error);
		} else {
			if (error != NULL && *error == NULL)
				*error = g_error_new (ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_STATUS_INVALID_FEATURE_NAME,
						      "[ArvGcPort::read] Invalid feature name");
		}
	} else {
		ArvBuffer *chunk_data_buffer;

		chunk_data_buffer = arv_gc_get_buffer (genicam);

		if (!ARV_IS_BUFFER (chunk_data_buffer)) {
			if (error != NULL && *error == NULL)
				*error = g_error_new (ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_STATUS_BUFFER_NOT_FOUND,
						      "[ArvGcPort::read] Buffer not found");
		} else {
			char *chunk_data;
			size_t chunk_data_size;
			guint chunk_id;

			chunk_id = g_ascii_strtoll (arv_gc_property_node_get_string (port->priv->chunk_id, NULL), NULL, 16);
			chunk_data = (char *) arv_buffer_get_chunk_data (chunk_data_buffer, chunk_id, &chunk_data_size);

			if (chunk_data != NULL) {
				memcpy (buffer, chunk_data + address, MIN (chunk_data_size - address, length));
			} else {
				if (error != NULL && *error == NULL)
					*error = g_error_new (ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_STATUS_CHUNK_NOT_FOUND,
							      "[ArvGcPort::read] Chunk 0x%08x not found", chunk_id);
			}
		}
	}
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
	
	if (port->priv->chunk_id == NULL) {
		device = arv_gc_get_device (genicam);

		if (ARV_IS_DEVICE (device)) {
			/* For schema < 1.1.0 and length == 4, register write must be used instead of memory write.
			 * See Appendix 3 of Genicam 2.0 specification. */
			if (_register_workaround_check (port, length)) {
				guint32 value;

				/* For schema < 1.1.0, all registers are big endian. */
				value = *((guint32 *) buffer);
				value = GUINT32_FROM_BE (value);

				arv_device_write_register (device, address, value, error);
			} else
				arv_device_write_memory (device, address, length, buffer, error);
		} else {
			if (error != NULL && *error == NULL)
				*error = g_error_new (ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_STATUS_INVALID_FEATURE_NAME,
						      "[ArvGcPort::write] Invalid feature name");
		}
	} else {
		ArvBuffer *chunk_data_buffer;

		chunk_data_buffer = arv_gc_get_buffer (genicam);

		if (!ARV_IS_BUFFER (chunk_data_buffer)) {
			if (error != NULL && *error == NULL)
				*error = g_error_new (ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_STATUS_BUFFER_NOT_FOUND,
						      "[ArvGcPort::write] Buffer not found");
		} else {
			char *chunk_data;
			size_t chunk_data_size;
			guint chunk_id;

			chunk_id = g_ascii_strtoll (arv_gc_property_node_get_string (port->priv->chunk_id, NULL), NULL, 16);
			chunk_data = (char *) arv_buffer_get_chunk_data (chunk_data_buffer, chunk_id, &chunk_data_size);

			if (chunk_data != NULL) {
				memcpy (chunk_data + address, buffer, MIN (chunk_data_size - address, length));
			} else {
				if (error != NULL && *error == NULL)
					*error = g_error_new (ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_STATUS_CHUNK_NOT_FOUND,
							      "[ArvGcPort::write] Chunk 0x%08x not found", chunk_id);
			}
		}
	}
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
	gc_port->priv = G_TYPE_INSTANCE_GET_PRIVATE (gc_port, ARV_TYPE_GC_PORT, ArvGcPortPrivate);
	gc_port->priv->chunk_id = 0;
}

static void
_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
arv_gc_port_class_init (ArvGcPortClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);

	g_type_class_add_private (this_class, sizeof (ArvGcPortPrivate));

	parent_class = g_type_class_peek_parent (this_class);

	object_class->finalize = _finalize;
	dom_node_class->get_node_name = _get_node_name;
	dom_node_class->post_new_child = _post_new_child;
	dom_node_class->pre_remove_child = _pre_remove_child;
}

G_DEFINE_TYPE (ArvGcPort, arv_gc_port, ARV_TYPE_GC_FEATURE_NODE)
