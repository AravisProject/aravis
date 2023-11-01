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

/**
 * SECTION: arvgcport
 * @short_description: Class for Port nodes
 */

#include <arvgcport.h>
#include <arvgcregisterdescriptionnode.h>
#include <arvgcfeaturenodeprivate.h>
#include <arvdevice.h>
#include <arvgvdevice.h>
#include <arvchunkparserprivate.h>
#include <arvbuffer.h>
#include <arvgcpropertynode.h>
#include <arvgc.h>
#include <memory.h>

typedef struct {
	const char *vendor_selection;
	const char *model_selection;
} ArvGvLegacyInfos;

/*
 * Some GigEVision devices incorrectly report a Genicam schema version greater or equal to 1.1, while implementing the
 * legacy behavior for register access. This list allows to force the use of the legacy endianness mechanism. Vendor and
 * model listed below are those found in the Genicam XML data, which can be obtained using the `genicam` command of
 * `arv-tool`.  They are stored in the ModelName and VendorName attributes of the RegisterDescription element.
 *
 * The documentation about the legacy endianness mechanism is in the 3.1 appendix ('Endianess of GigE Vision Cameras')
 * of the GenICam Standard.
 *
 * There is a chance this part of Aravis is due to a misunderstanding of how a GigEVision device is supposed to behave
 * (Remember we can not use the GigEVision specification documentation).  But until now, there was no evidence in the
 * issue reports it is the case. If you think this should be implemented differently, don't hesitate to explain your
 * thoughts on Aravis issue report system, or even better, to open a pull request. The related Aravis issues are
 * available here:
 *
 * https://github.com/AravisProject/aravis/labels/5.%20Genicam%201.0%20legacy%20mode
 */

static ArvGvLegacyInfos arv_gc_port_legacy_infos[] = {
   { .vendor_selection = "Imperx",                      .model_selection = "IpxGEVCamera"},
   { .vendor_selection = "KowaOptronics",               .model_selection = "SC130ET3"},
   { .vendor_selection = "NIT",                         .model_selection = "Tachyon16k"},
   { .vendor_selection = "PleoraTechnologiesInc",       .model_selection = "iPORTCLGigE"},
   { .vendor_selection = "PleoraTechnologiesInc",       .model_selection = "NTxGigE"},
   { .vendor_selection = "TeledyneDALSA",               .model_selection = "ICE"},
   { .vendor_selection = "Sony",                        .model_selection = "XCG_CGSeries"},
   { .vendor_selection = "EVK",                         .model_selection = "HELIOS"},
   { .vendor_selection = "AT_Automation_Technology_GmbH",.model_selection = "C6_X_GigE"},
};

typedef struct {
	ArvGcPropertyNode *chunk_id;
	ArvGcPropertyNode *event_id;
	gboolean has_done_legacy_check;
	gboolean has_legacy_infos;
} ArvGcPortPrivate;

struct _ArvGcPort {
	ArvGcFeatureNode node;

	ArvGcPortPrivate *priv;
};

struct _ArvGcPortClass {
	ArvGcFeatureNodeClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvGcPort, arv_gc_port, ARV_TYPE_GC_FEATURE_NODE, G_ADD_PRIVATE(ArvGcPort))

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
			case ARV_GC_PROPERTY_NODE_TYPE_EVENT_ID:
				node->priv->event_id = property_node;
				break;
			default:
				ARV_DOM_NODE_CLASS (arv_gc_port_parent_class)->post_new_child (self, child);
				break;
		}
	} else
		ARV_DOM_NODE_CLASS (arv_gc_port_parent_class)->post_new_child (self, child);
}

static void
_pre_remove_child (ArvDomNode *self, ArvDomNode *child)
{
	g_assert_not_reached ();
}

/* ArvGcPort implementation */

static gboolean
_use_legacy_endianness_mechanism (ArvGcPort *port, guint64 length)
{
	if (!port->priv->has_done_legacy_check) {
		ArvDomDocument *document;
		ArvGcRegisterDescriptionNode *register_description;
		const char *vendor_name;
		const char *model_name;
		int i;

		document = arv_dom_node_get_owner_document (ARV_DOM_NODE (port));
		register_description = ARV_GC_REGISTER_DESCRIPTION_NODE (arv_dom_document_get_document_element (document));
		vendor_name = arv_gc_register_description_node_get_vendor_name(register_description);
		model_name = arv_gc_register_description_node_get_model_name(register_description);

		if (arv_gc_register_description_node_compare_schema_version (register_description, 1, 1, 0) < 0) {
			port->priv->has_legacy_infos = TRUE;
		} else {
			for (i = 0; i < G_N_ELEMENTS (arv_gc_port_legacy_infos); i++) {
				if (g_pattern_match_simple(arv_gc_port_legacy_infos[i].vendor_selection, vendor_name) == TRUE &&
					g_pattern_match_simple(arv_gc_port_legacy_infos[i].model_selection, model_name) == TRUE) {
					port->priv->has_legacy_infos = TRUE;
					break;
				}
			}
		}

		port->priv->has_done_legacy_check = TRUE;
	}

	return length == 4 && port->priv->has_legacy_infos;
}

