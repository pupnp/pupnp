if(UPNP_ENABLE_TESTING)
	enable_testing()
	add_subdirectory(test)
endif()

list(APPEND UPNP_SOURCES
	ixml/src/attr.c
	ixml/src/document.c
	ixml/src/element.c
	ixml/src/ixml.c
	ixml/src/ixmldebug.c
	ixml/src/ixmlmembuf.c
	ixml/src/ixmlparser.c
	ixml/src/namedNodeMap.c
	ixml/src/node.c
	ixml/src/nodeList.c
)

set(UPNP_HEADERS
	ixml/inc/ixml.h
	ixml/inc/ixmldebug.h)

set (UPNP_SOURCES ${UPNP_SOURCES} PARENT_SCOPE)
set (UPNP_HEADERS ${UPNP_HEADERS} PARENT_SCOPE)