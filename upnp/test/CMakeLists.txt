SET (TEST_ENV "PATH=$<TARGET_FILE_DIR:upnp_shared>\;$<TARGET_FILE_DIR:ixml_shared>\;")

IF (FETCHCONTENT_UPDATES_DISCONNECTED_PT4W)
	SET (TEST_ENV "${TEST_ENV}$<TARGET_FILE_DIR:Threads::Threads>\;")
ENDIF (FETCHCONTENT_UPDATES_DISCONNECTED_PT4W)

SET (TEST_ENV "${TEST_ENV}%PATH%")

ADD_EXECUTABLE (test-upnp-init ${WIN_EXE}
	test_init.c
)

TARGET_LINK_LIBRARIES (test-upnp-init
	upnp_shared
)

ADD_TEST (NAME test-upnp-init
	COMMAND test-upnp-init
)

SET_TESTS_PROPERTIES (test-upnp-init PROPERTIES
	ENVIRONMENT "${TEST_ENV}"
)

ADD_EXECUTABLE (test-upnp-init-static
	test_init.c
)

TARGET_LINK_LIBRARIES (test-upnp-init-static
	upnp_static
)

ADD_TEST (NAME test-upnp-init-static
	COMMAND test-upnp-init-static
)

ADD_EXECUTABLE (test-upnp-list
	test_list.c
)

ADD_TEST (NAME test-upnp-list
	COMMAND test-upnp-list
)

SET_TESTS_PROPERTIES (test-upnp-list PROPERTIES
	ENVIRONMENT "${TEST_ENV}"
)

TARGET_LINK_LIBRARIES (test-upnp-list
	upnp_shared
)

ADD_EXECUTABLE (test-upnp-list-static
	test_list.c
)

TARGET_LINK_LIBRARIES (test-upnp-list-static
	upnp_static
)

ADD_TEST (NAME test-upnp-list-static
	COMMAND test-upnp-list-static
)

ADD_EXECUTABLE (test-upnp-log
	test_log.c
)

TARGET_LINK_LIBRARIES (test-upnp-log
	upnp_shared
)

ADD_TEST (NAME test-upnp-log
	COMMAND test-upnp-log
)

SET_TESTS_PROPERTIES (test-upnp-log PROPERTIES
	ENVIRONMENT "${TEST_ENV}"
)

ADD_EXECUTABLE (test-upnp-log-static
	test_log.c
)

TARGET_LINK_LIBRARIES (test-upnp-log-static
	upnp_static
)

ADD_TEST (NAME test-upnp-log-static
	COMMAND test-upnp-log-static
)

ADD_EXECUTABLE (test-upnp-url
	test_url.c
)

TARGET_LINK_LIBRARIES (test-upnp-url
	upnp_shared
)

ADD_TEST (NAME test-upnp-url
	COMMAND test-upnp-url
)

SET_TESTS_PROPERTIES (test-upnp-url PROPERTIES
	ENVIRONMENT "${TEST_ENV}"
)

ADD_EXECUTABLE (test-upnp-url-static
	test_url.c
)

TARGET_LINK_LIBRARIES (test-upnp-url-static
	upnp_static
)

ADD_TEST (NAME test-upnp-url-static
	COMMAND test-upnp-url-static
)
