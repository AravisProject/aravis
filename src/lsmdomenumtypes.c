
/* Generated data (by glib-mkenums) */

#include "lsmdomenumtypes.h"

/* enumerations from "lsmdebug.h" */
#include "lsmdebug.h"

GType
lsm_debug_level_get_type (void)
{
	static GType the_type = 0;

	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ LSM_DEBUG_LEVEL_NONE,
			  "LSM_DEBUG_LEVEL_NONE",
			  "none" },
			{ LSM_DEBUG_LEVEL_WARNING,
			  "LSM_DEBUG_LEVEL_WARNING",
			  "warning" },
			{ LSM_DEBUG_LEVEL_DEBUG,
			  "LSM_DEBUG_LEVEL_DEBUG",
			  "debug" },
			{ LSM_DEBUG_LEVEL_LOG,
			  "LSM_DEBUG_LEVEL_LOG",
			  "log" },
			{ LSM_DEBUG_LEVEL_COUNT,
			  "LSM_DEBUG_LEVEL_COUNT",
			  "count" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("LsmDebugLevel"),
				values);
	}
	return the_type;
}

/* enumerations from "lsmdomnode.h" */
#include "lsmdomnode.h"

GType
lsm_dom_node_type_get_type (void)
{
	static GType the_type = 0;

	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ LSM_DOM_NODE_TYPE_ELEMENT_NODE,
			  "LSM_DOM_NODE_TYPE_ELEMENT_NODE",
			  "element-node" },
			{ LSM_DOM_NODE_TYPE_ATTRIBUTE_NODE,
			  "LSM_DOM_NODE_TYPE_ATTRIBUTE_NODE",
			  "attribute-node" },
			{ LSM_DOM_NODE_TYPE_TEXT_NODE,
			  "LSM_DOM_NODE_TYPE_TEXT_NODE",
			  "text-node" },
			{ LSM_DOM_NODE_TYPE_CDATA_SECTION_NODE,
			  "LSM_DOM_NODE_TYPE_CDATA_SECTION_NODE",
			  "cdata-section-node" },
			{ LSM_DOM_NODE_TYPE_ENTITY_REFERENCE_NODE,
			  "LSM_DOM_NODE_TYPE_ENTITY_REFERENCE_NODE",
			  "entity-reference-node" },
			{ LSM_DOM_NODE_TYPE_ENTITY_NODE,
			  "LSM_DOM_NODE_TYPE_ENTITY_NODE",
			  "entity-node" },
			{ LSM_DOM_NODE_TYPE_PROCESSING_INSTRUCTION_NODE,
			  "LSM_DOM_NODE_TYPE_PROCESSING_INSTRUCTION_NODE",
			  "processing-instruction-node" },
			{ LSM_DOM_NODE_TYPE_COMMENT_NODE,
			  "LSM_DOM_NODE_TYPE_COMMENT_NODE",
			  "comment-node" },
			{ LSM_DOM_NODE_TYPE_DOCUMENT_NODE,
			  "LSM_DOM_NODE_TYPE_DOCUMENT_NODE",
			  "document-node" },
			{ LSM_DOM_NODE_TYPE_DOCUMENT_TYPE_NODE,
			  "LSM_DOM_NODE_TYPE_DOCUMENT_TYPE_NODE",
			  "document-type-node" },
			{ LSM_DOM_NODE_TYPE_DOCUMENT_FRAGMENT_NODE,
			  "LSM_DOM_NODE_TYPE_DOCUMENT_FRAGMENT_NODE",
			  "document-fragment-node" },
			{ LSM_DOM_NODE_TYPE_NOTATION_NODE,
			  "LSM_DOM_NODE_TYPE_NOTATION_NODE",
			  "notation-node" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("LsmDomNodeType"),
				values);
	}
	return the_type;
}


/* Generated data ends here */

