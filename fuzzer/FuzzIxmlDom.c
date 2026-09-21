#include "ixml.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DEPTH 64
#define MAX_NODES 4096

static void visit_attributes(IXML_Node *nodeptr)
{
	IXML_NamedNodeMap *map;
	unsigned long i;
	unsigned long len;

	map = ixmlNode_getAttributes(nodeptr);
	if (!map) {
		return;
	}

	len = ixmlNamedNodeMap_getLength(map);
	for (i = 0; i < len; i++) {
		IXML_Node *attr = ixmlNamedNodeMap_item(map, i);
		const DOMString name;

		if (!attr) {
			continue;
		}
		(void)ixmlNode_getNodeValue(attr);
		(void)ixmlNode_getLocalName(attr);
		(void)ixmlNode_getNamespaceURI(attr);
		(void)ixmlNode_getPrefix(attr);

		name = ixmlNode_getNodeName(attr);
		if (name) {
			(void)ixmlNamedNodeMap_getNamedItem(map, name);
		}
	}
	ixmlNamedNodeMap_free(map);
}

static void walk(IXML_Node *nodeptr, int depth, unsigned long *budget)
{
	IXML_Node *child;

	if (!nodeptr || depth > MAX_DEPTH || *budget == 0) {
		return;
	}
	(*budget)--;

	if (ixmlNode_hasAttributes(nodeptr)) {
		visit_attributes(nodeptr);
	}

	for (child = ixmlNode_getFirstChild(nodeptr); child;
			child = ixmlNode_getNextSibling(child)) {
		walk(child, depth + 1, budget);
	}
}

static void visit_child_list(IXML_Node *nodeptr)
{
	IXML_NodeList *children = ixmlNode_getChildNodes(nodeptr);
	unsigned long i;
	unsigned long len;

	if (!children) {
		return;
	}
	len = ixmlNodeList_length(children);
	for (i = 0; i < len; i++) {
		(void)ixmlNodeList_item(children, i);
	}
	ixmlNodeList_free(children);
}

static void query_document(IXML_Document *doc)
{
	IXML_NodeList *list;
	IXML_Node *first;

	list = ixmlDocument_getElementsByTagName(doc, "*");
	if (list) {
		unsigned long i;
		unsigned long len = ixmlNodeList_length(list);

		for (i = 0; i < len; i++) {
			IXML_Node *node = ixmlNodeList_item(list, i);
			IXML_Element *element = (IXML_Element *)node;
			const DOMString tag;

			if (!node) {
				continue;
			}
			tag = ixmlElement_getTagName(element);
			if (tag) {
				(void)ixmlElement_getAttribute(element, tag);
				(void)ixmlElement_hasAttribute(element, tag);
				(void)ixmlElement_getAttributeNode(element, tag);
			}
		}
		ixmlNodeList_free(list);
	}

	list = ixmlDocument_getElementsByTagNameNS(doc, "*", "*");
	if (list) {
		ixmlNodeList_free(list);
	}

	first = ixmlNode_getFirstChild((IXML_Node *)doc);
	if (first) {
		IXML_Node *clone = ixmlNode_cloneNode(first, 1);

		if (clone) {
			(void)ixmlNode_getFirstChild(clone);
			(void)ixmlNode_hasChildNodes(clone);
			ixmlNode_free(clone);
		}
	}
}

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
	IXML_Document *doc = NULL;
	char *xml;

	if (Size < 1 || Size > 65536) {
		return 0;
	}

	xml = malloc(Size + 1);
	if (!xml) {
		return 0;
	}
	memcpy(xml, Data, Size);
	xml[Size] = '\0';

	if (ixmlParseBufferEx(xml, &doc) == IXML_SUCCESS && doc) {
		unsigned long budget = MAX_NODES;

		walk((IXML_Node *)doc, 0, &budget);
		visit_child_list((IXML_Node *)doc);
		query_document(doc);
		ixmlDocument_free(doc);
	}

	free(xml);

	return 0;
}
