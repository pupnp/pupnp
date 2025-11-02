#include "ixml.h"

#include <stdio.h>

int main(void);

int main(void)
{
	int err = 0;
	int ixml_err = IXML_SUCCESS;
	IXML_Document *doc;
	IXML_Element *el;
	IXML_Attr *a;
	IXML_Attr *repl;
	const char *ns;
	const char *qname;
	IXML_Attr *ans;
	IXML_Attr *repl2;

	puts("\n\t1. Creates a document");
	ixml_err = ixmlDocument_createDocumentEx(&doc);
	if (ixml_err != IXML_SUCCESS) {
		err = 1;
		goto end_1;
	}
	puts("\t2. Adds one element to the document");
	el = ixmlDocument_createElement(doc, "E");
	if (!el) {
		err = 2;
		goto end_2;
	}

	puts("\t3. Create a non-namespaced attribute");
	a = ixmlDocument_createAttribute(doc, "x");
	if (!a) {
		err = 3;
		goto end_3;
	}
	puts("\t4. Attach a non-namespaced attribute");
	repl = NULL;
	ixml_err = ixmlElement_setAttributeNode(el, a, &repl);
	if (ixml_err != IXML_SUCCESS) {
		err = 4;
		goto end_3;
	}

	puts("\t5. Attach a namespaced attribute with the same name");
	ns = "urn:ns";
	qname = "p:x"; // prefix:local
	ans = ixmlDocument_createAttributeNS(doc, ns, qname);
	if (!ans) {
		err = 5;
		goto end_3;
	}

	puts("\t6. Crashes inside ixmlElement_setAttributeNodeNS\n"
	     "\t   while strcmp'ing attrNode->localName,\n"
	     "\t   which is NULL for the non-NS attr");
	repl2 = NULL;
	ixml_err = ixmlElement_setAttributeNodeNS(el, ans, &repl2);
	if (ixml_err != IXML_SUCCESS) {
		err = 6;
		goto end_3;
	}

end_3:
	puts("\t7. Free Element");
	ixmlElement_free(el);
end_2:
	puts("\t8. Free Document");
	ixmlDocument_free(doc);
end_1:
	printf("\t9. Finish, error code is %d, ixml error code is %d.\n",
		err,
		ixml_err);

	return err;
}
