ADD_EXECUTABLE (ixml_test
	EXCLUDE_FROM_ALL
	test_document.c
)

TARGET_INCLUDE_DIRECTORIES (ixml_test
	PRIVATE ${UPNP_SOURCE_DIR}/ixml/inc
	PRIVATE ${UPNP_SOURCE_DIR}/upnp/inc
)

TARGET_LINK_LIBRARIES (ixml_test
	ixml_shared
)
