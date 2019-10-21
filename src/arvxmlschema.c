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

/**
 * SECTION: arvxmlschema
 * @short_description: XML Schema storage
 */

#include <arvxmlschema.h>
#include <libxml/parser.h>
#include <libxml/valid.h>
#include <libxml/xmlschemas.h>
#include <gio/gio.h>
#include <arvdebug.h>
#include <arvmisc.h>

typedef struct {
	char *xsd;
	size_t xsd_size;
	xmlSchemaParserCtxtPtr parser_ctxt;
	xmlSchemaPtr schema;
	xmlSchemaValidCtxtPtr valid_ctxt;
} ArvXmlSchemaPrivate;

struct _ArvXmlSchema
{
  GObject parent_instance;

  ArvXmlSchemaPrivate *priv;
};

struct _ArvXmlSchemaClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (ArvXmlSchema, arv_xml_schema, G_TYPE_OBJECT, G_ADD_PRIVATE (ArvXmlSchema))

static GQuark
arv_xml_schema_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0) {
		q = g_quark_from_static_string ("vmo-xml-schema-error-quark");
	}

	return q;
}

#define ARV_XML_SCHEMA_ERROR arv_xml_schema_error_quark ()

typedef struct {
	int line;
	int column;
	GError **error;
} XmlSchemaError;

static void
_structured_error_handler (void *ctx, xmlErrorPtr error)
{
	XmlSchemaError *schema_error = ctx;

	if (schema_error->error == NULL ||
	    *(schema_error->error) != NULL)
		return;

	schema_error->line = error->line;
	schema_error->column = error->int2;

	g_set_error_literal (schema_error->error,
			     ARV_XML_SCHEMA_ERROR,
			     ARV_XML_SCHEMA_ERROR_INVALID_STRUCTURE,
			     error->message);
}

gboolean
arv_xml_schema_validate (ArvXmlSchema *schema, const void *xml, size_t size, int *line, int *column, GError **error)
{
	xmlDocPtr xml_doc;
	int result;
	static GMutex mutex;
	XmlSchemaError schema_error = {0, 0, error};

	g_return_val_if_fail (ARV_IS_XML_SCHEMA (schema), FALSE);
	g_return_val_if_fail (xml != NULL && size > 0, FALSE);
	g_return_val_if_fail (schema->priv->valid_ctxt != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_mutex_lock (&mutex);

	xmlSetStructuredErrorFunc (&schema_error, _structured_error_handler);

	xml_doc = xmlParseMemory (xml, size);

	if (xml_doc != NULL) {
		xmlSchemaSetValidStructuredErrors (schema->priv->valid_ctxt, _structured_error_handler, &schema_error);
		xmlSchemaSetParserStructuredErrors (schema->priv->parser_ctxt, _structured_error_handler, &schema_error);

		result = xmlSchemaValidateDoc (schema->priv->valid_ctxt, xml_doc) == 0;

		xmlFreeDoc (xml_doc);
	} else
		result = FALSE;

	if (line != NULL)
		*line = schema_error.line;
	if (column != NULL)
		*column = schema_error.column;

	g_mutex_unlock (&mutex);

	return result;
}

ArvXmlSchema *
arv_xml_schema_new_from_file (GFile *file)
{
	ArvXmlSchema *schema;

	schema = g_object_new (ARV_TYPE_XML_SCHEMA, NULL);

	if (G_IS_FILE (file))
		g_file_load_contents (file, NULL, &schema->priv->xsd, &schema->priv->xsd_size, NULL, NULL);
	if (schema->priv->xsd != NULL)
		schema->priv->parser_ctxt = xmlSchemaNewMemParserCtxt(schema->priv->xsd, schema->priv->xsd_size);
	if (schema->priv->parser_ctxt != NULL)
		schema->priv->schema = xmlSchemaParse (schema->priv->parser_ctxt);
	if (schema->priv->schema != NULL)
		schema->priv->valid_ctxt = xmlSchemaNewValidCtxt (schema->priv->schema);
	else {
		char *uri = g_file_get_uri (file);
		arv_warning_dom ("[XmlSchema::new_from_file] Invalid xsd file '%s'", uri);
		g_free (uri);
	}

	return schema;
}

ArvXmlSchema *
arv_xml_schema_new_from_path (const char *path)
{
	ArvXmlSchema *schema;
	GFile *file;

	file = g_file_new_for_path (path);
	schema = arv_xml_schema_new_from_file (file);
	g_object_unref (file);

	return schema;
}

static void
arv_xml_schema_init (ArvXmlSchema *self)
{
	self->priv = arv_xml_schema_get_instance_private (self);
}

static void
_finalize (GObject *object)
{
	ArvXmlSchema *schema = ARV_XML_SCHEMA (object);

	g_clear_pointer (&schema->priv->valid_ctxt, xmlSchemaFreeValidCtxt);
	g_clear_pointer (&schema->priv->schema, xmlSchemaFree);
	g_clear_pointer (&schema->priv->parser_ctxt, xmlSchemaFreeParserCtxt);
	g_clear_pointer (&schema->priv->xsd, g_free);

	G_OBJECT_CLASS (arv_xml_schema_parent_class)->finalize (object);
}

static void
arv_xml_schema_class_init (ArvXmlSchemaClass *this_class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (this_class);

	gobject_class->finalize = _finalize;

	xmlLineNumbersDefault (1);
}
