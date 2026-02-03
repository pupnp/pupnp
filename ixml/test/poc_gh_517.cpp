extern "C" {
#include <ixml.h>
}

int main(void)
{
	// Create an empty document
	IXML_Document *doc = ixmlDocument_createDocument();
	if (!doc) {
		return 1;
	}

	// Create a root element
	IXML_Element *el = ixmlDocument_createElement(doc, "root");
	if (!el) {
		return 2;
	}

	// Step 1: Create a namespaced attribute on the element via the setter
	// API
	if (ixmlElement_setAttributeNS(el, "urn:test", "p:name", "v1") !=
		IXML_SUCCESS) {
		return 3;
	}

	// Step 2: Add an unrelated non-NS attribute (ensures element has >=2
	// attrs)
	IXML_Attr *a = ixmlDocument_createAttribute(doc, "other");
	if (!a) {
		return 4;
	}
	IXML_Attr *repl = nullptr;
	if (ixmlElement_setAttributeNode(el, a, &repl) != IXML_SUCCESS) {
		return 5;
	}
	if (repl) {
		return 6; // shouldn't replace here
	}

	// Step 3: Replace the previously set namespaced attribute using an
	// explicit Attr node
	IXML_Attr *ans =
		ixmlDocument_createAttributeNS(doc, "urn:test", "p:name");
	if (!ans) {
		return 7;
	}
	if (ixmlElement_setAttributeNode(el, ans, &repl) != IXML_SUCCESS) {
		return 8;
	}

	// Step 4: Free the replaced attribute now detached from the element.
	if (repl && repl->ownerElement == nullptr) {
		ixmlAttr_free(repl); // Crashes in ixmlNode_free
	}

	// Step 5: Finish by cleaning up allocations.
	if (el) {
		ixmlElement_free(el);
	}
	ixmlDocument_free(doc);

	return 0;
}
