UPNP_Add_Unit_Test(test-upnp-list test_list.c)
UPNP_Add_Unit_Test(test-upnp-init test_init.c)
UPNP_Add_Unit_Test(test-upnp-log test_log.c)
UPNP_Add_Unit_Test(test-upnp-url test_url.c)

if(UPNP_BUILD_SHARED)
	add_executable(
		test-upnp-parse-uri
		test_parse_uri.c
		test_parse_uri_impl.c
	)
	target_link_libraries(test-upnp-parse-uri PRIVATE upnp_shared)
	# On Windows, Winsock2 functions (htons, getaddrinfo, freeaddrinfo, inet_pton)
	# used in parse_hostport() are not linked automatically. We must explicitly link
	# against ws2_32 to resolve these symbols at link time.
	if(WIN32)
		target_link_libraries(test-upnp-parse-uri PRIVATE ws2_32)
	endif()
	target_include_directories(
		test-upnp-parse-uri
		PRIVATE ${PUPNP_BINARY_DIR}
		PRIVATE ${PUPNP_BINARY_DIR}/upnp/inc
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../inc
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/inc
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/threadutil
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ixml/inc
	)
	if(HAVE_MACRO_PREFIX_MAP)
		target_compile_options(
			test-upnp-parse-uri
			PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=.
		)
	endif()
	add_test(NAME test-upnp-parse-uri COMMAND test-upnp-parse-uri)
endif()

if(UPNP_BUILD_STATIC)
	add_executable(
		test-upnp-parse-uri-static
		test_parse_uri.c
		test_parse_uri_impl.c
	)
	target_link_libraries(test-upnp-parse-uri-static PRIVATE upnp_static)
	# On Windows, Winsock2 functions (htons, getaddrinfo, freeaddrinfo, inet_pton)
	# used in parse_hostport() are not linked automatically. We must explicitly link
	# against ws2_32 to resolve these symbols at link time.
	if(WIN32)
		target_link_libraries(test-upnp-parse-uri-static PRIVATE ws2_32)
	endif()
	target_include_directories(
		test-upnp-parse-uri-static
		PRIVATE ${PUPNP_BINARY_DIR}
		PRIVATE ${PUPNP_BINARY_DIR}/upnp/inc
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../inc
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/inc
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/threadutil
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ixml/inc
	)
	if(HAVE_MACRO_PREFIX_MAP)
		target_compile_options(
			test-upnp-parse-uri-static
			PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=.
		)
	endif()
	add_test(NAME test-upnp-parse-uri-static COMMAND test-upnp-parse-uri-static)
endif()
