#ifndef XML_ALIAS_H
#define XML_ALIAS_H

#include "membuffer.h"

struct s_xml_alias
{
	/*! name of DOC from root; e.g.: /foo/bar/mydesc.xml */
	membuffer name;
	/*! the XML document contents */
	membuffer doc;
	/*! . */
	time_t last_modified;
	/*! . */
	int *ct;
};

typedef struct s_xml_alias xml_alias_t;

#endif /* XML_ALIAS_H */
