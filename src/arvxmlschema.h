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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author:
 * 	Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_XML_SCHEMA_H
#define ARV_XML_SCHEMA_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define ARV_XML_SCHEMA_ERROR arv_xml_schema_error_quark()

ARV_API GQuark		arv_xml_schema_error_quark	(void);

/**
 * ArvXmlSchemaError:
 * @ARV_XML_SCHEMA_ERROR_INVALID_STRUCTURE: invalid structure
 */

typedef enum {
	ARV_XML_SCHEMA_ERROR_INVALID_STRUCTURE
} ArvXmlSchemaError;

#define ARV_TYPE_XML_SCHEMA                  (arv_xml_schema_get_type ())
ARV_API G_DECLARE_FINAL_TYPE (ArvXmlSchema, arv_xml_schema, ARV, XML_SCHEMA, GObject)

ARV_API ArvXmlSchema *	arv_xml_schema_new_from_memory	(const char *buffer, size_t size);
ARV_API ArvXmlSchema *	arv_xml_schema_new_from_file	(GFile *file);
ARV_API ArvXmlSchema *	arv_xml_schema_new_from_path	(const char *path);
ARV_API gboolean 	arv_xml_schema_validate		(ArvXmlSchema *schema, const void *xml, size_t size,
							 int *line, int *column, GError **error);

G_END_DECLS

#endif
