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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author:
 * 	Emmanuel Pacaud <emmanuel@gnome.org>
 */

#ifndef ARV_XML_SCHEMA_H
#define ARV_XML_SCHEMA_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvtypes.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef enum {
	ARV_XML_SCHEMA_ERROR_INVALID_STRUCTURE
} ArvXmlSchemaError;

#define ARV_TYPE_XML_SCHEMA                  (arv_xml_schema_get_type ())
#define ARV_XML_SCHEMA(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_XML_SCHEMA, ArvXmlSchema))
#define ARV_IS_XML_SCHEMA(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_XML_SCHEMA))
#define ARV_XML_SCHEMA_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_XML_SCHEMA, ArvXmlSchemaClass))
#define ARV_IS_XML_SCHEMA_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_XML_SCHEMA))
#define ARV_XML_SCHEMA_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), ARV_TYPE_XML_SCHEMA, ArvXmlSchemaClass))

typedef struct _ArvXmlSchemaPrivate ArvXmlSchemaPrivate;
typedef struct _ArvXmlSchemaClass   ArvXmlSchemaClass;

struct _ArvXmlSchema
{
  GObject parent_instance;

  ArvXmlSchemaPrivate *priv;
};

struct _ArvXmlSchemaClass
{
  GObjectClass parent_class;
};

GType arv_xml_schema_get_type (void);

ArvXmlSchema * 		arv_xml_schema_new_from_file 	(GFile *file);
ArvXmlSchema * 		arv_xml_schema_new_from_path 	(const char *path);
gboolean 		arv_xml_schema_validate 	(ArvXmlSchema *schema, const void *xml, size_t size,
							 int *line, int *column, GError **error);

G_END_DECLS

#endif