void
arv_gc_port_read (ArvGcPort *port, void *buffer, guint64 address, guint64 length, GError **error)
{
	ArvGc *genicam;

	g_return_if_fail (ARV_IS_GC_PORT (port));
	g_return_if_fail (buffer != NULL);

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (port));

	if (port->priv->chunk_id != NULL) {
		ArvBuffer *chunk_data_buffer;

		chunk_data_buffer = arv_gc_get_buffer (genicam);

		if (!ARV_IS_BUFFER (chunk_data_buffer)) {
			g_set_error (error, ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_ERROR_BUFFER_NOT_FOUND,
				     "[%s] Buffer not found",
                                     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (port)));
		} else {
			char *chunk_data;
			size_t chunk_data_size;
			guint chunk_id;

			chunk_id = g_ascii_strtoll (arv_gc_property_node_get_string (port->priv->chunk_id, NULL), NULL, 16);
			chunk_data = (char *) arv_buffer_get_chunk_data (chunk_data_buffer, chunk_id, &chunk_data_size);

			if (chunk_data != NULL) {
				memcpy (buffer, chunk_data + address, MIN (chunk_data_size - address, length));
			} else {
				g_set_error (error, ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_ERROR_CHUNK_NOT_FOUND,
					     "[%s] Chunk 0x%08x not found",
                                             arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (port)),
                                             chunk_id);
			}
		}
	} else if (port->priv->event_id != NULL) {
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_NO_EVENT_IMPLEMENTATION,
			     "[%s] Events not implemented",
                             arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (port)));
	} else {
		ArvDevice *device;

		device = arv_gc_get_device (genicam);
		if (ARV_IS_DEVICE (device)) {
			/* For schema < 1.1.0 and length == 4, register read must be used instead of memory read.
			 * Only applies to GigE Vision devices. See Appendix 3 of Genicam 2.0 specification. */
			if (ARV_IS_GV_DEVICE (device) && _use_legacy_endianness_mechanism (port, length)) {
				guint32 value = 0;

				arv_device_read_register (device, address, &value, error);

				/* For schema < 1.1.0, all registers are big endian. */
				*((guint32 *) buffer) = GUINT32_TO_BE (value);
			} else
				arv_device_read_memory (device, address, length, buffer, error);
		} else {
			g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_NO_DEVICE_SET,
				     "[%s] No device set",
                                     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (port)));
		}
	}
}

void
arv_gc_port_write (ArvGcPort *port, void *buffer, guint64 address, guint64 length, GError **error)
{
	ArvGc *genicam;
	ArvDevice *device;

	g_return_if_fail (ARV_IS_GC_PORT (port));
	g_return_if_fail (buffer != NULL);

	genicam = arv_gc_node_get_genicam (ARV_GC_NODE (port));

	arv_gc_feature_node_increment_change_count (ARV_GC_FEATURE_NODE (port));

	if (port->priv->chunk_id != NULL) {
		ArvBuffer *chunk_data_buffer;

		chunk_data_buffer = arv_gc_get_buffer (genicam);

		if (!ARV_IS_BUFFER (chunk_data_buffer)) {
			g_set_error (error, ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_ERROR_BUFFER_NOT_FOUND,
				     "[%s] Buffer not found",
                                     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (port)));
		} else {
			char *chunk_data;
			size_t chunk_data_size;
			guint chunk_id;

			chunk_id = g_ascii_strtoll (arv_gc_property_node_get_string (port->priv->chunk_id, NULL), NULL, 16);
			chunk_data = (char *) arv_buffer_get_chunk_data (chunk_data_buffer, chunk_id, &chunk_data_size);

			if (chunk_data != NULL) {
				memcpy (chunk_data + address, buffer, MIN (chunk_data_size - address, length));
			} else {
				g_set_error (error, ARV_CHUNK_PARSER_ERROR, ARV_CHUNK_PARSER_ERROR_CHUNK_NOT_FOUND,
					     "[%s] Chunk 0x%08x not found",
                                             arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (port)),
                                             chunk_id);
			}
		}
	} else if (port->priv->event_id != NULL) {
		g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_NO_EVENT_IMPLEMENTATION,
			     "[%s] Events  not implemented",
                             arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (port)));
	} else {
		device = arv_gc_get_device (genicam);

		if (ARV_IS_DEVICE (device)) {
			/* For schema < 1.1.0 and length == 4, register write must be used instead of memory write.
			 * Only applies to GigE Vision devices. See Appendix 3 of Genicam 2.0 specification. */
			if (ARV_IS_GV_DEVICE (device) && _use_legacy_endianness_mechanism (port, length)) {
				guint32 value;

				/* For schema < 1.1.0, all registers are big endian. */
				value = *((guint32 *) buffer);
				value = GUINT32_FROM_BE (value);

				arv_device_write_register (device, address, value, error);
			} else
				arv_device_write_memory (device, address, length, buffer, error);
		} else {
			g_set_error (error, ARV_GC_ERROR, ARV_GC_ERROR_NO_DEVICE_SET,
				     "[%s] No device set",
                                     arv_gc_feature_node_get_name (ARV_GC_FEATURE_NODE (port)));
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
	gc_port->priv = arv_gc_port_get_instance_private (gc_port);
	gc_port->priv->chunk_id = 0;
	gc_port->priv->has_done_legacy_check = FALSE;
	gc_port->priv->has_legacy_infos = FALSE;
}

static void
_finalize (GObject *object)
{
	G_OBJECT_CLASS (arv_gc_port_parent_class)->finalize (object);
}

static void
arv_gc_port_class_init (ArvGcPortClass *this_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (this_class);
	ArvDomNodeClass *dom_node_class = ARV_DOM_NODE_CLASS (this_class);

	object_class->finalize = _finalize;
	dom_node_class->get_node_name = _get_node_name;
	dom_node_class->post_new_child = _post_new_child;
	dom_node_class->pre_remove_child = _pre_remove_child;
}
