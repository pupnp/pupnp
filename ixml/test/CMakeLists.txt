file (GLOB XML_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testdata/*.xml
)

	if (UPNP_BUILD_SHARED)
	add_executable (test-ixml-shared
		test_document.c
	)

	target_link_libraries (test-ixml-shared
		PRIVATE ixml_shared
	)

	add_test (NAME test-ixml
		COMMAND test-ixml-shared ${XML_FILES}
	)

	set_tests_properties (test-ixml PROPERTIES
		ENVIRONMENT "PATH=$<TARGET_FILE_DIR:ixml_shared>\;%PATH%"
	)
endif()

if (UPNP_BUILD_STATIC)
	add_executable (test-ixml-static
		test_document.c
	)

	target_link_libraries (test-ixml-static
		PRIVATE ixml_static
	)

	add_test (NAME test-ixml-static
		COMMAND test-ixml-static ${XML_FILES}
	)
endif()
