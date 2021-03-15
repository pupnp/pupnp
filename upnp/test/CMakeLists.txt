if (WIN32)
	set (TEST_ENV "PATH=$<TARGET_FILE_DIR:upnp_shared>\;$<TARGET_FILE_DIR:ixml_shared>\;")
	string (APPEND TEST_ENV "$<TARGET_FILE_DIR:Threads::Threads>\;")
	string (APPEND TEST_ENV "%PATH%")
endif()

add_executable (test-upnp-init ${WIN_EXE}
	test_init.c
)

target_compile_definitions (test-upnp-init
	PRIVATE $<$<CONFIG:Debug>:UPNP_HAVE_DEBUG>
)

target_link_libraries (test-upnp-init
	upnp_shared
)

add_test (NAME test-upnp-init
	COMMAND test-upnp-init
)

set_tests_properties (test-upnp-init PROPERTIES
	ENVIRONMENT "${TEST_ENV}"
)

add_executable (test-upnp-init-static
	test_init.c
)

target_link_libraries (test-upnp-init-static
	upnp_static
)

target_compile_definitions (test-upnp-init-static
	PRIVATE $<$<CONFIG:Debug>:UPNP_HAVE_DEBUG>
)

add_test (NAME test-upnp-init-static
	COMMAND test-upnp-init-static
)

add_executable (test-upnp-list
	test_list.c
)

add_test (NAME test-upnp-list
	COMMAND test-upnp-list
)

set_tests_properties (test-upnp-list PROPERTIES
	ENVIRONMENT "${TEST_ENV}"
)

target_link_libraries (test-upnp-list
	upnp_shared
)

add_executable (test-upnp-list-static
	test_list.c
)

target_link_libraries (test-upnp-list-static
	upnp_static
)

add_test (NAME test-upnp-list-static
	COMMAND test-upnp-list-static
)

add_executable (test-upnp-log
	test_log.c
)

target_compile_definitions (test-upnp-log
	PRIVATE $<$<CONFIG:Debug>:UPNP_HAVE_DEBUG>
)

target_link_libraries (test-upnp-log
	upnp_shared
)

add_test (NAME test-upnp-log
	COMMAND test-upnp-log
)

set_tests_properties (test-upnp-log PROPERTIES
	ENVIRONMENT "${TEST_ENV}"
)

add_executable (test-upnp-log-static
	test_log.c
)

target_compile_definitions (test-upnp-log-static
	PRIVATE $<$<CONFIG:Debug>:UPNP_HAVE_DEBUG>
)

target_link_libraries (test-upnp-log-static
	upnp_static
)

add_test (NAME test-upnp-log-static
	COMMAND test-upnp-log-static
)

add_executable (test-upnp-url
	test_url.c
)

target_link_libraries (test-upnp-url
	upnp_shared
)

add_test (NAME test-upnp-url
	COMMAND test-upnp-url
)

set_tests_properties (test-upnp-url PROPERTIES
	ENVIRONMENT "${TEST_ENV}"
)

add_executable (test-upnp-url-static
	test_url.c
)

target_link_libraries (test-upnp-url-static
	upnp_static
)

add_test (NAME test-upnp-url-static
	COMMAND test-upnp-url-static
)
