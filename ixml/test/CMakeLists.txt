FILE (GLOB XML_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testdata/*.xml
)

ADD_EXECUTABLE (test-ixml-shared
	test_document.c
)

TARGET_LINK_LIBRARIES (test-ixml-shared
	PRIVATE ixml_shared
)

ADD_TEST (NAME test-ixml
	COMMAND test-ixml-shared ${XML_FILES}
)

SET_TESTS_PROPERTIES (test-ixml PROPERTIES
	ENVIRONMENT "PATH=$<TARGET_FILE_DIR:ixml_shared>\;%PATH%"
)

ADD_EXECUTABLE (test-ixml-static
	test_document.c
)

TARGET_LINK_LIBRARIES (test-ixml-static
	PRIVATE ixml_static
)

ADD_TEST (NAME test-ixml-static
	COMMAND test-ixml-static ${XML_FILES}
)
