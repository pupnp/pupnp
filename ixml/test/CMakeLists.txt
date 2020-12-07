FILE (GLOB XML_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testdata/*.xml
)

	test_document.c
)

TARGET_INCLUDE_DIRECTORIES (test_ixml_shared
	PRIVATE ${UPNP_SOURCE_DIR}/ixml/inc
	PRIVATE ${UPNP_SOURCE_DIR}/upnp/inc
)

ADD_TEST (NAME test-ixml
	COMMAND test-ixml-shared ${XML_FILES}
TARGET_LINK_LIBRARIES (test_ixml_shared
	ixml_shared
)

ADD_EXECUTABLE (test_ixml_static
	EXCLUDE_FROM_ALL
	test_document.c
)

TARGET_INCLUDE_DIRECTORIES (test_ixml_static
	PRIVATE ${UPNP_SOURCE_DIR}/ixml/inc
	PRIVATE ${UPNP_SOURCE_DIR}/upnp/inc
)

ADD_TEST (NAME test-ixml-static
	COMMAND test-ixml-static ${XML_FILES}
TARGET_LINK_LIBRARIES (test_ixml_static
	ixml_static
)
