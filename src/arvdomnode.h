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
 * Author:
 * 	Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_DOM_NODE_H
#define ARV_DOM_NODE_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef enum  {
	ARV_DOM_NODE_TYPE_ELEMENT_NODE = 1,
	ARV_DOM_NODE_TYPE_ATTRIBUTE_NODE,
	ARV_DOM_NODE_TYPE_TEXT_NODE,
	ARV_DOM_NODE_TYPE_CDATA_SECTION_NODE,
	ARV_DOM_NODE_TYPE_ENTITY_REFERENCE_NODE,
	ARV_DOM_NODE_TYPE_ENTITY_NODE,
	ARV_DOM_NODE_TYPE_PROCESSING_INSTRUCTION_NODE,
	ARV_DOM_NODE_TYPE_COMMENT_NODE,
	ARV_DOM_NODE_TYPE_DOCUMENT_NODE,
	ARV_DOM_NODE_TYPE_DOCUMENT_TYPE_NODE,
	ARV_DOM_NODE_TYPE_DOCUMENT_FRAGMENT_NODE,
	ARV_DOM_NODE_TYPE_NOTATION_NODE
} ArvDomNodeType;

#define ARV_TYPE_DOM_NODE             (arv_dom_node_get_type ())
ARV_API G_DECLARE_DERIVABLE_TYPE (ArvDomNode, arv_dom_node, ARV, DOM_NODE, GObject)

struct _ArvDomNodeClass {
	GObjectClass parent_class;

	/* DOM node virtuals */

	const char* 		(*get_node_name) 	(ArvDomNode* self);
	const char* 		(*get_node_value) 	(ArvDomNode* self);
	void 			(*set_node_value) 	(ArvDomNode* self, const char* new_value);
	ArvDomNodeType 		(*get_node_type) 	(ArvDomNode* self);

	/* Validation virtuals */

	gboolean		(*can_append_child) 	(ArvDomNode *self, ArvDomNode *new_child);

	/* Implementation virtuals */

	void			(*post_new_child) 	(ArvDomNode *parent, ArvDomNode *child);
	void			(*pre_remove_child) 	(ArvDomNode *parent, ArvDomNode *child);
	void			(*changed)		(ArvDomNode *self);
	gboolean		(*child_changed)	(ArvDomNode *self, ArvDomNode *child);
};

ARV_API const char *		arv_dom_node_get_node_name		(ArvDomNode *self);
ARV_API const char *		arv_dom_node_get_node_value		(ArvDomNode *self);
ARV_API void			arv_dom_node_set_node_value		(ArvDomNode *self, const char *new_value);
ARV_API ArvDomNodeType		arv_dom_node_get_node_type		(ArvDomNode *self);
ARV_API ArvDomNode *		arv_dom_node_get_parent_node		(ArvDomNode *self);
ARV_API ArvDomNodeList *	arv_dom_node_get_child_nodes		(ArvDomNode *self);
ARV_API ArvDomNode *		arv_dom_node_get_first_child		(ArvDomNode *self);
ARV_API ArvDomNode *		arv_dom_node_get_last_child		(ArvDomNode *self);
ARV_API ArvDomNode *		arv_dom_node_get_previous_sibling	(ArvDomNode *self);
ARV_API ArvDomNode *		arv_dom_node_get_next_sibling		(ArvDomNode *self);
#if 0
ARV_API ArvDomNamedNodeMap * 	arv_dom_node_get_attributes		(ArvDomNode *self);
#endif
ARV_API ArvDomNode *		arv_dom_node_insert_before		(ArvDomNode *self, ArvDomNode *new_child, ArvDomNode *ref_child);
ARV_API ArvDomNode *		arv_dom_node_replace_child		(ArvDomNode *self, ArvDomNode *new_child, ArvDomNode *old_child);
ARV_API ArvDomNode *		arv_dom_node_append_child		(ArvDomNode *self, ArvDomNode *new_child);
ARV_API ArvDomNode *		arv_dom_node_remove_child		(ArvDomNode *self, ArvDomNode *old_child);
ARV_API gboolean		arv_dom_node_has_child_nodes		(ArvDomNode *self);

ARV_API void			arv_dom_node_changed			(ArvDomNode *self);

ARV_API ArvDomDocument *	arv_dom_node_get_owner_document		(ArvDomNode *self);

G_END_DECLS

#endif
