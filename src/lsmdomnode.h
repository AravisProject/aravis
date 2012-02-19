/* Lasem
 *
 * Copyright © 2007-2008 Emmanuel Pacaud
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

#ifndef LSM_DOM_NODE_H
#define LSM_DOM_NODE_H

#include <lsmdomtypes.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef enum  {
	LSM_DOM_NODE_TYPE_ELEMENT_NODE = 1,
	LSM_DOM_NODE_TYPE_ATTRIBUTE_NODE,
	LSM_DOM_NODE_TYPE_TEXT_NODE,
	LSM_DOM_NODE_TYPE_CDATA_SECTION_NODE,
	LSM_DOM_NODE_TYPE_ENTITY_REFERENCE_NODE,
	LSM_DOM_NODE_TYPE_ENTITY_NODE,
	LSM_DOM_NODE_TYPE_PROCESSING_INSTRUCTION_NODE,
	LSM_DOM_NODE_TYPE_COMMENT_NODE,
	LSM_DOM_NODE_TYPE_DOCUMENT_NODE,
	LSM_DOM_NODE_TYPE_DOCUMENT_TYPE_NODE,
	LSM_DOM_NODE_TYPE_DOCUMENT_FRAGMENT_NODE,
	LSM_DOM_NODE_TYPE_NOTATION_NODE
} LsmDomNodeType;

#define LSM_TYPE_DOM_NODE             (lsm_dom_node_get_type ())
#define LSM_DOM_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), LSM_TYPE_DOM_NODE, LsmDomNode))
#define LSM_DOM_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), LSM_TYPE_DOM_NODE, LsmDomNodeClass))
#define LSM_IS_DOM_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LSM_TYPE_DOM_NODE))
#define LSM_IS_DOM_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), LSM_TYPE_DOM_NODE))
#define LSM_DOM_NODE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), LSM_TYPE_DOM_NODE, LsmDomNodeClass))

typedef struct _LsmDomNodeClass LsmDomNodeClass;

struct _LsmDomNode {
	GObject	object;

	LsmDomNode	*next_sibling;
	LsmDomNode	*previous_sibling;
	LsmDomNode	*parent_node;
	LsmDomNode	*first_child;
	LsmDomNode	*last_child;
};

struct _LsmDomNodeClass {
	GObjectClass parent_class;

	/* DOM node virtuals */

	const char* 		(*get_node_name) 	(LsmDomNode* self);
	const char* 		(*get_node_value) 	(LsmDomNode* self);
	void 			(*set_node_value) 	(LsmDomNode* self, const char* new_value);
	LsmDomNodeType 		(*get_node_type) 	(LsmDomNode* self);

	/* Validation virtuals */

	gboolean		(*can_append_child) 	(LsmDomNode *self, LsmDomNode *new_child);

	/* Implementation virtuals */

	void			(*post_new_child) 	(LsmDomNode *parent, LsmDomNode *child);
	void			(*pre_remove_child) 	(LsmDomNode *parent, LsmDomNode *child);
	void			(*changed)		(LsmDomNode *self);
	gboolean		(*child_changed)	(LsmDomNode *self, LsmDomNode *child);

	void			(*write_to_stream)	(LsmDomNode *self, GOutputStream *stream, GError **error);
};

GType lsm_dom_node_get_type (void);

const char * 		lsm_dom_node_get_node_name 		(LsmDomNode* self);
const char * 		lsm_dom_node_get_node_value 		(LsmDomNode* self);
void 			lsm_dom_node_set_node_value 		(LsmDomNode* self, const char* new_value);
LsmDomNodeType 		lsm_dom_node_get_node_type 		(LsmDomNode* self);
LsmDomNode * 		lsm_dom_node_get_parent_node 		(LsmDomNode* self);
LsmDomNodeList *	lsm_dom_node_get_child_nodes 		(LsmDomNode* self);
LsmDomNode * 		lsm_dom_node_get_first_child 		(LsmDomNode* self);
LsmDomNode * 		lsm_dom_node_get_last_child 		(LsmDomNode* self);
LsmDomNode * 		lsm_dom_node_get_previous_sibling 	(LsmDomNode* self);
LsmDomNode * 		lsm_dom_node_get_next_sibling 		(LsmDomNode* self);
#if 0
LsmDomNamedNodeMap * 	lsm_dom_node_get_attributes 		(LsmDomNode* self);
#endif
LsmDomNode * 		lsm_dom_node_insert_before		(LsmDomNode* self, LsmDomNode* new_child, LsmDomNode* ref_child);
LsmDomNode * 		lsm_dom_node_replace_child 		(LsmDomNode* self, LsmDomNode* new_child, LsmDomNode* old_child);
LsmDomNode * 		lsm_dom_node_append_child 		(LsmDomNode* self, LsmDomNode* new_child);
LsmDomNode * 		lsm_dom_node_remove_child 		(LsmDomNode* self, LsmDomNode* old_child);
gboolean 		lsm_dom_node_has_child_nodes 		(LsmDomNode* self);

void 			lsm_dom_node_changed 			(LsmDomNode *self);

LsmDomDocument *	lsm_dom_node_get_owner_document 	(LsmDomNode* self);

void			lsm_dom_node_write_to_stream		(LsmDomNode *self, GOutputStream *stream,
								 GError **error);

G_END_DECLS

#endif
