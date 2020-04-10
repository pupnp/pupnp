#SUBDIRS 		= doc

ADD_SUBDIRECTORY (test)

ADD_LIBRARY (ixml_objects OBJECT
	src/attr.c
	src/document.c
	src/element.c
	src/inc/ixmlmembuf.h
	src/inc/ixmlparser.h
	src/ixml.c
	src/ixmldebug.c
	src/ixmlparser.c
	src/ixmlmembuf.c
	src/namedNodeMap.c
	src/node.c
	src/nodeList.c
)

IF (script_support)
	TARGET_COMPILE_DEFINITIONS (
		PRIVATE IXML_HAVE_SCRIPTSUPPORT
	)
ENDIF (script_support)

TARGET_INCLUDE_DIRECTORIES (ixml_objects
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc/
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/inc/
	PRIVATE ${UPNP_BINARY_DIR}/
	PRIVATE ${UPNP_SOURCE_DIR}/upnp/inc/
)

SET_PROPERTY (TARGET ixml_objects
	PROPERTY POSITION_INDEPENDENT_CODE 1
)

ADD_LIBRARY (ixml_shared SHARED
	$<TARGET_OBJECTS:ixml_objects>
)

SET_TARGET_PROPERTIES(ixml_shared PROPERTIES
	OUTPUT_NAME ixml
	VERSION ${IXML_VERSION}
	SOVERSION ${IXML_VERSION_MAJOR}
)

ADD_LIBRARY (ixml_static STATIC
	$<TARGET_OBJECTS:ixml_objects>
)

SET_TARGET_PROPERTIES(ixml_static PROPERTIES
	OUTPUT_NAME ixml
)

#upnpinclude_HEADERS	= \
#			inc/ixml.h \
#			inc/ixmldebug.h
#
#check_PROGRAMS          = test_document
#TESTS                   = test/test_document.sh
#
#test_document_SOURCES 	= test/test_document.c
#
#EXTRA_DIST		= test/test_document.sh test/testdata
#
#dist-hook:
#	rm -rf `find $(distdir)/test/testdata/ -type f \! -name '*ml' -print`
#
#clean-local:
#	@if [ -d bin ] ; then  rm -rf bin ; fi


