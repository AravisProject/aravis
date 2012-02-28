/* Aravis - Digital camera library
 *
 * Copyright © 2009-2012 Emmanuel Pacaud
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

#ifndef ARV_GC_FEATURE_NODE_H
#define ARV_GC_FEATURE_NODE_H

#include <arvtypes.h>
#include <arvdomelement.h>

G_BEGIN_DECLS

#define ARV_TYPE_GC_NODE             (arv_gc_feature_node_get_type ())
#define ARV_GC_FEATURE_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GC_NODE, ArvGcFeatureNode))
#define ARV_GC_FEATURE_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GC_NODE, ArvGcFeatureNodeClass))
#define ARV_IS_GC_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GC_NODE))
#define ARV_IS_GC_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GC_NODE))
#define ARV_GC_FEATURE_NODE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GC_NODE, ArvGcFeatureNodeClass))

typedef struct _ArvGcFeatureNodePrivate ArvGcFeatureNodePrivate;
typedef struct _ArvGcFeatureNodeClass ArvGcFeatureNodeClass;

struct _ArvGcFeatureNode {
	ArvDomElement	base;

	ArvGcFeatureNodePrivate *priv;
};

struct _ArvGcFeatureNodeClass {
	ArvDomElementClass parent_class;

	void		(*set_attribute)		(ArvGcFeatureNode *gc_feature_node, const char *name, const char *value);
	void 		(*add_element)			(ArvGcFeatureNode *gc_feature_node, const char *name, const char *content,
							 const char **attributes);
	GType		(*get_value_type)		(ArvGcFeatureNode *gc_feature_node);
	gboolean 	(*can_add_child) 		(ArvGcFeatureNode *gc_feature_node, ArvGcFeatureNode *child);

	void		(*set_value_from_string)	(ArvGcFeatureNode *gc_feature_node, const char *string);
	const char *	(*get_value_as_string)		(ArvGcFeatureNode *gc_feature_node);
};

GType arv_gc_feature_node_get_type (void);

ArvGcFeatureNode * 	arv_gc_feature_node_new 			(void);
GType 		arv_gc_feature_node_get_value_type 		(ArvGcFeatureNode *gc_feature_node);
void		arv_gc_feature_node_set_value_from_string	(ArvGcFeatureNode *gc_feature_node, const char *string);
const char *	arv_gc_feature_node_get_value_as_string		(ArvGcFeatureNode *gc_feature_node);
void		arv_gc_feature_node_set_genicam			(ArvGcFeatureNode *gc_feature_node, ArvGc *genicam);
ArvGc * 	arv_gc_feature_node_get_genicam			(ArvGcFeatureNode *gc_feature_node);
const char *	arv_gc_feature_node_get_name			(ArvGcFeatureNode *gc_feature_node);
const char *	arv_gc_feature_node_get_tooltip			(ArvGcFeatureNode *gc_feature_node);
const char *	arv_gc_feature_node_get_description		(ArvGcFeatureNode *gc_feature_node);
gboolean	arv_gc_feature_node_is_available		(ArvGcFeatureNode *gc_feature_node);
void		arv_gc_feature_node_set_attribute 		(ArvGcFeatureNode *gc_feature_node, const char *name, const char *value);
void 		arv_gc_feature_node_add_element 		(ArvGcFeatureNode *gc_feature_node, const char *name, const char *content,
							 const char **attributes);
gboolean 	arv_gc_feature_node_can_add_child 		(ArvGcFeatureNode *gc_feature_node, ArvGcFeatureNode *child);
void 		arv_gc_feature_node_add_child 			(ArvGcFeatureNode *gc_feature_node, ArvGcFeatureNode *child);
const GSList *	arv_gc_feature_node_get_childs 			(ArvGcFeatureNode *gc_feature_node);
unsigned int 	arv_gc_feature_node_get_n_childs 		(ArvGcFeatureNode *gc_feature_node);
void 		arv_gc_feature_node_inc_modification_count 	(ArvGcFeatureNode *gc_feature_node);
gint 		arv_gc_feature_node_get_modification_count 	(ArvGcFeatureNode *gc_feature_node);

const char * 	arv_gc_feature_node_get_content (ArvGcFeatureNode *node);

G_END_DECLS

#endif
