UPNP_Add_Unit_Test(test-upnp-list test_list.c)
UPNP_Add_Unit_Test(test-upnp-init test_init.c)
UPNP_Add_Unit_Test(test-upnp-log test_log.c)
UPNP_Add_Unit_Test(test-upnp-url test_url.c)

# On Solaris/illumos, getaddrinfo and freeaddrinfo live in libsocket (and
# sometimes libnsl) rather than in the C library.  find_library() probes for
# each library at configure time and stores the full path (or "<VAR>-NOTFOUND")
# in the named variable.  On Linux, macOS, and other platforms that bundle
# these symbols in libc the probe simply sets the variable to "<VAR>-NOTFOUND",
# which is treated as "not found" by the generator expressions below.
find_library(SOCKET_LIBRARY NAMES socket)
find_library(NSL_LIBRARY NAMES nsl)

if(UPNP_BUILD_SHARED)
	add_executable(
		test-upnp-parse-uri
		test_parse_uri.c
		test_parse_uri_impl.c
	)
	# Link against the Solaris socket/nsl libraries when they were found.
	#
	# On Windows, Winsock2 functions (htons, getaddrinfo, freeaddrinfo, inet_pton)
	# used in parse_hostport() are not linked automatically. We must explicitly link
	# against ws2_32 to resolve these symbols at link time.
	#
	# The generator expression $<$<BOOL:${VAR}>:${VAR}> works as follows:
	#   - The inner $<BOOL:${VAR}> evaluates to 1 when VAR is a non-empty,
	#     non-false value (i.e. when find_library found the library), and to
	#     0 otherwise (VAR is empty or ends with "-NOTFOUND").
	#   - The outer $<condition:value> expands to 'value' when condition is 1,
	#     and to nothing (an empty string) when condition is 0.
	# Together they add the library path only on platforms that have it.
	target_link_libraries(test-upnp-parse-uri PRIVATE upnp_shared
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
		$<$<PLATFORM_ID:Windows>:ws2_32>
	)
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
	target_link_libraries(test-upnp-parse-uri-static PRIVATE upnp_static
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
		$<$<PLATFORM_ID:Windows>:ws2_32>
	)
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
